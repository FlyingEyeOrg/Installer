#include "windows/hwnd_wrapper.hpp"

namespace windows {

// 静态变量初始化
HINSTANCE hwnd_wrapper::app_instance_ = nullptr;
std::mutex hwnd_wrapper::window_class_manager::mutex_;
std::unordered_map<DWORD, ATOM> hwnd_wrapper::window_class_manager::class_map_;
ATOM hwnd_wrapper::window_class_manager::default_class_atom_ = 0;

// 生成基于 class_style 的类名
std::wstring hwnd_wrapper::window_class_manager::generate_class_name(
    DWORD class_style) {
    std::wstringstream ss;
    ss << L"hwnd_wrapper_class_";
    ss << std::hex << class_style;
    return ss.str();
}

// 根据 class_style 获取窗口类原子
ATOM hwnd_wrapper::window_class_manager::get_class_atom(DWORD class_style) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 首先检查是否已注册
    auto it = class_map_.find(class_style);
    if (it != class_map_.end()) {
        return it->second;  // 已注册，返回现有原子
    }

    // 需要注册新类
    HINSTANCE hInstance = ::GetModuleHandleW(nullptr);
    std::wstring className = generate_class_name(class_style);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = class_style;
    wc.lpfnWndProc = global_wnd_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR);

    wc.hInstance = hInstance;
    wc.hIcon = nullptr;                              // 默认值
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);  // 默认光标
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);   // 默认背景
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = className.c_str();
    wc.hIconSm = nullptr;  // 默认值

    ATOM atom = ::RegisterClassExW(&wc);
    if (atom != 0) {
        class_map_[class_style] = atom;
    }

    return atom;
}

// 获取默认窗口类原子 (CS_HREDRAW | CS_VREDRAW)
ATOM hwnd_wrapper::window_class_manager::get_default_class_atom() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (default_class_atom_ != 0) {
        return default_class_atom_;
    }

    default_class_atom_ = get_class_atom(CS_HREDRAW | CS_VREDRAW);
    return default_class_atom_;
}

// 清理所有注册的窗口类
void hwnd_wrapper::window_class_manager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);

    HINSTANCE hInstance = ::GetModuleHandleW(nullptr);

    for (const auto& [style, atom] : class_map_) {
        ::UnregisterClassW(MAKEINTATOM(atom), hInstance);
    }

    class_map_.clear();
    default_class_atom_ = 0;
}

// 获取已注册的类数量
size_t hwnd_wrapper::window_class_manager::get_registered_count() {
    std::lock_guard<std::mutex> lock(mutex_);
    return class_map_.size();
}

// 获取指定样式的类名
std::wstring hwnd_wrapper::window_class_manager::get_class_name(
    DWORD class_style) {
    return generate_class_name(class_style);
}

// 全局窗口过程实现
LRESULT CALLBACK hwnd_wrapper::global_wnd_proc(HWND hwnd, UINT u_msg,
                                               WPARAM w_param, LPARAM l_param) {
    // 在 WM_NCCREATE 中保存对象指针
    if (u_msg == WM_NCCREATE) {
        CREATESTRUCTW* p_create = reinterpret_cast<CREATESTRUCTW*>(l_param);
        if (p_create && p_create->lpCreateParams) {
            hwnd_wrapper* wrapper =
                reinterpret_cast<hwnd_wrapper*>(p_create->lpCreateParams);
            // 保存到 USERDATA
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                                reinterpret_cast<LONG_PTR>(wrapper));
            wrapper->handle_ = hwnd;
        }
    }

    // 获取对象指针
    hwnd_wrapper* wrapper = reinterpret_cast<hwnd_wrapper*>(
        ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    LRESULT result = 0;
    bool handled = false;

    if (wrapper) {
        // 调用对象的处理函数
        result = wrapper->wnd_proc(u_msg, w_param, l_param, handled);

        // 特殊处理 WM_DESTROY - 清理资源
        if (u_msg == WM_DESTROY) {
            // 清除 USERDATA，避免野指针
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            wrapper->handle_ = nullptr;
        }
    }

    if (!handled) {
        result = ::DefWindowProcW(hwnd, u_msg, w_param, l_param);
    }

    return result;
}

LRESULT CALLBACK hwnd_wrapper::wnd_proc(UINT u_msg, WPARAM w_param,
                                        LPARAM l_param, bool& handled) {
    handled = false;

    // 调用所有 hook 函数
    for (const auto& hook : hooks_) {
        bool hook_handled = false;
        LRESULT hook_result =
            hook(handle_, u_msg, w_param, l_param, hook_handled);
        if (hook_handled) {
            handled = true;
            return hook_result;
        }
    }

    return 0;
}

// 构造函数
hwnd_wrapper::hwnd_wrapper(DWORD class_style, DWORD window_exstyle,
                           DWORD window_style, const std::wstring& name, int x,
                           int y, int width, int height, HWND parent,
                           const std::vector<hwnd_wrapper_hook_func>& hooks)
    : hooks_(hooks) {
    // 确保 app_instance_ 已初始化
    if (app_instance_ == nullptr) {
        app_instance_ = ::GetModuleHandleW(nullptr);
    }

    // 根据 class_style 获取窗口类原子
    class_atom_ = window_class_manager::get_class_atom(class_style);

    if (class_atom_ == 0) {
        // 注册失败
        return;
    }

    // 创建窗口
    handle_ = ::CreateWindowExW(window_exstyle, MAKEINTATOM(class_atom_),
                                name.c_str(), window_style, x, y, width, height,
                                parent, nullptr, app_instance_, this);
}

// 析构函数
hwnd_wrapper::~hwnd_wrapper() {
    if (handle_ && ::IsWindow(handle_)) {
        ::DestroyWindow(handle_);
    }
    handle_ = nullptr;
    // 注意：不在这里注销窗口类，由 window_class_manager 统一管理
}

// 移动构造函数
hwnd_wrapper::hwnd_wrapper(hwnd_wrapper&& other) noexcept
    : handle_(other.handle_),
      hooks_(std::move(other.hooks_)),
      class_atom_(other.class_atom_) {
    other.handle_ = nullptr;
    other.class_atom_ = 0;

    // 更新 USERDATA 指针
    if (handle_) {
        ::SetWindowLongPtrW(handle_, GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(this));
    }
}

// 移动赋值运算符
hwnd_wrapper& hwnd_wrapper::operator=(hwnd_wrapper&& other) noexcept {
    if (this != &other) {
        // 清理现有窗口
        if (handle_ && ::IsWindow(handle_)) {
            ::DestroyWindow(handle_);
        }

        // 移动资源
        handle_ = other.handle_;
        hooks_ = std::move(other.hooks_);
        class_atom_ = other.class_atom_;

        other.handle_ = nullptr;
        other.class_atom_ = 0;

        // 更新 USERDATA 指针
        if (handle_) {
            ::SetWindowLongPtrW(handle_, GWLP_USERDATA,
                                reinterpret_cast<LONG_PTR>(this));
        }
    }
    return *this;
}

// 静态方法
void hwnd_wrapper::cleanup() { window_class_manager::cleanup(); }

size_t hwnd_wrapper::get_registered_class_count() {
    return window_class_manager::get_registered_count();
}

std::wstring hwnd_wrapper::get_class_name(DWORD class_style) {
    return window_class_manager::get_class_name(class_style);
}

// 添加 hook
void hwnd_wrapper::add_hook(const hwnd_wrapper_hook_func& hook) {
    hooks_.push_back(hook);
}

// 添加 hook（移动语义）
void hwnd_wrapper::add_hook(hwnd_wrapper_hook_func&& hook) {
    hooks_.push_back(std::move(hook));
}

// 移除 hook
bool hwnd_wrapper::remove_hook(const hwnd_wrapper_hook_func& hook) {
    auto it = std::find(hooks_.begin(), hooks_.end(), hook);
    if (it != hooks_.end()) {
        hooks_.erase(it);
        return true;
    }
    return false;
}

// 移除所有 hooks
void hwnd_wrapper::clear_hooks() { hooks_.clear(); }

// 获取 hook 数量
size_t hwnd_wrapper::hook_count() const { return hooks_.size(); }

// 获取窗口句柄
HWND hwnd_wrapper::get_handle() const { return handle_; }

// 检查窗口是否有效
bool hwnd_wrapper::is_valid() const {
    return handle_ != nullptr && ::IsWindow(handle_);
}

// 获取窗口类原子
ATOM hwnd_wrapper::get_class_atom() const { return class_atom_; }

// 获取窗口类样式
DWORD hwnd_wrapper::get_class_style() const {
    if (class_atom_ == 0) return 0;

    HINSTANCE hInstance = ::GetModuleHandleW(nullptr);
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);

    if (::GetClassInfoExW(hInstance, MAKEINTATOM(class_atom_), &wc)) {
        return wc.style;
    }
    return 0;
}
}  // namespace windows