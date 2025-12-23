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
  curveModeParam = parameters.getRawParameterValue("curveMode");
  constantDelayParam = parameters.getRawParameterValue("constantPhase");
  slopeParam = parameters.getRawParameterValue("slope");
  offsetParam = parameters.getRawParameterValue("offset");
  fftSizeParam = parameters.getRawParameterValue("fftSize");
  overlapParam = parameters.getRawParameterValue("overlap");
  mixParam = parameters.getRawParameterValue("mix");

  // Gain parameters
  gainCurveModeParam = parameters.getRawParameterValue("gainCurveMode");
  gainConstantParam = parameters.getRawParameterValue("gainConstant");
  gainSlopeParam = parameters.getRawParameterValue("gainSlope");
  gainOffsetParam = parameters.getRawParameterValue("gainOffset");
}

CopyCatSpectralDelayProcessor::~CopyCatSpectralDelayProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
CopyCatSpectralDelayProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  // Curve mode: 0 = Constant, 1 = Linear
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "curveMode", "Curve Mode", juce::StringArray{"Constant", "Linear"}, 0));

  // Constant phase value (0 to 360 degrees, stored as radians internally)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "constantPhase", "Phase",
      juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f), 0.0f, juce::String(),
      juce::AudioProcessorParameter::genericParameter,
      [](float value, int) {
        return juce::String(value, 1) +
               juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
      },
      nullptr));

  // Slope for linear mode (phase change per decade)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "slope", "Slope", juce::NormalisableRange<float>(-180.0f, 180.0f, 0.1f),
      0.0f, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) {
        return juce::String(value, 1) +
               juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
      },
      nullptr));

  // Offset for linear mode (degrees)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "offset", "Offset", juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f),
      0.0f, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) {
        return juce::String(value, 1) +
               juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
      },
      nullptr));

  // FFT Size: indices for 256, 512, 1024, 2048, 4096, 8192
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "fftSize", "FFT Size",
      juce::StringArray{"256", "512", "1024", "2048", "4096", "8192"},
      3)); // Default: 2048

  // Overlap: 2x, 4x, 8x
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "overlap", "Overlap", juce::StringArray{"2x", "4x", "8x"},
      1)); // Default: 4x

  // === Gain parameters ===
  // Gain curve mode: 0 = Constant, 1 = Linear
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "gainCurveMode", "Gain Curve Mode",
      juce::StringArray{"Constant", "Linear"}, 0));

  // Constant gain value (0 to 200%, where 100% = unity gain)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "gainConstant", "Gain",
      juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + "%"; }, nullptr));

  // Gain slope for linear mode (change per decade, -100% to +100%)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "gainSlope", "Gain Slope",
      juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + "%"; }, nullptr));

  // Gain offset for linear mode (0% to 200%)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "gainOffset", "Gain Offset",
      juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + "%"; }, nullptr));

  // Mix (dry/wet)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
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

  // Report latency to host for automatic delay compensation
  setLatencySamples(engine.getLatencyInSamples());
}

void CopyCatSpectralDelayProcessor::releaseResources() {}

bool CopyCatSpectralDelayProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  // Support stereo only
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void CopyCatSpectralDelayProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer & /*midiMessages*/) {
  juce::ScopedNoDenormals noDenormals;

  // Update engine parameters
  int curveMode = static_cast<int>(*curveModeParam);

  // Convert degrees to radians
  const float degToRad = juce::MathConstants<float>::pi / 180.0f;

  if (curveMode == 0) {
    // Constant mode
    engine.setConstantPhase(*constantDelayParam * degToRad);
  } else {
    // Linear mode (convert degrees to radians)
    engine.setLinearCurve(*slopeParam * degToRad, *offsetParam * degToRad);
  }

  engine.setMix(*mixParam / 100.0f);

  // Update gain parameters
  int gainCurveMode = static_cast<int>(*gainCurveModeParam);
  if (gainCurveMode == 0) {
    // Constant mode
    engine.setConstantGain(*gainConstantParam);
  } else {
    // Linear mode
    engine.setLinearGainCurve(*gainSlopeParam, *gainOffsetParam);
  }

  // Check for FFT size changes
  int fftSizeIndex = static_cast<int>(*fftSizeParam);
  int fftSizes[] = {256, 512, 1024, 2048, 4096, 8192};
  engine.setFFTSize(fftSizes[fftSizeIndex]);

  // Check for overlap changes
  int overlapIndex = static_cast<int>(*overlapParam);
  int overlapFactors[] = {2, 4, 8};
  engine.setOverlapFactor(overlapFactors[overlapIndex]);

  // Update latency if FFT size changed
  setLatencySamples(engine.getLatencyInSamples());

  // Process audio
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
  auto state = parameters.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void CopyCatSpectralDelayProcessor::setStateInformation(const void *data,
                                                        int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(parameters.state.getType()))
      parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new CopyCatSpectralDelayProcessor();
}
