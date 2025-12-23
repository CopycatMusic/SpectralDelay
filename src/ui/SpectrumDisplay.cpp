#include "SpectrumDisplay.h"
#include <cmath>

//==============================================================================
SpectrumDisplay::SpectrumDisplay() {
  // Set up colors for a modern dark theme
  backgroundColour = juce::Colour(0xFF0d1117);
  gridColour = juce::Colours::white.withAlpha(0.1f);
  spectrumColour = juce::Colour(0xFF00d4ff);
  spectrumFillColour = spectrumColour.withAlpha(0.3f);

  // Curve colors
  curveColour = juce::Colour(0xFFff6b6b);      // Coral red
  pointColour = juce::Colour(0xFFffd93d);      // Golden yellow
  pointHoverColour = juce::Colour(0xFFffffff); // White when hovered
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
  float minMagnitude = 1e10f;

  // Copy and smooth data
  for (int i = 0; i < numBins; ++i) {
    float newValue = magnitudes[i];
    smoothedData[i] =
        smoothedData[i] * smoothingFactor + newValue * (1.0f - smoothingFactor);
    magnitudeData[i] = smoothedData[i];

    if (smoothedData[i] > peakMagnitude) {
      peakMagnitude = smoothedData[i];
    }

    if (smoothedData[i] > 1e-10f && smoothedData[i] < minMagnitude) {
      minMagnitude = smoothedData[i];
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
  repaint();
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

  // Grid removed by user request
  // (Previously drew frequency and dB grid lines)

  // Draw spectrum
  if (currentNumBins > 0 && !magnitudeData.empty()) {
    float binWidth =
        static_cast<float>(currentSampleRate) / (currentNumBins * 2);

    if (displayMode == DisplayMode::Bar) {
      // Bar chart mode - vertical lines
      for (int i = 1; i < currentNumBins; ++i) {
        float freq = i * binWidth;

        if (freq < minFreq || freq > maxFreq)
          continue;

        float x =
            graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
        float y = graphBounds.getY() +
                  magnitudeToY(magnitudeData[i]) * graphBounds.getHeight();

        y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

        // Draw vertical line from bottom to magnitude
        g.setColour(spectrumColour);
        g.drawLine(x, y, x, graphBounds.getBottom(), 1.0f);
      }
    } else {
      // Line chart mode (default)
      juce::Path spectrumPath;
      bool pathStarted = false;
      float lastY = graphBounds.getBottom(); // Default to bottom

      for (int i = 1; i < currentNumBins; ++i) {
        float freq = i * binWidth;

        if (freq > maxFreq)
          break;

        float x =
            graphBounds.getX() + frequencyToX(freq) * graphBounds.getWidth();
        float y = graphBounds.getY() +
                  magnitudeToY(magnitudeData[i]) * graphBounds.getHeight();

        y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

        if (!pathStarted) {
          // Start from left edge at the first point's Y level
          spectrumPath.startNewSubPath(graphBounds.getX(), y);
          spectrumPath.lineTo(x, y);
          pathStarted = true;
        } else {
          spectrumPath.lineTo(x, y);
        }
        lastY = y;
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

  // Draw bypass overlay (dim the display when bypassed)
  if (isBypassed) {
    g.setColour(juce::Colour(0xFF161b22).withAlpha(0.7f));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Draw "BYPASSED" text
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(16.0f));
    g.drawText("BYPASSED", bounds, juce::Justification::centred);
  }
}

void SpectrumDisplay::drawCurve(juce::Graphics &g) {
  if (editingCurve == nullptr)
    return;

  auto graphBounds = getLocalBounds().toFloat().reduced(10.0f);
  const auto &points = editingCurve->getPoints();

  // Draw the curve line
  g.setColour(curveColour);

  if (points.empty()) {
    // No points: draw horizontal line at default value
    float y = valueToY(editingCurve->getDefaultValue());
    float screenY = graphBounds.getY() + y * graphBounds.getHeight();
    g.drawHorizontalLine(static_cast<int>(screenY), graphBounds.getX(),
                         graphBounds.getRight());
  } else if (points.size() == 1) {
    // One point: horizontal line at that value
    float y = valueToY(points[0].value);
    float screenY = graphBounds.getY() + y * graphBounds.getHeight();
    g.drawHorizontalLine(static_cast<int>(screenY), graphBounds.getX(),
                         graphBounds.getRight());
  } else {
    // Multiple points: draw polyline
    juce::Path curvePath;

    // First segment: horizontal from left edge to first point
    float firstX = frequencyToX(points[0].frequency);
    float firstY = valueToY(points[0].value);
    float screenX = graphBounds.getX() + firstX * graphBounds.getWidth();
    float screenY = graphBounds.getY() + firstY * graphBounds.getHeight();

    curvePath.startNewSubPath(graphBounds.getX(), screenY);
    curvePath.lineTo(screenX, screenY);

    // Middle segments
    for (size_t i = 1; i < points.size(); ++i) {
      float x = frequencyToX(points[i].frequency);
      float y = valueToY(points[i].value);
      screenX = graphBounds.getX() + x * graphBounds.getWidth();
      screenY = graphBounds.getY() + y * graphBounds.getHeight();
      curvePath.lineTo(screenX, screenY);
    }

    // Last segment: horizontal to right edge
    curvePath.lineTo(graphBounds.getRight(), screenY);

    g.strokePath(curvePath, juce::PathStrokeType(2.0f));
  }

  // Draw points
  for (int i = 0; i < static_cast<int>(points.size()); ++i) {
    float x = frequencyToX(points[i].frequency);
    float y = valueToY(points[i].value);
    float screenX = graphBounds.getX() + x * graphBounds.getWidth();
    float screenY = graphBounds.getY() + y * graphBounds.getHeight();

    float radius = (i == hoveredPointIndex) ? pointHoverRadius : pointRadius;
    juce::Colour colour =
        (i == hoveredPointIndex) ? pointHoverColour : pointColour;

    g.setColour(colour);
    g.fillEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2);

    // Draw outline
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawEllipse(screenX - radius, screenY - radius, radius * 2, radius * 2,
                  1.5f);
  }
}

void SpectrumDisplay::resized() {}

//==============================================================================
float SpectrumDisplay::frequencyToX(float frequency) const {
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

  auto graphBounds = getLocalBounds().toFloat().reduced(10.0f);
  const auto &points = editingCurve->getPoints();

  for (int i = 0; i < static_cast<int>(points.size()); ++i) {
    float px = frequencyToX(points[i].frequency);
    float py = valueToY(points[i].value);
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

  auto graphBounds = getLocalBounds().toFloat().reduced(10.0f);

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

    float frequency = xToFrequency(normX);
    float value = yToValue(normY);

    // Clamp value to valid range
    value = juce::jlimit(curveValueMin, curveValueMax, value);

    int newIndex = editingCurve->addPoint(frequency, value);
    draggedPointIndex = newIndex;
    isDragging = true;

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

  auto graphBounds = getLocalBounds().toFloat().reduced(10.0f);
  const auto &points = editingCurve->getPoints();

  float normX = (e.x - graphBounds.getX()) / graphBounds.getWidth();
  float normY = (e.y - graphBounds.getY()) / graphBounds.getHeight();

  normX = juce::jlimit(0.0f, 1.0f, normX);
  normY = juce::jlimit(0.0f, 1.0f, normY);

  float frequency = xToFrequency(normX);
  float value = yToValue(normY);

  // Clamp frequency so it doesn't cross neighboring points
  float minAllowedFreq = minFreq;
  float maxAllowedFreq = maxFreq;

  if (draggedPointIndex > 0) {
    // Can't go left past the previous point
    minAllowedFreq = points[draggedPointIndex - 1].frequency + 1.0f;
  }
  if (draggedPointIndex < static_cast<int>(points.size()) - 1) {
    // Can't go right past the next point
    maxAllowedFreq = points[draggedPointIndex + 1].frequency - 1.0f;
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
