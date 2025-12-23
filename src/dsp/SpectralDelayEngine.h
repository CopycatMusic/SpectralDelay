#pragma once

#include "../utils/ParameterCurve.h"
#include "STFTProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

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
  bool isDelayMode() const { return useDelayMode; }

  /**
   * Set feedback invert mode (phase inversion).
   */
  void setFeedbackInvert(bool invert) { feedbackInvert = invert; }
  bool isFeedbackInvert() const { return feedbackInvert; }

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
   * Set delay multiplier (0-100%).
   */
  void setDelayMultiplier(float mult) { delayMultiplier = mult / 100.0f; }
  float getDelayMultiplier() const { return delayMultiplier * 100.0f; }

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
  int getNumBins() const;
  int getLatencyInSamples() const;
  double getSampleRate() const { return currentSampleRate; }

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

  // STFT processors for each channel
  STFTProcessor stftProcessorL;
  STFTProcessor stftProcessorR;

  // Parameter curve for phase distribution
  ParameterCurve phaseCurve;

  // Cached phase shifts per bin
  std::vector<float> phaseShifts;
  bool phaseNeedsUpdate = true;

  // Parameter curve for gain distribution
  ParameterCurve gainCurve;

  // Cached gain values per bin (in percent, 100 = unity gain)
  std::vector<float> gainValues;
  bool gainNeedsUpdate = true;

  // Parameter curve for feedback distribution
  ParameterCurve feedbackCurve;

  // Cached feedback values per bin (0-100%)
  std::vector<float> feedbackValues;
  bool feedbackNeedsUpdate = true;

  // Feedback spectrum buffers (store post-Phase output for next frame)
  std::vector<std::complex<float>> feedbackSpectrumL;
  std::vector<std::complex<float>> feedbackSpectrumR;

  // Feedback invert (phase inversion) and multiplier
  bool feedbackInvert = false;
  float feedbackMultiplier = 1.0f; // 0.0-1.0 (stored as ratio)

  // Gain multiplier (default 100%)
  float gainMultiplier = 1.0f;

  // Delay multiplier (default 20%)
  float delayMultiplier = 0.2f;

  // Parameter curve for delay distribution
  ParameterCurve delayCurve;

  // Cached delay values per bin (in milliseconds)
  std::vector<float> delayValuesMs;
  bool delayNeedsUpdate = true;

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
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      dryDelayL{8192};
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      dryDelayR{8192};

  // Mix parameters
  float mixAmount = 0.5f;

  // Current settings
  double currentSampleRate = 44100.0;
  int currentFFTSize = 2048;
  int currentOverlapFactor = 4;
  int currentHopSize = 512;
  int currentNumBins = 1025;

  // For thread safety
  juce::SpinLock paramLock;

  // Input spectrum for visualization (before DSP processing)
  std::vector<float> inputMagnitudeSpectrum;

  // Delay mode (false = Phase rotation, true = Frame delay)
  bool useDelayMode = false;

  // Output spectrum analyzer (analyzes signal after mix)
  std::unique_ptr<juce::dsp::FFT> outputFFT;
  std::vector<float> outputFFTBuffer;
  std::vector<float> outputWindow;
  std::vector<float> outputMagnitudeSpectrum;
  std::vector<float> outputAccumBuffer; // Accumulated samples for FFT
  int outputAccumPos = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralDelayEngine)
};
