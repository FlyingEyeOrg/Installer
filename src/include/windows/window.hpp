#pragma once
#define UNICODE
#define _UNICODE

#include <Windows.h>
#include <windowsx.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "hwnd_wrapper.hpp"

namespace windows {

// 前向声明
class application;

// 窗口基类
class window {
   public:
    // 窗口过程函数类型
    using window_proc_t = std::function<LRESULT(window*, UINT, WPARAM, LPARAM)>;

    // hook 函数类型
    using hook_func = windows::hwnd_wrapper_hook_func;

   private:
    std::unique_ptr<windows::hwnd_wrapper> wrapper_;
    window_proc_t custom_window_proc_;

    // 将 hwnd_wrapper 的 hook 转换为 window 的消息处理
    LRESULT handle_hook(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param,
                        bool& handled) {
        handled = false;

        // 处理窗口消息
        LRESULT result = handle_message(u_msg, w_param, l_param);

        // 如果有自定义窗口过程，调用它
        if (custom_window_proc_) {
            result = custom_window_proc_(this, u_msg, w_param, l_param);
            handled = true;
        } else if (result != 0) {
            handled = true;
        }

        return result;
    }

    // 内部消息处理
    LRESULT handle_message(UINT u_msg, WPARAM w_param, LPARAM l_param) {
        switch (u_msg) {
            case WM_CREATE:
                return on_create();
            case WM_DESTROY:
                return on_destroy();
            case WM_CLOSE:
                return on_close();
            case WM_SIZE: {
                int width = LOWORD(l_param);
                int height = HIWORD(l_param);
                return on_size(width, height);
            }
            case WM_MOVE: {
                int x = LOWORD(l_param);
                int y = HIWORD(l_param);
                return on_move(x, y);
            }
            case WM_PAINT:
                return on_paint();
            case WM_MOUSEMOVE: {
                int x = GET_X_LPARAM(l_param);
                int y = GET_Y_LPARAM(l_param);
                return on_mouse_move(x, y, w_param);
            }
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN: {
                int x = GET_X_LPARAM(l_param);
                int y = GET_Y_LPARAM(l_param);
                WPARAM button = (u_msg == WM_LBUTTONDOWN)   ? MK_LBUTTON
                                : (u_msg == WM_MBUTTONDOWN) ? MK_MBUTTON
                                                            : MK_RBUTTON;
                return on_mouse_down(x, y, button, w_param);
            }
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP: {
                int x = GET_X_LPARAM(l_param);
                int y = GET_Y_LPARAM(l_param);
                WPARAM button = (u_msg == WM_LBUTTONUP)   ? MK_LBUTTON
                                : (u_msg == WM_MBUTTONUP) ? MK_MBUTTON
                                                          : MK_RBUTTON;
                return on_mouse_up(x, y, button, w_param);
            }
            case WM_KEYDOWN:
                return on_key_down(w_param, l_param);
            case WM_KEYUP:
                return on_key_up(w_param, l_param);
            case WM_CHAR:
                return on_char(static_cast<wchar_t>(w_param), l_param);
            case WM_COMMAND: {
                WORD id = LOWORD(w_param);
                WORD code = HIWORD(w_param);
                HWND control = reinterpret_cast<HWND>(l_param);
                return on_command(id, code, control);
            }
            case WM_NOTIFY: {
                NMHDR* nmhdr = reinterpret_cast<NMHDR*>(l_param);
                return on_notify(w_param, reinterpret_cast<LPARAM>(nmhdr));
            }
            case WM_TIMER:
                return on_timer(w_param);
        }
        return 0;
    }

   public:
    // 构造函数
    window() = default;

    explicit window(const std::wstring& title, int width = 800,
                    int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
                    HWND parent = nullptr, HMENU menu = nullptr) {
        create(title, width, height, style, parent, menu);
    }

    // 虚析构函数
    virtual ~window() {
        if (wrapper_) {
            wrapper_.reset();
        }
    }

    // 禁止拷贝
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    // 允许移动
    window(window&& other) noexcept
        : wrapper_(std::move(other.wrapper_)),
          custom_window_proc_(std::move(other.custom_window_proc_)) {
        if (wrapper_) {
            update_hook();
        }
    }

    window& operator=(window&& other) noexcept {
        if (this != &other) {
            wrapper_ = std::move(other.wrapper_);
            custom_window_proc_ = std::move(other.custom_window_proc_);
            if (wrapper_) {
                update_hook();
            }
        }
        return *this;
    }

    // 创建窗口
    bool create(const std::wstring& title = L"Window", int width = 800,
                int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
                HWND parent = nullptr, HMENU menu = nullptr) {
        // 创建 hook 函数
        auto hook = [this](HWND hwnd, UINT u_msg, WPARAM w_param,
                           LPARAM l_param, bool& handled) {
            return this->handle_hook(hwnd, u_msg, w_param, l_param, handled);
        };

        std::vector<hook_func> hooks = {hook};

        // 创建 hwnd_wrapper
        wrapper_ = std::make_unique<windows::hwnd_wrapper>(
            CS_HREDRAW | CS_VREDRAW,  // class_style
            0,                        // window_exstyle
            style,                    // window_style
            title,                    // title
            CW_USEDEFAULT,            // x
            CW_USEDEFAULT,            // y
            width,                    // width
            height,                   // height
            parent,                   // parent
            hooks                     // hooks
        );

        return is_valid();
    }

    bool create_with_class(const std::wstring& class_name,
                           const std::wstring& title = L"Window",
                           int width = 800, int height = 600,
                           DWORD style = WS_OVERLAPPEDWINDOW,
                           HWND parent = nullptr, HMENU menu = nullptr) {
        // 注意：hwnd_wrapper 使用动态生成的类名，这里为了保持兼容性
        // 调用普通的 create 方法
        return create(title, width, height, style, parent, menu);
    }

    // 窗口过程
    virtual LRESULT window_proc(UINT u_msg, WPARAM w_param, LPARAM l_param) {
        return handle_message(u_msg, w_param, l_param);
    }

    // 设置自定义窗口过程
    void set_window_proc(window_proc_t proc) {
        custom_window_proc_ = std::move(proc);
    }

    // 窗口操作
    HWND get_handle() const {
        return wrapper_ ? wrapper_->get_handle() : nullptr;
    }

    bool is_valid() const { return wrapper_ && wrapper_->is_valid(); }

    void show(int n_cmd_show = SW_SHOW) {
        if (wrapper_) {
            ::ShowWindow(wrapper_->get_handle(), n_cmd_show);
        }
    }

    void hide() {
        if (wrapper_) {
            ::ShowWindow(wrapper_->get_handle(), SW_HIDE);
        }
    }

    void enable() {
        if (wrapper_) {
            ::EnableWindow(wrapper_->get_handle(), TRUE);
        }
    }

    void disable() {
        if (wrapper_) {
            ::EnableWindow(wrapper_->get_handle(), FALSE);
        }
    }

    void set_title(const std::wstring& title) {
        if (wrapper_) {
            ::SetWindowTextW(wrapper_->get_handle(), title.c_str());
        }
    }

    std::wstring get_title() const {
        if (!wrapper_) return L"";

        int length = ::GetWindowTextLengthW(wrapper_->get_handle());
        if (length == 0) return L"";

        std::wstring title(length, L'\0');
        ::GetWindowTextW(wrapper_->get_handle(), &title[0], length + 1);
        return title;
    }

    RECT get_rect() const {
        RECT rect = {0};
        if (wrapper_) {
            ::GetWindowRect(wrapper_->get_handle(), &rect);
        }
        return rect;
    }

    RECT get_client_rect() const {
        RECT rect = {0};
        if (wrapper_) {
            ::GetClientRect(wrapper_->get_handle(), &rect);
        }
        return rect;
    }

    void set_position(int x, int y, int width, int height,
                      bool repaint = true) {
        if (wrapper_) {
            ::SetWindowPos(wrapper_->get_handle(), nullptr, x, y, width, height,
                           SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
        }
    }

    void set_size(int width, int height, bool repaint = true) {
        if (wrapper_) {
            ::SetWindowPos(
                wrapper_->get_handle(), nullptr, 0, 0, width, height,
                SWP_NOMOVE | SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
        }
    }

    void move(int x, int y, bool repaint = true) {
        if (wrapper_) {
            ::SetWindowPos(
                wrapper_->get_handle(), nullptr, x, y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
        }
    }

    void set_focus() {
        if (wrapper_) {
            ::SetFocus(wrapper_->get_handle());
        }
    }

    bool has_focus() const {
        if (!wrapper_) return false;
        return ::GetFocus() == wrapper_->get_handle();
    }

    void destroy() {
        if (wrapper_) {
            ::DestroyWindow(wrapper_->get_handle());
        }
    }

    void update() {
        if (wrapper_) {
            ::UpdateWindow(wrapper_->get_handle());
        }
    }

    void repaint() {
        if (wrapper_) {
            ::InvalidateRect(wrapper_->get_handle(), nullptr, TRUE);
            ::UpdateWindow(wrapper_->get_handle());
        }
    }

    // Hook 管理
    void add_hook(const hook_func& hook) {
        if (wrapper_) {
            wrapper_->add_hook(hook);
        }
    }

    void add_hook(hook_func&& hook) {
        if (wrapper_) {
            wrapper_->add_hook(std::move(hook));
        }
    }

    bool remove_hook(const hook_func& hook) {
        if (wrapper_) {
            return wrapper_->remove_hook(hook);
        }
        return false;
    }

    void clear_hooks() {
        if (wrapper_) {
            wrapper_->clear_hooks();
        }
    }

    size_t hook_count() const { return wrapper_ ? wrapper_->hook_count() : 0; }

    // 消息处理虚函数
    virtual LRESULT on_create() { return 0; }
    virtual LRESULT on_destroy() { return 0; }
    virtual LRESULT on_close() { return 0; }
    virtual LRESULT on_size(int width, int height) { return 0; }
    virtual LRESULT on_move(int x, int y) { return 0; }
    virtual LRESULT on_paint() { return 0; }
    virtual LRESULT on_mouse_move(int x, int y, WPARAM flags) { return 0; }
    virtual LRESULT on_mouse_down(int x, int y, WPARAM button, WPARAM flags) {
        return 0;
    }
    virtual LRESULT on_mouse_up(int x, int y, WPARAM button, WPARAM flags) {
        return 0;
    }
    virtual LRESULT on_key_down(WPARAM key, LPARAM flags) { return 0; }
    virtual LRESULT on_key_up(WPARAM key, LPARAM flags) { return 0; }
    virtual LRESULT on_char(wchar_t ch, LPARAM flags) { return 0; }
    virtual LRESULT on_command(WORD id, WORD code, HWND control) { return 0; }
    virtual LRESULT on_notify(WPARAM id, LPARAM nmhdr) { return 0; }
    virtual LRESULT on_timer(WPARAM timer_id) { return 0; }

   private:
    // 更新 hook 引用（用于移动操作后）
    void update_hook() {
        if (wrapper_ && wrapper_->hook_count() > 0) {
            // 重新绑定 this 指针到 hook
            auto hook = [this](HWND hwnd, UINT u_msg, WPARAM w_param,
                               LPARAM l_param, bool& handled) {
                return this->handle_hook(hwnd, u_msg, w_param, l_param,
                                         handled);
            };

            // 移除旧的 hook 并添加新的
            wrapper_->clear_hooks();
            wrapper_->add_hook(std::move(hook));
        }
    }
};
}  // namespace windows