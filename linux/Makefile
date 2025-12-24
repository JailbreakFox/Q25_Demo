# ============================================================================
# Q25 Demo Makefile - 本地编译 (x64)
# ============================================================================
# 使用方法:
#   make          - 编译所有demo
#   make all      - 编译所有demo
#   make clean    - 清理编译产物
#   make <name>   - 编译单个demo，如: make axis_control_demo
# ============================================================================

# 编译器配置
CXX      := g++
CXXFLAGS := -std=c++11 -pthread -Wall -O2
LDFLAGS  := -pthread

# 输出目录
BUILD_DIR := build/linux/x64

# Demo 列表
DEMOS := \
    auto_charge_demo \
    axis_control_demo \
    emergency_stop_demo \
    gait_switch_demo \
    height_control_demo \
    motion_mode_demo \
    power_control_demo \
    stand_lie_demo \
    status_receiver_demo

# 目标可执行文件（带路径）
TARGETS := $(addprefix $(BUILD_DIR)/, $(DEMOS))

# ============================================================================
# 规则
# ============================================================================

.PHONY: all clean help $(DEMOS)

all: $(TARGETS)
	@echo ""
	@echo "=========================================="
	@echo " 编译完成！可执行文件位于: $(BUILD_DIR)/"
	@echo "=========================================="

# 创建输出目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# 编译规则: build/linux/x64/xxx_demo <- xxx_demo.cpp
$(BUILD_DIR)/%: %.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# 快捷目标: make axis_control_demo
$(DEMOS): %: $(BUILD_DIR)/%

clean:
	@echo "[CLEAN] 清理编译产物..."
	@rm -rf $(BUILD_DIR)
	@echo "[CLEAN] 完成"

help:
	@echo ""
	@echo "Q25 Demo Makefile 使用说明"
	@echo "=========================================="
	@echo "  make          - 编译所有demo"
	@echo "  make all      - 编译所有demo"
	@echo "  make clean    - 清理编译产物"
	@echo "  make <name>   - 编译单个demo"
	@echo ""
	@echo "可用的demo:"
	@echo "  $(DEMOS)" | tr ' ' '\n' | sed 's/^/    /'
	@echo ""
	@echo "输出目录: $(BUILD_DIR)/"
	@echo "=========================================="

