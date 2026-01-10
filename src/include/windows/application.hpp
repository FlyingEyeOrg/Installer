#pragma once
#define UNICODE
#define _UNICODE

#include <Windows.h>

#include <memory>
#include <mutex>

namespace windows {

class application {
   private:
    bool running_ = false;
    int exit_code_ = 0;

    // 私有构造函数
    application() = default;

   public:
    // 禁止拷贝和移动
    application(const application&) = delete;
    application& operator=(const application&) = delete;
    application(application&&) = delete;
    application& operator=(application&&) = delete;

    ~application() {
        if (running_) {
            quit();
        }
    }

    // 获取单例实例
    static application* get_instance() {
        static application single_app;
        return &single_app;
    }

    // 运行消息循环
    int run() {
        if (running_) {
            return exit_code_;
        }

        running_ = true;
        exit_code_ = 0;

        MSG msg = {0};

        // 标准 Windows 消息循环
        while (running_) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    exit_code_ = static_cast<int>(msg.wParam);
                    break;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                // 空闲处理
                if (!on_idle()) {
                    WaitMessage();
                }
            }
        }

        running_ = false;
        return exit_code_;
    }

    // 退出应用
    void quit(int exit_code = 0) {
        exit_code_ = exit_code;
        running_ = false;
        PostQuitMessage(exit_code);
    }

    // 状态查询
    bool is_running() const { return running_; }
    int get_exit_code() const { return exit_code_; }

    // 空闲处理 - 可被子类重写
    virtual bool on_idle() { return false; }

    // 实用函数
    static HINSTANCE get_app_instance() { return GetModuleHandle(nullptr); }

    // 快速运行的静态方法
    static int run_app() { return get_instance()->run(); }

    // 快速退出的静态方法
    static void quit_app(int exit_code = 0) { get_instance()->quit(exit_code); }

    // 快速检查是否运行的静态方法
    static bool is_app_running() { return get_instance()->is_running(); }
};

}  // namespace windows