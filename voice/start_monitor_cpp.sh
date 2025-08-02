#!/bin/bash

# C++音频监控程序启动脚本

echo "=== C++音频监控程序启动脚本 ==="
echo ""

# 检查可执行文件是否存在
if [ ! -f "build/audio_monitor" ]; then
    echo "错误：audio_monitor 可执行文件不存在"
    echo "请先运行 ./install_dependencies_cpp.sh 安装依赖并编译程序"
    exit 1
fi

# 检查模型目录是否存在
MODEL_DIR="models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16"
if [ ! -d "$MODEL_DIR" ]; then
    echo "警告：模型目录不存在: $MODEL_DIR"
    echo "请确保模型文件已正确下载"
    echo ""
fi

# 显示使用说明
echo "使用方法："
echo "1. 查看音频设备："
echo "   ./build/audio_monitor --list-devices"
echo ""
echo "2. 使用默认设备运行："
echo "   ./build/audio_monitor"
echo ""
echo "3. 指定设备运行："
echo "   ./build/audio_monitor --device <设备索引>"
echo ""
echo "4. 指定模型目录："
echo "   ./build/audio_monitor --model-dir <模型目录>"
echo ""

# 询问用户选择
echo "请选择操作："
echo "1) 查看音频设备"
echo "2) 使用默认设备启动监控"
echo "3) 指定设备启动监控"
echo "4) 指定模型目录启动监控"
echo "5) 退出"
echo ""
read -p "请输入选择 (1-5): " choice

case $choice in
    1)
        echo "正在列出音频设备..."
        ./build/audio_monitor --list-devices
        ;;
    2)
        echo "正在启动音频监控（默认设备）..."
        ./build/audio_monitor
        ;;
    3)
        read -p "请输入设备索引: " device_idx
        echo "正在启动音频监控（设备 $device_idx）..."
        ./build/audio_monitor --device $device_idx
        ;;
    4)
        read -p "请输入模型目录路径: " model_dir
        echo "正在启动音频监控（模型目录: $model_dir）..."
        ./build/audio_monitor --model-dir "$model_dir"
        ;;
    5)
        echo "退出程序"
        exit 0
        ;;
    *)
        echo "无效选择，退出程序"
        exit 1
        ;;
esac 