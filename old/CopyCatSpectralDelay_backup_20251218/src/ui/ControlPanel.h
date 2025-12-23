#pragma once

#include "../utils/ParameterCurve.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * Control panel component for the right side of the plugin UI.
 * Contains parameter selection, mode switching, and parameter controls.
 */
class ControlPanel : public juce::Component,
                     private juce::ComboBox::Listener,
                     private juce::Button::Listener,
                     private juce::Slider::Listener {
public:
  explicit ControlPanel(juce::AudioProcessorValueTreeState &apvts);
  ~ControlPanel() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Callback for curve updates
  std::function<void(const ParameterCurve &, float, float)> onCurveChanged;

private:
  void comboBoxChanged(juce::ComboBox *comboBox) override;
  void buttonClicked(juce::Button *button) override;
  void sliderValueChanged(juce::Slider *slider) override;

  void updateCurveCallback();
  void updateControlVisibility();

  // Reference to parameter tree
  juce::AudioProcessorValueTreeState &parameters;

  // Parameter selector
  juce::Label parameterLabel;
  juce::ComboBox parameterSelector;

  // Mode selection
  juce::Label modeLabel;
  juce::TextButton constantModeButton;
  juce::TextButton curveModeButton;

  // Constant mode controls
  juce::Label constantValueLabel;
  juce::Slider constantValueSlider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      constantDelayAttachment;

  // Curve mode controls
  juce::Label slopeLabel;
  juce::Slider slopeSlider;
  juce::Label offsetLabel;
  juce::Slider offsetSlider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      slopeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      offsetAttachment;

  // FFT settings
  juce::Label fftSizeLabel;
  juce::ComboBox fftSizeSelector;
  juce::Label overlapLabel;
  juce::ComboBox overlapSelector;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      fftSizeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      overlapAttachment;

  // Mix control
  juce::Label mixLabel;
  juce::Slider mixSlider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mixAttachment;

  // Current mode
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      curveModeAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      gainCurveModeAttachment;
  juce::ComboBox curveModeHidden;     // Hidden combo box for phase curve mode
  juce::ComboBox gainCurveModeHidden; // Hidden combo box for gain curve mode
  bool isConstantMode = true;

  // Current parameter type (0 = Phase, 1 = Gain)
  int currentParameterType = 0;

  // Current curve for display
  ParameterCurve currentCurve;

  // Method to switch parameter attachments
  void updateAttachments();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};
