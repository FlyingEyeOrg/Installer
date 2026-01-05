// main_window.hpp (使用示例)
#pragma once
#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <string>

#include "window_base.hpp"


namespace win32 {

// 具体窗口类示例
class main_window : public window_base {
   public:
    main_window(const std::wstring& title = L"Main Window", int width = 800,
                int height = 600);

   protected:
    virtual LRESULT on_create() override;
    virtual LRESULT on_paint() override;
    virtual LRESULT on_size(WPARAM wparam, LPARAM lparam) override;
    virtual LRESULT on_key(UINT msg, WPARAM wparam, LPARAM lparam) override;
    virtual LRESULT on_mouse_move(WPARAM wparam, LPARAM lparam) override;

   private:
    void draw_content(HDC hdc);

    std::wstring content_text_{L"Hello, Win32 Window!"};
    int mouse_x_{0};
    int mouse_y_{0};
    bool show_mouse_pos_{false};
};

}  // namespace win32

#endif  // MAIN_WINDOW_HPP