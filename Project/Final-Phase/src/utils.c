#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "../include/utils.h"

// تولید شناسه منحصر به فرد
int generate_unique_id(char *buffer, size_t buffer_size) {
    if (buffer_size < 8) {
        return -1;
    }
    
    // استفاده از زمان و شناسه فرآیند برای تولید ID منحصر به فرد
    time_t now = time(NULL);
    pid_t pid = getpid();
    
    // ترکیب زمان و PID برای ایجاد شناسه تصادفی
    unsigned int seed = (unsigned int)(now ^ pid);
    srand(seed);
    
    // تولید یک رشته 8 کاراکتری شامل اعداد و حروف
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < buffer_size - 1; i++) {
        int index = rand() % (sizeof(charset) - 1);
        buffer[i] = charset[index];
    }
    buffer[buffer_size - 1] = '\0';
    
    return 0;
}

// بررسی وجود دایرکتوری
bool directory_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// ایجاد دایرکتوری
int create_directory(const char *path, int mode) {
    // بررسی وجود دایرکتوری
    if (directory_exists(path)) {
        return 0;  // دایرکتوری از قبل وجود دارد
    }
    
    // ایجاد دایرکتوری با مود مشخص شده
    if (mkdir(path, mode) != 0) {
        // اگر دایرکتوری والد وجود نداشته باشد، سعی می‌کنیم آن را هم بسازیم
        if (errno == ENOENT) {
            char parent_path[512];
            strncpy(parent_path, path, sizeof(parent_path) - 1);
            
            // یافتن آخرین '/'
            char *last_slash = strrchr(parent_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
                
                // ایجاد دایرکتوری والد
                if (create_directory(parent_path, mode) == 0) {
                    // تلاش مجدد برای ایجاد دایرکتوری اصلی
                    return mkdir(path, mode) == 0 ? 0 : -1;
                }
            }
        }
        
        return -1;
    }
    
    return 0;
}

// حذف دایرکتوری
int remove_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char file_path[512];
    
    // باز کردن دایرکتوری
    dir = opendir(path);
    if (!dir) {
        return -1;
    }
    
    // حذف همه فایل‌ها و زیر دایرکتوری‌ها
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
        
        if (entry->d_type == DT_DIR) {
            // بازگشتی حذف زیر دایرکتوری
            remove_directory(file_path);
        } else {
            // حذف فایل
            unlink(file_path);
        }
    }
    
    closedir(dir);
    
    // حذف دایرکتوری خالی
    return rmdir(path);
}

// ثبت لاگ
void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // تاریخ و زمان فعلی
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);
    
    // چاپ لاگ با تاریخ و زمان
    printf("%s INFO: ", timestamp);
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

// ثبت خطا
void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // تاریخ و زمان فعلی
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);
    
    // چاپ خطا با تاریخ و زمان
    fprintf(stderr, "%s ERROR: ", timestamp);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}

// بررسی دسترسی‌های root
bool has_root_privileges() {
    return (geteuid() == 0);
}

// کپی فایل
int copy_file(const char *source, const char *destination) {
    int source_fd, dest_fd;
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    
    // باز کردن فایل منبع
    source_fd = open(source, O_RDONLY);
    if (source_fd == -1) {
        log_error("خطا در باز کردن فایل منبع: %s", source);
        return -1;
    }
    
    // ایجاد فایل مقصد
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        log_error("خطا در ایجاد فایل مقصد: %s", destination);
        close(source_fd);
        return -1;
    }
    
    // کپی داده‌ها
    while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            log_error("خطا در نوشتن به فایل مقصد");
            close(source_fd);
            close(dest_fd);
            return -1;
        }
    }
    
    // بررسی خطای خواندن
    if (bytes_read == -1) {
        log_error("خطا در خواندن از فایل منبع");
        close(source_fd);
        close(dest_fd);
        return -1;
    }
    
    // بستن فایل‌ها
    close(source_fd);
    close(dest_fd);
    
    return 0;
}

// حذف فایل
int remove_file(const char *path) {
    if (unlink(path) != 0) {
        log_error("خطا در حذف فایل: %s", path);
        return -1;
    }
    
    return 0;
}