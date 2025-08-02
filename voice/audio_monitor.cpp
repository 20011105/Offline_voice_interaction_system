// audio_monitor.cpp
// 音频监控和语音转文本程序 (C++版本)
// 功能：
// 1. 检测音频设备
// 2. 实时监听音频输入
// 3. 使用VAD检测语音活动
// 4. 将语音转换为文本并打印到终端

// audio_monitor.cpp (最终修正版)

#include "audio_monitor.h"
#include "globals.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <signal.h>

// 使用 using namespace 来简化代码
using namespace sherpa_onnx::cxx;

AudioMonitor::AudioMonitor(const std::string& model_dir, const std::string& vad_model_path)
    : model_dir_("/home/lx/桌面/Voice/LLM_Voice_Flow-master/voice/models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16"),
      vad_model_path_(vad_model_path),
      sample_rate_(16000),
      samples_per_read_(static_cast<int>(0.1 * 16000)),
      is_speech_detected_(false)
{
    init_models();
}

AudioMonitor::~AudioMonitor() {
    if (audio_stream_) {
        if (Pa_IsStreamActive(audio_stream_)) {
            Pa_StopStream(audio_stream_);
        }
        Pa_CloseStream(audio_stream_);
    }
}

void AudioMonitor::init_models() {
    std::cout << "正在初始化模型..." << std::endl;
    init_vad();
    init_asr();
    std::cout << "模型初始化完成！" << std::endl;
}

void AudioMonitor::init_vad() {
    if (vad_model_path_.empty()) {
        vad_model_path_ = download_vad_model();
    }
    VadModelConfig config;
    config.silero_vad.model = vad_model_path_;
    config.sample_rate = sample_rate_;
    config.silero_vad.threshold = 0.5;
    config.silero_vad.min_speech_duration = 0.25;
    config.silero_vad.min_silence_duration = 0.5;
    config.silero_vad.window_size = 512;

    // 1. 修正：为 Create 方法提供第二个参数 (缓冲区大小，单位：秒)
    vad_ = std::make_unique<VoiceActivityDetector>(
        VoiceActivityDetector::Create(config, 30.0f)
    );
    std::cout << "VAD模型初始化完成" << std::endl;
}

void AudioMonitor::init_asr() {
    OnlineRecognizerConfig config;
    config.model_config.transducer.encoder = model_dir_ + "/encoder-epoch-99-avg-1.int8.onnx";
    config.model_config.transducer.decoder = model_dir_ + "/decoder-epoch-99-avg-1.int8.onnx";
    config.model_config.transducer.joiner = model_dir_ + "/joiner-epoch-99-avg-1.int8.onnx";
    config.model_config.tokens = model_dir_ + "/tokens.txt";

    if (!file_exists(config.model_config.transducer.encoder)) {
        std::cout << "未找到int8量化模型，尝试使用fp32模型..." << std::endl;
        config.model_config.transducer.encoder = model_dir_ + "/encoder-epoch-99-avg-1.onnx";
        config.model_config.transducer.decoder = model_dir_ + "/decoder-epoch-99-avg-1.onnx";
        config.model_config.transducer.joiner = model_dir_ + "/joiner-epoch-99-avg-1.onnx";
    } else {
        std::cout << "使用int8量化模型以提高性能" << std::endl;
    }
    
    config.model_config.num_threads = 4;
    
    // 2. 修正：使用 Create 返回的对象来构造 unique_ptr
    recognizer_ = std::make_unique<OnlineRecognizer>(
        OnlineRecognizer::Create(config)
    );
    stream_ = std::make_unique<OnlineStream>(
        recognizer_->CreateStream()
    );
    std::cout << "ASR模型初始化完成" << std::endl;
}

std::string AudioMonitor::download_vad_model() {
    std::string vad_model_path = "/home/lx/桌面/Voice/LLM_Voice_Flow-master/voice/models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16/silero_vad.onnx";
    if (!file_exists(vad_model_path)) {
        std::cout << "正在下载VAD模型..." << std::endl;
        std::string cmd = "wget -q -O " + vad_model_path + " https://github.com/snakers4/silero-vad/raw/master/files/silero_vad.onnx";
        system(cmd.c_str());
        std::cout << "VAD模型下载完成" << std::endl;
    }
    return vad_model_path;
}

bool AudioMonitor::file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

std::vector<AudioDevice> AudioMonitor::list_audio_devices() {
    std::vector<AudioDevice> devices;
    int num_devices = Pa_GetDeviceCount();
    std::cout << "\n=== 可用的音频设备 ===" << std::endl;
    for (int i = 0; i < num_devices; ++i) {
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
        if (device_info && device_info->maxInputChannels > 0) {
            AudioDevice device;
            device.index = i;
            device.name = device_info->name;
            device.is_input = true;
            devices.push_back(device);
            std::cout << i << ": " << device.name << " (输入)" << std::endl;
        }
    }
    int default_input = Pa_GetDefaultInputDevice();
    if (default_input != paNoDevice) {
        const PaDeviceInfo* default_info = Pa_GetDeviceInfo(default_input);
        if (default_info) {
            std::cout << "\n默认输入设备: " << default_info->name << " (索引 " << default_input << ")" << std::endl;
        }
    }
    return devices;
}

void AudioMonitor::start_monitoring(int device_idx, const std::function<void(const std::string&)>& callback) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "!!! PortAudio 初始化失败: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    auto devices = list_audio_devices();
    if (devices.empty()) {
        std::cerr << "错误：未找到任何音频输入设备。" << std::endl;
        Pa_Terminate();
        return;
    }

    if (device_idx == -1) {
        device_idx = Pa_GetDefaultInputDevice();
        if (device_idx == paNoDevice) {
            std::cerr << "错误：没有默认输入设备，将使用第一个可用设备。" << std::endl;
            device_idx = devices[0].index;
        }
    }

    PaStreamParameters input_parameters;
    input_parameters.device = device_idx;
    input_parameters.channelCount = 1;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = 0.1;
    input_parameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&audio_stream_, &input_parameters, nullptr, sample_rate_,
                        samples_per_read_, paNoFlag, nullptr, nullptr);
    if (err != paNoError) {
        std::cerr << "打开音频流失败: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return;
    }

    err = Pa_StartStream(audio_stream_);
    if (err != paNoError) {
        std::cerr << "启动音频流失败: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(audio_stream_);
        Pa_Terminate();
        return;
    }
    
    std::cout << "\n成功打开设备: " << Pa_GetDeviceInfo(device_idx)->name << " (索引 " << device_idx << ")" << std::endl;
    std::cout << "请开始说话... (按Ctrl+C退出)" << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    std::vector<float> buffer(samples_per_read_);
    while (g_running) {
        err = Pa_ReadStream(audio_stream_, buffer.data(), samples_per_read_);
        if (err != paNoError) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if (g_is_tts_speaking) {
            continue;
        }
        
        vad_->AcceptWaveform(buffer.data(), buffer.size());

        if (vad_->IsDetected() && !is_speech_detected_) {
            std::cout << "\n🎤 检测到语音..." << std::endl;
            is_speech_detected_ = true;
            last_result_.clear();
        }

        while (!vad_->IsEmpty()) {
            auto segment = vad_->Front();
            // 3. 修正：使用类成员变量 sample_rate_
            stream_->AcceptWaveform(sample_rate_, segment.samples.data(), segment.samples.size());
            while (recognizer_->IsReady(stream_.get())) {
                recognizer_->Decode(stream_.get());
            }
            auto result = recognizer_->GetResult(stream_.get());
            if (!result.text.empty() && result.text != last_result_) {
                last_result_ = result.text;
                std::cout << "📝 识别结果: " << result.text << std::endl;
            }
            vad_->Pop();
        }
        
        if (!vad_->IsDetected() && is_speech_detected_) {
            std::cout << "🔇 语音结束" << std::endl;
            is_speech_detected_ = false;
            if (!last_result_.empty()) {
                callback(last_result_);
            }
            // 3. 修正 unique_ptr 的重新赋值
            stream_ = std::make_unique<OnlineStream>(recognizer_->CreateStream());
        }
    }

    Pa_StopStream(audio_stream_);
    Pa_CloseStream(audio_stream_);
    Pa_Terminate();
}

// #include <iostream>
// #include <string>
// #include <vector>
// #include <chrono>
// #include <thread>
// #include <atomic>
// #include <signal.h>
// #include <fstream>
// #include <cstdlib>
// #include <memory>
// #include <portaudio.h>
// #include <sherpa-onnx/c-api/cxx-api.h>
// #include <functional>

// using namespace sherpa_onnx::cxx;

// // 全局变量用于信号处理
// std::atomic<bool> g_running(true);
// // #include "audio_monitor.h"

// // 信号处理函数
// void signal_handler(int signal) {
//     if (signal == SIGINT || signal == SIGTERM) {
//         std::cout << "\n\n程序被用户中断" << std::endl;
//         g_running = false;
//     }
// }

// // 音频设备信息结构
// struct AudioDevice {
//     int index;
//     std::string name;
//     bool is_input;
//     int max_input_channels;
//     int max_output_channels;
//     double default_sample_rate;
// };

// // 音频监控器类
// class AudioMonitor {
// private:
//     std::string model_dir_ = "/home/lx/桌面/Voice/LLM_Voice_Flow-master/voice/models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16";
//     std::string vad_model_path_;
//     int sample_rate_;
//     int samples_per_read_;
    
//     std::unique_ptr<OnlineRecognizer> recognizer_;
//     std::unique_ptr<VoiceActivityDetector> vad_;
//     std::unique_ptr<OnlineStream> stream_;
    
//     std::atomic<bool> is_speech_detected_;
//     std::string last_result_;
    
//     // PortAudio相关
//     PaStream* audio_stream_;
    
// public:
//     AudioMonitor(const std::string& model_dir, const std::string& vad_model_path = "")
//         // : model_dir_(model_dir)
//         : vad_model_path_(vad_model_path)
//         , sample_rate_(16000)
//         , samples_per_read_(static_cast<int>(0.1 * sample_rate_)) // 200ms
//         , is_speech_detected_(false)
//         , audio_stream_(nullptr)
//     {
//         init_models();
//     }
    
//     ~AudioMonitor() {
//         if (audio_stream_) {
//             Pa_CloseStream(audio_stream_);
//         }
//     }
    
//     // 初始化模型
//     void init_models() {
//         std::cout << "正在初始化模型..." << std::endl;
        
//         // 初始化VAD
//         init_vad();
        
//         // 初始化ASR
//         init_asr();
        
//         std::cout << "模型初始化完成！" << std::endl;
//     }
    
//     // 初始化VAD模型
//     void init_vad() {
//         if (vad_model_path_.empty()) {
//             vad_model_path_ = download_vad_model();
//         }
        
//         VadModelConfig config;
//         config.silero_vad.model = vad_model_path_;
//         config.sample_rate = sample_rate_;
//         config.silero_vad.threshold = 0.5;
//         config.silero_vad.min_speech_duration = 0.25;
//         config.silero_vad.min_silence_duration = 0.5;
//         config.silero_vad.window_size = 512;
//         config.debug = false;
        
//         vad_ = std::make_unique<VoiceActivityDetector>(VoiceActivityDetector::Create(config, 30));
//         if (!vad_->Get()) {
//             std::cerr << "错误：VAD模型初始化失败" << std::endl;
//             exit(-1);
//         }
        
//         std::cout << "VAD模型初始化完成" << std::endl;
//     }
    
//     // 初始化ASR模型
//     void init_asr() {
//         OnlineRecognizerConfig config;
        
//         // 检查模型文件
//         std::string encoder_path = model_dir_ + "/encoder-epoch-99-avg-1.onnx";
//         std::string decoder_path = model_dir_ + "/decoder-epoch-99-avg-1.onnx";
//         std::string joiner_path = model_dir_ + "/joiner-epoch-99-avg-1.onnx";
//         std::string tokens_path = model_dir_ + "/tokens.txt";
        
//         // 如果int8模型存在，使用int8模型以提高性能
//         std::string encoder_int8_path = model_dir_ + "/encoder-epoch-99-avg-1.int8.onnx";
//         std::string decoder_int8_path = model_dir_ + "/decoder-epoch-99-avg-1.int8.onnx";
//         std::string joiner_int8_path = model_dir_ + "/joiner-epoch-99-avg-1.int8.onnx";
        
//         // 检查int8模型是否存在
//         if (file_exists(encoder_int8_path) && file_exists(decoder_int8_path) && file_exists(joiner_int8_path)) {
//             encoder_path = encoder_int8_path;
//             decoder_path = decoder_int8_path;
//             joiner_path = joiner_int8_path;
//             std::cout << "使用int8量化模型以提高性能" << std::endl;
//         }
        
//         // 验证模型文件
//         if (!file_exists(encoder_path) || !file_exists(decoder_path) || 
//             !file_exists(joiner_path) || !file_exists(tokens_path)) {
//             std::cerr << "错误：模型文件不存在" << std::endl;
//             std::cerr << "请确保以下文件存在：" << std::endl;
//             std::cerr << "  " << encoder_path << std::endl;
//             std::cerr << "  " << decoder_path << std::endl;
//             std::cerr << "  " << joiner_path << std::endl;
//             std::cerr << "  " << tokens_path << std::endl;
//             exit(-1);
//         }
        
//         // 配置识别器
//         config.model_config.transducer.encoder = encoder_path;
//         config.model_config.transducer.decoder = decoder_path;
//         config.model_config.transducer.joiner = joiner_path;
//         config.model_config.tokens = tokens_path;
//         config.model_config.num_threads = 4;
//         config.model_config.debug = false;
        
//         recognizer_ = std::make_unique<OnlineRecognizer>(OnlineRecognizer::Create(config));
//         if (!recognizer_->Get()) {
//             std::cerr << "错误：ASR模型初始化失败" << std::endl;
//             exit(-1);
//         }
        
//         stream_ = std::make_unique<OnlineStream>(recognizer_->CreateStream());
//         std::cout << "ASR模型初始化完成" << std::endl;
//     }
    
//     // 下载VAD模型
//     std::string download_vad_model() {
//         std::string vad_model_path = "/home/lx/桌面/Voice/LLM_Voice_Flow-master/voice/models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16/silero_vad.onnx";
//         std::string vad_model_url = "https://github.com/snakers4/silero-vad/raw/master/src/silero_vad/data/silero_vad.onnx";
        
//         if (!file_exists(vad_model_path)) {
//             std::cout << "正在下载VAD模型..." << std::endl;
//             std::string cmd = "wget -O " + vad_model_path + " " + vad_model_url;
//             int result = system(cmd.c_str());
//             if (result != 0) {
//                 std::cerr << "下载VAD模型失败，请手动下载silero_vad.onnx文件" << std::endl;
//                 exit(-1);
//             }
//             std::cout << "VAD模型下载完成" << std::endl;
//         }
        
//         return vad_model_path;
//     }
    
//     // 检查文件是否存在
//     bool file_exists(const std::string& path) {
//         std::ifstream file(path);
//         return file.good();
//     }
    
//     // 列出音频设备
//     std::vector<AudioDevice> list_audio_devices() {
//         std::vector<AudioDevice> devices;
        
//         int num_devices = Pa_GetDeviceCount();
//         std::cout << "\n=== 可用的音频设备 ===" << std::endl;
        
//         for (int i = 0; i < num_devices; ++i) {
//             const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
//             if (device_info) {
//                 AudioDevice device;
//                 device.index = i;
//                 device.name = device_info->name;
//                 device.is_input = (device_info->maxInputChannels > 0);
//                 device.max_input_channels = device_info->maxInputChannels;
//                 device.max_output_channels = device_info->maxOutputChannels;
//                 device.default_sample_rate = device_info->defaultSampleRate;
                
//                 std::string device_type = device.is_input ? "输入" : "输出";
//                 std::cout << i << ": " << device.name << " (" << device_type << ")" << std::endl;
                
//                 devices.push_back(device);
//             }
//         }
        
//         int default_input = Pa_GetDefaultInputDevice();
//         if (default_input != paNoDevice) {
//             const PaDeviceInfo* default_info = Pa_GetDeviceInfo(default_input);
//             if (default_info) {
//                 std::cout << "\n默认输入设备: " << default_info->name << std::endl;
//             }
//         }
        
//         return devices;
//     }
    

//     // 在 AudioMonitor 类中，我们需要修改 start_monitoring，让它接受一个回调
//     // 开始音频监控
//     void start_monitoring(int device_idx, const std::function<void(const std::string&)>& callback) {
//         // 1. 首先，初始化 PortAudio
//         PaError err = Pa_Initialize();
//         if (err != paNoError) {
//             std::cerr << "!!! PortAudio 初始化失败，错误信息: " << Pa_GetErrorText(err) << std::endl;
//             return;
//         }

//         // 2. 初始化成功后，再获取设备列表
//         auto devices = list_audio_devices();

//         // 如果设备列表为空，则没有可用的设备
//         if (devices.empty()) {
//             std::cerr << "错误：未找到任何音频输入设备。" << std::endl;
//             Pa_Terminate(); // 退出前终止PortAudio
//             return;
//         }

//         // 3. 如果未指定设备，则获取默认输入设备
//         if (device_idx == -1) {
//             device_idx = Pa_GetDefaultInputDevice();
//             if (device_idx == paNoDevice) {
//                 std::cerr << "错误：没有默认的音频输入设备，请通过 --device 参数指定一个。" << std::endl;
//                 Pa_Terminate();
//                 return;
//             }
//         }

//         // 4. 检查最终的设备索引是否有效
//         if (device_idx >= static_cast<int>(devices.size()) || device_idx < 0) {
//             std::cerr << "错误：设备索引 " << device_idx << " 超出范围。" << std::endl;
//             Pa_Terminate();
//             return;
//         }

//         // 5. 配置并打开指定的音频流 (使用 Pa_OpenStream)
//         PaStreamParameters input_parameters;
//         input_parameters.device = device_idx;
//         input_parameters.channelCount = 1;
//         input_parameters.sampleFormat = paFloat32;
//         input_parameters.suggestedLatency = 0.1; // 100ms 延迟
//         input_parameters.hostApiSpecificStreamInfo = nullptr;

//         err = Pa_OpenStream(
//             &audio_stream_,
//             &input_parameters,
//             nullptr, // 没有输出
//             sample_rate_,
//             samples_per_read_,
//             paNoFlag,
//             nullptr,
//             nullptr
//         );

//         if (err != paNoError) {
//             std::cerr << "打开音频流失败: " << Pa_GetErrorText(err) << std::endl;
//             Pa_Terminate();
//             return;
//         }

//         std::string device_name = devices[device_idx].name;
//         std::cout << "\n成功打开设备: " << device_name << " (索引 " << device_idx << ")" << std::endl;
//         std::cout << "请开始说话... (按Ctrl+C退出)" << std::endl;
//         std::cout << "--------------------------------------------------" << std::endl;

//         // 6. 启动音频流
//         err = Pa_StartStream(audio_stream_);
//         if (err != paNoError) {
//             std::cerr << "启动音频流失败: " << Pa_GetErrorText(err) << std::endl;
//             Pa_CloseStream(audio_stream_);
//             Pa_Terminate();
//             return;
//         }

//         // 7. 音频处理循环 (保持不变)
//         std::vector<float> buffer(samples_per_read_);
//         while (g_running) {
//             err = Pa_ReadStream(audio_stream_, buffer.data(), samples_per_read_);
//             if (err != paNoError) {
//                 std::cerr << "读取音频数据失败: " << Pa_GetErrorText(err) << std::endl;
//                 break;
//             }

//             vad_->AcceptWaveform(buffer.data(), buffer.size());

//             if (vad_->IsDetected() && !is_speech_detected_) {
//                 std::cout << "\n🎤 检测到语音..." << std::endl;
//                 is_speech_detected_ = true;
//                 last_result_.clear();
//             }

//             while (!vad_->IsEmpty()) {
//                 auto segment = vad_->Front();
//                 stream_->AcceptWaveform(sample_rate_, segment.samples.data(), segment.samples.size());
//                 while (recognizer_->IsReady(stream_.get())) {
//                     recognizer_->Decode(stream_.get());
//                 }
//                 OnlineRecognizerResult result = recognizer_->GetResult(stream_.get());
//                 if (!result.text.empty() && result.text != last_result_) {
//                     last_result_ = result.text;
//                     std::cout << "📝 识别结果: " << result.text << std::endl;
//                 }
//                 vad_->Pop();
//             }
//             // 在 AudioMonitor 类中，我们需要修改 start_monitoring，让它接受一个回调
//             /*#############################################################################*/
//             if (!vad_->IsDetected() && is_speech_detected_) {
//                 std::cout << "🔇 语音结束" << std::endl;
//                 is_speech_detected_ = false;
//                 if (!last_result_.empty()) {
//                     callback(last_result_); // 调用回调函数
//                 }
//                 stream_ = std::make_unique<OnlineStream>(recognizer_->CreateStream());
//             }
//         }

//         // 8. 清理资源
//         Pa_StopStream(audio_stream_);
//         Pa_CloseStream(audio_stream_);
//         Pa_Terminate();
//     }
// };

// int main(int argc, char* argv[]) {
//     // 设置信号处理
//     signal(SIGINT, signal_handler);
//     signal(SIGTERM, signal_handler);
    
//     // 解析命令行参数
//     std::string model_dir = "models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16";
//     std::string vad_model_path = "";
//     int device_idx = -1;
//     bool list_devices = false;
    
//     for (int i = 1; i < argc; ++i) {
//         std::string arg = argv[i];
//         if (arg == "--model-dir" && i + 1 < argc) {
//             model_dir = argv[++i];
//         } else if (arg == "--vad-model" && i + 1 < argc) {
//             vad_model_path = argv[++i];
//         } else if (arg == "--device" && i + 1 < argc) {
//             device_idx = std::stoi(argv[++i]);
//         } else if (arg == "--list-devices") {
//             list_devices = true;
//         } else if (arg == "--help" || arg == "-h") {
//             std::cout << "音频监控和语音转文本程序 (C++版本)" << std::endl;
//             std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
//             std::cout << "选项:" << std::endl;
//             std::cout << "  --model-dir DIR     语音识别模型目录" << std::endl;
//             std::cout << "  --vad-model PATH    VAD模型文件路径" << std::endl;
//             std::cout << "  --device INDEX      音频设备索引" << std::endl;
//             std::cout << "  --list-devices      列出所有音频设备并退出" << std::endl;
//             std::cout << "  --help, -h          显示此帮助信息" << std::endl;
//             return 0;
//         }
//     }
    
//     try {
//         // 创建音频监控器
//         AudioMonitor monitor(model_dir, vad_model_path);
        
//         if (list_devices) {
//             monitor.list_audio_devices();
//             return 0;
//         }
        
//         // 开始监控
//         monitor.start_monitoring(device_idx);
        
//     } catch (const std::exception& e) {
//         std::cerr << "错误: " << e.what() << std::endl;
//         return -1;
//     }
    
//     return 0;
// } 