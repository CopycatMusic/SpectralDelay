#include "SpectralDelayEngine.h"
#include <cmath>

//==============================================================================
SpectralDelayEngine::SpectralDelayEngine() {
  // Set default values for curves
  phaseCurve.setDefaultValue(0.0f);    // Default: no phase shift
  gainCurve.setDefaultValue(100.0f);   // Default: unity gain (100%)
  feedbackCurve.setDefaultValue(0.0f); // Default: no feedback
  delayCurve.setDefaultValue(0.0f);    // Default: no delay
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

  // Initialize delay values cache and per-bin delay buffers
  delayValuesMs.resize(currentNumBins, 0.0f);
  delayNeedsUpdate = true;

  // Calculate max delay in STFT frames (2000ms at current sample rate)
  // For sample-level delay, we store complex spectrum values per bin
  float maxDelayMs = 2000.0f;
  float hopSizeSec = static_cast<float>(currentFFTSize) /
                     static_cast<float>(currentOverlapFactor) /
                     static_cast<float>(sampleRate);
  maxDelaySamples = static_cast<int>(maxDelayMs / 1000.0f / hopSizeSec) + 1;

  // Allocate delay buffers for each bin
  delayBufferL.resize(currentNumBins);
  delayBufferR.resize(currentNumBins);
  delayWritePosL.resize(currentNumBins, 0);
  delayWritePosR.resize(currentNumBins, 0);
  for (int bin = 0; bin < currentNumBins; ++bin) {
    delayBufferL[bin].resize(maxDelaySamples, std::complex<float>(0.0f, 0.0f));
    delayBufferR[bin].resize(maxDelaySamples, std::complex<float>(0.0f, 0.0f));
  }

  // Log memory usage (only in Debug builds)
  size_t delayMemoryBytes =
      currentNumBins * maxDelaySamples * sizeof(std::complex<float>) * 2;
  DBG("=== SpectralDelayEngine Memory Usage ===");
  DBG("FFT Size: " + juce::String(currentFFTSize) +
      ", Bins: " + juce::String(currentNumBins));
  DBG("Max delay frames: " + juce::String(maxDelaySamples));
  DBG("Delay buffer memory: " +
      juce::String(delayMemoryBytes / 1024.0f / 1024.0f, 2) + " MB");

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

  // Initialize output FFT analyzer (for post-mix spectrum display)
  int outputFFTOrder = static_cast<int>(std::log2(currentFFTSize));
  outputFFT = std::make_unique<juce::dsp::FFT>(outputFFTOrder);
  outputFFTBuffer.resize(currentFFTSize * 2, 0.0f);
  outputMagnitudeSpectrum.resize(currentNumBins, 0.0f);
  outputAccumBuffer.resize(currentFFTSize, 0.0f);
  outputAccumPos = 0;

  // Create Hann window for output FFT
  outputWindow.resize(currentFFTSize);
  for (int i = 0; i < currentFFTSize; ++i) {
    outputWindow[i] =
        0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                static_cast<float>(i) /
                                static_cast<float>(currentFFTSize)));
  }
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

  // Update delay values if needed
  if (delayNeedsUpdate) {
    updateDelayValues();
  }

  // Process with spectral modifications
  auto *leftIn = buffer.getReadPointer(0);
  auto *leftOut = buffer.getWritePointer(0);

  stftProcessorL.process(
      leftIn, leftOut, numSamples,
      [this](std::complex<float> *spectrum, int numBins) {
        // Capture input spectrum magnitude before DSP
        float normFactor = 1.0f / static_cast<float>(currentFFTSize);
        for (int bin = 0; bin < numBins &&
                          bin < static_cast<int>(inputMagnitudeSpectrum.size());
             ++bin) {
          inputMagnitudeSpectrum[bin] = std::abs(spectrum[bin]) * normFactor;
        }
        // DSP flow: Gain -> (+Feedback) -> [Phase or Delay] -> (storeFeedback)
        applyGainModulation(spectrum, numBins);
        applyFeedback(spectrum, numBins, feedbackSpectrumL);

        // Apply either Phase rotation or Delay buffer based on mode
        if (useDelayMode) {
          applyDelay(spectrum, numBins, delayBufferL, delayWritePosL);
        } else {
          applyPhaseModulation(spectrum, numBins);
        }

        storeFeedback(spectrum, numBins, feedbackSpectrumL);
      });

  auto *rightIn = buffer.getReadPointer(1);
  auto *rightOut = buffer.getWritePointer(1);

  stftProcessorR.process(rightIn, rightOut, numSamples,
                         [this](std::complex<float> *spectrum, int numBins) {
                           // DSP flow: Gain -> (+Feedback) -> [Phase or Delay]
                           applyGainModulation(spectrum, numBins);
                           applyFeedback(spectrum, numBins, feedbackSpectrumR);

                           if (useDelayMode) {
                             applyDelay(spectrum, numBins, delayBufferR,
                                        delayWritePosR);
                           } else {
                             applyPhaseModulation(spectrum, numBins);
                           }

                           storeFeedback(spectrum, numBins, feedbackSpectrumR);
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

  // Analyze output spectrum (left channel only, after mix)
  if (outputFFT != nullptr && !outputAccumBuffer.empty()) {
    auto *outputData = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i) {
      outputAccumBuffer[outputAccumPos] = outputData[i];
      outputAccumPos++;

      // When buffer is full, perform FFT analysis
      if (outputAccumPos >= currentFFTSize) {
        // Apply window and copy to FFT buffer
        for (int j = 0; j < currentFFTSize; ++j) {
          outputFFTBuffer[j] = outputAccumBuffer[j] * outputWindow[j];
        }
        // Clear imaginary part
        for (int j = currentFFTSize; j < currentFFTSize * 2; ++j) {
          outputFFTBuffer[j] = 0.0f;
        }

        // Perform FFT
        outputFFT->performRealOnlyForwardTransform(outputFFTBuffer.data(),
                                                   true);

        // Calculate magnitude spectrum
        auto *spectrum =
            reinterpret_cast<std::complex<float> *>(outputFFTBuffer.data());
        float normFactor = 1.0f / static_cast<float>(currentFFTSize);
        for (int bin = 0; bin < currentNumBins; ++bin) {
          outputMagnitudeSpectrum[bin] = std::abs(spectrum[bin]) * normFactor;
        }

        // Reset accumulator (with overlap - keep half the samples)
        int shiftAmount = currentFFTSize / 2;
        for (int j = 0; j < shiftAmount; ++j) {
          outputAccumBuffer[j] = outputAccumBuffer[j + shiftAmount];
        }
        outputAccumPos = shiftAmount;
      }
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

  // Recalculate maxDelaySamples based on new hop size
  float maxDelayMs = 2000.0f;
  float hopSizeSec = static_cast<float>(currentHopSize) /
                     static_cast<float>(currentSampleRate);
  maxDelaySamples = static_cast<int>(maxDelayMs / 1000.0f / hopSizeSec) + 1;

  // Resize phase and gain buffers
  phaseShifts.resize(currentNumBins, 0.0f);
  phaseNeedsUpdate = true;

  gainValues.resize(currentNumBins, 100.0f);
  gainNeedsUpdate = true;

  // Resize feedback buffers and clear them
  feedbackValues.resize(currentNumBins, 0.0f);
  feedbackSpectrumL.assign(currentNumBins, std::complex<float>(0.0f, 0.0f));
  feedbackSpectrumR.assign(currentNumBins, std::complex<float>(0.0f, 0.0f));
  feedbackNeedsUpdate = true;

  // Resize and clear delay buffers
  delayValuesMs.resize(currentNumBins, 0.0f);
  delayBufferL.resize(currentNumBins);
  delayBufferR.resize(currentNumBins);
  // Reset all write positions to 0
  delayWritePosL.assign(currentNumBins, 0);
  delayWritePosR.assign(currentNumBins, 0);
  // Clear and resize each bin's delay buffer
  for (int bin = 0; bin < currentNumBins; ++bin) {
    delayBufferL[bin].assign(maxDelaySamples, std::complex<float>(0.0f, 0.0f));
    delayBufferR[bin].assign(maxDelaySamples, std::complex<float>(0.0f, 0.0f));
  }
  delayNeedsUpdate = true;

  // Resize input spectrum storage
  inputMagnitudeSpectrum.resize(currentNumBins, 0.0f);

  // Resize output FFT analyzer buffers (critical for UI thread safety)
  int outputFFTOrder = static_cast<int>(std::log2(currentFFTSize));
  outputFFT = std::make_unique<juce::dsp::FFT>(outputFFTOrder);
  outputFFTBuffer.resize(currentFFTSize * 2, 0.0f);
  outputMagnitudeSpectrum.assign(currentNumBins, 0.0f);
  outputAccumBuffer.assign(currentFFTSize, 0.0f);
  outputAccumPos = 0;

  // Recreate Hann window for output FFT
  outputWindow.resize(currentFFTSize);
  for (int i = 0; i < currentFFTSize; ++i) {
    outputWindow[i] =
        0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                static_cast<float>(i) /
                                static_cast<float>(currentFFTSize)));
  }

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
  // Return post-mix output spectrum for accurate visualization
  return outputMagnitudeSpectrum.data();
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

    // Clamp degrees to valid range (0 to 360)
    phaseDegrees = juce::jlimit(0.0f, 360.0f, phaseDegrees);

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
    // Convert percent to linear gain (100% = 1.0), apply multiplier
    float gain = (gainValues[bin] / 100.0f) * gainMultiplier;
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

  // Get the invert factor (-1 or 1)
  float invertFactor = feedbackInvert ? -1.0f : 1.0f;

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(feedbackValues.size()) &&
       bin < static_cast<int>(feedbackBuffer.size());
       ++bin) {
    // Convert percent to linear feedback amount (100% = 1.0)
    // Apply multiplier (0-1) to scale the overall feedback
    float feedbackAmount =
        (feedbackValues[bin] / 100.0f) * feedbackMultiplier * invertFactor;

    // Mix feedback buffer with current spectrum
    // new_spectrum = current + previous * feedback
    spectrum[bin] += feedbackBuffer[bin] * feedbackAmount;
  }
}

void SpectralDelayEngine::storeFeedback(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::complex<float>> &feedbackBuffer) {

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(feedbackBuffer.size()); ++bin) {
    // Store current post-Phase spectrum for next frame's feedback
    feedbackBuffer[bin] = spectrum[bin];
  }
}

//==============================================================================
void SpectralDelayEngine::updateDelayValues() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  for (int i = 0; i < currentNumBins; ++i) {
    float freq = binToFrequency(i);
    float delayMs = delayCurve.getValueAtFrequency(freq);

    // Clamp to valid range (0 to 2000ms)
    delayMs = juce::jlimit(0.0f, 2000.0f, delayMs);

    delayValuesMs[i] = delayMs;
  }

  delayNeedsUpdate = false;
}

void SpectralDelayEngine::applyDelay(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::vector<std::complex<float>>> &delayBuffer,
    std::vector<int> &writePos) {

  // Calculate hop time in ms
  float hopTimeMs = static_cast<float>(currentHopSize) /
                    static_cast<float>(currentSampleRate) * 1000.0f;

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(delayValuesMs.size()) &&
       bin < static_cast<int>(delayBuffer.size());
       ++bin) {

    // Calculate delay in frames (apply multiplier)
    float delayMs = delayValuesMs[bin] * delayMultiplier;
    int delayFrames = static_cast<int>(delayMs / hopTimeMs);

    // Clamp to buffer size
    if (delayFrames >= maxDelaySamples) {
      delayFrames = maxDelaySamples - 1;
    }

    // Get write position for this bin
    int wp = writePos[bin];
    int bufferSize = static_cast<int>(delayBuffer[bin].size());

    // Write current value to buffer
    delayBuffer[bin][wp] = spectrum[bin];

    // Read delayed value (circular buffer)
    int readPos = (wp - delayFrames + bufferSize) % bufferSize;
    spectrum[bin] = delayBuffer[bin][readPos];

    // Advance write position
    writePos[bin] = (wp + 1) % bufferSize;
  }
}
