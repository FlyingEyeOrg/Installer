// #include "windows/application.hpp"
#include "windows/application.hpp"
#include "windows/hwnd_wrapper.hpp"
#include "windows/hwnd_wrapper_hook_tests.hpp"
#include "windows/window.hpp"

using namespace windows;

class main_window : public window {
   public:
    main_window(std::wstring title) : window(title) {}

};

int main() {
    main_window main(L"HelloWorld");
    main.show();
    application::run_app();
    return 0;
}