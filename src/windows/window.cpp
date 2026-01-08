#include "windows/window.hpp"

#include <windowsx.h>

#include "windows/application.hpp"
#include "windows/window_resource.hpp"

window::window() = default;

window::window(const std::wstring& title, int width, int height, DWORD style,
               HWND parent, HMENU menu) {
    create(title, width, height, style, parent, menu);
}

window::~window() {
    if (is_valid()) {
        destroy();
    }
}

bool window::create(const std::wstring& title, int width, int height,
                    DWORD style, HWND parent, HMENU menu) {
    if (is_valid()) {
        return false;
    }

    auto& resource = window_resource::instance();

    // 确保窗口类已注册
    if (!resource.is_class_registered(resource.get_default_class_name())) {
        resource.register_default_class(application::global_window_proc);
    }

    handle_ = resource.create_window(
        resource.get_default_class_name(), title.empty() ? L"Window" : title,
        style, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        static_cast<void*>(this),  // 传递 this 指针
        parent, menu);

    return is_valid();
}

bool window::create_with_class(const std::wstring& class_name,
                               const std::wstring& title, int width, int height,
                               DWORD style, HWND parent, HMENU menu) {
    if (is_valid()) {
        return false;
    }

    auto& resource = window_resource::instance();

    // 确保窗口类已注册
    if (!resource.is_class_registered(class_name)) {
        // 自动注册窗口类
        window_resource::window_class_info info;
        info.class_name = class_name;
        if (!resource.register_class(info, application::global_window_proc)) {
            return false;
        }
    }

    resource.create_window(class_name, title.empty() ? L"Window" : title, style,
                           CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                           static_cast<void*>(this),  // 传递 this 指针
                           parent, menu);

    return is_valid();
}

LRESULT window::window_proc(UINT u_msg, WPARAM w_param, LPARAM l_param) {
    if (custom_window_proc_) {
        return custom_window_proc_(this, u_msg, w_param, l_param);
    }

    return handle_message(u_msg, w_param, l_param);
}

void window::set_window_proc(window_proc_t proc) {
    custom_window_proc_ = std::move(proc);
}

// 窗口操作实现
void window::show(int n_cmd_show) {
    if (is_valid()) {
        ::ShowWindow(handle_, n_cmd_show);
        ::UpdateWindow(handle_);
    }
}

void window::hide() {
    if (is_valid()) {
        ::ShowWindow(handle_, SW_HIDE);
    }
}

void window::enable() {
    if (is_valid()) {
        ::EnableWindow(handle_, TRUE);
    }
}

void window::disable() {
    if (is_valid()) {
        ::EnableWindow(handle_, FALSE);
    }
}

void window::set_title(const std::wstring& title) {
    if (is_valid()) {
        ::SetWindowTextW(handle_, title.c_str());
    }
}

std::wstring window::get_title() const {
    if (!is_valid()) {
        return L"";
    }

    int length = ::GetWindowTextLengthW(handle_);
    if (length <= 0) {
        return L"";
    }

    std::wstring title(length + 1, L'\0');
    ::GetWindowTextW(handle_, &title[0], length + 1);
    title.resize(length);
    return title;
}

RECT window::get_rect() const {
    RECT rect = {};
    if (is_valid()) {
        ::GetWindowRect(handle_, &rect);
    }
    return rect;
}

RECT window::get_client_rect() const {
    RECT rect = {};
    if (is_valid()) {
        ::GetClientRect(handle_, &rect);
    }
    return rect;
}

void window::set_position(int x, int y, int width, int height, bool repaint) {
    if (is_valid()) {
        ::SetWindowPos(handle_, nullptr, x, y, width, height,
                       SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
    }
}

void window::set_size(int width, int height, bool repaint) {
    if (is_valid()) {
        RECT rect = get_rect();
        ::SetWindowPos(handle_, nullptr, rect.left, rect.top, width, height,
                       SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
    }
}

void window::move(int x, int y, bool repaint) {
    if (is_valid()) {
        RECT rect = get_rect();
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        ::SetWindowPos(handle_, nullptr, x, y, width, height,
                       SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
    }
}

void window::set_focus() {
    if (is_valid()) {
        ::SetFocus(handle_);
    }
}

bool window::has_focus() const {
    if (is_valid()) {
        return ::GetFocus() == handle_;
    }
    return false;
}

void window::destroy() {
    if (is_valid()) {
        ::DestroyWindow(handle_);
        handle_ = nullptr;
    }
}

void window::update() {
    if (is_valid()) {
        ::UpdateWindow(handle_);
    }
}

void window::repaint() {
    if (is_valid()) {
        ::InvalidateRect(handle_, nullptr, TRUE);
        ::UpdateWindow(handle_);
    }
}

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

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(handle_, &ps);
            RECT rect;
            ::GetClientRect(handle_, &rect);
            ::FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
            ::EndPaint(handle_, &ps);
            return 0;
        }

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

    return ::DefWindowProcW(handle_, u_msg, w_param, l_param);
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
LRESULT window::on_paint() { return 0; }
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