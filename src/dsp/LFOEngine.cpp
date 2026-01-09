#include "LFOEngine.h"

LFOEngine::LFOEngine() { initializeDefaultCurves(); }

LFOEngine::~LFOEngine() {}

void LFOEngine::prepare(double newSampleRate) {
  sampleRate = newSampleRate;
  // Reinitialize smoothers with new sample rate
  for (int i = 0; i < kNumParams; ++i) {
    float smoothSeconds = smoothMs[i] / 1000.0f;
    smoothedOutputs[i].reset(sampleRate, smoothSeconds);
  }
}

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

float LFOEngine::getFrameAveragedModulation(Parameter param,
                                            int numSamples) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 1.0f;

  float amount = amounts[idx] / 100.0f; // Convert to 0-1 range
  if (amount <= 0.0f)
    return 1.0f; // No modulation

  if (numSamples <= 0 || speeds[idx] <= 0.0f)
    return getModulationValue(param); // Fallback to single sample

  // Calculate phase increment per sample
  float frequency = 1000.0f / speeds[idx];
  float phaseIncrementPerSample = static_cast<float>(frequency / sampleRate);

  // Determine how many sub-samples to use for integration
  // Use at least 8 samples but cap at numSamples to avoid excessive computation
  // More samples = more accurate average but more CPU
  int integrationSamples =
      juce::jmin(numSamples, juce::jmax(8, numSamples / 64));

  float sum = 0.0f;
  float tempPhase = phases[idx];
  float stepSize =
      static_cast<float>(numSamples) / static_cast<float>(integrationSamples);

  for (int i = 0; i < integrationSamples; ++i) {
    // Calculate phase at this sub-sample point
    int sampleOffset = static_cast<int>(i * stepSize);
    float samplePhase = tempPhase + phaseIncrementPerSample * sampleOffset;

    // Wrap phase to [0, 1)
    while (samplePhase >= 1.0f)
      samplePhase -= 1.0f;

    // Get curve value at this phase
    float curveValue =
        curves[idx].getValueAtNormalizedPosition(samplePhase) / 100.0f;
    sum += curveValue;
  }

  // Calculate average
  float averageCurve = sum / static_cast<float>(integrationSamples);

  // Apply amount: blend between 1.0 (no mod) and curve value
  return 1.0f - amount + (amount * averageCurve);
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

void LFOEngine::setSmooth(Parameter param, float smooth) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    smoothMs[idx] = juce::jlimit(0.0f, 100.0f, smooth);
    // Update smoother time constant
    float smoothSeconds = smoothMs[idx] / 1000.0f;
    smoothedOutputs[idx].reset(sampleRate, smoothSeconds);
  }
}

float LFOEngine::getSmooth(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 0.0f;
  return smoothMs[idx];
}

void LFOEngine::updateSmoothedModulation(Parameter param, float targetValue,
                                         int numSamples) {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return;

  smoothedOutputs[idx].setTargetValue(targetValue);
  smoothedOutputs[idx].skip(numSamples);
}

float LFOEngine::getSmoothedModulationValue(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 1.0f;

  return smoothedOutputs[idx].getCurrentValue();
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
    // Initialize smoothers with default sample rate and no smoothing
    smoothedOutputs[i].reset(sampleRate, 0.0);
    smoothedOutputs[i].setCurrentAndTargetValue(1.0f);
  }
}
