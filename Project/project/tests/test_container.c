#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../include/container.h"
#include "../include/namespace.h"
#include "../include/cgroup.h"
#include "../include/filesystem.h"
#include "../include/utils.h"

// تست مدیریت کانتینر
void test_container_manager() {
    printf("تست مدیریت کانتینر...\n");
    
    // ایجاد مدیریت‌کننده
    container_manager_t *manager = container_manager_create(10);
    assert(manager != NULL);
    assert(manager->max_containers == 10);
    assert(manager->container_count == 0);
    
    // پاک‌سازی
    container_manager_destroy(manager);
    
    printf("تست مدیریت کانتینر با موفقیت انجام شد\n");
}

// تست ایجاد کانتینر
void test_container_create() {
    printf("تست ایجاد کانتینر...\n");
    
    // ایجاد مدیریت‌کننده
    container_manager_t *manager = container_manager_create(10);
    assert(manager != NULL);
    
    // آرگومان‌های تست
    char *args[] = {"/bin/echo", "hello", NULL};
    
    // ایجاد کانتینر
    int result = container_create(manager, "test_container", "/bin/echo", args, 2);
    assert(result == 0);
    assert(manager->container_count == 1);
    
    // بررسی مشخصات کانتینر
    container_config_t *config = &manager->containers[0];
    assert(strcmp(config->name, "test_container") == 0);
    assert(strcmp(config->binary_path, "/bin/echo") == 0);
    assert(config->running == false);
    
    // پاک‌سازی
    container_manager_destroy(manager);
    
    printf("تست ایجاد کانتینر با موفقیت انجام شد\n");
}

// تست عملکرد جستجو
void test_container_find() {
    printf("تست جستجوی کانتینر...\n");
    
    // ایجاد مدیریت‌کننده
    container_manager_t *manager = container_manager_create(10);
    assert(manager != NULL);
    
    // آرگومان‌های تست
    char *args[] = {"/bin/echo", "hello", NULL};
    
    // ایجاد کانتینر
    int result = container_create(manager, "test_container", "/bin/echo", args, 2);
    assert(result == 0);
    
    // ذخیره شناسه کانتینر
    char container_id[64];
    strncpy(container_id, manager->containers[0].id, sizeof(container_id));
    
    // جستجوی کانتینر
    container_config_t *found = container_find_by_id(manager, container_id);
    assert(found != NULL);
    assert(strcmp(found->name, "test_container") == 0);
    
    // جستجوی کانتینر نامعتبر
    found = container_find_by_id(manager, "invalid_id");
    assert(found == NULL);
    
    // پاک‌سازی
    container_manager_destroy(manager);
    
    printf("تست جستجوی کانتینر با موفقیت انجام شد\n");
}

// اجرای همه تست‌ها
int main() {
    printf("شروع آزمون‌های واحد...\n");
    
    test_container_manager();
    test_container_create();
    test_container_find();
    
    printf("تمام آزمون‌ها با موفقیت انجام شدند\n");
    return 0;
}