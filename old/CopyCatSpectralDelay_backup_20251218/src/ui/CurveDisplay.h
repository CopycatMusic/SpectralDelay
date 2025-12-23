#pragma once

#include "../utils/ParameterCurve.h"
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * Curve display overlay component.
 * Draws the parameter curve on top of the spectrum display.
 */
class CurveDisplay : public juce::Component {
public:
  CurveDisplay();
  ~CurveDisplay() override;

  /**
   * Set the curve to display.
   * @param curve The parameter curve
   * @param minVal Minimum display value (for Y axis scaling)
   * @param maxVal Maximum display value
   */
  void setCurve(const ParameterCurve &curve, float minVal, float maxVal);

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  float frequencyToX(float frequency) const;
  float valueToY(float value) const;

  // Current curve parameters
  ParameterCurve::Mode currentMode = ParameterCurve::Mode::Constant;
  float constantValue = 250.0f;
  float slopeValue = 0.0f;
  float offsetValue = 250.0f;

  // Display range
  float minFreq = 20.0f;
  float maxFreq = 20000.0f;
  float minValue = 1.0f;
  float maxValue = 2000.0f;

  // Colors
  juce::Colour curveColour;
  juce::Colour curveGlowColour;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveDisplay)
};
