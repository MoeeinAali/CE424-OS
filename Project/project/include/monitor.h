#ifndef MONITOR_H
#define MONITOR_H

#include "container.h"
#include <stdint.h>

// راه‌اندازی مانیتورینگ eBPF
int monitor_init();

// پاک‌سازی مانیتورینگ
int monitor_cleanup();

// شروع مانیتورینگ یک کانتینر
int monitor_container(container_config_t *config);

// توقف مانیتورینگ یک کانتینر
int monitor_stop_container(container_config_t *config);

// گزارش‌گیری از وضعیت منابع
int monitor_get_resource_usage(container_config_t *config, uint64_t *cpu_usage, uint64_t *mem_usage, uint64_t *io_read, uint64_t *io_write);

// ثبت رویدادهای namespace
int monitor_namespace_events();

// ثبت رویدادهای cgroup
int monitor_cgroup_events();

// ثبت فراخوانی‌های سیستمی
int monitor_syscall_events();

// ذخیره‌سازی لاگ‌های eBPF
int monitor_save_logs(const char *container_id);

#endif /* MONITOR_H */