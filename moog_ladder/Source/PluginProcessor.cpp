#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"
#include "MoogLadderEditor.h"

MoogLadderAudioProcessor::MoogLadderAudioProcessor()
    : paramCutoff(parameters, "Cutoff", "Hz", 20.0f, 20000.0f, 1000.0f),
      paramResonance(parameters, "Resonance", "", 0.0f, 1.0f, 0.1f),
      paramDrive(parameters, "Drive", "", 0.1f, 10.0f, 1.0f),
      paramMode(parameters, "Mode", juce::StringArray{"LP4", "LP2", "HP4", "HP2", "BP4", "BP2", "Notch"}, 0),
      paramSaturator(parameters, "Saturator", juce::StringArray{"Algebraic", "Tanh"}, 0),
      paramQuality(parameters, "Quality", juce::StringArray{"Static", "Relinearized", "Outer2"}, 1),
      paramAdaa(parameters, "ADAA", juce::StringArray{"Off", "On"}, 1)
{
  aef::setParameterDefault(parameters.valueTreeState, paramInputGain.paramID, -12.0f);
  aef::setParameterDefault(parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> MoogLadderAudioProcessor::createEffectEngine()
{
  return std::make_unique<MiddleProcessorEffectEngine>(
      "com.kbuss.nudsp.ssmel.moog_ladder", "Moog Ladder", "moog_ladder");
}

void MoogLadderAudioProcessor::updateCustomEffectParameters()
{
  const auto moogId = getKbussEngine().middleProcessorId();
  if (moogId == kbuss::kInvalidObjectId) return;

  auto& engine = getKbussEngine();

  engine.setParamDomain(moogId, "cutoff", readParameterValue(paramCutoff.paramID, paramCutoff.defaultValue));
  engine.setParamDomain(moogId, "resonance", readParameterValue(paramResonance.paramID, paramResonance.defaultValue));
  engine.setParamDomain(moogId, "drive", readParameterValue(paramDrive.paramID, paramDrive.defaultValue));
  engine.setParamNormalized(moogId, "mode", readParameterValue(paramMode.paramID, (float)paramMode.defaultChoice));
  engine.setParamNormalized(moogId, "saturator",
                            readParameterValue(paramSaturator.paramID, (float)paramSaturator.defaultChoice));
  engine.setParamNormalized(moogId, "quality",
                            readParameterValue(paramQuality.paramID, (float)paramQuality.defaultChoice));
  engine.setParamNormalized(moogId, "adaa_enabled",
                            readParameterValue(paramAdaa.paramID, (float)paramAdaa.defaultChoice));
}

void MoogLadderAudioProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
  aef::mixBufferToMonoDual(buffer, getTotalNumInputChannels(), buffer.getNumSamples());
  AudioEffectFrameworkProcessor::processBlock(buffer, midiMessages);
  aef::duplicateMonoToStereoOutput(buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* MoogLadderAudioProcessor::createEditor() { return new MoogLadderEditor(*this); }

const juce::String MoogLadderAudioProcessor::getName() const { return JucePlugin_Name; }

bool MoogLadderAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MoogLadderAudioProcessor(); }
