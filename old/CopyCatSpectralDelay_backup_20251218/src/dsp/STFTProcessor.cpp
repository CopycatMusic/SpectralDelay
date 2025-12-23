#include "STFTProcessor.h"
#include <cmath>
#include <cstring>

STFTProcessor::STFTProcessor() {
  fftOrder = defaultFftOrder;
  fftSize = 1 << fftOrder;
  numBins = fftSize / 2 + 1;
  hopSize = fftSize / overlapFactor;

  fft = std::make_unique<juce::dsp::FFT>(fftOrder);

  // Note: window size is fftSize + 1 because JUCE's windows are symmetrical,
  // which is wrong for overlap-add processing. To make the window periodic,
  // set size to fftSize + 1 but only use the first fftSize samples.
  window = std::make_unique<juce::dsp::WindowingFunction<float>>(
      fftSize + 1, juce::dsp::WindowingFunction<float>::hann, false);

  initBuffers();
  prepared = true;
}

void STFTProcessor::prepare(double sr, int /*maxBlockSize*/) {
  sampleRate = sr;
  // Buffers are already initialized in constructor
  prepared = true;
}

void STFTProcessor::reset() {
  count = 0;
  pos = 0;

  // Zero out the circular buffers
  std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
  std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);
  std::fill(fftData.begin(), fftData.end(), 0.0f);
  std::fill(magnitudeSpectrum.begin(), magnitudeSpectrum.end(), 0.0f);
}

void STFTProcessor::setFFTSize(int size) {
  jassert(size >= 256 && size <= maxFftSize);
  jassert((size & (size - 1)) == 0); // Must be power of 2

  if (size != fftSize) {
    fftSize = size;
    fftOrder = static_cast<int>(std::log2(fftSize));
    numBins = fftSize / 2 + 1;
    hopSize = fftSize / overlapFactor;

    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(
        fftSize + 1, juce::dsp::WindowingFunction<float>::hann, false);

    initBuffers();
  }
}

void STFTProcessor::setOverlapFactor(int factor) {
  jassert(factor == 2 || factor == 4 || factor == 8);

  if (factor != overlapFactor) {
    overlapFactor = factor;
    hopSize = fftSize / overlapFactor;

    // Update window correction based on overlap
    // For Hann window: 2x overlap -> 1.0, 4x overlap -> 2/3, 8x overlap -> 1/3
    if (overlapFactor == 2) {
      windowCorrection = 1.0f;
    } else if (overlapFactor == 4) {
      windowCorrection = 2.0f / 3.0f;
    } else {
      windowCorrection = 1.0f / 3.0f;
    }

    initBuffers();
  }
}

void STFTProcessor::initBuffers() {
  inputFifo.assign(fftSize, 0.0f);
  outputFifo.assign(fftSize, 0.0f);
  fftData.assign(fftSize * 2, 0.0f);
  magnitudeSpectrum.assign(numBins, 0.0f);

  count = 0;
  pos = 0;
  prepared = true;
}

void STFTProcessor::process(
    const float *input, float *output, int numSamples,
    std::function<void(std::complex<float> *, int numBins)> spectralProcess) {

  if (!prepared || !fft) {
    std::memcpy(output, input, numSamples * sizeof(float));
    return;
  }

  for (int i = 0; i < numSamples; ++i) {
    // Push the new sample value into the input FIFO
    inputFifo[pos] = input[i];

    // Read the output value from the output FIFO. Since it takes fftSize
    // timesteps before actual samples are read from this FIFO instead of
    // the initial zeros, the sound output is delayed by fftSize samples.
    float outputSample = outputFifo[pos];

    // Once we've read the sample, set this position in the FIFO back to
    // zero so we can add the IFFT results to it later.
    outputFifo[pos] = 0.0f;

    // Advance the FIFO index and wrap around if necessary
    pos = (pos + 1) % fftSize;

    // Process the FFT frame once we've collected hopSize samples
    count++;
    if (count == hopSize) {
      count = 0;
      processFrame(spectralProcess);
    }

    output[i] = outputSample;
  }
}

void STFTProcessor::processFrame(
    std::function<void(std::complex<float> *, int)> &callback) {

  float *inputPtr = inputFifo.data();
  float *fftPtr = fftData.data();

  // Copy the input FIFO into the FFT working space in two parts.
  // This handles the circular buffer wrap-around.
  std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
  if (pos > 0) {
    std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));
  }

  // Apply the analysis window to avoid spectral leakage
  window->multiplyWithWindowingTable(fftPtr, fftSize);

  // Perform the forward FFT
  fft->performRealOnlyForwardTransform(fftPtr, true);

  // Get complex spectrum pointer
  auto *spectrum = reinterpret_cast<std::complex<float> *>(fftPtr);

  // Update magnitude spectrum for visualization
  // Normalize by fftSize to get proper dB values relative to input signal
  float normFactor = 1.0f / static_cast<float>(fftSize);
  for (int i = 0; i < numBins; ++i) {
    magnitudeSpectrum[i] = std::abs(spectrum[i]) * normFactor;
  }

  // Apply spectral processing callback
  if (callback) {
    callback(spectrum, numBins);
  }

  // Perform the inverse FFT
  fft->performRealOnlyInverseTransform(fftPtr);

  // Apply the synthesis window for resynthesis
  window->multiplyWithWindowingTable(fftPtr, fftSize);

  // Scale down the output samples because of the overlapping windows
  for (int i = 0; i < fftSize; ++i) {
    fftPtr[i] *= windowCorrection;
  }

  // Add the IFFT results to the output FIFO in two parts.
  // This handles the circular buffer wrap-around correctly.
  for (int i = 0; i < pos; ++i) {
    outputFifo[i] += fftData[i + fftSize - pos];
  }
  for (int i = 0; i < fftSize - pos; ++i) {
    outputFifo[i + pos] += fftData[i];
  }
}
