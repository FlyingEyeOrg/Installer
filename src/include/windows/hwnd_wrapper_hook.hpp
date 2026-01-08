#pragma once
#include <Windows.h>

#include <functional>

namespace windows {
typedef LRESULT(CALLBACK* hwnd_wrapper_hook)(HWND, UINT, WPARAM, LPARAM, bool&);

using hwnd_wrapper_hook_func =
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM, bool&)>;
}  // namespace windows