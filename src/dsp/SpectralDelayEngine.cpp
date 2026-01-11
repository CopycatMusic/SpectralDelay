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
  phaseShifts.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);
  phaseNeedsUpdate = true;

  // Initialize gain values cache
  gainValues.resize(static_cast<size_t>(currentNumBins.load()), 100.0f);
  gainNeedsUpdate = true;

  // Initialize feedback values cache and spectrum buffers
  feedbackValues.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);
  feedbackNeedsUpdate = true;
  feedbackSpectrumL.resize(static_cast<size_t>(currentNumBins.load()),
                           std::complex<float>(0.0f, 0.0f));
  feedbackSpectrumR.resize(static_cast<size_t>(currentNumBins.load()),
                           std::complex<float>(0.0f, 0.0f));

  // Initialize delay values cache and per-bin delay buffers
  delayValuesMs.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);
  delayNeedsUpdate = true;

  // Calculate max delay in STFT frames (2000ms at current sample rate)
  // For sample-level delay, we store complex spectrum values per bin
  float maxDelayMs = 2000.0f;
  float hopSizeSec = static_cast<float>(currentFFTSize.load()) /
                     static_cast<float>(currentOverlapFactor.load()) /
                     static_cast<float>(sampleRate);
  maxDelaySamples = static_cast<int>(maxDelayMs / 1000.0f / hopSizeSec) + 1;

  // Allocate delay buffers for each bin
  delayBufferL.resize(static_cast<size_t>(currentNumBins.load()));
  delayBufferR.resize(static_cast<size_t>(currentNumBins.load()));
  delayWritePosL.resize(static_cast<size_t>(currentNumBins.load()), 0);
  delayWritePosR.resize(static_cast<size_t>(currentNumBins.load()), 0);
  for (int bin = 0; bin < currentNumBins.load(); ++bin) {
    delayBufferL[static_cast<size_t>(bin)].resize(
        static_cast<size_t>(maxDelaySamples), std::complex<float>(0.0f, 0.0f));
    delayBufferR[static_cast<size_t>(bin)].resize(
        static_cast<size_t>(maxDelaySamples), std::complex<float>(0.0f, 0.0f));
  }

  // Log memory usage (only in Debug builds)
  float memMB = static_cast<float>(currentNumBins.load()) *
                static_cast<float>(maxDelaySamples) *
                sizeof(std::complex<float>) * 2 / 1024.0f / 1024.0f;
  DBG("=== SpectralDelayEngine Memory Usage ===");
  DBG("FFT Size: " + juce::String(currentFFTSize.load()) +
      ", Bins: " + juce::String(currentNumBins.load()));
  DBG("Max delay frames: " + juce::String(maxDelaySamples));
  DBG("Delay buffer memory: " + juce::String(memMB, 2) + " MB");

  // Initialize input spectrum storage
  inputMagnitudeSpectrum.resize(static_cast<size_t>(currentNumBins.load()),
                                0.0f);

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
  int outputFFTOrder =
      static_cast<int>(std::log2(static_cast<float>(currentFFTSize.load())));
  outputFFT = std::make_unique<juce::dsp::FFT>(outputFFTOrder);
  outputFFTBuffer.resize(static_cast<size_t>(currentFFTSize.load()) * 2, 0.0f);
  outputMagnitudeSpectrum.resize(static_cast<size_t>(currentNumBins.load()),
                                 0.0f);
  outputAccumBuffer.resize(static_cast<size_t>(currentFFTSize.load()), 0.0f);
  outputAccumPos = 0;

  // Create Hann window for output FFT
  outputWindow.resize(static_cast<size_t>(currentFFTSize.load()));
  for (int i = 0; i < currentFFTSize.load(); ++i) {
    outputWindow[static_cast<size_t>(i)] =
        0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                static_cast<float>(i) /
                                static_cast<float>(currentFFTSize.load())));
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
        float normFactor = 1.0f / static_cast<float>(currentFFTSize.load());
        for (int bin = 0; bin < numBins &&
                          bin < static_cast<int>(inputMagnitudeSpectrum.size());
             ++bin) {
          inputMagnitudeSpectrum[static_cast<size_t>(bin)] =
              std::abs(spectrum[bin]) * normFactor;
        }
        // DSP flow depends on gain mode:
        // Pre:  Gain -> (+Feedback) -> [Phase or Delay] -> Stereo ->
        // storeFeedback Post: (+Feedback) -> [Phase or Delay] -> Stereo ->
        // storeFeedback -> Gain
        bool isPostGain = gainPostMode.load();

        // Apply gain only in Pre mode (before feedback)
        if (!isPostGain) {
          applyGainModulation(spectrum, numBins);
        }

        applyFeedback(spectrum, numBins, feedbackSpectrumL);

        // Apply either Phase rotation or Delay buffer based on mode
        if (useDelayMode) {
          applyDelay(spectrum, numBins, delayBufferL, delayWritePosL);
        } else {
          applyPhaseModulation(spectrum, numBins);
        }

        // Apply stereo width (left channel)
        applyStereoWidthChannel(spectrum, numBins, true);

        storeFeedback(spectrum, numBins, feedbackSpectrumL);

        // Apply gain in Post mode (after storeFeedback)
        if (isPostGain) {
          applyGainModulation(spectrum, numBins);
        }
      });

  auto *rightIn = buffer.getReadPointer(1);
  auto *rightOut = buffer.getWritePointer(1);

  stftProcessorR.process(rightIn, rightOut, numSamples,
                         [this](std::complex<float> *spectrum, int numBins) {
                           // DSP flow depends on gain mode (same as left
                           // channel)
                           bool isPostGain = gainPostMode.load();

                           // Apply gain only in Pre mode (before feedback)
                           if (!isPostGain) {
                             applyGainModulation(spectrum, numBins);
                           }

                           applyFeedback(spectrum, numBins, feedbackSpectrumR);

                           if (useDelayMode) {
                             applyDelay(spectrum, numBins, delayBufferR,
                                        delayWritePosR);
                           } else {
                             applyPhaseModulation(spectrum, numBins);
                           }

                           // Apply stereo width (right channel)
                           applyStereoWidthChannel(spectrum, numBins, false);

                           storeFeedback(spectrum, numBins, feedbackSpectrumR);

                           // Apply gain in Post mode (after storeFeedback)
                           if (isPostGain) {
                             applyGainModulation(spectrum, numBins);
                           }
                         });

  // With STFT disabled, buffer already contains the input signal
  // which will be used as the \"wet\" signal in the mix below

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
      outputAccumBuffer[static_cast<size_t>(outputAccumPos)] = outputData[i];
      outputAccumPos++;

      // When buffer is full, perform FFT analysis
      if (outputAccumPos >= currentFFTSize.load()) {
        // Apply window and copy to FFT buffer
        for (int j = 0; j < currentFFTSize.load(); ++j) {
          outputFFTBuffer[static_cast<size_t>(j)] =
              outputAccumBuffer[static_cast<size_t>(j)] *
              outputWindow[static_cast<size_t>(j)];
        }
        // Clear imaginary part
        for (int j = currentFFTSize.load(); j < currentFFTSize.load() * 2;
             ++j) {
          outputFFTBuffer[static_cast<size_t>(j)] = 0.0f;
        }

        // Perform FFT
        outputFFT->performRealOnlyForwardTransform(outputFFTBuffer.data(),
                                                   true);

        // Calculate magnitude spectrum
        auto *spectrum =
            reinterpret_cast<std::complex<float> *>(outputFFTBuffer.data());
        float normFactor = 1.0f / static_cast<float>(currentFFTSize.load());

        {
          std::lock_guard<std::mutex> lock(spectrumMutex);
          for (int bin = 0; bin < currentNumBins.load(); ++bin) {
            outputMagnitudeSpectrum[static_cast<size_t>(bin)] =
                std::abs(spectrum[bin]) * normFactor;
          }
          visualizationMagnitudeSpectrum = outputMagnitudeSpectrum;
          visualizationInputSpectrum = inputMagnitudeSpectrum;
        }

        // Reset accumulator (with overlap - keep half the samples)
        int shiftAmount = currentFFTSize.load() / 2;
        for (int j = 0; j < shiftAmount; ++j) {
          outputAccumBuffer[static_cast<size_t>(j)] =
              outputAccumBuffer[static_cast<size_t>(j + shiftAmount)];
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
    float phaseShift = phaseShifts[static_cast<size_t>(bin)];

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
  if (size == currentFFTSize.load())
    return;

  currentFFTSize = size;

  stftProcessorL.setFFTSize(size);
  stftProcessorR.setFFTSize(size);

  currentNumBins = stftProcessorL.getNumBins();
  currentHopSize = stftProcessorL.getHopSize();

  // Recalculate maxDelaySamples based on new hop size
  float maxDelayMs = 2000.0f;
  float hopSizeSec = static_cast<float>(currentHopSize.load()) /
                     static_cast<float>(currentSampleRate.load());
  maxDelaySamples = static_cast<int>(maxDelayMs / 1000.0f / hopSizeSec) + 1;

  // Resize phase and gain buffers
  phaseShifts.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);
  phaseNeedsUpdate = true;

  gainValues.resize(static_cast<size_t>(currentNumBins.load()), 100.0f);
  gainNeedsUpdate = true;

  // Resize feedback buffers and clear them
  feedbackValues.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);
  feedbackSpectrumL.assign(static_cast<size_t>(currentNumBins.load()),
                           std::complex<float>(0.0f, 0.0f));
  feedbackSpectrumR.assign(static_cast<size_t>(currentNumBins.load()),
                           std::complex<float>(0.0f, 0.0f));
  feedbackNeedsUpdate = true;

  // Resize and clear delay buffers
  delayValuesMs.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);
  delayBufferL.resize(static_cast<size_t>(currentNumBins.load()));
  delayBufferR.resize(static_cast<size_t>(currentNumBins.load()));
  // Reset all write positions to 0
  delayWritePosL.assign(static_cast<size_t>(currentNumBins.load()), 0);
  delayWritePosR.assign(static_cast<size_t>(currentNumBins.load()), 0);
  // Clear and resize each bin's delay buffer
  for (int bin = 0; bin < currentNumBins.load(); ++bin) {
    delayBufferL[static_cast<size_t>(bin)].assign(
        static_cast<size_t>(maxDelaySamples), std::complex<float>(0.0f, 0.0f));
    delayBufferR[static_cast<size_t>(bin)].assign(
        static_cast<size_t>(maxDelaySamples), std::complex<float>(0.0f, 0.0f));
  }
  delayNeedsUpdate = true;

  // Resize input spectrum storage
  inputMagnitudeSpectrum.resize(static_cast<size_t>(currentNumBins.load()),
                                0.0f);

  // Resize output FFT analyzer buffers (critical for UI thread safety)
  int outputFFTOrder =
      static_cast<int>(std::log2(static_cast<float>(currentFFTSize.load())));
  outputFFT = std::make_unique<juce::dsp::FFT>(outputFFTOrder);
  outputFFTBuffer.resize(static_cast<size_t>(currentFFTSize.load()) * 2, 0.0f);
  outputMagnitudeSpectrum.assign(static_cast<size_t>(currentNumBins.load()),
                                 0.0f);

  {
    std::lock_guard<std::mutex> lock(spectrumMutex);
    visualizationMagnitudeSpectrum.assign(
        static_cast<size_t>(currentNumBins.load()), 0.0f);
    visualizationInputSpectrum.assign(
        static_cast<size_t>(currentNumBins.load()), 0.0f);
  }

  outputAccumBuffer.assign(static_cast<size_t>(currentFFTSize.load()), 0.0f);
  outputAccumPos = 0;

  // Recreate Hann window for output FFT
  outputWindow.resize(static_cast<size_t>(currentFFTSize.load()));
  for (int i = 0; i < currentFFTSize.load(); ++i) {
    outputWindow[static_cast<size_t>(i)] =
        0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                static_cast<float>(i) /
                                static_cast<float>(currentFFTSize.load())));
  }

  // Update dry signal delay to match new STFT latency
  int latency = stftProcessorL.getLatencyInSamples();
  dryDelayL.setDelay(static_cast<float>(latency));
  dryDelayR.setDelay(static_cast<float>(latency));
}

void SpectralDelayEngine::setOverlapFactor(int factor) {
  if (factor == currentOverlapFactor.load())
    return;

  currentOverlapFactor = factor;

  stftProcessorL.setOverlapFactor(factor);
  stftProcessorR.setOverlapFactor(factor);

  currentHopSize = stftProcessorL.getHopSize();
  phaseNeedsUpdate = true;
}

//==============================================================================
const float *SpectralDelayEngine::getMagnitudeSpectrum() const {
  std::lock_guard<std::mutex> lock(spectrumMutex);
  return visualizationMagnitudeSpectrum.data();
}

const float *SpectralDelayEngine::getInputMagnitudeSpectrum() const {
  std::lock_guard<std::mutex> lock(spectrumMutex);
  return visualizationInputSpectrum.data();
}

int SpectralDelayEngine::getLatencyInSamples() const {
  return stftProcessorL.getLatencyInSamples();
}

//==============================================================================
void SpectralDelayEngine::updatePhaseShifts() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  int bins = currentNumBins.load();
  for (int i = 0; i < bins; ++i) {
    float freq = binToFrequency(i);
    float phaseDegrees = phaseCurve.getValueAtFrequency(freq);

    // Clamp degrees to valid range (0 to 360)
    phaseDegrees = juce::jlimit(0.0f, 360.0f, phaseDegrees);

    // Convert degrees to radians
    float phaseRadians = phaseDegrees * juce::MathConstants<float>::pi / 180.0f;

    phaseShifts[static_cast<size_t>(i)] = phaseRadians;
  }

  phaseNeedsUpdate = false;
}

float SpectralDelayEngine::binToFrequency(int binIndex) const {
  float fftSize = static_cast<float>(currentFFTSize.load());
  if (fftSize < 1.0f)
    return 0.0f;

  return static_cast<float>(binIndex) *
         static_cast<float>(currentSampleRate.load()) / fftSize;
}

//==============================================================================
void SpectralDelayEngine::updateGainValues() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  int bins = currentNumBins.load();
  for (int i = 0; i < bins; ++i) {
    float freq = binToFrequency(i);
    float gain = gainCurve.getValueAtFrequency(freq);

    // Clamp to valid range (0 to 200%)
    gain = juce::jlimit(0.0f, 200.0f, gain);

    gainValues[static_cast<size_t>(i)] = gain;
  }

  gainNeedsUpdate = false;
}

void SpectralDelayEngine::applyGainModulation(std::complex<float> *spectrum,
                                              int numBins) {
  for (int bin = 0; bin < numBins && bin < static_cast<int>(gainValues.size());
       ++bin) {
    // Convert percent to linear gain (100% = 1.0), apply multiplier
    float gain =
        (gainValues[static_cast<size_t>(bin)] / 100.0f) * gainMultiplier;
    spectrum[bin] *= gain;
  }
}

//==============================================================================
void SpectralDelayEngine::updateFeedbackValues() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  int bins = currentNumBins.load();
  for (int i = 0; i < bins; ++i) {
    float freq = binToFrequency(i);
    float feedback = feedbackCurve.getValueAtFrequency(freq);

    // Clamp to valid range (0 to 100%)
    feedback = juce::jlimit(0.0f, 100.0f, feedback);

    feedbackValues[static_cast<size_t>(i)] = feedback;
  }

  feedbackNeedsUpdate = false;
}

void SpectralDelayEngine::applyFeedback(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::complex<float>> &feedbackBuffer) {

  // Get the invert factor (-1 or 1)
  float invertFactor = feedbackInvert.load() ? -1.0f : 1.0f;

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(feedbackValues.size()) &&
       bin < static_cast<int>(feedbackBuffer.size());
       ++bin) {
    // Convert percent to linear feedback amount (100% = 1.0)
    // Apply multiplier (0-1) to scale the overall feedback
    float feedbackAmount = (feedbackValues[static_cast<size_t>(bin)] / 100.0f) *
                           feedbackMultiplier.load() * invertFactor;

    // Mix feedback buffer with current spectrum
    // new_spectrum = current + previous * feedback
    spectrum[bin] += feedbackBuffer[static_cast<size_t>(bin)] * feedbackAmount;
  }
}

void SpectralDelayEngine::storeFeedback(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::complex<float>> &feedbackBuffer) {

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(feedbackBuffer.size()); ++bin) {
    // Store current post-Phase spectrum for next frame's feedback
    feedbackBuffer[static_cast<size_t>(bin)] = spectrum[bin];
  }
}

//==============================================================================
void SpectralDelayEngine::updateDelayValues() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  int bins = currentNumBins.load();
  for (int i = 0; i < bins; ++i) {
    float freq = binToFrequency(i);
    float delayMs = delayCurve.getValueAtFrequency(freq);

    // Clamp to valid range (0 to 2000ms)
    delayMs = juce::jlimit(0.0f, 2000.0f, delayMs);

    delayValuesMs[static_cast<size_t>(i)] = delayMs;
  }

  delayNeedsUpdate = false;
}

void SpectralDelayEngine::applyDelay(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::vector<std::complex<float>>> &delayBuffer,
    std::vector<int> &writePos) {

  // Calculate hop time in ms - defensive check for zero sample rate or hop size
  float sr = static_cast<float>(currentSampleRate.load());
  float hop = static_cast<float>(currentHopSize.load());
  int fftSize = currentFFTSize.load();

  if (sr <= 0.0f || hop <= 0.0f || fftSize <= 0)
    return;

  float hopTimeMs = (hop / sr) * 1000.0f;
  bool useHighPrecision = highPrecisionDelay.load();

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(delayValuesMs.size()) &&
       bin < static_cast<int>(delayBuffer.size());
       ++bin) {

    // Calculate delay in frames (apply multiplier)
    float delayMs =
        delayValuesMs[static_cast<size_t>(bin)] * delayMultiplier.load();
    float delayFramesFloat = delayMs / hopTimeMs;

    // Separate integer and fractional parts
    int delayFramesInt = static_cast<int>(delayFramesFloat);
    float fractionalFrames =
        delayFramesFloat - static_cast<float>(delayFramesInt);

    // Clamp to buffer size
    if (delayFramesInt >= maxDelaySamples) {
      delayFramesInt = maxDelaySamples - 1;
      fractionalFrames = 0.0f; // No fractional part if clamped
    }

    // Get write position for this bin
    size_t binIdx = static_cast<size_t>(bin);
    int wp = writePos[binIdx];
    int bufferSize = static_cast<int>(delayBuffer[binIdx].size());

    // Write current value to buffer
    delayBuffer[binIdx][static_cast<size_t>(wp)] = spectrum[bin];

    // Read delayed value (circular buffer)
    int readPos = (wp - delayFramesInt + bufferSize) % bufferSize;
    std::complex<float> delayedSample =
        delayBuffer[binIdx][static_cast<size_t>(readPos)];

    // High precision mode: apply phase rotation for fractional frame
    // compensation
    if (useHighPrecision && fractionalFrames > 0.0001f) {
      // Calculate frequency for this bin
      float binFreq =
          static_cast<float>(bin) * sr / static_cast<float>(fftSize);

      // Calculate time offset for fractional frames (in seconds)
      float fractionalDelaySec = fractionalFrames * hop / sr;

      // Phase offset: φ = -2π × f × Δt
      float phaseOffset =
          -2.0f * juce::MathConstants<float>::pi * binFreq * fractionalDelaySec;

      // Apply phase rotation
      std::complex<float> rotator(std::cos(phaseOffset), std::sin(phaseOffset));
      delayedSample *= rotator;
    }

    spectrum[bin] = delayedSample;

    // Advance write position
    writePos[binIdx] = (wp + 1) % bufferSize;
  }
}

// ==============================================================================
float SpectralDelayEngine::getPhaseOffset(int binIndex) const {
  // Golden ratio based pseudo-random hash
  // Produces deterministic values in range -1 to +1 for each bin
  const float goldenRatio = 0.618033988749895f;
  float hash = std::fmod(static_cast<float>(binIndex) * goldenRatio, 1.0f);
  return hash * 2.0f - 1.0f;
}

void SpectralDelayEngine::applyStereoWidth(std::complex<float> *spectrumL,
                                           std::complex<float> *spectrumR,
                                           int numBins) {
  // This version is for single-channel processing with isLeftChannel flag
  // spectrumR is ignored when processing per-channel
}

// Per-channel stereo width application
void SpectralDelayEngine::applyStereoWidthChannel(std::complex<float> *spectrum,
                                                  int numBins,
                                                  bool isLeftChannel) {
  float amount = stereoAmount.load();
  if (amount <= 0.0f)
    return;

  float lowCutFreq = stereoLowCut.load();
  const float transitionWidth = 100.0f;  // Hz for smooth transition
  const float maxPhaseShift = 1.047198f; // π/3 radians = 60 degrees

  // Left channel gets negative phase shift, right gets positive
  float channelSign = isLeftChannel ? -1.0f : 1.0f;

  for (int bin = 1; bin < numBins; ++bin) {
    float freq = binToFrequency(bin);

    // Skip bins below low cut frequency
    if (freq < lowCutFreq)
      continue;

    // Calculate smooth transition factor
    float factor = 1.0f;
    if (freq < lowCutFreq + transitionWidth) {
      // Smoothstep transition
      float t = (freq - lowCutFreq) / transitionWidth;
      factor = t * t * (3.0f - 2.0f * t);
    }

    // Get pseudo-random phase offset for this bin (-1 to +1)
    float randomOffset = getPhaseOffset(bin);

    // Calculate final phase shift (with channel sign)
    float phaseShift =
        randomOffset * amount * factor * maxPhaseShift * channelSign;

    // Apply phase rotation using complex multiplication
    float cosVal = std::cos(phaseShift);
    float sinVal = std::sin(phaseShift);

    float real = spectrum[bin].real();
    float imag = spectrum[bin].imag();
    spectrum[bin] = std::complex<float>(real * cosVal - imag * sinVal,
                                        real * sinVal + imag * cosVal);
  }
}
