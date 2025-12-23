#pragma once

#include "../utils/ParameterCurve.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

//==============================================================================
/**
 * Real-time spectrum display with interactive curve editing.
 * Displays FFT magnitude data and allows users to edit parameter curves
 * by clicking to add points, dragging to move, and double-clicking to delete.
 */
class SpectrumDisplay : public juce::Component {
public:
  SpectrumDisplay();
  ~SpectrumDisplay() override;

  /**
   * Update spectrum data from the audio engine.
   */
  void setSpectralData(const float *magnitudes, int numBins, double sampleRate);

  /**
   * Set the curve to edit.
   */
  void setEditingCurve(ParameterCurve *curve, float valueMin, float valueMax);

  /**
   * Get the curve being edited.
   */
  ParameterCurve *getEditingCurve() { return editingCurve; }

  /**
   * Set display range in dB.
   */
  void setDbRange(float minDb, float maxDb);

  // Display mode
  enum class DisplayMode { Line, Bar };

  /**
   * Set spectrum display mode (line or bar chart).
   */
  void setDisplayMode(DisplayMode mode) {
    displayMode = mode;
    repaint();
  }

  /**
   * Set bypass state - dims the display when bypassed.
   */
  void setBypassed(bool bypassed) {
    isBypassed = bypassed;
    repaint();
  }

  /**
   * Callback when curve points are modified.
   */
  std::function<void()> onCurveModified;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Mouse interaction
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

private:
  float frequencyToX(float frequency) const;
  float magnitudeToY(float magnitude) const;
  float xToFrequency(float x) const;
  float yToValue(float y) const;
  float valueToY(float value) const;

  // Hit test: returns point index or -1 if not near any point
  int hitTestPoint(float x, float y) const;

  // Draw curve points and lines
  void drawCurve(juce::Graphics &g);

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
  float maxDbFloor = -12.0f;
  float maxDbCeiling = 30.0f;
  float minDbFloor = -120.0f;
  float minDbCeiling = -60.0f;
  float attackTimeMs = 50.0f;
  float releaseTimeMs = 1000.0f;
  float lastUpdateTime = 0.0f;

  // Smoothing
  std::vector<float> smoothedData;
  float smoothingFactor = 0.8f;

  // Colors
  juce::Colour backgroundColour;
  juce::Colour gridColour;
  juce::Colour spectrumColour;
  juce::Colour spectrumFillColour;
  juce::Colour curveColour;
  juce::Colour pointColour;
  juce::Colour pointHoverColour;

  // Curve editing state
  ParameterCurve *editingCurve = nullptr;
  float curveValueMin = 0.0f;
  float curveValueMax = 360.0f;
  int hoveredPointIndex = -1;
  int draggedPointIndex = -1;
  bool isDragging = false;

  // Point visual settings
  static constexpr float pointRadius = 6.0f;
  static constexpr float pointHoverRadius = 8.0f;
  static constexpr float hitTestRadius = 12.0f;

  // Display mode (line or bar chart)
  DisplayMode displayMode = DisplayMode::Line;

  // Bypass state
  bool isBypassed = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
