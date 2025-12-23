#include "PerBinDelayBuffer.h"

//==============================================================================
PerBinDelayBuffer::PerBinDelayBuffer() {}

PerBinDelayBuffer::~PerBinDelayBuffer() {}

//==============================================================================
void PerBinDelayBuffer::prepare(int numBins, int maxDelay) {
  currentNumBins = numBins;
  maxDelayFrames = maxDelay;

  // Resize buffers
  binBuffers.resize(numBins);
  delayTimes.resize(numBins, 0);

  for (int i = 0; i < numBins; ++i) {
    binBuffers[i].resize(maxDelayFrames + 1, std::complex<float>(0.0f, 0.0f));
  }

  reset();
}

void PerBinDelayBuffer::reset() {
  writePosition = 0;

  for (auto &buffer : binBuffers) {
    std::fill(buffer.begin(), buffer.end(), std::complex<float>(0.0f, 0.0f));
  }
}

//==============================================================================
void PerBinDelayBuffer::setDelayForBin(int binIndex, int delayFrames) {
  if (binIndex >= 0 && binIndex < currentNumBins) {
    delayTimes[binIndex] = juce::jlimit(0, maxDelayFrames, delayFrames);
  }
}

void PerBinDelayBuffer::setDelayForAllBins(const int *delayFramesArray) {
  for (int i = 0; i < currentNumBins; ++i) {
    delayTimes[i] = juce::jlimit(0, maxDelayFrames, delayFramesArray[i]);
  }
}

int PerBinDelayBuffer::getDelayForBin(int binIndex) const {
  if (binIndex >= 0 && binIndex < currentNumBins) {
    return delayTimes[binIndex];
  }
  return 0;
}

//==============================================================================
std::complex<float> PerBinDelayBuffer::processFrame(int binIndex,
                                                    std::complex<float> input) {
  if (binIndex < 0 || binIndex >= currentNumBins)
    return input;

  auto &buffer = binBuffers[binIndex];
  int bufferSize = static_cast<int>(buffer.size());

  // Calculate read position based on delay
  int delay = delayTimes[binIndex];
  int readPosition = (writePosition - delay + bufferSize) % bufferSize;

  // Read the delayed output FIRST
  std::complex<float> output = buffer[readPosition];

  // Then write input to current position
  buffer[writePosition] = input;

  return output;
}

void PerBinDelayBuffer::processAllBins(std::complex<float> *data, int numBins) {
  // Debug: Check if delays are being applied
  static int debugCount = 0;
  if (debugCount < 5) {
    DBG("processAllBins: numBins=" + juce::String(numBins) +
        ", delay[0]=" + juce::String(delayTimes.empty() ? -1 : delayTimes[0]) +
        ", writePos=" + juce::String(writePosition) +
        ", input[0]=" + juce::String(std::abs(data[0])));
    debugCount++;
  }

  // Process each bin
  for (int i = 0; i < numBins && i < currentNumBins; ++i) {
    data[i] = processFrame(i, data[i]);
  }

  // Advance write position (shared by all bins)
  int bufferSize = maxDelayFrames + 1;
  writePosition = (writePosition + 1) % bufferSize;
}
