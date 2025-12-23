#pragma once

#include <complex>
#include <juce_core/juce_core.h>
#include <vector>

//==============================================================================
/**
 * Per-bin delay buffer for spectral delay processing.
 * Maintains a separate circular buffer for each FFT bin, allowing independent
 * delay times per frequency band.
 */
class PerBinDelayBuffer {
public:
  PerBinDelayBuffer();
  ~PerBinDelayBuffer();

  /**
   * Prepare the buffer for a given number of bins and maximum delay in frames.
   * @param numBins Number of FFT bins
   * @param maxDelayFrames Maximum delay in STFT frames
   */
  void prepare(int numBins, int maxDelayFrames);

  /**
   * Reset all buffers to zero.
   */
  void reset();

  /**
   * Set delay time for a specific bin.
   * @param binIndex Bin index (0 to numBins-1)
   * @param delayFrames Delay in STFT frames
   */
  void setDelayForBin(int binIndex, int delayFrames);

  /**
   * Set delay times for all bins at once.
   * @param delayFrames Array of delay values (length = numBins)
   */
  void setDelayForAllBins(const int *delayFrames);

  /**
   * Process a single frame: write input and read delayed output.
   * @param binIndex Bin index
   * @param input Current input value
   * @return Delayed output value
   */
  std::complex<float> processFrame(int binIndex, std::complex<float> input);

  /**
   * Process all bins in a frame.
   * @param data Complex spectrum data (will be modified in place)
   * @param numBins Number of bins
   */
  void processAllBins(std::complex<float> *data, int numBins);

  /**
   * Get the current delay time for a bin.
   */
  int getDelayForBin(int binIndex) const;

  /**
   * Get maximum supported delay in frames.
   */
  int getMaxDelayFrames() const { return maxDelayFrames; }

private:
  // Per-bin circular buffers storing complex values
  std::vector<std::vector<std::complex<float>>> binBuffers;

  // Write position for each bin (all bins share the same position)
  int writePosition = 0;

  // Delay time in frames for each bin
  std::vector<int> delayTimes;

  int currentNumBins = 0;
  int maxDelayFrames = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerBinDelayBuffer)
};
