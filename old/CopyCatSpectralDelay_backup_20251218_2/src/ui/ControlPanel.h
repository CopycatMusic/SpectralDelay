#pragma once

#include "../utils/ParameterCurve.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
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

  // Callback when display mode changes
  std::function<void(bool)> onDisplayModeChanged;

  // Callback when spectrum source changes (false = Output, true = Input)
  std::function<void(bool)> onSpectrumSourceChanged;

private:
  void comboBoxChanged(juce::ComboBox *comboBox) override;
  void sliderValueChanged(juce::Slider *slider) override;

  // Reference to parameter tree
  juce::AudioProcessorValueTreeState &parameters;

  // Parameter selector
  juce::Label parameterLabel;
  juce::ComboBox parameterSelector;

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

  // Display mode toggle
  juce::Label displayModeLabel;
  juce::TextButton displayModeButton;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      displayModeAttachment;

  // Spectrum source toggle
  juce::Label spectrumSourceLabel;
  juce::TextButton spectrumSourceButton;

  // Current parameter type (0 = Phase, 1 = Gain)
  int currentParameterType = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};
