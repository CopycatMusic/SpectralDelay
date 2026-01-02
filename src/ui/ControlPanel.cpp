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
  feedbackMultLabel.setText("Multiply:", juce::dontSendNotification);
  feedbackMultLabel.setColour(juce::Label::textColourId,
                              CustomLookAndFeel::getTextPrimary());
  addChildComponent(feedbackMultLabel);

  feedbackMultSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  feedbackMultSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  feedbackMultSlider.onValueChange = [this]() { updateMaxLabels(); };
  addChildComponent(feedbackMultSlider);
  feedbackMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "feedbackMult", feedbackMultSlider);

  // Feedback MAX label
  feedbackMaxLabel.setColour(juce::Label::textColourId,
                             CustomLookAndFeel::getTextPrimary());
  addChildComponent(feedbackMaxLabel);

  feedbackInvertButton.setClickingTogglesState(true);
  feedbackInvertButton.onClick = [this]() {
    if (onFeedbackInvertChanged) {
      onFeedbackInvertChanged(feedbackInvertButton.getToggleState());
    }
  };
  addChildComponent(feedbackInvertButton);

  // Gain multiply control - initially hidden
  gainMultLabel.setText("Multiply:", juce::dontSendNotification);
  gainMultLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(gainMultLabel);

  gainMultSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  gainMultSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  gainMultSlider.onValueChange = [this]() { updateMaxLabels(); };
  addChildComponent(gainMultSlider);
  gainMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "gainMult", gainMultSlider);

  // Gain MAX label
  gainMaxLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  addChildComponent(gainMaxLabel);

  // Delay multiply control - initially hidden
  delayMultLabel.setText("Multiply:", juce::dontSendNotification);
  delayMultLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayMultLabel);

  delayMultSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  delayMultSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  delayMultSlider.onValueChange = [this]() { updateMaxLabels(); };
  addChildComponent(delayMultSlider);
  delayMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "delayMult", delayMultSlider);

  // Delay MAX label
  delayMaxLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayMaxLabel);

  // FFT settings
  fftSizeLabel.setText("FFT Size:", juce::dontSendNotification);
  fftSizeLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(fftSizeLabel);

  fftSizeSelector.addItem("256", 1);
  fftSizeSelector.addItem("512", 2);
  fftSizeSelector.addItem("1024", 3);
  fftSizeSelector.addItem("2048", 4);
  fftSizeSelector.addItem("4096", 5);
  fftSizeSelector.addItem("8192", 6);
  addAndMakeVisible(fftSizeSelector);
  fftSizeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          parameters, "fftSize", fftSizeSelector);

  // Mix control (horizontal slider style)
  mixLabel.setText("Mix:", juce::dontSendNotification);
  mixLabel.setColour(juce::Label::textColourId,
                     CustomLookAndFeel::getTextPrimary());
  addAndMakeVisible(mixLabel);

  mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  mixSlider.addListener(this);
  addAndMakeVisible(mixSlider);
  mixAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "mix", mixSlider);

  // Display mode toggle
  displayModeLabel.setText("Display:", juce::dontSendNotification);
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

  // Top section needs: param buttons (1 row) + mode row (1 row) + spacing
  int topSectionMinHeight = (2 * rowHeight) + (2 * spacing) + margin;
  int topSectionEndY = topSectionMinHeight;

  // Bottom section: FFT, Display, Mix (4 rows)
  int globalRowsCount = 4;
  int globalSectionHeight =
      (globalRowsCount * rowHeight) + (globalRowsCount * spacing) + margin;
  int bottomSeparatorY = getHeight() - globalSectionHeight - margin;

  // Middle section: Multiply label + slider + MAX (3 rows)
  int middleRowsCount = 3;
  int middleSectionHeight =
      (middleRowsCount * rowHeight) + (middleRowsCount * spacing) + margin;
  int middleSeparatorY = bottomSeparatorY - middleSectionHeight - margin;

  // Clamp middle separator to not go above top section
  middleSeparatorY = std::max(middleSeparatorY, topSectionEndY);

  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.2f));

  // Draw separator above middle section (centered in the gap)
  int middleLineY = middleSeparatorY + margin;
  g.fillRect(margin, middleLineY, getWidth() - margin * 2, 1);

  // Draw separator above bottom section (centered in the gap, only if there's
  // room)
  if (bottomSeparatorY > middleSeparatorY + middleSectionHeight) {
    int bottomLineY = bottomSeparatorY + margin;
    g.fillRect(margin, bottomLineY, getWidth() - margin * 2, 1);
  }
}

void ControlPanel::resized() {
  int rowHeight = 25;
  int labelWidth = 80;
  int spacing = 8;
  int margin = 10;

  int width = getWidth();
  int height = getHeight();

  // ============ Calculate section boundaries ============
  // Top section needs: param buttons (1 row) + mode row (1 row) + spacing
  int topSectionMinHeight = (2 * rowHeight) + (2 * spacing) + margin;
  int topSectionEndY = topSectionMinHeight;

  // Bottom section: FFT, Display, Mix (4 rows)
  int globalRowsCount = 4;
  int globalSectionHeight =
      (globalRowsCount * rowHeight) + (globalRowsCount * spacing) + margin;
  int bottomSeparatorY = height - globalSectionHeight - margin;

  // Middle section: Multiply label + slider + MAX (3 rows)
  int middleRowsCount = 3;
  int middleSectionHeight =
      (middleRowsCount * rowHeight) + (middleRowsCount * spacing) + margin;
  int middleSeparatorY = bottomSeparatorY - middleSectionHeight - margin;

  // Clamp middle separator to not go above top section
  middleSeparatorY = std::max(middleSeparatorY, topSectionEndY);
  // ============ MIDDLE SECTION: Multiply controls (fixed position)
  // ============
  int middleStartY = middleSeparatorY + margin + spacing;

  // ============ BOTTOM SECTION: Global settings (fixed at bottom) ============
  int bottomY = bottomSeparatorY + margin + spacing;

  // FFT Size row
  juce::Rectangle<int> fftRow(margin, bottomY, width - margin * 2, rowHeight);
  fftSizeLabel.setBounds(fftRow.removeFromLeft(labelWidth));
  fftRow.removeFromLeft(spacing);
  fftSizeSelector.setBounds(fftRow);
  bottomY += rowHeight + spacing;

  // Display row
  juce::Rectangle<int> displayRow(margin, bottomY, width - margin * 2,
                                  rowHeight);
  displayModeLabel.setBounds(displayRow.removeFromLeft(labelWidth));
  displayRow.removeFromLeft(spacing);
  int buttonAreaWidth = displayRow.getWidth();
  int singleButtonWidth = (buttonAreaWidth - spacing) / 2;
  displayModeButton.setBounds(displayRow.removeFromLeft(singleButtonWidth));
  displayRow.removeFromLeft(spacing);
  spectrumSourceButton.setBounds(displayRow);
  bottomY += rowHeight + spacing;

  spectrumSourceLabel.setVisible(false);

  // Mix label row
  juce::Rectangle<int> mixLabelRow(margin, bottomY, width - margin * 2,
                                   rowHeight);
  mixLabel.setBounds(mixLabelRow);
  bottomY += rowHeight + spacing;

  // Mix slider row
  juce::Rectangle<int> mixSliderRow(margin, bottomY, width - margin * 2,
                                    rowHeight);
  mixSlider.setBounds(mixSliderRow);

  // ============ MIDDLE SECTION: Multiply controls (fixed position)
  // ============
  int midY = middleStartY;

  // Multiply label (show whichever is active)
  juce::Rectangle<int> multLabelRow(margin, midY, width - margin * 2,
                                    rowHeight);
  if (currentParameterType == 0) {
    gainMultLabel.setBounds(multLabelRow);
  } else if (currentParameterType == 1) {
    feedbackMultLabel.setBounds(multLabelRow);
  } else if (currentParameterType == 2 && currentDelayMode) {
    delayMultLabel.setBounds(multLabelRow);
  }
  midY += rowHeight + spacing;

  // Multiply slider
  juce::Rectangle<int> multSliderRow(margin, midY, width - margin * 2,
                                     rowHeight);
  if (currentParameterType == 0) {
    gainMultSlider.setBounds(multSliderRow);
  } else if (currentParameterType == 1) {
    feedbackMultSlider.setBounds(multSliderRow);
  } else if (currentParameterType == 2 && currentDelayMode) {
    delayMultSlider.setBounds(multSliderRow);
  }
  midY += rowHeight + spacing;

  // MAX label
  juce::Rectangle<int> maxLabelRow(margin, midY, width - margin * 2, rowHeight);
  if (currentParameterType == 0) {
    gainMaxLabel.setBounds(maxLabelRow);
  } else if (currentParameterType == 1) {
    feedbackMaxLabel.setBounds(maxLabelRow);
  } else if (currentParameterType == 2 && currentDelayMode) {
    delayMaxLabel.setBounds(maxLabelRow);
  }

  // ============ TOP SECTION: Parameter controls ============
  int topY = margin;

  // Parameter selector buttons row
  juce::Rectangle<int> paramRow(margin, topY, width - margin * 2, rowHeight);
  int buttonWidth = (paramRow.getWidth() - spacing * 2) / 3;
  gainButton.setBounds(paramRow.removeFromLeft(buttonWidth));
  paramRow.removeFromLeft(spacing);
  delayButton.setBounds(paramRow.removeFromLeft(buttonWidth));
  paramRow.removeFromLeft(spacing);
  feedbackButton.setBounds(paramRow);
  topY += rowHeight + spacing;

  // Mode/Invert row (variable content based on parameter type)
  juce::Rectangle<int> modeRow(margin, topY, width - margin * 2, rowHeight);

  if (currentParameterType == 2) {
    // Time: show delay mode toggle
    delayModeLabel.setBounds(modeRow.removeFromLeft(labelWidth));
    modeRow.removeFromLeft(spacing);
    delayModeButton.setBounds(modeRow);
  } else if (currentParameterType == 1) {
    // Feedback: show invert button
    feedbackInvertButton.setBounds(modeRow);
  }
  // Gain: nothing extra in the mode row
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

  bool showFeedbackControls = (paramType == 1);
  feedbackMultLabel.setVisible(showFeedbackControls);
  feedbackMultSlider.setVisible(showFeedbackControls);
  feedbackInvertButton.setVisible(false); // Hidden as requested
  feedbackMaxLabel.setVisible(showFeedbackControls);

  // Show/hide gain multiply control (Gain = paramType 0)
  bool showGainControls = (paramType == 0);
  gainMultLabel.setVisible(showGainControls);
  gainMultSlider.setVisible(showGainControls);
  gainMaxLabel.setVisible(showGainControls);

  // Show/hide delay multiply control (only when Delay mode is active, not
  // Phase)
  bool showDelayControls =
      (paramType == 2); // Force true as Delay mode is default
  delayMultLabel.setVisible(showDelayControls);
  delayMultSlider.setVisible(showDelayControls);
  delayMaxLabel.setVisible(showDelayControls);

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
