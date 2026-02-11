cmake_minimum_required(VERSION 3.15)

project(StepSequencer VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)

# JUCE Setup - using symlink to shared JUCE
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../simple-delay-vst/JUCE/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../simple-delay-vst/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)
else()
    message(FATAL_ERROR "JUCE not found. Ensure simple-delay-vst/JUCE exists.")
endif()

# Plugin Configuration
juce_add_plugin(StepSequencer
    COMPANY_NAME "Null Invocation"
    BUNDLE_ID "com.nullinvocation.stepsequencer"
    PLUGIN_MANUFACTURER_CODE NulI
    PLUGIN_CODE SSeq
    FORMATS VST3 AU
    PRODUCT_NAME "Step Sequencer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument" "Fx"
)

# Source Files
target_sources(StepSequencer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

# Link JUCE modules
target_link_libraries(StepSequencer
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compiler definitions
target_compile_definitions(StepSequencer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)
