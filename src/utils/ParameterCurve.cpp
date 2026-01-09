#include "ParameterCurve.h"

//==============================================================================
ParameterCurve::ParameterCurve() {}

ParameterCurve::~ParameterCurve() {}

//==============================================================================
int ParameterCurve::addPoint(float frequency, float value) {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  CurvePoint newPoint{frequency, value};
  points.push_back(newPoint);

  // Sort by frequency
  std::sort(points.begin(), points.end());

  // Find and return the index of the added point
  for (int i = 0; i < static_cast<int>(points.size()); ++i) {
    if (points[i].frequency == frequency && points[i].value == value) {
      return i;
    }
  }
  return static_cast<int>(points.size()) - 1;
}

void ParameterCurve::removePoint(int index) {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  if (index >= 0 && index < static_cast<int>(points.size())) {
    points.erase(points.begin() + index);
  }
}

void ParameterCurve::movePoint(int index, float frequency, float value) {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  if (index >= 0 && index < static_cast<int>(points.size())) {
    points[index].frequency = frequency;
    points[index].value = value;

    // Re-sort after moving
    std::sort(points.begin(), points.end());
  }
}

const CurvePoint *ParameterCurve::getPoint(int index) const {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  if (index >= 0 && index < static_cast<int>(points.size())) {
    return &points[static_cast<size_t>(index)];
  }
  return nullptr;
}

void ParameterCurve::clearPoints() {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  points.clear();
}

//==============================================================================
float ParameterCurve::getValueAtFrequency(float frequency) const {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);

  // No points: return default value
  if (points.empty()) {
    return defaultValue;
  }

  // One point: constant mode
  if (points.size() == 1) {
    return points[0].value;
  }

  // Multiple points: linear interpolation with horizontal edge extension

  // Before first point: use first point's value
  if (frequency <= points.front().frequency) {
    return points.front().value;
  }

  // After last point: use last point's value
  if (frequency >= points.back().frequency) {
    return points.back().value;
  }

  // Find surrounding points and interpolate
  for (size_t i = 0; i < points.size() - 1; ++i) {
    if (frequency >= points[i].frequency &&
        frequency <= points[i + 1].frequency) {
      // Linear interpolation - avoid division by zero
      float freqDiff = points[i + 1].frequency - points[i].frequency;
      if (freqDiff > 1e-6f) {
        float t = (frequency - points[i].frequency) / freqDiff;
        return points[i].value + t * (points[i + 1].value - points[i].value);
      }
      return points[i].value;
    }
  }

  // Fallback (shouldn't reach here)
  return defaultValue;
}

float ParameterCurve::getValueAtNormalizedPosition(float position) const {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);

  // No points: return default value
  if (points.empty()) {
    return defaultValue;
  }

  // One point: constant mode
  if (points.size() == 1) {
    return points[0].value;
  }

  // For LFO curves, points use normalized 0-1 X axis (stored as "frequency")
  // Clamp position to [0, 1]
  position = std::max(0.0f, std::min(1.0f, position));

  // Before first point: use first point's value
  if (position <= points.front().frequency) {
    return points.front().value;
  }

  // After last point: use last point's value
  if (position >= points.back().frequency) {
    return points.back().value;
  }

  // Find surrounding points and interpolate
  for (size_t i = 0; i < points.size() - 1; ++i) {
    if (position >= points[i].frequency &&
        position <= points[i + 1].frequency) {
      float posDiff = points[i + 1].frequency - points[i].frequency;
      if (posDiff > 1e-6f) {
        float t = (position - points[i].frequency) / posDiff;
        return points[i].value + t * (points[i + 1].value - points[i].value);
      }
      return points[i].value;
    }
  }

  return defaultValue;
}

void ParameterCurve::fillBinValues(float *output, int numBins, float sampleRate,
                                   int fftSize) const {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  for (int i = 0; i < numBins; ++i) {
    float frequency =
        static_cast<float>(i) * sampleRate / static_cast<float>(fftSize);
    output[i] = getValueAtFrequency(frequency);
  }
}

void ParameterCurve::getValueRange(float &minValue, float &maxValue,
                                   float /*minFreq*/, float /*maxFreq*/) const {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  if (points.empty()) {
    minValue = defaultValue;
    maxValue = defaultValue;
    return;
  }

  minValue = points[0].value;
  maxValue = points[0].value;

  for (const auto &point : points) {
    minValue = std::min(minValue, point.value);
    maxValue = std::max(maxValue, point.value);
  }
}

//==============================================================================
juce::String ParameterCurve::serializePoints() const {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  juce::String result;

  for (size_t i = 0; i < points.size(); ++i) {
    if (i > 0)
      result += ";";
    result += juce::String(points[i].frequency, 2) + "," +
              juce::String(points[i].value, 2);
  }

  return result;
}

void ParameterCurve::deserializePoints(const juce::String &data) {
  std::lock_guard<std::recursive_mutex> lock(curveMutex);
  points.clear();

  if (data.isEmpty())
    return;

  juce::StringArray pairs;
  pairs.addTokens(data, ";", "");

  for (const auto &pair : pairs) {
    juce::StringArray values;
    values.addTokens(pair, ",", "");

    if (values.size() == 2) {
      float freq = values[0].getFloatValue();
      float val = values[1].getFloatValue();
      points.push_back({freq, val});
    }
  }

  // Ensure sorted
  std::sort(points.begin(), points.end());
}