#pragma once

#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <vector>

//==============================================================================
/**
 * A point on the parameter curve.
 */
struct CurvePoint {
  float frequency; // Hz (X axis)
  float value;     // Parameter value (Y axis)

  bool operator<(const CurvePoint &other) const {
    return frequency < other.frequency;
  }
};

//==============================================================================
/**
 * Multi-point parameter curve for mapping values across frequency bins.
 *
 * Behavior:
 * - 0 points: Returns default value
 * - 1 point: Constant mode (same value for all frequencies)
 * - N points: Linear interpolation between points, horizontal extension at
 * edges
 */
class ParameterCurve {
public:
  ParameterCurve();
  ~ParameterCurve();

  /**
   * Set the default value (used when no points exist).
   */
  void setDefaultValue(float value) { defaultValue = value; }
  float getDefaultValue() const { return defaultValue; }

  /**
   * Get all points.
   */
  const std::vector<CurvePoint> &getPoints() const { return points; }

  /**
   * Get number of points.
   */
  int getNumPoints() const { return static_cast<int>(points.size()); }

  /**
   * Add a new point. Points are automatically sorted by frequency.
   * @return Index of the added point
   */
  int addPoint(float frequency, float value);

  /**
   * Remove a point by index.
   */
  void removePoint(int index);

  /**
   * Move a point to a new position.
   */
  void movePoint(int index, float frequency, float value);

  /**
   * Get point by index.
   */
  const CurvePoint *getPoint(int index) const;

  /**
   * Clear all points.
   */
  void clearPoints();

  /**
   * Calculate value at a given frequency using linear interpolation.
   * @param frequency Frequency in Hz
   * @return Parameter value at that frequency
   */
  float getValueAtFrequency(float frequency) const;

  /**
   * Fill an array with values for each FFT bin.
   */
  void fillBinValues(float *output, int numBins, float sampleRate,
                     int fftSize) const;

  /**
   * Get value range based on current points.
   */
  void getValueRange(float &minValue, float &maxValue, float minFreq = 20.0f,
                     float maxFreq = 20000.0f) const;

  /**
   * Serialize points to a string for DAW state saving.
   */
  juce::String serializePoints() const;

  /**
   * Deserialize points from a string.
   */
  void deserializePoints(const juce::String &data);

private:
  std::vector<CurvePoint> points;
  float defaultValue = 100.0f; // Default for gain (100%), phase would use 0.0f

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterCurve)
};
