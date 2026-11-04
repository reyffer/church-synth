// Custom standalone application (Standalone target only).
//
// Identical in behaviour to JUCE's default standalone wrapper, except that it
// opens every connected MIDI input automatically, so a controller works
// straight away without visiting Options > Audio/MIDI Settings.

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include <memory>

namespace
{
class ChurchSynthStandaloneApp final : public juce::JUCEApplication
{
public:
    ChurchSynthStandaloneApp()
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName = "Church Synth";
        opts.filenameSuffix = ".settings";
        opts.osxLibrarySubFolder = "Application Support";
        opts.folderName = "Church Synth";
        settings.setStorageParameters (opts);
    }

    const juce::String getApplicationName() override { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    void initialise (const juce::String&) override
    {
        window = std::make_unique<juce::StandaloneFilterWindow> (
            getApplicationName(),
            juce::Colour (0xff1e1f24),
            settings.getUserSettings(),
            false,          // settings owned by the ApplicationProperties
            juce::String(), // default audio device
            nullptr,        // default device setup
            juce::Array<juce::StandalonePluginHolder::PluginInOuts>(),
            true);          // auto-open all MIDI inputs
        window->setVisible (true);
    }

    void shutdown() override
    {
        window = nullptr;
        settings.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (window != nullptr)
            window->pluginHolder->savePluginState();
        quit();
    }

private:
    juce::ApplicationProperties settings;
    std::unique_ptr<juce::StandaloneFilterWindow> window;
};
} // namespace

juce::JUCEApplicationBase* juce_CreateApplication();
juce::JUCEApplicationBase* juce_CreateApplication() { return new ChurchSynthStandaloneApp(); }
