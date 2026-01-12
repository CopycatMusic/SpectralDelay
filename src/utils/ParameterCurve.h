#pragma once

#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <mutex>
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
   * Get all points. Note: For thread safety, return a copy of the points
   * vector.
   */
  std::vector<CurvePoint> getPoints() const {
    std::lock_guard<std::recursive_mutex> lock(curveMutex);
    return points;
  }

  /**
   * Get number of points.
   */
  int getNumPoints() const {
    std::lock_guard<std::recursive_mutex> lock(curveMutex);
    return static_cast<int>(points.size());
  }

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
   * Calculate value at a normalized position (0-1 range).
   * Used for LFO curves where X axis is 0-1 instead of frequency.
   * @param position Normalized position (0.0 to 1.0)
   * @return Parameter value at that position
   */
  float getValueAtNormalizedPosition(float position) const;

  /**
   * Fill an array with values for each FFT bin.
   */
  void fillBinValues(float *output, int numBins, float sampleRate,
                     int fftSize) const;

  /**
   * Get value range based on current points.
   */
  void getValueRange(float &minValue, float &maxValue, float minFreq = 20.0f,
                     float maxFreq = 22000.0f) const;

  /**
   * Serialize points to a string for DAW state saving.
   */
  juce::String serializePoints() const;

  /**
   * Deserialize points from a string.
   */
  void deserializePoints(const juce::String &data);

private:
  mutable std::recursive_mutex curveMutex;
  std::vector<CurvePoint> points;
  float defaultValue = 100.0f; // Default for gain (100%), phase would use 0.0f

  // Internal helper that assumes mutex is already locked
  float getValueAtFrequencyInternal(float frequency) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterCurve)
};
