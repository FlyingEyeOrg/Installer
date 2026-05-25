#include "installer_controller.hpp"
#include <Windows.h>

namespace installer {
using namespace winelement;
using UIElement = winelement::elements::UIElement;

static constexpr float kPageWidth = 580.0F;
static constexpr float kPagePadding = 32.0F;
static constexpr float kFooterHeight = 56.0F;

std::unique_ptr<UIElement> Wizard::make_spacer() {
    auto el = std::make_unique<UIElement>();
    el->set_height(16.0F);
    return el;
}

// ---- footer ----

std::unique_ptr<UIElement> Wizard::build_footer() {
    auto footer = std::make_unique<controls::StackPanel>();
    footer->set_orientation(controls::Orientation::Horizontal);
    footer->set_height(kFooterHeight);
    footer->set_padding(16.0F);
    footer->set_gap(12.0F);
    footer->configure_layout([](layout::LayoutElement& item) {
        item.set_justify_content(layout::JustifyContent::FlexEnd);
        item.set_align_items(layout::Align::Center);
    });

    auto& cancel = footer->append_new_child<controls::Button>();
    cancel.set_text("Cancel")
        .set_type(controls::ButtonType::Default)
        .set_size(controls::ButtonSize::Default);
    cancel.clicked() +=
        [this](const controls::ButtonClickEvent&) { on_cancel(); };
    cancel_btn_ = &cancel;

    auto& back = footer->append_new_child<controls::Button>();
    back.set_text("Back")
        .set_type(controls::ButtonType::Default)
        .set_size(controls::ButtonSize::Default);
    back.clicked() += [this](const controls::ButtonClickEvent&) { on_back(); };
    back_btn_ = &back;

    auto& next = footer->append_new_child<controls::Button>();
    next.set_text("Next")
        .set_type(controls::ButtonType::Primary)
        .set_size(controls::ButtonSize::Default);
    next.clicked() += [this](const controls::ButtonClickEvent&) { on_next(); };
    next_btn_ = &next;

    return footer;
}

// ---- page: welcome ----

std::unique_ptr<UIElement> Wizard::build_welcome() {
    auto page = std::make_unique<controls::StackPanel>();
    page->set_gap(12.0F);
    page->set_padding(kPagePadding);
    page->configure_layout([](layout::LayoutElement& item) {
        item.set_width(layout::Length::points(kPageWidth))
            .set_align_items(layout::Align::Center);
    });

    page->append_child(make_spacer());

    auto& title = page->append_new_child<controls::Text>();
    title.set_text("Welcome to Installer")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    page->append_child(make_spacer());

    auto& desc = page->append_new_child<controls::Text>();
    desc.set_text(
            "This wizard will guide you through the installation.\n\n"
            "Please click Next to continue.")
        .set_type(controls::TextType::Primary);

    return page;
}

// ---- page: license ----

std::unique_ptr<UIElement> Wizard::build_license() {
    auto page = std::make_unique<controls::StackPanel>();
    page->set_gap(12.0F);
    page->set_padding(kPagePadding);
    page->configure_layout([](layout::LayoutElement& item) {
        item.set_width(layout::Length::points(kPageWidth))
            .set_align_items(layout::Align::Stretch);
    });

    auto& title = page->append_new_child<controls::Text>();
    title.set_text("License Agreement")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    auto& subtitle = page->append_new_child<controls::Text>();
    subtitle.set_text("Please read the following license agreement carefully.")
        .set_type(controls::TextType::Primary);

    auto& border = page->append_new_child<controls::Border>();
    border.set_preset(controls::BorderPreset::Plain);
    border.configure_layout(
        [](layout::LayoutElement& item) { item.set_flex(1.0F); });

    auto& license_text = border.append_new_child<controls::Text>();
    license_text
        .set_text(
            "SOFTWARE LICENSE AGREEMENT\n\n"
            "Copyright (c) 2024\n\n"
            "Permission is hereby granted, free of charge, to any person "
            "obtaining a copy of this software and associated documentation "
            "files (the \"Software\"), to deal in the Software without "
            "restriction, including without limitation the rights to use, "
            "copy, modify, merge, publish, distribute, sublicense, and/or "
            "sell copies of the Software, and to permit persons to whom the "
            "Software is furnished to do so, subject to the following "
            "conditions:\n\n"
            "The above copyright notice and this permission notice shall be "
            "included in all copies or substantial portions of the "
            "Software.\n\n"
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY "
            "KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE "
            "WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR "
            "PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR "
            "COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, "
            "ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE "
            "USE OR OTHER DEALINGS IN THE SOFTWARE.")
        .set_type(controls::TextType::Primary);

    auto& checkbox_row = page->append_new_child<controls::StackPanel>();
    checkbox_row.set_orientation(controls::Orientation::Horizontal);
    checkbox_row.set_gap(8.0F);
    checkbox_row.configure_layout([](layout::LayoutElement& item) {
        item.set_align_items(layout::Align::Center);
    });

    auto& checkbox = checkbox_row.append_new_child<controls::Button>();
    checkbox
        .set_text(license_accepted ? "[x] I accept the agreement"
                                   : "[  ] I accept the agreement")
        .set_type(controls::ButtonType::Text);
    checkbox.clicked() += [this, &checkbox](const controls::ButtonClickEvent&) {
        license_accepted = !license_accepted;
        checkbox.set_text(license_accepted ? "[x] I accept the agreement"
                                           : "[  ] I accept the agreement");
        update_nav_buttons();
    };

    return page;
}

// ---- page: directory ----

std::unique_ptr<UIElement> Wizard::build_directory() {
    auto page = std::make_unique<controls::StackPanel>();
    page->set_gap(12.0F);
    page->set_padding(kPagePadding);
    page->configure_layout([](layout::LayoutElement& item) {
        item.set_width(layout::Length::points(kPageWidth))
            .set_align_items(layout::Align::Stretch);
    });

    auto& title = page->append_new_child<controls::Text>();
    title.set_text("Choose Install Location")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    auto& desc = page->append_new_child<controls::Text>();
    desc.set_text("Select the destination folder for installation.")
        .set_type(controls::TextType::Primary);

    page->append_child(make_spacer());

    auto& path_row = page->append_new_child<controls::StackPanel>();
    path_row.set_orientation(controls::Orientation::Horizontal);
    path_row.set_gap(8.0F);
    path_row.configure_layout([](layout::LayoutElement& item) {
        item.set_align_items(layout::Align::Center);
    });

    auto& path_label = path_row.append_new_child<controls::Text>();
    path_label.set_text("Path:").set_type(controls::TextType::Primary);

    if (install_path.empty()) {
        install_path = "C:\\Program Files\\Installer";
    }

    auto& path_input = path_row.append_new_child<controls::Input>();
    path_input.set_text(install_path);
    path_input.configure_layout(
        [](layout::LayoutElement& item) { item.set_flex(1.0F); });
    path_input.input_changed() +=
        [this](std::string_view text) { install_path = text; };

    auto& browse = path_row.append_new_child<controls::Button>();
    browse.set_text("Browse...")
        .set_type(controls::ButtonType::Default)
        .set_size(controls::ButtonSize::Default);

    auto& info = page->append_new_child<controls::Text>();
    info.set_text("At least 200 MB of free disk space is required.")
        .set_type(controls::TextType::Primary);

    return page;
}

// ---- page: options ----

std::unique_ptr<UIElement> Wizard::build_options() {
    auto page = std::make_unique<controls::StackPanel>();
    page->set_gap(12.0F);
    page->set_padding(kPagePadding);
    page->configure_layout([](layout::LayoutElement& item) {
        item.set_width(layout::Length::points(kPageWidth))
            .set_align_items(layout::Align::Stretch);
    });

    auto& title = page->append_new_child<controls::Text>();
    title.set_text("Installation Options")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    page->append_child(make_spacer());

    auto& shortcut_btn = page->append_new_child<controls::Button>();
    shortcut_btn
        .set_text(create_shortcut ? "[x] Create a desktop shortcut"
                                  : "[  ] Create a desktop shortcut")
        .set_type(controls::ButtonType::Text);
    shortcut_btn.clicked() +=
        [this, &shortcut_btn](const controls::ButtonClickEvent&) {
            create_shortcut = !create_shortcut;
            shortcut_btn.set_text(create_shortcut
                                      ? "[x] Create a desktop shortcut"
                                      : "[  ] Create a desktop shortcut");
        };

    auto& startup_btn = page->append_new_child<controls::Button>();
    startup_btn
        .set_text(auto_start ? "[x] Add to startup" : "[  ] Add to startup")
        .set_type(controls::ButtonType::Text);
    startup_btn.clicked() +=
        [this, &startup_btn](const controls::ButtonClickEvent&) {
            auto_start = !auto_start;
            startup_btn.set_text(auto_start ? "[x] Add to startup"
                                            : "[  ] Add to startup");
        };

    page->append_child(make_spacer());

    auto& summary = page->append_new_child<controls::Text>();
    summary
        .set_text("Install path: " + install_path +
                  "\nReady to begin installation.")
        .set_type(controls::TextType::Primary);

    return page;
}

// ---- page: progress ----

std::unique_ptr<UIElement> Wizard::build_progress() {
    auto page = std::make_unique<controls::StackPanel>();
    page->set_gap(12.0F);
    page->set_padding(kPagePadding);
    page->configure_layout([](layout::LayoutElement& item) {
        item.set_width(layout::Length::points(kPageWidth))
            .set_align_items(layout::Align::Stretch);
    });

    auto& title = page->append_new_child<controls::Text>();
    title.set_text("Installing...")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    page->append_child(make_spacer());

    auto& status = page->append_new_child<controls::Text>();
    status.set_text("Preparing installation...")
        .set_type(controls::TextType::Primary);

    // Progress track
    auto& track = page->append_new_child<controls::Border>();
    track.set_section_role(controls::BorderSectionRole::Frame);
    track.set_height(8.0F);

    auto& bar = track.append_new_child<UIElement>();
    bar.set_background(rendering::Color::rgba(64, 158, 255));
    bar.set_corner_radius(4.0F);
    bar.set_width(20.0F);
    bar.set_height(layout::Length::percent(100.0F));

    auto& detail = page->append_new_child<controls::Text>();
    detail.set_text("Extracting files...")
        .set_type(controls::TextType::Primary);

    return page;
}

// ---- page: complete ----

std::unique_ptr<UIElement> Wizard::build_complete() {
    auto page = std::make_unique<controls::StackPanel>();
    page->set_gap(12.0F);
    page->set_padding(kPagePadding);
    page->configure_layout([](layout::LayoutElement& item) {
        item.set_width(layout::Length::points(kPageWidth))
            .set_align_items(layout::Align::Center);
    });

    page->append_child(make_spacer());

    auto& icon = page->append_new_child<controls::Text>();
    icon.set_text("OK")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Success);

    page->append_child(make_spacer());

    auto& title = page->append_new_child<controls::Text>();
    title.set_text("Installation Complete")
        .set_size(controls::TextSize::Large)
        .set_type(controls::TextType::Primary);

    auto& desc = page->append_new_child<controls::Text>();
    desc.set_text(
            "The application has been successfully installed.\n"
            "Click Finish to exit the wizard.")
        .set_type(controls::TextType::Primary);

    return page;
}

// ---- navigation ----

void Wizard::switch_page(int step) {
    current_step_ = step;
    page_area_->clear_children();

    std::unique_ptr<UIElement> page;
    switch (step) {
        case Step::Welcome:
            page = build_welcome();
            break;
        case Step::License:
            page = build_license();
            break;
        case Step::Directory:
            page = build_directory();
            break;
        case Step::Options:
            page = build_options();
            break;
        case Step::Progress:
            page = build_progress();
            break;
        case Step::Complete:
            page = build_complete();
            break;
        default:
            return;
    }
    page_area_->append_child(std::move(page));
    update_nav_buttons();
}

void Wizard::update_nav_buttons() {
    back_btn_->set_visible(current_step_ > Step::Welcome &&
                           current_step_ < Step::Progress);
    cancel_btn_->set_visible(current_step_ < Step::Progress);

    switch (current_step_) {
        case Step::Welcome:
            next_btn_->set_text("Next").set_visible(true);
            break;
        case Step::License:
            next_btn_->set_text("Next").set_visible(true);
            next_btn_->set_disabled(!license_accepted);
            break;
        case Step::Directory:
            next_btn_->set_text("Next").set_visible(true);
            break;
        case Step::Options:
            next_btn_->set_text("Install")
                .set_type(controls::ButtonType::Success)
                .set_visible(true);
            break;
        case Step::Progress:
            next_btn_->set_visible(false);
            back_btn_->set_visible(false);
            cancel_btn_->set_text("Cancel").set_disabled(true);
            break;
        case Step::Complete:
            cancel_btn_->set_visible(false);
            back_btn_->set_visible(false);
            next_btn_->set_text("Finish")
                .set_type(controls::ButtonType::Primary)
                .set_visible(true);
            break;
    }
}

void Wizard::on_next() {
    switch (current_step_) {
        case Step::Welcome:
            switch_page(Step::License);
            break;
        case Step::License:
            switch_page(Step::Directory);
            break;
        case Step::Directory:
            switch_page(Step::Options);
            break;
        case Step::Options:
            start_installation();
            break;
        case Step::Complete:
            platform::Application::current_dispatcher().request_quit(0);
            break;
        default:
            break;
    }
}

void Wizard::on_back() {
    switch (current_step_) {
        case Step::License:
            switch_page(Step::Welcome);
            break;
        case Step::Directory:
            switch_page(Step::License);
            break;
        case Step::Options:
            switch_page(Step::Directory);
            break;
        default:
            break;
    }
}

void Wizard::on_cancel() {
    platform::Application::current_dispatcher().request_quit(0);
}

void Wizard::start_installation() {
    switch_page(Step::Progress);

    // Placeholder: in production, run install on worker thread
    platform::Application::current_dispatcher().post(
        [this]() { switch_page(Step::Complete); });
}

// ---- build root UI ----

std::unique_ptr<UIElement> Wizard::build() {
    auto root = std::make_unique<UIElement>();
    root->configure_layout([](layout::LayoutElement& item) {
        item.set_flex_direction(layout::FlexDirection::Column);
    });

    auto& header = root->append_new_child<UIElement>();
    header.set_background(rendering::Color::rgba(0x40, 0x9c, 0x00));
    header.configure_layout([](layout::LayoutElement& item) {
        item.set_height(layout::Length::points(80.0F));
    });

    auto& body = root->append_new_child<UIElement>();
    body.set_background(rendering::Color::rgba(255, 255, 255));
    body.configure_layout(
        [](layout::LayoutElement& item) 
        { 
            item.set_flex(1.0F); 
            item.set_align_items(layout::Align::FlexStart);
            item.set_flex_direction(layout::FlexDirection::Column);
            item.set_justify_content(layout::JustifyContent::FlexStart);
        });

    auto& button = body.append_new_child<controls::Button>();
    button.set_text("Start Installation")
        .set_type(controls::ButtonType::Primary)
        .set_size(controls::ButtonSize::Large);

    button.configure_layout([](layout::LayoutElement& item) {
        item.set_align_self(layout::Align::Center);
        item.set_height(layout::Length::points(20.0F));
    });
    button.clicked() += [this](const controls::ButtonClickEvent& event) 
    { 
        MessageBoxA(nullptr, "Installation started!", "Installer", MB_OK | MB_ICONINFORMATION);
    };

    auto& tail = root->append_new_child<UIElement>();
    tail.set_background(rendering::Color::rgba(0x25, 0x25, 0x25));
    tail.configure_layout([](layout::LayoutElement& item) {
        item.set_height(layout::Length::points(20.0F));
    });

    return root;
}

Wizard::Wizard() = default;

}  // namespace installer
