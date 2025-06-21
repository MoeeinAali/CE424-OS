#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/cgroup.h"
#include "../include/utils.h"

// راه‌اندازی cgroup برای کانتینر
int cgroup_setup(container_config_t *config) {
    // اطمینان از وجود دایرکتوری پایه cgroup
    if (!directory_exists(CGROUP_BASE_PATH)) {
        if (create_directory(CGROUP_BASE_PATH, 0755) != 0) {
            log_error("خطا در ایجاد دایرکتوری پایه cgroup");
            return -1;
        }
    }
    
    // ساخت مسیر کامل cgroup برای این کانتینر
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    // ایجاد دایرکتوری cgroup برای کانتینر
    if (create_directory(cgroup_full_path, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری cgroup برای کانتینر");
        return -1;
    }
    
    // تنظیم مسیر cgroup در پیکربندی کانتینر
    snprintf(config->cgroup_path, sizeof(config->cgroup_path), "/simplecontainer/%s", config->id);
    
    return 0;
}

// پاک‌سازی cgroup
int cgroup_cleanup(container_config_t *config) {
    // ساخت مسیر کامل cgroup
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    // حذف دایرکتوری cgroup
    if (remove_directory(cgroup_full_path) != 0) {
        log_error("خطا در حذف دایرکتوری cgroup");
        return -1;
    }
    
    return 0;
}

// نوشتن یک مقدار در فایل cgroup
static int write_cgroup_file(const char *cgroup_path, const char *file, const char *value) {
    // ساخت مسیر کامل فایل
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", cgroup_path, file);
    
    // باز کردن فایل
    int fd = open(file_path, O_WRONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن فایل cgroup: %s", file_path);
        return -1;
    }
    
    // نوشتن مقدار
    ssize_t bytes_written = write(fd, value, strlen(value));
    close(fd);
    
    if (bytes_written != strlen(value)) {
        log_error("خطا در نوشتن به فایل cgroup: %s", file_path);
        return -1;
    }
    
    return 0;
}

// خواندن یک مقدار از فایل cgroup
static int read_cgroup_file(const char *cgroup_path, const char *file, char *buffer, size_t buffer_size) {
    // ساخت مسیر کامل فایل
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", cgroup_path, file);
    
    // باز کردن فایل
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن فایل cgroup: %s", file_path);
        return -1;
    }
    
    // خواندن مقدار
    ssize_t bytes_read = read(fd, buffer, buffer_size - 1);
    close(fd);
    
    if (bytes_read <= 0) {
        log_error("خطا در خواندن از فایل cgroup: %s", file_path);
        return -1;
    }
    
    buffer[bytes_read] = '\0';
    return 0;
}

// تنظیم محدودیت حافظه
int cgroup_set_memory_limit(container_config_t *config, uint64_t limit_bytes) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    // فعال‌سازی محدودیت حافظه
    if (write_cgroup_file(cgroup_full_path, "memory.max", "1") != 0) {
        log_error("خطا در فعال‌سازی محدودیت حافظه");
        return -1;
    }
    
    // تنظیم محدودیت حافظه
    char limit_str[32];
    snprintf(limit_str, sizeof(limit_str), "%lu", limit_bytes);
    
    if (write_cgroup_file(cgroup_full_path, "memory.max", limit_str) != 0) {
        log_error("خطا در تنظیم محدودیت حافظه");
        return -1;
    }
    
    return 0;
}

// تنظیم سهم CPU
int cgroup_set_cpu_shares(container_config_t *config, uint64_t shares) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    // تنظیم سهم CPU
    char shares_str[32];
    snprintf(shares_str, sizeof(shares_str), "%lu", shares);
    
    if (write_cgroup_file(cgroup_full_path, "cpu.weight", shares_str) != 0) {
        log_error("خطا در تنظیم سهم CPU");
        return -1;
    }
    
    return 0;
}

// تنظیم تخصیص CPU
int cgroup_set_cpu_affinity(container_config_t *config, int cpu_id) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    // ساخت ماسک CPU
    char cpuset_str[256] = {0};
    snprintf(cpuset_str, sizeof(cpuset_str), "%d", cpu_id);
    
    if (write_cgroup_file(cgroup_full_path, "cpuset.cpus", cpuset_str) != 0) {
        log_error("خطا در تنظیم تخصیص CPU");
        return -1;
    }
    
    return 0;
}

// تنظیم وزن I/O
int cgroup_set_io_weight(container_config_t *config, uint64_t weight) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    // تنظیم وزن I/O
    char weight_str[32];
    snprintf(weight_str, sizeof(weight_str), "%lu", weight);
    
    if (write_cgroup_file(cgroup_full_path, "io.weight", weight_str) != 0) {
        log_error("خطا در تنظیم وزن I/O");
        return -1;
    }
    
    return 0;
}

// توقف موقت کانتینر با استفاده از freezer
int cgroup_freeze_container(container_config_t *config) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    if (write_cgroup_file(cgroup_full_path, "cgroup.freeze", "1") != 0) {
        log_error("خطا در متوقف کردن موقت کانتینر");
        return -1;
    }
    
    return 0;
}

// ادامه اجرای کانتینر
int cgroup_unfreeze_container(container_config_t *config) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    if (write_cgroup_file(cgroup_full_path, "cgroup.freeze", "0") != 0) {
        log_error("خطا در ادامه اجرای کانتینر");
        return -1;
    }
    
    return 0;
}

// دریافت مصرف حافظه
int cgroup_get_memory_usage(container_config_t *config, uint64_t *usage) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    char buffer[128];
    if (read_cgroup_file(cgroup_full_path, "memory.current", buffer, sizeof(buffer)) != 0) {
        log_error("خطا در خواندن مصرف حافظه");
        return -1;
    }
    
    *usage = strtoull(buffer, NULL, 10);
    return 0;
}

// دریافت مصرف CPU
int cgroup_get_cpu_usage(container_config_t *config, uint64_t *usage) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    char buffer[128];
    if (read_cgroup_file(cgroup_full_path, "cpu.stat", buffer, sizeof(buffer)) != 0) {
        log_error("خطا در خواندن مصرف CPU");
        return -1;
    }
    
    // پارس کردن خروجی cpu.stat
    char *usage_line = strstr(buffer, "usage_usec");
    if (!usage_line) {
        log_error("خطا در پارس کردن مصرف CPU");
        return -1;
    }
    
    *usage = strtoull(usage_line + 11, NULL, 10) / 10000; // تبدیل به درصد
    return 0;
}

// دریافت مصرف I/O
int cgroup_get_io_usage(container_config_t *config, uint64_t *read_bytes, uint64_t *write_bytes) {
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    char buffer[1024];
    if (read_cgroup_file(cgroup_full_path, "io.stat", buffer, sizeof(buffer)) != 0) {
        log_error("خطا در خواندن مصرف I/O");
        return -1;
    }
    
    // پارس کردن خروجی io.stat
    // فرمت خروجی: major:minor rbytes=X wbytes=Y ...
    
    *read_bytes = 0;
    *write_bytes = 0;
    
    char *line = strtok(buffer, "\n");
    while (line) {
        char *rbytes_str = strstr(line, "rbytes=");
        char *wbytes_str = strstr(line, "wbytes=");
        
        if (rbytes_str) {
            *read_bytes += strtoull(rbytes_str + 7, NULL, 10);
        }
        
        if (wbytes_str) {
            *write_bytes += strtoull(wbytes_str + 7, NULL, 10);
        }
        
        line = strtok(NULL, "\n");
    }
    
    return 0;
}