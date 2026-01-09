#pragma once

#include "../utils/ParameterCurve.h"
#include <juce_audio_basics/juce_audio_basics.h>

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
   * Get the current modulation value for a parameter.
   * @return Value between 0.0 and 1.0 based on curve and amount
   */
  float getModulationValue(Parameter param) const;

  /**
   * Get the current phase (0.0 to 1.0) for a parameter's LFO.
   */
  float getPhase(Parameter param) const;

  // Parameter setters/getters
  void setAmount(Parameter param, float amount); // 0-100%
  float getAmount(Parameter param) const;

  void setSpeed(Parameter param, float speedMs); // 100-2000ms
  float getSpeed(Parameter param) const;

  ParameterCurve &getCurve(Parameter param);
  const ParameterCurve &getCurve(Parameter param) const;

  /**
   * Get current sample rate.
   */
  double getSampleRate() const { return sampleRate; }

  /**
   * Initialize curves with default values (flat line at 100%).
   */
  void initializeDefaultCurves();

private:
  static constexpr int kNumParams = static_cast<int>(Parameter::Count);

  double sampleRate = 44100.0;

  // Per-parameter LFO state
  float phases[kNumParams] = {0.0f, 0.0f, 0.0f};
  float amounts[kNumParams] = {0.0f, 0.0f, 0.0f};      // Default 0%
  float speeds[kNumParams] = {500.0f, 500.0f, 500.0f}; // Default 500ms

  ParameterCurve curves[kNumParams];

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOEngine)
};
