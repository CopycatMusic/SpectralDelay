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

  // FFT Size: indices for 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
  // 32768
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "fftSize", "FFT Size",
      juce::StringArray{"32", "64", "128", "256", "512", "1024", "2048", "4096",
                        "8192", "16384", "32768"},
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

  // Convert FFT size index to actual size
  int fftSizeIndex = static_cast<int>(*fftSizeParam);
  int fftSizes[] = {32,   64,   128,  256,   512,  1024,
                    2048, 4096, 8192, 16384, 32768};
  engine.setFFTSize(fftSizes[fftSizeIndex]);

  // Set fixed overlap factor (4x)
  engine.setOverlapFactor(4);

  // Report latency to host
  setLatencySamples(engine.getLatencyInSamples());
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

  // Update mix
  if (mixParam) {
    engine.setMix(*mixParam / 100.0f);
  }

  // Check for FFT size changes
  if (fftSizeParam) {
    int fftSizeIndex = static_cast<int>(*fftSizeParam);
    int fftSizes[] = {32,   64,   128,  256,   512,  1024,
                      2048, 4096, 8192, 16384, 32768};
    engine.setFFTSize(fftSizes[fftSizeIndex]);
  }

  // Prepare LFO engine with sample rate if not already done
  if (lfoEngine.getSampleRate() != getSampleRate()) {
    lfoEngine.prepare(getSampleRate());
  }

  // Advance LFO phases by buffer size
  lfoEngine.advance(buffer.getNumSamples());

  // Get base multipliers from automation parameters
  auto *feedbackMultParam = parameters.getRawParameterValue("feedbackMult");
  auto *gainMultParam = parameters.getRawParameterValue("gainMult");
  auto *delayMultParam = parameters.getRawParameterValue("delayMult");

  float feedbackMult = feedbackMultParam ? feedbackMultParam->load() : 100.0f;
  float gainMult = gainMultParam ? gainMultParam->load() : 100.0f;
  float delayMult = delayMultParam ? delayMultParam->load() : 20.0f;

  // Apply LFO modulation to multipliers
  // LFO returns a value 0.0-1.0, representing the modulation factor
  float gainLfoMod = lfoEngine.getModulationValue(LFOEngine::Parameter::Gain);
  float feedbackLfoMod =
      lfoEngine.getModulationValue(LFOEngine::Parameter::Feedback);
  float delayLfoMod = lfoEngine.getModulationValue(LFOEngine::Parameter::Time);

  // Final multipliers = base * LFO modulation
  engine.setGainMultiplier(gainMult * gainLfoMod);
  engine.setFeedbackMultiplier(feedbackMult * feedbackLfoMod);
  engine.setDelayMultiplier(delayMult * delayLfoMod);

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

  // Save LFO state for each parameter type
  // Gain LFO
  xml->setAttribute("lfoGainAmount", static_cast<double>(lfoEngine.getAmount(
                                         LFOEngine::Parameter::Gain)));
  xml->setAttribute("lfoGainSpeed", static_cast<double>(lfoEngine.getSpeed(
                                        LFOEngine::Parameter::Gain)));
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
      "lfoFeedbackCurve",
      lfoEngine.getCurve(LFOEngine::Parameter::Feedback).serializePoints());

  // Time LFO
  xml->setAttribute("lfoTimeAmount", static_cast<double>(lfoEngine.getAmount(
                                         LFOEngine::Parameter::Time)));
  xml->setAttribute("lfoTimeSpeed", static_cast<double>(lfoEngine.getSpeed(
                                        LFOEngine::Parameter::Time)));
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
