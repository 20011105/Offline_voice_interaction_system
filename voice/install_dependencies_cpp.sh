#!/bin/bash

# Ubuntu音频监控和语音转文本程序依赖安装脚本 (C++版本)

echo "=== Ubuntu音频监控程序依赖安装脚本 (C++版本) ==="
echo "此脚本将安装运行C++音频监控程序所需的所有依赖"
echo ""

# 检查是否为Ubuntu系统
if ! grep -q "Ubuntu" /etc/os-release; then
    echo "警告：此脚本专为Ubuntu系统设计，其他系统可能需要不同的安装方法"
    echo ""
fi

# 更新包管理器
echo "正在更新包管理器..."
sudo apt update

# 安装系统依赖
echo "正在安装系统依赖..."
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    libportaudio2 \
    libportaudio-dev \
    libasound2-dev \
    libsndfile1 \
    libsndfile1-dev \
    ffmpeg

# 安装sherpa-onnx
echo "正在安装sherpa-onnx..."
echo "注意：sherpa-onnx需要从源码编译，这可能需要一些时间..."

# 克隆sherpa-onnx仓库
# if [ ! -d "sherpa-onnx" ]; then
#     echo "正在克隆sherpa-onnx仓库..."
#     git clone https://github.com/k2-fsa/sherpa-onnx.git
# fi

cd sherpa-onnx

# 创建构建目录
mkdir -p build
cd build

# 配置CMake
echo "正在配置sherpa-onnx..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSHERPA_ONNX_ENABLE_PYTHON=OFF \
      -DSHERPA_ONNX_ENABLE_TESTS=OFF \
      -DSHERPA_ONNX_ENABLE_C_API=ON \
      -DSHERPA_ONNX_ENABLE_CXX_API=ON \
      ..

# 编译
echo "正在编译sherpa-onnx..."
make -j$(nproc)

# 安装
echo "正在安装sherpa-onnx..."
sudo make install

# 更新动态链接库缓存
sudo ldconfig

cd ../..

# 编译音频监控程序
echo "正在编译音频监控程序..."
mkdir -p build
cd build

cmake ..
make

cd ..

# 检查编译结果
echo ""
echo "=== 安装完成检查 ==="

# 检查可执行文件
if [ -f "build/audio_monitor" ]; then
    echo "✅ 音频监控程序编译成功"
else
    echo "❌ 音频监控程序编译失败"
fi

# 检查依赖库
echo ""
echo "检查依赖库："
if ldconfig -p | grep -q libportaudio; then
    echo "✅ PortAudio 库已安装"
else
    echo "❌ PortAudio 库未找到"
fi

if ldconfig -p | grep -q libsherpa-onnx; then
    echo "✅ sherpa-onnx 库已安装"
else
    echo "❌ sherpa-onnx 库未找到"
fi

# 检查音频设备
echo ""
echo "检查音频设备："
if [ -f "build/audio_monitor" ]; then
    ./build/audio_monitor --list-devices 2>/dev/null || echo "无法检查音频设备"
fi

echo ""
echo "=== 安装完成 ==="
echo "现在您可以运行C++音频监控程序："
echo "./build/audio_monitor"
echo ""
echo "或者查看可用设备："
echo "./build/audio_monitor --list-devices"
echo ""
echo "编译选项："
echo "  Debug版本: cmake -DCMAKE_BUILD_TYPE=Debug .. && make"
echo "  Release版本: cmake -DCMAKE_BUILD_TYPE=Release .. && make" 