cmake_minimum_required(VERSION 3.15)

project(CopyCatSpectralDelay VERSION 1.0.0)

# Set the path to JUCE - User has it installed in /Applications/JUCE
add_subdirectory("/Applications/JUCE" build/JUCE)

# Create the plugin target
juce_add_plugin(CopyCatSpectralDelay
    COMPANY_NAME "CopyCat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "CpCt"
    PLUGIN_CODE "CCSD"
    FORMATS VST3
    PRODUCT_NAME "CopyCat Spectral Delay")

# Add source files
target_sources(CopyCatSpectralDelay
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
        
        src/ui/SpectrumDisplay.cpp
        src/ui/SpectrumDisplay.h
        src/ui/CurveDisplay.cpp
        src/ui/CurveDisplay.h
        src/ui/ControlPanel.cpp
        src/ui/ControlPanel.h
        src/ui/CustomLookAndFeel.cpp
        src/ui/CustomLookAndFeel.h
        
        src/utils/ParameterCurve.cpp
        src/utils/ParameterCurve.h)

# Link against JUCE modules
target_link_libraries(CopyCatSpectralDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_include_directories(CopyCatSpectralDelay PRIVATE "/Applications/JUCE/modules")

# Compile definitions
target_compile_definitions(CopyCatSpectralDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
