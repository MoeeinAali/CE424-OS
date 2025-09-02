#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include "../include/filesystem.h"
#include "../include/utils.h"

// تنظیم فایل‌سیستم ریشه کانتینر
int setup_container_rootfs(container_config_t *config) {
    // ایجاد دایرکتوری‌های مورد نیاز
    if (prepare_container_directories(config) != 0) {
        log_error("خطا در ایجاد دایرکتوری‌های کانتینر");
        return -1;
    }
    
    // راه‌اندازی overlayfs
    if (setup_overlayfs(config) != 0) {
        log_error("خطا در راه‌اندازی overlayfs");
        return -1;
    }
    
    return 0;
}

// پاک‌سازی فایل‌سیستم ریشه کانتینر
int cleanup_container_rootfs(container_config_t *config) {
    // جدا کردن فایل‌سیستم overlayfs
    if (umount(config->rootfs) != 0) {
        log_error("خطا در جدا کردن فایل‌سیستم overlayfs");
        return -1;
    }
    
    // حذف دایرکتوری‌های کانتینر
    if (remove_directory(config->rootfs) != 0 ||
        remove_directory(config->overlay_workdir) != 0) {
        log_error("خطا در حذف دایرکتوری‌های کانتینر");
        return -1;
    }
    
    return 0;
}

// راه‌اندازی overlayfs
int setup_overlayfs(container_config_t *config) {
    // مسیرهای مورد نیاز برای overlayfs - افزایش اندازه buffer
    char lowerdir[2048] = "/var/lib/simplecontainer/rootfs/base";
    char upperdir[2048];
    char workdir[2048];
    char mountopts[4096];  // افزایش اندازه به 4KB
    
    // تنظیم مسیرها
    int ret1 = snprintf(upperdir, sizeof(upperdir), "%s/upper", config->overlay_workdir);
    int ret2 = snprintf(workdir, sizeof(workdir), "%s/work", config->overlay_workdir);
    
    // بررسی overflow
    if (ret1 >= (int)sizeof(upperdir) || ret2 >= (int)sizeof(workdir)) {
        log_error("مسیر overlay_workdir خیلی طولانی است");
        return -1;
    }
    
    // ایجاد دایرکتوری‌های مورد نیاز
    if (create_directory(upperdir, 0755) != 0 ||
        create_directory(workdir, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری‌های overlayfs");
        return -1;
    }
    
    // ساخت گزینه‌های mount برای overlayfs
    int ret3 = snprintf(mountopts, sizeof(mountopts),
                       "lowerdir=%s,upperdir=%s,workdir=%s",
                       lowerdir, upperdir, workdir);
    
    // بررسی overflow
    if (ret3 >= (int)sizeof(mountopts)) {
        log_error("مسیرهای overlayfs خیلی طولانی هستند");
        return -1;
    }
    
    // نصب overlayfs
    if (mount("overlay", config->rootfs, "overlay", 0, mountopts) != 0) {
        log_error("خطا در نصب overlayfs");
        return -1;
    }
    
    log_message("overlayfs با موفقیت راه‌اندازی شد");
    return 0;
}

// اجرای chroot
int do_chroot(const char *path) {
    // تغییر ریشه فایل‌سیستم به مسیر مشخص شده
    if (chroot(path) != 0) {
        log_error("خطا در اجرای chroot به %s", path);
        return -1;
    }
    
    // تغییر دایرکتوری جاری به ریشه جدید
    if (chdir("/") != 0) {
        log_error("خطا در تغییر دایرکتوری به /");
        return -1;
    }
    
    return 0;
}

// ایجاد مسیرهای مورد نیاز در کانتینر
int prepare_container_directories(container_config_t *config) {
    // ایجاد دایرکتوری ریشه کانتینر
    if (create_directory(config->rootfs, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری ریشه کانتینر");
        return -1;
    }
    
    // ایجاد دایرکتوری کاری overlayfs
    if (create_directory(config->overlay_workdir, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری کاری overlayfs");
        return -1;
    }
    
    // ایجاد دایرکتوری‌های ضروری داخل کانتینر - افزایش اندازه buffer
    char proc_dir[2048], sys_dir[2048], dev_dir[2048], tmp_dir[2048];
    
    int ret1 = snprintf(proc_dir, sizeof(proc_dir), "%s/proc", config->rootfs);
    int ret2 = snprintf(sys_dir, sizeof(sys_dir), "%s/sys", config->rootfs);
    int ret3 = snprintf(dev_dir, sizeof(dev_dir), "%s/dev", config->rootfs);
    int ret4 = snprintf(tmp_dir, sizeof(tmp_dir), "%s/tmp", config->rootfs);
    
    // بررسی overflow
    if (ret1 >= (int)sizeof(proc_dir) || ret2 >= (int)sizeof(sys_dir) ||
        ret3 >= (int)sizeof(dev_dir) || ret4 >= (int)sizeof(tmp_dir)) {
        log_error("مسیر rootfs خیلی طولانی است");
        return -1;
    }
    
    if (create_directory(proc_dir, 0755) != 0 ||
        create_directory(sys_dir, 0755) != 0 ||
        create_directory(dev_dir, 0755) != 0 ||
        create_directory(tmp_dir, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری‌های ضروری کانتینر");
        return -1;
    }
    
    return 0;
}

// نصب فایل‌سیستم‌های ضروری
int mount_essential_filesystems(container_config_t *config) {
    (void)config;  // جلوگیری از warning unused parameter
    
    // نصب proc
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        log_error("خطا در نصب /proc");
        return -1;
    }
    
    // نصب sysfs
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        log_error("خطا در نصب /sys");
        return -1;
    }
    
    // نصب devtmpfs
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0) {
        log_error("خطا در نصب /dev");
        return -1;
    }
    
    // نصب tmpfs
    if (mount("tmpfs", "/tmp", "tmpfs", 0, NULL) != 0) {
        log_error("خطا در نصب /tmp");
        return -1;
    }
    
    log_message("فایل‌سیستم‌های ضروری با موفقیت نصب شدند");
    return 0;
}

// بارگذاری تصویر کانتینر (اختیاری)
int load_container_image(container_config_t *config, const char *image_path) {
    (void)config;  // جلوگیری از warning unused parameter
    
    // بررسی وجود تصویر
    struct stat st;
    if (stat(image_path, &st) != 0) {
        log_error("تصویر کانتینر یافت نشد: %s", image_path);
        return -1;
    }
    
    // مسیر پایه برای تصاویر
    char base_image_dir[512];
    snprintf(base_image_dir, sizeof(base_image_dir), "/var/lib/simplecontainer/images");
    
    // ایجاد دایرکتوری تصاویر
    if (create_directory(base_image_dir, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری تصاویر");
        return -1;
    }
    
    // در پیاده‌سازی کامل، اینجا تصویر استخراج می‌شود
    log_message("تصویر %s بارگذاری شد", image_path);
    
    return 0;
}