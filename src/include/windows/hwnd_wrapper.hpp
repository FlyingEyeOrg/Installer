#pragma once
#define UNICODE
#define _UNICODE

#include <objbase.h>
#include <windows.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "hwnd_wrapper_hook.hpp"

namespace windows {

class hwnd_wrapper {
   private:
    // 窗口类管理器
    class window_class_manager {
       private:
        static std::mutex mutex_;
        static std::unordered_map<DWORD, ATOM>
            class_map_;  // class_style -> ATOM
        static ATOM default_class_atom_;

       public:
        // 生成基于 class_style 的类名
        static std::wstring generate_class_name(DWORD class_style);

        // 根据 class_style 获取窗口类原子
        static ATOM get_class_atom(DWORD class_style);

        // 获取默认窗口类原子 (CS_HREDRAW | CS_VREDRAW)
        static ATOM get_default_class_atom();

        // 清理所有注册的窗口类
        static void cleanup();

        // 获取已注册的类数量
        static size_t get_registered_count();

        // 获取指定样式的类名
        static std::wstring get_class_name(DWORD class_style);
    };

    static HINSTANCE app_instance_;
    HWND handle_ = nullptr;
    std::vector<hwnd_wrapper_hook_func> hooks_;
    ATOM class_atom_ = 0;  // 保存窗口类原子

    // 全局窗口过程
    static LRESULT CALLBACK global_wnd_proc(HWND hwnd, UINT u_msg,
                                            WPARAM w_param, LPARAM l_param);

    LRESULT CALLBACK wnd_proc(UINT u_msg, WPARAM w_param, LPARAM l_param,
                              bool& handled);

   public:
    hwnd_wrapper(DWORD class_style = CS_HREDRAW | CS_VREDRAW,
                 DWORD window_exstyle = 0,
                 DWORD window_style = WS_OVERLAPPEDWINDOW,
                 const std::wstring& name = L"windows_app",
                 int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                 int width = CW_USEDEFAULT, int height = CW_USEDEFAULT,
                 HWND parent = nullptr,
                 const std::vector<hwnd_wrapper_hook_func>& hooks = {});

    virtual ~hwnd_wrapper();

    // 禁止拷贝
    hwnd_wrapper(const hwnd_wrapper&) = delete;
    hwnd_wrapper& operator=(const hwnd_wrapper&) = delete;

    // 允许移动
    hwnd_wrapper(hwnd_wrapper&& other) noexcept;
    hwnd_wrapper& operator=(hwnd_wrapper&& other) noexcept;

    // 静态方法
    static void cleanup();
    static size_t get_registered_class_count();
    static std::wstring get_class_name(DWORD class_style);

    // hook 管理
    void add_hook(const hwnd_wrapper_hook_func& hook);
    void add_hook(hwnd_wrapper_hook_func&& hook);
    bool remove_hook(const hwnd_wrapper_hook_func& hook);
    void clear_hooks();
    size_t hook_count() const;

    // 窗口操作
    HWND get_handle() const;
    bool is_valid() const;

    // 获取窗口类原子
    ATOM get_class_atom() const;

    // 获取窗口类样式
    DWORD get_class_style() const;
};
}  // namespace windows