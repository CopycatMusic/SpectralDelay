#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * Custom look and feel for the CopyCat Spectral Delay plugin.
 * Provides a modern, dark theme with vibrant accent colors.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
  CustomLookAndFeel();
  ~CustomLookAndFeel() override;

  // Static color getters for use throughout the application
  static juce::Colour getBackgroundDark() { return juce::Colour(0xFF0f1c2e); }
  static juce::Colour getBackgroundMedium() { return juce::Colour(0xFF0D1827); }
  static juce::Colour getBackgroundPanel() { return juce::Colour(0xFF0a131f); }
  static juce::Colour getAccentPrimary() { return juce::Colour(0xFF45D4ED); }
  // Sky Blue
  static juce::Colour getAccentSecondary() { return juce::Colour(0xFFE63946); }
  // Punch Red
  static juce::Colour getAccentTertiary() { return juce::Colour(0xFFffd93d); }
  // Mustard Yellow
  static juce::Colour getTextPrimary() { return juce::Colour(0xFFF1FAEE); }

  // https://coolors.co/0d1117-161b22-00d4ff-ff6b6b-ffd93d-ffffff
  // new color version
  // https://coolors.co/0f1c2e-0D1827-0a131f-45D4ED-E63946-ffd93d-FF1FAEE
  // DO NOT CHANGE THIS LINK

  // Slider
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override;

  // Button
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

  void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

  // ComboBox
  void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &box) override;

  void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                         bool isSeparator, bool isActive, bool isHighlighted,
                         bool isTicked, bool hasSubMenu,
                         const juce::String &text,
                         const juce::String &shortcutKeyText,
                         const juce::Drawable *icon,
                         const juce::Colour *textColour) override;

private:
  // Color palette (cached from static getters)
  juce::Colour backgroundDark;
  juce::Colour backgroundMedium;
  juce::Colour accentPrimary;
  juce::Colour accentSecondary;
  juce::Colour accentTertiary;
  juce::Colour textPrimary;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};
