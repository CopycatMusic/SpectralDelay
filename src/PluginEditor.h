#pragma once

#include "PluginProcessor.h"
#include "ui/ControlPanel.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/SpectrumDisplay.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
class CopyCatSpectralDelayEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer {
public:
  explicit CopyCatSpectralDelayEditor(CopyCatSpectralDelayProcessor &);
  ~CopyCatSpectralDelayEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void paintOverChildren(juce::Graphics &) override;
  void resized() override;

private:
  void timerCallback() override;
  void updateEditingCurve();
  void updateAxisLabel();

  //==============================================================================
  CopyCatSpectralDelayProcessor &processorRef;

  CustomLookAndFeel customLookAndFeel;

  // UI Components
  SpectrumDisplay spectrumDisplay;
  ControlPanel controlPanel;

  // Title labels
  juce::Label brandLabelCopy;
  juce::Label brandLabelCat;
  juce::Label titleLabel;
  juce::Label authorLabel;

  juce::Font titleFont;
  juce::Font defaultFont;

  // Spectrum source selection (false = Output, true = Input)
  bool showInputSpectrum = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CopyCatSpectralDelayEditor)
};
