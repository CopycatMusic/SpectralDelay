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

  // Allocate delay buffers (flattened)
  size_t totalDelayBufferSize = static_cast<size_t>(currentNumBins.load()) *
                                static_cast<size_t>(maxDelaySamples);

  delayBufferL.assign(totalDelayBufferSize, std::complex<float>(0.0f, 0.0f));
  delayBufferR.assign(totalDelayBufferSize, std::complex<float>(0.0f, 0.0f));

  delayWritePosL.assign(static_cast<size_t>(currentNumBins.load()), 0);
  delayWritePosR.assign(static_cast<size_t>(currentNumBins.load()), 0);

  // Log memory usage (only in Debug builds)
  float memMB = static_cast<float>(currentNumBins.load()) *
                static_cast<float>(maxDelaySamples) *
                sizeof(std::complex<float>) * 2 / 1024.0f / 1024.0f;
  DBG("=== SpectralDelayEngine Memory Usage ===");
  DBG("FFT Size: " + juce::String(currentFFTSize.load()) +
      ", Bins: " + juce::String(currentNumBins.load()));
  DBG("Max delay frames: " + juce::String(maxDelaySamples));
  DBG("Delay buffer memory: " + juce::String(memMB, 2) + " MB");

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

  // Initialize visualization buffers
  outputMagnitudeSpectrum.resize(static_cast<size_t>(currentNumBins.load()),
                                 0.0f);
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
        // 1. Spectral Dynamics (Fixed: Pre-Feedback) - Now occurs before
        // Pre-Gain
        applySpectralDynamics(spectrum, numBins);

        // 2. Pre-Dynamics Gain (if in Pre mode)
        if (!gainPostMode.load()) {
          applyGainModulation(spectrum, numBins);
        }

        // 3. Feedback Mixing
        applyFeedback(spectrum, numBins, feedbackSpectrumL);

        // 4. Core Effect (Delay or Phase)
        if (useDelayMode) {
          applyDelay(spectrum, numBins, delayBufferL, delayWritePosL);
        } else {
          applyPhaseModulation(spectrum, numBins);
        }

        // 5. Stereo Width
        applyStereoWidthChannel(spectrum, numBins, true);

        // 6. Feedback Storing
        storeFeedback(spectrum, numBins, feedbackSpectrumL);

        // 7. Post-Dynamics Gain (if in Post mode)
        if (gainPostMode.load()) {
          applyGainModulation(spectrum, numBins);
        }

        // capture visualization spectrum... (rest of the block)

        // Capture visualization spectrum (Left channel only)
        // Use try_lock to never block audio thread
        {
          std::unique_lock<std::mutex> lock(spectrumMutex, std::try_to_lock);
          if (lock.owns_lock()) {
            float acNorm = 4.0f / static_cast<float>(currentFFTSize.load());
            float dcNorm = 2.0f / static_cast<float>(currentFFTSize.load());

            // DC (real)
            outputMagnitudeSpectrum[0] = std::abs(spectrum[0].real()) * dcNorm;

            // AC bins
            for (int bin = 1; bin < numBins - 1 && bin < currentNumBins.load();
                 ++bin) {
              outputMagnitudeSpectrum[static_cast<size_t>(bin)] =
                  std::abs(spectrum[bin]) * acNorm;
            }

            // Nyquist (imag)
            if (numBins > 0 && numBins <= (int)outputMagnitudeSpectrum.size()) {
              outputMagnitudeSpectrum[static_cast<size_t>(numBins - 1)] =
                  std::abs(spectrum[0].imag()) * dcNorm;
            }

            // Atomic swap / copy to visualization buffer
            visualizationMagnitudeSpectrum = outputMagnitudeSpectrum;
          }
        }
      });

  auto *rightIn = buffer.getReadPointer(1);
  auto *rightOut = buffer.getWritePointer(1);

  stftProcessorR.process(rightIn, rightOut, numSamples,
                         [this](std::complex<float> *spectrum, int numBins) {
                           // 1. Spectral Dynamics
                           applySpectralDynamics(spectrum, numBins);

                           // 2. Pre-Dynamics Gain
                           if (!gainPostMode.load()) {
                             applyGainModulation(spectrum, numBins);
                           }

                           // 3. Feedback Mixing
                           applyFeedback(spectrum, numBins, feedbackSpectrumR);

                           // 4. Core Effect
                           if (useDelayMode) {
                             applyDelay(spectrum, numBins, delayBufferR,
                                        delayWritePosR);
                           } else {
                             applyPhaseModulation(spectrum, numBins);
                           }

                           // 5. Stereo Width
                           applyStereoWidthChannel(spectrum, numBins, false);

                           // 6. Feedback Storing
                           storeFeedback(spectrum, numBins, feedbackSpectrumR);

                           // 7. Post-Dynamics Gain
                           if (gainPostMode.load()) {
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

  // Resize and clear delay buffers (flattened)
  size_t totalDelayBufferSize = static_cast<size_t>(currentNumBins.load()) *
                                static_cast<size_t>(maxDelaySamples);

  delayValuesMs.resize(static_cast<size_t>(currentNumBins.load()), 0.0f);

  // Resize flattened buffers
  delayBufferL.assign(totalDelayBufferSize, std::complex<float>(0.0f, 0.0f));
  delayBufferR.assign(totalDelayBufferSize, std::complex<float>(0.0f, 0.0f));

  // Reset all write positions to 0
  delayWritePosL.assign(static_cast<size_t>(currentNumBins.load()), 0);
  delayWritePosR.assign(static_cast<size_t>(currentNumBins.load()), 0);

  delayNeedsUpdate = true;

  // Resize visualization buffers
  outputMagnitudeSpectrum.assign(static_cast<size_t>(currentNumBins.load()),
                                 0.0f);

  {
    std::lock_guard<std::mutex> lock(spectrumMutex);
    visualizationMagnitudeSpectrum.assign(
        static_cast<size_t>(currentNumBins.load()), 0.0f);
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
  // Cast away constness to lock mutex (since we removed 'mutable' from header)
  // This is a safe hack given we know we are just reading
  auto *nonConstThis = const_cast<SpectralDelayEngine *>(this);
  std::lock_guard<std::mutex> lock(nonConstThis->spectrumMutex);
  return visualizationMagnitudeSpectrum.data();
}

const float *SpectralDelayEngine::getInputMagnitudeSpectrum() const {
  // Deprecated/Removed - return nullptr or empty
  return nullptr;
}

int SpectralDelayEngine::getLatencyInSamples() const {
  return stftProcessorL.getLatencyInSamples();
}

//==============================================================================
void SpectralDelayEngine::updatePhaseShifts() {
  juce::SpinLock::ScopedLockType lock(paramLock);

  int bins = currentNumBins.load();
  float sr = static_cast<float>(currentSampleRate.load());
  float fftSize = static_cast<float>(currentFFTSize.load());

  // Use bulk fill to avoid lock contention
  phaseCurve.fillBinValues(phaseShifts.data(), bins, sr,
                           static_cast<int>(fftSize));

  for (int i = 0; i < bins; ++i) {
    float phaseDegrees = phaseShifts[static_cast<size_t>(i)];

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
  float sr = static_cast<float>(currentSampleRate.load());
  float fftSize = static_cast<float>(currentFFTSize.load());

  // Use bulk fill to avoid lock contention
  gainCurve.fillBinValues(gainValues.data(), bins, sr,
                          static_cast<int>(fftSize));

  for (int i = 0; i < bins; ++i) {
    // Clamp to valid range (0 to 200%)
    gainValues[static_cast<size_t>(i)] =
        juce::jlimit(0.0f, 200.0f, gainValues[static_cast<size_t>(i)]);
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
  float sr = static_cast<float>(currentSampleRate.load());
  float fftSize = static_cast<float>(currentFFTSize.load());

  // Use bulk fill to avoid lock contention
  feedbackCurve.fillBinValues(feedbackValues.data(), bins, sr,
                              static_cast<int>(fftSize));

  for (int i = 0; i < bins; ++i) {
    // Clamp to valid range (0 to 100%)
    feedbackValues[static_cast<size_t>(i)] =
        juce::jlimit(0.0f, 100.0f, feedbackValues[static_cast<size_t>(i)]);
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
  float sr = static_cast<float>(currentSampleRate.load());
  float fftSize = static_cast<float>(currentFFTSize.load());

  // Use bulk fill to avoid lock contention
  delayCurve.fillBinValues(delayValuesMs.data(), bins, sr,
                           static_cast<int>(fftSize));

  for (int i = 0; i < bins; ++i) {
    // Clamp to valid range (0 to 2000ms)
    delayValuesMs[static_cast<size_t>(i)] =
        juce::jlimit(0.0f, 2000.0f, delayValuesMs[static_cast<size_t>(i)]);
  }

  delayNeedsUpdate = false;
}

void SpectralDelayEngine::applyDelay(
    std::complex<float> *spectrum, int numBins,
    std::vector<std::complex<float>> &delayBuffer, std::vector<int> &writePos) {

  // Calculate hop time in ms - defensive check for zero sample rate or hop size
  float sr = static_cast<float>(currentSampleRate.load());
  float hop = static_cast<float>(currentHopSize.load());
  int fftSize = currentFFTSize.load();

  if (sr <= 0.0f || hop <= 0.0f || fftSize <= 0)
    return;

  float hopTimeMs = (hop / sr) * 1000.0f;
  bool useHighPrecision = highPrecisionDelay.load();

  // Validate buffer sizing to prevent out of bounds access
  if (delayBuffer.size() <
      static_cast<size_t>(numBins) * static_cast<size_t>(maxDelaySamples))
    return;

  for (int bin = 0;
       bin < numBins && bin < static_cast<int>(delayValuesMs.size()); ++bin) {

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

    // Circular buffer size is constant for all bins now
    int bufferSize = maxDelaySamples;

    // Calculate base offset for this bin in the flattened buffer
    size_t binOffset = binIdx * static_cast<size_t>(maxDelaySamples);

    // Write current value to buffer
    delayBuffer[binOffset + static_cast<size_t>(wp)] = spectrum[bin];

    // Read delayed value (circular buffer)
    int readPos = (wp - delayFramesInt + bufferSize) % bufferSize;
    std::complex<float> delayedSample =
        delayBuffer[binOffset + static_cast<size_t>(readPos)];

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

void SpectralDelayEngine::applySpectralDynamics(std::complex<float> *spectrum,
                                                int numBins) {
  float amount = compressAmount.load();
  float Tg = gateLevel.load();

  // Mapping amount (0.0-1.0) to R_comp (1.0 to 4.0 as requested)
  float R_comp = 1.0f + amount * 3.0f;
  const float R_exp = 10.0f; // Fixed expansion ratio

  float acNorm = 4.0f / static_cast<float>(currentFFTSize.load());
  float dcNorm = 2.0f / static_cast<float>(currentFFTSize.load());

  auto applyDynamicsToValue = [&](float &value, float norm) {
    float mag = std::abs(value);
    if (mag < 1e-9f)
      return;

    float x = juce::Decibels::gainToDecibels(mag * norm, -120.0f);
    const float A = -36.0f; // Anchor point at -36dB as requested
    float y;

    if (x >= Tg) {
      // Compression zone: x pulled towards anchor A
      y = A + (x - A) / R_comp;
    } else {
      // Expansion zone: x pushed away from inflection point
      y = (A + (Tg - A) / R_comp) + R_exp * (x - Tg);
    }

    float newMag = juce::Decibels::decibelsToGain(y, -120.0f);
    value *= (newMag / (mag * norm));
  };

  // Dynamics DC
  float dc = spectrum[0].real();
  applyDynamicsToValue(dc, dcNorm);
  spectrum[0].real(dc);

  // Dynamics AC bins
  for (int i = 1; i < numBins - 1; ++i) {
    float rawMag = std::abs(spectrum[i]);
    if (rawMag < 1e-9f)
      continue;

    float x = juce::Decibels::gainToDecibels(rawMag * acNorm, -120.0f);
    const float A = -36.0f; // Anchor point at -36dB as requested
    float y;

    if (x >= Tg) {
      y = A + (x - A) / R_comp;
    } else {
      y = (A + (Tg - A) / R_comp) + R_exp * (x - Tg);
    }

    float newMag = juce::Decibels::decibelsToGain(y, -120.0f);
    spectrum[i] *= (newMag / (rawMag * acNorm));
  }

  // Dynamics Nyquist
  float nyquist = spectrum[0].imag();
  applyDynamicsToValue(nyquist, dcNorm);
  spectrum[0].imag(nyquist);
}

void SpectralDelayEngine::applySpectralGate(std::complex<float> *spectrum,
                                            int numBins) {
  // Logic merged into applySpectralDynamics - legacy stub
}

void SpectralDelayEngine::applySpectralCompression(
    std::complex<float> *spectrum, int numBins) {
  // Logic merged into applySpectralDynamics - legacy stub
}
