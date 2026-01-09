#pragma once

#include "dsp/LFOEngine.h"
#include "dsp/SpectralDelayEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
class CopyCatSpectralDelayProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  CopyCatSpectralDelayProcessor();
  ~CopyCatSpectralDelayProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  using AudioProcessor::processBlock;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  // Access to engine for UI
  SpectralDelayEngine &getEngine() { return engine; }
  const SpectralDelayEngine &getEngine() const { return engine; }

  // Parameters
  juce::AudioProcessorValueTreeState &getParameters() { return parameters; }

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  //==============================================================================
  juce::AudioProcessorValueTreeState parameters;
  SpectralDelayEngine engine;
  LFOEngine lfoEngine;

  // Parameter pointers for quick access
  std::atomic<float> *fftSizeParam = nullptr;
  std::atomic<float> *mixParam = nullptr;

  // UI state storage (for DAW recall)
  int uiParameterType = 2;       // 0=Gain, 1=Feedback, 2=Time
  bool uiDisplayModeBar = false; // false=Line, true=Bar
  bool uiShowInputSpectrum = false;
  bool uiLfoMode = false; // false=Normal, true=LFO editing mode

public:
  // UI state accessors
  int getUIParameterType() const { return uiParameterType; }
  void setUIParameterType(int type) { uiParameterType = type; }
  bool getUIDisplayModeBar() const { return uiDisplayModeBar; }
  void setUIDisplayModeBar(bool bar) { uiDisplayModeBar = bar; }
  bool getUIShowInputSpectrum() const { return uiShowInputSpectrum; }
  void setUIShowInputSpectrum(bool input) { uiShowInputSpectrum = input; }
  bool getUILfoMode() const { return uiLfoMode; }
  void setUILfoMode(bool lfoMode) { uiLfoMode = lfoMode; }

  // LFO Engine access for UI
  LFOEngine &getLfoEngine() { return lfoEngine; }
  const LFOEngine &getLfoEngine() const { return lfoEngine; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CopyCatSpectralDelayProcessor)
};
