# Validation status

Source version: **1.2.0**. Local checks performed on **2026-10-13–14**.

## Confirmed locally

- Fresh Release build with JUCE 8.0.12, CMake, Ninja and AppleClang on Apple Silicon.
- AU/VST3 version 1.2.0 was built on Apple Silicon. The plugin now has one target
  with all functionality enabled; consolidation checks are recorded below.
- The consolidated macOS universal and Windows x64 Release builds passed in
  [GitHub Actions run 38489077946](https://github.com/CopycatMusic/SpectralDelay/actions/runs/38489077946)
  for build commit `d9dad27`. The release tag additionally contains licence,
  screenshot and packaging-documentation updates; plugin source, CMake and fonts
  are identical to that successful build.
- Current editor instantiated and rendered using the piano demo as real DSP input.
  The README main image is a native JUCE component render. The four feature images
  are complete-window screenshots of the installed 1.2.0 AU in a temporary native
  host, with the same piano demo processed by the actual AU binary.
- All CMake source/resource inputs, Markdown media references and image files exist.
- Workflow YAML parses, both Git diff whitespace checks pass, and local signing tools
  and archives are excluded from the published source tree.
- Third-party font and STFT reference licence notices are included.

## Pending and known limits

- Additional host/platform compatibility, a DSP regression suite and installer
  verification still need separate testing.
- No automated DSP reconstruction/regression suite or published CPU benchmark yet.
- The author completed local macOS DAW testing and reported no issues on
  2026-10-14. The host name and individual test coverage were not recorded. Intel
  and Windows runtime compatibility and signed installers remain unverified.
- Current source still emits compiler warnings. See [ENGINEERING_REVIEW.md](ENGINEERING_REVIEW.md)
  for audio-thread allocation, curve locking, display synchronization and reset work.

Compilation and interface rendering do not establish audio correctness or
compatibility with every host. macOS release bundles are ad-hoc signed and not
notarized; Windows bundles are not code-signed.

## Single-plugin consolidation

The alternate build target and feature-switch branches were removed on 2026-10-13.
The original plugin name, bundle identifier, AU/VST3 codes and parameter IDs are
retained. The single target builds successfully as Release AU and VST3 on arm64.
Both bundles report version 1.2.0, and their AU component identifiers and VST3 class
IDs match the original plugin.

A state fixture exported by the previous complete-feature build was imported into
the consolidated processor. The check passed for parameter values, Delay and LFO
curve points, LFO sync/smoothing, all 12 parameters, state save/reload and editor
instantiation. This is a local compatibility check, not a DAW session-recall test.

The author tested the installed arm64 candidate in a local DAW and approved
publication on 2026-10-14. Universal macOS and Windows x64 builds of the same
consolidated source subsequently passed the release CI linked above.

## Installed AU validation

On 2026-10-13, the local arm64 1.2.0 AU/VST3 candidates were installed in the
user plugin folders after backing up the previous user/system copies. Installed
binaries match the reviewed candidates and pass signature verification.

`auval -v aufx CCSD CpCt` loaded version 1.2.0 and reported
`AU VALIDATION SUCCEEDED`, including format, render and parameter-scheduling checks.
The author subsequently reported successful local DAW testing on 2026-10-14.
No automated VST3 host-validation run was performed.

## Published archive verification

The 1.2.0 draft release ZIPs were downloaded and checked before publication on
2026-10-14. The binary and project-source archives passed integrity checks and matched the GitHub asset
digests. The macOS AU/VST3 bundles report 1.2.0, contain arm64 and x86_64 slices,
retain executable permissions after extraction and pass ad-hoc signature checks.
The Windows VST3 reports 1.2.0 and contains an x64 PE binary. Neither archive
contains an alternate limited-feature plugin. SHA-256 checksums accompany the
[public Release](https://github.com/CopycatMusic/SpectralDelay/releases/tag/v1.2.0-release).
