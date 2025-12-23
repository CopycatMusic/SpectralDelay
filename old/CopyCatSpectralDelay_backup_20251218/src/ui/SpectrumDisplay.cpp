#include "SpectrumDisplay.h"
#include <cmath>

//==============================================================================
SpectrumDisplay::SpectrumDisplay() {
  // Set up colors for a modern dark theme
  backgroundColour = juce::Colour(0xFF0d1117);
  gridColour = juce::Colours::white.withAlpha(0.1f);
  spectrumColour = juce::Colour(0xFF00d4ff);
  spectrumFillColour = spectrumColour.withAlpha(0.3f);
}

SpectrumDisplay::~SpectrumDisplay() {}

//==============================================================================
void SpectrumDisplay::setSpectralData(const float *magnitudes, int numBins,
                                      double sampleRate) {
  if (magnitudes == nullptr || numBins <= 0)
    return;

  currentNumBins = numBins;
  currentSampleRate = sampleRate;

  // Resize if needed
  if (magnitudeData.size() != static_cast<size_t>(numBins)) {
    magnitudeData.resize(numBins);
    smoothedData.resize(numBins, 0.0f);
  }

  // Find peak and min magnitude for auto-scaling
  float peakMagnitude = 0.0f;
  float minMagnitude = 1e10f; // Start with a very large value

  // Copy and smooth data
  for (int i = 0; i < numBins; ++i) {
    float newValue = magnitudes[i];
    smoothedData[i] =
        smoothedData[i] * smoothingFactor + newValue * (1.0f - smoothingFactor);
    magnitudeData[i] = smoothedData[i];

    // Track peak
    if (smoothedData[i] > peakMagnitude) {
      peakMagnitude = smoothedData[i];
    }

    // Track minimum (ignore very small values that are essentially noise floor)
    if (smoothedData[i] > 1e-10f && smoothedData[i] < minMagnitude) {
      minMagnitude = smoothedData[i];
    }
  }

  // Auto-scaling logic
  if (autoScale) {
    // Convert peak to dB
    float peakDb = 20.0f * std::log10(std::max(peakMagnitude, 1e-10f));

    // Convert min to dB (with a safety floor)
    float minDbValue = 20.0f * std::log10(std::max(minMagnitude, 1e-10f));

    // === Upper limit (maxDb) ===
    // Add headroom (6dB above peak)
    targetMaxDb = peakDb + 6.0f;
    // Clamp to allowed range
    targetMaxDb = std::max(maxDbFloor, std::min(maxDbCeiling, targetMaxDb));

    // === Lower limit (minDb) ===
    // Subtract some headroom (6dB below min)
    targetMinDb = minDbValue - 6.0f;
    // Clamp to allowed range
    targetMinDb = std::max(minDbFloor, std::min(minDbCeiling, targetMinDb));

    // Ensure minimum dynamic range of 30dB
    float minDynamicRange = 30.0f;
    if (targetMaxDb - targetMinDb < minDynamicRange) {
      targetMinDb = targetMaxDb - minDynamicRange;
    }

    // Calculate smoothing coefficients based on time
    // Assuming ~60 FPS update rate (16.67ms per frame)
    float frameTimeMs = 16.67f;
    float attackCoeff = 1.0f - std::exp(-frameTimeMs / attackTimeMs);
    float releaseCoeff = 1.0f - std::exp(-frameTimeMs / releaseTimeMs);

    // Apply smoothing to maxDb
    if (targetMaxDb > currentMaxDb) {
      // Fast attack when signal is louder
      currentMaxDb += (targetMaxDb - currentMaxDb) * attackCoeff;
    } else {
      // Slow release when signal is quieter
      currentMaxDb += (targetMaxDb - currentMaxDb) * releaseCoeff;
    }

    // Apply smoothing to minDb
    if (targetMinDb < currentMinDb) {
      // Fast attack when signal floor is lower
      currentMinDb += (targetMinDb - currentMinDb) * attackCoeff;
    } else {
      // Slow release when signal floor is higher
      currentMinDb += (targetMinDb - currentMinDb) * releaseCoeff;
    }

    // Update display range
    maxDb = currentMaxDb;
    minDb = currentMinDb;
  }
}

void SpectrumDisplay::setDbRange(float min, float max) {
  minDb = min;
  maxDb = max;
}

//==============================================================================
void SpectrumDisplay::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(backgroundColour);
  g.fillRoundedRectangle(bounds, 8.0f);

  // Border
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

  auto graphBounds = bounds.reduced(10.0f);

  // Draw grid lines
  g.setColour(gridColour);

  // Frequency grid (logarithmic)
  std::vector<float> freqLines = {50, 100, 200, 500, 1000, 2000, 5000, 10000};
  for (float freq : freqLines) {
    if (freq >= minFreq && freq <= maxFreq) {
      float x = frequencyToX(freq);
      g.drawVerticalLine(
          static_cast<int>(graphBounds.getX() + x * graphBounds.getWidth()),
          graphBounds.getY(), graphBounds.getBottom());

      // Frequency label
      juce::String label;
      if (freq >= 1000)
        label = juce::String(static_cast<int>(freq / 1000)) + "k";
      else
        label = juce::String(static_cast<int>(freq));

      g.setFont(10.0f);
      g.drawText(label,
                 static_cast<int>(graphBounds.getX() +
                                  x * graphBounds.getWidth() - 15),
                 static_cast<int>(graphBounds.getBottom() - 15), 30, 15,
                 juce::Justification::centred);
    }
  }

  // dB grid
  for (float db = minDb; db <= maxDb; db += 20.0f) {
    float normY = (db - minDb) / (maxDb - minDb);
    float y = graphBounds.getBottom() - normY * graphBounds.getHeight();
    g.drawHorizontalLine(static_cast<int>(y), graphBounds.getX(),
                         graphBounds.getRight());

    // dB label
    g.drawText(juce::String(static_cast<int>(db)) + " dB",
               static_cast<int>(graphBounds.getRight() - 40),
               static_cast<int>(y - 8), 35, 16,
               juce::Justification::centredRight);
  }

  // Draw spectrum
  if (currentNumBins > 0 && !magnitudeData.empty()) {
    juce::Path spectrumPath;
    bool pathStarted = false;

    // Calculate bin frequency step
    float binWidth =
        static_cast<float>(currentSampleRate) / (currentNumBins * 2);

    for (int i = 1; i < currentNumBins; ++i) {
      float freq = i * binWidth;

      if (freq < minFreq || freq > maxFreq)
        continue;

      float x =
          graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
      float y = graphBounds.getY() +
                magnitudeToY(magnitudeData[i]) * graphBounds.getHeight();

      // Clamp y to graph bounds
      y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

      if (!pathStarted) {
        spectrumPath.startNewSubPath(x, y);
        pathStarted = true;
      } else {
        spectrumPath.lineTo(x, y);
      }
    }

    if (pathStarted) {
      // Draw filled area
      juce::Path fillPath = spectrumPath;
      fillPath.lineTo(graphBounds.getRight(), graphBounds.getBottom());
      fillPath.lineTo(graphBounds.getX(), graphBounds.getBottom());
      fillPath.closeSubPath();

      g.setColour(spectrumFillColour);
      g.fillPath(fillPath);

      // Draw spectrum line
      g.setColour(spectrumColour);
      g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
    }
  }
}

void SpectrumDisplay::resized() {}

//==============================================================================
float SpectrumDisplay::frequencyToX(float frequency) const {
  // Logarithmic mapping
  float logMin = std::log10(minFreq);
  float logMax = std::log10(maxFreq);
  float logFreq = std::log10(std::max(frequency, minFreq));

  return (logFreq - logMin) / (logMax - logMin);
}

float SpectrumDisplay::magnitudeToY(float magnitude) const {
  // Convert to dB
  float db = 20.0f * std::log10(std::max(magnitude, 1e-10f));

  // Normalize to 0-1 range (but invert for screen coordinates)
  float normalized = (db - minDb) / (maxDb - minDb);
  return 1.0f - juce::jlimit(0.0f, 1.0f, normalized);
}

float SpectrumDisplay::xToFrequency(float x) const {
  float logMin = std::log10(minFreq);
  float logMax = std::log10(maxFreq);
  float logFreq = logMin + x * (logMax - logMin);

  return std::pow(10.0f, logFreq);
}
