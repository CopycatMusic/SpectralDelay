cmake_minimum_required(VERSION 3.22)

# Preserve architecture overrides; default to a universal macOS build.
if(CMAKE_HOST_APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()
project(CopyCatSpectralDelay VERSION 1.2.0 LANGUAGES C CXX)

option(COPYCAT_COPY_PLUGIN_AFTER_BUILD "Install plugins into local plugin folders after building" OFF)

set(COPYCAT_PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND COPYCAT_PLUGIN_FORMATS AU)
endif()

# ============================================================================
# JUCE Path Configuration (supports local dev and CI builds)
# ============================================================================
# Priority: 1. Command line -DJUCE_DIR=...  2. Environment variable  3. Platform default

if(NOT DEFINED JUCE_DIR)
    if(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR "$ENV{JUCE_DIR}")
    elseif(WIN32)
        set(JUCE_DIR "C:/JUCE")
    else()
        set(JUCE_DIR "/Applications/JUCE")
    endif()
endif()

message(STATUS "Using JUCE from: ${JUCE_DIR}")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE source tree not found at '${JUCE_DIR}'. Set -DJUCE_DIR=/path/to/JUCE (tested with 8.0.12).")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")

# ============================================================================
# Binary Resources (Fonts)
# ============================================================================
juce_add_binary_data(FontResources
    SOURCES
        resources/fonts/Montserrat-Bold.ttf
        resources/fonts/Montserrat-Medium.ttf
        resources/fonts/Montserrat-Light.ttf)

# Plugin target: all functionality is enabled.
juce_add_plugin(CopyCatSpectralDelay
    COMPANY_NAME "CopyCat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPYCAT_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE "CpCt"
    PLUGIN_CODE "CCSD"
    FORMATS ${COPYCAT_PLUGIN_FORMATS}
    BUNDLE_ID "com.copycat.spectraldelay"
    PRODUCT_NAME "CopyCat Spectral Delay")

# ============================================================================
# Source Files
# ============================================================================
target_sources(CopyCatSpectralDelay
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginProcessor.h
        src/PluginEditor.cpp
        src/PluginEditor.h

        src/dsp/STFTProcessor.cpp
        src/dsp/STFTProcessor.h
        src/dsp/SpectralDelayEngine.cpp
        src/dsp/SpectralDelayEngine.h
        src/dsp/LFOEngine.cpp
        src/dsp/LFOEngine.h

        src/ui/SpectrumDisplay.cpp
        src/ui/SpectrumDisplay.h
        src/ui/ControlPanel.cpp
        src/ui/ControlPanel.h
        src/ui/CustomLookAndFeel.cpp
        src/ui/CustomLookAndFeel.h

        src/utils/ParameterCurve.cpp
        src/utils/ParameterCurve.h)

# ============================================================================
# Dependencies
# ============================================================================
target_link_libraries(CopyCatSpectralDelay
    PRIVATE
        FontResources
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_features(CopyCatSpectralDelay PRIVATE cxx_std_17)

# ============================================================================
# Compile Definitions
# ============================================================================
target_compile_definitions(CopyCatSpectralDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
