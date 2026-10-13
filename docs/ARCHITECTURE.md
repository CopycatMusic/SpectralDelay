# Architecture

## Signal path

Stereo input → STFT per channel → gate → compression → optional pre-gain →
feedback mix → delay or phase processing → feedback storage → optional post-gain
→ inverse FFT / overlap-add → latency-compensated dry/wet mix.

The diffuse control adds bin-dependent stereo delay variation in the delay path.
The processor uses 4× overlap and reports the current FFT size as STFT latency.

## Responsibilities

- `PluginProcessor`: JUCE host integration, APVTS parameters, host tempo/transport,
  Full-version LFO modulation, smoothing, latency reporting and state serialization.
- `STFTProcessor`: periodic Hann analysis/synthesis windows, FFT processing,
  circular input/output FIFOs and overlap-add.
- `SpectralDelayEngine`: per-bin parameter caches, complex delay/feedback buffers,
  spectral dynamics, stereo processing and dry compensation.
- `LFOEngine`: curve-based modulation, host synchronization and smoothing.
- `ParameterCurve`: control points, logarithmic frequency interpolation and state data.
- `PluginEditor` / `SpectrumDisplay` / `ControlPanel`: editing and display.

JUCE supplies the host wrappers, FFT API, GUI, APVTS and supporting DSP utilities.
STFT code credits hollance/fft-juce. The curves, processing stages, modulation and
UI integrate into this application; this document does not certify sole authorship
of every line or replace a detailed third-party provenance review.

## Engineering tradeoffs

- Larger FFTs offer more frequency resolution at greater latency; at 48 kHz,
  the reported 2048-sample latency is about 42.7 ms.
- Per-bin complex delay memory gives independent frequency-band timing. Buffer
  sizing depends on both bin count and hop duration, rather than FFT size alone.
- Full and Lite share source with a compile-time LFO feature switch and separate
  host identifiers to avoid colliding installations.
- Audio and UI currently share curve and spectrum data. Some locking reduces
  conflicts, but the handoff is not yet safe enough for a hard real-time claim.

See [ENGINEERING_REVIEW.md](ENGINEERING_REVIEW.md) for the next implementation work.
