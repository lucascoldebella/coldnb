# Coldnb E-commerce Backend
# C Backend with libmicrohttpd, PostgreSQL, and OpenSSL

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS += -Iinclude -I/usr/include/postgresql
# Security hardening: stack canaries, buffer overflow detection, ASLR (PIE)
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE
# Linker hardening: Position Independent Executable (ASLR), Full RELRO (GOT protection)
LDFLAGS = -pie -Wl,-z,relro,-z,now

# Debug/Release flags
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Libraries
LIBS = -lmicrohttpd -lpq -lcurl -lssl -lcrypto -lsodium -luuid -lpthread -lcjson -lm

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
TEST_DIR = tests

# Source files
SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# Main executable
TARGET = $(BIN_DIR)/coldnb-server

# Test sources and executables
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/tests/%.o,$(TEST_SRCS))
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/tests/%,$(TEST_SRCS))

# Default target
.PHONY: all
all: release

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Release build
.PHONY: release
release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# Link main executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Built: $@"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR): | $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/tests: | $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/tests

$(BIN_DIR)/tests: | $(BIN_DIR)
	mkdir -p $(BIN_DIR)/tests

# Build tests
.PHONY: tests
tests: CFLAGS += $(DEBUG_FLAGS)
tests: $(TEST_BINS)

$(BIN_DIR)/tests/%: $(OBJ_DIR)/tests/%.o $(filter-out $(OBJ_DIR)/main.o,$(OBJS)) | $(BIN_DIR)/tests
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)/tests
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

# Run tests
.PHONY: test
test: tests
	@for test in $(BIN_DIR)/tests/*; do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "All tests passed!"

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Install (requires root)
.PHONY: install
install: release
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/coldnb-server
	install -d /etc/coldnb
	install -d /etc/coldnb/secrets
	chmod 700 /etc/coldnb/secrets
	install -d /var/log/coldnb
	install -d /var/www/coldnb/uploads/products
	@echo "Installed coldnb-server"

# Uninstall
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/coldnb-server
	@echo "Uninstalled coldnb-server (config and data preserved)"

# Format code (requires clang-format)
.PHONY: format
format:
	find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | xargs clang-format -i

# Static analysis (requires cppcheck)
.PHONY: check
check:
	cppcheck --enable=all --std=c11 -I$(INC_DIR) $(SRC_DIR)

# Build password generator tool
.PHONY: tools
tools: $(BIN_DIR)/generate_admin_password

$(BIN_DIR)/generate_admin_password: scripts/generate_admin_password.c | $(BIN_DIR)
	$(CC) -o $@ $< -lsodium
	@echo "Built: $@"

# Generate compile_commands.json for IDE support
.PHONY: compile_commands
compile_commands:
	bear -- make clean all

# Show help
.PHONY: help
help:
	@echo "Coldnb Backend Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all (default) - Build release version"
	@echo "  debug         - Build with debug symbols"
	@echo "  release       - Build optimized release"
	@echo "  tests         - Build test executables"
	@echo "  test          - Build and run tests"
	@echo "  clean         - Remove build artifacts"
	@echo "  install       - Install to system (requires root)"
	@echo "  uninstall     - Remove from system"
	@echo "  format        - Format code with clang-format"
	@echo "  check         - Run static analysis"
	@echo "  help          - Show this help"

# Include dependency files
-include $(DEPS)
