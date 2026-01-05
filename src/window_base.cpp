// window_base.cpp
#include "win32/window_base.hpp"

#include <windowsx.h>

#include <cassert>
#include <random>
#include <sstream>

namespace win32 {

// 生成唯一的窗口类名
std::wstring window_base::generate_class_name() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 0xFFFF);

    std::wstringstream ss;
    ss << L"Win32_Window_Class_" << dis(gen);
    return ss.str();
}

// 获取窗口映射表
std::unordered_map<HWND, window_base*>& window_base::window_map() {
    static std::unordered_map<HWND, window_base*> map;
    return map;
}

window_base::window_base(const std::wstring& title, int width, int height)
    : title_(title),
      width_(width),
      height_(height),
      class_name_(generate_class_name()) {}

window_base::~window_base() {
    if (handle_) {
        window_map().erase(handle_);
    }
}

window_base::window_base(window_base&& other) noexcept
    : handle_(other.handle_),
      parent_(other.parent_),
      instance_(other.instance_),
      class_name_(std::move(other.class_name_)),
      title_(std::move(other.title_)),
      width_(other.width_),
      height_(other.height_),
      is_created_(other.is_created_),
      message_handler_(std::move(other.message_handler_)),
      close_handler_(std::move(other.close_handler_)),
      create_handler_(std::move(other.create_handler_)),
      destroy_handler_(std::move(other.destroy_handler_)),
      paint_handler_(std::move(other.paint_handler_)),
      resize_handler_(std::move(other.resize_handler_)),
      key_down_handler_(std::move(other.key_down_handler_)),
      key_up_handler_(std::move(other.key_up_handler_)),
      mouse_move_handler_(std::move(other.mouse_move_handler_)),
      mouse_down_handler_(std::move(other.mouse_down_handler_)),
      mouse_up_handler_(std::move(other.mouse_up_handler_)),
      mouse_wheel_handler_(std::move(other.mouse_wheel_handler_)) {
    if (handle_) {
        window_map()[handle_] = this;
    }

    other.handle_ = nullptr;
    other.parent_ = nullptr;
    other.instance_ = nullptr;
    other.is_created_ = false;
    other.width_ = 0;
    other.height_ = 0;
}

window_base& window_base::operator=(window_base&& other) noexcept {
    if (this != &other) {
        if (handle_) {
            window_map().erase(handle_);
        }

        handle_ = other.handle_;
        parent_ = other.parent_;
        instance_ = other.instance_;
        class_name_ = std::move(other.class_name_);
        title_ = std::move(other.title_);
        width_ = other.width_;
        height_ = other.height_;
        is_created_ = other.is_created_;
        message_handler_ = std::move(other.message_handler_);
        close_handler_ = std::move(other.close_handler_);
        create_handler_ = std::move(other.create_handler_);
        destroy_handler_ = std::move(other.destroy_handler_);
        paint_handler_ = std::move(other.paint_handler_);
        resize_handler_ = std::move(other.resize_handler_);
        key_down_handler_ = std::move(other.key_down_handler_);
        key_up_handler_ = std::move(other.key_up_handler_);
        mouse_move_handler_ = std::move(other.mouse_move_handler_);
        mouse_down_handler_ = std::move(other.mouse_down_handler_);
        mouse_up_handler_ = std::move(other.mouse_up_handler_);
        mouse_wheel_handler_ = std::move(other.mouse_wheel_handler_);

        if (handle_) {
            window_map()[handle_] = this;
        }

        other.handle_ = nullptr;
        other.parent_ = nullptr;
        other.instance_ = nullptr;
        other.is_created_ = false;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

bool window_base::register_class(const std::wstring& class_name,
                                 HINSTANCE instance,
                                 const style& window_style) {
    if (!instance) {
        instance = GetModuleHandleW(nullptr);
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = window_style.class_style;
    wc.lpfnWndProc = static_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = window_style.icon ? window_style.icon
                                 : LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = window_style.cursor ? window_style.cursor
                                     : LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = window_style.background;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = class_name.c_str();
    wc.hIconSm = window_style.icon_small ? window_style.icon_small
                                         : LoadIconW(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wc) != 0;
}

void window_base::unregister_class(const std::wstring& class_name,
                                   HINSTANCE instance) {
    if (!instance) {
        instance = GetModuleHandleW(nullptr);
    }
    UnregisterClassW(class_name.c_str(), instance);
}

bool window_base::create(HINSTANCE instance, HWND parent,
                         const style& window_style) {
    if (is_created_) {
        return true;
    }

    if (!instance) {
        instance = GetModuleHandleW(nullptr);
    }
    instance_ = instance;
    parent_ = parent;

    if (!register_class(class_name_, instance_, window_style)) {
        return false;
    }

    // 计算窗口大小
    RECT rect = {0, 0, width_, height_};
    AdjustWindowRectEx(&rect, window_style.style, FALSE, window_style.ex_style);
    int adjusted_width = rect.right - rect.left;
    int adjusted_height = rect.bottom - rect.top;

    // 获取屏幕中心位置
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    int x = (screen_width - adjusted_width) / 2;
    int y = (screen_height - adjusted_height) / 2;

    handle_ = CreateWindowExW(window_style.ex_style, class_name_.c_str(),
                              title_.c_str(), window_style.style, x, y,
                              adjusted_width, adjusted_height, parent_, nullptr,
                              instance_, this);

    if (!handle_) {
        return false;
    }

    window_map()[handle_] = this;
    is_created_ = true;
    return true;
}

void window_base::show(int cmd_show) {
    assert(is_created_ && "Window must be created before showing");
    ShowWindow(handle_, cmd_show);
    UpdateWindow(handle_);
}

void window_base::hide() {
    assert(is_created_);
    ShowWindow(handle_, SW_HIDE);
}

void window_base::minimize() {
    assert(is_created_);
    ShowWindow(handle_, SW_MINIMIZE);
}

void window_base::maximize() {
    assert(is_created_);
    ShowWindow(handle_, SW_MAXIMIZE);
}

void window_base::restore() {
    assert(is_created_);
    ShowWindow(handle_, SW_RESTORE);
}

void window_base::close() {
    assert(is_created_);
    SendMessageW(handle_, WM_CLOSE, 0, 0);
}

void window_base::destroy() {
    if (handle_) {
        DestroyWindow(handle_);
    }
}

bool window_base::is_visible() const {
    assert(is_created_);
    return IsWindowVisible(handle_) != FALSE;
}

bool window_base::is_minimized() const {
    assert(is_created_);
    return IsIconic(handle_) != FALSE;
}

bool window_base::is_maximized() const {
    assert(is_created_);
    return IsZoomed(handle_) != FALSE;
}

std::wstring window_base::title() const {
    assert(is_created_);
    int length = GetWindowTextLengthW(handle_);
    if (length <= 0) {
        return L"";
    }

    std::wstring title(length, L'\0');
    GetWindowTextW(handle_, title.data(), length + 1);
    return title;
}

void window_base::set_title(const std::wstring& title) {
    title_ = title;
    if (is_created_) {
        SetWindowTextW(handle_, title_.c_str());
    }
}

void window_base::set_size(int width, int height, bool redraw) {
    width_ = width;
    height_ = height;
    if (is_created_) {
        SetWindowPos(handle_, nullptr, 0, 0, width, height,
                     SWP_NOMOVE | SWP_NOZORDER | (redraw ? 0 : SWP_NOREDRAW));
    }
}

void window_base::set_position(int x, int y, bool redraw) {
    if (is_created_) {
        SetWindowPos(handle_, nullptr, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | (redraw ? 0 : SWP_NOREDRAW));
    }
}

void window_base::set_client_size(int width, int height, bool redraw) {
    if (!is_created_) {
        width_ = width;
        height_ = height;
        return;
    }

    RECT rect = {0, 0, width, height};
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(handle_, GWL_STYLE));
    DWORD ex_style =
        static_cast<DWORD>(GetWindowLongPtrW(handle_, GWL_EXSTYLE));

    AdjustWindowRectEx(&rect, style, FALSE, ex_style);

    SetWindowPos(handle_, nullptr, 0, 0, rect.right - rect.left,
                 rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | (redraw ? 0 : SWP_NOREDRAW));
}

void window_base::get_position(int& x, int& y) const {
    assert(is_created_);
    RECT rect;
    GetWindowRect(handle_, &rect);
    x = rect.left;
    y = rect.top;
}

void window_base::get_size(int& width, int& height) const {
    assert(is_created_);
    RECT rect;
    GetWindowRect(handle_, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

void window_base::get_client_size(int& width, int& height) const {
    assert(is_created_);
    RECT rect;
    GetClientRect(handle_, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

RECT window_base::get_client_rect() const {
    assert(is_created_);
    RECT rect = {};
    GetClientRect(handle_, &rect);
    return rect;
}

RECT window_base::get_window_rect() const {
    assert(is_created_);
    RECT rect = {};
    GetWindowRect(handle_, &rect);
    return rect;
}

LRESULT window_base::send_message(UINT msg, WPARAM wparam,
                                  LPARAM lparam) const {
    assert(is_created_);
    return SendMessageW(handle_, msg, wparam, lparam);
}

bool window_base::post_message(UINT msg, WPARAM wparam, LPARAM lparam) const {
    assert(is_created_);
    return PostMessageW(handle_, msg, wparam, lparam) != FALSE;
}

window_base* window_base::from_handle(HWND hwnd) {
    auto it = window_map().find(hwnd);
    return it != window_map().end() ? it->second : nullptr;
}

LRESULT CALLBACK window_base::static_window_proc(HWND hwnd, UINT msg,
                                                 WPARAM wparam, LPARAM lparam) {
    window_base* window = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
        window = static_cast<window_base*>(create_struct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(window));
        window->handle_ = hwnd;
        window_map()[hwnd] = window;
    } else {
        window = from_handle(hwnd);
    }

    if (window) {
        return window->window_proc(hwnd, msg, wparam, lparam);
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT window_base::window_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                 LPARAM lparam) {
    // 调用用户自定义消息处理器
    if (message_handler_) {
        auto result = message_handler_(hwnd, msg, wparam, lparam);
        if (result.has_value()) {
            return result.value();
        }
    }

    switch (msg) {
        case WM_CREATE:
            return on_create();

        case WM_DESTROY:
            return on_destroy();

        case WM_CLOSE:
            return on_close();

        case WM_PAINT:
            return on_paint();

        case WM_SIZE:
            return on_size(wparam, lparam);

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            return on_key(msg, wparam, lparam);

        case WM_CHAR:
            return on_char(wparam, lparam);

        case WM_MOUSEMOVE:
            return on_mouse_move(wparam, lparam);

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return on_mouse_button(msg, wparam, lparam);

        case WM_MOUSEWHEEL:
            return on_mouse_wheel(wparam, lparam);

        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

LRESULT window_base::on_create() {
    if (create_handler_) {
        create_handler_();
    }
    return 0;
}

LRESULT window_base::on_destroy() {
    if (destroy_handler_) {
        destroy_handler_();
    }

    window_map().erase(handle_);
    handle_ = nullptr;
    is_created_ = false;

    return 0;
}

LRESULT window_base::on_close() {
    if (close_handler_) {
        close_handler_();
        return 0;
    }

    DestroyWindow(handle_);
    return 0;
}

LRESULT window_base::on_paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(handle_, &ps);

    if (paint_handler_) {
        paint_handler_(hdc);
    } else {
        // 默认绘制
        FillRect(hdc, &ps.rcPaint,
                 reinterpret_cast<HBRUSH>(
                     GetClassLongPtrW(handle_, GCLP_HBRBACKGROUND)));
    }

    EndPaint(handle_, &ps);
    return 0;
}

LRESULT window_base::on_size(WPARAM wparam, LPARAM lparam) {
    width_ = LOWORD(lparam);
    height_ = HIWORD(lparam);

    if (resize_handler_) {
        resize_handler_(width_, height_);
    }

    return 0;
}

LRESULT window_base::on_key(UINT msg, WPARAM wparam, LPARAM lparam) {
    bool is_down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    bool is_up = (msg == WM_KEYUP || msg == WM_SYSKEYUP);

    if (is_down && key_down_handler_) {
        key_down_handler_(wparam, (lparam & 0x40000000) != 0, lparam);
    } else if (is_up && key_up_handler_) {
        key_up_handler_(wparam, false, lparam);
    }

    return 0;
}

LRESULT window_base::on_char(WPARAM wparam, LPARAM lparam) { return 0; }

LRESULT window_base::on_mouse_move(WPARAM wparam, LPARAM lparam) {
    int x = GET_X_LPARAM(lparam);
    int y = GET_Y_LPARAM(lparam);

    if (mouse_move_handler_) {
        mouse_move_handler_(x, y, wparam);
    }

    return 0;
}

LRESULT window_base::on_mouse_button(UINT msg, WPARAM wparam, LPARAM lparam) {
    int x = GET_X_LPARAM(lparam);
    int y = GET_Y_LPARAM(lparam);
    int button = 0;

    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            button = 1;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            button = 2;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            button = 3;
            break;
    }

    if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
        msg == WM_MBUTTONDOWN) {
        if (mouse_down_handler_) {
            mouse_down_handler_(x, y, wparam, button);
        }
    } else {
        if (mouse_up_handler_) {
            mouse_up_handler_(x, y, wparam, button);
        }
    }

    return 0;
}

LRESULT window_base::on_mouse_wheel(WPARAM wparam, LPARAM lparam) {
    POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    ScreenToClient(handle_, &pt);

    int delta = GET_WHEEL_DELTA_WPARAM(wparam);

    if (mouse_wheel_handler_) {
        mouse_wheel_handler_(pt.x, pt.y, wparam, delta);
    }

    return 0;
}

}  // namespace win32