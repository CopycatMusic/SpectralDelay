#include "LFOEngine.h"

LFOEngine::LFOEngine() { initializeDefaultCurves(); }

LFOEngine::~LFOEngine() {}

void LFOEngine::prepare(double newSampleRate) {
  sampleRate = newSampleRate;
  // Reinitialize smoothers with new sample rate
  for (int i = 0; i < kNumParams; ++i) {
    float smoothSeconds = smoothMs[i].load() / 1000.0f;
    smoothedOutputs[i].reset(sampleRate, smoothSeconds);
  }
}

void LFOEngine::reset() {
  for (int i = 0; i < kNumParams; ++i) {
    phases[i] = 0.0f;
  }
}

void LFOEngine::updateHostInfo(double bpm, double ppqPosition,
                               bool currentlyPlaying) {
  currentBpm = bpm;
  currentPpqPosition = ppqPosition;
  isPlaying = currentlyPlaying;
}

void LFOEngine::advance(int numSamples) {
  for (int i = 0; i < kNumParams; ++i) {
    // If synced, phase is determined by playhead in getModulationValue
    // Only advance internal phase when NOT synced, or when synced but host is
    // playing (for smooth transitions when host starts/stops)

    // Skip internal phase update if synced and host is paused
    if (isSynced[i] && !isPlaying) {
      continue;
    }

    float speedInMs = timeModeSpeeds[i];
    if (syncModes[i] != SyncMode::Time) {
      speedInMs = getBeatDurationMs(static_cast<int>(syncIntervals[i]),
                                    currentBpm, syncModes[i]);
    }

    if (speedInMs > 0.0f) {
      float frequency = 1000.0f / speedInMs;
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

  float currentPhase = phases[idx];

  // If synced, override phase with playhead position
  if (isSynced[idx]) {
    double beatsPerCycle = 1.0;
    if (syncModes[idx] != SyncMode::Time) {
      // In beat mode, the cycle duration is directly related to beats
      beatsPerCycle = getBeatsForIndex(static_cast<int>(syncIntervals[idx]));

      // Apply Triplet/Dotted multipliers
      if (syncModes[idx] == SyncMode::Triplet)
        beatsPerCycle *= (2.0 / 3.0);
      else if (syncModes[idx] == SyncMode::Dotted)
        beatsPerCycle *= 1.5;
    } else {
      // In time mode, calculate beats per cycle based on BPM
      float msPerBeat = 60000.0f / static_cast<float>(currentBpm);
      beatsPerCycle = static_cast<double>(timeModeSpeeds[idx] / msPerBeat);
    }

    if (beatsPerCycle > 0.0) {
      currentPhase = static_cast<float>(
          std::fmod(currentPpqPosition / beatsPerCycle, 1.0));
      if (currentPhase < 0)
        currentPhase += 1.0f;
    }
  }

  // Get value from curve at current phase position (0-1 range)
  float curveValue = curves[idx].getValueAtNormalizedPosition(currentPhase);

  // Normalize curve value (assuming 0-100 range)
  float normalizedCurve = curveValue / 100.0f;

  // Apply amount: blend between 1.0 (no mod) and curve value
  return 1.0f - amount + (amount * normalizedCurve);
}

// Helper function to get integrated phase average
float LFOEngine::getFrameAveragedModulation(Parameter param,
                                            int numSamples) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 1.0f;

  float amount = amounts[idx] / 100.0f;
  if (amount <= 0.0f)
    return 1.0f;

  float speedInMs = timeModeSpeeds[idx];
  if (syncModes[idx] != SyncMode::Time) {
    speedInMs = getBeatDurationMs(static_cast<int>(syncIntervals[idx]),
                                  currentBpm, syncModes[idx]);
  }

  if (numSamples <= 0 || speedInMs <= 0.0f)
    return getModulationValue(param);

  // Calculate phase increment per sample
  float frequency = 1000.0f / speedInMs;
  float phaseIncrementPerSample = static_cast<float>(frequency / sampleRate);

  int integrationSamples =
      juce::jmin(numSamples, juce::jmax(8, numSamples / 64));
  float sum = 0.0f;

  float startPhase = phases[idx];
  if (isSynced[idx]) {
    // For synced mode, use the calculated phase at the start of original
    // advance
    double beatsPerCycle = 1.0;
    if (syncModes[idx] != SyncMode::Time) {
      beatsPerCycle = getBeatsForIndex(static_cast<int>(syncIntervals[idx]));

      // Apply Triplet/Dotted multipliers
      if (syncModes[idx] == SyncMode::Triplet)
        beatsPerCycle *= (2.0 / 3.0);
      else if (syncModes[idx] == SyncMode::Dotted)
        beatsPerCycle *= 1.5;
    } else {
      float msPerBeat = 60000.0f / static_cast<float>(currentBpm);
      beatsPerCycle = static_cast<double>(timeModeSpeeds[idx] / msPerBeat);
    }
    startPhase =
        static_cast<float>(std::fmod(currentPpqPosition / beatsPerCycle, 1.0));
    if (startPhase < 0)
      startPhase += 1.0f;
  }

  float stepSize =
      static_cast<float>(numSamples) / static_cast<float>(integrationSamples);

  for (int i = 0; i < integrationSamples; ++i) {
    int sampleOffset = static_cast<int>(i * stepSize);
    float samplePhase = startPhase + phaseIncrementPerSample * sampleOffset;

    while (samplePhase >= 1.0f)
      samplePhase -= 1.0f;

    float curveValue =
        curves[idx].getValueAtNormalizedPosition(samplePhase) / 100.0f;
    sum += curveValue;
  }

  float averageCurve = sum / static_cast<float>(integrationSamples);
  return 1.0f - amount + (amount * averageCurve);
}

float LFOEngine::getPhase(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 0.0f;

  float currentPhase = phases[idx];

  // If synced, calculate phase from playhead position (same logic as
  // getModulationValue)
  if (isSynced[idx]) {
    double beatsPerCycle = 1.0;
    if (syncModes[idx] != SyncMode::Time) {
      beatsPerCycle = getBeatsForIndex(static_cast<int>(syncIntervals[idx]));

      // Apply Triplet/Dotted multipliers
      if (syncModes[idx] == SyncMode::Triplet)
        beatsPerCycle *= (2.0 / 3.0);
      else if (syncModes[idx] == SyncMode::Dotted)
        beatsPerCycle *= 1.5;
    } else {
      float msPerBeat = 60000.0f / static_cast<float>(currentBpm);
      beatsPerCycle = static_cast<double>(timeModeSpeeds[idx] / msPerBeat);
    }

    if (beatsPerCycle > 0.0) {
      currentPhase = static_cast<float>(
          std::fmod(currentPpqPosition / beatsPerCycle, 1.0));
      if (currentPhase < 0)
        currentPhase += 1.0f;
    }
  }

  return currentPhase;
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

void LFOEngine::setSpeed(Parameter param, float speedOrIndex) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    if (syncModes[idx] != SyncMode::Time)
      syncIntervals[idx] = juce::jlimit(0.0f, 8.0f, speedOrIndex);
    else
      timeModeSpeeds[idx] = juce::jlimit(100.0f, 10000.0f, speedOrIndex);
  }
}

float LFOEngine::getSpeed(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 500.0f;

  if (syncModes[idx] != SyncMode::Time)
    return syncIntervals[idx];
  else
    return timeModeSpeeds[idx];
}

void LFOEngine::setTimeSpeed(Parameter param, float speedMs) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    timeModeSpeeds[idx] = juce::jlimit(100.0f, 10000.0f, speedMs);
  }
}

float LFOEngine::getTimeSpeed(Parameter param) const {
  int idx = static_cast<int>(param);
  return (idx >= 0 && idx < kNumParams) ? timeModeSpeeds[idx] : 500.0f;
}

void LFOEngine::setSyncInterval(Parameter param, float interval) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    syncIntervals[idx] = juce::jlimit(0.0f, 8.0f, interval);
  }
}

float LFOEngine::getSyncInterval(Parameter param) const {
  int idx = static_cast<int>(param);
  return (idx >= 0 && idx < kNumParams) ? syncIntervals[idx] : 4.0f;
}

void LFOEngine::setSmooth(Parameter param, float smooth) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    float newSmoothMs = juce::jlimit(0.0f, 200.0f, smooth);
    if (std::abs(newSmoothMs - smoothMs[idx].load()) > 0.001f) {
      smoothMs[idx].store(newSmoothMs);
      smoothNeedsUpdate[idx].store(true);
    }
  }
}

float LFOEngine::getSmooth(Parameter param) const {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return 0.0f;
  return smoothMs[idx].load();
}

void LFOEngine::setSyncMode(Parameter param, SyncMode mode) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    if (syncModes[idx] != mode) {
      syncModes[idx] = mode;
    }
  }
}

LFOEngine::SyncMode LFOEngine::getSyncMode(Parameter param) const {
  int idx = static_cast<int>(param);
  return (idx >= 0 && idx < kNumParams) ? syncModes[idx] : SyncMode::Time;
}

void LFOEngine::setSynced(Parameter param, bool synced) {
  int idx = static_cast<int>(param);
  if (idx >= 0 && idx < kNumParams) {
    isSynced[idx] = synced;
  }
}

bool LFOEngine::getSynced(Parameter param) const {
  int idx = static_cast<int>(param);
  return (idx >= 0 && idx < kNumParams) ? isSynced[idx] : false;
}

float LFOEngine::getBeatDurationMs(int beatIndex, double bpm, SyncMode mode) {
  float beats = getBeatsForIndex(beatIndex);

  // Apply Triplet/Dotted multipliers
  if (mode == SyncMode::Triplet)
    beats *= (2.0f / 3.0f);
  else if (mode == SyncMode::Dotted)
    beats *= 1.5f;

  float msPerBeat = 60000.0f / static_cast<float>(bpm);
  return beats * msPerBeat;
}

juce::String LFOEngine::getBeatString(int beatIndex, SyncMode mode) {
  static const char *labels[] = {"1/16", "1/8", "1/4", "1/2", "1",
                                 "2",    "4",   "8",   "16"};
  int idx = juce::jlimit(0, 8, beatIndex);
  juce::String label = labels[idx];

  switch (mode) {
  case SyncMode::Triplet:
    label += "T";
    break;
  case SyncMode::Dotted:
    label += ".";
    break;
  default:
    break;
  }

  return label;
}

void LFOEngine::updateSmoothedModulation(Parameter param, float targetValue,
                                         int numSamples) {
  int idx = static_cast<int>(param);
  if (idx < 0 || idx >= kNumParams)
    return;

  // Update ramp duration if it changed in UI
  if (smoothNeedsUpdate[idx].load()) {
    float smoothSeconds = smoothMs[idx].load() / 1000.0f;
    if (smoothSeconds <= 0.0f) {
      // Snap to target if smooth is 0. Explicitly set ramp to 0.0s.
      smoothedOutputs[idx].reset(sampleRate, 0.0);
      smoothedOutputs[idx].setCurrentAndTargetValue(targetValue);
    } else {
      // Set new ramp duration while preserving current progress
      smoothedOutputs[idx].reset(sampleRate, smoothSeconds);
    }
    smoothNeedsUpdate[idx].store(false);
  }

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
  for (int i = 0; i < kNumParams; ++i) {
    curves[i].clearPoints();
    curves[i].setDefaultValue(100.0f);
    smoothMs[i].store(0.0f);
    smoothNeedsUpdate[i].store(false);
    smoothedOutputs[i].reset(sampleRate, 0.0);
    smoothedOutputs[i].setCurrentAndTargetValue(1.0f);
    syncModes[i] = SyncMode::Time;
    isSynced[i] = false;
  }
}

float LFOEngine::getBeatsForIndex(int beatIndex) {
  static const float subdivisionBeats[] = {0.25f, 0.5f,  1.0f,  2.0f, 4.0f,
                                           8.0f,  16.0f, 32.0f, 64.0f};
  return subdivisionBeats[juce::jlimit(0, 8, beatIndex)];
}
