#ifndef CLI_H
#define CLI_H

#include "container.h"

// تعاریف دستورات
#define CMD_RUN     "run"
#define CMD_LIST    "list"
#define CMD_STOP    "stop"
#define CMD_START   "start"
#define CMD_STATUS  "status"
#define CMD_HELP    "help"

// پردازش دستورات ورودی
int cli_process_command(container_manager_t *manager, int argc, char **argv);

// تابع‌های پردازش دستورات
int cli_run(container_manager_t *manager, int argc, char **argv);
int cli_list(container_manager_t *manager);
int cli_stop(container_manager_t *manager, const char *container_id);
int cli_start(container_manager_t *manager, const char *container_id);
int cli_status(container_manager_t *manager, const char *container_id);
void cli_help();

// پارس کردن آرگومان‌های دستور
int cli_parse_args(int argc, char **argv, char **binary_path, char ***container_args, int *container_argc);

#endif /* CLI_H */