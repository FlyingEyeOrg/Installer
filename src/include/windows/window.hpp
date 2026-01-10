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
   private:
    std::unique_ptr<windows::hwnd_wrapper> wrapper_;

    // 将 hwnd_wrapper 的 hook 转换为 window 的消息处理
    LRESULT handle_hook(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param,
                        bool& handled);

    // 内部消息处理
    LRESULT handle_message(UINT u_msg, WPARAM w_param, LPARAM l_param);

   public:
    // 构造函数
    window() = delete;
    explicit window(const std::wstring& title, int width = 800,
                    int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
                    HWND parent = nullptr, HMENU menu = nullptr);

    // 虚析构函数
    virtual ~window();

    // 禁止拷贝
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    // 允许移动
    window(window&& other) noexcept;
    window& operator=(window&& other) noexcept;

    // 创建窗口
    bool create(const std::wstring& title = L"Window", int width = 800,
                int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
                HWND parent = nullptr, HMENU menu = nullptr);

    // 窗口操作
    HWND get_handle() const;
    bool is_valid() const;
    void show(int n_cmd_show = SW_SHOW);
    void hide();
    void enable();
    void disable();
    void set_title(const std::wstring& title);
    std::wstring get_title() const;
    RECT get_rect() const;
    RECT get_client_rect() const;
    void set_position(int x, int y, int width, int height, bool repaint = true);
    void set_size(int width, int height, bool repaint = true);
    void move(int x, int y, bool repaint = true);
    void set_focus();
    bool has_focus() const;
    void destroy();
    void update();
    void repaint();

    // 消息处理虚函数
    virtual LRESULT on_create();
    virtual LRESULT on_destroy();
    virtual LRESULT on_close();
    virtual LRESULT on_size(int width, int height);
    virtual LRESULT on_move(int x, int y);
    virtual LRESULT on_paint();
    virtual LRESULT on_mouse_move(int x, int y, WPARAM flags);
    virtual LRESULT on_mouse_down(int x, int y, WPARAM button, WPARAM flags);
    virtual LRESULT on_mouse_up(int x, int y, WPARAM button, WPARAM flags);
    virtual LRESULT on_key_down(WPARAM key, LPARAM flags);
    virtual LRESULT on_key_up(WPARAM key, LPARAM flags);
    virtual LRESULT on_char(wchar_t ch, LPARAM flags);
    virtual LRESULT on_command(WORD id, WORD code, HWND control);
    virtual LRESULT on_notify(WPARAM id, LPARAM nmhdr);
    virtual LRESULT on_timer(WPARAM timer_id);

   private:
    // 更新 hook 引用（用于移动操作后）
    void update_hook();
};
}  // namespace windows