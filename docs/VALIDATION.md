# Validation status

Source version: **1.2.0**. Local checks performed on **2026-10-13**.

## Confirmed locally

- Fresh Release build with JUCE 8.0.12, CMake, Ninja and AppleClang on Apple Silicon.
- Full and Lite AU/VST3: all four bundles built, with version 1.2.0 and arm64 binaries.
- Current editor instantiated and rendered using the piano demo as real DSP input.
  The README main image is a native JUCE component render, not a DAW-host screenshot.
- All CMake source/resource inputs, Markdown media references and image files exist.
- Workflow YAML parses, both Git diff whitespace checks pass, and local signing tools
  and archives are excluded from the published source tree.
- Third-party font and STFT reference licence notices are included.

## Pending and known limits

- Updated macOS universal and Windows CI must be verified after upload. Existing
  earlier macOS CI passed; earlier Windows CI failed at toolchain configuration.
- No automated DSP reconstruction/regression suite or published CPU benchmark yet.
- DAW listening, automation, session recall, Intel compatibility and signed installer
  distribution were not validated in this cleanup pass.
- Current source still emits compiler warnings. See [ENGINEERING_REVIEW.md](ENGINEERING_REVIEW.md)
  for audio-thread allocation, curve locking, display synchronization and reset work.

Compilation and interface rendering do not establish audio correctness or
compatibility with every host. CI artifacts are unsigned and not notarized.
