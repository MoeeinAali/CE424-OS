#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>
#include <stdbool.h>

// ساختار مشخصات کانتینر
typedef struct {
    char id[64];                // شناسه منحصر به فرد
    char name[256];             // نام کانتینر
    char rootfs[512];           // مسیر فایل‌سیستم ریشه
    char overlay_workdir[512];  // دایرکتوری کاری overlayfs
    char binary_path[512];      // مسیر باینری اجرایی
    char **args;                // آرگومان‌های اجرایی
    int argc;                   // تعداد آرگومان‌ها
    
    // محدودیت‌های منابع
    uint64_t mem_limit_bytes;   // محدودیت حافظه (بایت)
    uint64_t cpu_shares;        // سهم CPU
    int cpu_affinity;           // تخصیص CPU مشخص (-1 برای غیرفعال)
    uint64_t io_weight;         // وزن I/O
    
    pid_t container_pid;        // PID فرآیند اصلی کانتینر
    bool running;               // وضعیت اجرا
    char cgroup_path[512];      // مسیر cgroup
} container_config_t;

// ساختار‌ مدیریت کانتینر
typedef struct {
    container_config_t *containers;
    int max_containers;
    int container_count;
} container_manager_t;

// توابع مدیریت کانتینر
container_manager_t* container_manager_create(int max_containers);
void container_manager_destroy(container_manager_t *manager);

// توابع عملیاتی کانتینر
int container_create(container_manager_t *manager, const char *name, const char *binary_path, char **args, int argc);
int container_start(container_manager_t *manager, const char *container_id);
int container_stop(container_manager_t *manager, const char *container_id);
int container_status(container_manager_t *manager, const char *container_id);
int container_list(container_manager_t *manager);

// تنظیم محدودیت‌های منابع
int container_set_memory_limit(container_manager_t *manager, const char *container_id, uint64_t mem_limit_bytes);
int container_set_cpu_shares(container_manager_t *manager, const char *container_id, uint64_t cpu_shares);
int container_set_cpu_affinity(container_manager_t *manager, const char *container_id, int cpu_id);
int container_set_io_weight(container_manager_t *manager, const char *container_id, uint64_t io_weight);

// مدیریت داخلی
container_config_t* container_find_by_id(container_manager_t *manager, const char *container_id);
int container_setup_environment(container_config_t *config);
int container_cleanup_environment(container_config_t *config);

#endif /* CONTAINER_H */