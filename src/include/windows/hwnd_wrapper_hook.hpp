// hwnd_wrapper_hook.hpp
#pragma once
#include <Windows.h>

#include <atomic>
#include <compare>  // C++20 三路比较
#include <functional>
#include <utility>

namespace windows {
using hwnd_wrapper_hook =
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM, bool&)>;

class hwnd_wrapper_hook_func {
   private:
    hwnd_wrapper_hook hook_;
    size_t hook_id_;

    // 线程安全的ID生成
    static size_t generate_hook_id() {
        static std::atomic<size_t> hook_id{0};
        return hook_id.fetch_add(1, std::memory_order_relaxed);
    }

    // 默认构造函数
    hwnd_wrapper_hook_func() = delete;

   public:
    // 显式构造函数
    explicit hwnd_wrapper_hook_func(hwnd_wrapper_hook hook)
        : hook_(std::move(hook)), hook_id_(generate_hook_id()) {}

    // 添加拷贝构造函数
    hwnd_wrapper_hook_func(const hwnd_wrapper_hook_func& other)
        : hook_(other.hook_), hook_id_(other.hook_id_) {}

    // 添加拷贝赋值运算符
    hwnd_wrapper_hook_func& operator=(const hwnd_wrapper_hook_func& other) {
        if (this != &other) {
            hook_ = other.hook_;
            hook_id_ = other.hook_id_;
        }
        return *this;
    }

    // 允许移动
    hwnd_wrapper_hook_func(hwnd_wrapper_hook_func&&) = default;
    hwnd_wrapper_hook_func& operator=(hwnd_wrapper_hook_func&&) = default;

    // 比较运算符
    bool operator==(const hwnd_wrapper_hook_func& other) const {
        return hook_id_ == other.hook_id_;
    }

    bool operator!=(const hwnd_wrapper_hook_func& other) const {
        return !(*this == other);
    }

    // C++20 三路比较（可选但推荐）
    auto operator<=>(const hwnd_wrapper_hook_func& other) const {
        return hook_id_ <=> other.hook_id_;
    }

    // 获取ID（用于调试或日志）
    size_t get_id() const { return hook_id_; }

    // 调用钩子函数
    LRESULT operator()(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                       bool& handled) const {
        return hook_(hwnd, msg, wparam, lparam, handled);
    }

    // 检查是否包含有效的hook函数
    explicit operator bool() const { return static_cast<bool>(hook_); }
};
}  // namespace windows