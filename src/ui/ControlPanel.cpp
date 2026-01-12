#include "ControlPanel.h"
#include "../dsp/LFOEngine.h"
#include "CustomLookAndFeel.h"

//==============================================================================
ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState &apvts)
    : parameters(apvts) {

  // Parameter selector buttons (no label)

  gainButton.setRadioGroupId(1);
  gainButton.setClickingTogglesState(true);
  gainButton.onClick = [this]() { selectParameter(0); };
  addAndMakeVisible(gainButton);

  feedbackButton.setRadioGroupId(1);
  feedbackButton.setClickingTogglesState(true);
  feedbackButton.onClick = [this]() { selectParameter(1); };
  addAndMakeVisible(feedbackButton);

  delayButton.setRadioGroupId(1);
  delayButton.setClickingTogglesState(true);
  delayButton.setToggleState(true, juce::dontSendNotification);
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
  feedbackMultLabel.setText("Multiply", juce::dontSendNotification);
  feedbackMultLabel.setJustificationType(juce::Justification::centred);
  feedbackMultLabel.setColour(juce::Label::textColourId,
                              CustomLookAndFeel::getTextPrimary());
  addChildComponent(feedbackMultLabel);

  feedbackMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  feedbackMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
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
  gainMultLabel.setText("Multiply", juce::dontSendNotification);
  gainMultLabel.setJustificationType(juce::Justification::centred);
  gainMultLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(gainMultLabel);

  gainMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  gainMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  addChildComponent(gainMultSlider);
  gainMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "gainMult", gainMultSlider);

  // Gain Pre/Post button - initially hidden
  gainPostButton.setClickingTogglesState(true);
  gainPostButton.setButtonText("Pre");
  gainPostButton.setColour(juce::TextButton::buttonOnColourId,
                           CustomLookAndFeel::getAccentPrimary());
  gainPostButton.onClick = [this]() {
    bool isPost = gainPostButton.getToggleState();
    gainPostButton.setButtonText(isPost ? "Post" : "Pre");
    if (onGainPostModeChanged) {
      onGainPostModeChanged(isPost);
    }
  };
  addAndMakeVisible(gainPostButton);

  // Delay multiply control - initially hidden
  delayMultLabel.setText("Multiply", juce::dontSendNotification);
  delayMultLabel.setJustificationType(juce::Justification::centred);
  delayMultLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayMultLabel);

  delayMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  delayMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  addChildComponent(delayMultSlider);
  delayMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "delayMult", delayMultSlider);

  // Stereo width control - only visible for Time/Delay parameter
  stereoLabel.setText("Stereo", juce::dontSendNotification);
  stereoLabel.setJustificationType(juce::Justification::centred);
  stereoLabel.setColour(juce::Label::textColourId,
                        CustomLookAndFeel::getTextPrimary());
  addChildComponent(stereoLabel);

  stereoSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  stereoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  stereoSlider.setRange(0.0, 100.0, 1.0);
  stereoSlider.setValue(0.0);
  stereoSlider.setTextValueSuffix("%");
  stereoSlider.onValueChange = [this]() {
    if (onStereoChanged) {
      onStereoChanged(static_cast<float>(stereoSlider.getValue()));
    }
  };
  addChildComponent(stereoSlider);

  // Stereo Cutoff control
  stereoLowCutLabel.setText("Cutoff", juce::dontSendNotification);
  stereoLowCutLabel.setJustificationType(juce::Justification::centred);
  stereoLowCutLabel.setColour(juce::Label::textColourId,
                              CustomLookAndFeel::getTextPrimary());
  addChildComponent(stereoLowCutLabel);

  stereoLowCutSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  stereoLowCutSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  stereoLowCutSlider.setRange(20.0, 500.0, 1.0);
  stereoLowCutSlider.setValue(150.0);
  stereoLowCutSlider.setTextValueSuffix("Hz");
  stereoLowCutSlider.onValueChange = [this]() {
    if (onStereoLowCutChanged) {
      onStereoLowCutChanged(static_cast<float>(stereoLowCutSlider.getValue()));
    }
  };
  addChildComponent(stereoLowCutSlider);

  // FFT settings (now a knob)
  fftSizeLabel.setText("FFT Size", juce::dontSendNotification);
  fftSizeLabel.setJustificationType(juce::Justification::centred);
  fftSizeLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(fftSizeLabel);

  fftSizeSelector.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  fftSizeSelector.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 65, 15);
  // Range 0-9 corresponds to 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192,
  // 16384
  fftSizeSelector.setRange(0.0, 9.0, 1.0);
  // Custom text display for FFT sizes
  fftSizeSelector.textFromValueFunction = [](double value) {
    int index = static_cast<int>(value);
    int size = 32; // Start from 32
    for (int i = 0; i < index; ++i)
      size *= 2;
    return juce::String(size);
  };
  addAndMakeVisible(fftSizeSelector);

  fftSizeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "fftSize", fftSizeSelector);

  // Mix control
  mixLabel.setText("Mix", juce::dontSendNotification);
  mixLabel.setJustificationType(juce::Justification::centred);
  mixLabel.setColour(juce::Label::textColourId,
                     CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(mixLabel);

  mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 15);
  mixSlider.addListener(this);
  addAndMakeVisible(mixSlider);
  mixAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "mix", mixSlider);

  // High precision delay mode button
  hiResButton.setClickingTogglesState(true);
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

  // LFO mode button (at the top of control panel)
  lfoButton.setClickingTogglesState(true);
  lfoButton.onClick = [this]() {
    setLfoMode(lfoButton.getToggleState());
    if (onLfoModeChanged) {
      onLfoModeChanged(currentLfoMode);
    }
  };
  addAndMakeVisible(lfoButton);

  // LFO Amount slider (0-100%)
  lfoAmountLabel.setText("Amount", juce::dontSendNotification);
  lfoAmountLabel.setJustificationType(juce::Justification::centred);
  lfoAmountLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoAmountLabel);

  lfoAmountSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoAmountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
  lfoAmountSlider.setRange(0.0, 100.0, 1.0);
  lfoAmountSlider.setValue(100.0); // Default 100%
  lfoAmountSlider.setTextValueSuffix("%");
  lfoAmountSlider.onValueChange = [this]() {
    if (onLfoAmountChanged) {
      onLfoAmountChanged(currentParameterType,
                         static_cast<float>(lfoAmountSlider.getValue()));
    }
  };
  addAndMakeVisible(lfoAmountSlider);

  // LFO Speed slider (100-2000ms)
  lfoSpeedLabel.setText("Time", juce::dontSendNotification);
  lfoSpeedLabel.setJustificationType(juce::Justification::centred);
  lfoSpeedLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(lfoSpeedLabel);

  lfoSpeedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
  lfoSpeedSlider.setRange(100.0, 2000.0, 1.0);
  lfoSpeedSlider.setValue(500.0); // Default 500ms
  lfoSpeedSlider.setTextValueSuffix("ms");
  lfoSpeedSlider.onValueChange = [this]() {
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
  lfoSmoothSlider.setRange(0.0, 200.0, 1.0);
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

    lfoBeatModeButton.setButtonText(getSyncModeString(nextMode));
    updateLfoSpeedSliderSettings(nextMode);

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
  lfoHGridSlider.setValue(0.0);
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
  lfoVGridSlider.setValue(0.0);
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
  lfoVGridSlider.onValueChange = [this]() {
    static const int gv[] = {0, 2, 3, 4, 6, 8, 12, 16, 32};
    int idx = static_cast<int>(lfoVGridSlider.getValue());
    int value = gv[juce::jlimit(0, 8, idx)];
    if (onLfoVGridChanged) {
      onLfoVGridChanged(value);
    }
  };
  addAndMakeVisible(lfoVGridSlider);
}

ControlPanel::~ControlPanel() { mixSlider.removeListener(this); }

//==============================================================================
void ControlPanel::paint(juce::Graphics &g) {
  g.fillAll(CustomLookAndFeel::getBackgroundPanel());

  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.3f));

  // Draw separator below parameter selector row (LFO button +
  // Gain/Delay/Feedback) Top section = UIConstants::kMargin + LFO row +
  // UIConstants::kSpacing + param row
  // + UIConstants::kSpacing
  int topSectionEndY = UIConstants::kMargin + UIConstants::kRowHeight +
                       UIConstants::kSpacing + UIConstants::kRowHeight +
                       UIConstants::kSpacing;
  g.fillRect(UIConstants::kMargin, topSectionEndY,
             getWidth() - UIConstants::kMargin * 2, 1);

  // Draw separator above the bottom section only when not in LFO mode
  // (bottom section is hidden in LFO mode)
  {
    // Must match resized() calculations exactly!
    int smallKnobSize = UIConstants::kSmallKnobSize; // 55
    int displayRowHeight = UIConstants::kRowHeight + UIConstants::kSpacing;

    int middleSectionHeight = 0;
    if (currentParameterType == 2) {
      // Side-by-side Time mode height
      // Right side controls: Knob(55) - TextBox is on the side
      middleSectionHeight = (smallKnobSize + UIConstants::kSpacing) * 2;
    } else {
      // Original centered Gain/Feedback mode height
      middleSectionHeight = UIConstants::kLabelHeight +
                            UIConstants::kLargeKnobSize +
                            UIConstants::kTextBoxHeight;
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
  int displayRowHeight = UIConstants::kRowHeight + UIConstants::kSpacing;

  int bottomY = height - UIConstants::kMargin - knobSectionHeight -
                UIConstants::kSpacing - displayRowHeight;

  // --- Display Row: Line/Bar and HiRes on the same row ---
  juce::Rectangle<int> displayRow(UIConstants::kMargin, bottomY,
                                  width - UIConstants::kMargin * 2,
                                  UIConstants::kRowHeight);
  int buttonAreaWidth = displayRow.getWidth();
  int singleButtonWidth = (buttonAreaWidth - UIConstants::kSpacing) / 2;
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

  // FFT Knob (LEFT-WING: Text on Left, Knob on Right)
  int fftX = UIConstants::kMargin + (halfWidth - sliderWidthWithText) / 2;
  // LookAndFeel shifts TextBox to 27-42px. We put Label at 12-27px.
  fftSizeLabel.setBounds(fftX, bottomY + 12, textBoxWidth,
                         UIConstants::kLabelHeight);
  fftSizeSelector.setBounds(fftX, bottomY, sliderWidthWithText,
                            UIConstants::kSmallKnobSize);

  // Mix Knob (RIGHT-WING: Knob on Left, Text on Right)
  int mixX =
      UIConstants::kMargin + halfWidth + (halfWidth - sliderWidthWithText) / 2;
  mixSlider.setBounds(mixX, bottomY, sliderWidthWithText,
                      UIConstants::kSmallKnobSize);
  mixLabel.setBounds(mixX + UIConstants::kSmallKnobSize + textGap, bottomY + 12,
                     textBoxWidth, UIConstants::kLabelHeight);

  bottomY += UIConstants::kSmallKnobSize + UIConstants::kMargin +
             UIConstants::kSpacing;

  spectrumSourceLabel.setVisible(false);

  // ============ TOP SECTION: Auto-arrange from top ============
  int currentY = UIConstants::kMargin;

  // Row 1: LFO button
  juce::Rectangle<int> lfoRow(UIConstants::kMargin, currentY,
                              width - UIConstants::kMargin * 2,
                              UIConstants::kRowHeight);
  lfoButton.setBounds(lfoRow);
  currentY += UIConstants::kRowHeight + UIConstants::kSpacing;

  // Row 2: Parameter selector buttons (Gain, Time, Feedback)
  juce::Rectangle<int> paramRow(UIConstants::kMargin, currentY,
                                width - UIConstants::kMargin * 2,
                                UIConstants::kRowHeight);
  int buttonWidth = (paramRow.getWidth() - UIConstants::kSpacing * 2) / 3;
  gainButton.setBounds(paramRow.removeFromLeft(buttonWidth));
  paramRow.removeFromLeft(UIConstants::kSpacing);
  delayButton.setBounds(paramRow.removeFromLeft(buttonWidth));
  paramRow.removeFromLeft(UIConstants::kSpacing);
  feedbackButton.setBounds(paramRow);
  currentY += UIConstants::kRowHeight + UIConstants::kSpacing;

  // Row 3: Mode row removed (Phase mode removed, always Delay mode)

  // Spacer
  currentY += UIConstants::kSpacing;

  // ============ MULTIPLY CONTROLS or LFO CONTROLS ============
  if (currentLfoMode) {
    // LFO mode: Time, Amount, Smooth aligned with Gain, Delay, Feedback buttons
    int knobWithTextHeight =
        UIConstants::kMediumKnobSize + UIConstants::kTextBoxHeight;
    int lfoRowSpacing = 12;

    // Align knob centers with button centers (Each button is 90px wide)
    int speedX = UIConstants::kMargin + 45 - (UIConstants::kMediumKnobSize / 2);
    int amountX =
        UIConstants::kMargin + 90 + 45 - (UIConstants::kMediumKnobSize / 2);
    int smoothX =
        UIConstants::kMargin + 180 + 45 - (UIConstants::kMediumKnobSize / 2);

    // Labels row
    lfoSpeedLabel.setBounds(speedX, currentY, UIConstants::kMediumKnobSize,
                            UIConstants::kLabelHeight);
    lfoAmountLabel.setBounds(amountX, currentY, UIConstants::kMediumKnobSize,
                             UIConstants::kLabelHeight);
    lfoSmoothLabel.setBounds(smoothX, currentY, UIConstants::kMediumKnobSize,
                             UIConstants::kLabelHeight);

    currentY += UIConstants::kLabelHeight + 2;

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
                                       UIConstants::kRowHeight);
    int optionButtonWidth =
        (lfoOptionsRow.getWidth() - UIConstants::kSpacing) / 2;
    lfoBeatModeButton.setBounds(
        lfoOptionsRow.removeFromLeft(optionButtonWidth));
    lfoOptionsRow.removeFromLeft(UIConstants::kSpacing);
    lfoSyncButton.setBounds(lfoOptionsRow);

    currentY += UIConstants::kRowHeight + (lfoRowSpacing / 2);

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
                            UIConstants::kLabelHeight);
    lfoHGridSlider.setBounds(hGridX, currentY, sliderWidthWithText,
                             UIConstants::kSmallKnobSize);

    // V Grid (Right-Wing: Knob on Left, Text on Right)
    int vGridX = UIConstants::kMargin + halfControlWidth +
                 (halfControlWidth - sliderWidthWithText) / 2;
    lfoVGridSlider.setBounds(vGridX, currentY, sliderWidthWithText,
                             UIConstants::kSmallKnobSize);
    lfoVGridLabel.setBounds(vGridX + UIConstants::kSmallKnobSize + textGap,
                            currentY + 12, textBoxWidth,
                            UIConstants::kLabelHeight);

    currentY += UIConstants::kSmallKnobSize + (lfoRowSpacing / 2);
  } else { // Normal mode: show Multiply knob (CENTERED)

    if (currentParameterType == 2 && !currentLfoMode) {
      // NEW COMPOSITE LAYOUT FOR TIME MODE:
      // Left side: Large Multiply knob (90px)
      // Right side: Two stacked small knobs (50px) for Stereo and Cutoff
      // Textboxes for Stereo/Cutoff are now on the RIGHT.

      // Composite layout centered vs left/right wing height
      int compositeHeight =
          (UIConstants::kSmallKnobSize + UIConstants::kSpacing) * 2;

      int textBoxWidth = 65;
      int textGap = 5;
      int sliderWidthWithText =
          UIConstants::kSmallKnobSize + textBoxWidth + textGap;

      int leftAreaWidth =
          (width - UIConstants::kMargin * 2) * 0.55f; // ~148px for Large knob
      int rightAreaWidth =
          (width - UIConstants::kMargin * 2) - leftAreaWidth; // ~122px

      // LEFT SIDE: Multiply
      int largeKnobWithTextHeight = UIConstants::kLargeKnobSize +
                                    UIConstants::kTextBoxHeight; // 90+15=105
      int largeControlHeight =
          UIConstants::kLabelHeight + largeKnobWithTextHeight; // 15+105=120

      // Vertical centering offset
      int largeYOffset = (compositeHeight - largeControlHeight) / 2;

      delayMultLabel.setBounds(UIConstants::kMargin, currentY + largeYOffset,
                               leftAreaWidth, UIConstants::kLabelHeight);
      delayMultSlider.setBounds(
          UIConstants::kMargin +
              (leftAreaWidth - UIConstants::kLargeKnobSize) / 2,
          currentY + largeYOffset + UIConstants::kLabelHeight,
          UIConstants::kLargeKnobSize, largeKnobWithTextHeight);

      // RIGHT SIDE: Stacked Stereo and Cutoff (Text on Right)
      int rightX = UIConstants::kMargin + leftAreaWidth;

      // Stereo (TOP)
      int stereoKnobY = currentY;
      stereoSlider.setBounds(
          rightX + (rightAreaWidth - sliderWidthWithText) / 2, stereoKnobY,
          sliderWidthWithText, UIConstants::kSmallKnobSize);
      stereoLabel.setBounds(
          rightX + (rightAreaWidth - sliderWidthWithText) / 2 +
              UIConstants::kSmallKnobSize + textGap,
          stereoKnobY + 12, textBoxWidth, UIConstants::kLabelHeight);

      // Cutoff (BOTTOM)
      int cutoffKnobY =
          stereoKnobY + UIConstants::kSmallKnobSize + UIConstants::kSpacing;
      stereoLowCutSlider.setBounds(
          rightX + (rightAreaWidth - sliderWidthWithText) / 2, cutoffKnobY,
          sliderWidthWithText, UIConstants::kSmallKnobSize);
      stereoLowCutLabel.setBounds(
          rightX + (rightAreaWidth - sliderWidthWithText) / 2 +
              UIConstants::kSmallKnobSize + textGap,
          cutoffKnobY + 12, textBoxWidth, UIConstants::kLabelHeight);

      currentY += compositeHeight;
    } else {
      // ORIGINAL CENTERED LAYOUT FOR GAIN/FEEDBACK OR LFO MODE
      // Multiply label
      juce::Rectangle<int> multLabelRow(UIConstants::kMargin, currentY,
                                        width - UIConstants::kMargin * 2,
                                        UIConstants::kLabelHeight);

      if (currentParameterType == 0)
        gainMultLabel.setBounds(multLabelRow);
      else if (currentParameterType == 1)
        feedbackMultLabel.setBounds(multLabelRow);
      else if (currentParameterType == 2)
        delayMultLabel.setBounds(multLabelRow);

      currentY += UIConstants::kLabelHeight;

      // Multiply knob
      int largeKnobWithTextHeight =
          UIConstants::kLargeKnobSize + UIConstants::kTextBoxHeight;
      int knobX = (width - UIConstants::kLargeKnobSize) / 2; // Center X

      if (currentParameterType == 0) {
        gainMultSlider.setBounds(knobX, currentY, UIConstants::kLargeKnobSize,
                                 largeKnobWithTextHeight);
        // Pre/Post button
        int buttonX = knobX + UIConstants::kLargeKnobSize + 10;
        int buttonY =
            currentY +
            (UIConstants::kLargeKnobSize - UIConstants::kRowHeight) / 2;
        gainPostButton.setBounds(buttonX, buttonY, UIConstants::kSmallKnobSize,
                                 UIConstants::kRowHeight);
      } else if (currentParameterType == 1)
        feedbackMultSlider.setBounds(knobX, currentY,
                                     UIConstants::kLargeKnobSize,
                                     largeKnobWithTextHeight);
      else if (currentParameterType == 2)
        delayMultSlider.setBounds(knobX, currentY, UIConstants::kLargeKnobSize,
                                  largeKnobWithTextHeight);

      currentY += largeKnobWithTextHeight;
    }

    // MAX label (Below knob) - HIDDEN
    /*
    juce::Rectangle<int> maxLabelRow(margin, currentY, width - margin * 2, 20);
    if (currentParameterType == 0)
      gainMaxLabel.setBounds(maxLabelRow);
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

  // Show/hide delay multiply control (only when Delay mode is active, not
  // Phase)
  bool showDelayControls = (paramType == 2) && !currentLfoMode;
  delayMultLabel.setVisible(showDelayControls);
  delayMultSlider.setVisible(showDelayControls);

  // Show/hide stereo width controls (only for Time/Delay parameter)
  bool showStereoControls = (paramType == 2) && !currentLfoMode;
  stereoLabel.setVisible(showStereoControls);
  stereoSlider.setVisible(showStereoControls);
  stereoLowCutLabel.setVisible(showStereoControls);
  stereoLowCutSlider.setVisible(showStereoControls);

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
  currentLfoMode = isLfoMode;
  lfoButton.setToggleState(isLfoMode, juce::dontSendNotification);

  // Sync visibility of all components through selectParameter
  selectParameter(currentParameterType);

  // Repaint to update separator line visibility
  repaint();
}

void ControlPanel::updateLfoControls(float amount, float speedOrIndex,
                                     float smoothMs, SyncMode mode,
                                     bool isSynced) {
  lfoAmountSlider.setValue(amount, juce::dontSendNotification);
  lfoSmoothSlider.setValue(smoothMs, juce::dontSendNotification);

  // Update mode buttons
  lfoBeatModeButton.setButtonText(getSyncModeString(mode));
  lfoSyncButton.setToggleState(isSynced, juce::dontSendNotification);
  lfoSyncButton.setButtonText(isSynced ? "Sync On" : "Sync Off");

  // Update speed slider range and value based on mode
  updateLfoSpeedSliderSettings(mode);
  lfoSpeedSlider.setValue(speedOrIndex, juce::dontSendNotification);
}

void ControlPanel::setGainPostMode(bool post) {
  gainPostButton.setToggleState(post, juce::dontSendNotification);
  gainPostButton.setButtonText(post ? "Post" : "Pre");
}

void ControlPanel::updateLfoSpeedSliderSettings(SyncMode mode) {
  if (mode != SyncMode::Time) {
    lfoSpeedSlider.setRange(0.0, 8.0,
                            1.0); // Integer steps for beat subdivisions
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
    // Time mode: 100ms to 10000ms (10s), non-linear with 1000ms (1s) at center
    lfoSpeedSlider.setRange(100.0, 10000.0, 1.0);
    lfoSpeedSlider.setSkewFactorFromMidPoint(1000.0); // 1s at center
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
      // Default: assume milliseconds
      return text.getDoubleValue();
    };
  }
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
