#include "ControlPanel.h"
#include "CustomLookAndFeel.h"

//==============================================================================
ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState &apvts)
    : parameters(apvts) {

  // Parameter selector buttons (no label)

  gainButton.setRadioGroupId(1);
  gainButton.setClickingTogglesState(true);
  gainButton.setToggleState(true, juce::dontSendNotification);
  gainButton.onClick = [this]() { selectParameter(0); };
  addAndMakeVisible(gainButton);

  feedbackButton.setRadioGroupId(1);
  feedbackButton.setClickingTogglesState(true);
  feedbackButton.onClick = [this]() { selectParameter(1); };
  addAndMakeVisible(feedbackButton);

  delayButton.setRadioGroupId(1);
  delayButton.setClickingTogglesState(true);
  delayButton.onClick = [this]() { selectParameter(2); };
  addAndMakeVisible(delayButton);

  // Delay mode toggle (Phase vs Delay) - initially hidden
  delayModeButton.setButtonText("Delay");
  delayModeButton.setToggleState(true, juce::dontSendNotification);
  delayModeButton.setClickingTogglesState(true);
  delayModeButton.onClick = [this]() {
    currentDelayMode = delayModeButton.getToggleState();
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
  feedbackMultLabel.setText("Multiply:", juce::dontSendNotification);
  feedbackMultLabel.setColour(juce::Label::textColourId,
                              CustomLookAndFeel::getTextPrimary());
  addChildComponent(feedbackMultLabel);

  feedbackMultSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  feedbackMultSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
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
  gainMultLabel.setText("Multiply:", juce::dontSendNotification);
  gainMultLabel.setColour(juce::Label::textColourId,
                          CustomLookAndFeel::getTextPrimary());
  addChildComponent(gainMultLabel);

  gainMultSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  gainMultSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  addChildComponent(gainMultSlider);
  gainMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "gainMult", gainMultSlider);

  // Delay multiply control - initially hidden
  delayMultLabel.setText("Multiply:", juce::dontSendNotification);
  delayMultLabel.setColour(juce::Label::textColourId,
                           CustomLookAndFeel::getTextPrimary());
  addChildComponent(delayMultLabel);

  delayMultSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  delayMultSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  addChildComponent(delayMultSlider);
  delayMultAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "delayMult", delayMultSlider);

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

  // Draw separator line between top (parameter controls) and bottom (global
  // settings)
  int rowHeight = 25;
  int spacing = 8;
  int margin = 10;
  int globalRowsCount = 4; // FFT, Display (with Source), Mix Label, Mix Slider
  int globalSectionHeight =
      (globalRowsCount * rowHeight) + (globalRowsCount * spacing) + margin;
  int separatorY = getHeight() - globalSectionHeight - margin;

  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.2f));
  g.fillRect(margin, separatorY, getWidth() - margin * 2, 1);
}

void ControlPanel::resized() {
  int rowHeight = 25;
  int labelWidth = 80;
  int spacing = 8;
  int margin = 10;

  int width = getWidth();
  int height = getHeight();

  // ============ BOTTOM SECTION: Global settings (fixed at bottom) ============
  int globalRowsCount = 4; // FFT, Display (with Source), Mix Label, Mix Slider
  int globalSectionHeight =
      (globalRowsCount * rowHeight) + (globalRowsCount * spacing) + margin;
  int separatorY = height - globalSectionHeight - margin;

  int bottomY = separatorY + margin + spacing;

  // FFT Size row
  juce::Rectangle<int> fftRow(margin, bottomY, width - margin * 2, rowHeight);
  fftSizeLabel.setBounds(fftRow.removeFromLeft(labelWidth));
  fftRow.removeFromLeft(spacing); // Add spacing after label
  fftSizeSelector.setBounds(fftRow);
  bottomY += rowHeight + spacing;

  // Display row (combined: Display label + Bar/Line button + Input/Output
  // button)
  juce::Rectangle<int> displayRow(margin, bottomY, width - margin * 2,
                                  rowHeight);
  displayModeLabel.setBounds(displayRow.removeFromLeft(labelWidth));
  displayRow.removeFromLeft(spacing); // Add spacing after label
  // Split remaining space for two buttons
  int buttonAreaWidth = displayRow.getWidth();
  int singleButtonWidth = (buttonAreaWidth - spacing) / 2;
  displayModeButton.setBounds(displayRow.removeFromLeft(singleButtonWidth));
  displayRow.removeFromLeft(spacing);
  spectrumSourceButton.setBounds(displayRow);
  bottomY += rowHeight + spacing;

  // Hide spectrum source label (no longer needed)
  spectrumSourceLabel.setVisible(false);

  // Mix label row
  juce::Rectangle<int> mixLabelRow(margin, bottomY, width - margin * 2,
                                   rowHeight);
  mixLabel.setBounds(mixLabelRow);
  bottomY += rowHeight + spacing;

  // Mix slider row (full width)
  juce::Rectangle<int> mixSliderRow(margin, bottomY, width - margin * 2,
                                    rowHeight);
  mixSliderRow.removeFromLeft(spacing); // Add left margin
  mixSlider.setBounds(mixSliderRow.reduced(spacing, 0));

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

  // Delay mode toggle (only visible when Time is selected)
  if (currentParameterType == 2) {
    topY += rowHeight + spacing;
    juce::Rectangle<int> delayModeRow(margin, topY, width - margin * 2,
                                      rowHeight);
    delayModeLabel.setBounds(delayModeRow.removeFromLeft(labelWidth));
    delayModeRow.removeFromLeft(spacing); // Add spacing after label
    delayModeButton.setBounds(delayModeRow);
  }

  // Feedback controls (only visible when Feedback is selected)
  if (currentParameterType == 1) {
    topY += rowHeight + spacing;
    juce::Rectangle<int> invertRow(margin, topY, width - margin * 2, rowHeight);
    feedbackInvertButton.setBounds(invertRow);

    // Multiply label on its own row
    topY += rowHeight + spacing;
    juce::Rectangle<int> multLabelRow(margin, topY, width - margin * 2,
                                      rowHeight);
    feedbackMultLabel.setBounds(multLabelRow);
    // Slider on next row (full width)
    topY += rowHeight + spacing;
    juce::Rectangle<int> multSliderRow(margin, topY, width - margin * 2,
                                       rowHeight);
    feedbackMultSlider.setBounds(multSliderRow.reduced(spacing, 0));
  }

  // Gain multiply control (only visible when Gain is selected)
  if (currentParameterType == 0) {
    // Multiply label on its own row
    topY += rowHeight + spacing;
    juce::Rectangle<int> gainMultLabelRow(margin, topY, width - margin * 2,
                                          rowHeight);
    gainMultLabel.setBounds(gainMultLabelRow);
    // Slider on next row (full width)
    topY += rowHeight + spacing;
    juce::Rectangle<int> gainMultSliderRow(margin, topY, width - margin * 2,
                                           rowHeight);
    gainMultSlider.setBounds(gainMultSliderRow.reduced(spacing, 0));
  }

  // Delay multiply control (only visible when Delay mode is active)
  if (currentParameterType == 2 && currentDelayMode) {
    // Multiply label on its own row
    topY += rowHeight + spacing;
    juce::Rectangle<int> delayMultLabelRow(margin, topY, width - margin * 2,
                                           rowHeight);
    delayMultLabel.setBounds(delayMultLabelRow);
    // Slider on next row (full width)
    topY += rowHeight + spacing;
    juce::Rectangle<int> delayMultSliderRow(margin, topY, width - margin * 2,
                                            rowHeight);
    delayMultSlider.setBounds(delayMultSliderRow.reduced(spacing, 0));
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
  bool showModeToggle = (paramType == 2);
  delayModeButton.setVisible(showModeToggle);
  delayModeLabel.setVisible(showModeToggle);

  // Show/hide feedback controls based on parameter selection
  bool showFeedbackControls = (paramType == 1);
  feedbackMultLabel.setVisible(showFeedbackControls);
  feedbackMultSlider.setVisible(showFeedbackControls);
  feedbackInvertButton.setVisible(showFeedbackControls);

  // Show/hide gain multiply control (Gain = paramType 0)
  bool showGainControls = (paramType == 0);
  gainMultLabel.setVisible(showGainControls);
  gainMultSlider.setVisible(showGainControls);

  // Show/hide delay multiply control (only when Delay mode is active, not
  // Phase)
  bool showDelayControls = (paramType == 2) && currentDelayMode;
  delayMultLabel.setVisible(showDelayControls);
  delayMultSlider.setVisible(showDelayControls);

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
