cmake_minimum_required(VERSION 3.10)

# 项目名称
project(audio_monitor)

# 1. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 查找 PortAudio (依然使用 PkgConfig)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(ZMQ REQUIRED libzmq)


# 使用 find_library 直接查找 ZMQ 库
#find_library(ZMQ_LIBRARY NAMES zmq)
#if(NOT ZMQ_LIBRARY)
#    message(FATAL_ERROR "ZeroMQ library (libzmq) not found.")
#endif()


# 3. 添加可执行文件
# add_executable(audio_monitor audio_monitor.cpp)
add_executable(voice_assistant
  audio_main.cpp
  audio_monitor.cpp
)

# 4. 添加头文件目录
target_include_directories(voice_assistant
  PRIVATE
    "." 
    "/usr/local/include"      # sherpa-onnx, cargs, 和你的ZMQ组件头文件
    ${PORTAUDIO_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
)


# 5. 手动指定库的链接顺序 (这是最关键的修正)
# 我们将手动、按正确的依赖顺序列出所有需要的库
target_link_libraries(voice_assistant
  PRIVATE
    # PortAudio 库
    ${PORTAUDIO_LIBRARIES}
    # 您自己的ZMQ组件库
    zmq_component

    # sherpa-onnx 库，从高层到底层排列
    sherpa-onnx-cxx-api
    sherpa-onnx-c-api
    sherpa-onnx-core
    sherpa-onnx-kaldifst-core
    kaldi-native-fbank-core
    kaldi-decoder-core
    sherpa-onnx-fst
    sherpa-onnx-fstfar
    onnxruntime
    piper_phonemize
    espeak-ng
    ssentencepiece_core
    ucd
    cargs
    # zmq_component

    # 系统的ZMQ库 (因为 zmq_component 和我们代码都依赖它)
    ${ZMQ_LIBRARIES}

    # 底层系统库
    pthread
    dl
    m
    rt
)