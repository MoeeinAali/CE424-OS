#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <sys/types.h>
#include <stdbool.h>
#include "container.h"

// تنظیم همه namespace ها
int setup_namespaces(container_config_t *config);

// تنظیم namespace های خاص
int setup_user_namespace();
int setup_pid_namespace();
int setup_mount_namespace();
int setup_uts_namespace(const char *hostname);
int setup_network_namespace();
int setup_ipc_namespace();

// اتصال به namespace موجود
int join_namespace(pid_t pid, int nstype);

// دریافت نام namespace بر اساس نوع
const char* get_namespace_path(int nstype);

// بررسی وجود namespace
bool namespace_exists(pid_t pid, int nstype);

#endif // NAMESPACE_H