# Makefile for SimpleContainer
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lbpf -lelf

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

# ابزار اصلی
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET = simplecontainer

# مثال‌ها
EXAMPLES_DIR = examples
EXAMPLES_SOURCES = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLES_TARGETS = $(patsubst $(EXAMPLES_DIR)/%.c,$(EXAMPLES_DIR)/%,$(EXAMPLES_SOURCES))

# ایجاد دایرکتوری‌های مورد نیاز
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(EXAMPLES_DIR))

# هدف پیش‌فرض
all: $(TARGET) examples

# ساخت ابزار اصلی
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build completed successfully!"

# کامپایل فایل‌های منبع
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c -o $@ $<

# ساخت مثال‌ها
examples: $(EXAMPLES_TARGETS)

$(EXAMPLES_DIR)/%: $(EXAMPLES_DIR)/%.c
	@echo "Building example $@..."
	@$(CC) $(CFLAGS) -o $@ $<

# نصب
install: $(TARGET)
	@echo "Installing SimpleContainer..."
	@mkdir -p /var/lib/simplecontainer/rootfs
	@mkdir -p /var/lib/simplecontainer/overlays
	@mkdir -p /var/lib/simplecontainer/logs
	@cp $(TARGET) /usr/local/bin/
	@echo "Installation completed."

# پاک‌سازی
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR)/*.o $(TARGET) $(EXAMPLES_TARGETS)
	@echo "Clean completed."

# پاک‌سازی کامل
distclean: clean
	@echo "Performing full cleanup..."
	@rm -rf $(BUILD_DIR)
	@echo "Full cleanup completed."

# راه‌اندازی تست‌ها
test: $(TARGET) examples
	@echo "Running tests..."
	@./examples/hello
	@echo "Tests completed."

.PHONY: all examples install clean distclean test