# Validation status

Source version: **1.2.0**. Local checks performed on **2026-10-13**.

## Confirmed locally

- Fresh Release build with JUCE 8.0.12, CMake, Ninja and AppleClang on Apple Silicon.
- Full and Lite AU/VST3: all four bundles built, with version 1.2.0 and arm64 binaries.
- macOS universal and Windows Release builds both passed in
  [GitHub Actions run 38382530567](https://github.com/CopycatMusic/SpectralDelay/actions/runs/38382530567)
  for source commit `950c6f9`. The subsequent README update only adds the Gumroad link.
- Current editor instantiated and rendered using the piano demo as real DSP input.
  The README main image is a native JUCE component render, not a DAW-host screenshot.
- All CMake source/resource inputs, Markdown media references and image files exist.
- Workflow YAML parses, both Git diff whitespace checks pass, and local signing tools
  and archives are excluded from the published source tree.
- Third-party font and STFT reference licence notices are included.

## Pending and known limits

- macOS universal and Windows CI compile checks passed. Real-host compatibility,
  runtime audio correctness and installer verification still need separate testing.
- No automated DSP reconstruction/regression suite or published CPU benchmark yet.
- DAW listening, automation, session recall, Intel compatibility and signed installer
  distribution were not validated in this cleanup pass.
- Current source still emits compiler warnings. See [ENGINEERING_REVIEW.md](ENGINEERING_REVIEW.md)
  for audio-thread allocation, curve locking, display synchronization and reset work.

Compilation and interface rendering do not establish audio correctness or
compatibility with every host. CI artifacts are unsigned and not notarized.
