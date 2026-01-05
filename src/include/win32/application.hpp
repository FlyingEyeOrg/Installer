// application.hpp
#pragma once
#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <Windows.h>

#include <functional>
#include <memory>
#include <vector>


namespace win32 {

// 应用程序类，负责消息循环
class application {
   public:
    using idle_handler = std::function<bool()>;

    // 单例模式
    static application& instance() {
        static application app;
        return app;
    }

    // 运行应用程序
    int run();

    // 退出应用程序
    void quit(int exit_code = 0);

    // 添加空闲处理函数
    void add_idle_handler(const idle_handler& handler);

    // 处理消息
    bool process_messages();

    // 获取实例句柄
    HINSTANCE instance_handle() const noexcept { return instance_; }

    // 设置实例句柄
    void set_instance_handle(HINSTANCE instance) { instance_ = instance; }

   private:
    application() = default;
    ~application() = default;

    // 禁止拷贝
    application(const application&) = delete;
    application& operator=(const application&) = delete;

    HINSTANCE instance_{nullptr};
    int exit_code_{0};
    bool is_running_{false};
    std::vector<idle_handler> idle_handlers_;
};

}  // namespace win32

#endif  // APPLICATION_HPP