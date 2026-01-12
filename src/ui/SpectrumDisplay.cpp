#include "SpectrumDisplay.h"
#include "CustomLookAndFeel.h"
#include "UIConstants.h"
#include <cmath>

//==============================================================================
SpectrumDisplay::SpectrumDisplay() {
  // Set up colors from centralized palette
  backgroundColour = CustomLookAndFeel::getBackgroundPanel();
  gridColour = CustomLookAndFeel::getTextPrimary().withAlpha(0.2f);
  spectrumColour = CustomLookAndFeel::getAccentPrimary();
  spectrumFillColour = spectrumColour.withAlpha(0.3f);

  // Curve colors
  curveColour = CustomLookAndFeel::getAccentSecondary();
  pointColour = CustomLookAndFeel::getAccentTertiary();
  pointHoverColour = CustomLookAndFeel::getTextPrimary();

  // Set up clear button
  addAndMakeVisible(clearButton);
  clearButton.setTooltip("Clear all points");

  clearButton.onClick = [this] {
    if (editingCurve != nullptr) {
      editingCurve->clearPoints();
      updateLabelsVisibility();
      if (onCurveModified)
        onCurveModified();
      repaint();
    }
  };

  // Set up max value label
  maxValueLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextSecondary());
  maxValueLabel.setFont(juce::Font(UIConstants::kDefaultFontSize));
  maxValueLabel.setJustificationType(juce::Justification::left);
  addAndMakeVisible(maxValueLabel);

  updateLabelsVisibility();
}

SpectrumDisplay::~SpectrumDisplay() {}

//==============================================================================
void SpectrumDisplay::setSpectralData(const float *magnitudes, int numBins,
                                      double sampleRate) {
  if (magnitudes == nullptr || numBins <= 0)
    return;

  currentNumBins = numBins;
  currentSampleRate = sampleRate;

  // Resize if needed - clear smoothed data when numBins changes to avoid stale
  // display
  if (magnitudeData.size() != static_cast<size_t>(numBins)) {
    magnitudeData.resize(static_cast<size_t>(numBins));
    smoothedData.assign(static_cast<size_t>(numBins),
                        0.0f); // Clear to zero on resize

    // Reset auto-scaling to force recalculation
    currentMaxDb = maxDbFloor;
    currentMinDb = minDbCeiling;
  }

  // Copy data from the engine into our UI-local buffer
  // This is safe because magnitudes is a pointer to a visualization buffer in
  // the engine
  for (int i = 0; i < numBins; ++i) {
    magnitudeData[static_cast<size_t>(i)] = magnitudes[i];
  }

  // Find peak and min magnitude for auto-scaling
  float peakMagnitude = 0.0f;
  float minMagnitude = 1e10f;

  // Copy and smooth data
  for (int i = 0; i < numBins; ++i) {
    float newValue = magnitudes[i];
    size_t idx = static_cast<size_t>(i);
    smoothedData[idx] = smoothedData[idx] * smoothingFactor +
                        newValue * (1.0f - smoothingFactor);
    magnitudeData[idx] = smoothedData[idx];

    if (smoothedData[idx] > peakMagnitude) {
      peakMagnitude = smoothedData[idx];
    }

    if (smoothedData[idx] > 1e-10f && smoothedData[idx] < minMagnitude) {
      minMagnitude = smoothedData[idx];
    }
  }

  // Auto-scaling logic
  if (autoScale) {
    float peakDb = 20.0f * std::log10(std::max(peakMagnitude, 1e-10f));
    float minDbValue = 20.0f * std::log10(std::max(minMagnitude, 1e-10f));

    targetMaxDb = peakDb + 6.0f;
    targetMaxDb = std::max(maxDbFloor, std::min(maxDbCeiling, targetMaxDb));

    targetMinDb = minDbValue - 6.0f;
    targetMinDb = std::max(minDbFloor, std::min(minDbCeiling, targetMinDb));

    float minDynamicRange = 50.0f;
    if (targetMaxDb - targetMinDb < minDynamicRange) {
      targetMinDb = targetMaxDb - minDynamicRange;
    }

    float frameTimeMs = 16.67f;
    float attackCoeff = 1.0f - std::exp(-frameTimeMs / attackTimeMs);
    float releaseCoeff = 1.0f - std::exp(-frameTimeMs / releaseTimeMs);

    if (targetMaxDb > currentMaxDb) {
      currentMaxDb += (targetMaxDb - currentMaxDb) * attackCoeff;
    } else {
      currentMaxDb += (targetMaxDb - currentMaxDb) * releaseCoeff;
    }

    if (targetMinDb < currentMinDb) {
      currentMinDb += (targetMinDb - currentMinDb) * attackCoeff;
    } else {
      currentMinDb += (targetMinDb - currentMinDb) * releaseCoeff;
    }

    maxDb = currentMaxDb;
    minDb = currentMinDb;
  }
}

void SpectrumDisplay::setDbRange(float min, float max) {
  minDb = min;
  maxDb = max;
}

void SpectrumDisplay::setEditingCurve(ParameterCurve *curve, float valueMin,
                                      float valueMax) {
  editingCurve = curve;
  curveValueMin = valueMin;
  curveValueMax = valueMax;
  updateLabelsVisibility();
  repaint();
}

void SpectrumDisplay::updateLabelsVisibility() {
  // Clear button is only visible when editing a curve AND there are points
  if (editingCurve == nullptr) {
    clearButton.setVisible(false);
  } else {
    clearButton.setVisible(editingCurve->getNumPoints() > 0);
  }

  // Max value label is visible when NOT in LFO mode
  maxValueLabel.setVisible(!lfoMode);
}

void SpectrumDisplay::updateClearButtonVisibility() {
  updateLabelsVisibility();
}

void SpectrumDisplay::setBackgroundCurves(ParameterCurve *gain,
                                          ParameterCurve *feedback,
                                          ParameterCurve *delay,
                                          ParameterCurve *phase) {
  gainCurve = gain;
  feedbackCurve = feedback;
  delayCurve = delay;
  phaseCurve = phase;
  repaint();
}

void SpectrumDisplay::setAxisLabel(const juce::String &unit, float displayMax) {
  axisUnit = unit;
  axisDisplayMax = displayMax;

  // Update max value label text
  if (unit.isEmpty()) {
    maxValueLabel.setText("", juce::dontSendNotification);
  } else {
    // Round to integer for cleaner display if it's large, otherwise keep 1
    // decimal
    juce::String valStr = (displayMax >= 10.0f)
                              ? juce::String(static_cast<int>(displayMax))
                              : juce::String(displayMax, 1);
    maxValueLabel.setText("MAX: " + valStr + " " + unit,
                          juce::dontSendNotification);
  }

  repaint();
}

//==============================================================================
void SpectrumDisplay::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(backgroundColour);
  g.fillRect(bounds);

  auto graphBounds = getGraphBounds();

  // Draw LFO grid lines when in LFO mode
  if (lfoMode) {
    g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.2f));

    // Draw vertical grid lines (V Grid - X axis divisions)
    if (lfoVGrid > 0) {
      for (int i = 1; i < lfoVGrid; ++i) {
        float x = graphBounds.getX() +
                  (static_cast<float>(i) / static_cast<float>(lfoVGrid)) *
                      graphBounds.getWidth();
        g.drawLine(x, graphBounds.getY(), x, graphBounds.getBottom(), 1.0f);
      }
    }

    // Draw horizontal grid lines (H Grid - Y axis divisions)
    if (lfoHGrid > 0) {
      for (int i = 1; i < lfoHGrid; ++i) {
        float y = graphBounds.getY() +
                  (static_cast<float>(i) / static_cast<float>(lfoHGrid)) *
                      graphBounds.getHeight();
        g.drawLine(graphBounds.getX(), y, graphBounds.getRight(), y, 1.0f);
      }
    }
  }

  // Draw frequency grid lines (only when not in LFO mode)
  if (!lfoMode && showFrequencyGrid) {
    drawFrequencyGrid(g);
  }

  // Draw spectrum (only when not in LFO mode)
  if (!lfoMode && currentNumBins > 0 && !magnitudeData.empty()) {
    float binWidth =
        static_cast<float>(currentSampleRate) / (currentNumBins * 2);

    if (displayMode == DisplayMode::Bar) {
      // Bar chart mode - vertical lines
      if (magnitudeData.size() >= static_cast<size_t>(currentNumBins)) {
        for (int i = 1; i < currentNumBins; ++i) {
          float freq = i * binWidth;

          if (freq < minFreq || freq > maxFreq)
            continue;

          float x =
              graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
          float y = graphBounds.getY() +
                    magnitudeToY(magnitudeData[static_cast<size_t>(i)]) *
                        graphBounds.getHeight();

          y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

          // Draw vertical line from bottom to magnitude
          g.setColour(spectrumColour);
          g.drawLine(x, y, x, graphBounds.getBottom(), 1.0f);
        }
      }
    } else {
      // Line chart mode (default)
      juce::Path spectrumPath;
      bool pathStarted = false;
      for (int i = 1; i < currentNumBins; ++i) {
        float freq = i * binWidth;

        if (freq > maxFreq)
          break;

        float x =
            graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
        float y = graphBounds.getY() +
                  magnitudeToY(magnitudeData[static_cast<size_t>(i)]) *
                      graphBounds.getHeight();

        y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

        if (!pathStarted) {
          // Start from left edge at the first point's Y level
          spectrumPath.startNewSubPath(graphBounds.getX(), y);
          spectrumPath.lineTo(x, y);
          pathStarted = true;
        } else {
          spectrumPath.lineTo(x, y);
        }
      }

      if (pathStarted) {
        juce::Path fillPath = spectrumPath;
        fillPath.lineTo(graphBounds.getRight(), graphBounds.getBottom());
        fillPath.lineTo(graphBounds.getX(), graphBounds.getBottom());
        fillPath.closeSubPath();

        g.setColour(spectrumFillColour);
        g.fillPath(fillPath);

        g.setColour(spectrumColour);
        g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
      }
    }
  }

  // Draw parameter curve
  drawCurve(g);

  // Draw LFO position indicator (blue vertical line) when in LFO mode
  if (lfoMode) {
    float lineX = graphBounds.getX() + lfoPhase * graphBounds.getWidth();

    // Draw a glowing blue line
    g.setColour(juce::Colour(0x44AADDFF)); // Light blue with alpha
    g.drawLine(lineX, graphBounds.getY(), lineX, graphBounds.getBottom(), 4.0f);

    g.setColour(juce::Colour(0xFFAADDFF)); // Solid light blue
    g.drawLine(lineX, graphBounds.getY(), lineX, graphBounds.getBottom(), 2.0f);
  }

  // Draw vertical divider line at right edge
  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.3f));
  g.fillRect(getWidth() - 1, 0, 1, getHeight());
}

void SpectrumDisplay::drawCurve(juce::Graphics &g) {
  // Draw background curves in gray
  juce::Colour ghostColour = CustomLookAndFeel::getTextSecondary();

  if (lfoMode) {
    // LFO mode: show the gray curve for the currently selected parameter type
    // Pass useFrequencyAxis=true to use logarithmic frequency axis for the
    // background curve
    float mod = lfoModulations[currentParameterType];
    if (currentParameterType == 0 && gainCurve != nullptr) {
      drawParameterCurve(g, gainCurve, ghostColour, 0.0f, 200.0f, false, true,
                         mod);
    } else if (currentParameterType == 1 && feedbackCurve != nullptr) {
      drawParameterCurve(g, feedbackCurve, ghostColour, 0.0f, 100.0f, false,
                         true, mod);
    } else if (currentParameterType == 2) {
      if (useDelayMode && delayCurve != nullptr) {
        drawParameterCurve(g, delayCurve, ghostColour, 0.0f, 2000.0f, false,
                           true, mod);
      } else if (!useDelayMode && phaseCurve != nullptr) {
        drawParameterCurve(g, phaseCurve, ghostColour, 0.0f, 360.0f, false,
                           true, mod);
      }
    }
  } else {
    // Normal mode: always show all three gray curves
    // Apply real-time LFO modulation to these background curves for animation

    // Gain
    if (gainCurve != nullptr) {
      drawParameterCurve(g, gainCurve, ghostColour, 0.0f, 200.0f, false, false,
                         lfoModulations[0]);
    }

    // Feedback
    if (feedbackCurve != nullptr) {
      drawParameterCurve(g, feedbackCurve, ghostColour, 0.0f, 100.0f, false,
                         false, lfoModulations[1]);
    }

    // Time (Delay or Phase based on mode)
    if (useDelayMode && delayCurve != nullptr) {
      drawParameterCurve(g, delayCurve, ghostColour, 0.0f, 2000.0f, false,
                         false, lfoModulations[2]);
    } else if (!useDelayMode && phaseCurve != nullptr) {
      drawParameterCurve(g, phaseCurve, ghostColour, 0.0f, 360.0f, false, false,
                         lfoModulations[2]);
    }
  }

  // Finally, draw the active editing curve on top (in both modes)
  // The editing curve is "static" (what the user is drawing), so
  // modulationFactor is 1.0f
  if (editingCurve != nullptr) {
    drawParameterCurve(g, editingCurve, curveColour, curveValueMin,
                       curveValueMax, true, false, 1.0f);
  }
}

void SpectrumDisplay::drawParameterCurve(juce::Graphics &g,
                                         ParameterCurve *curve,
                                         juce::Colour colour, float minVal,
                                         float maxVal, bool isActive,
                                         bool useFrequencyAxis,
                                         float modulationFactor) {
  if (curve == nullptr)
    return;

  auto graphBounds = getGraphBounds();
  const auto &points = curve->getPoints();

  // Draw the curve line
  g.setColour(colour);

  // Helper to convert frequency to X coordinate
  // If useFrequencyAxis is true, always use logarithmic frequency axis
  // Otherwise, use the mode-dependent frequencyToX()
  auto freqToX = [this, useFrequencyAxis](float frequency) {
    if (useFrequencyAxis || !lfoMode) {
      // Use logarithmic frequency axis
      float logMin = std::log10(minFreq);
      float logMax = std::log10(maxFreq);
      float logFreq = std::log10(std::max(frequency, minFreq));
      return (logFreq - logMin) / (logMax - logMin);
    }
    // LFO mode with linear axis
    return juce::jlimit(0.0f, 1.0f, frequency);
  };

  // Helper to convert value to Y relative to this curve's range
  // Apply modulationFactor to the value before mapping to Y
  auto valToY = [minVal, maxVal, modulationFactor](float val) {
    float modulatedVal = val * modulationFactor;
    return 1.0f - (modulatedVal - minVal) / (maxVal - minVal);
  };

  if (points.empty()) {
    // No points: draw horizontal line at default value
    float y = valToY(curve->getDefaultValue());
    float screenY = graphBounds.getY() + y * graphBounds.getHeight();
    g.drawLine(graphBounds.getX(), screenY, graphBounds.getRight(), screenY,
               isActive ? 3.0f : 2.5f);
  } else if (points.size() == 1) {
    // One point: horizontal line at that value
    float y = valToY(points[0].value);
    float screenY = graphBounds.getY() + y * graphBounds.getHeight();
    g.drawLine(graphBounds.getX(), screenY, graphBounds.getRight(), screenY,
               isActive ? 3.0f : 2.5f);
  } else {
    // Multiple points: draw polyline
    juce::Path curvePath;

    // First segment: horizontal from left edge to first point
    float firstX = freqToX(points[0].frequency);
    float firstY = valToY(points[0].value);
    float screenX = graphBounds.getX() + firstX * graphBounds.getWidth();
    float screenY = graphBounds.getY() + firstY * graphBounds.getHeight();

    curvePath.startNewSubPath(graphBounds.getX(), screenY);
    curvePath.lineTo(screenX, screenY);

    // Middle segments
    for (size_t i = 1; i < points.size(); ++i) {
      float x = freqToX(points[i].frequency);
      float y = valToY(points[i].value);
      screenX = graphBounds.getX() + x * graphBounds.getWidth();
      screenY = graphBounds.getY() + y * graphBounds.getHeight();
      curvePath.lineTo(screenX, screenY);
    }

    // Last segment: horizontal to right edge
    curvePath.lineTo(graphBounds.getRight(), screenY);

    g.strokePath(curvePath, juce::PathStrokeType(isActive ? 3.0f : 2.5f));
  }

  // Draw points only for the active curve
  if (isActive) {
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
      size_t idx = static_cast<size_t>(i);
      float x = freqToX(points[idx].frequency);
      float y = valToY(points[idx].value);
      float screenX = graphBounds.getX() + x * graphBounds.getWidth();
      float screenY = graphBounds.getY() + y * graphBounds.getHeight();

      float radius = (i == hoveredPointIndex) ? pointHoverRadius : pointRadius;
      juce::Colour pColour =
          (i == hoveredPointIndex) ? pointHoverColour : pointColour;

      g.setColour(pColour);
      g.fillEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2);

      // Draw outline
      g.setColour(juce::Colours::black.withAlpha(0.5f));
      g.drawEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2,
                    1.5f);
    }
  }
}

void SpectrumDisplay::resized() {
  auto graphBounds = getGraphBounds().toNearestInt();

  // Position clear button in top-left of the graph area with 2px margin
  int buttonSize = 24;
  clearButton.setBounds(graphBounds.getX() + 2, graphBounds.getY() + 2,
                        buttonSize, buttonSize);

  // Position max value label to the right of the clear button
  // Allow plenty of width for long labels like "MAX: 2000 ms"
  int labelWidth = 100;
  int labelHeight = 20;
  maxValueLabel.setBounds(clearButton.getRight() + 5,
                          clearButton.getY() + (buttonSize - labelHeight) / 2,
                          labelWidth, labelHeight);
}

//==============================================================================
juce::Rectangle<float> SpectrumDisplay::getGraphBounds() const {
  // Use the full bounds reduced by padding - no reserved space for axis labels
  // (labels are drawn on top of the graph at the right edge)
  return getLocalBounds().toFloat().reduced(10.0f);
}

float SpectrumDisplay::snapToGrid(float value, int divisions) const {
  if (divisions <= 0)
    return value;
  float step = 1.0f / static_cast<float>(divisions);
  return std::round(value / step) * step;
}

//==============================================================================
float SpectrumDisplay::frequencyToX(float frequency) const {
  if (lfoMode) {
    // LFO mode: linear 0-1 axis
    return juce::jlimit(0.0f, 1.0f, frequency);
  }
  // Normal mode: logarithmic frequency axis
  float logMin = std::log10(minFreq);
  float logMax = std::log10(maxFreq);
  float logFreq = std::log10(std::max(frequency, minFreq));
  return (logFreq - logMin) / (logMax - logMin);
}

float SpectrumDisplay::magnitudeToY(float magnitude) const {
  float db = 20.0f * std::log10(std::max(magnitude, 1e-10f));
  float normalized = (db - minDb) / (maxDb - minDb);
  return 1.0f - juce::jlimit(0.0f, 1.0f, normalized);
}

float SpectrumDisplay::xToFrequency(float x) const {
  if (lfoMode) {
    // LFO mode: linear 0-1 axis
    return juce::jlimit(0.0f, 1.0f, x);
  }
  // Normal mode: logarithmic frequency axis
  float logMin = std::log10(minFreq);
  float logMax = std::log10(maxFreq);
  float logFreq = logMin + x * (logMax - logMin);
  return std::pow(10.0f, logFreq);
}

float SpectrumDisplay::yToValue(float y) const {
  // y is normalized 0-1, where 0=top (max value), 1=bottom (min value)
  return curveValueMax - y * (curveValueMax - curveValueMin);
}

float SpectrumDisplay::valueToY(float value) const {
  // Invert: top of screen = max value
  return 1.0f - (value - curveValueMin) / (curveValueMax - curveValueMin);
}

int SpectrumDisplay::hitTestPoint(float x, float y) const {
  if (editingCurve == nullptr)
    return -1;

  auto graphBounds = getGraphBounds();
  const auto &points = editingCurve->getPoints();

  for (int i = 0; i < static_cast<int>(points.size()); ++i) {
    size_t idx = static_cast<size_t>(i);
    float px = frequencyToX(points[idx].frequency);
    float py = valueToY(points[idx].value);
    float screenX = graphBounds.getX() + px * graphBounds.getWidth();
    float screenY = graphBounds.getY() + py * graphBounds.getHeight();

    float dx = x - screenX;
    float dy = y - screenY;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance <= hitTestRadius) {
      return i;
    }
  }

  return -1;
}

//==============================================================================
void SpectrumDisplay::mouseDown(const juce::MouseEvent &e) {
  if (editingCurve == nullptr)
    return;

  auto graphBounds = getGraphBounds();

  // Check if we clicked on an existing point
  int pointIndex =
      hitTestPoint(static_cast<float>(e.x), static_cast<float>(e.y));

  if (pointIndex >= 0) {
    // Start dragging this point
    draggedPointIndex = pointIndex;
    isDragging = true;
  } else {
    // Add a new point at click position and start dragging
    float normX = (e.x - graphBounds.getX()) / graphBounds.getWidth();
    float normY = (e.y - graphBounds.getY()) / graphBounds.getHeight();

    normX = juce::jlimit(0.0f, 1.0f, normX);
    normY = juce::jlimit(0.0f, 1.0f, normY);

    // Apply grid snapping in LFO mode
    if (lfoMode) {
      if (lfoVGrid > 0)
        normX = snapToGrid(normX, lfoVGrid);
      if (lfoHGrid > 0)
        normY = snapToGrid(normY, lfoHGrid);
    }

    float frequency = xToFrequency(normX);
    float value = yToValue(normY);

    // Clamp value to valid range
    value = juce::jlimit(curveValueMin, curveValueMax, value);

    int newIndex = editingCurve->addPoint(frequency, value);
    draggedPointIndex = newIndex;
    isDragging = true;
    updateClearButtonVisibility();

    if (onCurveModified)
      onCurveModified();
  }

  repaint();
}

void SpectrumDisplay::mouseUp(const juce::MouseEvent & /*e*/) {
  isDragging = false;
  draggedPointIndex = -1;
}

void SpectrumDisplay::mouseDrag(const juce::MouseEvent &e) {
  if (!isDragging || draggedPointIndex < 0 || editingCurve == nullptr)
    return;

  auto graphBounds = getGraphBounds();
  const auto &points = editingCurve->getPoints();

  float normX = (e.x - graphBounds.getX()) / graphBounds.getWidth();
  float normY = (e.y - graphBounds.getY()) / graphBounds.getHeight();

  normX = juce::jlimit(0.0f, 1.0f, normX);
  normY = juce::jlimit(0.0f, 1.0f, normY);

  // Apply grid snapping in LFO mode
  if (lfoMode) {
    if (lfoVGrid > 0)
      normX = snapToGrid(normX, lfoVGrid);
    if (lfoHGrid > 0)
      normY = snapToGrid(normY, lfoHGrid);
  }

  float frequency = xToFrequency(normX);
  float value = yToValue(normY);

  // Clamp frequency so it doesn't cross neighboring points
  // Use appropriate range based on mode (LFO: 0-1, normal: Hz)
  float minAllowedFreq = lfoMode ? 0.0f : minFreq;
  float maxAllowedFreq = lfoMode ? 1.0f : maxFreq;
  float freqOffset = lfoMode ? 0.001f : 1.0f; // Small offset for LFO mode

  if (draggedPointIndex > 0) {
    // Can't go left past the previous point
    minAllowedFreq =
        points[static_cast<size_t>(draggedPointIndex) - 1].frequency +
        freqOffset;
  }
  if (draggedPointIndex < static_cast<int>(points.size()) - 1) {
    // Can't go right past the next point
    maxAllowedFreq =
        points[static_cast<size_t>(draggedPointIndex) + 1].frequency -
        freqOffset;
  }

  frequency = juce::jlimit(minAllowedFreq, maxAllowedFreq, frequency);
  value = juce::jlimit(curveValueMin, curveValueMax, value);

  editingCurve->movePoint(draggedPointIndex, frequency, value);

  if (onCurveModified)
    onCurveModified();

  repaint();
}

void SpectrumDisplay::mouseDoubleClick(const juce::MouseEvent &e) {
  if (editingCurve == nullptr)
    return;

  // Check if we double-clicked on a point
  int pointIndex =
      hitTestPoint(static_cast<float>(e.x), static_cast<float>(e.y));

  if (pointIndex >= 0) {
    editingCurve->removePoint(pointIndex);
    hoveredPointIndex = -1;
    updateClearButtonVisibility();

    if (onCurveModified)
      onCurveModified();

    repaint();
  }
}

void SpectrumDisplay::mouseMove(const juce::MouseEvent &e) {
  int newHovered =
      hitTestPoint(static_cast<float>(e.x), static_cast<float>(e.y));

  if (newHovered != hoveredPointIndex) {
    hoveredPointIndex = newHovered;
    repaint();
  }
}

void SpectrumDisplay::drawFrequencyGrid(juce::Graphics &g) {
  auto graphBounds = getGraphBounds();

  // Major grid frequencies: 100Hz, 1kHz, 10kHz (alpha 0.2)
  const float majorFreqs[] = {100.0f, 1000.0f, 10000.0f};
  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.2f));

  for (float freq : majorFreqs) {
    if (freq >= minFreq && freq <= maxFreq) {
      float x =
          graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
      g.drawLine(x, graphBounds.getY(), x, graphBounds.getBottom(), 1.0f);
    }
  }

  // Minor grid frequencies: 20-90Hz, 200-900Hz, 2k-9kHz (alpha 0.1)
  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.1f));

  // 20-90 Hz (excluding 100)
  for (int i = 2; i <= 9; ++i) {
    float freq = static_cast<float>(i * 10);
    if (freq >= minFreq && freq <= maxFreq) {
      float x =
          graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
      g.drawLine(x, graphBounds.getY(), x, graphBounds.getBottom(), 1.0f);
    }
  }

  // 200-900 Hz (excluding 1000)
  for (int i = 2; i <= 9; ++i) {
    float freq = static_cast<float>(i * 100);
    if (freq >= minFreq && freq <= maxFreq) {
      float x =
          graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
      g.drawLine(x, graphBounds.getY(), x, graphBounds.getBottom(), 1.0f);
    }
  }

  // 2k-9k Hz (excluding 10000)
  for (int i = 2; i <= 9; ++i) {
    float freq = static_cast<float>(i * 1000);
    if (freq >= minFreq && freq <= maxFreq) {
      float x =
          graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
      g.drawLine(x, graphBounds.getY(), x, graphBounds.getBottom(), 1.0f);
    }
  }
}
