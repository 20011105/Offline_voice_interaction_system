// audio_monitor.h (修正版)
#ifndef AUDIO_MONITOR_H
#define AUDIO_MONITOR_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>      // <--- 1. 增加了 <atomic> 头文件
#include <portaudio.h> // <--- 2. 直接包含 <portaudio.h>
#include <sherpa-onnx/c-api/cxx-api.h>

struct AudioDevice {
    int index;
    std::string name;
    bool is_input;
};

class AudioMonitor {
public:
    AudioMonitor(const std::string& model_dir, const std::string& vad_model_path = "");
    ~AudioMonitor();
    void start_monitoring(int device_idx, const std::function<void(const std::string&)>& callback);

private:
    void init_models();
    void init_vad();
    void init_asr();
    std::string download_vad_model();
    bool file_exists(const std::string& path);
    std::vector<AudioDevice> list_audio_devices();

    std::string model_dir_;
    std::string vad_model_path_;
    int sample_rate_;
    int samples_per_read_;
    
    std::unique_ptr<sherpa_onnx::cxx::OnlineRecognizer> recognizer_;
    std::unique_ptr<sherpa_onnx::cxx::VoiceActivityDetector> vad_;
    std::unique_ptr<sherpa_onnx::cxx::OnlineStream> stream_;
    
    std::atomic<bool> is_speech_detected_; // 3. 移除了不必要的初始化
    std::string last_result_;
    
    PaStream* audio_stream_ = nullptr; // 4. 将 struct PaStream* 改为 PaStream*
};

#endif // AUDIO_MONITOR_H