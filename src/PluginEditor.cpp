#include "PluginEditor.h"
#include "BinaryData.h"
#include "PluginProcessor.h"

//==============================================================================
CopyCatSpectralDelayEditor::CopyCatSpectralDelayEditor(
    CopyCatSpectralDelayProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      controlPanel(p.getParameters()) {
  setLookAndFeel(&customLookAndFeel);

  // Load embedded Montserrat font
  auto montserratTypeface = juce::Typeface::createSystemTypefaceFor(
      BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);

  // Title labels
  brandLabel.setText("COPYCAT", juce::dontSendNotification);
  brandLabel.setFont(juce::Font(montserratTypeface).withHeight(32.0f));
  brandLabel.setColour(juce::Label::textColourId,
                       CustomLookAndFeel::getTextPrimary());
  brandLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(brandLabel);

  titleLabel.setText("SPECTRAL DELAY", juce::dontSendNotification);
  titleLabel.setFont(juce::Font(montserratTypeface).withHeight(32.0f));
  titleLabel.setColour(juce::Label::textColourId,
                       CustomLookAndFeel::getTextPrimary().withAlpha(0.6f));
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(titleLabel);

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

  // Connect LFO mode toggle
  controlPanel.onLfoModeChanged = [this](bool isLfo) {
    processorRef.setUILfoMode(isLfo);
    updateEditingCurve();
  };

  // Connect LFO amount change
  controlPanel.onLfoAmountChanged = [this](int paramType, float amount) {
    auto &lfoEngine = processorRef.getLfoEngine();
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);
    lfoEngine.setAmount(param, amount);
  };

  // Connect LFO speed change
  controlPanel.onLfoSpeedChanged = [this](int paramType, float speedMs) {
    auto &lfoEngine = processorRef.getLfoEngine();
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);
    lfoEngine.setSpeed(param, speedMs);
  };

  // Connect LFO smooth change
  controlPanel.onLfoSmoothChanged = [this](int paramType, float smoothMs) {
    auto &lfoEngine = processorRef.getLfoEngine();
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);
    lfoEngine.setSmooth(param, smoothMs);
  };

  // Connect LFO beat mode change
  controlPanel.onLfoBeatModeChanged = [this](int paramType, bool isBeat) {
    auto &lfoEngine = processorRef.getLfoEngine();
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);
    lfoEngine.setBeatMode(param, isBeat);
    // Mode change might reset speed to a default index/ms, so refresh UI
    updateEditingCurve();
  };

  // Connect LFO sync change
  controlPanel.onLfoSyncChanged = [this](int paramType, bool isSynced) {
    auto &lfoEngine = processorRef.getLfoEngine();
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);
    lfoEngine.setSynced(param, isSynced);
  };

  // Connect LFO grid controls
  controlPanel.onLfoHGridChanged = [this](int divisions) {
    spectrumDisplay.setLfoHGrid(divisions);
  };
  controlPanel.onLfoVGridChanged = [this](int divisions) {
    spectrumDisplay.setLfoVGrid(divisions);
  };

  // Connect stereo width controls
  controlPanel.onStereoChanged = [this](float amount) {
    processorRef.getEngine().setStereoAmount(amount);
  };
  controlPanel.onStereoLowCutChanged = [this](float freq) {
    processorRef.getEngine().setStereoLowCut(freq);
  };

  // Connect high precision delay mode
  controlPanel.onHighPrecisionDelayChanged = [this](bool enabled) {
    processorRef.getEngine().setHighPrecisionDelay(enabled);
  };

  // Sync UI with current engine state (for restoring saved state)
  controlPanel.setDelayMode(processorRef.getEngine().isDelayMode());
  controlPanel.setFeedbackInvert(processorRef.getEngine().isFeedbackInvert());
  controlPanel.setHighPrecisionDelay(
      processorRef.getEngine().isHighPrecisionDelay());

  // Restore UI state from Processor (for DAW recall)
  controlPanel.selectParameter(processorRef.getUIParameterType());
  controlPanel.setLfoMode(processorRef.getUILfoMode());
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
  setSize(950, 450);

  // Start timer for spectrum update (30 Hz)
  startTimerHz(30);
}

CopyCatSpectralDelayEditor::~CopyCatSpectralDelayEditor() {
  setLookAndFeel(nullptr);
}

//==============================================================================
void CopyCatSpectralDelayEditor::updateEditingCurve() {
  auto &engine = processorRef.getEngine();
  auto &lfoEngine = processorRef.getLfoEngine();
  int paramType = controlPanel.getCurrentParameterType();
  bool isLfoMode = controlPanel.isLfoMode();

  // Always update the current parameter type for gray curve display
  spectrumDisplay.setCurrentParameterType(paramType);

  if (isLfoMode) {
    // LFO mode: edit LFO curves (not SpectralDelayEngine curves)
    // LFO curves use X axis as 0-1 normalized position
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);
    spectrumDisplay.setBackgroundCurves(
        &engine.getGainCurve(), &engine.getFeedbackCurve(),
        &engine.getDelayCurve(), &engine.getPhaseCurve());
    spectrumDisplay.setEditingCurve(&lfoEngine.getCurve(param), 0.0f, 100.0f);
    spectrumDisplay.setLfoMode(true);
    spectrumDisplay.setTimeMode(controlPanel.isDelayMode());

    // Update LFO controls with current parameter's values
    controlPanel.updateLfoControls(
        lfoEngine.getAmount(param), lfoEngine.getSpeed(param),
        lfoEngine.getSmooth(param), lfoEngine.getBeatMode(param),
        lfoEngine.getSynced(param));

  } else {
    // Normal mode: edit SpectralDelayEngine curves
    spectrumDisplay.setLfoMode(false);
    spectrumDisplay.setBackgroundCurves(
        &engine.getGainCurve(), &engine.getFeedbackCurve(),
        &engine.getDelayCurve(), &engine.getPhaseCurve());
    spectrumDisplay.setTimeMode(controlPanel.isDelayMode());

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

  // No divider line - panels manage their own borders
}

void CopyCatSpectralDelayEditor::resized() {
  auto bounds = getLocalBounds();

  // Title bar
  auto titleArea = bounds.removeFromTop(40);
  auto titleLabelArea =
      titleArea.removeFromLeft(450).withTrimmedLeft(15).withTrimmedTop(5);

  // Position brand and title
  int brandWidth = brandLabel.getFont().getStringWidth(brandLabel.getText());
  brandLabel.setBounds(
      titleLabelArea.removeFromLeft(brandWidth + 8)); // 8px space after COPYCAT
  titleLabel.setBounds(titleLabelArea);

  // Split remaining area: right side is fixed 290px
  int spectrumWidth = bounds.getWidth() - 290;

  // Left side: Spectrum display with curve editing
  auto leftArea = bounds.removeFromLeft(spectrumWidth);
  spectrumDisplay.setBounds(leftArea);

  // Right side: Control panel (takes the remaining 290px)
  auto rightArea = bounds;
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

  // Update LFO state in UI (phase and modulation values for animation)
  auto &lfoEngine = processorRef.getLfoEngine();

  // Set modulation values for all parameters to enable animation of gray lines
  // Use smoothed values so the animation matches the actual audio processing
  spectrumDisplay.setLfoModulation(
      0, lfoEngine.getSmoothedModulationValue(LFOEngine::Parameter::Gain));
  spectrumDisplay.setLfoModulation(
      1, lfoEngine.getSmoothedModulationValue(LFOEngine::Parameter::Feedback));
  spectrumDisplay.setLfoModulation(
      2, lfoEngine.getSmoothedModulationValue(LFOEngine::Parameter::Time));

  // Update axis label (in case multiply slider changed)
  updateAxisLabel();

  // Update LFO phase display when in LFO mode
  if (controlPanel.isLfoMode()) {
    int paramType = controlPanel.getCurrentParameterType();
    LFOEngine::Parameter param = static_cast<LFOEngine::Parameter>(paramType);

    // Read the current phase from the engine (advanced by processBlock)
    spectrumDisplay.setLfoPhase(lfoEngine.getPhase(param));
  }

  // Trigger repaint
  spectrumDisplay.repaint();
}
