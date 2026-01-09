#include "ControlPanel.h"
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
    delayMaxLabel.setVisible(showDelayMult);
    updateMaxLabels();
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
  feedbackMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  feedbackMultSlider.onValueChange = [this]() { updateMaxLabels(); };
  addChildComponent(feedbackMultSlider);
  feedbackMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "feedbackMult", feedbackMultSlider);

  // Feedback MAX label
  feedbackMaxLabel.setColour(juce::Label::textColourId,
                             CustomLookAndFeel::getTextPrimary());
  feedbackMaxLabel.setJustificationType(juce::Justification::centred);
  addChildComponent(feedbackMaxLabel);

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
  gainMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  gainMultSlider.onValueChange = [this]() { updateMaxLabels(); };
  addChildComponent(gainMultSlider);
  gainMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "gainMult", gainMultSlider);

  // Gain MAX label
  gainMaxLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  gainMaxLabel.setJustificationType(juce::Justification::centred);
  addChildComponent(gainMaxLabel);

  // Delay multiply control - initially hidden
  delayMultLabel.setText("Multiply", juce::dontSendNotification);
  delayMultLabel.setJustificationType(juce::Justification::centred);
  delayMultLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayMultLabel);

  delayMultSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  delayMultSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  delayMultSlider.onValueChange = [this]() { updateMaxLabels(); };
  addChildComponent(delayMultSlider);
  delayMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "delayMult", delayMultSlider);

  // Delay MAX label
  delayMaxLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  delayMaxLabel.setJustificationType(juce::Justification::centred);
  addChildComponent(delayMaxLabel);

  // FFT settings (now a knob)
  fftSizeLabel.setText("FFT Size", juce::dontSendNotification);
  fftSizeLabel.setJustificationType(juce::Justification::centred);
  fftSizeLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(fftSizeLabel);

  fftSizeSelector.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  fftSizeSelector.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  // Range 0-10 corresponds to 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192,
  // 16384, 32768
  fftSizeSelector.setRange(0.0, 10.0, 1.0);
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
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  mixSlider.addListener(this);
  addAndMakeVisible(mixSlider);
  mixAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "mix", mixSlider);

  // Display mode toggle
  displayModeLabel.setText("Display:", juce::dontSendNotification);
  displayModeLabel.setJustificationType(juce::Justification::left);
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
  addChildComponent(lfoAmountLabel);

  lfoAmountSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoAmountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  lfoAmountSlider.setRange(0.0, 100.0, 1.0);
  lfoAmountSlider.setValue(100.0); // Default 100%
  lfoAmountSlider.setTextValueSuffix("%");
  lfoAmountSlider.onValueChange = [this]() {
    if (onLfoAmountChanged) {
      onLfoAmountChanged(currentParameterType,
                         static_cast<float>(lfoAmountSlider.getValue()));
    }
  };
  addChildComponent(lfoAmountSlider);

  // LFO Speed slider (100-2000ms)
  lfoSpeedLabel.setText("Time", juce::dontSendNotification);
  lfoSpeedLabel.setJustificationType(juce::Justification::centred);
  lfoSpeedLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(lfoSpeedLabel);

  lfoSpeedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  lfoSpeedSlider.setRange(100.0, 2000.0, 1.0);
  lfoSpeedSlider.setValue(500.0); // Default 500ms
  lfoSpeedSlider.setTextValueSuffix("ms");
  lfoSpeedSlider.onValueChange = [this]() {
    if (onLfoSpeedChanged) {
      onLfoSpeedChanged(currentParameterType,
                        static_cast<float>(lfoSpeedSlider.getValue()));
    }
  };
  addChildComponent(lfoSpeedSlider);

  // LFO Smooth slider (0-100ms, displayed as 0-100%)
  lfoSmoothLabel.setText("Smooth", juce::dontSendNotification);
  lfoSmoothLabel.setJustificationType(juce::Justification::centred);
  lfoSmoothLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(lfoSmoothLabel);

  lfoSmoothSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  lfoSmoothSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  lfoSmoothSlider.setRange(0.0, 100.0, 1.0);
  lfoSmoothSlider.setValue(0.0); // Default 0ms (no smooth)
  lfoSmoothSlider.setTextValueSuffix("%");
  lfoSmoothSlider.onValueChange = [this]() {
    if (onLfoSmoothChanged) {
      onLfoSmoothChanged(currentParameterType,
                         static_cast<float>(lfoSmoothSlider.getValue()));
    }
  };
  addChildComponent(lfoSmoothSlider);
}

ControlPanel::~ControlPanel() { mixSlider.removeListener(this); }

//==============================================================================
void ControlPanel::paint(juce::Graphics &g) {
  g.fillAll(CustomLookAndFeel::getBackgroundPanel());

  // Draw border
  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.1f));
  g.drawRect(getLocalBounds(), 1);

  int rowHeight = 25;
  int spacing = 8;
  int margin = 10;

  // Bottom section: FFT & Mix (Knob Height) + Display (Row Height)
  // Mix and FFT are same row now
  int knobHeight = 90; // Approx height for knob + label + text
  int globalSectionHeight =
      knobHeight + (rowHeight + spacing) + (spacing * 2) + margin;
  int bottomSeparatorY = getHeight() - globalSectionHeight - margin;

  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.2f));

  // Draw separator above bottom section
  int bottomLineY = bottomSeparatorY + margin;
  g.fillRect(margin, bottomLineY, getWidth() - margin * 2, 1);
}

void ControlPanel::resized() {
  int rowHeight = 25;
  int labelWidth = 80;
  int spacing = 8;
  int margin = 10;
  int knobSize = 80; // Size of the rotary knob area

  int width = getWidth();
  int height = getHeight();

  // ============ BOTTOM SECTION: Fixed at bottom ============

  // Mix and FFT are 20 (Label) + 80 (Knob) + spacing
  int knobSectionHeight = 20 + knobSize;

  // Display row
  int displayRowHeight = rowHeight + spacing;

  int bottomY =
      height - margin - displayRowHeight - margin - knobSectionHeight - spacing;

  // --- Display Row ---
  juce::Rectangle<int> displayRow(margin, bottomY, width - margin * 2,
                                  rowHeight);
  displayModeLabel.setBounds(displayRow.removeFromLeft(labelWidth));
  displayRow.removeFromLeft(spacing);
  int buttonAreaWidth = displayRow.getWidth();
  int singleButtonWidth = (buttonAreaWidth - spacing) / 2;
  displayModeButton.setBounds(displayRow.removeFromLeft(singleButtonWidth));
  displayRow.removeFromLeft(spacing);
  spectrumSourceButton.setBounds(displayRow);

  bottomY += displayRowHeight + margin;

  // --- Labels Row for FFT and Mix ---
  juce::Rectangle<int> labelsRow(margin, bottomY, width - margin * 2, 20);

  // FFT Label (Left half)
  juce::Rectangle<int> fftLabelArea =
      labelsRow.removeFromLeft((width - margin * 2) / 2);
  fftSizeLabel.setBounds(fftLabelArea);

  // Mix Label (Right half)
  mixLabel.setBounds(labelsRow);

  bottomY += 20;

  // --- Knobs Row for FFT and Mix ---
  int halfWidth = (width - margin * 2) / 2;

  // FFT Knob (Left Center)
  fftSizeSelector.setBounds(margin + halfWidth / 2 - knobSize / 2, bottomY,
                            knobSize, knobSize);

  // Mix Knob (Right Center)
  mixSlider.setBounds(margin + halfWidth + halfWidth / 2 - knobSize / 2,
                      bottomY, knobSize, knobSize);

  bottomY += knobSize + margin + spacing;

  spectrumSourceLabel.setVisible(false);

  // ============ TOP SECTION: Auto-arrange from top ============
  int currentY = margin;

  // Row 1: LFO button
  juce::Rectangle<int> lfoRow(margin, currentY, width - margin * 2, rowHeight);
  lfoButton.setBounds(lfoRow);
  currentY += rowHeight + spacing;

  // Row 2: Parameter selector buttons (Gain, Time, Feedback)
  juce::Rectangle<int> paramRow(margin, currentY, width - margin * 2,
                                rowHeight);
  int buttonWidth = (paramRow.getWidth() - spacing * 2) / 3;
  gainButton.setBounds(paramRow.removeFromLeft(buttonWidth));
  paramRow.removeFromLeft(spacing);
  delayButton.setBounds(paramRow.removeFromLeft(buttonWidth));
  paramRow.removeFromLeft(spacing);
  feedbackButton.setBounds(paramRow);
  currentY += rowHeight + spacing;

  // Row 3: Mode/Invert row (optional, based on parameter type)
  if (currentParameterType == 2 && delayModeButton.isVisible()) {
    juce::Rectangle<int> modeRow(margin, currentY, width - margin * 2,
                                 rowHeight);
    delayModeLabel.setBounds(modeRow.removeFromLeft(labelWidth));
    modeRow.removeFromLeft(spacing);
    delayModeButton.setBounds(modeRow);
    currentY += rowHeight + spacing;
  }

  // Spacer
  currentY += spacing;

  // ============ MULTIPLY CONTROLS or LFO CONTROLS ============
  if (currentLfoMode) {
    // LFO mode: Time, Amount, Smooth aligned with Gain, Delay, Feedback buttons
    int smallKnobSize = 65;
    int textBoxHeight = 20;
    int knobWithTextHeight = smallKnobSize + textBoxHeight;
    int lfoRowSpacing = 12;
    int labelHeight = 20; // Assuming label height is 20px

    // Align knob centers with button centers (Each button is 90px wide)
    int speedX = margin + 45 - (smallKnobSize / 2);
    int amountX = margin + 90 + 45 - (smallKnobSize / 2);
    int smoothX = margin + 180 + 45 - (smallKnobSize / 2);

    // Labels row
    lfoSpeedLabel.setBounds(speedX, currentY, smallKnobSize, labelHeight);
    lfoAmountLabel.setBounds(amountX, currentY, smallKnobSize, labelHeight);
    lfoSmoothLabel.setBounds(smoothX, currentY, smallKnobSize, labelHeight);

    currentY += labelHeight + 2;

    // Knobs row
    lfoSpeedSlider.setBounds(speedX, currentY, smallKnobSize,
                             knobWithTextHeight);
    lfoAmountSlider.setBounds(amountX, currentY, smallKnobSize,
                              knobWithTextHeight);
    lfoSmoothSlider.setBounds(smoothX, currentY, smallKnobSize,
                              knobWithTextHeight);

    currentY += knobWithTextHeight + lfoRowSpacing;
  } else { // Normal mode: show Multiply knob (CENTERED)

    // Multiply label
    juce::Rectangle<int> multLabelRow(margin, currentY, width - margin * 2, 20);

    if (currentParameterType == 0)
      gainMultLabel.setBounds(multLabelRow);
    else if (currentParameterType == 1)
      feedbackMultLabel.setBounds(multLabelRow);
    else if (currentParameterType == 2)
      delayMultLabel.setBounds(multLabelRow);

    currentY += 20; // Label height

    // Multiply knob
    juce::Rectangle<int> knobRow(margin, currentY, width - margin * 2,
                                 knobSize);
    int knobX = (width - knobSize) / 2; // Center X

    if (currentParameterType == 0)
      gainMultSlider.setBounds(knobX, currentY, knobSize, knobSize);
    else if (currentParameterType == 1)
      feedbackMultSlider.setBounds(knobX, currentY, knobSize, knobSize);
    else if (currentParameterType == 2)
      delayMultSlider.setBounds(knobX, currentY, knobSize, knobSize);

    currentY += knobSize;

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
  feedbackMaxLabel.setVisible(showFeedbackControls);

  // Show/hide gain multiply control (Gain = paramType 0)
  bool showGainControls = (paramType == 0) && !currentLfoMode;
  gainMultLabel.setVisible(showGainControls);
  gainMultSlider.setVisible(showGainControls);
  gainMaxLabel.setVisible(showGainControls);

  // Show/hide delay multiply control (only when Delay mode is active, not
  // Phase)
  bool showDelayControls = (paramType == 2) && !currentLfoMode;
  delayMultLabel.setVisible(showDelayControls);
  delayMultSlider.setVisible(showDelayControls);
  delayMaxLabel.setVisible(showDelayControls);

  // Show/hide LFO controls based on LFO mode
  lfoAmountLabel.setVisible(currentLfoMode);
  lfoAmountSlider.setVisible(currentLfoMode);
  lfoSpeedLabel.setVisible(currentLfoMode);
  lfoSpeedSlider.setVisible(currentLfoMode);
  lfoSmoothLabel.setVisible(currentLfoMode);
  lfoSmoothSlider.setVisible(currentLfoMode);

  updateMaxLabels(); // Update MAX label values
  resized();         // Update layout

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

void ControlPanel::updateMaxLabels() {
  // Gain: base max 200%, scaled by multiply slider (0-100%)
  float gainMult = static_cast<float>(gainMultSlider.getValue()) / 100.0f;
  float gainMax = 200.0f * gainMult;
  gainMaxLabel.setText("MAX: " + juce::String(static_cast<int>(gainMax)) + "%",
                       juce::dontSendNotification);

  // Feedback: base max 100%, scaled by multiply slider (0-100%)
  float feedbackMult =
      static_cast<float>(feedbackMultSlider.getValue()) / 100.0f;
  float feedbackMax = 100.0f * feedbackMult;
  feedbackMaxLabel.setText(
      "MAX: " + juce::String(static_cast<int>(feedbackMax)) + "%",
      juce::dontSendNotification);

  // Delay: base max 2000ms, scaled by multiply slider (0-100%)
  float delayMult = static_cast<float>(delayMultSlider.getValue()) / 100.0f;
  float delayMax = 2000.0f * delayMult;
  delayMaxLabel.setText("MAX: " + juce::String(static_cast<int>(delayMax)) +
                            "ms",
                        juce::dontSendNotification);
}

void ControlPanel::setLfoMode(bool isLfoMode) {
  currentLfoMode = isLfoMode;
  lfoButton.setToggleState(isLfoMode, juce::dontSendNotification);

  // Show/hide LFO controls
  lfoAmountLabel.setVisible(isLfoMode);
  lfoAmountSlider.setVisible(isLfoMode);
  lfoSpeedLabel.setVisible(isLfoMode);
  lfoSpeedSlider.setVisible(isLfoMode);
  lfoSmoothLabel.setVisible(isLfoMode);
  lfoSmoothSlider.setVisible(isLfoMode);

  // Hide normal Multiply controls in LFO mode
  if (isLfoMode) {
    gainMultLabel.setVisible(false);
    gainMultSlider.setVisible(false);
    gainMaxLabel.setVisible(false);
    feedbackMultLabel.setVisible(false);
    feedbackMultSlider.setVisible(false);
    feedbackMaxLabel.setVisible(false);
    delayMultLabel.setVisible(false);
    delayMultSlider.setVisible(false);
    delayMaxLabel.setVisible(false);
  } else {
    selectParameter(currentParameterType); // Restore normal visibility
  }

  resized();
}

void ControlPanel::updateLfoSliders(float amount, float speedMs,
                                    float smoothMs) {
  lfoAmountSlider.setValue(amount, juce::dontSendNotification);
  lfoSpeedSlider.setValue(speedMs, juce::dontSendNotification);
  lfoSmoothSlider.setValue(smoothMs, juce::dontSendNotification);
}
