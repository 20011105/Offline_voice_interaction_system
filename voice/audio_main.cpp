#include "globals.h"       // 包含我们创建的全局变量头文件
#include "audio_monitor.h" // 包含AudioMonitor的头文件
#include "ZmqClient.h"     // 您的ZMQ客户端头文件
#include <iostream>
#include <functional>
#include <thread>
#include <signal.h>
#include <memory>
#include <zmq.hpp> // 确保包含了zmq.hpp

// --- 全局变量定义 ---
// 定义在 globals.h 中声明的全局变量
std::atomic<bool> g_running(true);
std::atomic<bool> g_is_tts_speaking(false);

// 定义ZMQ客户端指针
std::unique_ptr<zmq_component::ZmqClient> g_zmq_client;


// --- 函数实现 ---

// 定义信号处理函数
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\n程序被用户中断" << std::endl;
        g_running = false;
    }
}

// 线程函数：专门用于监听TTS服务的状态更新
void tts_status_listener() {
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    subscriber.connect("tcp://localhost:6677");
    subscriber.set(zmq::sockopt::subscribe, "STATUS::");

    std::cout << "[Status] 状态监听线程已启动，正在监听 TTS 状态..." << std::endl;

    while (g_running) {
        zmq::message_t topic;
        if (subscriber.recv(topic, zmq::recv_flags::dontwait)) {
            std::string status = topic.to_string();
            if (status == "STATUS::SPEAKING") {
                g_is_tts_speaking = true;
                std::cout << "[Status] TTS正在讲话，暂停识别..." << std::endl;
            } else if (status == "STATUS::IDLE") {
                g_is_tts_speaking = false;
                std::cout << "[Status] TTS已结束，恢复识别。" << std::endl;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    std::cout << "[Status] 状态监听线程已退出。" << std::endl;
}

// 回调函数：当ASR识别出完整一句话后，此函数被调用
void on_speech_recognized(const std::string& text) {
    if (text.empty()) {
        return;
    }
    std::cout << "\n[ASR] 识别到最终文本: " << text << std::endl;
    if (!g_zmq_client) {
        std::cerr << "[错误] ZMQ客户端未初始化！" << std::endl;
        return;
    }
    try {
        std::cout << "[ZMQ] 正在发送给Windows LLM服务..." << std::endl;
        std::string response = g_zmq_client->request(text);
        std::cout << "\n🤖 LLM 确认: " << response << "\n" << std::endl;
    } catch (const zmq_component::ZmqCommunicationError& e) {
        std::cerr << "[ZMQ] 通信错误: " << e.what() << std::endl;
    }
}

// --- 主函数 ---
int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::string server_address = "tcp://192.168.118.1:6666";
    int device_idx = -1; 

    try {
        g_zmq_client = std::make_unique<zmq_component::ZmqClient>(server_address);
        g_zmq_client->setTimeout(15000); 
    } catch(const std::exception& e) {
        std::cerr << "初始化ZMQ客户端失败: " << e.what() << std::endl;
        return -1;
    }
    
    AudioMonitor monitor("./models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16");
    
    std::cout << "===== 语音助手已启动 (v3.0 Refactored) =====" << std::endl;
    
    std::thread status_thread(tts_status_listener);
    monitor.start_monitoring(device_idx, on_speech_recognized);

    std::cout << "主监控循环已退出，正在等待状态监听线程结束..." << std::endl;
    status_thread.join();
    
    std::cout << "程序已完全退出。" << std::endl;
    return 0;
}



// #include "audio_monitor.cpp" // 直接包含cpp以简化编译，或者在CMake中链接
// #include "ZmqClient.h"     // 您的ZMQ客户端头文件
// #include <iostream>
// #include <functional>
// #include <atomic>
// #include <thread>           // 确保为新线程包含头文件
// #include <signal.h>


// // 全局ZMQ客户端指针
// std::unique_ptr<zmq_component::ZmqClient> g_zmq_client;
// // 全局状态，控制ASR是否工作
// std::atomic<bool> g_is_tts_speaking(false);

// // 当ASR识别出完整一句话后，此函数被调用
// void on_speech_recognized(const std::string& text) {
//     if (text.empty()) {
//         return;
//     }

//     std::cout << "\n[ASR] 识别到最终文本: " << text << std::endl;
    
//     if (!g_zmq_client) {
//         std::cerr << "[错误] ZMQ客户端未初始化！" << std::endl;
//         return;
//     }

//     try {
//         std::cout << "[ZMQ] 正在发送给Windows LLM服务..." << std::endl;
//         // 发送请求并阻塞等待回复
//         std::string response = g_zmq_client->request(text);
//         std::cout << "\n🤖 LLM 回答: " << response << "\n" << std::endl;
//     } catch (const zmq_component::ZmqCommunicationError& e) {
//         std::cerr << "[ZMQ] 通信错误: " << e.what() << std::endl;
//         std::cerr << "      请确保Windows端的LLM服务正在运行，并且IP地址正确。" << std::endl;
//     }
// }

// // 用于监听TTS状态的独立线程
// void tts_status_listener() {
//     zmq::context_t context(1);
//     zmq::socket_t subscriber(context, zmq::socket_type::sub);
//     subscriber.connect("tcp://localhost:6677"); // 连接到TTS的PUB端口
//     subscriber.set(zmq::sockopt::subscribe, "STATUS::"); // 只接收STATUS::开头的消息

//     while (g_running) {
//         zmq::message_t topic;
//         // 尝试接收消息，设置一个超时以避免永久阻塞
//         if (subscriber.recv(topic, zmq::recv_flags::dontwait)) {
//             std::string status = topic.to_string();
            
//             if (status == "STATUS::SPEAKING") {
//                 g_is_tts_speaking = true;
//                 std::cout << "[Status] TTS正在讲话，暂停识别..." << std::endl;
//             } else if (status == "STATUS::IDLE") {
//                 g_is_tts_speaking = false;
//                 std::cout << "[Status] TTS已结束，恢复识别。" << std::endl;
//             }
//         } else {
//             // 在没有消息时短暂休眠，避免CPU空转
//             std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         }
//     }
//     std::cout << "[Status] 状态监听线程已退出。" << std::endl;
// }

// int main(int argc, char* argv[]) {
//     // !!! 重要：请将 "WINDOWS_IP_ADDRESS" 替换为您的Windows主机的IP地址 !!!
//     // 例如: "tcp://192.168.1.100:6666"
//     std::string server_address = "tcp://192.168.118.1:6666";
//     int device_idx = -1; // 从命令行解析或使用默认值-1

//     // 初始化ZMQ客户端
//     try {
//         g_zmq_client = std::make_unique<zmq_component::ZmqClient>(server_address);
//         // 设置超时，例如5秒，以防LLM处理过慢导致永久等待
//         g_zmq_client->setTimeout(5000); 
//     } catch(const std::exception& e) {
//         std::cerr << "初始化ZMQ客户端失败: " << e.what() << std::endl;
//         return -1;
//     }
    
//     // 初始化音频监控器
//     // 假设 AudioMonitor 的构造函数已经被修改，不再硬编码路径
//     AudioMonitor monitor("./models/sherpa-onnx-streaming-zipformer-small-bilingual-zh-en-2023-02-16");
    
//     // --- 启动线程 ---
//     std::cout << "===== 语音助手已启动 (v2.0 PUB/SUB) =====" << std::endl;
//     std::cout << "按 Ctrl+C 退出程序。" << std::endl;
//     // 1. 在后台启动TTS状态监听线程
//     std::thread status_thread(tts_status_listener);
    

//     // 这里我们需要一种新的循环机制来驱动ASR并调用 on_speech_recognized
//     // 以下是一个简化的演示逻辑，您需要将其整合进您的 AudioMonitor 类中
//     // 为了让您能直接运行，我们在这里模拟一个简化的调用流程。
//     // 您需要将 audio_monitor.cpp 的 start_monitoring 改造为非阻塞的，或者能返回结果的函数。
//     // ---
//     // 假设改造后的 `monitor.get_one_utterance()` 会阻塞直到识别一句话
//     // while(true) {
//     //    std::string text = monitor.get_one_utterance();
//     //    on_speech_recognized(text);
//     // }
//     // ---

//     // 鉴于您已有的代码，我们直接启动它，并假设您已在其中加入了回调逻辑
//     // 请确保您的 start_monitoring 函数在识别到最终文本后能调用 on_speech_recognized
//     monitor.start_monitoring(device_idx,on_speech_recognized); // 使用默认设备
//     // --- 清理 ---
//     std::cout << "主监控循环已退出，正在等待状态监听线程结束..." << std::endl;
//     // 等待状态监听线程正常结束
//     status_thread.join();
    
//     std::cout << "程序已完全退出。" << std::endl;

//     return 0;
// }