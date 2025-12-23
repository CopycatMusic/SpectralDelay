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
  parameterSelector.setSelectedId(1);
  parameterSelector.addListener(this);
  addAndMakeVisible(parameterSelector);

  // Mode selection
  modeLabel.setText("Mode:", juce::dontSendNotification);
  modeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(modeLabel);

  constantModeButton.setButtonText("Constant");
  constantModeButton.setClickingTogglesState(true);
  constantModeButton.setToggleState(true, juce::dontSendNotification);
  constantModeButton.addListener(this);
  addAndMakeVisible(constantModeButton);

  curveModeButton.setButtonText("Curve");
  curveModeButton.setClickingTogglesState(true);
  curveModeButton.addListener(this);
  addAndMakeVisible(curveModeButton);

  // Hidden combo box for curve mode attachment
  curveModeHidden.addItem("Constant", 1);
  curveModeHidden.addItem("Linear", 2);
  curveModeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          parameters, "curveMode", curveModeHidden);

  // Hidden combo box for gain curve mode attachment
  gainCurveModeHidden.addItem("Constant", 1);
  gainCurveModeHidden.addItem("Linear", 2);
  gainCurveModeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          parameters, "gainCurveMode", gainCurveModeHidden);

  // Constant mode controls
  constantValueLabel.setText("Delay:", juce::dontSendNotification);
  constantValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(constantValueLabel);

  constantValueSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  constantValueSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70,
                                      25);
  constantValueSlider.addListener(this);
  addAndMakeVisible(constantValueSlider);
  constantDelayAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "constantPhase", constantValueSlider);

  // Curve mode controls
  slopeLabel.setText("Slope:", juce::dontSendNotification);
  slopeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(slopeLabel);

  slopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  slopeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 25);
  slopeSlider.addListener(this);
  addAndMakeVisible(slopeSlider);
  slopeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "slope", slopeSlider);

  offsetLabel.setText("Offset:", juce::dontSendNotification);
  offsetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(offsetLabel);

  offsetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  offsetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 25);
  offsetSlider.addListener(this);
  addAndMakeVisible(offsetSlider);
  offsetAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "offset", offsetSlider);

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
  mixLabel.setText("Mix:", juce::dontSendNotification);
  mixLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(mixLabel);

  mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  mixSlider.addListener(this);
  addAndMakeVisible(mixSlider);
  mixAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          parameters, "mix", mixSlider);

  // Initialize visibility
  updateControlVisibility();
  updateCurveCallback();
}

ControlPanel::~ControlPanel() {
  parameterSelector.removeListener(this);
  constantModeButton.removeListener(this);
  curveModeButton.removeListener(this);
  constantValueSlider.removeListener(this);
  slopeSlider.removeListener(this);
  offsetSlider.removeListener(this);
  mixSlider.removeListener(this);
}

//==============================================================================
void ControlPanel::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Sections backgrounds
  g.setColour(juce::Colours::white.withAlpha(0.05f));

  // Parameter section
  g.fillRoundedRectangle(bounds.removeFromTop(60).reduced(5), 6.0f);

  // Mode section
  bounds.removeFromTop(5);
  g.fillRoundedRectangle(bounds.removeFromTop(50).reduced(5), 6.0f);

  // Controls section
  bounds.removeFromTop(5);
  g.fillRoundedRectangle(bounds.removeFromTop(140).reduced(5), 6.0f);

  // Bottom section (FFT + Mix)
  g.fillRoundedRectangle(bounds.reduced(5), 6.0f);
}

void ControlPanel::resized() {
  auto bounds = getLocalBounds().reduced(10);

  // Parameter selector row
  auto paramRow = bounds.removeFromTop(40);
  parameterLabel.setBounds(paramRow.removeFromLeft(80).reduced(0, 8));
  parameterSelector.setBounds(paramRow.reduced(0, 5));

  bounds.removeFromTop(15);

  // Mode selection row
  auto modeRow = bounds.removeFromTop(35);
  modeLabel.setBounds(modeRow.removeFromLeft(50).reduced(0, 5));
  constantModeButton.setBounds(modeRow.removeFromLeft(90).reduced(5, 3));
  curveModeButton.setBounds(modeRow.removeFromLeft(90).reduced(5, 3));

  bounds.removeFromTop(15);

  // Controls section
  auto controlsArea = bounds.removeFromTop(130);

  // Constant mode control
  auto constantRow = controlsArea.removeFromTop(35);
  constantValueLabel.setBounds(constantRow.removeFromLeft(60).reduced(0, 5));
  constantValueSlider.setBounds(constantRow.reduced(0, 2));

  controlsArea.removeFromTop(10);

  // Curve mode controls
  auto slopeRow = controlsArea.removeFromTop(35);
  slopeLabel.setBounds(slopeRow.removeFromLeft(60).reduced(0, 5));
  slopeSlider.setBounds(slopeRow.reduced(0, 2));

  controlsArea.removeFromTop(5);

  auto offsetRow = controlsArea.removeFromTop(35);
  offsetLabel.setBounds(offsetRow.removeFromLeft(60).reduced(0, 5));
  offsetSlider.setBounds(offsetRow.reduced(0, 2));

  bounds.removeFromTop(15);

  // FFT settings row
  auto fftRow = bounds.removeFromTop(35);
  fftSizeLabel.setBounds(fftRow.removeFromLeft(65).reduced(0, 5));
  fftSizeSelector.setBounds(fftRow.removeFromLeft(80).reduced(2, 3));
  overlapLabel.setBounds(fftRow.removeFromLeft(60).reduced(5, 5));
  overlapSelector.setBounds(fftRow.removeFromLeft(60).reduced(2, 3));

  bounds.removeFromTop(10);

  // Mix knob row
  auto mixRow = bounds.removeFromTop(100);
  auto mixArea = mixRow.withSizeKeepingCentre(100, 90);
  mixLabel.setBounds(mixArea.removeFromTop(20));
  mixSlider.setBounds(mixArea);
}

//==============================================================================
void ControlPanel::comboBoxChanged(juce::ComboBox *comboBox) {
  if (comboBox == &parameterSelector) {
    currentParameterType = parameterSelector.getSelectedId() - 1;
    updateAttachments();
    updateCurveCallback();
  }
}

void ControlPanel::buttonClicked(juce::Button *button) {
  if (button == &constantModeButton) {
    isConstantMode = true;
    constantModeButton.setToggleState(true, juce::dontSendNotification);
    curveModeButton.setToggleState(false, juce::dontSendNotification);
    // Update the correct curve mode based on current parameter type
    if (currentParameterType == 0) {
      curveModeHidden.setSelectedId(1, juce::sendNotification);
    } else {
      gainCurveModeHidden.setSelectedId(1, juce::sendNotification);
    }
    updateControlVisibility();
    updateCurveCallback();
  } else if (button == &curveModeButton) {
    isConstantMode = false;
    curveModeButton.setToggleState(true, juce::dontSendNotification);
    constantModeButton.setToggleState(false, juce::dontSendNotification);
    // Update the correct curve mode based on current parameter type
    if (currentParameterType == 0) {
      curveModeHidden.setSelectedId(2, juce::sendNotification);
    } else {
      gainCurveModeHidden.setSelectedId(2, juce::sendNotification);
    }
    updateControlVisibility();
    updateCurveCallback();
  }
}

void ControlPanel::sliderValueChanged(juce::Slider *slider) {
  if (slider == &constantValueSlider || slider == &slopeSlider ||
      slider == &offsetSlider) {
    updateCurveCallback();
  }
}

void ControlPanel::updateControlVisibility() {
  // Constant mode controls
  constantValueLabel.setVisible(isConstantMode);
  constantValueSlider.setVisible(isConstantMode);

  // Curve mode controls
  slopeLabel.setVisible(!isConstantMode);
  slopeSlider.setVisible(!isConstantMode);
  offsetLabel.setVisible(!isConstantMode);
  offsetSlider.setVisible(!isConstantMode);
}

void ControlPanel::updateCurveCallback() {
  if (isConstantMode) {
    currentCurve.setMode(ParameterCurve::Mode::Constant);
    currentCurve.setConstantValue(
        static_cast<float>(constantValueSlider.getValue()));
  } else {
    currentCurve.setMode(ParameterCurve::Mode::Linear);
    currentCurve.setLinearParams(static_cast<float>(slopeSlider.getValue()),
                                 static_cast<float>(offsetSlider.getValue()));
  }

  if (onCurveChanged) {
    // Provide different ranges based on parameter type
    if (currentParameterType == 0) {
      // Phase: 0 - 360 degrees
      onCurveChanged(currentCurve, 0.0f, 360.0f);
    } else {
      // Gain: 0 - 200%
      onCurveChanged(currentCurve, 0.0f, 200.0f);
    }
  }
}

void ControlPanel::updateAttachments() {
  // Reset all attachments first
  constantDelayAttachment.reset();
  slopeAttachment.reset();
  offsetAttachment.reset();

  if (currentParameterType == 0) {
    // Phase mode
    constantValueLabel.setText("Phase:", juce::dontSendNotification);
    slopeLabel.setText("Slope:", juce::dontSendNotification);
    offsetLabel.setText("Offset:", juce::dontSendNotification);

    // Rebind to phase parameters
    constantDelayAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "constantPhase", constantValueSlider);
    slopeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "slope", slopeSlider);
    offsetAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "offset", offsetSlider);

    // Sync mode button with phase curve mode
    int curveMode = static_cast<int>(curveModeHidden.getSelectedId());
    isConstantMode = (curveMode == 1);
  } else {
    // Gain mode
    constantValueLabel.setText("Gain:", juce::dontSendNotification);
    slopeLabel.setText("Slope:", juce::dontSendNotification);
    offsetLabel.setText("Offset:", juce::dontSendNotification);

    // Rebind to gain parameters
    constantDelayAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "gainConstant", constantValueSlider);
    slopeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "gainSlope", slopeSlider);
    offsetAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            parameters, "gainOffset", offsetSlider);

    // Sync mode button with gain curve mode
    int curveMode = static_cast<int>(gainCurveModeHidden.getSelectedId());
    isConstantMode = (curveMode == 1);
  }

  // Update button states
  constantModeButton.setToggleState(isConstantMode, juce::dontSendNotification);
  curveModeButton.setToggleState(!isConstantMode, juce::dontSendNotification);

  // Update control visibility
  updateControlVisibility();
}
