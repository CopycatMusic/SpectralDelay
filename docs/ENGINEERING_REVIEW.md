# Engineering review — 2026-10-13

This pass cleans repository contents, build configuration and documentation.
It does not change the DSP algorithm, parameter IDs or plugin identifiers.
The items below are code findings, not measured performance results.

## Highest priority

1. **FFT changes on the audio thread.** `PluginProcessor::processBlock` calls
   `SpectralDelayEngine::setFFTSize`. A changed size recreates FFT/window objects,
   assigns delay/envelope buffers and takes `spectrumMutex`. Avoid allocations and
   blocking there: prepare supported configurations outside the callback, design
   a bounded handoff and test switching under automation. Account for memory cost
   before preallocating every possible delay-buffer configuration.
2. **Spectrum buffer lifetime and synchronization.** `getMagnitudeSpectrum` locks
   the mutex but returns a raw pointer after releasing it. The audio callback can
   update/reallocate the vector while the editor copies it. Return a coherent
   snapshot or publish a preallocated buffer with explicit ownership; test GUI
   refresh while switching FFT sizes.
3. **Curve update contention and notifications.** The audio process calls curve
   cache updates that take a SpinLock and a recursive mutex. A `true` dirty flag
   followed by a later `false` assignment can also lose an intervening edit.
   Publish a versioned, immutable curve/bin snapshot from the UI and let the audio
   thread read the last complete snapshot without waiting.

## Follow-up validation and performance

- Add neutral STFT reconstruction tests (impulse, sine, noise), checking reported
  latency and output after startup/crop compensation across supported FFT sizes.
- Test DC and Nyquist using JUCE's actual real-FFT layout. Current comments and
  display code treat `spectrum[0].imag()` as Nyquist; verify this before changing
  processing or presenting spectrum values as correct.
- Check reset behavior: engine `reset()` clears STFT and dry delay lines but not
  all spectral feedback/delay state. Define stop/restart tail semantics and test
  stale-state behavior before modifying it.
- Reserve dry and visualization buffers in preparation; ensure block-size changes
  do not allocate during steady processing. Profile before replacing data layouts.
- Benchmark callback duration (including high percentiles), CPU, memory, sample
  rates, block sizes and active controls; do not claim a percentage speedup without
  matched baseline measurements.
- Verify bypass/dry alignment, host session recall, feedback stability, FFT-size
  automation and Full/Lite coexistence in a real DAW.

## Validation

See [VALIDATION.md](VALIDATION.md) for checks and their limits. Windows CI uses
`windows-2022` to match its Visual Studio 2022 generator; see the
[official runner inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md).
