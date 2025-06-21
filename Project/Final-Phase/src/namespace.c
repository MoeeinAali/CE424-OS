#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include "../include/namespace.h"
#include "../include/utils.h"

// تنظیم namespace های مختلف
int setup_pid_namespace() {
    // namespace PID قبلاً با فراخوانی clone ایجاد شده است
    // در اینجا تنظیمات اضافی برای PID namespace انجام می‌شود
    
    // نصب procfs برای نمایش صحیح PID‌ها
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        log_error("خطا در نصب /proc");
        return -1;
    }
    
    return 0;
}

int setup_mount_namespace() {
    // ایزوله کردن mount namespace با تنظیم MS_PRIVATE
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        log_error("خطا در تنظیم MS_PRIVATE برای /");
        return -1;
    }
    
    return 0;
}

int setup_uts_namespace(const char *hostname) {
    // تنظیم hostname برای کانتینر
    if (sethostname(hostname, strlen(hostname)) != 0) {
        log_error("خطا در تنظیم hostname");
        return -1;
    }
    
    return 0;
}

int setup_user_namespace() {
    // تنظیم mapping برای user namespace
    // برای سادگی، کاربر داخل کانتینر به عنوان root اجرا می‌شود
    
    // نگاشت UID داخل کانتینر (0) به UID خارج از کانتینر
    int uid_map_fd = open("/proc/self/uid_map", O_WRONLY);
    if (uid_map_fd == -1) {
        log_error("خطا در باز کردن /proc/self/uid_map");
        return -1;
    }
    
    char uid_map[100];
    snprintf(uid_map, sizeof(uid_map), "0 %d 1", getuid());
    if (write(uid_map_fd, uid_map, strlen(uid_map)) != strlen(uid_map)) {
        log_error("خطا در نوشتن به uid_map");
        close(uid_map_fd);
        return -1;
    }
    close(uid_map_fd);
    
    // نگاشت GID داخل کانتینر (0) به GID خارج از کانتینر
    // ابتدا باید setgroups را غیرفعال کنیم
    int setgroups_fd = open("/proc/self/setgroups", O_WRONLY);
    if (setgroups_fd != -1) {
        if (write(setgroups_fd, "deny", 4) != 4) {
            log_error("خطا در نوشتن به setgroups");
            close(setgroups_fd);
            return -1;
        }
        close(setgroups_fd);
    }
    
    int gid_map_fd = open("/proc/self/gid_map", O_WRONLY);
    if (gid_map_fd == -1) {
        log_error("خطا در باز کردن /proc/self/gid_map");
        return -1;
    }
    
    char gid_map[100];
    snprintf(gid_map, sizeof(gid_map), "0 %d 1", getgid());
    if (write(gid_map_fd, gid_map, strlen(gid_map)) != strlen(gid_map)) {
        log_error("خطا در نوشتن به gid_map");
        close(gid_map_fd);
        return -1;
    }
    close(gid_map_fd);
    
    return 0;
}

int setup_network_namespace() {
    // در اینجا می‌توان تنظیمات شبکه برای کانتینر را انجام داد
    // مانند راه‌اندازی یک interface loopback

    // فعال‌سازی interface loopback
    system("ip link set lo up");

    return 0;
}

int setup_ipc_namespace() {
    // namespace IPC قبلاً با فراخوانی clone ایجاد شده است
    // در اینجا می‌توان تنظیمات اضافی برای IPC namespace انجام داد
    
    return 0;
}

// راه‌اندازی تمام namespace‌ها
int setup_namespaces(container_config_t *config) {
    // تنظیم mount namespace
    if (setup_mount_namespace() != 0) {
        return -1;
    }
    
    // تنظیم uts namespace (hostname)
    if (setup_uts_namespace(config->name) != 0) {
        return -1;
    }
    
    // تنظیم user namespace
    if (setup_user_namespace() != 0) {
        return -1;
    }
    
    // تنظیم network namespace
    if (setup_network_namespace() != 0) {
        return -1;
    }
    
    // تنظیم ipc namespace
    if (setup_ipc_namespace() != 0) {
        return -1;
    }
    
    // تنظیم pid namespace باید بعد از chroot انجام شود
    
    return 0;
}

// پیوستن به یک namespace موجود
int join_namespace(const char *ns_path, int nstype) {
    int fd = open(ns_path, O_RDONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن namespace: %s", ns_path);
        return -1;
    }
    
    if (setns(fd, nstype) == -1) {
        log_error("خطا در پیوستن به namespace");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// دریافت مسیر namespace برای یک فرآیند
int get_namespace_path(pid_t pid, int nstype, char *buf, size_t bufsize) {
    const char *ns_name;
    
    // تعیین نام namespace بر اساس نوع
    switch (nstype) {
        case CLONE_NEWPID:
            ns_name = "pid";
            break;
        case CLONE_NEWNS:
            ns_name = "mnt";
            break;
        case CLONE_NEWUTS:
            ns_name = "uts";
            break;
        case CLONE_NEWUSER:
            ns_name = "user";
            break;
        case CLONE_NEWNET:
            ns_name = "net";
            break;
        case CLONE_NEWIPC:
            ns_name = "ipc";
            break;
        default:
            log_error("نوع namespace نامعتبر");
            return -1;
    }
    
    // ساخت مسیر namespace
    if (pid == 0) {
        snprintf(buf, bufsize, "/proc/self/ns/%s", ns_name);
    } else {
        snprintf(buf, bufsize, "/proc/%d/ns/%s", pid, ns_name);
    }
    
    return 0;
}