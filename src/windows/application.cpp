// application.cpp
#include "windows/application.hpp"

#include <algorithm>
#include <mutex>

#include "windows/window.hpp"
#include "windows/window_resource.hpp"

// 实现类定义
class application::impl {
   public:
    HINSTANCE hInstance_ = nullptr;
    int exit_code_ = 0;
    bool running_ = false;

    // 消息处理器
    std::vector<message_filter> message_filters_;
    idle_handler idle_handler_;

    // 生命周期处理器
    std::function<void()> init_handler_;
    std::function<void()> exit_handler_;

    // 全局窗口过程
    static LRESULT CALLBACK static_window_proc(HWND hwnd, UINT u_msg,
                                               WPARAM w_param, LPARAM l_param);

    // 内部消息处理
    LRESULT window_proc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

    // 处理消息过滤器
    bool process_message_filters(MSG& msg);

    // 运行空闲处理
    bool process_idle();
};

// 静态成员初始化
application& application::instance() {
    static application app;
    return app;
}

application::application() : pimpl_(std::make_unique<impl>()) {
    pimpl_->hInstance_ = ::GetModuleHandleW(nullptr);
}

application::~application() = default;

// 全局窗口过程
LRESULT CALLBACK application::impl::static_window_proc(HWND hwnd, UINT u_msg,
                                                       WPARAM w_param,
                                                       LPARAM l_param) {
    return application::instance().pimpl_->window_proc(hwnd, u_msg, w_param,
                                                       l_param);
}

LRESULT application::impl::window_proc(HWND hwnd, UINT u_msg, WPARAM w_param,
                                       LPARAM l_param) {
    auto& resource = window_resource::get_instance();
    auto win = resource.find_window(hwnd);

    // 处理创建消息
    if (u_msg == WM_NCCREATE && !win) {
        CREATESTRUCTW* p_create = reinterpret_cast<CREATESTRUCTW*>(l_param);
        if (p_create && p_create->lpCreateParams) {
            window* win_ptr =
                reinterpret_cast<window*>(p_create->lpCreateParams);
            win_ptr->set_window_handle(hwnd);

            // 使用安全的更新方法
            resource.update_window_mapping(hwnd, win_ptr);

            // 重新查找窗口
            win = resource.find_window(hwnd);
        }
    }

    if (win) {
        LRESULT result = win->window_proc(u_msg, w_param, l_param);

        if (u_msg == WM_DESTROY) {
            // 窗口销毁时从映射中移除
            resource.remove_window_mapping(hwnd);

            // 检查是否所有窗口都已关闭
            if (resource.get_window_count() == 0) {
                application::instance().quit();
            }
        }

        return result;
    }

    return DefWindowProcW(hwnd, u_msg, w_param, l_param);
}

bool application::impl::process_message_filters(MSG& msg) {
    for (auto& filter : message_filters_) {
        if (filter(msg)) {
            return true;  // 消息已被处理
        }
    }
    return false;
}

bool application::impl::process_idle() {
    if (idle_handler_) {
        return idle_handler_();
    }
    return false;  // 没有更多空闲工作
}

int application::run() {
    if (pimpl_->running_) {
        return pimpl_->exit_code_;
    }

    pimpl_->running_ = true;

    // 执行初始化处理器
    if (pimpl_->init_handler_) {
        pimpl_->init_handler_();
    }

    MSG msg = {};
    while (pimpl_->running_) {
        // 处理消息
        if (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                pimpl_->exit_code_ = static_cast<int>(msg.wParam);
                break;
            }

            // 处理消息过滤器
            if (!pimpl_->process_message_filters(msg)) {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
        } else {
            // 空闲处理
            if (!pimpl_->process_idle()) {
                // 没有空闲工作，等待消息
                ::WaitMessage();
            }
        }
    }

    pimpl_->running_ = false;

    // 执行退出处理器
    if (pimpl_->exit_handler_) {
        pimpl_->exit_handler_();
    }

    return pimpl_->exit_code_;
}

void application::quit(int exit_code) {
    pimpl_->running_ = false;
    pimpl_->exit_code_ = exit_code;
    ::PostQuitMessage(exit_code);
}

void application::add_message_filter(const message_filter& filter) {
    pimpl_->message_filters_.push_back(filter);
}

void application::set_idle_handler(const idle_handler& handler) {
    pimpl_->idle_handler_ = handler;
}

HINSTANCE application::get_instance() const { return pimpl_->hInstance_; }

WNDPROC application::get_global_window_proc() const {
    return &impl::static_window_proc;
}

void application::on_init(const std::function<void()>& handler) {
    pimpl_->init_handler_ = handler;
}

void application::on_exit(const std::function<void()>& handler) {
    pimpl_->exit_handler_ = handler;
}