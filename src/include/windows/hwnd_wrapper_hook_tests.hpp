#include <fmt/color.h>
#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <vector>

#include "windows/hwnd_wrapper_hook.hpp"

using namespace windows;

// 函数指针声明
LRESULT test_func_ptr(HWND, UINT, WPARAM, LPARAM, bool&) { return 0; }

void test_constructors() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试构造函数 ===\n");

    // 1. 测试从函数指针构造
    hwnd_wrapper_hook_func hook1(test_func_ptr);
    fmt::print("从函数指针构造: ID = {}\n", hook1.get_id());

    // 2. 测试从lambda构造
    hwnd_wrapper_hook_func hook2(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 123; });
    fmt::print("从lambda构造: ID = {}\n", hook2.get_id());

    // 3. 测试从std::function构造
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM, bool&)> func =
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 456; };
    hwnd_wrapper_hook_func hook3(func);
    fmt::print("从std::function构造: ID = {}\n", hook3.get_id());

    fmt::print(fmt::fg(fmt::color::green), "✓ 构造函数测试完成\n");
}

void test_copy_semantics() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试拷贝语义 ===\n");

    int call_count = 0;
    auto original = hwnd_wrapper_hook_func(
        [&call_count](HWND, UINT, WPARAM, LPARAM, bool&) {
            call_count++;
            return 999;
        });

    size_t original_id = original.get_id();
    fmt::print("原始对象 ID: {}\n", original_id);

    // 1. 测试拷贝构造
    hwnd_wrapper_hook_func copy_constructed = original;
    fmt::print("拷贝构造后 ID: {}\n", copy_constructed.get_id());
    assert(copy_constructed.get_id() == original_id);
    fmt::print(fmt::fg(fmt::color::green), "✓ 拷贝构造保留ID\n");

    // 2. 测试拷贝赋值
    hwnd_wrapper_hook_func copy_assigned(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 0; });
    size_t old_id = copy_assigned.get_id();
    copy_assigned = original;
    fmt::print("拷贝赋值前 ID: {}, 拷贝赋值后 ID: {}\n", old_id,
               copy_assigned.get_id());
    assert(copy_assigned.get_id() == original_id);
    fmt::print(fmt::fg(fmt::color::green), "✓ 拷贝赋值保留ID\n");

    // 3. 测试拷贝后功能相同
    bool handled1 = false, handled2 = false;
    HWND dummy = nullptr;

    LRESULT result1 = original(dummy, WM_USER, 0, 0, handled1);
    LRESULT result2 = copy_constructed(dummy, WM_USER, 0, 0, handled2);

    assert(result1 == 999 && result2 == 999);
    assert(call_count == 2);
    fmt::print(fmt::fg(fmt::color::green), "✓ 拷贝后功能相同\n");
}

void test_move_semantics() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试移动语义 ===\n");

    int moved_from_called = 0;
    auto hook1 = hwnd_wrapper_hook_func(
        [&moved_from_called](HWND, UINT, WPARAM, LPARAM, bool&) {
            moved_from_called++;
            return 111;
        });

    size_t original_id = hook1.get_id();
    fmt::print("移动前 ID: {}\n", original_id);

    // 测试移动构造
    hwnd_wrapper_hook_func hook2 = std::move(hook1);
    fmt::print("移动构造后 ID: {}\n", hook2.get_id());
    assert(hook2.get_id() == original_id);

    // 测试移动后原对象应无效
    assert(!static_cast<bool>(hook1));
    fmt::print(fmt::fg(fmt::color::green), "✓ 移动后原对象无效\n");

    // 测试移动后功能正常
    bool handled = false;
    HWND dummy = nullptr;
    LRESULT result = hook2(dummy, WM_USER, 0, 0, handled);
    assert(result == 111);
    assert(moved_from_called == 1);
    fmt::print(fmt::fg(fmt::color::green), "✓ 移动后功能正常\n");
}

void test_comparison_operators() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试比较运算符 ===\n");

    // 创建不同ID的hook
    auto hook1 = hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 1; });
    auto hook2 = hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 2; });
    auto hook1_copy = hook1;  // 同ID

    fmt::print("hook1 ID: {}, hook2 ID: {}, hook1_copy ID: {}\n",
               hook1.get_id(), hook2.get_id(), hook1_copy.get_id());

    // 测试相等运算符
    assert(hook1 == hook1_copy);
    assert(!(hook1 == hook2));
    fmt::print(fmt::fg(fmt::color::green), "✓ 相等运算符工作正常\n");

    // 测试不等运算符
    assert(hook1 != hook2);
    assert(!(hook1 != hook1_copy));
    fmt::print(fmt::fg(fmt::color::green), "✓ 不等运算符工作正常\n");

// 测试三路比较 (C++20)
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    auto cmp = hook1 <=> hook2;
    assert(cmp != 0);
    assert((hook1 <=> hook1_copy) == 0);
    fmt::print(fmt::fg(fmt::color::green), "✓ 三路比较运算符工作正常\n");
#else
    fmt::print(fmt::fg(fmt::color::yellow), "⚠ 编译器不支持三路比较\n");
#endif
}

void test_function_call() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试函数调用 ===\n");

    int call_count = 0;
    bool last_handled = false;

    auto hook = hwnd_wrapper_hook_func([&](HWND hwnd, UINT msg, WPARAM wparam,
                                           LPARAM lparam, bool& handled) {
        call_count++;
        last_handled = handled;

        fmt::print(
            "调用 {}: hwnd={}, msg=0x{:x}, wparam=0x{:x}, lparam=0x{:x}\n",
            call_count, reinterpret_cast<uintptr_t>(hwnd), msg, wparam, lparam);

        if (msg == WM_CLOSE) {
            handled = true;
            return 1001;
        }
        return 0;
    });

    HWND dummy_hwnd = reinterpret_cast<HWND>(0x1234);
    bool handled = false;

    // 测试普通消息
    LRESULT result1 = hook(dummy_hwnd, WM_PAINT, 0, 0, handled);
    assert(result1 == 0);
    assert(call_count == 1);
    assert(!handled);
    fmt::print(fmt::fg(fmt::color::green), "✓ 普通消息调用正常\n");

    // 测试特殊消息
    LRESULT result2 = hook(dummy_hwnd, WM_CLOSE, 1, 2, handled);
    assert(result2 == 1001);
    assert(call_count == 2);
    assert(handled);
    fmt::print(fmt::fg(fmt::color::green), "✓ 特殊消息调用正常\n");
}

void test_bool_conversion() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试布尔转换 ===\n");

    // 测试有效hook
    auto valid_hook = hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 0; });
    assert(static_cast<bool>(valid_hook));
    if (valid_hook) {
        fmt::print(fmt::fg(fmt::color::green), "✓ 有效hook的布尔转换正确\n");
    }

    // 测试移动后的hook
    auto moved_hook = std::move(valid_hook);
    assert(!static_cast<bool>(valid_hook));
    assert(static_cast<bool>(moved_hook));

    if (!valid_hook) {
        fmt::print(fmt::fg(fmt::color::green),
                   "✓ 移动后原对象的布尔转换正确\n");
    }

    if (moved_hook) {
        fmt::print(fmt::fg(fmt::color::green),
                   "✓ 移动后新对象的布尔转换正确\n");
    }
}

void test_container_usage() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试容器使用 ===\n");

    std::vector<hwnd_wrapper_hook_func> hooks;

    // 测试添加hook
    hooks.push_back(hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 1; }));
    hooks.push_back(hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 2; }));
    hooks.push_back(hooks[0]);  // 拷贝第一个hook

    fmt::print("vector中有 {} 个hook\n", hooks.size());
    assert(hooks.size() == 3);
    fmt::print(fmt::fg(fmt::color::green), "✓ vector添加正常\n");

    // 测试查找
    auto it = std::find(hooks.begin(), hooks.end(), hooks[0]);
    assert(it != hooks.end());
    fmt::print(fmt::fg(fmt::color::green), "✓ 查找功能正常\n");

    // 测试删除
    hooks.erase(
        std::remove_if(hooks.begin(), hooks.end(),
                       [&](const auto& hook) { return hook == hooks[1]; }),
        hooks.end());
    assert(hooks.size() == 2);
    fmt::print(fmt::fg(fmt::color::green), "✓ 删除功能正常\n");

// 测试排序
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    std::sort(hooks.begin(), hooks.end());
    fmt::print(fmt::fg(fmt::color::green), "✓ 排序功能正常\n");
#endif
}

void test_id_generation() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试ID生成 ===\n");

    std::vector<size_t> ids;

    // 创建多个hook，验证ID递增
    for (int i = 0; i < 5; ++i) {
        auto hook = hwnd_wrapper_hook_func(
            [](HWND, UINT, WPARAM, LPARAM, bool&) { return 0; });
        ids.push_back(hook.get_id());
        fmt::print("hook {} ID: {}\n", i + 1, hook.get_id());
    }

    // 验证ID单调递增
    for (size_t i = 1; i < ids.size(); ++i) {
        assert(ids[i] > ids[i - 1]);
    }
    fmt::print(fmt::fg(fmt::color::green), "✓ ID生成单调递增\n");
}

void test_edge_cases() {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== 测试边界情况 ===\n");

    // 1. 测试自赋值
    auto hook = hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 42; });
    size_t original_id = hook.get_id();
    hook = hook;  // 自赋值
    assert(hook.get_id() == original_id);
    fmt::print(fmt::fg(fmt::color::green), "✓ 自赋值安全\n");

    // 2. 测试大ID生成
    for (int i = 0; i < 1000; ++i) {
        hwnd_wrapper_hook_func(
            [](HWND, UINT, WPARAM, LPARAM, bool&) { return 0; });
    }
    auto last_hook = hwnd_wrapper_hook_func(
        [](HWND, UINT, WPARAM, LPARAM, bool&) { return 0; });
    fmt::print("生成1000个hook后，ID: {}\n", last_hook.get_id());
    fmt::print(fmt::fg(fmt::color::green), "✓ 大量ID生成正常\n");
}

int hwnd_wrapper_hook_tests() {
    fmt::print(fmt::fg(fmt::color::green) | fmt::emphasis::bold,
               "开始测试 hwnd_wrapper_hook_func 类型...\n");

    int passed_tests = 0;
    int total_tests = 8;

    try {
        test_constructors();
        passed_tests++;

        test_copy_semantics();
        passed_tests++;

        test_move_semantics();
        passed_tests++;

        test_comparison_operators();
        passed_tests++;

        test_function_call();
        passed_tests++;

        test_bool_conversion();
        passed_tests++;

        test_container_usage();
        passed_tests++;

        test_id_generation();
        passed_tests++;

        test_edge_cases();
        passed_tests++;

        fmt::print(fmt::fg(fmt::color::green) | fmt::emphasis::bold,
                   "\n✅ 所有 {} 个测试全部通过！\n", passed_tests);

    } catch (const std::exception& e) {
        fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold,
                   "\n❌ 测试失败: {}\n", e.what());
        fmt::print("通过 {}/{} 个测试\n", passed_tests, total_tests);
        return 1;
    } catch (...) {
        fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold,
                   "\n❌ 未知错误\n");
        fmt::print("通过 {}/{} 个测试\n", passed_tests, total_tests);
        return 1;
    }

    return 0;
}