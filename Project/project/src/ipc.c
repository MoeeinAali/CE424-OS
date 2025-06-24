#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include "../include/ipc.h"
#include "../include/utils.h"

// ساختار کانال IPC
typedef struct {
    char name[64];
    int type;        // نوع کانال: 1=shm, 2=sem, 3=msg
    int id;          // شناسه منبع IPC
    key_t key;       // کلید منبع IPC
} ipc_channel_t;

// حداکثر تعداد کانال‌های IPC
#define MAX_IPC_CHANNELS 32

// آرایه کانال‌های IPC
static ipc_channel_t channels[MAX_IPC_CHANNELS];
static int channel_count = 0;

// راه‌اندازی IPC
int ipc_setup() {
    // پاک‌سازی آرایه کانال‌ها
    memset(channels, 0, sizeof(channels));
    channel_count = 0;
    
    log_message("سیستم IPC راه‌اندازی شد");
    return 0;
}

// پاک‌سازی IPC
int ipc_cleanup() {
    // آزادسازی تمام منابع IPC
    for (int i = 0; i < channel_count; i++) {
        if (channels[i].type == 1) {
            // حافظه مشترک
            shmctl(channels[i].id, IPC_RMID, NULL);
        } else if (channels[i].type == 2) {
            // سمافور
            semctl(channels[i].id, 0, IPC_RMID);
        } else if (channels[i].type == 3) {
            // صف پیام
            msgctl(channels[i].id, IPC_RMID, NULL);
        }
    }
    
    // پاک‌سازی آرایه کانال‌ها
    memset(channels, 0, sizeof(channels));
    channel_count = 0;
    
    log_message("منابع IPC پاک‌سازی شدند");
    return 0;
}

// یافتن یک کانال با نام
static ipc_channel_t* find_channel(const char *channel_name) {
    for (int i = 0; i < channel_count; i++) {
        if (strcmp(channels[i].name, channel_name) == 0) {
            return &channels[i];
        }
    }
    return NULL;
}

// ایجاد کانال IPC برای کانتینر
int ipc_create_channel(container_config_t *config, const char *channel_name) {
    if (channel_count >= MAX_IPC_CHANNELS) {
        log_error("حداکثر تعداد کانال‌های IPC ایجاد شده است");
        return -1;
    }
    
    // بررسی وجود کانال با نام تکراری
    if (find_channel(channel_name) != NULL) {
        log_error("کانال IPC با نام %s قبلاً ایجاد شده است", channel_name);
        return -1;
    }
    
    // ایجاد کلید IPC منحصر به فرد
    key_t key = ftok("/var/lib/simplecontainer", channel_count + 1);
    if (key == -1) {
        log_error("خطا در ایجاد کلید IPC");
        return -1;
    }
    
    // ایجاد حافظه مشترک
    int shm_id = shmget(key, 4096, IPC_CREAT | 0666);
    if (shm_id == -1) {
        log_error("خطا در ایجاد حافظه مشترک");
        return -1;
    }
    
    // ثبت کانال جدید
    ipc_channel_t *channel = &channels[channel_count++];
    strncpy(channel->name, channel_name, sizeof(channel->name) - 1);
    channel->type = 1;  // حافظه مشترک
    channel->id = shm_id;
    channel->key = key;
    
    log_message("کانال IPC %s برای کانتینر %s ایجاد شد", channel_name, config->id);
    return 0;
}

// اتصال دو کانتینر از طریق IPC
int ipc_connect_containers(const char *container_id1, const char *container_id2, const char *channel_name) {
    // بررسی وجود کانال
    ipc_channel_t *channel = find_channel(channel_name);
    if (channel == NULL) {
        log_error("کانال IPC با نام %s پیدا نشد", channel_name);
        return -1;
    }
    
    log_message("کانتینرهای %s و %s از طریق کانال %s به هم متصل شدند", 
                container_id1, container_id2, channel_name);
    return 0;
}

// ارسال پیام بین کانتینرها
int ipc_send_message(const char *channel_name, const void *data, size_t data_size) {
    // بررسی وجود کانال
    ipc_channel_t *channel = find_channel(channel_name);
    if (channel == NULL) {
        log_error("کانال IPC با نام %s پیدا نشد", channel_name);
        return -1;
    }
    
    // بررسی نوع کانال
    if (channel->type != 1) {
        log_error("کانال %s از نوع حافظه مشترک نیست", channel_name);
        return -1;
    }
    
    // دسترسی به حافظه مشترک
    void *shm_addr = shmat(channel->id, NULL, 0);
    if (shm_addr == (void *) -1) {
        log_error("خطا در اتصال به حافظه مشترک");
        return -1;
    }
    
    // کپی داده به حافظه مشترک
    if (data_size > 4096) {
        data_size = 4096;  // محدود کردن اندازه داده
    }
    
    // ساختار داده: 4 بایت اول اندازه و بقیه داده اصلی
    *((uint32_t *)shm_addr) = data_size;
    memcpy((char *)shm_addr + 4, data, data_size);
    
    // جدا شدن از حافظه مشترک
    shmdt(shm_addr);
    
    log_message("%lu بایت داده از طریق کانال %s ارسال شد", data_size, channel_name);
    return 0;
}

// دریافت پیام از کانتینر دیگر
int ipc_receive_message(const char *channel_name, void *buffer, size_t buffer_size) {
    // بررسی وجود کانال
    ipc_channel_t *channel = find_channel(channel_name);
    if (channel == NULL) {
        log_error("کانال IPC با نام %s پیدا نشد", channel_name);
        return -1;
    }
    
    // بررسی نوع کانال
    if (channel->type != 1) {
        log_error("کانال %s از نوع حافظه مشترک نیست", channel_name);
        return -1;
    }
    
    // دسترسی به حافظه مشترک
    void *shm_addr = shmat(channel->id, NULL, SHM_RDONLY);
    if (shm_addr == (void *) -1) {
        log_error("خطا در اتصال به حافظه مشترک");
        return -1;
    }
    
    // خواندن اندازه داده
    uint32_t data_size = *((uint32_t *)shm_addr);
    
    // بررسی اندازه بافر
    if (data_size > buffer_size) {
        log_error("بافر کوچکتر از داده است (%u > %lu)", data_size, buffer_size);
        shmdt(shm_addr);
        return -1;
    }
    
    // کپی داده از حافظه مشترک
    memcpy(buffer, (char *)shm_addr + 4, data_size);
    
    // جدا شدن از حافظه مشترک
    shmdt(shm_addr);
    
    log_message("%u بایت داده از طریق کانال %s دریافت شد", data_size, channel_name);
    return data_size;
}