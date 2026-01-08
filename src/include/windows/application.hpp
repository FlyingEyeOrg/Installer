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
    // 消息过滤器类型
    using message_filter = std::function<bool(MSG&)>;

    // 空闲处理器类型
    using idle_handler = std::function<bool()>;

    // 窗口映射
    using window_map = std::unordered_map<HWND, window*>;

    // 获取单例实例
    static application& instance();

    // 运行应用程序消息循环
    int run();

    // 退出应用程序
    void quit(int exit_code = 0);

    // 添加消息过滤器
    void add_message_filter(const message_filter& filter);

    // 设置空闲处理器
    void set_idle_handler(const idle_handler& handler);

    // 获取应用程序实例句柄
    HINSTANCE get_instance() const;

    // 窗口管理
    bool register_window(HWND hwnd, window* win);
    bool unregister_window(HWND hwnd);
    window* find_window(HWND hwnd) const;
    std::vector<HWND> get_all_windows() const;
    size_t get_window_count() const;

    // 生命周期处理器
    void on_init(const std::function<void()>& handler);
    void on_exit(const std::function<void()>& handler);

    // 全局窗口过程
    static LRESULT CALLBACK global_window_proc(HWND hwnd, UINT u_msg,
                                               WPARAM w_param, LPARAM l_param);

   private:
    application();
    ~application();

    // 禁止拷贝
    application(const application&) = delete;
    application& operator=(const application&) = delete;

    // 私有实现
    class impl;
    std::unique_ptr<impl> pimpl_;
};