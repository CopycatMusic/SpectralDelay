#pragma once

#include <array>
#include <complex>
#include <functional>
#include <juce_dsp/juce_dsp.h>

/**
 * STFT processor with overlap-add implementation.
 * Based on hollance/fft-juce reference implementation.
 */
class STFTProcessor {
public:
  STFTProcessor();
  ~STFTProcessor() = default;

  void prepare(double sampleRate, int maxBlockSize);
  void reset();

  void setFFTSize(int size);
  void setOverlapFactor(int factor);

  int getFFTSize() const { return fftSize; }
  int getNumBins() const { return fftSize / 2 + 1; }
  int getHopSize() const { return hopSize; }
  int getLatencyInSamples() const { return fftSize; }

  /**
   * Process audio: input -> STFT -> spectral callback -> ISTFT -> output
   */
  void process(
      const float *input, float *output, int numSamples,
      std::function<void(std::complex<float> *, int numBins)> spectralProcess);

  const float *getMagnitudeSpectrum() const { return magnitudeSpectrum.data(); }

private:
  void initBuffers();
  void processFrame(std::function<void(std::complex<float> *, int)> &callback);

  // FFT configuration
  static constexpr int defaultFftOrder = 11; // 2048 samples
  static constexpr int maxFftSize = 8192;

  int fftSize = 1 << defaultFftOrder; // 2048
  int fftOrder = defaultFftOrder;
  int numBins = fftSize / 2 + 1;
  int overlapFactor = 4; // 75% overlap
  int hopSize = fftSize / overlapFactor;

  // Gain correction for using Hann window with 75% overlap
  float windowCorrection = 2.0f / 3.0f;

  std::unique_ptr<juce::dsp::FFT> fft;
  std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

  // Counts up until the next hop
  int count = 0;

  // Read/write position in the circular buffers
  int pos = 0;

  // Circular buffers for incoming and outgoing audio data
  std::vector<float> inputFifo;
  std::vector<float> outputFifo;

  // The FFT working space. Contains interleaved complex numbers.
  std::vector<float> fftData;

  // Magnitude spectrum for visualization
  std::vector<float> magnitudeSpectrum;

  double sampleRate = 44100.0;
  bool prepared = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(STFTProcessor)
};
