#pragma once

#include "../dsp/LFOEngine.h"
#include "../utils/ParameterCurve.h"
#include "UIConstants.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * Control panel component for the right side of the plugin UI.
 * Contains parameter selection and FFT/mix settings.
 * Curve editing is now done directly on the SpectrumDisplay.
 */
class ControlPanel : public juce::Component,
                     private juce::ComboBox::Listener,
                     private juce::Slider::Listener {
public:
  explicit ControlPanel(juce::AudioProcessorValueTreeState &apvts);
  ~ControlPanel() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Get currently selected parameter type (0 = Phase, 1 = Gain)
  int getCurrentParameterType() const { return currentParameterType; }

  // Callback when parameter type selection changes
  std::function<void(int)> onParameterTypeChanged;

  // Callback when delay mode changes (false = Phase, true = Delay)
  std::function<void(bool)> onDelayModeChanged;

  // Callback when display mode changes
  std::function<void(bool)> onDisplayModeChanged;

  // Callback when spectrum source changes (false = Output, true = Input)
  std::function<void(bool)> onSpectrumSourceChanged;

  // Callback when feedback invert changes
  std::function<void(bool)> onFeedbackInvertChanged;

  // Callback when LFO mode changes
  std::function<void(bool)> onLfoModeChanged;

  // Callback when LFO amount changes (for current parameter)
  std::function<void(int, float)> onLfoAmountChanged; // (paramType, amount)

  // Callback when LFO speed changes (for current parameter)
  std::function<void(int, float)> onLfoSpeedChanged; // (paramType, speedMs)

  // Callback when LFO smooth changes (for current parameter)
  std::function<void(int, float)> onLfoSmoothChanged; // (paramType, smoothMs)

  // Callback when LFO sync mode changes
  using SyncMode = LFOEngine::SyncMode;
  std::function<void(int, SyncMode)> onLfoSyncModeChanged; // (paramType, mode)

  // Callback to get current LFO sync mode
  std::function<SyncMode(int)> onGetLfoSyncMode; // (paramType)

  // Callback when LFO sync changes
  std::function<void(int, bool)> onLfoSyncChanged; // (paramType, isSynced)

  // Callback when LFO grid changes
  std::function<void(int)> onLfoHGridChanged; // H Grid divisions
  std::function<void(int)> onLfoVGridChanged; // V Grid divisions

  // Callback when feedback multiplier changes
  std::function<void(float)> onFeedbackMultiplierChanged;

  // Callback when stereo width changes
  std::function<void(float)> onStereoChanged;       // 0-100%
  std::function<void(float)> onStereoLowCutChanged; // Hz

  // Callback when high precision delay mode changes
  std::function<void(bool)> onHighPrecisionDelayChanged;

  // Callback when gain post mode changes
  std::function<void(bool)> onGainPostModeChanged;

  // Get currently active LFO sync mode
  SyncMode getLfoSyncMode() const {
    if (onGetLfoSyncMode)
      return onGetLfoSyncMode(currentParameterType);
    return SyncMode::Time;
  }

  // Get current delay mode (always Delay mode, Phase mode removed)
  bool isDelayMode() const { return true; }

  // Set delay mode UI state (no-op, Phase mode removed)
  void setDelayMode(bool /*isDelay*/) {
    // Phase mode has been removed, always use Delay mode
  }

  // Set high precision delay mode UI state (for restoring from saved state)
  void setHighPrecisionDelay(bool enabled) {
    hiResButton.setToggleState(enabled, juce::dontSendNotification);
  }
  bool isHighPrecisionDelay() const { return hiResButton.getToggleState(); }

  // Set display mode UI state (for restoring from saved state)
  void setDisplayMode(bool isBar) {
    displayModeButton.setToggleState(isBar, juce::dontSendNotification);
    displayModeButton.setButtonText(isBar ? "Bar" : "Line");
  }

  // Set spectrum source UI state (for restoring from saved state)
  void setShowInputSpectrum(bool isInput) {
    spectrumSourceButton.setToggleState(isInput, juce::dontSendNotification);
    spectrumSourceButton.setButtonText(isInput ? "Input" : "Output");
  }

  // Set feedback invert UI state (for restoring from saved state)
  void setFeedbackInvert(bool invert) {
    feedbackInvertButton.setToggleState(invert, juce::dontSendNotification);
  }

  void setGainPostMode(bool post);

  // Select parameter type (for restoring from saved state)
  void selectParameter(int paramType);

  // Set LFO mode UI state
  void setLfoMode(bool isLfoMode);
  bool isLfoMode() const { return currentLfoMode; }

  /**
   * Update LFO controls with current values from engine.
   * Includes mode-specific updates for the speed slider.
   */
  void updateLfoControls(float amount, float speedOrIndex, float smoothMs,
                         SyncMode mode, bool isSynced);

private:
  // Layout constants are now in UIConstants.h
  // Use UIConstants::kMargin, UIConstants::kSpacing, etc.

  void comboBoxChanged(juce::ComboBox *comboBox) override;
  void sliderValueChanged(juce::Slider *slider) override;
  void updateMaxLabels(); // Update MAX value labels based on multiply sliders

  // Helper to update LFO speed slider suffix and range
  void updateLfoSpeedSliderSettings(SyncMode mode);

  juce::String getSyncModeString(SyncMode mode);

  // Reference to parameter tree
  juce::AudioProcessorValueTreeState &parameters;

  // Parameter selector buttons
  juce::Label parameterLabel;
  juce::TextButton gainButton{"Gain"};
  juce::TextButton delayButton{"Delay"};
  juce::TextButton feedbackButton{"Feedback"};

  // FFT settings
  juce::Label fftSizeLabel;
  juce::Slider fftSizeSelector;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      fftSizeAttachment;

  // Mix control
  juce::Label mixLabel;
  juce::Slider mixSlider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mixAttachment;

  // High precision delay mode button
  juce::TextButton hiResButton{"HiRes"};

  // Display mode toggle
  juce::Label displayModeLabel;
  juce::TextButton displayModeButton;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      displayModeAttachment;

  // Spectrum source toggle
  juce::Label spectrumSourceLabel;
  juce::TextButton spectrumSourceButton;

  // Delay mode toggle (Phase vs Delay)
  juce::Label delayModeLabel;
  juce::TextButton delayModeButton;

  // Feedback controls (Invert and Multiply)
  juce::Label feedbackMultLabel;
  juce::Slider feedbackMultSlider;
  juce::TextButton feedbackInvertButton{"Phase Invert"};
  juce::Label feedbackMaxLabel; // Shows "MAX: xxx%"
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      feedbackMultAttachment;

  // Gain multiply control
  juce::Label gainMultLabel;
  juce::Slider gainMultSlider;
  juce::Label gainMaxLabel;               // Shows "MAX: xxx%"
  juce::TextButton gainPostButton{"Pre"}; // Pre/Post gain mode
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      gainMultAttachment;

  // Delay multiply control
  juce::Label delayMultLabel;
  juce::Slider delayMultSlider;
  juce::Label delayMaxLabel; // Shows "MAX: xxxms"
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delayMultAttachment;

  // Stereo width controls (for Time/Delay parameter)
  juce::Label stereoLabel;
  juce::Slider stereoSlider;
  juce::Label stereoLowCutLabel;
  juce::Slider stereoLowCutSlider;

  // Current parameter type (0 = Gain, 1 = Feedback, 2 = Delay)
  int currentParameterType = 2;

  // Current delay mode (false = Phase, true = Delay)
  bool currentDelayMode = true;

  // LFO mode state
  bool currentLfoMode = false;

  // LFO button
  juce::TextButton lfoButton{"LFO"};

  // LFO controls (shown when LFO mode is active)
  juce::Label lfoAmountLabel;
  juce::Slider lfoAmountSlider;
  juce::Label lfoSpeedLabel;
  juce::Slider lfoSpeedSlider;
  juce::Label lfoSmoothLabel;
  juce::Slider lfoSmoothSlider;

  // New LFO mode controls
  juce::TextButton lfoBeatModeButton{
      "Time"}; // Cycles through Time/Note/Triplet/Dotted
  juce::TextButton lfoSyncButton{"Sync Off"};

  // LFO grid controls
  juce::Label lfoHGridLabel;
  juce::Slider lfoHGridSlider;
  juce::Label lfoVGridLabel;
  juce::Slider lfoVGridSlider;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};
