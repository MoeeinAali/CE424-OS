این راهنما نحوه دمو کردن و آزمایش قابلیت‌های مختلف SimpleContainer را به صورت گام به گام شرح می‌دهد.

## ساخت پروژه

```bash
make
```

## ساخت باینری‌های تست

```bash
cd examples
clang -o hello hello_world.c
clang -o resource_test resource_test.c
```

---
---


<br/>

# دمو‌ها


#### دمو 1: اجرای کانتینر ساده Hello World

این دمو یک برنامه ساده Hello World را در یک کانتینر ایزوله اجرا می‌کند:

```bash
sudo ./simplecontainer run --name hello_container ./examples/hello
```

خروجی:

```
کانتینر با شناسه a1b2c3d4 ایجاد شد
شروع کانتینر...
Hello from SimpleContainer!
کانتینر با کد خروج 0 به پایان رسید
```

---

#### دمو 2: مشاهده لیست کانتینرها

```bash
sudo ./simplecontainer list
```

خروجی:

```bash
تعداد کانتینرها: 1
--------------------------------------
شناسه     نام                   وضعیت     PID       
--------------------------------------
a1b2c3d4   hello_container      متوقف     -1        
```

---

#### دمو 3: اجرای کانتینر با محدودیت حافظه

این دمو نشان می‌دهد چگونه cgroup برای محدود کردن حافظه استفاده می‌شود:

```bash
sudo ./simplecontainer run --name memory_test --memory 50M ./examples/resource_test --memory-test
```

خروجی:
```bash
کانتینر با شناسه e5f6g7h8 ایجاد شد
شروع کانتینر...
تلاش برای تخصیص 100MB حافظه...
خطای تخصیص حافظه: محدودیت حافظه اعمال شد
کانتینر با کد خروج 1 به پایان رسید
```

---

#### دمو 4: اجرای کانتینر با تخصیص CPU خاص

این دمو نشان می‌دهد چگونه می‌توان یک کانتینر را به یک هسته CPU خاص محدود کرد:

```bash
sudo ./simplecontainer run --name cpu_test --cpu 0 ./examples/resource_test --cpu-test
```

خروجی:

```bash
کانتینر با شناسه i9j0k1l2 ایجاد شد
شروع کانتینر...
در حال اجرای محاسبات روی CPU...
CPU ID فعال: 0
کانتینر با کد خروج 0 به پایان رسید
```

---

#### دمو 5: راه‌اندازی کانتینر Shell و آزمایش ایزولاسیون

این دمو یک shell در کانتینر اجرا می‌کند تا بتوانید به صورت تعاملی ایزولاسیون را بررسی کنید:

```bash
sudo ./simplecontainer run --name shell_container /bin/bash
```


دستورات مفید برای آزمایش داخل کانتینر:

```bash
# بررسی hostname (باید نام کانتینر باشد)
hostname

# بررسی فرآیندها (باید فقط فرآیندهای داخل کانتینر را نشان دهد)
ps aux

# بررسی فایل‌سیستم (باید ایزوله باشد)
ls /
mount

# بررسی کاربر (باید root باشد حتی اگر با کاربر غیر-root اجرا شده باشد)
id
whoami

# خروج از کانتینر
exit
```

---

#### دمو 6: مشاهده وضعیت و منابع کانتینر

```bash
# شناسه کانتینر را از خروجی دستور list بدست آورید
sudo ./simplecontainer status <container_id>
```

خروجی:

```bash
شناسه: i9j0k1l2
نام: cpu_test
وضعیت: متوقف
محدودیت حافظه: 512 MB
سهم CPU: 1024
تخصیص CPU: 0
وزن I/O: 100
```

---

#### دمو 7: مشاهده لاگ‌های eBPF

این دمو نشان می‌دهد چگونه سیستم eBPF، فعالیت‌های سطح پایین را ثبت می‌کند:

```bash
sudo cat /var/lib/simplecontainer/logs/<container_id>.log
```
خروجی:
```bash
[2025-06-21 12:10:15] SYSCALL: clone (pid=1234)
[2025-06-21 12:10:15] NAMESPACE: created PID namespace
[2025-06-21 12:10:15] NAMESPACE: created UTS namespace
[2025-06-21 12:10:15] NAMESPACE: created mount namespace
[2025-06-21 12:10:15] CGROUP: memory limit set to 52428800 bytes
[2025-06-21 12:10:16] SYSCALL: execve (pid=1234, binary="/examples/resource_test")
[2025-06-21 12:10:16] SYSCALL: mmap (pid=1234, size=104857600)
[2025-06-21 12:10:16] CGROUP: memory.max limit triggered
[2025-06-21 12:10:16] SYSCALL: exit_group (pid=1234, exit_code=1)
```

---

#### دمو 8: اجرای چندین کانتینر همزمان

این دمو نشان می‌دهد چگونه چندین کانتینر به طور همزمان اجرا می‌شوند:

```bash
# اجرای کانتینر با sleep طولانی
sudo ./simplecontainer run --name long_running --detach /bin/sleep 300

# اجرای کانتینر دیگر
sudo ./simplecontainer run --name another_container --detach /bin/sleep 300

# مشاهده لیست کانتینرهای در حال اجرا
sudo ./simplecontainer list
```

خروجی:
```bash
# تعداد کانتینرها: 2
# --------------------------------------
# شناسه     نام                   وضعیت     PID       
# --------------------------------------
# m3n4o5p6   long_running         اجرا       1234      
# q7r8s9t0   another_container    اجرا       1235      
```

---

#### دمو 9: توقف و راه‌اندازی مجدد کانتینر

```bash
# توقف یک کانتینر
sudo ./simplecontainer stop m3n4o5p6

# خروجی:
# کانتینر m3n4o5p6 متوقف شد

# شروع مجدد کانتینر
sudo ./simplecontainer start m3n4o5p6

# خروجی:
# کانتینر m3n4o5p6 با PID 1236 شروع شد

# بررسی وضعیت
sudo ./simplecontainer status m3n4o5p6
```

 خروجی:
```bash
# شناسه: m3n4o5p6
# نام: long_running
# وضعیت: در حال اجرا
# PID: 1236
# مصرف CPU: 0%
# مصرف حافظه: 1 MB
# خواندن I/O: 0 KB
# نوشتن I/O: 0 KB
# محدودیت حافظه: 512 MB
# سهم CPU: 1024
# تخصیص CPU: تمام هسته‌ها
# وزن I/O: 100
```

---

#### دمو 10: آزمایش محدودیت I/O

```bash
sudo ./simplecontainer run --name io_test --io-weight 50 ./examples/resource_test --io-test
```

خروجی:
```bash
# کانتینر با شناسه u1v2w3x4 ایجاد شد
# شروع کانتینر...
# در حال انجام عملیات I/O سنگین...
# I/O انجام شد با سرعت محدود
# کانتینر با کد خروج 0 به پایان رسید
```