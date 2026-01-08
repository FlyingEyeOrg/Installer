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
        }

        // 特殊处理 WM_DESTROY - 清理资源
        if (u_msg == WM_DESTROY && wrapper) {
            // 清除 USERDATA，避免野指针
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            wrapper->handle_ = nullptr;
        }

        if (!handled) {
            result = ::DefWindowProcW(hwnd, u_msg, w_param, l_param);
        }

        return result;
    }

    LRESULT CALLBACK wnd_proc(UINT u_msg, WPARAM w_param, LPARAM l_param,
                              bool& handled) {
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

   public:
    hwnd_wrapper(DWORD class_style = CS_HREDRAW | CS_VREDRAW,
                 DWORD window_exstyle = 0,
                 DWORD window_style = WS_OVERLAPPEDWINDOW,
                 const std::wstring& title = L"windows_app",
                 int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                 int width = CW_USEDEFAULT, int height = CW_USEDEFAULT,
                 HWND parent = nullptr,
                 const std::vector<hwnd_wrapper_hook_func>& hooks = {})
        : hooks_(hooks) {
        app_instance_ = ::GetModuleHandleW(nullptr);
        auto class_name =
            hwnd_wrapper_utils::generate_class_name(L"windows_app");

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = class_style;
        wc.lpfnWndProc = global_wnd_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(LONG_PTR);

        wc.hInstance = app_instance_;
        wc.hIcon = nullptr;
        wc.hCursor = nullptr;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = class_name.c_str();
        wc.hIconSm = nullptr;

        if (!::RegisterClassExW(&wc)) {
            // 注册失败
            return;
        }

        handle_ = ::CreateWindowExW(
            window_exstyle, class_name.c_str(), title.c_str(), window_style, x,
            y, width, height, parent, nullptr, app_instance_, this);
    }

    // 添加 hook
    void add_hook(const hwnd_wrapper_hook_func& hook) {
        hooks_.push_back(hook);
    }

    // 添加 hook（移动语义）
    void add_hook(hwnd_wrapper_hook_func&& hook) {
        hooks_.push_back(std::move(hook));
    }

    // 移除 hook
    bool remove_hook(const hwnd_wrapper_hook_func& hook) {
        auto it = std::find(hooks_.begin(), hooks_.end(), hook);
        if (it != hooks_.end()) {
            hooks_.erase(it);
            return true;
        }
        return false;
    }

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