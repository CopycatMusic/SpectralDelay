#pragma once

#include "../utils/ParameterCurve.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

//==============================================================================
/**
 * LFO Engine for modulating Time, Feedback, and Gain parameters.
 * Each parameter has its own independent LFO with:
 * - A curve (ParameterCurve) defining the shape
 * - Amount (0-100%)
 * - Speed (100-2000ms cycle duration)
 */
class LFOEngine {
public:
  // Must match ControlPanel parameter type order: 0=Gain, 1=Feedback, 2=Time
  enum class Parameter { Gain = 0, Feedback = 1, Time = 2, Count = 3 };
  enum class SyncMode { Time = 0, Note, Triplet, Dotted };

  LFOEngine();
  ~LFOEngine();

  /**
   * Prepare the LFO engine with sample rate.
   */
  void prepare(double sampleRate);

  /**
   * Reset all LFO phases.
   */
  void reset();

  /**
   * Advance all LFO phases by a number of samples.
   */
  void advance(int numSamples);

  /**
   * Get the current modulation value for a parameter (single point sample).
   * @return Value between 0.0 and 1.0 based on curve and amount
   */
  float getModulationValue(Parameter param) const;

  /**
   * Get frame-averaged modulation value for a parameter.
   * This integrates the LFO curve over numSamples to prevent aliasing
   * when the frame rate is lower than the LFO frequency.
   * @param param The parameter to get modulation for
   * @param numSamples The number of samples in the frame (hop size)
   * @return Value between 0.0 and 1.0 representing average modulation
   */
  float getFrameAveragedModulation(Parameter param, int numSamples) const;

  /**
   * Get the current phase (0.0 to 1.0) for a parameter's LFO.
   */
  float getPhase(Parameter param) const;

  // Parameter setters/getters
  void setAmount(Parameter param, float amount); // 0-100%
  float getAmount(Parameter param) const;

  void setSpeed(Parameter param, float speedMs); // 100-2000ms
  float getSpeed(Parameter param) const;

  void setSmooth(Parameter param, float smoothMs); // 0-100ms
  float getSmooth(Parameter param) const;

  /**
   * Get the smoothed modulation value for a parameter.
   * This applies user-controlled smoothing to the LFO output.
   * Call this after getFrameAveragedModulation to get the final smoothed value.
   */
  float getSmoothedModulationValue(Parameter param) const;

  /**
   * Update the smoother target and advance by numSamples.
   * Should be called in processBlock with the frame-averaged LFO value.
   */
  void updateSmoothedModulation(Parameter param, float targetValue,
                                int numSamples);

  ParameterCurve &getCurve(Parameter param);
  const ParameterCurve &getCurve(Parameter param) const;

  /**
   * Update host timing information.
   * @param bpm Host tempo in beats per minute
   * @param ppqPosition Current playhead position in pulses per quarter note
   * @param isPlaying Whether the host is currently in playback
   */
  void updateHostInfo(double bpm, double ppqPosition, bool isPlaying);

  /**
   * Initialize curves with default values (flat line at 100%).
   */
  void initializeDefaultCurves();

  // Mode setters/getters
  void setSyncMode(Parameter param, SyncMode mode);
  SyncMode getSyncMode(Parameter param) const;

  void setSynced(Parameter param, bool isSynced);
  bool getSynced(Parameter param) const;

  /**
   * Get the duration of a beat subdivision in milliseconds.
   */
  static float getBeatDurationMs(int beatIndex, double bpm, SyncMode mode);

  /**
   * Get the string representation of a beat subdivision index.
   */
  static juce::String getBeatString(int beatIndex, SyncMode mode);

private:
  static constexpr int kNumParams = static_cast<int>(Parameter::Count);

  double sampleRate = 44100.0;
  double currentBpm = 120.0;
  double currentPpqPosition = 0.0;
  bool isPlaying = false;

  // Per-parameter LFO state
  float phases[kNumParams] = {0.0f, 0.0f, 0.0f};
  float amounts[kNumParams] = {0.0f, 0.0f, 0.0f};      // Default 0%
  float speeds[kNumParams] = {500.0f, 500.0f, 500.0f}; // Default 500ms
  float smoothMs[kNumParams] = {0.0f, 0.0f, 0.0f}; // Default 0ms (no smooth)

  SyncMode syncModes[kNumParams] = {SyncMode::Time, SyncMode::Time,
                                    SyncMode::Time};
  bool isSynced[kNumParams] = {false, false, false};

  // Smoothed LFO output values
  mutable juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
      smoothedOutputs[kNumParams];

  ParameterCurve curves[kNumParams];

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOEngine)
};
