# Corresponding source for the 1.2.0 plugin bundles

The release includes `CopyCat_Spectral_Delay_Source.zip` containing this project's
source, CMake build files, resources, licence notices and build instructions.
The same source is available at:
https://github.com/CopycatMusic/SpectralDelay/tree/v1.2.0-release

The external JUCE dependency is the unmodified **8.0.12** source tree. Download
its source (including its bundled dependency notices) here:
https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.12.tar.gz

Place the JUCE source directory next to this project's CMakeLists.txt as `JUCE/`,
or pass its path using `-DJUCE_DIR=...`. See README.md for macOS and Windows build
commands. Release builds use CMake, C++17, JUCE 8.0.12, macOS arm64+x86_64 or Windows
x64, and `-DCOPYCAT_COPY_PLUGIN_AFTER_BUILD=OFF`.

Project source and binaries use AGPLv3; retained third-party notices describe the
original licences of bundled fonts, STFT reference code and JUCE dependencies.
