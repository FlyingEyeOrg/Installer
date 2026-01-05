// main.cpp (使用示例)
#include <string>

#include "win32/application.hpp"
#include "win32/main_window.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine,
                    int nCmdShow) {
    using namespace win32;

    // 设置应用程序实例
    application& app = application::instance();
    app.set_instance_handle(hInstance);

    try {
        // 创建主窗口
        main_window main_win(L"Win32 窗口示例", 1024, 768);

        // 自定义窗口样式
        window_base::style win_style;
        win_style.background = CreateSolidBrush(RGB(240, 240, 240));

        // 创建窗口
        if (!main_win.create(hInstance, nullptr, win_style)) {
            MessageBoxW(nullptr, L"创建窗口失败", L"错误",
                        MB_OK | MB_ICONERROR);
            return 1;
        }

        // 显示窗口
        main_win.show(nCmdShow);

        // 添加空闲处理
        app.add_idle_handler([&main_win]() {
            // 可以在这里处理一些后台任务
            static DWORD last_time = GetTickCount();
            DWORD current_time = GetTickCount();

            if (current_time - last_time > 1000) {  // 每秒执行一次
                // 更新窗口标题显示时间
                SYSTEMTIME sys_time;
                GetLocalTime(&sys_time);

                wchar_t time_str[64];
                wsprintfW(time_str, L"Win32 窗口 - %02d:%02d:%02d",
                          sys_time.wHour, sys_time.wMinute, sys_time.wSecond);

                main_win.set_title(time_str);
                last_time = current_time;
                return true;  // 表示有工作要做
            }
            return false;  // 没有工作要做
        });

        // 运行应用程序
        return app.run();

    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "exception", MB_OK | MB_ICONERROR);
        return 1;
    }
}