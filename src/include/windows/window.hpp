#pragma once
// 使用 unicode api
#define UNICODE
#define _UNICODE

#include <Windows.h>

#include <functional>
#include <memory>
#include <string>

// 前向声明
class application;
class window_resource;

// 窗口基类
class window {
    friend class application;

   public:
    // 窗口过程函数类型
    using window_proc_t = std::function<LRESULT(window*, UINT, WPARAM, LPARAM)>;

    // 构造函数 - 创建默认窗口
    window(const std::wstring& title = L"", int width = 800, int height = 600,
           DWORD style = WS_OVERLAPPEDWINDOW, HWND parent = nullptr,
           HMENU menu = nullptr);

    // 构造函数 - 创建指定类窗口
    window(const std::wstring& class_name, const std::wstring& title,
           int width = 800, int height = 600, DWORD style = WS_OVERLAPPEDWINDOW,
           HWND parent = nullptr, HMENU menu = nullptr);

    virtual ~window();

    // 禁止拷贝
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    // 允许移动
    window(window&& other) noexcept;
    window& operator=(window&& other) noexcept;

    // 窗口过程（可被子类重写）
    virtual LRESULT window_proc(UINT u_msg, WPARAM w_param, LPARAM l_param);

    // 设置自定义窗口过程函数
    void set_window_proc(window_proc_t proc);

    // 获取窗口句柄
    HWND get_handle() const;

    // 检查窗口是否有效
    bool is_valid() const;

    // 显示/隐藏窗口
    void show(int n_cmd_show = SW_SHOW);
    void hide();

    // 启用/禁用窗口
    void enable();
    void disable();

    // 设置窗口标题
    void set_title(const std::wstring& title);

    // 获取窗口标题
    std::wstring get_title() const;

    // 获取窗口位置和大小
    RECT get_rect() const;
    RECT get_client_rect() const;

    // 设置窗口位置和大小
    void set_position(int x, int y, int width, int height, bool repaint = true);
    void set_size(int width, int height, bool repaint = true);
    void move(int x, int y, bool repaint = true);

    // 窗口焦点
    void set_focus();
    bool has_focus() const;

    // 销毁窗口
    virtual void destroy();

    // 更新窗口
    void update();
    void repaint();

    // 消息处理
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

   protected:
    HWND handle_ = nullptr;

   private:
    window_proc_t custom_window_proc_;

    // 内部创建窗口
    bool create_internal(const std::wstring& class_name,
                         const std::wstring& title, int width, int height,
                         DWORD style, HWND parent, HMENU menu);

    static window_resource& get_resource();

    // 内部消息处理
    LRESULT handle_message(UINT u_msg, WPARAM w_param, LPARAM l_param);

    /// @brief 设置窗口句柄
    /// @param window_handle 窗口句柄
    void set_window_handle(HWND window_handle) {
        this->handle_ = window_handle;
    }
};