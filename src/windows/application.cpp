#include "windows/application.hpp"

#include <mutex>

#include "windows/window.hpp"
#include "windows/window_resource.hpp"

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

    mutable std::mutex windows_mutex_;

    // 处理消息过滤器
    bool process_message_filters(MSG& msg) {
        for (auto& filter : message_filters_) {
            if (filter(msg)) {
                return true;  // 消息已被处理
            }
        }
        return false;
    }

    // 运行空闲处理
    bool process_idle() {
        if (idle_handler_) {
            return idle_handler_();
        }
        return false;
    }
};

// 全局窗口过程
LRESULT CALLBACK application::global_window_proc(HWND hwnd, UINT u_msg,
                                                 WPARAM w_param,
                                                 LPARAM l_param) {
    auto& app = application::instance();

    // 处理WM_NCCREATE消息
    if (u_msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(l_param);
        if (pCreate && pCreate->lpCreateParams) {
            window* win_ptr =
                reinterpret_cast<window*>(pCreate->lpCreateParams);
            app.register_window(hwnd, win_ptr);
        }
    }

    // 查找窗口
    auto win = app.find_window(hwnd);
    if (win) {
        LRESULT result = win->window_proc(u_msg, w_param, l_param);

        // 处理窗口销毁
        if (u_msg == WM_DESTROY) {
            app.unregister_window(hwnd);

            // 如果所有窗口都已关闭，退出应用
            if (app.get_window_count() == 0) {
                app.quit();
            }
        }

        return result;
    }

    return ::DefWindowProcW(hwnd, u_msg, w_param, l_param);
}

application& application::instance() {
    static application app;
    return app;
}

application::application() : pimpl_(std::make_unique<impl>()) {
    pimpl_->hInstance_ = ::GetModuleHandleW(nullptr);

    // 注册默认窗口类（使用全局窗口过程）
    window_resource::instance().register_default_class(global_window_proc);
}

application::~application() = default;

bool application::register_window(HWND hwnd, window* win) {
    std::lock_guard<std::mutex> lock(pimpl_->windows_mutex_);
    if (!hwnd || !win) {
        return false;
    }

    pimpl_->windows_[hwnd] = win;
    return true;
}

bool application::unregister_window(HWND hwnd) {
    std::lock_guard<std::mutex> lock(pimpl_->windows_mutex_);
    return pimpl_->windows_.erase(hwnd) > 0;
}

window* application::find_window(HWND hwnd) const {
    std::lock_guard<std::mutex> lock(pimpl_->windows_mutex_);
    auto it = pimpl_->windows_.find(hwnd);
    return it != pimpl_->windows_.end() ? it->second : nullptr;
}

std::vector<HWND> application::get_all_windows() const {
    std::lock_guard<std::mutex> lock(pimpl_->windows_mutex_);
    std::vector<HWND> handles;
    handles.reserve(pimpl_->windows_.size());

    for (const auto& pair : pimpl_->windows_) {
        handles.push_back(pair.first);
    }

    return handles;
}

size_t application::get_window_count() const {
    std::lock_guard<std::mutex> lock(pimpl_->windows_mutex_);
    return pimpl_->windows_.size();
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

void application::on_init(const std::function<void()>& handler) {
    pimpl_->init_handler_ = handler;
}

void application::on_exit(const std::function<void()>& handler) {
    pimpl_->exit_handler_ = handler;
}