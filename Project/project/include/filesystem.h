#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "container.h"

// تنظیم فایل‌سیستم کانتینر
int setup_container_rootfs(container_config_t *config);

// پاک‌سازی فایل‌سیستم کانتینر
int cleanup_container_rootfs(container_config_t *config);

// راه‌اندازی overlayfs
int setup_overlayfs(container_config_t *config);

// اجرای chroot
int do_chroot(const char *path);

// ایجاد مسیرهای مورد نیاز در کانتینر
int prepare_container_directories(container_config_t *config);

// نصب فایل‌سیستم‌های ضروری
int mount_essential_filesystems(container_config_t *config);

// بارگذاری تصویر کانتینر (اختیاری)
int load_container_image(container_config_t *config, const char *image_path);

#endif /* FILESYSTEM_H */