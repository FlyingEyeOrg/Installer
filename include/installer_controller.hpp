#pragma once

#include <memory>
#include <string>

#include "winelement/controls.hpp"
#include "winelement/elements.hpp"
#include "winelement/layout.hpp"
#include "winelement/platform.hpp"

namespace installer {

class Wizard {
public:
    Wizard();

    // Build the full wizard UI root element
    std::unique_ptr<winelement::elements::UIElement> build();

    // Shared state
    std::string install_path;
    bool license_accepted = false;
    bool create_shortcut = true;
    bool auto_start = false;

private:
    using UIElement = winelement::elements::UIElement;
    using StackPanel = winelement::controls::StackPanel;
    using Button = winelement::controls::Button;

    enum Step : int { Welcome, License, Directory, Options, Progress, Complete, Count };

    StackPanel* page_area_ = nullptr;
    Button* back_btn_ = nullptr;
    Button* next_btn_ = nullptr;
    Button* cancel_btn_ = nullptr;

    int current_step_ = Step::Welcome;

    void switch_page(int step);
    void update_nav_buttons();

    // Page builders
    std::unique_ptr<UIElement> build_welcome();
    std::unique_ptr<UIElement> build_license();
    std::unique_ptr<UIElement> build_directory();
    std::unique_ptr<UIElement> build_options();
    std::unique_ptr<UIElement> build_progress();
    std::unique_ptr<UIElement> build_complete();

    // Shared helpers
    std::unique_ptr<UIElement> build_footer();
    std::unique_ptr<UIElement> make_spacer();

    // Navigation
    void on_next();
    void on_back();
    void on_cancel();
    void start_installation();
};

}  // namespace installer
