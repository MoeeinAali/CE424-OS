#ifndef CGROUP_H
#define CGROUP_H

#include <stdint.h>
#include "container.h"

// مسیر پایه cgroup v2
#define CGROUP_BASE_PATH "/sys/fs/cgroup"

// راه‌اندازی cgroup برای کانتینر
int cgroup_setup(container_config_t *config);

// پاک‌سازی cgroup
int cgroup_cleanup(container_config_t *config);

// تنظیم محدودیت حافظه
int cgroup_set_memory_limit(container_config_t *config, uint64_t limit_bytes);

// تنظیم سهم CPU
int cgroup_set_cpu_shares(container_config_t *config, uint64_t shares);

// تنظیم تخصیص CPU
int cgroup_set_cpu_affinity(container_config_t *config, int cpu_id);

// تنظیم وزن I/O
int cgroup_set_io_weight(container_config_t *config, uint64_t weight);

// اضافه کردن فرآیند به cgroup
int cgroup_add_process(container_config_t *config, pid_t pid);

// دریافت مصرف منابع
int cgroup_get_memory_usage(container_config_t *config, uint64_t *usage);
int cgroup_get_cpu_usage(container_config_t *config, uint64_t *usage);
int cgroup_get_io_usage(container_config_t *config, uint64_t *read_bytes, uint64_t *write_bytes);

// توابع کمکی
int write_cgroup_file(const char *cgroup_path, const char *file, const char *value);
int read_cgroup_file(const char *cgroup_path, const char *file, char *buffer, size_t buffer_size);

#endif // CGROUP_H