#pragma once
#include <Windows.h>

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

// 前向声明
class window;

class application {
   public:

    // 获取单例实例
    static application& instance();

    // 运行应用程序消息循环
    int run();

    // 退出应用程序
    void quit(int exit_code = 0);

    // 获取应用程序实例句柄
    HINSTANCE get_instance() const;

    std::vector<HWND> get_all_windows() const;
    size_t get_window_count() const;

    // 生命周期处理器
    void on_init(const std::function<void()>& handler);
    void on_exit(const std::function<void()>& handler);

   private:
    application();
    ~application();

    // 禁止拷贝
    application(const application&) = delete;
    application& operator=(const application&) = delete;
};