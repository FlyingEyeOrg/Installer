// window_base.hpp
#pragma once
#ifndef WINDOW_BASE_HPP
#define WINDOW_BASE_HPP

#include <Windows.h>

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace win32 {

// 前置声明
class window_base;

// 消息处理器类型别名
using message_handler =
    std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)>;
using close_handler = std::function<void()>;
using create_handler = std::function<void()>;
using destroy_handler = std::function<void()>;
using paint_handler = std::function<void(HDC)>;
using resize_handler = std::function<void(int, int)>;
using key_handler = std::function<void(WPARAM, bool, LPARAM)>;
using mouse_handler = std::function<void(int, int, WPARAM)>;
using mouse_button_handler =
    std::function<void(int, int, WPARAM, int)>;  // x, y, wparam, button
using mouse_wheel_handler =
    std::function<void(int, int, WPARAM, int)>;  // delta

// 窗口基类
class window_base {
   public:
    // 窗口样式
    struct style {
        DWORD style{WS_OVERLAPPEDWINDOW};
        DWORD ex_style{0};
        DWORD class_style{CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS};
        HICON icon{nullptr};
        HICON icon_small{nullptr};
        HCURSOR cursor{nullptr};
        HBRUSH background{reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)};
    };

    // 构造函数
    window_base(const std::wstring& title = L"Win32 Window", int width = 800,
                int height = 600);

    // 析构函数
    virtual ~window_base();

    // 禁止拷贝
    window_base(const window_base&) = delete;
    window_base& operator=(const window_base&) = delete;

    // 允许移动
    window_base(window_base&& other) noexcept;
    window_base& operator=(window_base&& other) noexcept;

    // 创建窗口
    virtual bool create(HINSTANCE instance = nullptr, HWND parent = nullptr,
                        const style& window_style = {});

    // 显示/隐藏窗口
    void show(int cmd_show = SW_SHOW);
    void hide();
    void minimize();
    void maximize();
    void restore();

    // 窗口管理
    void close();
    void destroy();
    bool is_visible() const;
    bool is_minimized() const;
    bool is_maximized() const;

    // 获取窗口属性
    HWND handle() const noexcept { return handle_; }
    HINSTANCE instance() const noexcept { return instance_; }
    std::wstring title() const;
    std::wstring class_name() const noexcept { return class_name_; }

    // 设置窗口属性
    void set_title(const std::wstring& title);
    void set_size(int width, int height, bool redraw = true);
    void set_position(int x, int y, bool redraw = true);
    void set_client_size(int width, int height, bool redraw = true);

    // 获取窗口几何信息
    void get_position(int& x, int& y) const;
    void get_size(int& width, int& height) const;
    void get_client_size(int& width, int& height) const;
    RECT get_client_rect() const;
    RECT get_window_rect() const;

    // 消息处理
    LRESULT send_message(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const;
    bool post_message(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const;

    // 事件处理器设置
    void set_message_handler(const message_handler& handler) {
        message_handler_ = handler;
    }

    void set_close_handler(const close_handler& handler) {
        close_handler_ = handler;
    }

    void set_create_handler(const create_handler& handler) {
        create_handler_ = handler;
    }

    void set_destroy_handler(const destroy_handler& handler) {
        destroy_handler_ = handler;
    }

    void set_paint_handler(const paint_handler& handler) {
        paint_handler_ = handler;
    }

    void set_resize_handler(const resize_handler& handler) {
        resize_handler_ = handler;
    }

    void set_key_down_handler(const key_handler& handler) {
        key_down_handler_ = handler;
    }

    void set_key_up_handler(const key_handler& handler) {
        key_up_handler_ = handler;
    }

    void set_mouse_move_handler(const mouse_handler& handler) {
        mouse_move_handler_ = handler;
    }

    void set_mouse_down_handler(const mouse_button_handler& handler) {
        mouse_down_handler_ = handler;
    }

    void set_mouse_up_handler(const mouse_button_handler& handler) {
        mouse_up_handler_ = handler;
    }

    void set_mouse_wheel_handler(const mouse_wheel_handler& handler) {
        mouse_wheel_handler_ = handler;
    }

    // 静态方法
    static window_base* from_handle(HWND hwnd);
    static bool register_class(const std::wstring& class_name,
                               HINSTANCE instance = nullptr,
                               const style& window_style = {});
    static void unregister_class(const std::wstring& class_name,
                                 HINSTANCE instance = nullptr);

   protected:
    // 窗口过程
    static LRESULT CALLBACK static_window_proc(HWND hwnd, UINT msg,
                                               WPARAM wparam, LPARAM lparam);
    virtual LRESULT window_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                LPARAM lparam);

    // 消息处理助手
    virtual LRESULT on_create();
    virtual LRESULT on_destroy();
    virtual LRESULT on_close();
    virtual LRESULT on_paint();
    virtual LRESULT on_size(WPARAM wparam, LPARAM lparam);
    virtual LRESULT on_key(UINT msg, WPARAM wparam, LPARAM lparam);
    virtual LRESULT on_char(WPARAM wparam, LPARAM lparam);
    virtual LRESULT on_mouse_move(WPARAM wparam, LPARAM lparam);
    virtual LRESULT on_mouse_button(UINT msg, WPARAM wparam, LPARAM lparam);
    virtual LRESULT on_mouse_wheel(WPARAM wparam, LPARAM lparam);

    // 窗口句柄映射
    static std::unordered_map<HWND, window_base*>& window_map();

    // 窗口资源
    HWND handle_{nullptr};
    HWND parent_{nullptr};
    HINSTANCE instance_{nullptr};
    std::wstring class_name_;

    // 窗口属性
    std::wstring title_;
    int width_;
    int height_;
    bool is_created_{false};

    // 事件处理器
    message_handler message_handler_;
    close_handler close_handler_;
    create_handler create_handler_;
    destroy_handler destroy_handler_;
    paint_handler paint_handler_;
    resize_handler resize_handler_;
    key_handler key_down_handler_;
    key_handler key_up_handler_;
    mouse_handler mouse_move_handler_;
    mouse_button_handler mouse_down_handler_;
    mouse_button_handler mouse_up_handler_;
    mouse_wheel_handler mouse_wheel_handler_;

   private:
    // 生成唯一的窗口类名
    static std::wstring generate_class_name();
};

}  // namespace win32

#endif  // WINDOW_BASE_HPP