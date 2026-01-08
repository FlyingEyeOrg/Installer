#pragma once
#define UNICODE
#define _UNICODE

#include <Windows.h>

#include <functional>
#include <memory>
#include <string>

// 前向声明
class application;

// 窗口基类
class window {
   public:
    // 窗口过程函数类型
    using window_proc_t = std::function<LRESULT(window*, UINT, WPARAM, LPARAM)>;

    // 构造函数
    window();
    explicit window(const std::wstring& title, int width = 800,
                    int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
                    HWND parent = nullptr, HMENU menu = nullptr);

    // 虚析构函数
    virtual ~window();

    // 禁止拷贝
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    // 创建窗口
    bool create(const std::wstring& title = L"Window", int width = 800,
                int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
                HWND parent = nullptr, HMENU menu = nullptr);

    bool create_with_class(const std::wstring& class_name,
                           const std::wstring& title = L"Window",
                           int width = 800, int height = 600,
                           DWORD style = WS_OVERLAPPEDWINDOW,
                           HWND parent = nullptr, HMENU menu = nullptr);

    // 窗口过程
    virtual LRESULT window_proc(UINT u_msg, WPARAM w_param, LPARAM l_param);

    // 设置自定义窗口过程
    void set_window_proc(window_proc_t proc);

    // 窗口操作
    HWND get_handle() const { return handle_; }
    bool is_valid() const { return handle_ && ::IsWindow(handle_); }

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
    window_proc_t custom_window_proc_;
    HWND handle_ = nullptr;

    void set_window_handle_from_global_window_proc(HWND window_handle) {
        this->handle_ = window_handle;
    }

    // 内部消息处理
    LRESULT handle_message(UINT u_msg, WPARAM w_param, LPARAM l_param);
};