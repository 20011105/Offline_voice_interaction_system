#!/bin/bash

# 快速编译脚本

echo "=== C++音频监控程序快速编译脚本 ==="
echo ""

# 检查依赖
echo "检查依赖..."

# 检查编译器
if ! command -v g++ &> /dev/null; then
    echo "错误：g++ 编译器未安装"
    echo "请运行: sudo apt install build-essential"
    exit 1
fi

# 检查PortAudio
if ! pkg-config --exists portaudio; then
    echo "错误：PortAudio 未安装"
    echo "请运行: sudo apt install libportaudio-dev"
    exit 1
fi

# 检查sherpa-onnx
if ! ldconfig -p | grep -q libsherpa-onnx; then
    echo "错误：sherpa-onnx 库未安装"
    echo "请先运行 ./install_dependencies_cpp.sh"
    exit 1
fi

echo "✅ 依赖检查通过"
echo ""

# 选择编译方式
echo "请选择编译方式："
echo "1) 使用Makefile (快速)"
echo "2) 使用CMake (推荐)"
echo "3) 直接编译"
echo ""
read -p "请输入选择 (1-3): " choice

case $choice in
    1)
        echo "使用Makefile编译..."
        make clean
        make release
        ;;
    2)
        echo "使用CMake编译..."
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc)
        cd ..
        ;;
    3)
        echo "直接编译..."
        g++ -std=c++17 -Wall -Wextra -O3 \
            -I/usr/local/include -I/usr/include \
            -o audio_monitor audio_monitor.cpp \
            -lportaudio -lsherpa-onnx-cxx-api -lpthread -lm
        ;;
    *)
        echo "无效选择，退出"
        exit 1
        ;;
esac

# 检查编译结果
echo ""
echo "=== 编译结果检查 ==="

if [ -f "audio_monitor" ]; then
    echo "✅ 编译成功: ./audio_monitor"
elif [ -f "build/audio_monitor" ]; then
    echo "✅ 编译成功: ./build/audio_monitor"
else
    echo "❌ 编译失败"
    exit 1
fi

# 测试程序
echo ""
echo "测试程序..."
if [ -f "audio_monitor" ]; then
    ./audio_monitor --help
elif [ -f "build/audio_monitor" ]; then
    ./build/audio_monitor --help
fi

echo ""
echo "=== 编译完成 ==="
echo "运行程序："
if [ -f "audio_monitor" ]; then
    echo "  ./audio_monitor"
    echo "  ./audio_monitor --list-devices"
elif [ -f "build/audio_monitor" ]; then
    echo "  ./build/audio_monitor"
    echo "  ./build/audio_monitor --list-devices"
fi 