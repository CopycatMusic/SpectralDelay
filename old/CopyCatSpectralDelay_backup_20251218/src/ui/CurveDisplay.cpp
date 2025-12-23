#include "CurveDisplay.h"
#include <cmath>

//==============================================================================
CurveDisplay::CurveDisplay() {
  // Set colors
  curveColour = juce::Colour(0xFFff6b6b);
  curveGlowColour = curveColour.withAlpha(0.3f);

  // Make component transparent to let spectrum show through
  setOpaque(false);
}

CurveDisplay::~CurveDisplay() {}

//==============================================================================
void CurveDisplay::setCurve(const ParameterCurve &curve, float minVal,
                            float maxVal) {
  currentMode = curve.getMode();
  constantValue = curve.getConstantValue();
  slopeValue = curve.getSlope();
  offsetValue = curve.getOffset();

  minValue = minVal;
  maxValue = maxVal;

  repaint();
}

//==============================================================================
void CurveDisplay::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(10.0f);

  juce::Path curvePath;
  bool pathStarted = false;

  // Draw curve across frequency range
  const int numPoints = 200;

  for (int i = 0; i < numPoints; ++i) {
    float normX = static_cast<float>(i) / (numPoints - 1);
    float freq =
        std::pow(10.0f, std::log10(minFreq) + normX * (std::log10(maxFreq) -
                                                       std::log10(minFreq)));

    float value;
    if (currentMode == ParameterCurve::Mode::Constant) {
      value = constantValue;
    } else {
      // Linear mode: value = slope * log10(freq) + offset
      value = slopeValue * std::log10(freq) + offsetValue;
    }

    // Clamp to display range
    value = juce::jlimit(minValue, maxValue, value);

    float x = bounds.getX() + normX * bounds.getWidth();
    float y = bounds.getBottom() -
              ((value - minValue) / (maxValue - minValue)) * bounds.getHeight();

    if (!pathStarted) {
      curvePath.startNewSubPath(x, y);
      pathStarted = true;
    } else {
      curvePath.lineTo(x, y);
    }
  }

  if (pathStarted) {
    // Draw glow effect
    g.setColour(curveGlowColour);
    g.strokePath(curvePath,
                 juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Draw main curve
    g.setColour(curveColour);
    g.strokePath(curvePath,
                 juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  }

  // Draw Y-axis labels on the right side (delay time in ms)
  g.setColour(curveColour.withAlpha(0.8f));
  g.setFont(10.0f);

  // Show some reference values
  std::vector<float> refValues = {1, 100, 500, 1000, 2000};
  for (float val : refValues) {
    if (val >= minValue && val <= maxValue) {
      float normY = (val - minValue) / (maxValue - minValue);
      float y = bounds.getBottom() - normY * bounds.getHeight();

      juce::String label = juce::String(static_cast<int>(val)) + " ms";
      g.drawText(label, static_cast<int>(bounds.getRight() - 45),
                 static_cast<int>(y - 8), 40, 16,
                 juce::Justification::centredRight);
    }
  }
}

void CurveDisplay::resized() {}

//==============================================================================
float CurveDisplay::frequencyToX(float frequency) const {
  float logMin = std::log10(minFreq);
  float logMax = std::log10(maxFreq);
  float logFreq = std::log10(std::max(frequency, minFreq));

  return (logFreq - logMin) / (logMax - logMin);
}

float CurveDisplay::valueToY(float value) const {
  return 1.0f - (value - minValue) / (maxValue - minValue);
}
