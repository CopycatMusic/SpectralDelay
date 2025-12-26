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
  titleLabel.setColour(juce::Label::textColourId,
                       CustomLookAndFeel::getTextPrimary());
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(titleLabel);

  // Version (top right)
  versionLabel.setText("v0.2.4beta", juce::dontSendNotification);
  versionLabel.setFont(juce::Font(14.0f));
  versionLabel.setColour(juce::Label::textColourId,
                         CustomLookAndFeel::getTextPrimary());
  versionLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(versionLabel);

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
      processorRef.getEngine().markGainNeedsUpdate();
    } else if (paramType == 1) {
      processorRef.getEngine().markFeedbackNeedsUpdate();
    } else {
      // Delay parameter: update Phase or Delay curve based on mode
      if (controlPanel.isDelayMode()) {
        processorRef.getEngine().markDelayNeedsUpdate();
      } else {
        processorRef.getEngine().markPhaseNeedsUpdate();
      }
    }
    // Store UI state for DAW recall
    processorRef.setUIParameterType(paramType);
  };

  // Connect display mode toggle
  controlPanel.onDisplayModeChanged = [this](bool isBar) {
    spectrumDisplay.setDisplayMode(isBar ? SpectrumDisplay::DisplayMode::Bar
                                         : SpectrumDisplay::DisplayMode::Line);
    processorRef.setUIDisplayModeBar(isBar);
    // Update APVTS parameter for DAW sync
    if (auto *param =
            processorRef.getParameters().getParameter("displayMode")) {
      param->setValueNotifyingHost(isBar ? 1.0f : 0.0f);
    }
  };

  // Connect spectrum source toggle
  controlPanel.onSpectrumSourceChanged = [this](bool isInput) {
    showInputSpectrum = isInput;
    processorRef.setUIShowInputSpectrum(isInput);
    // Update APVTS parameter for DAW sync
    if (auto *param =
            processorRef.getParameters().getParameter("spectrumSource")) {
      param->setValueNotifyingHost(isInput ? 1.0f : 0.0f);
    }
  };

  // Connect delay mode toggle
  controlPanel.onDelayModeChanged = [this](bool isDelay) {
    processorRef.getEngine().setDelayMode(isDelay);
    updateEditingCurve(); // Update curve display for new mode
  };

  // Connect feedback invert toggle
  controlPanel.onFeedbackInvertChanged = [this](bool invert) {
    processorRef.getEngine().setFeedbackInvert(invert);
  };

  // Connect feedback multiplier slider
  controlPanel.onFeedbackMultiplierChanged = [this](float mult) {
    processorRef.getEngine().setFeedbackMultiplier(mult);
  };

  // Sync UI with current engine state (for restoring saved state)
  controlPanel.setDelayMode(processorRef.getEngine().isDelayMode());
  controlPanel.setFeedbackInvert(processorRef.getEngine().isFeedbackInvert());

  // Restore UI state from Processor (for DAW recall)
  controlPanel.selectParameter(processorRef.getUIParameterType());
  controlPanel.setDisplayMode(processorRef.getUIDisplayModeBar());
  controlPanel.setShowInputSpectrum(processorRef.getUIShowInputSpectrum());
  spectrumDisplay.setDisplayMode(processorRef.getUIDisplayModeBar()
                                     ? SpectrumDisplay::DisplayMode::Bar
                                     : SpectrumDisplay::DisplayMode::Line);
  showInputSpectrum = processorRef.getUIShowInputSpectrum();

  // Initial curve setup (uses the restored parameter type)
  updateEditingCurve();

  // Set plugin window size (wider for better spectrum display)
  // Enable resizing with constraints
  setResizable(true, true);
  setResizeLimits(700, 400, 1600, 1000);
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
    // Gain: 0-200%
    spectrumDisplay.setEditingCurve(&engine.getGainCurve(), 0.0f, 200.0f);
  } else if (paramType == 1) {
    // Feedback: 0-100%
    spectrumDisplay.setEditingCurve(&engine.getFeedbackCurve(), 0.0f, 100.0f);
  } else {
    // Delay: switch between Phase curve (0-360) and Delay curve (0-2000ms)
    if (controlPanel.isDelayMode()) {
      spectrumDisplay.setEditingCurve(&engine.getDelayCurve(), 0.0f, 2000.0f);
    } else {
      spectrumDisplay.setEditingCurve(&engine.getPhaseCurve(), 0.0f, 360.0f);
    }
  }

  // Update axis label to match current parameter
  updateAxisLabel();
}

void CopyCatSpectralDelayEditor::updateAxisLabel() {
  auto &parameters = processorRef.getParameters();
  int paramType = controlPanel.getCurrentParameterType();

  if (paramType == 0) {
    // Gain: base max 200%, scaled by gainMult (0-100%)
    auto *gainMultParam = parameters.getRawParameterValue("gainMult");
    float mult = gainMultParam ? (*gainMultParam / 100.0f) : 1.0f;
    float displayMax = 200.0f * mult;
    spectrumDisplay.setAxisLabel("%", displayMax);
  } else if (paramType == 1) {
    // Feedback: base max 100%, scaled by feedbackMult (0-100%)
    auto *feedbackMultParam = parameters.getRawParameterValue("feedbackMult");
    float mult = feedbackMultParam ? (*feedbackMultParam / 100.0f) : 1.0f;
    float displayMax = 100.0f * mult;
    spectrumDisplay.setAxisLabel("%", displayMax);
  } else {
    // Time parameter
    if (controlPanel.isDelayMode()) {
      // Delay mode: base max 2000ms, scaled by delayMult (0-100%)
      auto *delayMultParam = parameters.getRawParameterValue("delayMult");
      float mult = delayMultParam ? (*delayMultParam / 100.0f) : 1.0f;
      float displayMax = 2000.0f * mult;
      spectrumDisplay.setAxisLabel("ms", displayMax);
    } else {
      // Phase mode: fixed 0-360°, no multiplier
      spectrumDisplay.setAxisLabel(juce::CharPointer_UTF8("\xc2\xb0"), 360.0f);
    }
  }
}

void CopyCatSpectralDelayEditor::paint(juce::Graphics &g) {
  // Background gradient using centralized palette
  juce::ColourGradient gradient(CustomLookAndFeel::getBackgroundDark(), 0.0f,
                                0.0f, CustomLookAndFeel::getBackgroundMedium(),
                                0.0f, static_cast<float>(getHeight()), false);
  g.setGradientFill(gradient);
  g.fillAll();

  // Divider line between left and right panels
  auto bounds = getLocalBounds();
  int dividerX = static_cast<int>(bounds.getWidth() * 0.70f);
  g.setColour(CustomLookAndFeel::getTextPrimary().withAlpha(0.2f));
  g.drawVerticalLine(dividerX, 50.0f,
                     static_cast<float>(bounds.getHeight() - 10));
}

void CopyCatSpectralDelayEditor::resized() {
  auto bounds = getLocalBounds();

  // Title bar
  auto titleArea = bounds.removeFromTop(50);
  titleLabel.setBounds(titleArea.removeFromLeft(250).reduced(15, 0));
  versionLabel.setBounds(titleArea.removeFromRight(100).reduced(15, 0));

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

  // Sync displayMode from APVTS (in case changed by DAW automation)
  auto &parameters = processorRef.getParameters();
  auto *displayModeParam = parameters.getRawParameterValue("displayMode");
  if (displayModeParam) {
    bool isBar = static_cast<int>(*displayModeParam) == 1;
    controlPanel.setDisplayMode(isBar);
    spectrumDisplay.setDisplayMode(isBar ? SpectrumDisplay::DisplayMode::Bar
                                         : SpectrumDisplay::DisplayMode::Line);
  }

  // Sync spectrumSource from APVTS
  auto *spectrumSourceParam = parameters.getRawParameterValue("spectrumSource");
  if (spectrumSourceParam) {
    bool isInput = static_cast<int>(*spectrumSourceParam) == 1;
    controlPanel.setShowInputSpectrum(isInput);
    showInputSpectrum = isInput;
  }

  // Check bypass state and update spectrum display
  spectrumDisplay.setBypassed(processorRef.isSuspended());

  // Choose spectrum source based on toggle
  const float *spectrumData = showInputSpectrum
                                  ? engine.getInputMagnitudeSpectrum()
                                  : engine.getMagnitudeSpectrum();

  spectrumDisplay.setSpectralData(spectrumData, engine.getNumBins(),
                                  engine.getSampleRate());

  // Update axis label (in case multiply slider changed)
  updateAxisLabel();

  // Trigger repaint
  spectrumDisplay.repaint();
}
