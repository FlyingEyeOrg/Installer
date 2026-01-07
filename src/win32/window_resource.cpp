#include "win32/window_resource.hpp"

#include <algorithm>

#include "win32/window.hpp"

// 静态成员初始化
std::mutex window_resource::windows_mutex_;
std::mutex window_resource::class_mutex_;

window_resource::window_resource() {
    instance_ = ::GetModuleHandleW(nullptr);

    // 自动注册默认窗口类
    register_default_class();
}

window_resource::~window_resource() { cleanup(); }

window_resource& window_resource::get_instance() {
    // 线程安全，编译器会在幕后处理所有同步
    static window_resource instance;
    return instance;
}

bool window_resource::register_default_class() {
    return register_window_class_default(default_class_name_);
}

bool window_resource::is_class_registered(
    const std::wstring& class_name) const {
    std::lock_guard<std::mutex> lock(class_mutex_);

    // 检查是否在我们的注册表中
    if (registered_classes_.find(class_name) != registered_classes_.end()) {
        return true;
    }

    // 检查是否在系统中已注册
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    return GetClassInfoExW(instance_, class_name.c_str(), &wc) != 0;
}

bool window_resource::register_window_class(const std::wstring& class_name,
                                            WNDPROC custom_proc, UINT style,
                                            HICON h_icon, HCURSOR h_cursor,
                                            HBRUSH hbr_background,
                                            LPCWSTR lpsz_menu_name,
                                            HICON h_icon_sm) {
    if (class_name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(class_mutex_);

    // 如果已经注册，先注销
    if (registered_classes_.find(class_name) != registered_classes_.end()) {
        ::UnregisterClassW(class_name.c_str(), instance_);
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = style;
    wc.lpfnWndProc = custom_proc ? custom_proc : global_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance_;
    wc.hIcon = h_icon;
    wc.hCursor = h_cursor;
    wc.hbrBackground = hbr_background;
    wc.lpszMenuName = lpsz_menu_name;
    wc.lpszClassName = class_name.c_str();
    wc.hIconSm = h_icon_sm;

    ATOM atom = RegisterClassExW(&wc);
    if (atom == 0) {
        return false;
    }

    // 保存窗口类信息
    window_class_info info;
    info.class_name = class_name;
    info.window_proc = custom_proc ? custom_proc : global_window_proc;
    info.style = style;
    info.h_icon = h_icon;
    info.h_cursor = h_cursor;
    info.hbr_background = hbr_background;
    info.lpsz_menu_name = lpsz_menu_name;
    info.h_icon_sm = h_icon_sm;

    registered_classes_[class_name] = info;
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
    // 使用默认窗口类
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
    if (class_name.empty() || !win) {
        return nullptr;
    }

    // 确保窗口类已注册
    if (!is_class_registered(class_name)) {
        if (!register_window_class_default(class_name)) {
            return nullptr;
        }
    }

    // 检查是否已经存在相同的window对象
    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        // 防止重复注册同一个window对象
        auto it = std::find_if(
            windows_.begin(), windows_.end(),
            [&win](const auto& pair) { return pair.second == win; });
        if (it != windows_.end()) {
            // 如果已经存在，移除旧的
            windows_.erase(it);
        }
        // 临时注册
        windows_[nullptr] = win;
    }

    HWND hwnd = ::CreateWindowW(class_name.c_str(), window_name.c_str(),
                                dw_style, x, y, width, height, hwnd_parent,
                                h_menu, instance_, win.get());

    if (!hwnd) {
        // 创建失败，清理临时注册
        std::lock_guard<std::mutex> lock(windows_mutex_);
        // 只移除与这个win对象关联的nullptr条目
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

LRESULT CALLBACK window_resource::global_window_proc(HWND hwnd, UINT u_msg,
                                                     WPARAM w_param,
                                                     LPARAM l_param) {
    // 先尝试查找已注册的窗口
    window_resource& instance = get_instance();
    std::shared_ptr<window> win = nullptr;

    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        auto it = instance.windows_.find(hwnd);
        if (it != instance.windows_.end()) {
            win = it->second;
        }
    }

    // 处理创建消息
    if (u_msg == WM_NCCREATE && !win) {
        CREATESTRUCTW* p_create = reinterpret_cast<CREATESTRUCTW*>(l_param);
        if (p_create && p_create->lpCreateParams) {
            window* win_ptr =
                reinterpret_cast<window*>(p_create->lpCreateParams);
            std::lock_guard<std::mutex> lock(windows_mutex_);
            // 设置窗口句柄
            win_ptr->set_window_handle(hwnd);
            // 查找并更新临时注册的窗口
            auto it =
                std::find_if(instance.windows_.begin(), instance.windows_.end(),
                             [win_ptr](const auto& pair) {
                                 return pair.second.get() == win_ptr;
                             });

            if (it != instance.windows_.end()) {
                win = it->second;
                // 移除旧条目，添加新条目
                instance.windows_.erase(it);
                instance.windows_[hwnd] = win;
            }
        }
    }

    // 处理消息
    if (win) {
        LRESULT result = win->window_proc(u_msg, w_param, l_param);

        // 处理销毁消息，检查是否是最后一个窗口
        if (u_msg == WM_DESTROY) {
            std::lock_guard<std::mutex> lock(windows_mutex_);
            // 确保从映射表中移除
            instance.windows_.erase(hwnd);

            // 检查是否还有窗口存在
            if (instance.windows_.empty()) {
                // 这是最后一个窗口，退出程序
                PostQuitMessage(0);
            }
        }

        return result;
    }

    // 默认消息处理
    return DefWindowProcW(hwnd, u_msg, w_param, l_param);
}

void window_resource::register_window(HWND hwnd, std::shared_ptr<window> win) {
    if (hwnd && win) {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        windows_[hwnd] = win;
    }
}

void window_resource::unregister_window(HWND hwnd) {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    windows_.erase(hwnd);
}

bool window_resource::unregister_class(const std::wstring& class_name) {
    std::lock_guard<std::mutex> lock(class_mutex_);

    auto it = registered_classes_.find(class_name);
    if (it != registered_classes_.end()) {
        if (::UnregisterClassW(class_name.c_str(), instance_)) {
            registered_classes_.erase(it);
            return true;
        }
    }
    return false;
}

void window_resource::destroy_window(HWND hwnd) {
    if (hwnd) {
        ::DestroyWindow(hwnd);
        unregister_window(hwnd);
    }
}

std::shared_ptr<window> window_resource::find_window(HWND hwnd) const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    auto it = windows_.find(hwnd);
    if (it != windows_.end()) {
        return it->second;
    }
    return nullptr;
}

HINSTANCE window_resource::get_current_instance() const { return instance_; }

std::vector<std::wstring> window_resource::get_registered_classes() const {
    std::lock_guard<std::mutex> lock(class_mutex_);
    std::vector<std::wstring> classes;
    for (const auto& pair : registered_classes_) {
        classes.push_back(pair.first);
    }
    return classes;
}

void window_resource::cleanup_all_windows() {
    std::vector<std::shared_ptr<window>> windows_copy;
    std::vector<HWND> window_handles;

    {
        std::lock_guard<std::mutex> lock(windows_mutex_);

        // 收集所有有效窗口句柄
        for (const auto& pair : windows_) {
            if (pair.first) {  // 跳过 nullptr 键
                window_handles.push_back(pair.first);
                windows_copy.push_back(pair.second);
            }
        }

        // 清空映射，防止递归问题
        windows_.clear();
    }

    // 销毁所有窗口（现在映射表已清空，不会触发递归）
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

std::size_t window_resource::get_window_count() const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    // 不统计 nullptr 键
    return std::count_if(
        windows_.begin(), windows_.end(),
        [](const auto& pair) { return pair.first != nullptr; });
}

std::size_t window_resource::get_window_count_by_class(
    const std::wstring& class_name) const {
    std::lock_guard<std::mutex> lock(windows_mutex_);

    std::size_t count = 0;
    for (const auto& pair : windows_) {
        if (pair.first) {  // 跳过 nullptr 键
            wchar_t actual_class[256] = {0};
            if (GetClassNameW(pair.first, actual_class, 256) > 0) {
                if (class_name == actual_class) {
                    ++count;
                }
            }
        }
    }
    return count;
}

int window_resource::run_message_loop() {
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

bool window_resource::process_pending_messages() {
    MSG msg = {};
    bool has_messages = false;

    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        has_messages = true;
        if (msg.message == WM_QUIT) {
            PostQuitMessage(static_cast<int>(msg.wParam));
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return has_messages;
}