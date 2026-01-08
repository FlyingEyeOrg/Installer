// // window_resource.hpp
// #pragma once
// #define UNICODE
// #define _UNICODE

// #include <Windows.h>

// #include <map>
// #include <string>


// class window_resource {
//    public:
//     // 窗口类信息
//     struct window_class_info {
//         std::wstring class_name;
//         WNDPROC window_proc = nullptr;  // 窗口过程
//         UINT style = CS_HREDRAW | CS_VREDRAW;
//         HICON h_icon = nullptr;
//         HCURSOR h_cursor = LoadCursor(nullptr, IDC_ARROW);
//         HBRUSH hbr_background = (HBRUSH)(COLOR_WINDOW + 1);
//         LPCWSTR lpsz_menu_name = nullptr;
//         HICON h_icon_sm = nullptr;
//     };

//    private:
//     HINSTANCE instance_ = nullptr;
//     static constexpr const wchar_t* default_class_name_ =
//         L"default_window_class";

//     // 已注册的窗口类
//     std::map<std::wstring, window_class_info> registered_classes_;

//     window_resource();

//    public:
//     // 单例模式
//     static window_resource& instance();

//     // 禁止拷贝
//     window_resource(const window_resource&) = delete;
//     window_resource& operator=(const window_resource&) = delete;

//     // 注册窗口类（可以指定窗口过程）
//     bool register_class(const window_class_info& info,
//                         WNDPROC window_proc = nullptr);

//     // 使用应用程序全局窗口过程注册窗口类
//     bool register_class_with_app_proc(const window_class_info& info);

//     // 注册默认窗口类
//     bool register_default_class(WNDPROC window_proc = nullptr);

//     // 注销窗口类
//     bool unregister_class(const std::wstring& class_name);

//     // 检查窗口类是否已注册
//     bool is_class_registered(const std::wstring& class_name) const;

//     // 获取默认窗口类名
//     const wchar_t* get_default_class_name() const {
//         return default_class_name_;
//     }

//     // 获取应用实例句柄
//     HINSTANCE get_instance() const { return instance_; }

//     // 创建窗口
//     HWND create_window(const std::wstring& class_name,
//                        const std::wstring& window_name, DWORD dw_style, int x,
//                        int y, int width, int height, void* user_data = nullptr,
//                        HWND hwnd_parent = nullptr, HMENU h_menu = nullptr);

//     ~window_resource();
// };