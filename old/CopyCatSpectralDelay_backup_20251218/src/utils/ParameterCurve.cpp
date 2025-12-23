#include "ParameterCurve.h"

//==============================================================================
ParameterCurve::ParameterCurve() {}

ParameterCurve::~ParameterCurve() {}

//==============================================================================
void ParameterCurve::setMode(Mode mode) { currentMode = mode; }

void ParameterCurve::setConstantValue(float value) { constantValue = value; }

void ParameterCurve::setLinearParams(float slope, float offset) {
  slopeValue = slope;
  offsetValue = offset;
}

//==============================================================================
float ParameterCurve::getValueAtFrequency(float frequency) const {
  switch (currentMode) {
  case Mode::Constant:
    return constantValue;

  case Mode::Linear: {
    // Avoid log of zero or negative
    if (frequency <= 0.0f)
      frequency = 1.0f;

    // value = slope * log10(freq) + offset
    float logFreq = std::log10(frequency);
    return slopeValue * logFreq + offsetValue;
  }

  default:
    return constantValue;
  }
}

void ParameterCurve::fillBinValues(float *output, int numBins, float sampleRate,
                                   int fftSize) const {
  for (int i = 0; i < numBins; ++i) {
    float frequency =
        static_cast<float>(i) * sampleRate / static_cast<float>(fftSize);
    output[i] = getValueAtFrequency(frequency);
  }
}

void ParameterCurve::getValueRange(float &minValue, float &maxValue,
                                   float minFreq, float maxFreq) const {
  switch (currentMode) {
  case Mode::Constant:
    minValue = constantValue;
    maxValue = constantValue;
    break;

  case Mode::Linear: {
    float valueAtMin = getValueAtFrequency(minFreq);
    float valueAtMax = getValueAtFrequency(maxFreq);

    if (valueAtMin < valueAtMax) {
      minValue = valueAtMin;
      maxValue = valueAtMax;
    } else {
      minValue = valueAtMax;
      maxValue = valueAtMin;
    }
    break;
  }

  default:
    minValue = 0.0f;
    maxValue = 2000.0f;
    break;
  }
}