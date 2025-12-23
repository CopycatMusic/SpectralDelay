#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

//==============================================================================
/**
 * Real-time spectrum display component.
 * Displays FFT magnitude data with logarithmic frequency axis and dB amplitude
 * scale.
 */
class SpectrumDisplay : public juce::Component {
public:
  SpectrumDisplay();
  ~SpectrumDisplay() override;

  /**
   * Update spectrum data from the audio engine.
   * @param magnitudes Magnitude data (linear scale)
   * @param numBins Number of frequency bins
   * @param sampleRate Current sample rate
   */
  void setSpectralData(const float *magnitudes, int numBins, double sampleRate);

  /**
   * Set display range in dB.
   */
  void setDbRange(float minDb, float maxDb);

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  float frequencyToX(float frequency) const;
  float magnitudeToY(float magnitude) const;
  float xToFrequency(float x) const;

  // Spectrum data
  std::vector<float> magnitudeData;
  int currentNumBins = 0;
  double currentSampleRate = 44100.0;

  // Display settings
  float minFreq = 20.0f;
  float maxFreq = 20000.0f;
  float minDb = -80.0f;
  float maxDb = 0.0f;

  // Auto-scaling settings
  bool autoScale = true;
  float targetMaxDb = 0.0f;
  float currentMaxDb = 0.0f;
  float targetMinDb = -80.0f;
  float currentMinDb = -80.0f;
  float maxDbFloor = -12.0f;     // Minimum upper limit
  float maxDbCeiling = 30.0f;    // Maximum upper limit
  float minDbFloor = -100.0f;    // Minimum lower limit
  float minDbCeiling = -20.0f;   // Maximum lower limit (don't go above this)
  float attackTimeMs = 50.0f;    // Fast expand (ms)
  float releaseTimeMs = 1000.0f; // Slow contract (ms)
  float lastUpdateTime = 0.0f;

  // Smoothing
  std::vector<float> smoothedData;
  float smoothingFactor = 0.8f;

  // Colors
  juce::Colour backgroundColour;
  juce::Colour gridColour;
  juce::Colour spectrumColour;
  juce::Colour spectrumFillColour;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
