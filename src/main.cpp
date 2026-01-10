// #include "windows/application.hpp"
#include "windows/application.hpp"
#include "windows/hwnd_wrapper.hpp"
#include "windows/hwnd_wrapper_hook_tests.hpp"
#include "windows/window.hpp"

using namespace windows;

int main() {
    window main_window(L"title1");
    main_window.show();

    window main_window1(L"title2");
    main_window1.show();

    application::run_app();
    return 0;
}