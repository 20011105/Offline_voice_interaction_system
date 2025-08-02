# Makefile for audio_monitor (C++ version)
# 简单的编译配置，适用于快速编译

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
INCLUDES = -I/usr/local/include -I/usr/include
LIBS = -lportaudio -lsherpa-onnx-cxx-api -lpthread -lm

# 目标文件
TARGET = audio_monitor
SOURCE = audio_monitor.cpp

# 默认目标
all: $(TARGET)

# 编译目标
$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# Debug版本
debug: CXXFLAGS += -g -O0
debug: $(TARGET)

# Release版本
release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

# 安装
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# 清理
clean:
	rm -f $(TARGET)

# 运行
run: $(TARGET)
	./$(TARGET)

# 列出设备
list-devices: $(TARGET)
	./$(TARGET) --list-devices

# 帮助
help:
	@echo "可用的目标："
	@echo "  all          - 编译程序 (默认)"
	@echo "  debug         - 编译Debug版本"
	@echo "  release       - 编译Release版本"
	@echo "  install       - 安装到系统"
	@echo "  clean         - 清理编译文件"
	@echo "  run           - 运行程序"
	@echo "  list-devices  - 列出音频设备"
	@echo "  help          - 显示此帮助信息"

.PHONY: all debug release install clean run list-devices help 