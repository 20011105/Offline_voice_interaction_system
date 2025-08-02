// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <atomic>

// 使用 extern 关键字声明全局变量，表示它的“实体”在别处定义
extern std::atomic<bool> g_running;
extern std::atomic<bool> g_is_tts_speaking;

// 声明信号处理函数
void signal_handler(int signal);

#endif // GLOBALS_H