#include <winelement/controls.hpp>
#include <winelement/layout.hpp>
#include <winelement/platform.hpp>

int main() {
    using namespace winelement;

    platform::Application app;
    platform::Window window(platform::WindowOptions{
        .title = L"Installer",
        .width = 640,
        .height = 480});

    auto root = std::make_unique<controls::StackPanel>();
    root->configure_layout([](layout::LayoutElement& item) {
        item.set_size(layout::Length::percent(100.0F), layout::Length::percent(100.0F))
            .set_justify_content(layout::JustifyContent::Center)
            .set_align_items(layout::Align::Center);
    });

    auto& title = root->append_new_child<controls::Text>();
    title.set_text("Installer")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    window.set_content(std::move(root));
    window.show();
    return app.run();
}
