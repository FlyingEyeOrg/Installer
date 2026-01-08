// #include "windows/window_resource.hpp"

// #include "windows/application.hpp"

// window_resource::window_resource() { instance_ = ::GetModuleHandleW(nullptr); }

// window_resource& window_resource::instance() {
//     static window_resource instance;
//     return instance;
// }

// window_resource::~window_resource() {
//     // 注销所有窗口类
//     for (const auto& pair : registered_classes_) {
//         ::UnregisterClassW(pair.first.c_str(), instance_);
//     }
//     registered_classes_.clear();
// }

// bool window_resource::register_class(const window_class_info& info,
//                                      WNDPROC window_proc) {
//     if (info.class_name.empty()) {
//         return false;
//     }

//     // 如果已注册，先注销
//     if (is_class_registered(info.class_name)) {
//         unregister_class(info.class_name);
//     }

//     WNDCLASSEXW wc = {};
//     wc.cbSize = sizeof(WNDCLASSEXW);
//     wc.style = info.style;
//     wc.lpfnWndProc =
//         window_proc ? window_proc : application::global_window_proc;
//     wc.hInstance = instance_;
//     wc.hIcon = info.h_icon;
//     wc.hCursor = info.h_cursor;
//     wc.hbrBackground = info.hbr_background;
//     wc.lpszMenuName = info.lpsz_menu_name;
//     wc.lpszClassName = info.class_name.c_str();
//     wc.hIconSm = info.h_icon_sm;

//     if (!::RegisterClassExW(&wc)) {
//         return false;
//     }

//     registered_classes_[info.class_name] = info;
//     return true;
// }

// bool window_resource::register_class_with_app_proc(
//     const window_class_info& info) {
//     return register_class(info, application::global_window_proc);
// }

// bool window_resource::register_default_class(WNDPROC window_proc) {
//     window_class_info info;
//     info.class_name = default_class_name_;

//     return register_class(info, window_proc);
// }

// bool window_resource::unregister_class(const std::wstring& class_name) {
//     auto it = registered_classes_.find(class_name);
//     if (it != registered_classes_.end()) {
//         registered_classes_.erase(it);
//         return ::UnregisterClassW(class_name.c_str(), instance_) != FALSE;
//     }
//     return false;
// }

// bool window_resource::is_class_registered(
//     const std::wstring& class_name) const {
//     return registered_classes_.find(class_name) != registered_classes_.end();
// }

// HWND window_resource::create_window(const std::wstring& class_name,
//                                     const std::wstring& window_name,
//                                     DWORD dw_style, int x, int y, int width,
//                                     int height, void* user_data,
//                                     HWND hwnd_parent, HMENU h_menu) {
//     if (!is_class_registered(class_name)) {
//         // 自动注册默认窗口类
//         if (class_name == default_class_name_) {
//             if (!register_default_class()) {
//                 return nullptr;
//             }
//         } else {
//             return nullptr;
//         }
//     }

//     DWORD ex_style = 0;

//     return ::CreateWindowExW(
//         ex_style, class_name.c_str(), window_name.c_str(), dw_style, x, y,
//         (width == CW_USEDEFAULT) ? CW_USEDEFAULT : (width > 0 ? width : 0),
//         (height == CW_USEDEFAULT) ? CW_USEDEFAULT : (height > 0 ? height : 0),
//         hwnd_parent, h_menu, instance_, user_data);
// }