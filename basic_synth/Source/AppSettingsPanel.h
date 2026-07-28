#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

class AudioSettingsPanel;

/** App-wide settings shell: left category list, right page content. */
class AppSettingsPanel final : public juce::Component
{
public:
    enum class Page
    {
        AudioSettings = 0
    };

    AppSettingsPanel (juce::AudioDeviceManager& deviceManager,
                      AtomLookAndFeel& lookAndFeel);
    ~AppSettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void selectPage (Page page);
    Page getSelectedPage() const noexcept { return selectedPage; }

    int getPreferredWidth() const noexcept { return 760; }
    int getPreferredHeight();
    int getMinimumWidth() const noexcept;
    int getMinimumHeight() const noexcept;

private:
    class NavItem;

    void rebuildNav();
    void showSelectedPage();
    int measureTallestPageHeight();

    juce::AudioDeviceManager& deviceManager;
    AtomLookAndFeel& atomLookAndFeel;

    juce::Component sidebar;
    juce::Component contentHost;
    atom::Label sidebarTitle;

    std::unique_ptr<AudioSettingsPanel> audioPage;
    std::vector<std::unique_ptr<NavItem>> navItems;

    Page selectedPage = Page::AudioSettings;
    static constexpr int sidebarWidth = 180;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppSettingsPanel)
};
