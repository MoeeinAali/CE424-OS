#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/wait.h>
#include <inttypes.h>
#include "../include/cli.h"
#include "../include/container.h"
#include "../include/utils.h"

// تعاریف برای getopt
static struct option long_options[] = {
    {"name", required_argument, 0, 'n'},
    {"memory", required_argument, 0, 'm'},
    {"cpu", required_argument, 0, 'c'},
    {"io-weight", required_argument, 0, 'i'},
    {"detach", no_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

// نمایش راهنمای دستورات
void cli_help() {
    printf("استفاده: simplecontainer <دستور> [گزینه‌ها] [آرگومان‌ها]\n\n");
    printf("دستورات:\n");
    printf("  run <باینری>    اجرای یک باینری درون کانتینر\n");
    printf("  list            نمایش لیست کانتینرها\n");
    printf("  stop <شناسه>    توقف یک کانتینر\n");
    printf("  start <شناسه>   راه‌اندازی مجدد یک کانتینر\n");
    printf("  status <شناسه>  نمایش وضعیت یک کانتینر\n");
    printf("  help            نمایش این پیام راهنما\n\n");
    
    printf("گزینه‌های run:\n");
    printf("  --name, -n <نام>        نام کانتینر\n");
    printf("  --memory, -m <مقدار>    محدودیت حافظه (مثال: 100M)\n");
    printf("  --cpu, -c <شماره>       تخصیص کانتینر به یک CPU خاص\n");
    printf("  --io-weight, -i <وزن>   وزن I/O (1-100)\n");
    printf("  --detach, -d            اجرا در پس‌زمینه\n");
    printf("  --help, -h              نمایش این پیام راهنما\n");
}

// پردازش دستورات ورودی
int cli_process_command(container_manager_t *manager, int argc, char **argv) {
    if (argc < 2) {
        cli_help();
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, CMD_RUN) == 0) {
        return cli_run(manager, argc - 1, argv + 1);
    } else if (strcmp(command, CMD_LIST) == 0) {
        return cli_list(manager);
    } else if (strcmp(command, CMD_STOP) == 0) {
        if (argc < 3) {
            fprintf(stderr, "خطا: شناسه کانتینر مشخص نشده است\n");
            return 1;
        }
        return cli_stop(manager, argv[2]);
    } else if (strcmp(command, CMD_START) == 0) {
        if (argc < 3) {
            fprintf(stderr, "خطا: شناسه کانتینر مشخص نشده است\n");
            return 1;
        }
        return cli_start(manager, argv[2]);
    } else if (strcmp(command, CMD_STATUS) == 0) {
        if (argc < 3) {
            fprintf(stderr, "خطا: شناسه کانتینر مشخص نشده است\n");
            return 1;
        }
        return cli_status(manager, argv[2]);
    } else if (strcmp(command, CMD_HELP) == 0) {
        cli_help();
        return 0;
    } else {
        fprintf(stderr, "خطا: دستور ناشناخته '%s'\n", command);
        cli_help();
        return 1;
    }
}

// پردازش دستور run
int cli_run(container_manager_t *manager, int argc, char **argv) {
    // مقادیر پیش‌فرض
    char container_name[256] = "container";
    uint64_t memory_limit = 512 * 1024 * 1024;  // 512 MB
    int cpu_affinity = -1;
    uint64_t io_weight = 100;
    bool detach = false;
    
    // پارس کردن گزینه‌ها
    optind = 0;  // بازنشانی optind
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "n:m:c:i:dh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'n':
                strncpy(container_name, optarg, sizeof(container_name) - 1);
                break;
                
            case 'm': {
                // پارس کردن محدودیت حافظه (مثلاً 100M, 2G)
                char *endptr;
                uint64_t value = strtoull(optarg, &endptr, 10);
                
                if (*endptr == 'K' || *endptr == 'k') {
                    memory_limit = value * 1024;
                } else if (*endptr == 'M' || *endptr == 'm') {
                    memory_limit = value * 1024 * 1024;
                } else if (*endptr == 'G' || *endptr == 'g') {
                    memory_limit = value * 1024 * 1024 * 1024;
                } else {
                    memory_limit = value;
                }
                break;
            }
                
            case 'c':
                cpu_affinity = atoi(optarg);
                break;
                
            case 'i':
                io_weight = atoi(optarg);
                break;
                
            case 'd':
                detach = true;
                break;
                
            case 'h':
                cli_help();
                return 0;
                
            default:
                fprintf(stderr, "خطا: گزینه نامعتبر\n");
                return 1;
        }
    }
    
    // بررسی وجود باینری
    if (optind >= argc) {
        fprintf(stderr, "خطا: مسیر باینری مشخص نشده است\n");
        return 1;
    }
    
    // دریافت مسیر باینری و آرگومان‌های آن
    char *binary_path;
    char **container_args;
    int container_argc;
    
    if (cli_parse_args(argc - optind, argv + optind, &binary_path, &container_args, &container_argc) != 0) {
        fprintf(stderr, "خطا در پارس کردن آرگومان‌ها\n");
        return 1;
    }
    
    // ایجاد کانتینر
    if (container_create(manager, container_name, binary_path, container_args, container_argc) != 0) {
        fprintf(stderr, "خطا در ایجاد کانتینر\n");
        free(container_args);
        return 1;
    }
    
    // تنظیم محدودیت‌های منابع
    container_config_t *config = &manager->containers[manager->container_count - 1];
    container_set_memory_limit(manager, config->id, memory_limit);
    container_set_cpu_affinity(manager, config->id, cpu_affinity);
    container_set_io_weight(manager, config->id, io_weight);
    
    // شروع کانتینر
    printf("شروع کانتینر...\n");
    if (container_start(manager, config->id) != 0) {
        fprintf(stderr, "خطا در شروع کانتینر\n");
        free(container_args);
        return 1;
    }
    
    // آزادسازی منابع
    free(container_args);
    
    // اگر در حالت detach نیست، منتظر پایان کانتینر بمان
    if (!detach) {
        int status;
        waitpid(config->container_pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("کانتینر با کد خروج %d به پایان رسید\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("کانتینر با سیگنال %d خاتمه یافت\n", WTERMSIG(status));
        }
    }
    
    return 0;
}

// نمایش لیست کانتینرها
int cli_list(container_manager_t *manager) {
    return container_list(manager);
}

// توقف کانتینر
int cli_stop(container_manager_t *manager, const char *container_id) {
    return container_stop(manager, container_id);
}

// راه‌اندازی مجدد کانتینر
int cli_start(container_manager_t *manager, const char *container_id) {
    return container_start(manager, container_id);
}

// نمایش وضعیت کانتینر
int cli_status(container_manager_t *manager, const char *container_id) {
    return container_status(manager, container_id);
}

// پارس کردن آرگومان‌های دستور
int cli_parse_args(int argc, char **argv, char **binary_path, char ***container_args, int *container_argc) {
    if (argc <= 0) {
        return -1;
    }
    
    *binary_path = argv[0];
    *container_argc = argc;
    
    *container_args = malloc(sizeof(char*) * (argc + 1));
    if (!*container_args) {
        return -1;
    }
    
    for (int i = 0; i < argc; i++) {
        (*container_args)[i] = strdup(argv[i]);
    }
    (*container_args)[argc] = NULL;
    
    return 0;
}
