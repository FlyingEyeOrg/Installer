// main_window.cpp
#include "win32/main_window.hpp"

#include <windowsx.h>

#include <sstream>

#include "win32/application.hpp"

namespace win32 {

main_window::main_window(const std::wstring& title, int width, int height)
    : window_base(title, width, height) {
    // 设置自定义样式
    set_close_handler([]() {
        // 可以在这里询问是否关闭
        if (MessageBoxW(nullptr, L"确定要关闭窗口吗？", L"确认",
                        MB_YESNO | MB_ICONQUESTION) == IDYES) {
            application::instance().quit(0);
        }
    });

    set_resize_handler([this](int width, int height) {
        // 窗口大小改变时重绘
        InvalidateRect(handle(), nullptr, TRUE);
    });
}

LRESULT main_window::on_create() {
    window_base::on_create();

    // 设置窗口最小大小
    RECT min_size = {0, 0, 400, 300};
    AdjustWindowRectEx(
        &min_size, static_cast<DWORD>(GetWindowLongPtrW(handle(), GWL_STYLE)),
        FALSE, static_cast<DWORD>(GetWindowLongPtrW(handle(), GWL_EXSTYLE)));
    SendMessageW(handle(), WM_GETMINMAXINFO, 0,
                 reinterpret_cast<LPARAM>(&min_size));

    return 0;
}

LRESULT main_window::on_paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(handle(), &ps);

    // 调用基类处理
    window_base::on_paint();

    // 自定义绘制
    draw_content(hdc);

    EndPaint(handle(), &ps);
    return 0;
}

LRESULT main_window::on_size(WPARAM wparam, LPARAM lparam) {
    window_base::on_size(wparam, lparam);

    // 只有在窗口大小改变时才重绘
    if (wparam != SIZE_MINIMIZED) {
        InvalidateRect(handle(), nullptr, TRUE);
    }

    return 0;
}

LRESULT main_window::on_key(UINT msg, WPARAM wparam, LPARAM lparam) {
    window_base::on_key(msg, wparam, lparam);

    if (msg == WM_KEYDOWN) {
        switch (wparam) {
            case VK_F1:
                show_mouse_pos_ = !show_mouse_pos_;
                InvalidateRect(handle(), nullptr, TRUE);
                break;
            case VK_ESCAPE:
                close();
                break;
        }
    }

    return 0;
}

LRESULT main_window::on_mouse_move(WPARAM wparam, LPARAM lparam) {
    window_base::on_mouse_move(wparam, lparam);

    mouse_x_ = GET_X_LPARAM(lparam);
    mouse_y_ = GET_Y_LPARAM(lparam);

    if (show_mouse_pos_) {
        InvalidateRect(handle(), nullptr, TRUE);
    }

    return 0;
}

void main_window::draw_content(HDC hdc) {
    RECT client_rect = get_client_rect();

    // 绘制背景
    HBRUSH background_brush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &client_rect, background_brush);
    DeleteObject(background_brush);

    // 绘制文本
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    RECT text_rect = client_rect;
    text_rect.top = 50;
    DrawTextW(hdc, content_text_.c_str(), -1, &text_rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 绘制鼠标位置
    if (show_mouse_pos_) {
        std::wstringstream ss;
        ss << L"鼠标位置: (" << mouse_x_ << ", " << mouse_y_ << ")";
        std::wstring mouse_text = ss.str();

        RECT mouse_rect = client_rect;
        mouse_rect.top = client_rect.bottom - 50;
        DrawTextW(hdc, mouse_text.c_str(), -1, &mouse_rect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 绘制窗口信息
    std::wstringstream info_ss;
    info_ss << L"窗口大小: " << width_ << " x " << height_;
    std::wstring info_text = info_ss.str();

    RECT info_rect = client_rect;
    info_rect.top = client_rect.bottom - 30;
    SetTextColor(hdc, RGB(100, 100, 100));
    DrawTextW(hdc, info_text.c_str(), -1, &info_rect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

}  // namespace win32