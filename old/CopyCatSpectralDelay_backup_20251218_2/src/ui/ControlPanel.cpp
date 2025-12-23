#include "ControlPanel.h"

//==============================================================================
ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState &apvts)
    : parameters(apvts) {

  // Parameter selector
  parameterLabel.setText("Parameter:", juce::dontSendNotification);
  parameterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(parameterLabel);

  parameterSelector.addItem("Phase", 1);
  parameterSelector.addItem("Gain", 2);
  parameterSelector.addItem("Feedback", 3);
  parameterSelector.setSelectedId(1);
  parameterSelector.addListener(this);
  addAndMakeVisible(parameterSelector);

  // FFT settings
  fftSizeLabel.setText("FFT Size:", juce::dontSendNotification);
  fftSizeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
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

  overlapLabel.setText("Overlap:", juce::dontSendNotification);
  overlapLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(overlapLabel);

  overlapSelector.addItem("2x", 1);
  overlapSelector.addItem("4x", 2);
  overlapSelector.addItem("8x", 3);
  addAndMakeVisible(overlapSelector);
  overlapAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          parameters, "overlap", overlapSelector);

  // Mix control
  mixLabel.setText("Mix", juce::dontSendNotification);
  mixLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  mixLabel.setJustificationType(juce::Justification::centred);
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
  displayModeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
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
                                juce::Colours::white);
  addAndMakeVisible(spectrumSourceLabel);

  spectrumSourceButton.setButtonText("Display Output");
  spectrumSourceButton.setClickingTogglesState(true);
  spectrumSourceButton.onClick = [this]() {
    bool isInput = spectrumSourceButton.getToggleState();
    spectrumSourceButton.setButtonText(isInput ? "Display Input"
                                               : "Display Output");
    if (onSpectrumSourceChanged) {
      onSpectrumSourceChanged(isInput);
    }
  };
  addAndMakeVisible(spectrumSourceButton);
}

ControlPanel::~ControlPanel() {
  parameterSelector.removeListener(this);
  mixSlider.removeListener(this);
}

//==============================================================================
void ControlPanel::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF161b22));

  // Draw border
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRect(getLocalBounds(), 1);

  // Draw section dividers
  g.setColour(juce::Colours::white.withAlpha(0.05f));
  g.fillRect(0, 55, getWidth(), 1);  // After parameter selector
  g.fillRect(0, 145, getWidth(), 1); // Before mix control
}

void ControlPanel::resized() {
  auto bounds = getLocalBounds().reduced(10);
  int rowHeight = 25;
  int labelWidth = 80;
  int spacing = 5;

  // Parameter selector row
  auto paramRow = bounds.removeFromTop(rowHeight);
  parameterLabel.setBounds(paramRow.removeFromLeft(labelWidth));
  parameterSelector.setBounds(paramRow.reduced(spacing, 0));

  bounds.removeFromTop(15); // Section spacing

  // Instruction text area (where modes/sliders used to be)
  // We'll just leave some space for the user to click on the spectrum

  bounds.removeFromTop(60); // Reserved space

  // FFT settings
  auto fftRow = bounds.removeFromTop(rowHeight);
  fftSizeLabel.setBounds(fftRow.removeFromLeft(labelWidth));
  fftSizeSelector.setBounds(fftRow.reduced(spacing, 0));

  bounds.removeFromTop(spacing);

  auto overlapRow = bounds.removeFromTop(rowHeight);
  overlapLabel.setBounds(overlapRow.removeFromLeft(labelWidth));
  overlapSelector.setBounds(overlapRow.reduced(spacing, 0));

  bounds.removeFromTop(spacing);

  // Display mode toggle
  auto displayModeRow = bounds.removeFromTop(rowHeight);
  displayModeLabel.setBounds(displayModeRow.removeFromLeft(labelWidth));
  displayModeButton.setBounds(displayModeRow.reduced(spacing, 0));

  bounds.removeFromTop(spacing);

  // Spectrum source toggle
  auto spectrumSourceRow = bounds.removeFromTop(rowHeight);
  spectrumSourceLabel.setBounds(spectrumSourceRow.removeFromLeft(labelWidth));
  spectrumSourceButton.setBounds(spectrumSourceRow.reduced(spacing, 0));

  bounds.removeFromTop(20); // Section spacing

  // Mix control at bottom
  auto mixArea = bounds.removeFromBottom(100);
  mixLabel.setBounds(mixArea.removeFromTop(20));
  mixSlider.setBounds(mixArea);
}

//==============================================================================
void ControlPanel::comboBoxChanged(juce::ComboBox *comboBox) {
  if (comboBox == &parameterSelector) {
    currentParameterType = parameterSelector.getSelectedId() - 1;
    if (onParameterTypeChanged) {
      onParameterTypeChanged(currentParameterType);
    }
  }
}

void ControlPanel::sliderValueChanged(juce::Slider * /*slider*/) {
  // Mix slider is auto-attached, no manual handling needed
}
