#include "installer_controller.hpp"
#include "winelement/controls.hpp"
#include "winelement/layout.hpp"
#include "winelement/platform.hpp"

int main() {
    using namespace winelement;

    platform::Application app;
    platform::Window window(platform::WindowOptions{
        .title = L"Installer",
        .width = 640,
        .height = 480});

    installer::Wizard wizard;
    auto root = wizard.build();
    window.set_content(std::move(root));

    window.show();
    return app.run();
}
