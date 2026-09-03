/*
  Custom Standalone entry — same pattern as bleach / basic_synth.
  No AEF JackCaptureRouting (that path reopens JACK and can abort).
*/

#include "../JuceLibraryCode/JuceHeader.h"

#include <cstdlib>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
class NativeStandaloneFilterWindow : public juce::StandaloneFilterWindow
{
public:
    NativeStandaloneFilterWindow (const juce::String& title,
                                  juce::Colour backgroundColour,
                                  juce::PropertySet* settingsToUse,
                                  bool takeOwnershipOfSettings,
                                  const juce::String& preferredDefaultDeviceName,
                                  const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions,
                                  const juce::Array<juce::StandalonePluginHolder::PluginInOuts>& constrainToConfiguration,
                                  bool autoOpenMidiDevices = true)
        : StandaloneFilterWindow (title,
                                  backgroundColour,
                                  settingsToUse,
                                  takeOwnershipOfSettings,
                                  preferredDefaultDeviceName,
                                  preferredSetupOptions,
                                  constrainToConfiguration,
                                  autoOpenMidiDevices)
    {
        setUsingNativeTitleBar (true);
        atom::setNativeTitleBarDarkMode (*this);
        setBackgroundColour (juce::LookAndFeel::getDefaultLookAndFeel()
                                 .findColour (juce::ResizableWindow::backgroundColourId));

        for (int i = getNumChildComponents(); --i >= 0;)
            if (auto* button = dynamic_cast<juce::TextButton*> (getChildComponent (i)))
                if (button->getButtonText() == "Options")
                    button->setVisible (false);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeStandaloneFilterWindow)
};

class SsmelEngineDemoStandaloneApp : public juce::JUCEApplication
{
public:
    SsmelEngineDemoStandaloneApp()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = getApplicationName();
        options.filenameSuffix = ".settings";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
       #endif
        appProperties.setStorageParameters (options);
    }

    const juce::String getApplicationName() override { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (createWindow());
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override { quit(); }

private:
    juce::StandaloneFilterWindow* createWindow()
    {
        return new NativeStandaloneFilterWindow (getApplicationName(),
                                                 juce::LookAndFeel::getDefaultLookAndFeel()
                                                     .findColour (juce::ResizableWindow::backgroundColourId),
                                                 appProperties.getUserSettings(),
                                                 false,
                                                 {},
                                                 nullptr,
                                                 {},
                                                 true);
    }

    juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::StandaloneFilterWindow> mainWindow;
};
} // namespace

juce::JUCEApplicationBase* juce_CreateApplication()
{
#if JUCE_LINUX
    if (std::getenv ("GDK_BACKEND") == nullptr)
        ::setenv ("GDK_BACKEND", "x11", 0);
#endif
    return new SsmelEngineDemoStandaloneApp();
}
