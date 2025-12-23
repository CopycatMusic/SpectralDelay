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
  overlapParam = parameters.getRawParameterValue("overlap");
  mixParam = parameters.getRawParameterValue("mix");
}

CopyCatSpectralDelayProcessor::~CopyCatSpectralDelayProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
CopyCatSpectralDelayProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  // FFT Size: indices for 256, 512, 1024, 2048, 4096, 8192
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "fftSize", "FFT Size",
      juce::StringArray{"256", "512", "1024", "2048", "4096", "8192"},
      3)); // Default: 2048

  // Overlap: 2x, 4x, 8x
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "overlap", "Overlap", juce::StringArray{"2x", "4x", "8x"},
      1)); // Default: 4x

  // Mix (dry/wet)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
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
  int fftSizes[] = {256, 512, 1024, 2048, 4096, 8192};
  engine.setFFTSize(fftSizes[fftSizeIndex]);

  // Convert overlap index to factor
  int overlapIndex = static_cast<int>(*overlapParam);
  int overlapFactors[] = {2, 4, 8};
  engine.setOverlapFactor(overlapFactors[overlapIndex]);

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
  engine.setMix(*mixParam / 100.0f);

  // Check for FFT size changes
  int fftSizeIndex = static_cast<int>(*fftSizeParam);
  int fftSizes[] = {256, 512, 1024, 2048, 4096, 8192};
  engine.setFFTSize(fftSizes[fftSizeIndex]);

  // Check for overlap changes
  int overlapIndex = static_cast<int>(*overlapParam);
  int overlapFactors[] = {2, 4, 8};
  engine.setOverlapFactor(overlapFactors[overlapIndex]);

  // Update feedback multiplier from automation parameter
  auto *feedbackMultParam = parameters.getRawParameterValue("feedbackMult");
  if (feedbackMultParam) {
    engine.setFeedbackMultiplier(*feedbackMultParam);
  }

  // Update gain multiplier from automation parameter
  auto *gainMultParam = parameters.getRawParameterValue("gainMult");
  if (gainMultParam) {
    engine.setGainMultiplier(*gainMultParam);
  }

  // Update delay multiplier from automation parameter
  auto *delayMultParam = parameters.getRawParameterValue("delayMult");
  if (delayMultParam) {
    engine.setDelayMultiplier(*delayMultParam);
  }

  // Update latency if FFT size changed
  setLatencySamples(engine.getLatencyInSamples());

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
      engine.setDelayMode(xmlState->getBoolAttribute("delayMode", false));
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
  }
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new CopyCatSpectralDelayProcessor();
}
