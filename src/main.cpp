#include <string>

#include "windows/application.hpp"
#include "windows/window.hpp"
#include "windows/window_resource.hpp"

class my_window : public window {
   public:
    // 使用默认构造函数创建默认窗口
    my_window() : window(L"My Window", 800, 600) {}

    // 或者指定参数
    my_window(const std::wstring& title, int width, int height)
        : window(title, width, height) {}

    // 重写消息处理
    virtual LRESULT on_paint() override {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(get_handle(), &ps);

        RECT rect = get_client_rect();
        DrawTextW(hdc, L"Hello, World!", -1, &rect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(get_handle(), &ps);
        return 0;
    }

    virtual LRESULT on_close() override {
        // 自定义关闭行为
        if (MessageBoxW(get_handle(), L"确定要关闭吗？", L"确认",
                        MB_YESNO | MB_ICONQUESTION) == IDYES) {
            return window::on_close();  // 调用基类关闭
        }
        return 0;  // 取消关闭
    }
};

// 示例2：使用自定义窗口类
class custom_class_window : public window {
   public:
    custom_class_window()
        : window(L"MyCustomClass", L"Custom Window", 400, 300) {}
};

int main() {
    // 初始化资源管理器
    window_resource& resource = window_resource::get_instance();

    // 创建窗口（在构造函数中自动创建）
    my_window main_window(L"主窗口", 800, 600);
    main_window.show();

    // 创建窗口（在构造函数中自动创建）
    my_window main_window1(L"主窗口", 800, 600);
    main_window1.show();

    application& app = application::instance();

    app.run();

    return 0;
}