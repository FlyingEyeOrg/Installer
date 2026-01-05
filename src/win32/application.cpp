// application.cpp
#include "win32/application.hpp"

#include "win32/window_base.hpp"

namespace win32 {

int application::run() {
    if (is_running_) {
        return exit_code_;
    }

    if (!instance_) {
        instance_ = GetModuleHandleW(nullptr);
    }

    is_running_ = true;
    exit_code_ = 0;

    MSG msg = {};

    // 主消息循环
    while (is_running_) {
        // 处理消息
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                is_running_ = false;
                exit_code_ = static_cast<int>(msg.wParam);
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        // 空闲处理
        else {
            bool has_idle_work = false;
            for (const auto& handler : idle_handlers_) {
                if (handler()) {
                    has_idle_work = true;
                }
            }

            // 如果没有空闲任务，等待消息
            if (!has_idle_work) {
                WaitMessage();
            }
        }
    }

    return exit_code_;
}

void application::quit(int exit_code) {
    exit_code_ = exit_code;
    is_running_ = false;
    PostQuitMessage(exit_code);
}

void application::add_idle_handler(const idle_handler& handler) {
    idle_handlers_.push_back(handler);
}

bool application::process_messages() {
    MSG msg = {};
    bool has_message = PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0;

    if (has_message) {
        if (msg.message == WM_QUIT) {
            is_running_ = false;
            exit_code_ = static_cast<int>(msg.wParam);
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return has_message;
}

}  // namespace win32