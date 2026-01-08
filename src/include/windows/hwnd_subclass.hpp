#pragma once

#include "windows/window.hpp"

// 依赖余 window，实现窗口子类化
class hwnd_subclass {
   private:
    window& window_;

    void hook_window_proc() {
        ::SetWindowLongW(window_.get_handle(), GWL_WNDPROC, 0);
    }

   public:
    hwnd_subclass(window&);
};