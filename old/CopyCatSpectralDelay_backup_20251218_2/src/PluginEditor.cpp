#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
CopyCatSpectralDelayEditor::CopyCatSpectralDelayEditor(
    CopyCatSpectralDelayProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      controlPanel(p.getParameters()) {
  setLookAndFeel(&customLookAndFeel);

  // Title
  titleLabel.setText("SPECTRAL DELAY", juce::dontSendNotification);
  titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(titleLabel);

  // Author
  authorLabel.setText("by CopyCat", juce::dontSendNotification);
  authorLabel.setFont(juce::Font(14.0f));
  authorLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  authorLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(authorLabel);

  // Spectrum display (with integrated curve editing)
  addAndMakeVisible(spectrumDisplay);

  // Control panel
  addAndMakeVisible(controlPanel);

  // Connect control panel parameter type change to update editing curve
  controlPanel.onParameterTypeChanged = [this](int /*paramType*/) {
    updateEditingCurve();
  };

  // Connect curve modification to mark engine needs update
  spectrumDisplay.onCurveModified = [this]() {
    int paramType = controlPanel.getCurrentParameterType();
    if (paramType == 0) {
      processorRef.getEngine().markPhaseNeedsUpdate();
    } else if (paramType == 1) {
      processorRef.getEngine().markGainNeedsUpdate();
    } else {
      processorRef.getEngine().markFeedbackNeedsUpdate();
    }
  };

  // Connect display mode toggle
  controlPanel.onDisplayModeChanged = [this](bool isBar) {
    spectrumDisplay.setDisplayMode(isBar ? SpectrumDisplay::DisplayMode::Bar
                                         : SpectrumDisplay::DisplayMode::Line);
  };

  // Connect spectrum source toggle
  controlPanel.onSpectrumSourceChanged = [this](bool isInput) {
    showInputSpectrum = isInput;
  };

  // Initial curve setup (default to Phase)
  updateEditingCurve();

  // Set plugin window size (wider for better spectrum display)
  setSize(950, 500);

  // Start timer for spectrum update (30 Hz)
  startTimerHz(30);
}

CopyCatSpectralDelayEditor::~CopyCatSpectralDelayEditor() {
  setLookAndFeel(nullptr);
}

//==============================================================================
void CopyCatSpectralDelayEditor::updateEditingCurve() {
  auto &engine = processorRef.getEngine();
  int paramType = controlPanel.getCurrentParameterType();

  if (paramType == 0) {
    // Phase: 0-720 degrees (2 full cycles)
    spectrumDisplay.setEditingCurve(&engine.getPhaseCurve(), 0.0f, 720.0f);
  } else if (paramType == 1) {
    // Gain: 0-200%
    spectrumDisplay.setEditingCurve(&engine.getGainCurve(), 0.0f, 200.0f);
  } else {
    // Feedback: 0-100%
    spectrumDisplay.setEditingCurve(&engine.getFeedbackCurve(), 0.0f, 100.0f);
  }
}

void CopyCatSpectralDelayEditor::paint(juce::Graphics &g) {
  // Background gradient
  juce::ColourGradient gradient(juce::Colour(0xFF1a1a2e), 0.0f, 0.0f,
                                juce::Colour(0xFF16213e), 0.0f,
                                static_cast<float>(getHeight()), false);
  g.setGradientFill(gradient);
  g.fillAll();

  // Divider line between left and right panels
  auto bounds = getLocalBounds();
  int dividerX = static_cast<int>(bounds.getWidth() * 0.70f);
  g.setColour(juce::Colours::white.withAlpha(0.2f));
  g.drawVerticalLine(dividerX, 50.0f,
                     static_cast<float>(bounds.getHeight() - 10));
}

void CopyCatSpectralDelayEditor::resized() {
  auto bounds = getLocalBounds();

  // Title bar
  auto titleArea = bounds.removeFromTop(50);
  titleLabel.setBounds(titleArea.removeFromLeft(250).reduced(15, 10));
  authorLabel.setBounds(titleArea.removeFromRight(150).reduced(15, 10));

  // Split remaining area (70% spectrum, 30% controls)
  int spectrumWidth = static_cast<int>(bounds.getWidth() * 0.70f);

  // Left side: Spectrum display with curve editing
  auto leftArea = bounds.removeFromLeft(spectrumWidth).reduced(10);
  spectrumDisplay.setBounds(leftArea);

  // Right side: Control panel
  auto rightArea = bounds.reduced(10);
  controlPanel.setBounds(rightArea);
}

void CopyCatSpectralDelayEditor::timerCallback() {
  // Update spectrum display from engine
  auto &engine = processorRef.getEngine();

  // Choose spectrum source based on toggle
  const float *spectrumData = showInputSpectrum
                                  ? engine.getInputMagnitudeSpectrum()
                                  : engine.getMagnitudeSpectrum();

  spectrumDisplay.setSpectralData(spectrumData, engine.getNumBins(),
                                  engine.getSampleRate());

  // Trigger repaint
  spectrumDisplay.repaint();
}
