#ifndef IPC_H
#define IPC_H

#include "container.h"

// راه‌اندازی IPC بین کانتینرها
int ipc_setup();

// پاک‌سازی IPC
int ipc_cleanup();

// ایجاد کانال IPC برای کانتینر
int ipc_create_channel(container_config_t *config, const char *channel_name);

// اتصال دو کانتینر از طریق IPC
int ipc_connect_containers(const char *container_id1, const char *container_id2, const char *channel_name);

// ارسال پیام بین کانتینرها
int ipc_send_message(const char *channel_name, const void *data, size_t data_size);

// دریافت پیام از کانتینر دیگر
int ipc_receive_message(const char *channel_name, void *buffer, size_t buffer_size);

#endif /* IPC_H */