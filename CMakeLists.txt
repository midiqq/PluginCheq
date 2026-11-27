cmake_minimum_required(VERSION 3.24)

project(PluginCheq VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_PLUGIN "Build VST3 and Standalone plugin targets using JUCE" ON)
option(BUILD_TESTS "Build Catch2 scientific validation tests" ON)

# ==============================================================================
# Pure C++20 DSP Core (No JUCE / GUI dependencies)
# ==============================================================================
add_library(dsp_core STATIC
    src/dsp/AnalysisTypes.h
    src/dsp/RingBuffer.h
    src/dsp/TripleBuffer.h
    src/dsp/WindowFunctions.h
    src/dsp/WindowFunctions.cpp
    src/dsp/FftEngine.h
    src/dsp/FftEngine.cpp
    src/dsp/DelayFinder.h
    src/dsp/DelayFinder.cpp
    src/dsp/SpectrumSmoothing.h
    src/dsp/SpectrumSmoothing.cpp
    src/dsp/DynamicsEngine.h
    src/dsp/HarmonicEngine.h
    src/dsp/TransferFunctionEngine.h
    src/dsp/TransferFunctionEngine.cpp
)

target_include_directories(dsp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(dsp_core PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(dsp_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

# ==============================================================================
# Catch2 Scientific Validation Tests
# ==============================================================================
if(BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.zip
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(dsp_tests
        tests/TestSystems.h
        tests/TransferFunctionTests.cpp
        tests/Phase2Tests.cpp
        tests/DynamicsHarmonicTests.cpp
        tests/RealTimeSafetyTests.cpp
    )

    target_link_libraries(dsp_tests PRIVATE
        dsp_core
        Catch2::Catch2WithMain
    )

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(dsp_tests)
endif()

# ==============================================================================
# JUCE 8 VST3 & Standalone Plugin
# ==============================================================================
if(BUILD_PLUGIN)
    FetchContent_Declare(
        JUCE
        URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.4.zip
    )
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(PluginCheq
        COMPANY_NAME "PluginCheq"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE Pchq
        PLUGIN_CODE Pchq
        FORMATS VST3
        PRODUCT_NAME "PluginCheq"
    )

    target_sources(PluginCheq PRIVATE
        src/plugin/Parameters.h
        src/plugin/AnalysisWorker.h
        src/plugin/AnalysisWorker.cpp
        src/plugin/PluginProcessor.h
        src/plugin/PluginProcessor.cpp
        src/ui/Theme.h
        src/ui/PluginCheqLookAndFeel.h
        src/ui/TransferPlotComponent.h
        src/ui/TransferPlotComponent.cpp
        src/ui/DynamicsPlotComponent.h
        src/ui/HarmonicPlotComponent.h
        src/ui/ControlBarComponent.h
        src/ui/ControlBarComponent.cpp
        src/ui/PluginEditor.h
        src/ui/PluginEditor.cpp
    )

    target_compile_definitions(PluginCheq PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
    )

    if(MSVC)
        target_compile_options(PluginCheq PRIVATE /utf-8)
    endif()

    target_link_libraries(PluginCheq
        PRIVATE
            dsp_core
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce_recommended_lto_flags
            juce_recommended_warning_flags
    )

    if(BUILD_TESTS)
        add_executable(plugin_tests
            tests/PluginLifecycleTests.cpp
        )
        target_include_directories(plugin_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
        target_link_libraries(plugin_tests PRIVATE
            PluginCheq
            dsp_core
            juce::juce_audio_utils
            juce::juce_dsp
            Catch2::Catch2WithMain
        )
        if(MSVC)
            target_compile_options(plugin_tests PRIVATE /utf-8)
        endif()
        catch_discover_tests(plugin_tests)
    endif()
endif()
