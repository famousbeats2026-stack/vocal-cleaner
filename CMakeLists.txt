cmake_minimum_required(VERSION 3.22)

# Famous Beats Vocal Cleaner — VST3 / AU plugin for macOS
# Removes background noise from vocals with the RNNoise neural denoiser.
# Build on a Mac (see README.md) or let GitHub Actions build it for you.

project(FamousBeatsVocalCleaner VERSION 1.0.0)

# The plugin runs on macOS 10.15 (Catalina) and newer, Intel and Apple Silicon.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")

# ---- JUCE (downloaded automatically at configure time, not bundled) ----
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)
FetchContent_MakeAvailable(JUCE)

# ---- RNNoise (C library, sources included in this project) ----
# Classic RNNoise 0.1 (xiph/rnnoise, master branch) plus the "sh" model
# (GregorR/rnnoise-models, somnolent-hogwash-2018-09-01) embedded below.
add_library(rnnoise STATIC
    Source/RNNoise/denoise.c
    Source/RNNoise/rnn.c
    Source/RNNoise/rnn_data.c
    Source/RNNoise/rnn_reader.c
    Source/RNNoise/pitch.c
    Source/RNNoise/celt_lpc.c
    Source/RNNoise/kiss_fft.c
)
target_include_directories(rnnoise PUBLIC Source/RNNoise)
# The plugin is a shared library (.vst3/.component), so this static
# library must be position-independent code (required on Linux; macOS
# builds PIC by default, but this keeps every platform the same).
set_property(TARGET rnnoise PROPERTY POSITION_INDEPENDENT_CODE ON)
# gnu99 (C extensions on) so M_PI is available in math.h on every platform.
set_property(TARGET rnnoise PROPERTY C_STANDARD 99)
set_property(TARGET rnnoise PROPERTY C_EXTENSIONS ON)
if(APPLE)
    # Silence the "shorten-64-to-32" style warnings from the old C code.
    target_compile_options(rnnoise PRIVATE -Wno-shorten-64-to-32)
endif()

# ---- The plugin ----
juce_add_plugin(FamousBeatsVocalCleaner
    COMPANY_NAME "Famous Beats Studio"
    PRODUCT_NAME "Famous Beats Vocal Cleaner"
    BUNDLE_ID com.famousbeatsstudio.vocalcleaner
    PLUGIN_MANUFACTURER_CODE FbSt
    PLUGIN_CODE Vcln
    FORMATS VST3 AU
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
)

# Generate JuceHeader.h (needed because the sources #include <JuceHeader.h>).
juce_generate_juce_header(FamousBeatsVocalCleaner)

# Embed the trained RNNoise model file into the plugin binary.
juce_add_binary_data(FamousBeatsVocalCleanerData
    SOURCES Source/RNNoise/sh.rnnn
)

target_sources(FamousBeatsVocalCleaner PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(FamousBeatsVocalCleaner
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(FamousBeatsVocalCleaner
    PRIVATE
    FamousBeatsVocalCleanerData
    rnnoise
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
