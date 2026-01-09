#pragma once
#define UNICODE
#define _UNICODE

#include <objbase.h>
#include <windows.h>

#include <algorithm>  // 添加 algorithm 头文件用于 find
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "hwnd_wrapper_hook.hpp"
#include "hwnd_wrapper_utils.hpp"

namespace windows {

class hwnd_wrapper {
   private:
    static HINSTANCE app_instance_;

    HWND handle_ = nullptr;
    std::vector<hwnd_wrapper_hook_func> hooks_;

    // 改进后的全局窗口过程
    static LRESULT CALLBACK global_wnd_proc(HWND hwnd, UINT u_msg,
                                            WPARAM w_param, LPARAM l_param);

    LRESULT CALLBACK wnd_proc(UINT u_msg, WPARAM w_param, LPARAM l_param,
                              bool& handled);

   public:
    hwnd_wrapper(DWORD class_style = CS_HREDRAW | CS_VREDRAW,
                 DWORD window_exstyle = 0,
                 DWORD window_style = WS_OVERLAPPEDWINDOW,
                 const std::wstring& title = L"windows_app",
                 int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                 int width = CW_USEDEFAULT, int height = CW_USEDEFAULT,
                 HWND parent = nullptr,
                 const std::vector<hwnd_wrapper_hook_func>& hooks = {});

    // 添加 hook
    void add_hook(const hwnd_wrapper_hook_func& hook);

    // 添加 hook（移动语义）
    void add_hook(hwnd_wrapper_hook_func&& hook);

    // 移除 hook
    bool remove_hook(const hwnd_wrapper_hook_func& hook);

    // 移除所有 hooks
    void clear_hooks() { hooks_.clear(); }

    // 获取 hook 数量
    size_t hook_count() const { return hooks_.size(); }

    // 获取窗口句柄
    HWND get_handle() const { return handle_; }

    // 检查窗口是否有效
    bool is_valid() const { return handle_ != nullptr && ::IsWindow(handle_); }
};
}  // namespace windows