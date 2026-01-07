// window_resource.cpp
#include "windows/window_resource.hpp"

#include <algorithm>

#include "windows/application.hpp"
#include "windows/window.hpp"

// 静态成员初始化
std::mutex window_resource::windows_mutex_;
std::mutex window_resource::class_mutex_;

window_resource::window_resource() {
    instance_ = ::GetModuleHandleW(nullptr);
    register_default_class();
}

window_resource::~window_resource() { cleanup(); }

window_resource& window_resource::get_instance() {
    static window_resource instance;
    return instance;
}

bool window_resource::register_default_class() {
    return register_window_class_default(default_class_name_);
}

bool window_resource::is_class_registered(
    const std::wstring& class_name) const {
    std::lock_guard<std::mutex> lock(class_mutex_);
    return registered_classes_.find(class_name) != registered_classes_.end();
}

bool window_resource::register_window_class(const std::wstring& class_name,
                                            WNDPROC custom_proc, UINT style,
                                            HICON h_icon, HCURSOR h_cursor,
                                            HBRUSH hbr_background,
                                            LPCWSTR lpsz_menu_name,
                                            HICON h_icon_sm) {
    if (class_name.empty()) return false;

    std::lock_guard<std::mutex> lock(class_mutex_);

    // 如果已经注册，先注销
    if (registered_classes_.find(class_name) != registered_classes_.end()) {
        ::UnregisterClassW(class_name.c_str(), instance_);
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = style;
    wc.lpfnWndProc = custom_proc
                         ? custom_proc
                         : application::instance().get_global_window_proc();
    wc.hInstance = instance_;
    wc.hIcon = h_icon;
    wc.hCursor = h_cursor;
    wc.hbrBackground = hbr_background;
    wc.lpszMenuName = lpsz_menu_name;
    wc.lpszClassName = class_name.c_str();
    wc.hIconSm = h_icon_sm;

    if (!RegisterClassExW(&wc)) return false;

    // 保存窗口类信息
    registered_classes_[class_name] = window_class_info{
        class_name,
        custom_proc ? custom_proc
                    : application::instance().get_global_window_proc(),
        style,
        h_icon,
        h_cursor,
        hbr_background,
        lpsz_menu_name,
        h_icon_sm};

    return true;
}

bool window_resource::register_window_class_default(
    const std::wstring& class_name, UINT style, HICON h_icon, HCURSOR h_cursor,
    HBRUSH hbr_background, LPCWSTR lpsz_menu_name, HICON h_icon_sm) {
    return register_window_class(class_name, nullptr, style, h_icon, h_cursor,
                                 hbr_background, lpsz_menu_name, h_icon_sm);
}

HWND window_resource::create_window(const std::wstring& window_name,
                                    DWORD dw_style, int x, int y, int width,
                                    int height, std::shared_ptr<window> win,
                                    HWND hwnd_parent, HMENU h_menu) {
    return create_window_with_class(default_class_name_, window_name, dw_style,
                                    x, y, width, height, win, hwnd_parent,
                                    h_menu);
}

HWND window_resource::create_window_with_class(const std::wstring& class_name,
                                               const std::wstring& window_name,
                                               DWORD dw_style, int x, int y,
                                               int width, int height,
                                               std::shared_ptr<window> win,
                                               HWND hwnd_parent, HMENU h_menu) {
    if (class_name.empty() || !win) return nullptr;

    // 确保窗口类已注册
    if (!is_class_registered(class_name)) {
        if (!register_window_class_default(class_name)) {
            return nullptr;
        }
    }

    // 检查是否已经存在相同的window对象
    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        auto it = std::find_if(
            windows_.begin(), windows_.end(),
            [&win](const auto& pair) { return pair.second == win; });
        if (it != windows_.end()) {
            windows_.erase(it);
        }
        windows_[nullptr] = win;
    }

    HWND hwnd = ::CreateWindowW(class_name.c_str(), window_name.c_str(),
                                dw_style, x, y, width, height, hwnd_parent,
                                h_menu, instance_, win.get());

    if (!hwnd) {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        auto it = std::find_if(windows_.begin(), windows_.end(),
                               [&win](const auto& pair) {
                                   return !pair.first && pair.second == win;
                               });
        if (it != windows_.end()) {
            windows_.erase(it);
        }
    }

    return hwnd;
}

std::shared_ptr<window> window_resource::find_window(HWND hwnd) const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    auto it = windows_.find(hwnd);
    return it != windows_.end() ? it->second : nullptr;
}

void window_resource::cleanup_all_windows() {
    std::vector<HWND> window_handles;
    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        for (const auto& pair : windows_) {
            if (pair.first) {
                window_handles.push_back(pair.first);
            }
        }
        windows_.clear();
    }

    for (HWND hwnd : window_handles) {
        ::DestroyWindow(hwnd);
    }
}

void window_resource::unregister_all_classes() {
    std::lock_guard<std::mutex> lock(class_mutex_);
    for (const auto& pair : registered_classes_) {
        ::UnregisterClassW(pair.first.c_str(), instance_);
    }
    registered_classes_.clear();
}

void window_resource::cleanup() {
    cleanup_all_windows();
    unregister_all_classes();
}

bool window_resource::update_window_mapping(HWND hwnd, window* win_ptr) {
    if (!hwnd || !win_ptr) return false;

    std::lock_guard<std::mutex> lock(windows_mutex_);

    // 查找是否有以空句柄存储的对应窗口
    auto it = std::find_if(
        windows_.begin(), windows_.end(), [win_ptr](const auto& pair) {
            return !pair.first && pair.second.get() == win_ptr;
        });

    if (it != windows_.end()) {
        // 找到对应的空句柄映射，更新为实际句柄
        auto win = it->second;
        windows_.erase(it);
        windows_[hwnd] = win;
        return true;
    }

    return false;
}

bool window_resource::remove_window_mapping(HWND hwnd) {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    return windows_.erase(hwnd) > 0;
}

std::size_t window_resource::get_window_count() const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    return std::count_if(
        windows_.begin(), windows_.end(),
        [](const auto& pair) { return pair.first != nullptr; });
}

bool window_resource::window_exists(HWND hwnd) const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    return windows_.find(hwnd) != windows_.end();
}

std::vector<HWND> window_resource::get_all_window_handles() const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    std::vector<HWND> handles;
    handles.reserve(windows_.size());

    for (const auto& pair : windows_) {
        if (pair.first) {  // 只返回有效的句柄
            handles.push_back(pair.first);
        }
    }

    return handles;
}