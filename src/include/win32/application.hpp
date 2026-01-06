// application.hpp
#pragma once

#include <windows.h>

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>


// 前向声明
class window;

class application {
    friend class window;  // 允许 window 访问私有成员

   private:
    // 私有构造函数，单例模式
    application() = default;

    // 禁止拷贝
    application(const application&) = delete;
    application& operator=(const application&) = delete;

    std::vector<std::shared_ptr<window>> m_windows;
    bool m_running = false;
    int m_exit_code = 0;

    // 空闲回调函数类型
    using idle_callback = std::function<void()>;
    std::vector<idle_callback> m_idle_callbacks;

    // 消息映射：窗口句柄到窗口指针
    static std::unordered_map<HWND, window*> s_window_map;
    static std::mutex s_window_map_mutex;

    // 窗口类注册标志
    static bool s_window_class_registered;

    // 派遣消息到指定窗口
    static LRESULT dispatch_to_window(HWND hwnd, UINT msg, WPARAM w_param,
                                      LPARAM l_param);

    // 窗口管理
    static void register_window(HWND hwnd, window* win);
    static void unregister_window(HWND hwnd);
    static window* find_window(HWND hwnd);

   public:
    // 获取单例实例
    static application& get_instance();

    // 运行应用
    int run();

    // 添加窗口（内部使用）
    void add_window(std::shared_ptr<window> win);

    // 移除窗口
    void remove_window(HWND hwnd);

    // 获取所有窗口
    const std::vector<std::shared_ptr<window>>& get_windows() const {
        return m_windows;
    }

    // 获取窗口数量
    size_t get_window_count() const { return m_windows.size(); }

    // 添加空闲回调
    void add_idle_callback(idle_callback callback) {
        m_idle_callbacks.push_back(callback);
    }

    // 退出应用
    void quit(int exit_code = 0);

    // 获取退出码
    int get_exit_code() const { return m_exit_code; }

    // 检查是否在运行
    bool is_running() const { return m_running; }

    // 全局窗口过程
    static LRESULT CALLBACK global_window_proc(HWND hwnd, UINT msg,
                                               WPARAM w_param, LPARAM l_param);

    // 静态辅助方法
    static void register_window_class();
    static const wchar_t* get_window_class_name();

    // 显示所有窗口
    void show_all_windows(int n_cmd_show = SW_SHOW);
};