// application.hpp
#pragma once
#include <Windows.h>

#include <functional>
#include <memory>
#include <vector>


class application {
   public:
    // 消息过滤器类型
    using message_filter = std::function<bool(MSG&)>;

    // 空闲处理器类型
    using idle_handler = std::function<bool()>;

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

    // 获取全局窗口过程
    WNDPROC get_global_window_proc() const;

    // 处理前置/后置消息循环逻辑
    void on_init(const std::function<void()>& handler);
    void on_exit(const std::function<void()>& handler);

   private:
    application();
    ~application();

    // 禁用拷贝
    application(const application&) = delete;
    application& operator=(const application&) = delete;

    class impl;
    std::unique_ptr<impl> pimpl_;
};