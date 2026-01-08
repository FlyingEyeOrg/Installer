#include <iostream>

#include "windows/application.hpp"
#include "windows/window.hpp"
#include "windows/window_resource.hpp"

// 自定义窗口类
class my_window : public window {
   public:
    my_window(const std::wstring& title, int width, int height)
        : window(title, width, height) {}

   protected:
    LRESULT on_create() override {
        std::cout << "Window created!" << std::endl;
        return 0;
    }

    LRESULT on_close() override {
        std::cout << "Window closing..." << std::endl;
        destroy();
        return 0;
    }

    LRESULT on_destroy() override {
        std::cout << "Window destroyed!" << std::endl;
        return 0;
    }

    LRESULT on_paint() override {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(get_handle(), &ps);

        RECT rect = get_client_rect();
        HBRUSH brush = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        // 绘制文本
        std::wstring text = L"Hello, Windows!";
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, text.c_str(), -1, &rect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(get_handle(), &ps);
        return 0;
    }

    LRESULT on_size(int width, int height) override {
        // 窗口大小改变时重绘
        repaint();
        return 0;
    }

    LRESULT on_mouse_down(int x, int y, WPARAM button, WPARAM flags) override {
        std::cout << "Mouse down at (" << x << ", " << y << ")" << std::endl;
        return 0;
    }

    LRESULT on_key_down(WPARAM key, LPARAM flags) override {
        if (key == VK_ESCAPE) {
            destroy();
        }
        return 0;
    }
};

int main() {
    auto& app = application::instance();

    // 注册自定义窗口类
    window_resource::window_class_info custom_class;
    custom_class.class_name = L"MyCustomWindowClass";
    custom_class.hbr_background = (HBRUSH)(COLOR_BTNFACE + 1);
    window_resource::instance().register_class(custom_class);

    // 添加消息过滤器
    app.add_message_filter([](MSG& msg) -> bool {
        // 可以在这里处理全局消息
        return false;
    });

    // 设置空闲处理器
    app.set_idle_handler([]() -> bool {
        // 在空闲时可以做些事情
        return false;
    });

    // 初始化逻辑
    app.on_init([]() { std::cout << "Application initialized!" << std::endl; });

    // 清理逻辑
    app.on_exit([]() { std::cout << "Application exiting..." << std::endl; });

    // 创建窗口
    auto win1 = std::make_shared<my_window>(L"Window 1", 800, 600);
    win1->show();

    // 使用自定义窗口类创建另一个窗口
    auto win2 = std::make_shared<window>();
    win2->create_with_class(L"MyCustomWindowClass", L"Window 2", 400, 300);
    win2->show();

    // 运行应用
    return app.run();
}