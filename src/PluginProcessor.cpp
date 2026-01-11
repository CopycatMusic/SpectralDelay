#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CopyCatSpectralDelayProcessor::CopyCatSpectralDelayProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("CopyCatSpectralDelay"),
                 createParameterLayout()) {
  // Get parameter pointers
  fftSizeParam = parameters.getRawParameterValue("fftSize");
  mixParam = parameters.getRawParameterValue("mix");
}

CopyCatSpectralDelayProcessor::~CopyCatSpectralDelayProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
CopyCatSpectralDelayProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  // FFT Size: indices for 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "fftSize", "FFT Size",
      juce::StringArray{"32", "64", "128", "256", "512", "1024", "2048", "4096",
                        "8192", "16384"},
      6)); // Default: 2048 (index 6)

  // Mix (dry/wet)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 0) + "%"; }, nullptr));

  // Display mode: 0 = Line, 1 = Bar
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "displayMode", "Display Mode", juce::StringArray{"Line", "Bar"},
      0)); // Default: Line

  // Spectrum source: 0 = Output, 1 = Input
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "spectrumSource", "Spectrum Source", juce::StringArray{"Output", "Input"},
      0)); // Default: Output

  // Feedback Multiplier (0-100%)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "feedbackMult", "Feedback Multiplier",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 0) + "%"; }, nullptr));

  // Gain Multiplier (0-100%, default 100%)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "gainMult", "Gain Multiplier",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 0) + "%"; }, nullptr));

  // Delay Multiplier (0-100%, default 20%, non-linear for finer control near 0)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "delayMult", "Delay Multiplier",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 0.5f), 20.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 0) + "%"; }, nullptr));

  return {params.begin(), params.end()};
}

//==============================================================================
const juce::String CopyCatSpectralDelayProcessor::getName() const {
  return JucePlugin_Name;
}

bool CopyCatSpectralDelayProcessor::acceptsMidi() const { return false; }

bool CopyCatSpectralDelayProcessor::producesMidi() const { return false; }

bool CopyCatSpectralDelayProcessor::isMidiEffect() const { return false; }

double CopyCatSpectralDelayProcessor::getTailLengthSeconds() const {
  // Maximum delay time
  return 2.0;
}

int CopyCatSpectralDelayProcessor::getNumPrograms() { return 1; }

int CopyCatSpectralDelayProcessor::getCurrentProgram() { return 0; }

void CopyCatSpectralDelayProcessor::setCurrentProgram(int /*index*/) {}

const juce::String
CopyCatSpectralDelayProcessor::getProgramName(int /*index*/) {
  return {};
}

void CopyCatSpectralDelayProcessor::changeProgramName(
    int /*index*/, const juce::String & /*newName*/) {}

//==============================================================================
void CopyCatSpectralDelayProcessor::prepareToPlay(double sampleRate,
                                                  int samplesPerBlock) {
  engine.prepare(sampleRate, samplesPerBlock);
  lfoEngine.prepare(
      sampleRate); // Initialize LFO engine with correct sample rate

  // Convert FFT size index to actual size
  int fftSizeIndex = static_cast<int>(*fftSizeParam);
  int fftSizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
  engine.setFFTSize(fftSizes[fftSizeIndex]);

  // Set fixed overlap factor (4x)
  engine.setOverlapFactor(4);

  // Report latency to host
  setLatencySamples(engine.getLatencyInSamples());

  // Initialize parameter smoothers with 10ms smoothing time
  // This prevents clicks from abrupt parameter changes
  constexpr float smoothingTimeSeconds = 0.01f; // 10ms
  smoothedGainMult.reset(sampleRate, smoothingTimeSeconds);
  smoothedFeedbackMult.reset(sampleRate, smoothingTimeSeconds);
  smoothedDelayMult.reset(sampleRate, smoothingTimeSeconds);

  // Set initial values (no smoothing on first set)
  auto *gainMultParam = parameters.getRawParameterValue("gainMult");
  auto *feedbackMultParam = parameters.getRawParameterValue("feedbackMult");
  auto *delayMultParam = parameters.getRawParameterValue("delayMult");

  smoothedGainMult.setCurrentAndTargetValue(
      gainMultParam ? gainMultParam->load() : 100.0f);
  smoothedFeedbackMult.setCurrentAndTargetValue(
      feedbackMultParam ? feedbackMultParam->load() : 100.0f);
  smoothedDelayMult.setCurrentAndTargetValue(
      delayMultParam ? delayMultParam->load() : 20.0f);
}

void CopyCatSpectralDelayProcessor::releaseResources() { engine.reset(); }

bool CopyCatSpectralDelayProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  // Only stereo is supported
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void CopyCatSpectralDelayProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer & /*midiMessages*/) {
  juce::ScopedNoDenormals noDenormals;

  int numSamples = buffer.getNumSamples();

  // Update mix
  if (mixParam) {
    engine.setMix(*mixParam / 100.0f);
  }

  // Check for FFT size changes
  if (fftSizeParam) {
    int fftSizeIndex = static_cast<int>(*fftSizeParam);
    int fftSizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
    engine.setFFTSize(fftSizes[fftSizeIndex]);
  }

  // Update LFO host info
  if (auto *playHead = getPlayHead()) {
    if (auto hostInfo = playHead->getPosition()) {
      double bpm = hostInfo->getBpm().orFallback(120.0);
      double ppq = hostInfo->getPpqPosition().orFallback(0.0);
      bool playing = hostInfo->getIsPlaying();
      lfoEngine.updateHostInfo(bpm, ppq, playing);
    }
  }

  // Advance LFO phases by buffer size
  lfoEngine.advance(numSamples);

  // Get base multipliers from automation parameters
  auto *feedbackMultParam = parameters.getRawParameterValue("feedbackMult");
  auto *gainMultParam = parameters.getRawParameterValue("gainMult");
  auto *delayMultParam = parameters.getRawParameterValue("delayMult");

  float feedbackMult = feedbackMultParam ? feedbackMultParam->load() : 100.0f;
  float gainMult = gainMultParam ? gainMultParam->load() : 100.0f;
  float delayMult = delayMultParam ? delayMultParam->load() : 20.0f;

  // Apply LFO modulation to multipliers
  // Use frame-averaged modulation to prevent aliasing with large FFT sizes
  float gainLfoRaw = lfoEngine.getFrameAveragedModulation(
      LFOEngine::Parameter::Gain, numSamples);
  float feedbackLfoRaw = lfoEngine.getFrameAveragedModulation(
      LFOEngine::Parameter::Feedback, numSamples);
  float delayLfoRaw = lfoEngine.getFrameAveragedModulation(
      LFOEngine::Parameter::Time, numSamples);

  // Apply user-controlled LFO smoothing (0-100ms per parameter)
  lfoEngine.updateSmoothedModulation(LFOEngine::Parameter::Gain, gainLfoRaw,
                                     numSamples);
  lfoEngine.updateSmoothedModulation(LFOEngine::Parameter::Feedback,
                                     feedbackLfoRaw, numSamples);
  lfoEngine.updateSmoothedModulation(LFOEngine::Parameter::Time, delayLfoRaw,
                                     numSamples);

  // Get smoothed LFO modulation values
  float gainLfoMod =
      lfoEngine.getSmoothedModulationValue(LFOEngine::Parameter::Gain);
  float feedbackLfoMod =
      lfoEngine.getSmoothedModulationValue(LFOEngine::Parameter::Feedback);
  float delayLfoMod =
      lfoEngine.getSmoothedModulationValue(LFOEngine::Parameter::Time);

  // Calculate final target values (base * LFO modulation)
  float targetGainMult = gainMult * gainLfoMod;
  float targetFeedbackMult = feedbackMult * feedbackLfoMod;
  float targetDelayMult = delayMult * delayLfoMod;

  // Set target values for system smoothers (10ms fixed, prevents clicks)
  smoothedGainMult.setTargetValue(targetGainMult);
  smoothedFeedbackMult.setTargetValue(targetFeedbackMult);
  smoothedDelayMult.setTargetValue(targetDelayMult);

  // Advance smoothers by numSamples and get current smoothed values
  smoothedGainMult.skip(numSamples);
  smoothedFeedbackMult.skip(numSamples);
  smoothedDelayMult.skip(numSamples);

  // Apply smoothed multipliers to engine
  engine.setGainMultiplier(smoothedGainMult.getCurrentValue());
  engine.setFeedbackMultiplier(smoothedFeedbackMult.getCurrentValue());
  engine.setDelayMultiplier(smoothedDelayMult.getCurrentValue());

  // Update latency if it changed
  int newLatency = engine.getLatencyInSamples();
  if (getLatencySamples() != newLatency) {
    setLatencySamples(newLatency);
  }

  // Process audio (curves are edited directly via UI)
  engine.process(buffer);
}

//==============================================================================
bool CopyCatSpectralDelayProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *CopyCatSpectralDelayProcessor::createEditor() {
  return new CopyCatSpectralDelayEditor(*this);
}

//==============================================================================
void CopyCatSpectralDelayProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  // Start with parameter state
  auto state = parameters.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());

  // Add curve point data
  xml->setAttribute("phaseCurve", engine.getPhaseCurve().serializePoints());
  xml->setAttribute("gainCurve", engine.getGainCurve().serializePoints());
  xml->setAttribute("feedbackCurve",
                    engine.getFeedbackCurve().serializePoints());
  xml->setAttribute("delayCurve", engine.getDelayCurve().serializePoints());

  // Save delay mode state (Phase vs Delay)
  xml->setAttribute("delayMode", engine.isDelayMode());

  // Save UI state
  xml->setAttribute("uiParameterType", uiParameterType);
  xml->setAttribute("uiDisplayModeBar", uiDisplayModeBar);
  xml->setAttribute("uiShowInputSpectrum", uiShowInputSpectrum);

  // Save feedback invert state
  xml->setAttribute("feedbackInvert", engine.isFeedbackInvert());

  // Save high precision delay mode
  xml->setAttribute("highPrecisionDelay", engine.isHighPrecisionDelay());

  // Save gain post mode
  xml->setAttribute("gainPostMode", engine.isGainPostMode());

  // Save stereo width parameters
  xml->setAttribute("stereoAmount",
                    static_cast<double>(engine.getStereoAmount()));
  xml->setAttribute("stereoLowCut",
                    static_cast<double>(engine.getStereoLowCut()));

  // Save LFO state for each parameter type
  // Gain LFO
  xml->setAttribute("lfoGainAmount", static_cast<double>(lfoEngine.getAmount(
                                         LFOEngine::Parameter::Gain)));
  xml->setAttribute("lfoGainSpeed", static_cast<double>(lfoEngine.getSpeed(
                                        LFOEngine::Parameter::Gain)));
  xml->setAttribute("lfoGainSmooth", static_cast<double>(lfoEngine.getSmooth(
                                         LFOEngine::Parameter::Gain)));
  xml->setAttribute("lfoGainSyncMode", static_cast<int>(lfoEngine.getSyncMode(
                                           LFOEngine::Parameter::Gain)));
  xml->setAttribute("lfoGainSynced",
                    lfoEngine.getSynced(LFOEngine::Parameter::Gain));
  xml->setAttribute(
      "lfoGainCurve",
      lfoEngine.getCurve(LFOEngine::Parameter::Gain).serializePoints());

  // Feedback LFO
  xml->setAttribute(
      "lfoFeedbackAmount",
      static_cast<double>(lfoEngine.getAmount(LFOEngine::Parameter::Feedback)));
  xml->setAttribute("lfoFeedbackSpeed", static_cast<double>(lfoEngine.getSpeed(
                                            LFOEngine::Parameter::Feedback)));
  xml->setAttribute(
      "lfoFeedbackSmooth",
      static_cast<double>(lfoEngine.getSmooth(LFOEngine::Parameter::Feedback)));
  xml->setAttribute(
      "lfoFeedbackSyncMode",
      static_cast<int>(lfoEngine.getSyncMode(LFOEngine::Parameter::Feedback)));
  xml->setAttribute("lfoFeedbackSynced",
                    lfoEngine.getSynced(LFOEngine::Parameter::Feedback));
  xml->setAttribute(
      "lfoFeedbackCurve",
      lfoEngine.getCurve(LFOEngine::Parameter::Feedback).serializePoints());

  // Time LFO
  xml->setAttribute("lfoTimeAmount", static_cast<double>(lfoEngine.getAmount(
                                         LFOEngine::Parameter::Time)));
  xml->setAttribute("lfoTimeSpeed", static_cast<double>(lfoEngine.getSpeed(
                                        LFOEngine::Parameter::Time)));
  xml->setAttribute("lfoTimeSmooth", static_cast<double>(lfoEngine.getSmooth(
                                         LFOEngine::Parameter::Time)));
  xml->setAttribute("lfoTimeSyncMode", static_cast<int>(lfoEngine.getSyncMode(
                                           LFOEngine::Parameter::Time)));
  xml->setAttribute("lfoTimeSynced",
                    lfoEngine.getSynced(LFOEngine::Parameter::Time));
  xml->setAttribute(
      "lfoTimeCurve",
      lfoEngine.getCurve(LFOEngine::Parameter::Time).serializePoints());

  copyXmlToBinary(*xml, destData);
}

void CopyCatSpectralDelayProcessor::setStateInformation(const void *data,
                                                        int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(parameters.state.getType())) {
      parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

    // Restore curve points
    if (xmlState->hasAttribute("phaseCurve")) {
      engine.getPhaseCurve().deserializePoints(
          xmlState->getStringAttribute("phaseCurve"));
      engine.markPhaseNeedsUpdate();
    }

    if (xmlState->hasAttribute("gainCurve")) {
      engine.getGainCurve().deserializePoints(
          xmlState->getStringAttribute("gainCurve"));
      engine.markGainNeedsUpdate();
    }

    if (xmlState->hasAttribute("feedbackCurve")) {
      engine.getFeedbackCurve().deserializePoints(
          xmlState->getStringAttribute("feedbackCurve"));
      engine.markFeedbackNeedsUpdate();
    }

    if (xmlState->hasAttribute("delayCurve")) {
      engine.getDelayCurve().deserializePoints(
          xmlState->getStringAttribute("delayCurve"));
      engine.markDelayNeedsUpdate();
    }

    // Restore delay mode state
    if (xmlState->hasAttribute("delayMode")) {
      engine.setDelayMode(xmlState->getBoolAttribute("delayMode", true));
    }

    // Restore UI state
    if (xmlState->hasAttribute("uiParameterType")) {
      uiParameterType = xmlState->getIntAttribute("uiParameterType", 0);
    }
    if (xmlState->hasAttribute("uiDisplayModeBar")) {
      uiDisplayModeBar = xmlState->getBoolAttribute("uiDisplayModeBar", false);
    }
    if (xmlState->hasAttribute("uiShowInputSpectrum")) {
      uiShowInputSpectrum =
          xmlState->getBoolAttribute("uiShowInputSpectrum", false);
    }

    // Restore feedback invert state
    if (xmlState->hasAttribute("feedbackInvert")) {
      engine.setFeedbackInvert(
          xmlState->getBoolAttribute("feedbackInvert", false));
    }

    // Restore high precision delay mode
    if (xmlState->hasAttribute("highPrecisionDelay")) {
      engine.setHighPrecisionDelay(
          xmlState->getBoolAttribute("highPrecisionDelay", false));
    }

    // Restore gain post mode
    if (xmlState->hasAttribute("gainPostMode")) {
      engine.setGainPostMode(xmlState->getBoolAttribute("gainPostMode", false));
    }

    // Restore stereo width parameters
    if (xmlState->hasAttribute("stereoAmount")) {
      engine.setStereoAmount(static_cast<float>(
          xmlState->getDoubleAttribute("stereoAmount", 0.0)));
    }
    if (xmlState->hasAttribute("stereoLowCut")) {
      engine.setStereoLowCut(static_cast<float>(
          xmlState->getDoubleAttribute("stereoLowCut", 150.0)));
    }

    // Restore LFO state for each parameter type
    // Gain LFO
    if (xmlState->hasAttribute("lfoGainAmount")) {
      lfoEngine.setAmount(LFOEngine::Parameter::Gain,
                          static_cast<float>(xmlState->getDoubleAttribute(
                              "lfoGainAmount", 0.0)));
    }
    if (xmlState->hasAttribute("lfoGainSpeed")) {
      lfoEngine.setSpeed(LFOEngine::Parameter::Gain,
                         static_cast<float>(xmlState->getDoubleAttribute(
                             "lfoGainSpeed", 500.0)));
    }
    if (xmlState->hasAttribute("lfoGainSmooth")) {
      lfoEngine.setSmooth(LFOEngine::Parameter::Gain,
                          static_cast<float>(xmlState->getDoubleAttribute(
                              "lfoGainSmooth", 0.0)));
    }
    if (xmlState->hasAttribute("lfoGainSyncMode")) {
      lfoEngine.setSyncMode(
          LFOEngine::Parameter::Gain,
          static_cast<LFOEngine::SyncMode>(
              xmlState->getIntAttribute("lfoGainSyncMode", 0)));
    } else if (xmlState->hasAttribute("lfoGainBeatMode")) {
      // Legacy compatibility: convert old bool to new enum
      bool wasBeat = xmlState->getBoolAttribute("lfoGainBeatMode", false);
      lfoEngine.setSyncMode(LFOEngine::Parameter::Gain,
                            wasBeat ? LFOEngine::SyncMode::Note
                                    : LFOEngine::SyncMode::Time);
    }
    if (xmlState->hasAttribute("lfoGainSynced")) {
      lfoEngine.setSynced(LFOEngine::Parameter::Gain,
                          xmlState->getBoolAttribute("lfoGainSynced"));
    }
    if (xmlState->hasAttribute("lfoGainCurve")) {
      lfoEngine.getCurve(LFOEngine::Parameter::Gain)
          .deserializePoints(xmlState->getStringAttribute("lfoGainCurve"));
    }

    // Feedback LFO
    if (xmlState->hasAttribute("lfoFeedbackAmount")) {
      lfoEngine.setAmount(LFOEngine::Parameter::Feedback,
                          static_cast<float>(xmlState->getDoubleAttribute(
                              "lfoFeedbackAmount", 0.0)));
    }
    if (xmlState->hasAttribute("lfoFeedbackSpeed")) {
      lfoEngine.setSpeed(LFOEngine::Parameter::Feedback,
                         static_cast<float>(xmlState->getDoubleAttribute(
                             "lfoFeedbackSpeed", 500.0)));
    }
    if (xmlState->hasAttribute("lfoFeedbackSmooth")) {
      lfoEngine.setSmooth(LFOEngine::Parameter::Feedback,
                          static_cast<float>(xmlState->getDoubleAttribute(
                              "lfoFeedbackSmooth", 0.0)));
    }
    if (xmlState->hasAttribute("lfoFeedbackSyncMode")) {
      lfoEngine.setSyncMode(
          LFOEngine::Parameter::Feedback,
          static_cast<LFOEngine::SyncMode>(
              xmlState->getIntAttribute("lfoFeedbackSyncMode", 0)));
    } else if (xmlState->hasAttribute("lfoFeedbackBeatMode")) {
      bool wasBeat = xmlState->getBoolAttribute("lfoFeedbackBeatMode", false);
      lfoEngine.setSyncMode(LFOEngine::Parameter::Feedback,
                            wasBeat ? LFOEngine::SyncMode::Note
                                    : LFOEngine::SyncMode::Time);
    }
    if (xmlState->hasAttribute("lfoFeedbackSynced")) {
      lfoEngine.setSynced(LFOEngine::Parameter::Feedback,
                          xmlState->getBoolAttribute("lfoFeedbackSynced"));
    }
    if (xmlState->hasAttribute("lfoFeedbackCurve")) {
      lfoEngine.getCurve(LFOEngine::Parameter::Feedback)
          .deserializePoints(xmlState->getStringAttribute("lfoFeedbackCurve"));
    }

    // Time LFO
    if (xmlState->hasAttribute("lfoTimeAmount")) {
      lfoEngine.setAmount(LFOEngine::Parameter::Time,
                          static_cast<float>(xmlState->getDoubleAttribute(
                              "lfoTimeAmount", 0.0)));
    }
    if (xmlState->hasAttribute("lfoTimeSpeed")) {
      lfoEngine.setSpeed(LFOEngine::Parameter::Time,
                         static_cast<float>(xmlState->getDoubleAttribute(
                             "lfoTimeSpeed", 500.0)));
    }
    if (xmlState->hasAttribute("lfoTimeSmooth")) {
      lfoEngine.setSmooth(LFOEngine::Parameter::Time,
                          static_cast<float>(xmlState->getDoubleAttribute(
                              "lfoTimeSmooth", 0.0)));
    }
    if (xmlState->hasAttribute("lfoTimeSyncMode")) {
      lfoEngine.setSyncMode(
          LFOEngine::Parameter::Time,
          static_cast<LFOEngine::SyncMode>(
              xmlState->getIntAttribute("lfoTimeSyncMode", 0)));
    } else if (xmlState->hasAttribute("lfoTimeBeatMode")) {
      bool wasBeat = xmlState->getBoolAttribute("lfoTimeBeatMode", false);
      lfoEngine.setSyncMode(LFOEngine::Parameter::Time,
                            wasBeat ? LFOEngine::SyncMode::Note
                                    : LFOEngine::SyncMode::Time);
    }
    if (xmlState->hasAttribute("lfoTimeSynced")) {
      lfoEngine.setSynced(LFOEngine::Parameter::Time,
                          xmlState->getBoolAttribute("lfoTimeSynced"));
    }
    if (xmlState->hasAttribute("lfoTimeCurve")) {
      lfoEngine.getCurve(LFOEngine::Parameter::Time)
          .deserializePoints(xmlState->getStringAttribute("lfoTimeCurve"));
    }
  }
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new CopyCatSpectralDelayProcessor();
}
