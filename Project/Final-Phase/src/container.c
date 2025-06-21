#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include "../include/container.h"
#include "../include/namespace.h"
#include "../include/cgroup.h"
#include "../include/filesystem.h"
#include "../include/monitor.h"
#include "../include/utils.h"

// ایجاد مدیریت‌کننده کانتینر
container_manager_t* container_manager_create(int max_containers) {
    container_manager_t *manager = malloc(sizeof(container_manager_t));
    if (!manager) {
        log_error("خطا در تخصیص حافظه برای مدیریت‌کننده کانتینر");
        return NULL;
    }

    manager->containers = malloc(sizeof(container_config_t) * max_containers);
    if (!manager->containers) {
        log_error("خطا در تخصیص حافظه برای آرایه کانتینرها");
        free(manager);
        return NULL;
    }

    manager->max_containers = max_containers;
    manager->container_count = 0;

    // ایجاد دایرکتوری‌های مورد نیاز
    create_directory("/var/lib/simplecontainer", 0755);
    create_directory("/var/lib/simplecontainer/rootfs", 0755);
    create_directory("/var/lib/simplecontainer/overlays", 0755);
    create_directory("/var/lib/simplecontainer/logs", 0755);

    return manager;
}

// آزاد‌سازی منابع مدیریت‌کننده کانتینر
void container_manager_destroy(container_manager_t *manager) {
    if (!manager) return;

    // توقف همه کانتینرهای فعال
    for (int i = 0; i < manager->container_count; i++) {
        if (manager->containers[i].running) {
            container_stop(manager, manager->containers[i].id);
        }
    }

    free(manager->containers);
    free(manager);
}

// ایجاد کانتینر جدید
int container_create(container_manager_t *manager, const char *name, const char *binary_path, char **args, int argc) {
    if (manager->container_count >= manager->max_containers) {
        log_error("تعداد کانتینرها به حداکثر رسیده است");
        return -1;
    }

    container_config_t *config = &manager->containers[manager->container_count];
    memset(config, 0, sizeof(container_config_t));

    // تنظیم شناسه و نام
    generate_unique_id(config->id, sizeof(config->id));
    strncpy(config->name, name, sizeof(config->name) - 1);
    
    // تنظیم مسیرها
    snprintf(config->rootfs, sizeof(config->rootfs), "/var/lib/simplecontainer/rootfs/%s", config->id);
    snprintf(config->overlay_workdir, sizeof(config->overlay_workdir), "/var/lib/simplecontainer/overlays/%s", config->id);
    strncpy(config->binary_path, binary_path, sizeof(config->binary_path) - 1);
    
    // تنظیم آرگومان‌ها
    config->args = malloc((argc + 1) * sizeof(char*));
    if (!config->args) {
        log_error("خطا در تخصیص حافظه برای آرگومان‌ها");
        return -1;
    }
    
    for (int i = 0; i < argc; i++) {
        config->args[i] = strdup(args[i]);
    }
    config->args[argc] = NULL;
    config->argc = argc;
    
    // تنظیم مقادیر پیش‌فرض محدودیت منابع
    config->mem_limit_bytes = 512 * 1024 * 1024;  // 512 MB
    config->cpu_shares = 1024;                     // سهم استاندارد
    config->cpu_affinity = -1;                     // بدون تخصیص CPU خاص
    config->io_weight = 100;                       // وزن استاندارد IO
    
    config->running = false;
    config->container_pid = -1;
    
    // تنظیم مسیر cgroup
    snprintf(config->cgroup_path, sizeof(config->cgroup_path), "/simplecontainer/%s", config->id);
    
    // آماده‌سازی فایل‌سیستم کانتینر
    if (setup_container_rootfs(config) != 0) {
        log_error("خطا در آماده‌سازی فایل‌سیستم کانتینر");
        free(config->args);
        return -1;
    }
    
    manager->container_count++;
    log_message("کانتینر با شناسه %s ایجاد شد", config->id);
    
    return 0;
}

// اجرای فرآیند کانتینر
static int container_process(void *arg) {
    container_config_t *config = (container_config_t *)arg;
    
    // تنظیم namespace‌ها
    if (setup_namespaces(config) != 0) {
        log_error("خطا در تنظیم namespace‌ها");
        return EXIT_FAILURE;
    }
    
    // تنظیم فایل‌سیستم ریشه
    if (do_chroot(config->rootfs) != 0) {
        log_error("خطا در تنظیم chroot");
        return EXIT_FAILURE;
    }
    
    // تغییر دایرکتوری به ریشه
    if (chdir("/") != 0) {
        log_error("خطا در تغییر دایرکتوری به /");
        return EXIT_FAILURE;
    }
    
    // نصب فایل‌سیستم‌های ضروری
    if (mount_essential_filesystems(config) != 0) {
        log_error("خطا در نصب فایل‌سیستم‌های ضروری");
        return EXIT_FAILURE;
    }
    
    // اجرای برنامه کاربر
    execv(config->binary_path, config->args);
    
    // اگر به اینجا برسیم، execv با خطا مواجه شده است
    log_error("خطا در اجرای برنامه: %s", config->binary_path);
    return EXIT_FAILURE;
}

// شروع کانتینر
int container_start(container_manager_t *manager, const char *container_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    if (config->running) {
        log_error("کانتینر %s در حال اجرا است", container_id);
        return -1;
    }
    
    // تنظیم cgroup
    if (cgroup_setup(config) != 0) {
        log_error("خطا در تنظیم cgroup");
        return -1;
    }
    
    // تنظیم محدودیت‌های منابع
    cgroup_set_memory_limit(config, config->mem_limit_bytes);
    cgroup_set_cpu_shares(config, config->cpu_shares);
    if (config->cpu_affinity >= 0) {
        cgroup_set_cpu_affinity(config, config->cpu_affinity);
    }
    cgroup_set_io_weight(config, config->io_weight);
    
    // اندازه استک برای فرآیند فرزند
    const int stack_size = 8 * 1024 * 1024;  // 8 MB
    void *stack = malloc(stack_size);
    if (!stack) {
        log_error("خطا در تخصیص حافظه برای استک");
        return -1;
    }
    
    // ایجاد فرآیند کانتینر با clone
    pid_t pid = clone(container_process, stack + stack_size,
                      CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWIPC,
                      config);
    
    if (pid == -1) {
        log_error("خطا در ایجاد فرآیند کانتینر");
        free(stack);
        return -1;
    }
    
    // اضافه کردن PID به cgroup
    char pid_str[16];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    char cgroup_procs_path[512];
    snprintf(cgroup_procs_path, sizeof(cgroup_procs_path), "/sys/fs/cgroup/unified%s/cgroup.procs", config->cgroup_path);
    
    int fd = open(cgroup_procs_path, O_WRONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن فایل cgroup.procs");
        kill(pid, SIGKILL);
        free(stack);
        return -1;
    }
    
    write(fd, pid_str, strlen(pid_str));
    close(fd);
    
    // به‌روزرسانی وضعیت کانتینر
    config->container_pid = pid;
    config->running = true;
    
    // شروع مانیتورینگ
    monitor_container(config);
    
    free(stack);
    log_message("کانتینر %s با PID %d شروع شد", container_id, pid);
    
    return 0;
}

// توقف کانتینر
int container_stop(container_manager_t *manager, const char *container_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    if (!config->running) {
        log_error("کانتینر %s در حال اجرا نیست", container_id);
        return -1;
    }
    
    // ارسال سیگنال SIGTERM به فرآیند کانتینر
    if (kill(config->container_pid, SIGTERM) == -1) {
        log_error("خطا در ارسال سیگنال SIGTERM به کانتینر");
        return -1;
    }
    
    // انتظار برای پایان کانتینر (با تایم‌اوت)
    int status;
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < 10) {  // تایم‌اوت 10 ثانیه
        if (waitpid(config->container_pid, &status, WNOHANG) != 0) {
            break;
        }
        usleep(100000);  // 100 میلی‌ثانیه
    }
    
    // اگر هنوز در حال اجراست، با SIGKILL متوقف کن
    if (kill(config->container_pid, 0) == 0) {
        log_message("ارسال SIGKILL به کانتینر %s", container_id);
        kill(config->container_pid, SIGKILL);
        waitpid(config->container_pid, &status, 0);
    }
    
    // توقف مانیتورینگ
    monitor_stop_container(config);
    
    // پاک‌سازی cgroup
    cgroup_cleanup(config);
    
    // به‌روزرسانی وضعیت کانتینر
    config->container_pid = -1;
    config->running = false;
    
    log_message("کانتینر %s متوقف شد", container_id);
    
    return 0;
}

// بررسی وضعیت کانتینر
int container_status(container_manager_t *manager, const char *container_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    printf("شناسه: %s\n", config->id);
    printf("نام: %s\n", config->name);
    printf("وضعیت: %s\n", config->running ? "در حال اجرا" : "متوقف");
    
    if (config->running) {
        printf("PID: %d\n", config->container_pid);
        
        // دریافت مصرف منابع
        uint64_t cpu_usage, mem_usage, io_read, io_write;
        if (monitor_get_resource_usage(config, &cpu_usage, &mem_usage, &io_read, &io_write) == 0) {
            printf("مصرف CPU: %lu%%\n", cpu_usage);
            printf("مصرف حافظه: %lu MB\n", mem_usage / (1024 * 1024));
            printf("خواندن I/O: %lu KB\n", io_read / 1024);
            printf("نوشتن I/O: %lu KB\n", io_write / 1024);
        }
    }
    
    printf("محدودیت حافظه: %lu MB\n", config->mem_limit_bytes / (1024 * 1024));
    printf("سهم CPU: %lu\n", config->cpu_shares);
    printf("تخصیص CPU: %s\n", config->cpu_affinity >= 0 ? 
           (char[]){config->cpu_affinity + '0', '\0'} : "تمام هسته‌ها");
    printf("وزن I/O: %lu\n", config->io_weight);
    
    return 0;
}

// نمایش لیست کانتینرها
int container_list(container_manager_t *manager) {
    printf("تعداد کانتینرها: %d\n", manager->container_count);
    printf("--------------------------------------\n");
    printf("%-10s %-20s %-10s %-10s\n", "شناسه", "نام", "وضعیت", "PID");
    printf("--------------------------------------\n");
    
    for (int i = 0; i < manager->container_count; i++) {
        container_config_t *config = &manager->containers[i];
        printf("%-10s %-20s %-10s %-10d\n", 
               config->id, 
               config->name, 
               config->running ? "اجرا" : "متوقف", 
               config->running ? config->container_pid : -1);
    }
    
    return 0;
}

// یافتن کانتینر با شناسه
container_config_t* container_find_by_id(container_manager_t *manager, const char *container_id) {
    for (int i = 0; i < manager->container_count; i++) {
        if (strcmp(manager->containers[i].id, container_id) == 0) {
            return &manager->containers[i];
        }
    }
    return NULL;
}

// تنظیم محدودیت حافظه
int container_set_memory_limit(container_manager_t *manager, const char *container_id, uint64_t mem_limit_bytes) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->mem_limit_bytes = mem_limit_bytes;
    
    // اگر کانتینر در حال اجراست، محدودیت را اعمال کن
    if (config->running) {
        return cgroup_set_memory_limit(config, mem_limit_bytes);
    }
    
    return 0;
}

// تنظیم سهم CPU
int container_set_cpu_shares(container_manager_t *manager, const char *container_id, uint64_t cpu_shares) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->cpu_shares = cpu_shares;
    
    // اگر کانتینر در حال اجراست، محدودیت را اعمال کن
    if (config->running) {
        return cgroup_set_cpu_shares(config, cpu_shares);
    }
    
    return 0;
}

// تنظیم تخصیص CPU
int container_set_cpu_affinity(container_manager_t *manager, const char *container_id, int cpu_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->cpu_affinity = cpu_id;
    
    // اگر کانتینر در حال اجراست، محدودیت را اعمال کن
    if (config->running) {
        return cgroup_set_cpu_affinity(config, cpu_id);
    }
    
    return 0;
}

// تنظیم وزن I/O
int container_set_io_weight(container_manager_t *manager, const char *container_id, uint64_t io_weight) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->io_weight = io_weight;
    
    // اگر کانتینر در حال اجراست، محدودیت را اعمال کن
    if (config->running) {
        return cgroup_set_io_weight(config, io_weight);
    }
    
    return 0;
}