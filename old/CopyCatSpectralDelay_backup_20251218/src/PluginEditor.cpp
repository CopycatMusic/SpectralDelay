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

  // Spectrum display (background)
  addAndMakeVisible(spectrumDisplay);

  // Curve display (overlay on top of spectrum)
  addAndMakeVisible(curveDisplay);

  // Control panel
  addAndMakeVisible(controlPanel);

  // Connect control panel to curve display
  controlPanel.onCurveChanged = [this](const ParameterCurve &curve,
                                       float minVal, float maxVal) {
    curveDisplay.setCurve(curve, minVal, maxVal);
  };

  // Set plugin window size
  setSize(800, 500);

  // Start timer for spectrum update (30 Hz)
  startTimerHz(30);
}

CopyCatSpectralDelayEditor::~CopyCatSpectralDelayEditor() {
  setLookAndFeel(nullptr);
}

//==============================================================================
void CopyCatSpectralDelayEditor::paint(juce::Graphics &g) {
  // Background gradient
  juce::ColourGradient gradient(juce::Colour(0xFF1a1a2e), 0.0f, 0.0f,
                                juce::Colour(0xFF16213e), 0.0f,
                                static_cast<float>(getHeight()), false);
  g.setGradientFill(gradient);
  g.fillAll();

  // Divider line between left and right panels
  auto bounds = getLocalBounds();
  int dividerX = bounds.getWidth() / 2;
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

  // Split remaining area
  int halfWidth = bounds.getWidth() / 2;

  // Left side: Spectrum + Curve overlay
  auto leftArea = bounds.removeFromLeft(halfWidth).reduced(10);
  spectrumDisplay.setBounds(leftArea);
  curveDisplay.setBounds(leftArea);

  // Right side: Control panel
  auto rightArea = bounds.reduced(10);
  controlPanel.setBounds(rightArea);
}

void CopyCatSpectralDelayEditor::timerCallback() {
  // Update spectrum display from engine
  auto &engine = processorRef.getEngine();
  spectrumDisplay.setSpectralData(engine.getMagnitudeSpectrum(),
                                  engine.getNumBins(), engine.getSampleRate());

  // Trigger repaint
  spectrumDisplay.repaint();
}
