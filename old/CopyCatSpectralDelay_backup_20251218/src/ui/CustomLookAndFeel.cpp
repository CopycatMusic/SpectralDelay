#include "CustomLookAndFeel.h"

//==============================================================================
CustomLookAndFeel::CustomLookAndFeel() {
  // Define color palette
  backgroundDark = juce::Colour(0xFF0d1117);
  backgroundMedium = juce::Colour(0xFF161b22);
  accentPrimary = juce::Colour(0xFF00d4ff);
  accentSecondary = juce::Colour(0xFFff6b6b);
  textPrimary = juce::Colours::white;
  textSecondary = juce::Colours::white.withAlpha(0.7f);

  // Set default colors
  setColour(juce::Slider::thumbColourId, accentPrimary);
  setColour(juce::Slider::trackColourId, accentPrimary.withAlpha(0.3f));
  setColour(juce::Slider::backgroundColourId, backgroundMedium);
  setColour(juce::Slider::textBoxTextColourId, textPrimary);
  setColour(juce::Slider::textBoxBackgroundColourId, backgroundDark);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);

  setColour(juce::TextButton::buttonColourId, backgroundMedium);
  setColour(juce::TextButton::buttonOnColourId, accentPrimary);
  setColour(juce::TextButton::textColourOffId, textSecondary);
  setColour(juce::TextButton::textColourOnId, backgroundDark);

  setColour(juce::ComboBox::backgroundColourId, backgroundMedium);
  setColour(juce::ComboBox::outlineColourId,
            juce::Colours::white.withAlpha(0.2f));
  setColour(juce::ComboBox::textColourId, textPrimary);
  setColour(juce::ComboBox::arrowColourId, textSecondary);

  setColour(juce::PopupMenu::backgroundColourId, backgroundMedium);
  setColour(juce::PopupMenu::textColourId, textPrimary);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, accentPrimary);
  setColour(juce::PopupMenu::highlightedTextColourId, backgroundDark);

  setColour(juce::Label::textColourId, textPrimary);
}

CustomLookAndFeel::~CustomLookAndFeel() {}

//==============================================================================
void CustomLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                         int width, int height, float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider &slider) {
  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(8);
  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto centreX = bounds.getCentreX();
  auto centreY = bounds.getCentreY();
  auto rx = centreX - radius;
  auto ry = centreY - radius;
  auto rw = radius * 2.0f;
  auto angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // Background arc
  juce::Path bgArc;
  bgArc.addCentredArc(centreX, centreY, radius - 4, radius - 4, 0.0f,
                      rotaryStartAngle, rotaryEndAngle, true);
  g.setColour(backgroundMedium);
  g.strokePath(bgArc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

  // Value arc
  juce::Path valueArc;
  valueArc.addCentredArc(centreX, centreY, radius - 4, radius - 4, 0.0f,
                         rotaryStartAngle, angle, true);

  // Gradient for value arc
  juce::ColourGradient gradient(accentPrimary, centreX - radius, centreY,
                                accentSecondary, centreX + radius, centreY,
                                false);
  g.setGradientFill(gradient);
  g.strokePath(valueArc,
               juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Thumb
  juce::Path thumb;
  auto thumbWidth = 4.0f;
  auto thumbLength = radius * 0.5f;
  thumb.addRoundedRectangle(-thumbWidth / 2, -thumbLength - 4, thumbWidth,
                            thumbLength, 2.0f);

  g.setColour(textPrimary);
  g.fillPath(thumb, juce::AffineTransform::rotation(angle).translated(centreX,
                                                                      centreY));

  // Center dot
  g.setColour(accentPrimary);
  g.fillEllipse(centreX - 4, centreY - 4, 8, 8);
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                         int width, int height, float sliderPos,
                                         float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style,
                                         juce::Slider &slider) {
  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

  if (style == juce::Slider::LinearHorizontal) {
    auto trackHeight = 6.0f;
    auto trackY = bounds.getCentreY() - trackHeight / 2;

    // Background track
    g.setColour(backgroundMedium);
    g.fillRoundedRectangle(bounds.getX(), trackY, bounds.getWidth(),
                           trackHeight, 3.0f);

    // Value track
    g.setColour(accentPrimary);
    g.fillRoundedRectangle(bounds.getX(), trackY, sliderPos - x, trackHeight,
                           3.0f);

    // Thumb
    auto thumbSize = 16.0f;
    g.setColour(textPrimary);
    g.fillEllipse(sliderPos - thumbSize / 2,
                  bounds.getCentreY() - thumbSize / 2, thumbSize, thumbSize);

    g.setColour(accentPrimary);
    g.fillEllipse(sliderPos - thumbSize / 2 + 3,
                  bounds.getCentreY() - thumbSize / 2 + 3, thumbSize - 6,
                  thumbSize - 6);
  } else {
    LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                     minSliderPos, maxSliderPos, style, slider);
  }
}

//==============================================================================
void CustomLookAndFeel::drawButtonBackground(
    juce::Graphics &g, juce::Button &button,
    const juce::Colour &backgroundColour, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
  auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

  auto baseColour = button.getToggleState() ? accentPrimary : backgroundMedium;

  if (shouldDrawButtonAsDown)
    baseColour = baseColour.darker(0.2f);
  else if (shouldDrawButtonAsHighlighted)
    baseColour = baseColour.brighter(0.1f);

  g.setColour(baseColour);
  g.fillRoundedRectangle(bounds, 6.0f);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void CustomLookAndFeel::drawButtonText(juce::Graphics &g,
                                       juce::TextButton &button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown) {
  auto font = juce::Font(14.0f);
  g.setFont(font);

  auto textColour = button.getToggleState() ? backgroundDark : textSecondary;

  if (shouldDrawButtonAsHighlighted && !button.getToggleState())
    textColour = textPrimary;

  g.setColour(textColour);
  g.drawText(button.getButtonText(), button.getLocalBounds(),
             juce::Justification::centred, true);
}

//==============================================================================
void CustomLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height,
                                     bool isButtonDown, int buttonX,
                                     int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox &box) {
  auto bounds =
      juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

  g.setColour(backgroundMedium);
  g.fillRoundedRectangle(bounds, 6.0f);

  g.setColour(juce::Colours::white.withAlpha(0.15f));
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

  // Arrow
  juce::Path arrow;
  auto arrowX = width - 20.0f;
  auto arrowY = height / 2.0f - 3.0f;
  arrow.addTriangle(arrowX, arrowY, arrowX + 10, arrowY, arrowX + 5,
                    arrowY + 6);

  g.setColour(textSecondary);
  g.fillPath(arrow);
}

void CustomLookAndFeel::drawPopupMenuItem(
    juce::Graphics &g, const juce::Rectangle<int> &area, bool isSeparator,
    bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const juce::String &text, const juce::String &shortcutKeyText,
    const juce::Drawable *icon, const juce::Colour *textColour) {
  if (isSeparator) {
    auto r = area.reduced(5, 0);
    r.removeFromTop(r.getHeight() / 2 - 1);

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.fillRect(r.removeFromTop(1));
  } else {
    auto r = area.reduced(1);

    if (isHighlighted && isActive) {
      g.setColour(accentPrimary);
      g.fillRect(r);

      g.setColour(backgroundDark);
    } else {
      g.setColour(isActive ? textPrimary : textSecondary);
    }

    r.reduce(juce::jmin(5, area.getWidth() / 20), 0);

    auto font = juce::Font(14.0f);
    auto maxFontHeight = r.getHeight() / 1.3f;

    if (font.getHeight() > maxFontHeight)
      font.setHeight(maxFontHeight);

    g.setFont(font);

    if (isTicked) {
      auto tickArea = r.removeFromLeft(r.getHeight()).reduced(4);
      // Use bullet point instead of checkmark to avoid encoding issues
      g.drawText(juce::String::charToString(0x2022), tickArea,
                 juce::Justification::centred);
    }

    g.drawFittedText(text, r, juce::Justification::centredLeft, 1);
  }
}
