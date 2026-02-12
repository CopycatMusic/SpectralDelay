#include "CustomLookAndFeel.h"
#include "BinaryData.h"
#include "ShadowButton.h"
#include "UIConstants.h"

//==============================================================================
juce::Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const juce::Font &f) {
  static auto typefaceMedium = juce::Typeface::createSystemTypefaceFor(
      BinaryData::MontserratMedium_ttf, BinaryData::MontserratMedium_ttfSize);
  static auto typefaceBold = juce::Typeface::createSystemTypefaceFor(
      BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);

  if (f.isBold())
    return typefaceBold;
  return typefaceMedium;
}

//==============================================================================
CustomLookAndFeel::CustomLookAndFeel() {
  // Initialize color palette from static getters
  backgroundDark = getBackgroundDark();
  backgroundMedium = getBackgroundMedium();
  accentPrimary = getAccentPrimary();
  accentSecondary = getAccentSecondary();
  accentTertiary = getAccentTertiary();
  textPrimary = getTextPrimary();
  textSecondary = getTextSecondary();
  textTertiary = getTextTertiary();

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
  setColour(juce::TextButton::textColourOffId, textPrimary);
  setColour(juce::TextButton::textColourOnId, textPrimary);

  setColour(juce::ComboBox::backgroundColourId, backgroundMedium);
  setColour(juce::ComboBox::outlineColourId, textPrimary.withAlpha(0.3f));
  setColour(juce::ComboBox::textColourId, textPrimary);
  setColour(juce::ComboBox::arrowColourId, textPrimary);

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
  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2);
  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto centreX = bounds.getCentreX();
  auto centreY = bounds.getCentreY();

  // Calculate current angle
  auto angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // 1. Background Track (Groove)
  // ----------------------------------------------------
  bool isMini = slider.getProperties().contains("isMini");
  float trackWidth = isMini ? 3.0f : 4.0f;
  juce::Path bgArc;
  bgArc.addCentredArc(centreX, centreY, radius - trackWidth / 2,
                      radius - trackWidth / 2, 0.0f, rotaryStartAngle,
                      rotaryEndAngle, true);

  // Outer edge (highlight)
  g.setColour(backgroundDark.brighter(0.1f));
  g.strokePath(bgArc,
               juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Inner track (shadow/dark base)
  g.setColour(backgroundDark.darker(0.8f));
  g.strokePath(bgArc,
               juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // 2. Value Arc (Lit area within the groove)
  // ----------------------------------------------------
  if (slider.isEnabled()) {
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius - trackWidth / 2,
                           radius - trackWidth / 2, 0.0f, rotaryStartAngle,
                           angle, true);

    // Subtle inner glow effect
    g.setColour(accentPrimary.withAlpha(0.3f));
    g.strokePath(valueArc,
                 juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Main accent arc
    g.setColour(accentPrimary);
    g.strokePath(valueArc, juce::PathStrokeType(trackWidth - 2.0f,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
  }

  // 3. Knob Body (The central rotating part)
  // ----------------------------------------------------
  float knobRadius = radius - trackWidth - 3.0f; // Leave a small gap

  // Drop shadow for the knob body
  if (knobRadius > 0) {
    juce::Path knobPath;
    knobPath.addEllipse(centreX - knobRadius, centreY - knobRadius,
                        knobRadius * 2.0f, knobRadius * 2.0f);

    // Subtle shadow
    juce::DropShadow shadow(
        juce::Colours::black.withAlpha(UIConstants::kShadowAlpha),
        UIConstants::kShadowRadius,
        {UIConstants::kShadowXOffset, UIConstants::kShadowYOffset});
    shadow.drawForPath(g, knobPath);

    // Knob gradient (subtle top-down lighting for 3D effect)
    juce::ColourGradient knobGradient(
        backgroundMedium.brighter(0.08f), centreX, centreY - knobRadius,
        backgroundMedium.darker(0.3f), centreX, centreY + knobRadius, false);

    g.setGradientFill(knobGradient);
    g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                  knobRadius * 2.0f);

    // Inner bezel (thin ring reflection)
    g.setColour(textSecondary.withAlpha(0.1f));
    g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                  knobRadius * 2.0f, 1.0f);
  }

  // 4. Pointer / Indicator (Ridge effect)
  // ----------------------------------------------------
  if (knobRadius > 5.0f) {
    juce::Path p;
    float pointerLength = knobRadius * 0.75f;
    float pointerThickness = isMini ? 3.0f : 4.0f; // Match track width

    p.startNewSubPath(0.0, 0.0);
    p.lineTo(0.0f, -pointerLength);

    p.applyTransform(
        juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    // Draw pointer (layered neon/glow effect matching the track)
    // 1. Glow layer (Wider, semi-transparent)
    g.setColour(accentPrimary.withAlpha(0.3f));
    g.strokePath(p, juce::PathStrokeType(pointerThickness,
                                         juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));

    // 2. Core layer (Narrower, solid)
    g.setColour(accentPrimary);
    g.strokePath(p, juce::PathStrokeType(pointerThickness - 2.0f,
                                         juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
  }
}

juce::Slider::SliderLayout
CustomLookAndFeel::getSliderLayout(juce::Slider &slider) {
  auto layout = LookAndFeel_V4::getSliderLayout(slider);

  auto style = slider.getTextBoxPosition();
  if (style == juce::Slider::TextBoxLeft ||
      style == juce::Slider::TextBoxRight) {
    auto bounds = slider.getLocalBounds();
    int knobSize = UIConstants::kSmallKnobSize;
    int gap = 5;

    // For small knobs (55px), shift TextBox to start at 27px (bottom half)
    // and add horizontal gap
    if (bounds.getHeight() == knobSize) {
      layout.textBoxBounds.setY(27);

      if (style == juce::Slider::TextBoxLeft) {
        layout.sliderBounds.setX(slider.getTextBoxWidth() + gap);
        layout.textBoxBounds.setX(0);
      } else {
        layout.sliderBounds.setX(0);
        layout.textBoxBounds.setX(knobSize + gap);
      }
    }
  }

  return layout;
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
  // Use a reduced area for the visual button body to leave space for shadows
  // within the component bounds.
  const float shadowMargin = (float)UIConstants::kButtonShadowMargin;
  auto visualBounds = button.getLocalBounds().toFloat().reduced(shadowMargin);
  const float cornerSize = 6.0f;

  auto segmentPos = ShadowButton::kSingle;
  if (auto *sb = dynamic_cast<ShadowButton *>(&button))
    segmentPos = sb->getSegmentPosition();

  // Create path based on segment position
  juce::Path buttonPath;
  if (segmentPos == ShadowButton::kSingle) {
    buttonPath.addRoundedRectangle(visualBounds, cornerSize);
  } else {
    buttonPath.addRoundedRectangle(
        visualBounds.getX(), visualBounds.getY(), visualBounds.getWidth(),
        visualBounds.getHeight(), cornerSize, cornerSize,
        segmentPos == ShadowButton::kLeft,  // Top Left
        segmentPos == ShadowButton::kRight, // Top Right
        segmentPos == ShadowButton::kLeft,  // Bottom Left
        segmentPos == ShadowButton::kRight  // Bottom Right
    );
  }

  // 1. Drop Shadow (now has space within component bounds)
  if (button.isEnabled()) {
    juce::DropShadow shadow(
        juce::Colours::black.withAlpha(UIConstants::kShadowAlpha),
        UIConstants::kShadowRadius,
        {UIConstants::kShadowXOffset, UIConstants::kShadowYOffset});
    shadow.drawForPath(g, buttonPath);
  }

  // 2. Base Background
  if (button.getToggleState()) {
    // Premium opaque gradient for "on" state
    auto accent = button.findColour(juce::TextButton::buttonOnColourId);
    juce::ColourGradient grad(accent.darker(0.4f), visualBounds.getX(),
                              visualBounds.getY(), accent.darker(0.7f),
                              visualBounds.getX(), visualBounds.getBottom(),
                              false);
    g.setGradientFill(grad);
  } else {
    auto baseColour = backgroundMedium;
    if (shouldDrawButtonAsDown)
      baseColour = baseColour.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted)
      baseColour = baseColour.brighter(0.1f);
    g.setColour(baseColour);
  }
  g.fillPath(buttonPath);

  // 3. Border
  g.setColour(textPrimary.withAlpha(0.1f));
  if (button.getToggleState())
    g.setColour(
        button.findColour(juce::TextButton::buttonOnColourId).withAlpha(0.4f));

  g.strokePath(buttonPath, juce::PathStrokeType(UIConstants::kBorderWidth));
}

void CustomLookAndFeel::drawButtonText(juce::Graphics &g,
                                       juce::TextButton &button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown) {
  g.setFont(defaultFont);

  // Match the visual bounds from drawButtonBackground
  const float shadowMargin = (float)UIConstants::kButtonShadowMargin;
  auto visualBounds = button.getLocalBounds().reduced((int)shadowMargin);

  auto textColour = button.getToggleState()
                        ? button.findColour(juce::TextButton::textColourOnId)
                        : button.findColour(juce::TextButton::textColourOffId);

  if (shouldDrawButtonAsHighlighted && !button.getToggleState())
    textColour = textPrimary;

  g.setColour(textColour);
  g.drawText(button.getButtonText(), visualBounds, juce::Justification::centred,
             true);
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

  g.setColour(textPrimary.withAlpha(0.2f));
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

  // Arrow
  juce::Path arrow;
  auto arrowX = width - 20.0f;
  auto arrowY = height / 2.0f - 3.0f;
  arrow.addTriangle(arrowX, arrowY, arrowX + 10, arrowY, arrowX + 5,
                    arrowY + 6);

  g.setColour(textPrimary);
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

    g.setColour(textPrimary.withAlpha(0.2f));
    g.fillRect(r.removeFromTop(1));
  } else {
    auto r = area.reduced(1);

    if (isHighlighted && isActive) {
      g.setColour(accentPrimary);
      g.fillRect(r);

      g.setColour(backgroundDark);
    } else {
      g.setColour(isActive ? textPrimary : getTextSecondary());
    }

    r.reduce(juce::jmin(5, area.getWidth() / 20), 0);

    auto font = defaultFont;
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
