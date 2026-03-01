#include "ControlPanel.h"
#include "../dsp/LFOEngine.h"
#include "BinaryData.h"
#include "CustomLookAndFeel.h"

//==============================================================================
ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState &apvts)
    : parameters(apvts) {

  // Parameter selector buttons (no label)

  gainButton.setRadioGroupId(1);
  gainButton.setClickingTogglesState(true);
  gainButton.setSegmentPosition(ShadowButton::kLeft);
  gainButton.onClick = [this]() { selectParameter(0); };
  addAndMakeVisible(gainButton);

  feedbackButton.setRadioGroupId(1);
  feedbackButton.setClickingTogglesState(true);
  feedbackButton.setSegmentPosition(ShadowButton::kRight);
  feedbackButton.onClick = [this]() { selectParameter(1); };
  addAndMakeVisible(feedbackButton);

  delayButton.setRadioGroupId(1);
  delayButton.setClickingTogglesState(true);
  delayButton.setToggleState(true, juce::dontSendNotification);
  delayButton.setSegmentPosition(ShadowButton::kMiddle);
  delayButton.onClick = [this]() { selectParameter(2); };
  addAndMakeVisible(delayButton);

  // Delay mode toggle (Phase vs Delay) - initially hidden
  // Default: Toggle off = Delay mode, Toggle on = Phase mode
  delayModeButton.setButtonText("Delay");
  delayModeButton.setToggleState(false,
                                 juce::dontSendNotification); // Off = Delay
  delayModeButton.setClickingTogglesState(true);
  delayModeButton.onClick = [this]() {
    // Inverted: toggle off = Delay, toggle on = Phase
    currentDelayMode = !delayModeButton.getToggleState();
    delayModeButton.setButtonText(currentDelayMode ? "Delay" : "Phase");

    // Show/hide delay multiply control based on mode (only show in Delay mode)
    bool showDelayMult = currentDelayMode && (currentParameterType == 2);
    delayMultLabel.setVisible(showDelayMult);
    delayMultSlider.setVisible(showDelayMult);
    resized();

    if (onDelayModeChanged) {
      onDelayModeChanged(currentDelayMode);
    }
    // Trigger curve update
    if (onParameterTypeChanged) {
      onParameterTypeChanged(currentParameterType);
    }
  };
  addChildComponent(delayModeButton); // Initially hidden

  // Mode label for delay mode toggle
  delayModeLabel.setText("Mode:", juce::dontSendNotification);
  delayModeLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayModeLabel); // Initially hidden

  // Feedback controls - initially hidden
  feedbackMultLabel.setText("Amount", juce::dontSendNotification);
  feedbackMultLabel.setJustificationType(juce::Justification::centred);
  feedbackMultLabel.setColour(juce::Label::textColourId,
                              CustomLookAndFeel::getTextPrimary());
  addChildComponent(feedbackMultLabel);

  feedbackMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  feedbackMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  feedbackMultSlider.setMouseDragSensitivity(300);
  addChildComponent(feedbackMultSlider);
  feedbackMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "feedbackMult", feedbackMultSlider);

  feedbackInvertButton.setClickingTogglesState(true);
  feedbackInvertButton.onClick = [this]() {
    if (onFeedbackInvertChanged) {
      onFeedbackInvertChanged(feedbackInvertButton.getToggleState());
    }
  };
  addChildComponent(feedbackInvertButton);

  // Gain multiply control - initially hidden
  gainMultLabel.setText("Gain", juce::dontSendNotification);
  gainMultLabel.setJustificationType(juce::Justification::centred);
  gainMultLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(gainMultLabel);

  gainMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  gainMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  gainMultSlider.setMouseDragSensitivity(300); // 300 for high precision knobs
  addChildComponent(gainMultSlider);
  gainMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "gainMult", gainMultSlider);

  // Gain Pre/Post button - initially hidden
  gainPostButton.setClickingTogglesState(true);
  gainPostButton.setButtonText("Pre Gain");
  gainPostButton.setColour(juce::TextButton::buttonOnColourId,
                           CustomLookAndFeel::getAccentPrimary());
  gainPostButton.onClick = [this]() {
    bool isPost = gainPostButton.getToggleState();
    gainPostButton.setButtonText(isPost ? "Post Gain" : "Pre Gain");
    if (onGainPostModeChanged) {
      onGainPostModeChanged(isPost);
    }
  };
  addChildComponent(gainPostButton);

  // Gate control - initially hidden
  gateLabel.setText("Gate", juce::dontSendNotification);
  gateLabel.setJustificationType(juce::Justification::centred);
  gateLabel.setColour(juce::Label::textColourId,
                      CustomLookAndFeel::getTextPrimary());
  addChildComponent(gateLabel);

  gateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  gateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  gateSlider.setMouseDragSensitivity(250);
  addChildComponent(gateSlider);
  gateAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "gate", gateSlider);

  // Compress control - initially hidden
  compressLabel.setText("Compress", juce::dontSendNotification);
  compressLabel.setJustificationType(juce::Justification::centred);
  compressLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(compressLabel);

  compressSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  compressSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  compressSlider.setMouseDragSensitivity(300);
  addChildComponent(compressSlider);
  compressAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "compress", compressSlider);

  // Delay multiply control - initially hidden
  delayMultLabel.setText("Amount", juce::dontSendNotification);
  delayMultLabel.setJustificationType(juce::Justification::centred);
  delayMultLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayMultLabel);

  delayMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  delayMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  delayMultSlider.setMouseDragSensitivity(300);
  delayMultSlider.textFromValueFunction = [](double value) {
    float ms = static_cast<float>(value) * 20.0f;
    return (ms >= 10.0f) ? juce::String(static_cast<int>(ms)) + " ms"
                         : juce::String(ms, 1) + " ms";
  };
  delayMultSlider.valueFromTextFunction = [](const juce::String &text) {
    return text.getFloatValue() / 20.0f;
  };
  addChildComponent(delayMultSlider);
  delayMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "delayMult", delayMultSlider);

  // Diffuse width control - only visible for Time/Delay parameter
  diffuseLabel.setText("Diffuse", juce::dontSendNotification);
  diffuseLabel.setJustificationType(juce::Justification::centred);
  diffuseLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  addChildComponent(diffuseLabel);

  diffuseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  diffuseSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  diffuseSlider.setMouseDragSensitivity(250);
  addChildComponent(diffuseSlider);
  diffuseAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "diffuse", diffuseSlider);

  // Stereo Cutoff control
  stereoLowCutLabel.setText("Cutoff", juce::dontSendNotification);
  stereoLowCutLabel.setJustificationType(juce::Justification::centred);
  stereoLowCutLabel.setColour(juce::Label::textColourId,
                              CustomLookAndFeel::getTextPrimary());
  addChildComponent(stereoLowCutLabel);

  stereoLowCutSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  stereoLowCutSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  stereoLowCutSlider.setMouseDragSensitivity(250);
  addChildComponent(stereoLowCutSlider);
  stereoLowCutAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "stereoLowCut", stereoLowCutSlider);

  // FFT settings (now a dropdown)
  fftSizeLabel.setText("FFT Size", juce::dontSendNotification);
  fftSizeLabel.setJustificationType(juce::Justification::centred);
  fftSizeLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(fftSizeLabel);

  fftSizeSelector.addItemList({"32", "64", "128", "256", "512", "1024", "2048",
                               "4096", "8192", "16384"},
                              1);
  addAndMakeVisible(fftSizeSelector);

  fftSizeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          parameters, "fftSize", fftSizeSelector);

  // Mix control
  mixLabel.setText("Mix", juce::dontSendNotification);
  mixLabel.setJustificationType(juce::Justification::centred);
  mixLabel.setColour(juce::Label::textColourId,
                     CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(mixLabel);

  mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  mixSlider.setMouseDragSensitivity(300);
  mixSlider.addListener(this);
  addAndMakeVisible(mixSlider);
  mixAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "mix", mixSlider);

  // High precision delay mode button
  hiResButton.setClickingTogglesState(true);
  hiResButton.setToggleState(true, juce::dontSendNotification); // Default: on
  hiResButton.setColour(juce::TextButton::buttonOnColourId,
                        CustomLookAndFeel::getAccentPrimary());
  hiResButton.onClick = [this]() {
    if (onHighPrecisionDelayChanged) {
      onHighPrecisionDelayChanged(hiResButton.getToggleState());
    }
  };
  addAndMakeVisible(hiResButton);

  // Display mode toggle
  displayModeLabel.setText("Display:", juce::dontSendNotification);
  displayModeLabel.setJustificationType(juce::Justification::centred);

  displayModeLabel.setColour(juce::Label::textColourId,
                             CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(displayModeLabel);

  // Grid toggle button
  gridButton.setClickingTogglesState(true);
  gridButton.setToggleState(true, juce::dontSendNotification); // Default: on
  gridButton.setColour(juce::TextButton::buttonOnColourId,
                       CustomLookAndFeel::getAccentPrimary());
  gridButton.onClick = [this]() {
    if (onGridDisplayChanged) {
      onGridDisplayChanged(gridButton.getToggleState());
    }
  };
  addAndMakeVisible(gridButton);

  displayModeButton.setButtonText("Line");
  displayModeButton.setClickingTogglesState(true);
  displayModeButton.onClick = [this]() {
    bool isBar = displayModeButton.getToggleState();
    displayModeButton.setButtonText(isBar ? "Bar" : "Line");
    if (onDisplayModeChanged) {
      onDisplayModeChanged(isBar);
    }
  };
  addAndMakeVisible(displayModeButton);

  // Spectrum source toggle
  spectrumSourceLabel.setText("Source:", juce::dontSendNotification);
  spectrumSourceLabel.setColour(juce::Label::textColourId,
                                CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(spectrumSourceLabel);

  spectrumSourceButton.setButtonText("Output");
  spectrumSourceButton.setClickingTogglesState(true);
  spectrumSourceButton.onClick = [this]() {
    bool isInput = spectrumSourceButton.getToggleState();
    spectrumSourceButton.setButtonText(isInput ? "Input" : "Output");
    if (onSpectrumSourceChanged) {
      onSpectrumSourceChanged(isInput);
    }
  };
  addAndMakeVisible(spectrumSourceButton);

  // LFO mode button (at the top of control panel) or LITE label
#if LITE_VERSION
  // Show "LITE" label in blue
  liteLabel.setText("LITE VERSION", juce::dontSendNotification);

  auto montserratBold = juce::Typeface::createSystemTypefaceFor(
      BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);
  liteLabel.setFont(juce::Font(montserratBold).withHeight(14.0f));

  liteLabel.setColour(juce::Label::textColourId,
                      CustomLookAndFeel::getAccentPrimary());
  liteLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(liteLabel);
#else
  // Full version: LFO toggle button
  lfoButton.setClickingTogglesState(true);
  lfoButton.onClick = [this]() {
    setLfoMode(lfoButton.getToggleState());
    if (onLfoModeChanged) {
      onLfoModeChanged(currentLfoMode);
    }
  };
  addAndMakeVisible(lfoButton);
#endif

#if !LITE_VERSION
  // LFO Amount slider (0-100%)
  lfoAmountLabel.setText("Amount", juce::dontSendNotification);
  lfoAmountLabel.setJustificationType(juce::Justification::centred);
  lfoAmountLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoAmountLabel);

  lfoAmountSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoAmountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
  lfoAmountSlider.setMouseDragSensitivity(250);
  // Note: range, suffix, and value are now managed by APVTS attachment
  addAndMakeVisible(lfoAmountSlider);

  // Initial attachment binding (will be updated in selectParameter)
  updateLfoAmountAttachment();

  // LFO Speed slider (100-2000ms)
  lfoSpeedLabel.setText("Time", juce::dontSendNotification);
  lfoSpeedLabel.setJustificationType(juce::Justification::centred);
  lfoSpeedLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoSpeedLabel);

  lfoSpeedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
  lfoSpeedSlider.setMouseDragSensitivity(250);
  lfoSpeedSlider.setRange(200.0, 20000.0, 0.0);
  lfoSpeedSlider.setValue(1000.0); // Default 1s
  lfoSpeedSlider.setTextValueSuffix("ms");
  lfoSpeedSlider.onValueChange = [this]() {
    if (isUpdating)
      return;
    if (onLfoSpeedChanged) {
      onLfoSpeedChanged(currentParameterType,
                        static_cast<float>(lfoSpeedSlider.getValue()));
    }
  };
  addAndMakeVisible(lfoSpeedSlider);

  // LFO Smooth slider (0-100ms, displayed as 0-100%)
  lfoSmoothLabel.setText("Smooth", juce::dontSendNotification);
  lfoSmoothLabel.setJustificationType(juce::Justification::centred);
  lfoSmoothLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoSmoothLabel);

  lfoSmoothSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoSmoothSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
  lfoSmoothSlider.setMouseDragSensitivity(250);
  lfoSmoothSlider.setRange(0.0, 200.0, 0.0);
  lfoSmoothSlider.setNumDecimalPlacesToDisplay(0);
  lfoSmoothSlider.setValue(0.0); // Default 0ms (no smooth)
  lfoSmoothSlider.setTextValueSuffix("ms");
  lfoSmoothSlider.onValueChange = [this]() {
    if (onLfoSmoothChanged) {
      onLfoSmoothChanged(currentParameterType,
                         static_cast<float>(lfoSmoothSlider.getValue()));
    }
  };
  addAndMakeVisible(lfoSmoothSlider);

  // LFO Mode toggle button (Cycles through Time, Note, Triplet, Dotted)
  lfoBeatModeButton.setClickingTogglesState(
      false); // We'll handle state manually
  lfoBeatModeButton.onClick = [this]() {
    SyncMode currentMode = SyncMode::Time;
    if (onGetLfoSyncMode) {
      currentMode = onGetLfoSyncMode(currentParameterType);
    }

    // Cycle: Time (0) -> Note (1) -> Triplet (2) -> Dotted (3) -> Time (0)
    int nextModeInt = (static_cast<int>(currentMode) + 1) % 4;
    SyncMode nextMode = static_cast<SyncMode>(nextModeInt);

    // Update engine only - let callback update UI to avoid range clamping
    // issues
    if (onLfoSyncModeChanged) {
      onLfoSyncModeChanged(currentParameterType, nextMode);
    }
  };
  addAndMakeVisible(lfoBeatModeButton);

  // LFO Host Sync toggle button
  lfoSyncButton.setClickingTogglesState(true);
  lfoSyncButton.onClick = [this]() {
    bool isSynced = lfoSyncButton.getToggleState();
    lfoSyncButton.setButtonText(isSynced ? "Sync On" : "Sync Off");
    if (onLfoSyncChanged) {
      onLfoSyncChanged(currentParameterType, isSynced);
    }
  };

  addAndMakeVisible(lfoSyncButton);

  // LFO H Grid slider (horizontal divisions - Y axis)
  lfoHGridLabel.setText("H Grid", juce::dontSendNotification);
  lfoHGridLabel.setJustificationType(juce::Justification::centred);
  lfoHGridLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoHGridLabel);

  // Grid values: index 0-8 maps to discrete values: 0, 2, 3, 4, 6, 8, 12, 16,
  // 32
  static const int gridValues[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
  static const int numGridValues = 9;

  lfoHGridSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoHGridSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 65, 15);
  lfoHGridSlider.setRange(0.0, numGridValues - 1, 1.0); // Index-based range
  lfoHGridSlider.textFromValueFunction = [](double value) {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int idx = juce::jlimit(0, 8, static_cast<int>(value));
    return juce::String(gv[idx]);
  };
  lfoHGridSlider.valueFromTextFunction = [](const juce::String &text) {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int val = text.getIntValue();
    // Find the closest index
    for (int i = 0; i < 9; ++i) {
      if (gv[i] == val)
        return static_cast<double>(i);
    }
    return 0.0;
  };
  lfoHGridSlider.setValue(0.0);
  lfoHGridSlider.onValueChange = [this]() {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int idx = static_cast<int>(lfoHGridSlider.getValue());
    int value = gv[juce::jlimit(0, 8, idx)];
    if (onLfoHGridChanged) {
      onLfoHGridChanged(value);
    }
  };
  addAndMakeVisible(lfoHGridSlider);

  // LFO V Grid slider (vertical divisions - X/time axis)
  lfoVGridLabel.setText("V Grid", juce::dontSendNotification);
  lfoVGridLabel.setJustificationType(juce::Justification::centred);
  lfoVGridLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoVGridLabel);

  lfoVGridSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoVGridSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  lfoVGridSlider.setRange(0.0, numGridValues - 1, 1.0); // Index-based range
  lfoVGridSlider.textFromValueFunction = [](double value) {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int idx = juce::jlimit(0, 8, static_cast<int>(value));
    return juce::String(gv[idx]);
  };
  lfoVGridSlider.valueFromTextFunction = [](const juce::String &text) {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int val = text.getIntValue();
    for (int i = 0; i < 9; ++i) {
      if (gv[i] == val)
        return static_cast<double>(i);
    }
    return 0.0;
  };
  lfoVGridSlider.setValue(5.0); // Default index 5 = 8 divisions
  lfoVGridSlider.onValueChange = [this]() {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int idx = static_cast<int>(lfoVGridSlider.getValue());
    int value = gv[juce::jlimit(0, 8, idx)];
    if (onLfoVGridChanged) {
      onLfoVGridChanged(value);
    }
  };
  addAndMakeVisible(lfoVGridSlider);
#endif // !LITE_VERSION
}

ControlPanel::~ControlPanel() { mixSlider.removeListener(this); }

//==============================================================================
void ControlPanel::paint(juce::Graphics &g) {
  g.fillAll(CustomLookAndFeel::getBackgroundDark());

  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.1f));

  // Draw separator below parameter selector row (LFO button +
  // Gain/Delay/Feedback) Top section = UIConstants::kMargin + LFO row +
  // UIConstants::kSpacing + param row
  // + UIConstants::kSpacing
  int topSectionEndY = UIConstants::kMargin + UIConstants::kButtonHeight +
                       UIConstants::kSpacing + UIConstants::kButtonHeight +
                       UIConstants::kSpacing;
  g.fillRect(UIConstants::kMargin, topSectionEndY,
             getWidth() - UIConstants::kMargin * 2, 1);

  // Draw separator above the bottom section only when not in LFO mode
  // (bottom section is hidden in LFO mode)
  {
    // Must match resized() calculations exactly!
    int smallKnobSize = UIConstants::kSmallKnobSize; // 55
    int displayRowHeight = UIConstants::kButtonHeight + UIConstants::kSpacing;

    int middleSectionHeight = 0;
    if (currentParameterType == 2) {
      // Side-by-side Time mode height
      // Right side controls: Knob(55) - TextBox is on the side
      middleSectionHeight = (smallKnobSize + UIConstants::kSpacing) * 2;
    } else {
      // Original centered Gain/Feedback mode height
      middleSectionHeight = UIConstants::kElementHeight +
                            UIConstants::kLargeKnobSize +
                            UIConstants::kElementHeight;
    }

    // Bottom section: 55px (Label and Value are on the sides)
    int knobSectionHeight = smallKnobSize;

    // The separator is drawn relative to the bottom section's start
    int bottomSectionStartY = getHeight() - UIConstants::kMargin -
                              knobSectionHeight - UIConstants::kSpacing -
                              displayRowHeight;
    int lineY = bottomSectionStartY - (UIConstants::kMargin / 2);
    g.fillRect(UIConstants::kMargin, lineY,
               getWidth() - UIConstants::kMargin * 2, 1);
  }
}

void ControlPanel::resized() {
  int width = getWidth();
  int height = getHeight();

  // Mix and FFT: 55px height total (Label and TextBox are on the sides)
  int knobSectionHeight = UIConstants::kSmallKnobSize;

  // Display row
  int displayRowHeight = UIConstants::kButtonHeight + UIConstants::kSpacing;

  int bottomY = height - UIConstants::kMargin - knobSectionHeight -
                UIConstants::kSpacing - displayRowHeight;

  // --- Display Row: Grid, Line/Bar, and HiRes on the same row ---
  juce::Rectangle<int> displayRow(UIConstants::kMargin, bottomY,
                                  width - UIConstants::kMargin * 2,
                                  UIConstants::kButtonHeight);
  int buttonAreaWidth = displayRow.getWidth();
  int singleButtonWidth =
      (buttonAreaWidth - UIConstants::kSpacing * 2) / 3; // 3 buttons now
  gridButton.setBounds(displayRow.removeFromLeft(singleButtonWidth));
  displayRow.removeFromLeft(UIConstants::kSpacing);
  displayModeButton.setBounds(displayRow.removeFromLeft(singleButtonWidth));
  displayRow.removeFromLeft(UIConstants::kSpacing);
  hiResButton.setBounds(displayRow);

  bottomY += displayRowHeight + UIConstants::kSpacing;

  // --- Knobs Row for FFT and Mix ---
  int halfWidth = (width - UIConstants::kMargin * 2) / 2;
  int textBoxWidth = 65;
  int textGap = 5;
  int sliderWidthWithText =
      UIConstants::kSmallKnobSize + textBoxWidth + textGap;

  // FFT Dropdown (LEFT-WING)
  int comboWidth = 90;
  int comboHeight = 22;
  int fftX = UIConstants::kMargin + (halfWidth - comboWidth) / 2;
  fftSizeLabel.setBounds(fftX, bottomY + 2, comboWidth,
                         UIConstants::kElementHeight);
  fftSizeSelector.setBounds(fftX, bottomY + 18, comboWidth, comboHeight);

  // Mix Knob (RIGHT-WING: Knob on Left, Text on Right)
  int mixX =
      UIConstants::kMargin + halfWidth + (halfWidth - sliderWidthWithText) / 2;
  mixSlider.setBounds(mixX, bottomY, sliderWidthWithText,
                      UIConstants::kSmallKnobSize);
  mixLabel.setBounds(mixX + UIConstants::kSmallKnobSize + textGap, bottomY + 12,
                     textBoxWidth, UIConstants::kElementHeight);

  bottomY += UIConstants::kSmallKnobSize + UIConstants::kMargin +
             UIConstants::kSpacing;

  spectrumSourceLabel.setVisible(false);

  // ============ TOP SECTION: Auto-arrange from top ============
  int currentY = UIConstants::kMargin;

  // Row 1: LFO button (Full) or LITE label (Lite)
  juce::Rectangle<int> lfoRow(UIConstants::kMargin, currentY,
                              width - UIConstants::kMargin * 2,
                              UIConstants::kButtonHeight);
#if LITE_VERSION
  liteLabel.setBounds(lfoRow);
#else
  lfoButton.setBounds(lfoRow);
#endif
  currentY += UIConstants::kButtonHeight + UIConstants::kSpacing;

  // Row 2: Parameter selector buttons (Gain, Time, Feedback)
  juce::Rectangle<int> paramRow(UIConstants::kMargin, currentY,
                                width - UIConstants::kMargin * 2,
                                UIConstants::kButtonHeight);

  // Segmented layout: buttons with 2px gap
  const int shadowMargin = UIConstants::kButtonShadowMargin;
  const int gap = 2;
  const int totalWidth = paramRow.getWidth();
  const int buttonWidth = (totalWidth - (gap * 2)) / 3;
  const int buttonWidthWithShadow = buttonWidth + (shadowMargin * 2);

  gainButton.setBounds(paramRow.getX() - shadowMargin, currentY - shadowMargin,
                       buttonWidthWithShadow,
                       UIConstants::kButtonHeight + (shadowMargin * 2));

  delayButton.setBounds(paramRow.getX() + buttonWidth + gap - shadowMargin,
                        currentY - shadowMargin, buttonWidthWithShadow,
                        UIConstants::kButtonHeight + (shadowMargin * 2));

  feedbackButton.setBounds(paramRow.getX() + (buttonWidth + gap) * 2 -
                               shadowMargin,
                           currentY - shadowMargin, buttonWidthWithShadow,
                           UIConstants::kButtonHeight + (shadowMargin * 2));

  currentY += UIConstants::kButtonHeight + UIConstants::kSpacing;

  // Row 3: Mode row removed (Phase mode removed, always Delay mode)

  // Spacer
  currentY += UIConstants::kSpacing;

  // ============ MULTIPLY CONTROLS or LFO CONTROLS ============
#if !LITE_VERSION
  if (currentLfoMode) {
    // LFO mode: Time, Amount, Smooth aligned with Gain, Delay, Feedback buttons
    int knobWithTextHeight =
        UIConstants::kMediumKnobSize + UIConstants::kElementHeight;
    int lfoRowSpacing = 12;

    // Align knob centers with button centers (Each button is 90px wide)
    int speedX = UIConstants::kMargin + 45 - (UIConstants::kMediumKnobSize / 2);
    int amountX =
        UIConstants::kMargin + 90 + 45 - (UIConstants::kMediumKnobSize / 2);
    int smoothX =
        UIConstants::kMargin + 180 + 45 - (UIConstants::kMediumKnobSize / 2);

    // Labels row
    lfoSpeedLabel.setBounds(speedX, currentY, UIConstants::kMediumKnobSize,
                            UIConstants::kElementHeight);
    lfoAmountLabel.setBounds(amountX, currentY, UIConstants::kMediumKnobSize,
                             UIConstants::kElementHeight);
    lfoSmoothLabel.setBounds(smoothX, currentY, UIConstants::kMediumKnobSize,
                             UIConstants::kElementHeight);

    currentY += UIConstants::kElementHeight + 2;

    // Knobs row
    lfoSpeedSlider.setBounds(speedX, currentY, UIConstants::kMediumKnobSize,
                             knobWithTextHeight);
    lfoAmountSlider.setBounds(amountX, currentY, UIConstants::kMediumKnobSize,
                              knobWithTextHeight);
    lfoSmoothSlider.setBounds(smoothX, currentY, UIConstants::kMediumKnobSize,
                              knobWithTextHeight);

    currentY += knobWithTextHeight + (lfoRowSpacing / 2);

    // Row 4: LFO Mode options (Time/Beat and Sync)
    juce::Rectangle<int> lfoOptionsRow(UIConstants::kMargin, currentY,
                                       width - UIConstants::kMargin * 2,
                                       UIConstants::kButtonHeight);
    int optionButtonWidth =
        (lfoOptionsRow.getWidth() - UIConstants::kSpacing) / 2;
    lfoBeatModeButton.setBounds(
        lfoOptionsRow.removeFromLeft(optionButtonWidth));
    lfoOptionsRow.removeFromLeft(UIConstants::kSpacing);
    lfoSyncButton.setBounds(lfoOptionsRow);

    currentY += UIConstants::kButtonHeight + (lfoRowSpacing / 2);

    // Row 5: Grid controls (H Grid and V Grid SYMMETRIC)
    int textBoxWidth = 65;
    int textGap = 5;
    int sliderWidthWithText =
        UIConstants::kSmallKnobSize + textBoxWidth + textGap;

    // Center the two grid units in the available width
    int halfControlWidth = (width - UIConstants::kMargin * 2) / 2;

    // H Grid (Left-Wing: Text on Left, Knob on Right)
    int hGridX =
        UIConstants::kMargin + (halfControlWidth - sliderWidthWithText) / 2;
    lfoHGridLabel.setBounds(hGridX, currentY + 12, textBoxWidth,
                            UIConstants::kElementHeight);
    lfoHGridSlider.setBounds(hGridX, currentY, sliderWidthWithText,
                             UIConstants::kSmallKnobSize);

    // V Grid (Right-Wing: Knob on Left, Text on Right)
    int vGridX = UIConstants::kMargin + halfControlWidth +
                 (halfControlWidth - sliderWidthWithText) / 2;
    lfoVGridSlider.setBounds(vGridX, currentY, sliderWidthWithText,
                             UIConstants::kSmallKnobSize);
    lfoVGridLabel.setBounds(vGridX + UIConstants::kSmallKnobSize + textGap,
                            currentY + 12, textBoxWidth,
                            UIConstants::kElementHeight);

    currentY += UIConstants::kSmallKnobSize + (lfoRowSpacing / 2);
  } else
#endif // !LITE_VERSION
  {    // Normal mode: show Multiply knob (CENTERED)

    if (!currentLfoMode) {
      if (currentParameterType == 0) {
        // HORIZONTAL COMPOSITE LAYOUT FOR GAIN/DYNAMICS:
        // Left: Gate (Small) | Center: Gain (Large) | Right: Compress (Small)
        // All center-aligned vertically.

        int usableWidth = width - UIConstants::kMargin * 2;
        int centerSlotWidth = 120;
        int sideSlotWidth = (usableWidth - centerSlotWidth) / 2;

        int gainHeight = UIConstants::kLargeKnobSize +
                         UIConstants::kElementHeight +
                         UIConstants::kElementHeight; // 90+15+15=120
        int knobCenterY = currentY + UIConstants::kElementHeight +
                          UIConstants::kLargeKnobSize / 2;

        // --- 1. GATE (LEFT SLOT) ---
        int gateX = UIConstants::kMargin +
                    (sideSlotWidth - UIConstants::kSmallKnobSize) / 2;
        gateLabel.setBounds(UIConstants::kMargin,
                            knobCenterY - UIConstants::kSmallKnobSize / 2 -
                                UIConstants::kElementHeight - 5,
                            sideSlotWidth, UIConstants::kElementHeight);
        gateSlider.setBounds(
            gateX, knobCenterY - UIConstants::kSmallKnobSize / 2,
            UIConstants::kSmallKnobSize,
            UIConstants::kSmallKnobSize + UIConstants::kElementHeight);

        // --- 2. GAIN (CENTER SLOT) ---
        int gainX = UIConstants::kMargin + sideSlotWidth +
                    (centerSlotWidth - UIConstants::kLargeKnobSize) / 2;
        gainMultLabel.setBounds(UIConstants::kMargin + sideSlotWidth, currentY,
                                centerSlotWidth, UIConstants::kElementHeight);
        gainMultSlider.setBounds(gainX, currentY + UIConstants::kElementHeight,
                                 UIConstants::kLargeKnobSize,
                                 UIConstants::kLargeKnobSize +
                                     UIConstants::kElementHeight);

        // --- 3. COMPRESS (RIGHT SLOT) ---
        int compressX = UIConstants::kMargin + sideSlotWidth + centerSlotWidth +
                        (sideSlotWidth - UIConstants::kSmallKnobSize) / 2;
        compressLabel.setBounds(UIConstants::kMargin + sideSlotWidth +
                                    centerSlotWidth,
                                knobCenterY - UIConstants::kSmallKnobSize / 2 -
                                    UIConstants::kElementHeight - 5,
                                sideSlotWidth, UIConstants::kElementHeight);
        compressSlider.setBounds(
            compressX, knobCenterY - UIConstants::kSmallKnobSize / 2,
            UIConstants::kSmallKnobSize,
            UIConstants::kSmallKnobSize + UIConstants::kElementHeight);

        // --- 4. BUTTONS ROW (TIED IN CENTER) ---
        int buttonY = currentY + gainHeight + UIConstants::kSpacing;
        juce::Rectangle<int> buttonsRow(UIConstants::kMargin, buttonY,
                                        width - UIConstants::kMargin * 2,
                                        UIConstants::kButtonHeight);
        gainPostButton.setBounds(buttonsRow);

        currentY = buttonY + UIConstants::kButtonHeight + UIConstants::kSpacing;

      } else if (currentParameterType == 1) {
        // ORIGINAL CENTERED LAYOUT FOR FEEDBACK
        juce::Rectangle<int> multLabelRow(UIConstants::kMargin, currentY,
                                          width - UIConstants::kMargin * 2,
                                          UIConstants::kElementHeight);
        feedbackMultLabel.setBounds(multLabelRow);
        currentY += UIConstants::kElementHeight;

        int knobX = (width - UIConstants::kLargeKnobSize) / 2;
        feedbackMultSlider.setBounds(
            knobX, currentY, UIConstants::kLargeKnobSize,
            UIConstants::kLargeKnobSize + UIConstants::kElementHeight);
        currentY += UIConstants::kLargeKnobSize + UIConstants::kElementHeight;

      } else if (currentParameterType == 2) {
        // NEW COMPOSITE LAYOUT FOR TIME MODE:
        // Left side: Large Multiply knob (90px)
        // Right side: Two stacked small knobs (50px) for Stereo and Cutoff
        int compositeHeight =
            (UIConstants::kSmallKnobSize + UIConstants::kSpacing) * 2;
        int textBoxWidth = 65;
        int textGap = 5;
        int sliderWidthWithText =
            UIConstants::kSmallKnobSize + textBoxWidth + textGap;

        int leftAreaWidth = (width - UIConstants::kMargin * 2) * 0.55f;
        int rightAreaWidth = (width - UIConstants::kMargin * 2) - leftAreaWidth;

        int largeKnobWithTextHeight =
            UIConstants::kLargeKnobSize + UIConstants::kElementHeight;
        int largeControlHeight =
            UIConstants::kElementHeight + largeKnobWithTextHeight;

        int largeYOffset = (compositeHeight - largeControlHeight) / 2;

        delayMultLabel.setBounds(UIConstants::kMargin, currentY + largeYOffset,
                                 leftAreaWidth, UIConstants::kElementHeight);
        delayMultSlider.setBounds(
            UIConstants::kMargin +
                (leftAreaWidth - UIConstants::kLargeKnobSize) / 2,
            currentY + largeYOffset + UIConstants::kElementHeight,
            UIConstants::kLargeKnobSize, largeKnobWithTextHeight);

        int rightX = UIConstants::kMargin + leftAreaWidth;
        int diffuseKnobY = currentY;
        int knobsOffsetX = -5; // User requested 5px left shift

        diffuseSlider.setBounds(
            rightX + (rightAreaWidth - sliderWidthWithText) / 2 + knobsOffsetX,
            diffuseKnobY, sliderWidthWithText, UIConstants::kSmallKnobSize);
        // Note: diffuseLabel is on the right of the knob in this layout
        diffuseLabel.setBounds(
            rightX + (rightAreaWidth - sliderWidthWithText) / 2 + knobsOffsetX +
                UIConstants::kSmallKnobSize + textGap,
            diffuseKnobY + 12, textBoxWidth, UIConstants::kElementHeight);

        int cutoffKnobY =
            diffuseKnobY + UIConstants::kSmallKnobSize + UIConstants::kSpacing;
        stereoLowCutSlider.setBounds(
            rightX + (rightAreaWidth - sliderWidthWithText) / 2 + knobsOffsetX,
            cutoffKnobY, sliderWidthWithText, UIConstants::kSmallKnobSize);
        stereoLowCutLabel.setBounds(
            rightX + (rightAreaWidth - sliderWidthWithText) / 2 + knobsOffsetX +
                UIConstants::kSmallKnobSize + textGap,
            cutoffKnobY + 12, textBoxWidth, UIConstants::kElementHeight);

        currentY += compositeHeight;
      }
    }

    // MAX label (Below knob) - HIDDEN
    /*
    juce::Rectangle<int> maxLabelRow(margin, currentY, width - margin * 2,
    20); if (currentParameterType == 0) gainMaxLabel.setBounds(maxLabelRow);
    else if (currentParameterType == 1)
      feedbackMaxLabel.setBounds(maxLabelRow);
    else if (currentParameterType == 2)
      delayMaxLabel.setBounds(maxLabelRow);
    */
  }
}

//==============================================================================
void ControlPanel::selectParameter(int paramType) {
  currentParameterType = paramType;

  // Update button toggle states
  gainButton.setToggleState(paramType == 0, juce::dontSendNotification);
  delayButton.setToggleState(paramType == 2, juce::dontSendNotification);
  feedbackButton.setToggleState(paramType == 1, juce::dontSendNotification);

  // Show/hide delay mode button and label based on parameter selection
  // Hidden as requested: default to Delay mode
  delayModeButton.setVisible(false);
  delayModeLabel.setVisible(false);

  bool showFeedbackControls = (paramType == 1) && !currentLfoMode;
  feedbackMultLabel.setVisible(showFeedbackControls);
  feedbackMultSlider.setVisible(showFeedbackControls);
  feedbackInvertButton.setVisible(false); // Hidden as requested

  // Show/hide gain multiply control (Gain = paramType 0)
  bool showGainControls = (paramType == 0) && !currentLfoMode;
  gainMultLabel.setVisible(showGainControls);
  gainMultSlider.setVisible(showGainControls);
  gainPostButton.setVisible(showGainControls);

  // Dynamics Cluster (Gate and Compress) - grouped with Gain
  gateLabel.setVisible(showGainControls);
  gateSlider.setVisible(showGainControls);
  compressLabel.setVisible(showGainControls);
  compressSlider.setVisible(showGainControls);

  // Show/hide delay multiply control (only when Delay mode is active, not
  // Phase)
  bool showDelayControls = (paramType == 2) && !currentLfoMode;
  delayMultLabel.setVisible(showDelayControls);
  delayMultSlider.setVisible(showDelayControls);

  // Show/hide diffuse width controls (only for Time/Delay parameter)
  bool showDiffuseControls = (paramType == 2) && !currentLfoMode;
  diffuseLabel.setVisible(showDiffuseControls);
  diffuseSlider.setVisible(showDiffuseControls);
  stereoLowCutLabel.setVisible(showDiffuseControls);
  stereoLowCutSlider.setVisible(showDiffuseControls);

  // Show/hide LFO controls based on LFO mode
  lfoAmountLabel.setVisible(currentLfoMode);
  lfoAmountSlider.setVisible(currentLfoMode);
  lfoSpeedLabel.setVisible(currentLfoMode);
  lfoSpeedSlider.setVisible(currentLfoMode);
  lfoSmoothLabel.setVisible(currentLfoMode);
  lfoSmoothSlider.setVisible(currentLfoMode);

  // New LFO mode controls
  lfoBeatModeButton.setVisible(currentLfoMode);
  lfoSyncButton.setVisible(currentLfoMode);

  // LFO grid controls
  lfoHGridLabel.setVisible(currentLfoMode);
  lfoHGridSlider.setVisible(currentLfoMode);
  lfoVGridLabel.setVisible(currentLfoMode);
  lfoVGridSlider.setVisible(currentLfoMode);

  // Update LFO amount attachment to bind to the correct parameter
  updateLfoAmountAttachment();

  // Bottom section controls: shown in all modes
  bool showBottomSection = true;
  fftSizeLabel.setVisible(showBottomSection);
  fftSizeSelector.setVisible(showBottomSection);
  mixLabel.setVisible(showBottomSection);
  mixSlider.setVisible(showBottomSection);
  displayModeLabel.setVisible(false); // Always hidden
  displayModeButton.setVisible(showBottomSection);
  spectrumSourceButton.setVisible(false); // Always hidden, default to Output
  hiResButton.setVisible(showBottomSection);

  resized(); // Update layout

  if (onParameterTypeChanged) {
    onParameterTypeChanged(currentParameterType);
  }
}

void ControlPanel::comboBoxChanged(juce::ComboBox * /*comboBox*/) {
  // No longer used for parameter selection
}

void ControlPanel::sliderValueChanged(juce::Slider * /*slider*/) {
  // Mix slider is auto-attached, no manual handling needed
}

void ControlPanel::setLfoMode(bool isLfoMode) {
#if !LITE_VERSION
  currentLfoMode = isLfoMode;
  lfoButton.setToggleState(isLfoMode, juce::dontSendNotification);

  // Sync visibility of all components through selectParameter
  selectParameter(currentParameterType);

  // Repaint to update separator line visibility
  repaint();
#else
  juce::ignoreUnused(isLfoMode);
#endif
}

void ControlPanel::updateLfoControls(float amount, float speedOrIndex,
                                     float smoothMs, SyncMode mode,
                                     bool isSynced) {
  juce::ScopedValueSetter<bool> updating(isUpdating, true);
  // Note: lfoAmountSlider value is now managed by APVTS attachment
  lfoSmoothSlider.setValue(smoothMs, juce::dontSendNotification);

  // Update mode buttons
  lfoBeatModeButton.setButtonText(getSyncModeString(mode));
  lfoSyncButton.setToggleState(isSynced, juce::dontSendNotification);
  lfoSyncButton.setButtonText(isSynced ? "Sync On" : "Sync Off");

  // Update speed slider range and value based on mode
  updateLfoSpeedSliderSettings(mode);
  lfoSpeedSlider.setValue(speedOrIndex, juce::dontSendNotification);
  lfoSpeedSlider.updateText(); // Force text refresh after mode/value sync
}

void ControlPanel::setGainPostMode(bool post) {
  gainPostButton.setToggleState(post, juce::dontSendNotification);
  gainPostButton.setButtonText(post ? "Post Gain" : "Pre Gain");
}

void ControlPanel::updateLfoSpeedSliderSettings(SyncMode mode) {
  if (mode != SyncMode::Time) {
    lfoSpeedSlider.setRange(0.0, 8.0,
                            1.0);      // Integer steps for beat subdivisions
    lfoSpeedSlider.setSkewFactor(1.0); // Ensure linear behavior
    lfoSpeedSlider.setTextValueSuffix("");
    lfoSpeedSlider.textFromValueFunction = [mode](double value) {
      return LFOEngine::getBeatString(static_cast<int>(value), mode);
    };
    lfoSpeedSlider.valueFromTextFunction = [mode](const juce::String &text) {
      for (int i = 0; i < 9; ++i) {
        if (LFOEngine::getBeatString(i, mode) == text)
          return static_cast<double>(i);
      }
      return 0.0;
    };
  } else {
    // Time mode: 200ms to 20000ms (20s), non-linear with 2000ms (2s) at
    // center
    lfoSpeedSlider.setRange(200.0, 20000.0,
                            0.0); // Continuous (0.0 step size)
    lfoSpeedSlider.setSkewFactorFromMidPoint(2000.0); // 2s at center
    lfoSpeedSlider.setTextValueSuffix("");
    lfoSpeedSlider.textFromValueFunction = [](double value) {
      if (value >= 1000.0) {
        // Display in seconds with 2 decimal places
        return juce::String(value / 1000.0, 2) + "s";
      } else {
        // Display in milliseconds (integer)
        return juce::String(static_cast<int>(value)) + "ms";
      }
    };
    lfoSpeedSlider.valueFromTextFunction = [](const juce::String &text) {
      if (text.endsWithIgnoreCase("s") && !text.endsWithIgnoreCase("ms")) {
        // Parse seconds
        return text.dropLastCharacters(1).getDoubleValue() * 1000.0;
      } else if (text.endsWithIgnoreCase("ms")) {
        // Parse milliseconds
        return text.dropLastCharacters(2).getDoubleValue();
      }
      return text.getDoubleValue();
    };
  }
}

void ControlPanel::setLfoHGrid(int divisions) {
  // Find index in gridValues[] = {0, 2, 3, 4, 6, 8, 12, 16, 32}
  static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
  int idx = 0;
  for (int i = 0; i < 9; ++i) {
    if (gv[i] == divisions) {
      idx = i;
      break;
    }
  }
  lfoHGridSlider.setValue(static_cast<double>(idx), juce::dontSendNotification);
  lfoHGridSlider.updateText();
}

void ControlPanel::setLfoVGrid(int divisions) {
  static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
  int idx = 0;
  for (int i = 0; i < 9; ++i) {
    if (gv[i] == divisions) {
      idx = i;
      break;
    }
  }
  lfoVGridSlider.setValue(static_cast<double>(idx), juce::dontSendNotification);
  lfoVGridSlider.updateText();
}

juce::String ControlPanel::getSyncModeString(SyncMode mode) {
  switch (mode) {
  case SyncMode::Time:
    return "Time";
  case SyncMode::Note:
    return "Note";
  case SyncMode::Triplet:
    return "Triplet";
  case SyncMode::Dotted:
    return "Dotted";
  }
  return "Time";
}

void ControlPanel::setDiffuseAmount(float amount) {
  diffuseSlider.setValue(amount, juce::dontSendNotification);
}

void ControlPanel::setStereoLowCut(float freq) {
  stereoLowCutSlider.setValue(freq, juce::dontSendNotification);
}

void ControlPanel::updateLfoAmountAttachment() {
  // Determine which APVTS parameter to bind based on current parameter type
  juce::String paramId;
  switch (currentParameterType) {
  case 0: // Gain
    paramId = "lfoGainAmount";
    break;
  case 1: // Feedback
    paramId = "lfoFeedbackAmount";
    break;
  case 2: // Time/Delay
  default:
    paramId = "lfoTimeAmount";
    break;
  }

  // Destroy old attachment first (must be done before creating new one)
  lfoAmountAttachment.reset();

  // Create new attachment to the appropriate parameter
  lfoAmountAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, paramId, lfoAmountSlider);
}
