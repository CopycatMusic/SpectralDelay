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
  // Phase modulation parameters

  /**
   * Set constant phase shift for all frequency bins (0 to 2π).
   */
  void setConstantPhase(float phaseRadians);

  /**
   * Set linear curve parameters for phase shift.
   * @param slope Phase change rate with frequency
   * @param offset Base phase in radians
   */
  void setLinearCurve(float slope, float offset);

  /**
   * Set dry/wet mix (0.0 = fully dry, 1.0 = fully wet).
   */
  void setMix(float dryWet);

  //==========================================================================
  // Gain modulation parameters

  /**
   * Set constant gain for all frequency bins (0-200%).
   * 100% = unity gain (no change)
   */
  void setConstantGain(float gainPercent);

  /**
   * Set linear curve parameters for gain.
   * @param slope Gain change per decade of frequency
   * @param offset Base gain in percent
   */
  void setLinearGainCurve(float slope, float offset);

  //==========================================================================
  // FFT settings

  void setFFTSize(int size);
  void setOverlapFactor(int factor);

  //==========================================================================
  // Visualization data access

  const float *getMagnitudeSpectrum() const;
  int getNumBins() const;
  int getLatencyInSamples() const;
  double getSampleRate() const { return currentSampleRate; }
  const ParameterCurve &getParameterCurve() const { return phaseCurve; }

private:
  void updatePhaseShifts();
  void updateGainValues();
  float binToFrequency(int binIndex) const;
  void applyPhaseModulation(std::complex<float> *spectrum, int numBins);
  void applyGainModulation(std::complex<float> *spectrum, int numBins);

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

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralDelayEngine)
};
