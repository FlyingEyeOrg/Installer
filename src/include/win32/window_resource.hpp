#pragma once
// 使用 unicode api
#define UNICODE
#define _UNICODE

#include <Windows.h>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// 前向声明 window 类
class window;

// 管理注册窗口类和窗口资源
// 提供窗口类注册和实例管理功能
class window_resource {
   private:
    HINSTANCE instance_ = nullptr;

    // 默认窗口类名
    static constexpr const wchar_t* default_class_name_ =
        L"default_window_class";

    // 窗口类信息结构
    struct window_class_info {
        std::wstring class_name;
        WNDPROC window_proc = nullptr;
        UINT style = 0;
        HICON h_icon = nullptr;
        HCURSOR h_cursor = nullptr;
        HBRUSH hbr_background = nullptr;
        LPCWSTR lpsz_menu_name = nullptr;
        HICON h_icon_sm = nullptr;
    };

    // 窗口类名 -> 窗口类信息
    std::map<std::wstring, window_class_info> registered_classes_;

    // 窗口句柄 -> 窗口实例
    std::unordered_map<HWND, std::shared_ptr<window>> windows_;

    static std::mutex windows_mutex_;
    static std::mutex class_mutex_;

    // 全局窗口过程
    static LRESULT CALLBACK global_window_proc(HWND hwnd, UINT u_msg,
                                               WPARAM w_param, LPARAM l_param);

    // 注册窗口实例
    void register_window(HWND hwnd, std::shared_ptr<window> win);

    // 注销窗口实例
    void unregister_window(HWND hwnd);

    // 内部注册默认窗口类
    bool register_default_class();

   public:
    // 禁止拷贝和赋值
    window_resource(const window_resource&) = delete;
    window_resource& operator=(const window_resource&) = delete;

    // 单例模式访问
    static window_resource& get_instance();

    // 注册窗口类
    bool register_window_class(
        const std::wstring& class_name, WNDPROC custom_proc = nullptr,
        UINT style = CS_HREDRAW | CS_VREDRAW, HICON h_icon = nullptr,
        HCURSOR h_cursor = LoadCursor(nullptr, IDC_ARROW),
        HBRUSH hbr_background = (HBRUSH)(COLOR_WINDOW + 1),
        LPCWSTR lpsz_menu_name = nullptr, HICON h_icon_sm = nullptr);

    // 注册窗口类（使用默认窗口过程）
    bool register_window_class_default(
        const std::wstring& class_name, UINT style = CS_HREDRAW | CS_VREDRAW,
        HICON h_icon = nullptr,
        HCURSOR h_cursor = LoadCursor(nullptr, IDC_ARROW),
        HBRUSH hbr_background = (HBRUSH)(COLOR_WINDOW + 1),
        LPCWSTR lpsz_menu_name = nullptr, HICON h_icon_sm = nullptr);

    // 创建窗口（使用默认窗口类）
    HWND create_window(const std::wstring& window_name, DWORD dw_style, int x,
                       int y, int width, int height,
                       std::shared_ptr<window> win, HWND hwnd_parent = nullptr,
                       HMENU h_menu = nullptr);

    // 创建窗口（指定窗口类）
    HWND create_window_with_class(const std::wstring& class_name,
                                  const std::wstring& window_name,
                                  DWORD dw_style, int x, int y, int width,
                                  int height, std::shared_ptr<window> win,
                                  HWND hwnd_parent = nullptr,
                                  HMENU h_menu = nullptr);

    // 检查窗口类是否已注册
    bool is_class_registered(const std::wstring& class_name) const;

    // 获取默认窗口类名
    const wchar_t* get_default_class_name() const {
        return default_class_name_;
    }

    // 获取已注册的窗口类列表
    std::vector<std::wstring> get_registered_classes() const;

    // 注销指定的窗口类
    bool unregister_class(const std::wstring& class_name);

    // 销毁窗口
    void destroy_window(HWND hwnd);

    // 查找窗口实例
    std::shared_ptr<window> find_window(HWND hwnd) const;

    // 获取当前实例
    HINSTANCE get_current_instance() const;

    // 清理所有窗口资源
    void cleanup_all_windows();

    // 卸载所有窗口类
    void unregister_all_classes();

    // 清理所有资源
    void cleanup();

    // 获取窗口实例数量
    std::size_t get_window_count() const;

    // 获取特定类的窗口数量
    std::size_t get_window_count_by_class(const std::wstring& class_name) const;

    // 消息循环辅助函数
    int run_message_loop();

    // 处理待处理消息
    bool process_pending_messages();

   private:
    window_resource();
    ~window_resource();
};