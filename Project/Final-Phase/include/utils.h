#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

// تولید شناسه منحصر به فرد
int generate_unique_id(char *buffer, size_t buffer_size);

// بررسی وجود دایرکتوری
bool directory_exists(const char *path);

// ایجاد دایرکتوری
int create_directory(const char *path, int mode);

// حذف دایرکتوری
int remove_directory(const char *path);

// ثبت لاگ
void log_message(const char *format, ...);

// ثبت خطا
void log_error(const char *format, ...);

// بررسی دسترسی‌های root
bool has_root_privileges();

// کپی فایل
int copy_file(const char *source, const char *destination);

// حذف فایل
int remove_file(const char *path);

#endif /* UTILS_H */