cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
project(CopyCatSpectralDelay VERSION 1.1.0)

# ============================================================================
# JUCE Path Configuration (supports local dev and CI builds)
# ============================================================================
# Priority: 1. Command line -DJUCE_DIR=...  2. Environment variable  3. Platform default

if(NOT DEFINED JUCE_DIR)
    if(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR $ENV{JUCE_DIR})
    elseif(WIN32)
        set(JUCE_DIR "C:/JUCE")
    else()
        set(JUCE_DIR "/Applications/JUCE")
    endif()
endif()

message(STATUS "Using JUCE from: ${JUCE_DIR}")
add_subdirectory(${JUCE_DIR} build/JUCE)

# ============================================================================
# Binary Resources (Fonts)
# ============================================================================
juce_add_binary_data(FontResources
    SOURCES
        resources/fonts/Montserrat-Bold.ttf
        resources/fonts/Montserrat-Light.ttf)

# ============================================================================
# Function to create plugin targets (Full and Lite versions)
# ============================================================================
function(create_spectral_delay_plugin TARGET_NAME IS_LITE PLUGIN_CODE BUNDLE_SUFFIX PRODUCT_SUFFIX)
    juce_add_plugin(${TARGET_NAME}
        COMPANY_NAME "CopyCat"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE 
        PLUGIN_MANUFACTURER_CODE "CpCt"
        PLUGIN_CODE "${PLUGIN_CODE}"
        FORMATS AU VST3
        BUNDLE_ID "com.copycat.spectraldelay${BUNDLE_SUFFIX}"
        PRODUCT_NAME "CopyCat Spectral Delay${PRODUCT_SUFFIX}")

    # ============================================================================
    # Source Files
    # ============================================================================
    target_sources(${TARGET_NAME}
        PRIVATE
            src/PluginProcessor.cpp
            src/PluginProcessor.h
            src/PluginEditor.cpp
            src/PluginEditor.h
            
            src/dsp/STFTProcessor.cpp
            src/dsp/STFTProcessor.h
            src/dsp/PerBinDelayBuffer.cpp
            src/dsp/PerBinDelayBuffer.h
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
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            FontResources
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_include_directories(${TARGET_NAME} PRIVATE "${JUCE_DIR}/modules")

    # ============================================================================
    # Compile Definitions
    # ============================================================================
    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    # ============================================================================
    # Lite Version Macro
    # ============================================================================
    if(IS_LITE)
        target_compile_definitions(${TARGET_NAME} PRIVATE LITE_VERSION=1)
    else()
        target_compile_definitions(${TARGET_NAME} PRIVATE LITE_VERSION=0)
    endif()
endfunction()

# ============================================================================
# Create Full Version
# ============================================================================
create_spectral_delay_plugin(CopyCatSpectralDelay OFF "CCSD" "" "")

# ============================================================================
# Create Lite Version
# ============================================================================
create_spectral_delay_plugin(CopyCatSpectralDelayLite ON "CCSL" ".lite" " Lite")
