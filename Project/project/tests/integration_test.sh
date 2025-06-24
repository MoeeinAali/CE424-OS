#!/bin/bash


# مسیر اجرایی
SIMPLECONTAINER="../simplecontainer"

# رنگ‌ها برای خروجی
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# تابع خروج خطا
error_exit() {
    echo -e "${RED}خطا: $1${NC}" >&2
    exit 1
}

# تابع اجرای تست
run_test() {
    TEST_NAME=$1
    shift
    echo "در حال اجرای تست: $TEST_NAME"
    
    # اجرای دستور تست
    OUTPUT=$("$@" 2>&1)
    RESULT=$?
    
    if [ $RESULT -eq 0 ]; then
        echo -e "${GREEN}✓ تست با موفقیت انجام شد${NC}"
    else
        echo -e "${RED}✗ تست با خطا مواجه شد (کد خروج: $RESULT)${NC}"
        echo -e "${RED}خروجی:${NC}"
        echo "$OUTPUT"
        return 1
    fi
    
    return 0
}

# بررسی وجود فایل اجرایی
if [ ! -f "$SIMPLECONTAINER" ]; then
    error_exit "فایل اجرایی $SIMPLECONTAINER پیدا نشد"
fi

# اطمینان از دسترسی root
if [ "$(id -u)" -ne 0 ]; then
    error_exit "این اسکریپت باید با دسترسی root اجرا شود"
fi

echo "شروع آزمون‌های یکپارچگی..."

# تست 1: اجرای hello world
run_test "Hello World" $SIMPLECONTAINER run --name test_hello ../examples/hello

# تست 2: محدودیت حافظه
run_test "محدودیت حافظه" $SIMPLECONTAINER run --name test_memory --memory 50M ../examples/resource_test --memory-test || true

# تست 3: تخصیص CPU
run_test "تخصیص CPU" $SIMPLECONTAINER run --name test_cpu --cpu 0 ../examples/resource_test --cpu-test

# تست 4: محدودیت I/O
run_test "محدودیت I/O" $SIMPLECONTAINER run --name test_io --io-weight 50 ../examples/resource_test --io-test

# تست 5: نمایش لیست کانتینرها
run_test "لیست کانتینرها" $SIMPLECONTAINER list

# تست 6: دریافت شناسه کانتینر از لیست
CONTAINER_ID=$($SIMPLECONTAINER list | grep test_hello | awk '{print $1}')

# تست 7: بررسی وضعیت کانتینر
if [ -n "$CONTAINER_ID" ]; then
    run_test "وضعیت کانتینر" $SIMPLECONTAINER status $CONTAINER_ID
else
    echo -e "${RED}نمی‌توان شناسه کانتینر را دریافت کرد${NC}"
fi

# تست 8: راه‌اندازی کانتینر shell
run_test "اجرای shell" $SIMPLECONTAINER run --name test_shell --detach /bin/sleep 10

# تست 9: توقف کانتینر
SHELL_ID=$($SIMPLECONTAINER list | grep test_shell | awk '{print $1}')
if [ -n "$SHELL_ID" ]; then
    run_test "توقف کانتینر" $SIMPLECONTAINER stop $SHELL_ID
else
    echo -e "${RED}نمی‌توان شناسه کانتینر shell را دریافت کرد${NC}"
fi

echo "آزمون‌های یکپارچگی به پایان رسید"