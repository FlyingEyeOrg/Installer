#include "win32/window.hpp"

#include <windowsx.h>

#include <memory>
#include <utility>

#include "win32/window_resource.hpp"

// 构造函数 - 创建默认窗口
window::window(const std::wstring& title, int width, int height, DWORD style,
               HWND parent, HMENU menu) {
    // 使用默认窗口类创建
    create_internal(L"", title, width, height, style, parent, menu);
}

// 构造函数 - 创建指定类窗口
window::window(const std::wstring& class_name, const std::wstring& title,
               int width, int height, DWORD style, HWND parent, HMENU menu) {
    // 使用指定窗口类创建
    create_internal(class_name, title, width, height, style, parent, menu);
}

// 析构函数
window::~window() {
    if (handle_) {
        destroy();
    }
}

// 移动构造函数
window::window(window&& other) noexcept
    : handle_(other.handle_),
      custom_window_proc_(std::move(other.custom_window_proc_)) {
    other.handle_ = nullptr;
}

// 移动赋值运算符
window& window::operator=(window&& other) noexcept {
    if (this != &other) {
        destroy();
        handle_ = other.handle_;
        custom_window_proc_ = std::move(other.custom_window_proc_);
        other.handle_ = nullptr;
    }
    return *this;
}

// 内部创建窗口实现
bool window::create_internal(const std::wstring& class_name,
                             const std::wstring& title, int width, int height,
                             DWORD style, HWND parent, HMENU menu) {
    if (handle_) {
        return false;  // 窗口已存在
    }

    // 获取资源管理器
    window_resource& resource = window_resource::get_instance();

    // 使用正确的shared_ptr构造方式
    auto self = std::shared_ptr<window>(this, [](window*) {});

    if (class_name.empty()) {
        // 使用默认窗口类
        handle_ =
            resource.create_window(title, style, CW_USEDEFAULT, CW_USEDEFAULT,
                                   width, height, self, parent, menu);
    } else {
        // 使用指定窗口类
        handle_ = resource.create_window_with_class(
            class_name, title, style, CW_USEDEFAULT, CW_USEDEFAULT, width,
            height, self, parent, menu);
    }

    return handle_ != nullptr;
}

// 窗口过程函数
LRESULT window::window_proc(UINT u_msg, WPARAM w_param, LPARAM l_param) {
    // 如果有自定义窗口过程，调用它
    if (custom_window_proc_) {
        return custom_window_proc_(this, u_msg, w_param, l_param);
    }

    // 否则调用内部消息处理器
    return handle_message(u_msg, w_param, l_param);
}

// 设置自定义窗口过程
void window::set_window_proc(window_proc_t proc) {
    custom_window_proc_ = std::move(proc);
}

// 获取窗口句柄
HWND window::get_handle() const { return handle_; }

// 检查窗口有效性
bool window::is_valid() const {
    return handle_ != nullptr && IsWindow(handle_);
}

// 显示窗口
void window::show(int n_cmd_show) {
    if (handle_) {
        ShowWindow(handle_, n_cmd_show);
        UpdateWindow(handle_);
    }
}

// 隐藏窗口
void window::hide() {
    if (handle_) {
        ShowWindow(handle_, SW_HIDE);
    }
}

// 启用窗口
void window::enable() {
    if (handle_) {
        EnableWindow(handle_, TRUE);
    }
}

// 禁用窗口
void window::disable() {
    if (handle_) {
        EnableWindow(handle_, FALSE);
    }
}

// 设置窗口标题
void window::set_title(const std::wstring& title) {
    if (handle_) {
        SetWindowTextW(handle_, title.c_str());
    }
}

// 获取窗口标题
std::wstring window::get_title() const {
    if (!handle_) {
        return L"";
    }

    int length = GetWindowTextLengthW(handle_);
    if (length <= 0) {
        return L"";
    }

    std::wstring title(length + 1, L'\0');
    GetWindowTextW(handle_, &title[0], length + 1);
    title.resize(length);
    return title;
}

// 获取窗口矩形
RECT window::get_rect() const {
    RECT rect = {};
    if (handle_) {
        GetWindowRect(handle_, &rect);
    }
    return rect;
}

// 获取客户区矩形
RECT window::get_client_rect() const {
    RECT rect = {};
    if (handle_) {
        GetClientRect(handle_, &rect);
    }
    return rect;
}

// 设置窗口位置和大小
void window::set_position(int x, int y, int width, int height, bool repaint) {
    if (handle_) {
        SetWindowPos(handle_, nullptr, x, y, width, height,
                     SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
    }
}

// 设置窗口大小
void window::set_size(int width, int height, bool repaint) {
    if (handle_) {
        RECT rect = get_rect();
        SetWindowPos(handle_, nullptr, rect.left, rect.top, width, height,
                     SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
    }
}

// 移动窗口
void window::move(int x, int y, bool repaint) {
    if (handle_) {
        RECT rect = get_rect();
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        SetWindowPos(handle_, nullptr, x, y, width, height,
                     SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
    }
}

// 设置窗口焦点
void window::set_focus() {
    if (handle_) {
        SetFocus(handle_);
    }
}

// 检查是否有焦点
bool window::has_focus() const {
    if (handle_) {
        return GetFocus() == handle_;
    }
    return false;
}

// 销毁窗口
void window::destroy() {
    if (handle_) {
        window_resource::get_instance().destroy_window(handle_);
        handle_ = nullptr;
    }
}

// 更新窗口
void window::update() {
    if (handle_) {
        UpdateWindow(handle_);
    }
}

// 重绘窗口
void window::repaint() {
    if (handle_) {
        InvalidateRect(handle_, nullptr, TRUE);
        UpdateWindow(handle_);
    }
}

// 获取资源管理器实例
window_resource& window::get_resource() {
    return window_resource::get_instance();
}

// 内部消息处理
LRESULT window::handle_message(UINT u_msg, WPARAM w_param, LPARAM l_param) {
    switch (u_msg) {
        case WM_CREATE:
            return on_create();

        case WM_DESTROY:
            return on_destroy();

        case WM_CLOSE:
            return on_close();

        case WM_SIZE:
            return on_size(LOWORD(l_param), HIWORD(l_param));

        case WM_MOVE:
            return on_move(LOWORD(l_param), HIWORD(l_param));

        case WM_PAINT:
            return on_paint();

        case WM_MOUSEMOVE:
            return on_mouse_move(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param),
                                 w_param);

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            return on_mouse_down(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param),
                                 u_msg, w_param);

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            return on_mouse_up(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param),
                               u_msg, w_param);

        case WM_KEYDOWN:
            return on_key_down(w_param, l_param);

        case WM_KEYUP:
            return on_key_up(w_param, l_param);

        case WM_CHAR:
            return on_char(static_cast<wchar_t>(w_param), l_param);

        case WM_COMMAND:
            return on_command(LOWORD(w_param), HIWORD(w_param),
                              reinterpret_cast<HWND>(l_param));

        case WM_NOTIFY:
            return on_notify(w_param, l_param);

        case WM_TIMER:
            return on_timer(w_param);
    }

    return DefWindowProcW(handle_, u_msg, w_param, l_param);
}

// 默认消息处理实现
LRESULT window::on_create() { return 0; }
LRESULT window::on_destroy() {
    handle_ = nullptr;
    return 0;
}
LRESULT window::on_close() {
    destroy();
    return 0;
}
LRESULT window::on_size(int width, int height) { return 0; }
LRESULT window::on_move(int x, int y) { return 0; }
LRESULT window::on_paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(handle_, &ps);
    RECT rect;
    GetClientRect(handle_, &rect);
    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
    EndPaint(handle_, &ps);
    return 0;
}
LRESULT window::on_mouse_move(int x, int y, WPARAM flags) { return 0; }
LRESULT window::on_mouse_down(int x, int y, WPARAM button, WPARAM flags) {
    return 0;
}
LRESULT window::on_mouse_up(int x, int y, WPARAM button, WPARAM flags) {
    return 0;
}
LRESULT window::on_key_down(WPARAM key, LPARAM flags) { return 0; }
LRESULT window::on_key_up(WPARAM key, LPARAM flags) { return 0; }
LRESULT window::on_char(wchar_t ch, LPARAM flags) { return 0; }
LRESULT window::on_command(WORD id, WORD code, HWND control) { return 0; }
LRESULT window::on_notify(WPARAM id, LPARAM nmhdr) { return 0; }
LRESULT window::on_timer(WPARAM timer_id) { return 0; }