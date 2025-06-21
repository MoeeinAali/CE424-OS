#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "container.h"

// تنظیم namespace های مختلف
int setup_pid_namespace();
int setup_mount_namespace();
int setup_uts_namespace(const char *hostname);
int setup_user_namespace();
int setup_network_namespace();
int setup_ipc_namespace();

// تابع راه‌اندازی تمام namespace‌ها
int setup_namespaces(container_config_t *config);

// توابع کمکی
int join_namespace(const char *ns_path, int nstype);
int get_namespace_path(pid_t pid, int nstype, char *buf, size_t bufsize);

#endif /* NAMESPACE_H */