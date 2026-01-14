#pragma once

#include "../utils/ParameterCurve.h"
#include "CustomLookAndFeel.h"
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
   * Set all available curves for background/ghost display.
   */
  void setBackgroundCurves(ParameterCurve *gain, ParameterCurve *feedback,
                           ParameterCurve *delay, ParameterCurve *phase);

  /**
   * Get the curve being edited.
   */
  ParameterCurve *getEditingCurve() { return editingCurve; }

  /**
   * Set display range in dB.
   */
  void setDbRange(float minDb, float maxDb);

  /**
   * Set axis label unit and displayed range (shown on right side of spectrum).
   * The displayMax value updates dynamically based on the multiply slider.
   * @param unit The unit string (e.g., "ms", "%", "°")
   * @param displayMax The maximum value to display (e.g., 2000 for 2000ms)
   */
  void setAxisLabel(const juce::String &unit, float displayMax);

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
   * Set dynamic mode for ghost curve selection.
   */
  void setTimeMode(bool isDelay) {
    useDelayMode = isDelay;
    repaint();
  }

  /**
   * Set LFO mode - changes display to show LFO curve with normalized X axis.
   */
  void setLfoMode(bool isLfo) {
    lfoMode = isLfo;
    updateLabelsVisibility();
    repaint();
  }

  bool isLfoMode() const { return lfoMode; }

  /**
   * Set current LFO phase for position indicator (0.0 to 1.0).
   */
  void setLfoPhase(float phase) {
    lfoPhase = phase;
    if (lfoMode)
      repaint(); // Only repaint if in LFO mode
  }

  /**
   * Set current parameter type for LFO mode gray curve display.
   * 0 = Gain, 1 = Feedback, 2 = Time
   */
  void setCurrentParameterType(int paramType) {
    currentParameterType = paramType;
    if (lfoMode)
      repaint();
  }

  /**
   * Set LFO modulation multiplier for a parameter to enable UI animation.
   * 0 = Gain, 1 = Feedback, 2 = Time
   */
  void setLfoModulation(int paramType, float value) {
    if (paramType >= 0 && paramType < 3) {
      lfoModulations[paramType] = value;
    }
  }

  /**
   * Set LFO horizontal grid divisions (Y axis snap).
   * 0 = no grid, N = divide into N parts with N+1 snap positions.
   */
  void setLfoHGrid(int divisions) {
    lfoHGrid = divisions;
    if (lfoMode)
      repaint();
  }

  /**
   * Set LFO vertical grid divisions (X axis snap).
   * 0 = no grid, N = divide into N parts with N+1 snap positions.
   */
  void setLfoVGrid(int divisions) {
    lfoVGrid = divisions;
    if (lfoMode)
      repaint();
  }

  /**
   * Set whether to show frequency grid lines (only in normal spectrum mode).
   */
  void setShowFrequencyGrid(bool show) {
    showFrequencyGrid = show;
    if (!lfoMode)
      repaint();
  }

  bool isShowFrequencyGrid() const { return showFrequencyGrid; }

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
  void updateLabelsVisibility();
  float frequencyToX(float frequency) const;
  float magnitudeToY(float magnitude) const;
  float xToFrequency(float x) const;
  float yToValue(float y) const;
  float valueToY(float value) const;

  // Hit test: returns point index or -1 if not near any point
  int hitTestPoint(float x, float y) const;

  // Draw curve points and lines
  void drawCurve(juce::Graphics &g);

  // Draw frequency grid lines (vertical)
  void drawFrequencyGrid(juce::Graphics &g);

  // Draw amplitude grid lines (horizontal)
  void drawAmplitudeGrid(juce::Graphics &g);

  // Helper to draw a single curve
  // useFrequencyAxis: if true, use logarithmic frequency axis even in LFO mode
  // modulationFactor: scales the Y value for animation
  void drawParameterCurve(juce::Graphics &g, ParameterCurve *curve,
                          juce::Colour colour, float minVal, float maxVal,
                          bool isActive, bool useFrequencyAxis = false,
                          float modulationFactor = 1.0f);

  // Get the graph bounds (excluding axis labels)
  juce::Rectangle<float> getGraphBounds() const;

  // Snap a normalized value (0-1) to grid if grid is enabled
  float snapToGrid(float value, int divisions) const;

  // Spectrum data
  std::vector<float> magnitudeData;
  int currentNumBins = 0;
  double currentSampleRate = 44100.0;

  // Display settings
  float minFreq = 20.0f;
  float maxFreq = 22000.0f;
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

  // Custom button class for modern graphics
  class IconButton : public juce::Button {
  public:
    IconButton() : Button("Clear") {}
    void paintButton(juce::Graphics &g, bool isMouseOver,
                     bool isButtonDown) override {
      auto bounds = getLocalBounds().toFloat();
      float alpha = isButtonDown ? 0.6f : (isMouseOver ? 0.4f : 0.25f);

      g.setColour(juce::Colours::black.withAlpha(alpha));
      g.fillRoundedRectangle(bounds, 8.0f);

      // Draw thin diagonal lines for "X"
      g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(
          isMouseOver ? 0.9f : 0.6f));
      float inset = bounds.getWidth() * 0.3f;
      g.drawLine(inset, inset, bounds.getWidth() - inset,
                 bounds.getHeight() - inset, 1.2f);
      g.drawLine(bounds.getWidth() - inset, inset, inset,
                 bounds.getHeight() - inset, 1.2f);
    }
  };

  // Labels
  juce::Label maxValueLabel;

  // Clear button for points
  IconButton clearButton;
  void updateClearButtonVisibility();

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
  ParameterCurve *gainCurve = nullptr;
  ParameterCurve *feedbackCurve = nullptr;
  ParameterCurve *delayCurve = nullptr;
  ParameterCurve *phaseCurve = nullptr;

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

  bool useDelayMode = true;
  bool lfoMode = false;
  float lfoPhase = 0.0f;        // Current LFO position (0.0 to 1.0)
  int currentParameterType = 2; // 0=Gain, 1=Feedback, 2=Time (default Time)
  float lfoModulations[3] = {1.0f, 1.0f, 1.0f}; // Current multipliers

  // LFO grid settings for snap-to-grid
  int lfoHGrid = 0; // Horizontal grid divisions (Y axis)
  int lfoVGrid = 0; // Vertical grid divisions (X axis)

  // Frequency grid display (normal spectrum mode only)
  bool showFrequencyGrid = true;

  // Axis label settings (right side Y-axis)
  juce::String axisUnit = "";
  float axisDisplayMax = 100.0f;
  static constexpr float axisLabelWidth =
      50.0f; // Reserved width for axis labels

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
