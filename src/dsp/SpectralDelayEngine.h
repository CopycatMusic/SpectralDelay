#pragma once

#include "../utils/ParameterCurve.h"
#include "STFTProcessor.h"
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <mutex>

//==============================================================================
/**
 * Spectral phase modulation engine - applies frequency-dependent phase shifts.
 * Similar to Kilohearts Disperser effect.
 */
class SpectralDelayEngine {
public:
  SpectralDelayEngine();
  ~SpectralDelayEngine();

  void prepare(double sampleRate, int maxBlockSize);
  void reset();
  void process(juce::AudioBuffer<float> &buffer);

  //==========================================================================
  // Parameter curve access for UI editing

  /**
   * Get phase curve for editing.
   */
  ParameterCurve &getPhaseCurve() { return phaseCurve; }
  const ParameterCurve &getPhaseCurve() const { return phaseCurve; }

  /**
   * Get gain curve for editing.
   */
  ParameterCurve &getGainCurve() { return gainCurve; }
  const ParameterCurve &getGainCurve() const { return gainCurve; }

  /**
   * Get the feedback curve for direct editing.
   */
  ParameterCurve &getFeedbackCurve() { return feedbackCurve; }
  const ParameterCurve &getFeedbackCurve() const { return feedbackCurve; }

  /**
   * Get the delay curve for direct editing.
   */
  ParameterCurve &getDelayCurve() { return delayCurve; }
  const ParameterCurve &getDelayCurve() const { return delayCurve; }

  /**
   * Mark phase curve as needing update (call after editing points).
   */
  void markPhaseNeedsUpdate() { phaseNeedsUpdate = true; }

  /**
   * Mark gain curve as needing update (call after editing points).
   */
  void markGainNeedsUpdate() { gainNeedsUpdate = true; }

  /**
   * Mark feedback curve as needing update (call after editing points).
   */
  void markFeedbackNeedsUpdate() { feedbackNeedsUpdate = true; }

  /**
   * Mark delay curve as needing update (call after editing points).
   */
  void markDelayNeedsUpdate() { delayNeedsUpdate = true; }

  /**
   * Set delay mode (false = Phase mode, true = Delay mode).
   */
  void setDelayMode(bool useDelay) { useDelayMode = useDelay; }

  /**
   * Get current delay mode.
   */
  bool isDelayMode() const { return useDelayMode.load(); }

  /**
   * Set feedback invert mode (phase inversion).
   */
  void setFeedbackInvert(bool invert) { feedbackInvert = invert; }
  bool isFeedbackInvert() const { return feedbackInvert.load(); }

  /**
   * Set feedback multiplier (0-100%).
   */
  void setFeedbackMultiplier(float mult) { feedbackMultiplier = mult / 100.0f; }
  float getFeedbackMultiplier() const { return feedbackMultiplier * 100.0f; }

  /**
   * Set gain multiplier (0-100%).
   */
  void setGainMultiplier(float mult) { gainMultiplier = mult / 100.0f; }
  float getGainMultiplier() const { return gainMultiplier * 100.0f; }

  /**
   * Set gain post mode (false = Pre, true = Post).
   * Pre: Gain is applied before feedback (only affects input)
   * Post: Gain is applied after all processing (affects entire output including
   * feedback)
   */
  void setGainPostMode(bool post) { gainPostMode = post; }
  bool isGainPostMode() const { return gainPostMode.load(); }

  /**
   * Set delay multiplier (0-100%).
   */
  void setDelayMultiplier(float mult) { delayMultiplier = mult / 100.0f; }
  float getDelayMultiplier() const { return delayMultiplier * 100.0f; }

  /**
   * Set stereo width amount (0-100%).
   */
  void setStereoAmount(float amount) { stereoAmount = amount / 100.0f; }
  float getStereoAmount() const { return stereoAmount * 100.0f; }

  /**
   * Set stereo low cut frequency (Hz).
   */
  void setStereoLowCut(float freq) { stereoLowCut = freq; }
  float getStereoLowCut() const { return stereoLowCut; }

  /**
   * Set high precision delay mode (uses phase interpolation for sub-frame
   * accuracy).
   */
  void setHighPrecisionDelay(bool enabled) { highPrecisionDelay = enabled; }
  bool isHighPrecisionDelay() const { return highPrecisionDelay.load(); }

  /**
   * Set dry/wet mix (0.0 = fully dry, 1.0 = fully wet).
   */
  void setMix(float dryWet);

  //==========================================================================
  // FFT settings

  void setFFTSize(int size);
  void setOverlapFactor(int factor);

  //==========================================================================
  // Visualization data access

  const float *getMagnitudeSpectrum() const;
  const float *getInputMagnitudeSpectrum() const;
  int getNumBins() const { return currentNumBins.load(); }
  int getLatencyInSamples() const;
  double getSampleRate() const { return currentSampleRate.load(); }

private:
  void updatePhaseShifts();
  void updateGainValues();
  void updateFeedbackValues();
  void updateDelayValues();
  float binToFrequency(int binIndex) const;
  void applyPhaseModulation(std::complex<float> *spectrum, int numBins);
  void applyGainModulation(std::complex<float> *spectrum, int numBins);
  void applyFeedback(std::complex<float> *spectrum, int numBins,
                     std::vector<std::complex<float>> &feedbackBuffer);
  void applyDelay(std::complex<float> *spectrum, int numBins,
                  std::vector<std::vector<std::complex<float>>> &delayBuffer,
                  std::vector<int> &writePos);
  void storeFeedback(std::complex<float> *spectrum, int numBins,
                     std::vector<std::complex<float>> &feedbackBuffer);
  void applyStereoWidth(std::complex<float> *spectrumL,
                        std::complex<float> *spectrumR, int numBins);
  void applyStereoWidthChannel(std::complex<float> *spectrum, int numBins,
                               bool isLeftChannel);
  float getPhaseOffset(int binIndex) const;

  // STFT processors for each channel
  STFTProcessor stftProcessorL;
  STFTProcessor stftProcessorR;

  // Parameter curve for phase distribution
  ParameterCurve phaseCurve;

  // Cached phase shifts per bin
  std::vector<float> phaseShifts;
  std::atomic<bool> phaseNeedsUpdate{true};

  // Parameter curve for gain distribution
  ParameterCurve gainCurve;

  // Cached gain values per bin (in percent, 100 = unity gain)
  std::vector<float> gainValues;
  std::atomic<bool> gainNeedsUpdate{true};

  // Parameter curve for feedback distribution
  ParameterCurve feedbackCurve;

  // Cached feedback values per bin (0-100%)
  std::vector<float> feedbackValues;
  std::atomic<bool> feedbackNeedsUpdate{true};

  // Feedback spectrum buffers (store post-Phase output for next frame)
  std::vector<std::complex<float>> feedbackSpectrumL;
  std::vector<std::complex<float>> feedbackSpectrumR;

  // Feedback invert (phase inversion) and multiplier
  std::atomic<bool> feedbackInvert{false};
  std::atomic<float> feedbackMultiplier{1.0f}; // 0.0-1.0 (stored as ratio)

  // Gain multiplier (default 100%)
  std::atomic<float> gainMultiplier{1.0f};

  // Gain post mode (false = Pre, true = Post)
  std::atomic<bool> gainPostMode{false};

  // Delay multiplier (default 20%)
  std::atomic<float> delayMultiplier{0.2f};

  // Stereo width parameters
  std::atomic<float> stereoAmount{0.0f};   // 0.0-1.0 (stored as ratio)
  std::atomic<float> stereoLowCut{150.0f}; // Hz

  // High precision delay mode (uses phase interpolation for sub-frame accuracy)
  std::atomic<bool> highPrecisionDelay{false};

  // Parameter curve for delay distribution
  ParameterCurve delayCurve;

  // Cached delay values per bin (in milliseconds)
  std::vector<float> delayValuesMs;
  std::atomic<bool> delayNeedsUpdate{true};

  // Per-bin delay buffers (circular buffers for each bin, each channel)
  // delayBufferL[bin] = circular buffer of complex samples
  std::vector<std::vector<std::complex<float>>> delayBufferL;
  std::vector<std::vector<std::complex<float>>> delayBufferR;
  std::vector<int> delayWritePosL; // Write position for each bin (L)
  std::vector<int> delayWritePosR; // Write position for each bin (R)
  int maxDelaySamples = 0;         // Max delay in STFT frames

  // Dry signal storage for mixing
  juce::AudioBuffer<float> dryBuffer;

  // Delay lines for dry signal compensation (one per channel)
  // Max size must match max FFT size (16384) to properly compensate for latency
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      dryDelayL{16384};
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      dryDelayR{16384};

  // Mix parameters
  std::atomic<float> mixAmount{0.5f};

  // Current settings
  std::atomic<double> currentSampleRate{44100.0};
  std::atomic<int> currentFFTSize{2048};
  std::atomic<int> currentOverlapFactor{4};
  std::atomic<int> currentHopSize{512};
  std::atomic<int> currentNumBins{1025};

  // For thread safety
  juce::SpinLock paramLock;

  // Output spectrum analyzer (analyzes signal after mix)
  std::unique_ptr<juce::dsp::FFT> outputFFT;
  std::vector<float> outputFFTBuffer;
  std::vector<float> outputWindow;
  std::vector<float> outputMagnitudeSpectrum;
  std::vector<float> visualizationMagnitudeSpectrum; // Copy for UI thread
  std::vector<float> inputMagnitudeSpectrum;         // Captures input
  std::vector<float> visualizationInputSpectrum;     // Copy for UI thread
  mutable std::mutex spectrumMutex; // Protects visualization spectrums

  // Delay mode (false = Phase rotation, true = Frame delay)
  std::atomic<bool> useDelayMode{true};

  std::vector<float> outputAccumBuffer; // Accumulated samples for FFT
  int outputAccumPos = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralDelayEngine)
};
