#include "JackCaptureRouting.h"

#include "AefJuceIncludes.h"

namespace effect_jack
{
namespace
{
juce::String runJackProcess (const juce::String& command)
{
    juce::ChildProcess process;
    if (! process.start (command))
        return {};
    return process.readAllProcessOutput();
}

bool jackClientHasPortKind (const juce::String& clientName, const juce::String& kind)
{
    if (clientName.isEmpty() || kind.isEmpty())
        return false;
    return runJackProcess ("jack_lsp").contains (clientName + ":" + kind);
}

juce::String preferJackCaptureDevice (const juce::StringArray& inputNames,
                                      const juce::String& outputName,
                                      const juce::String& currentInput)
{
    if (inputNames.isEmpty())
        return {};

    auto score = [&] (const juce::String& name) -> int
    {
        const bool hasCapture = jackClientHasPortKind (name, "capture");
        const bool hasMonitor = jackClientHasPortKind (name, "monitor");
        int s = 0;

        if (hasCapture)
            s += 100;
        else if (hasMonitor)
            s -= 150;

        if (outputName.isNotEmpty() && name == outputName && ! hasCapture)
            s -= 200;

        if (name.containsIgnoreCase ("monitor"))
            s -= 100;

        if (hasCapture && name.containsIgnoreCase ("Analog"))
            s += 25;

        if (hasCapture && (name.containsIgnoreCase ("IEC958") || name.containsIgnoreCase ("Digital")))
            s += 10;

        if (outputName.isNotEmpty())
        {
            const auto family = outputName.upToFirstOccurrenceOf (" Analog", false, false)
                                    .upToFirstOccurrenceOf (" Digital", false, false)
                                    .trim();
            if (family.isNotEmpty() && name.containsIgnoreCase (family))
                s += 30;
        }

        if (name == currentInput)
            s += 5;
        return s;
    };

    juce::String best = inputNames[0];
    int bestScore = score (best);
    for (int i = 1; i < inputNames.size(); ++i)
    {
        const int s = score (inputNames[i]);
        if (s > bestScore)
        {
            bestScore = s;
            best = inputNames[i];
        }
    }
    return best;
}

void reconnectJackAppInputsToCapture (const juce::String& appClient,
                                      const juce::String& inputClient)
{
    if (appClient.isEmpty() || inputClient.isEmpty())
        return;
    if (! jackClientHasPortKind (inputClient, "capture"))
        return;

    const auto allPorts = runJackProcess ("jack_lsp");
    juce::StringArray capturePorts, appInputs;

    for (auto& line : juce::StringArray::fromLines (allPorts))
    {
        const auto port = line.trim();
        if (port.startsWith (inputClient + ":capture"))
            capturePorts.addIfNotAlreadyThere (port);
        if (port.startsWith (appClient + ":in_"))
            appInputs.addIfNotAlreadyThere (port);
    }

    capturePorts.sort (true);
    appInputs.sort (true);
    if (capturePorts.isEmpty() || appInputs.isEmpty())
        return;

    const auto connections = runJackProcess ("jack_lsp -c");
    for (const auto& appIn : appInputs)
    {
        bool inSection = false;
        for (auto& raw : juce::StringArray::fromLines (connections))
        {
            const auto line = raw.trimEnd();
            if (! line.startsWith (" ") && ! line.startsWith ("\t"))
            {
                inSection = (line.trim() == appIn);
                continue;
            }
            if (! inSection)
                continue;

            const auto src = line.trim();
            if (src.isNotEmpty())
                runJackProcess ("jack_disconnect \"" + src + "\" \"" + appIn + "\"");
        }
    }

    const int n = juce::jmin (capturePorts.size(), appInputs.size());
    for (int i = 0; i < n; ++i)
        runJackProcess ("jack_connect \"" + capturePorts[i] + "\" \"" + appInputs[i] + "\"");
}

void disconnectJackAppOutputFeedback (const juce::String& appClient)
{
    if (appClient.isEmpty())
        return;

    juce::StringArray appInputs, appOutputs;
    for (auto& line : juce::StringArray::fromLines (runJackProcess ("jack_lsp")))
    {
        const auto port = line.trim();
        if (port.startsWith (appClient + ":in_"))
            appInputs.addIfNotAlreadyThere (port);
        if (port.startsWith (appClient + ":out_"))
            appOutputs.addIfNotAlreadyThere (port);
    }

    if (appInputs.isEmpty())
        return;

    const auto connectionsText = runJackProcess ("jack_lsp -c");
    for (const auto& appIn : appInputs)
    {
        bool inSection = false;
        for (auto& raw : juce::StringArray::fromLines (connectionsText))
        {
            const auto line = raw.trimEnd();
            if (! line.startsWith (" ") && ! line.startsWith ("\t"))
            {
                inSection = (line.trim() == appIn);
                continue;
            }
            if (! inSection)
                continue;

            const auto src = line.trim();
            const bool fromSelfOut = appOutputs.contains (src);
            const bool fromMonitor = src.contains (":monitor");
            if (fromSelfOut || fromMonitor)
                runJackProcess ("jack_disconnect \"" + src + "\" \"" + appIn + "\"");
        }
    }
}
}  // namespace

void ensureJackUsesCaptureInput (juce::AudioDeviceManager& deviceManager,
                                 const juce::String& appClientName)
{
    if (deviceManager.getCurrentAudioDeviceType() != "JACK")
        return;

    auto* type = deviceManager.getCurrentDeviceTypeObject();
    if (type == nullptr)
        return;

    type->scanForDevices();
    const auto inputNames = type->getDeviceNames (true);
    if (inputNames.isEmpty())
        return;

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);

    const auto best = preferJackCaptureDevice (inputNames, setup.outputDeviceName, setup.inputDeviceName);
    if (best.isNotEmpty() && best != setup.inputDeviceName)
    {
        setup.inputDeviceName = best;
        setup.useDefaultInputChannels = true;
        deviceManager.setAudioDeviceSetup (setup, true);
        deviceManager.getAudioDeviceSetup (setup);
    }

    disconnectJackAppOutputFeedback (appClientName);
    reconnectJackAppInputsToCapture (appClientName, setup.inputDeviceName);
}

void prepareJackInputForTuning (juce::AudioDeviceManager& deviceManager,
                                const juce::String& appClientName)
{
    ensureJackUsesCaptureInput (deviceManager, appClientName);
}
}  // namespace effect_jack
