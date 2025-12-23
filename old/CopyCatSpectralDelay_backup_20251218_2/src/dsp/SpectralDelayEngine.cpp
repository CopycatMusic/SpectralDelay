#include "SpectralDelayEngine.h"
#include <cmath>

//==============================================================================
SpectralDelayEngine::SpectralDelayEngine() {
  // Set default values for curves
  phaseCurve.setDefaultValue(0.0f);    // Default: no phase shift
  gainCurve.setDefaultValue(100.0f);   // Default: unity gain (100%)
  feedbackCurve.setDefaultValue(0.0f); // Default: no feedback
}

SpectralDelayEngine::~SpectralDelayEngine() {}

//==============================================================================
void SpectralDelayEngine::prepare(double sampleRate, int maxBlockSize) {
  currentSampleRate = sampleRate;

  // Prepare STFT processors
  stftProcessorL.prepare(sampleRate, maxBlockSize);
  stftProcessorR.prepare(sampleRate, maxBlockSize);

  currentNumBins = stftProcessorL.getNumBins();
  currentHopSize = stftProcessorL.getHopSize();

  // Initialize phase shifts cache
  phaseShifts.resize(currentNumBins, 0.0f);
  phaseNeedsUpdate = true;

  // Initialize gain values cache
  gainValues.resize(currentNumBins, 100.0f);
  gainNeedsUpdate = true;

  // Initialize feedback values cache and spectrum buffers
  feedbackValues.resize(currentNumBins, 0.0f);
  feedbackNeedsUpdate = true;
  feedbackSpectrumL.resize(currentNumBins, std::complex<float>(0.0f, 0.0f));
  feedbackSpectrumR.resize(currentNumBins, std::complex<float>(0.0f, 0.0f));

  // Initialize input spectrum storage
  inputMagnitudeSpectrum.resize(currentNumBins, 0.0f);

  // Prepare dry buffer
  dryBuffer.setSize(2, maxBlockSize);

  // Prepare dry signal delay lines to match STFT latency
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
  spec.numChannels = 1;

  dryDelayL.prepare(spec);
  dryDelayR.prepare(spec);

  // Set delay to match STFT latency
  int latency = stftProcessorL.getLatencyInSamples();
  dryDelayL.setDelay(static_cast<float>(latency));
  dryDelayR.setDelay(static_cast<float>(latency));
}

void SpectralDelayEngine::reset() {
  stftProcessorL.reset();
  stftProcessorR.reset();
  dryDelayL.reset();
  dryDelayR.reset();
}

//==============================================================================
void SpectralDelayEngine::process(juce::AudioBuffer<float> &buffer) {
  int numSamples = buffer.getNumSamples();
  int numChannels = buffer.getNumChannels();

  if (numChannels < 2 || numSamples == 0)
    return;

  // Store dry signal for mix
  dryBuffer.makeCopyOf(buffer, true);

  // Update phase shifts if needed
  if (phaseNeedsUpdate) {
    updatePhaseShifts();
  }

  // Update gain values if needed
  if (gainNeedsUpdate) {
    updateGainValues();
  }

  // Update feedback values if needed
  if (feedbackNeedsUpdate) {
    updateFeedbackValues();
  }

  // Process with spectral modifications
  auto *leftIn = buffer.getReadPointer(0);
  auto *leftOut = buffer.getWritePointer(0);

  stftProcessorL.process(
      leftIn, leftOut, numSamples,
      [this](std::complex<float> *spectrum, int numBins) {
        // Capture input spectrum magnitude before DSP
        // Apply same normalization as STFTProcessor uses for output spectrum
        float normFactor = 1.0f / static_cast<float>(currentFFTSize);
        for (int bin = 0; bin < numBins &&
                          bin < static_cast<int>(inputMagnitudeSpectrum.size());
             ++bin) {
          inputMagnitudeSpectrum[bin] = std::abs(spectrum[bin]) * normFactor;
        }
        // Apply DSP processing: Gain -> Feedback -> Phase
        applyGainModulation(spectrum, numBins);
        applyFeedback(spectrum, numBins, feedbackSpectrumL);
        applyPhaseModulation(spectrum, numBins);
      });

  auto *rightIn = buffer.getReadPointer(1);
  auto *rightOut = buffer.getWritePointer(1);

  stftProcessorR.process(rightIn, rightOut, numSamples,
                         [this](std::complex<float> *spectrum, int numBins) {
                           // Apply DSP processing: Gain -> Feedback -> Phase
                           applyGainModulation(spectrum, numBins);
                           applyFeedback(spectrum, numBins, feedbackSpectrumR);
                           applyPhaseModulation(spectrum, numBins);
                         });
  // With STFT disabled, buffer already contains the input signal
  // which will be used as the "wet" signal in the mix below

  // Apply dry/wet mix with delayed dry signal
  float wet = mixAmount;
  float dry = 1.0f - mixAmount;

  for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
    auto *wetData = buffer.getWritePointer(ch);
    auto *dryData = dryBuffer.getReadPointer(ch);
    auto &delayLine = (ch == 0) ? dryDelayL : dryDelayR;

    for (int i = 0; i < numSamples; ++i) {
      // Delay the dry signal to match STFT latency
      float delayedDry = delayLine.popSample(0);
      delayLine.pushSample(0, dryData[i]);

      wetData[i] = delayedDry * dry + wetData[i] * wet;
    }
  }
}

//==============================================================================
void SpectralDelayEngine::applyPhaseModulation(std::complex<float> *spectrum,
                                               int numBins) {
  for (int bin = 0; bin < numBins && bin < static_cast<int>(phaseShifts.size());
       ++bin) {
    float phaseShift = phaseShifts[bin];

    // Create rotation complex number: e^(i * phaseShift)
    std::complex<float> rotator(std::cos(phaseShift), std::sin(phaseShift));

    // Apply phase rotation
    spectrum[bin] *= rotator;
  }
}

//==============================================================================
void SpectralDelayEngine::setMix(float dryWet) {
  mixAmount = juce::jlimit(0.0f, 1.0f, dryWet);
}

//==============================================================================
void SpectralDelayEngine::setFFTSize(int size) {
  if (size == currentFFTSize)
    return;

  currentFFTSize = size;

  stftProcessorL.setFFTSize(size);
  stftProcessorR.setFFTSize(size);

  currentNumBins = stftProcessorL.getNumBins();
  currentHopSize = stftProcessorL.getHopSize();

  phaseShifts.resize(currentNumBins, 0.0f);
  phaseNeedsUpdate = true;

  gainValues.resize(currentNumBins, 100.0f);
  gainNeedsUpdate = true;

  // Update dry signal delay to match new STFT latency
  int latency = stftProcessorL.getLatencyInSamples();
  dryDelayL.setDelay(static_cast<float>(latency));
  dryDelayR.setDelay(static_cast<float>(latency));
}

void SpectralDelayEngine::setOverlapFactor(int factor) {
  if (factor == currentOverlapFactor)
    return;

  currentOverlapFactor = factor;

  stftProcessorL.setOverlapFactor(factor);
  stftProcessorR.setOverlapFactor(factor);

  currentHopSize = stftProcessorL.getHopSize();
  phaseNeedsUpdate = true;
}

//==============================================================================
const float *SpectralDelayEngine::getMagnitudeSpectrum() const {
  return stftProcessorL.getMagnitudeSpectrum();
}

const float *SpectralDelayEngine::getInputMagnitudeSpectrum() const {
  return inputMagnitudeSpectrum.data();
}

int SpectralDelayEngine::getNumBins() const { return currentNumBins; }

int SpectralDelayEngine::getLatencyInSamples() const {
  return stftProcessorL.getLatencyInSamples();
}

//==============================================================================
void SpectralDelayEngine::updatePhaseShifts() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  for (int i = 0; i < currentNumBins; ++i) {
    float freq = binToFrequency(i);
    float phaseDegrees = phaseCurve.getValueAtFrequency(freq);

    // Clamp degrees to valid range (0 to 720)
    phaseDegrees = juce::jlimit(0.0f, 720.0f, phaseDegrees);

    // Convert degrees to radians
    float phaseRadians = phaseDegrees * juce::MathConstants<float>::pi / 180.0f;

    phaseShifts[i] = phaseRadians;
  }

  phaseNeedsUpdate = false;
}

float SpectralDelayEngine::binToFrequency(int binIndex) const {
  return static_cast<float>(binIndex) * static_cast<float>(currentSampleRate) /
         static_cast<float>(currentFFTSize);
}

//==============================================================================
void SpectralDelayEngine::updateGainValues() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  for (int i = 0; i < currentNumBins; ++i) {
    float freq = binToFrequency(i);
    float gain = gainCurve.getValueAtFrequency(freq);

    // Clamp to valid range (0 to 200%)
    gain = juce::jlimit(0.0f, 200.0f, gain);

    gainValues[i] = gain;
  }

  gainNeedsUpdate = false;
}

void SpectralDelayEngine::applyGainModulation(std::complex<float> *spectrum,
                                              int numBins) {
  for (int bin = 0; bin < numBins && bin < static_cast<int>(gainValues.size());
       ++bin) {
    // Convert percent to linear gain (100% = 1.0)
    float gain = gainValues[bin] / 100.0f;
    spectrum[bin] *= gain;
  }
}

//==============================================================================
void SpectralDelayEngine::updateFeedbackValues() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  for (int i = 0; i < currentNumBins; ++i) {
    float freq = binToFrequency(i);
    float feedback = feedbackCurve.getValueAtFrequency(freq);

    // Clamp to valid range (0 to 100%)
    feedback = juce::jlimit(0.0f, 100.0f, feedback);

    feedbackValues[i] = feedback;
  }

  feedbackNeedsUpdate = false;
}

void SpectralDelayEngine::applyFeedback(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::complex<float>> &feedbackBuffer) {

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(feedbackValues.size()) &&
       bin < static_cast<int>(feedbackBuffer.size());
       ++bin) {
    // Convert percent to linear feedback amount (100% = 1.0)
    float feedbackAmount = feedbackValues[bin] / 100.0f;

    // Mix feedback buffer with current spectrum
    // new_spectrum = current + previous * feedback
    spectrum[bin] += feedbackBuffer[bin] * feedbackAmount;

    // Store current processed spectrum for next frame's feedback
    feedbackBuffer[bin] = spectrum[bin];
  }
}
