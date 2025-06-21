#ifndef CGROUP_H
#define CGROUP_H

#include "container.h"
#include <stdint.h>

// مسیر پایه cgroup v2
#define CGROUP_BASE_PATH "/sys/fs/cgroup/unified/simplecontainer"

// راه‌اندازی cgroup برای کانتینر
int cgroup_setup(container_config_t *config);

// پاک‌سازی cgroup
int cgroup_cleanup(container_config_t *config);

// تنظیم محدودیت‌های منابع
int cgroup_set_memory_limit(container_config_t *config, uint64_t limit_bytes);
int cgroup_set_cpu_shares(container_config_t *config, uint64_t shares);
int cgroup_set_cpu_affinity(container_config_t *config, int cpu_id);
int cgroup_set_io_weight(container_config_t *config, uint64_t weight);

// توقف موقت و ادامه کانتینر با استفاده از freezer
int cgroup_freeze_container(container_config_t *config);
int cgroup_unfreeze_container(container_config_t *config);

// مانیتورینگ مصرف منابع
int cgroup_get_memory_usage(container_config_t *config, uint64_t *usage);
int cgroup_get_cpu_usage(container_config_t *config, uint64_t *usage);
int cgroup_get_io_usage(container_config_t *config, uint64_t *read_bytes, uint64_t *write_bytes);

#endif /* CGROUP_H */