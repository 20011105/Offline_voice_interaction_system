# 音频监控和语音转文本程序 (C++版本)

这是一个基于sherpa-onnx的实时音频监控程序，使用C++实现，能够检测音频设备、监听语音活动并将语音转换为文本。

## 功能特性

- 🎤 **实时音频监控**：检测耳机、麦克风等音频设备
- 🗣️ **语音活动检测**：使用VAD（Voice Activity Detection）检测语音
- 📝 **语音转文本**：将检测到的语音实时转换为文本
- 🔧 **设备管理**：支持选择不同的音频输入设备
- 🚀 **高性能**：C++实现，支持int8量化模型以提高性能
- ⚡ **低延迟**：优化的音频处理管道

## 系统要求

- Ubuntu 18.04 或更高版本
- GCC 7.0 或更高版本
- CMake 3.10 或更高版本
- 音频输入设备（麦克风、耳机等）

## 快速开始

### 1. 安装依赖

运行安装脚本：

```bash
chmod +x install_dependencies_cpp.sh
./install_dependencies_cpp.sh
```

或者手动安装：

```bash
# 安装系统依赖
sudo apt update
sudo apt install -y build-essential cmake pkg-config git wget \
    libportaudio2 libportaudio-dev libasound2-dev \
    libsndfile1 libsndfile1-dev ffmpeg

# 编译安装sherpa-onnx
git clone https://github.com/k2-fsa/sherpa-onnx.git
cd sherpa-onnx
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSHERPA_ONNX_ENABLE_PYTHON=OFF \
      -DSHERPA_ONNX_ENABLE_TESTS=OFF \
      -DSHERPA_ONNX_ENABLE_C_API=ON \
      -DSHERPA_ONNX_ENABLE_CXX_API=ON \
      ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..

# 编译音频监控程序
mkdir build && cd build
cmake ..
make
cd ..
```

### 2. 检查音频设备

```bash
./build/audio_monitor --list-devices
```

### 3. 运行程序

使用默认设备：
```bash
./build/audio_monitor
```

指定特定设备：
```bash
./build/audio_monitor --device 1
```

指定模型目录：
```bash
./build/audio_monitor --model-dir models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16
```

## 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--model-dir` | 语音识别模型目录 | `models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16` |
| `--vad-model` | VAD模型文件路径 | 自动下载 |
| `--device` | 音频设备索引 | 默认设备 |
| `--list-devices` | 列出所有音频设备并退出 | False |
| `--help, -h` | 显示帮助信息 | False |

## 使用示例

### 基本使用

1. 运行程序：
   ```bash
   ./build/audio_monitor
   ```

2. 开始说话，程序会：
   - 显示 "🎤 检测到语音..."
   - 实时显示识别结果
   - 显示 "🔇 语音结束"

3. 按 `Ctrl+C` 退出程序

### 高级使用

指定特定音频设备：
```bash
# 首先查看可用设备
./build/audio_monitor --list-devices

# 使用设备索引1
./build/audio_monitor --device 1
```

使用自定义模型：
```bash
./build/audio_monitor --model-dir /path/to/your/models
```

## 编译选项

### Debug版本
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release版本（推荐）
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### 自定义编译
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
make -j$(nproc)
```

## 模型文件

程序需要以下模型文件：

### 语音识别模型
- `encoder-epoch-99-avg-1.onnx` (或 `.int8.onnx`)
- `decoder-epoch-99-avg-1.onnx` (或 `.int8.onnx`)
- `joiner-epoch-99-avg-1.onnx` (或 `.int8.onnx`)
- `tokens.txt`

### VAD模型
- `silero_vad.onnx` (程序会自动下载)

## 性能优化

### 编译优化
```bash
# 使用最高优化级别
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native -ffast-math" ..
```

### 运行时优化
- 使用int8量化模型（程序会自动检测）
- 调整线程数（在代码中修改 `num_threads` 参数）
- 调整VAD参数（修改 `threshold`、`min_speech_duration` 等参数）

## 故障排除

### 常见问题

1. **"PortAudio not found"**
   ```bash
   sudo apt install libportaudio-dev
   ```

2. **"sherpa-onnx not found"**
   ```bash
   # 重新编译安装sherpa-onnx
   cd sherpa-onnx
   rm -rf build
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release \
         -DSHERPA_ONNX_ENABLE_PYTHON=OFF \
         -DSHERPA_ONNX_ENABLE_TESTS=OFF \
         -DSHERPA_ONNX_ENABLE_C_API=ON \
         -DSHERPA_ONNX_ENABLE_CXX_API=ON \
         ..
   make -j$(nproc)
   sudo make install
   sudo ldconfig
   ```

3. **"模型文件不存在"**
   - 确保模型文件已正确下载
   - 检查 `--model-dir` 参数路径是否正确

4. **音频权限问题**
   ```bash
   # 将用户添加到audio组
   sudo usermod -a -G audio $USER
   # 重新登录或重启系统
   ```

5. **编译错误**
   ```bash
   # 清理并重新编译
   rm -rf build
   mkdir build && cd build
   cmake ..
   make clean
   make
   ```

### 调试技巧

1. **启用调试输出**
   ```bash
   # 在代码中设置 config.debug = true
   ```

2. **检查音频设备**
   ```bash
   ./build/audio_monitor --list-devices
   ```

3. **测试音频输入**
   ```bash
   # 使用arecord测试麦克风
   arecord -d 5 -f S16_LE -r 16000 test.wav
   aplay test.wav
   ```

## 技术细节

- **编程语言**：C++17
- **音频库**：PortAudio
- **语音识别**：sherpa-onnx
- **采样率**：16kHz
- **音频格式**：单声道，float32
- **VAD阈值**：0.5（可调整）
- **最小语音时长**：0.25秒
- **最小静音时长**：0.5秒

## 与Python版本的区别

| 特性 | C++版本 | Python版本 |
|------|---------|------------|
| 性能 | 更高 | 中等 |
| 内存使用 | 更低 | 较高 |
| 启动时间 | 更快 | 较慢 |
| 开发难度 | 较高 | 较低 |
| 依赖管理 | 复杂 | 简单 |
| 跨平台 | 需要重新编译 | 相对容易 |

## 许可证

本项目基于sherpa-onnx，遵循相应的开源许可证。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。 