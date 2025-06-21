```c
// بخشی از پیاده‌سازی PID Namespace در namespace.c
int setup_pid_namespace() {
    // namespace PID قبلاً با فراخوانی clone ایجاد شده است
    // نصب procfs برای نمایش صحیح PID‌ها
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        log_error("خطا در نصب /proc");
        return -1;
    }
    return 0;
}
```