#include "LFOEngine.h"

LFOEngine::LFOEngine() { initializeDefaultCurves(); }

LFOEngine::~LFOEngine() {}

void LFOEngine::prepare(double newSampleRate) { sampleRate = newSampleRate; }

void LFOEngine::reset() {
  for (int i = 0; i < kNumParams; ++i) {
    phases[i] = 0.0f;
  }
}

void LFOEngine::advance(int numSamples) {
  for (int i = 0; i < kNumParams; ++i) {
    if (speeds[i] > 0.0f) {
      // Calculate phase increment per sample
      // speed is in ms, so frequency = 1000 / speed Hz
      float frequency = 1000.0f / speeds[i];
      float phaseIncrement =
          static_cast<float>(frequency / sampleRate * numSamples);
      phases[i] += phaseIncrement;

      // Wrap phase to [0, 1)
      while (phases[i] >= 1.0f) {
        phases[i] -= 1.0f;
      }
    }
  }
}

float LFOEngine::getModulationValue(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 1.0f;

  float amount = amounts[idx] / 100.0f; // Convert to 0-1 range
  if (amount <= 0.0f)
    return 1.0f; // No modulation

  // Get value from curve at current phase position (0-1 range)
  // LFO curves use normalized X axis (0-1) instead of Hz frequency
  float curveValue = curves[idx].getValueAtNormalizedPosition(phases[idx]);

  // Normalize curve value (assuming 0-100 range)
  float normalizedCurve = curveValue / 100.0f;

  // Apply amount: blend between 1.0 (no mod) and curve value
  return 1.0f - amount + (amount * normalizedCurve);
}

float LFOEngine::getPhase(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 0.0f;
  return phases[idx];
}

void LFOEngine::setAmount(Parameter param, float amount) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    amounts[idx] = juce::jlimit(0.0f, 100.0f, amount);
  }
}

float LFOEngine::getAmount(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 0.0f;
  return amounts[idx];
}

void LFOEngine::setSpeed(Parameter param, float speedMs) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    speeds[idx] = juce::jlimit(100.0f, 2000.0f, speedMs);
  }
}

float LFOEngine::getSpeed(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 500.0f;
  return speeds[idx];
}

ParameterCurve &LFOEngine::getCurve(Parameter param) {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    idx = 0;
  return curves[idx];
}

const ParameterCurve &LFOEngine::getCurve(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    idx = 0;
  return curves[idx];
}

void LFOEngine::initializeDefaultCurves() {
  // Initialize all LFO curves with default value at 100% (top)
  // No points added - same as normal parameter curves
  for (int i = 0; i < kNumParams; ++i) {
    curves[i].clearPoints();
    curves[i].setDefaultValue(100.0f);
  }
}
