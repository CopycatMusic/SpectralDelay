#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

//==============================================================================
/**
 * Parameter curve for mapping values across frequency bins.
 * Supports constant value and linear slope modes.
 */
class ParameterCurve {
public:
  enum class Mode {
    Constant, // Same value for all frequencies
    Linear    // value = slope * log10(freq) + offset
  };

  ParameterCurve();
  ~ParameterCurve();

  /**
   * Set the curve mode.
   */
  void setMode(Mode mode);

  /**
   * Get the current mode.
   */
  Mode getMode() const { return currentMode; }

  /**
   * Set constant value (for Constant mode).
   */
  void setConstantValue(float value);

  /**
   * Get constant value.
   */
  float getConstantValue() const { return constantValue; }

  /**
   * Set linear parameters (for Linear mode).
   * Formula: value = slope * log10(frequency) + offset
   * @param slope Change in value per decade of frequency
   * @param offset Base value at 1 Hz
   */
  void setLinearParams(float slope, float offset);

  /**
   * Get slope value.
   */
  float getSlope() const { return slopeValue; }

  /**
   * Get offset value.
   */
  float getOffset() const { return offsetValue; }

  /**
   * Calculate value at a given frequency.
   * @param frequency Frequency in Hz
   * @return Parameter value at that frequency
   */
  float getValueAtFrequency(float frequency) const;

  /**
   * Fill an array with values for each FFT bin.
   * @param output Output array (must be at least numBins elements)
   * @param numBins Number of bins
   * @param sampleRate Sample rate
   * @param fftSize FFT size (to calculate bin frequencies)
   */
  void fillBinValues(float *output, int numBins, float sampleRate,
                     int fftSize) const;

  /**
   * Get value range based on current mode and parameters.
   * Useful for display normalization.
   */
  void getValueRange(float &minValue, float &maxValue, float minFreq = 20.0f,
                     float maxFreq = 20000.0f) const;

private:
  Mode currentMode = Mode::Constant;

  // Constant mode
  float constantValue = 250.0f;

  // Linear mode: value = slope * log10(freq) + offset
  float slopeValue = 0.0f;
  float offsetValue = 250.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterCurve)
};
