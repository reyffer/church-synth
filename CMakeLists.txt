cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(ChurchSynth VERSION 0.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(CS_BUILD_PLUGIN "Build the AU/VST3/Standalone plugin (needs JUCE)" ON)
option(CS_BUILD_TESTS  "Build DSP tests and the CPU benchmark" ON)

# ---------------------------------------------------------------------------
# DSP core: plain C++, no JUCE, so it can be tested and benchmarked anywhere.
add_library(cs_dsp STATIC
    source/dsp/SynthCore.cpp
    source/dsp/Content.cpp
    source/dsp/ContentFactory.cpp
    source/dsp/ModMatrix.cpp
    source/dsp/ModSources.cpp
    source/dsp/SeqArp.cpp
    source/dsp/fx/Effects.cpp
    source/dsp/fx/FxRack.cpp)
target_include_directories(cs_dsp PUBLIC source)
set_target_properties(cs_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(cs_dsp PRIVATE /O2 /W4 /fp:fast)
else()
    target_compile_options(cs_dsp PRIVATE -O3 -Wall -Wextra -ffp-contract=fast)
endif()

# ---------------------------------------------------------------------------
if(CS_BUILD_TESTS)
    enable_testing()
    add_executable(cs_tests tests/tests.cpp)
    target_link_libraries(cs_tests PRIVATE cs_dsp)
    target_include_directories(cs_tests PRIVATE tests)
    add_test(NAME dsp_tests COMMAND cs_tests)

    add_executable(cs_bench tests/bench.cpp)
    target_link_libraries(cs_bench PRIVATE cs_dsp)
    target_include_directories(cs_bench PRIVATE tests)
endif()

# ---------------------------------------------------------------------------
if(CS_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    if(APPLE)
        set(CS_FORMATS AU VST3 Standalone)
    else()
        set(CS_FORMATS VST3 Standalone)
    endif()

    juce_add_plugin(ChurchSynth
        PRODUCT_NAME "Church Synth"
        COMPANY_NAME "Reyffer"
        BUNDLE_ID com.reyffer.churchsynth
        PLUGIN_MANUFACTURER_CODE Ryfr
        PLUGIN_CODE Chsy
        FORMATS ${CS_FORMATS}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        VST3_CATEGORIES Instrument Synth
        DESCRIPTION "Lean hybrid synth for live worship")

    set(CS_PLUGIN_SOURCES
        source/plugin/Parameters.cpp
        source/plugin/PluginProcessor.cpp
        source/plugin/PresetManager.cpp
        source/plugin/FactoryPresets.cpp
        source/plugin/FactoryNotes.cpp
        source/plugin/PluginEditor.cpp)
    file(GLOB CS_UI_SOURCES CONFIGURE_DEPENDS source/plugin/ui/*.cpp)
    list(APPEND CS_PLUGIN_SOURCES ${CS_UI_SOURCES})

    # Embedded font (Inter, SIL Open Font License: resources/fonts/OFL.txt).
    juce_add_binary_data(cs_binary_data SOURCES
        resources/fonts/Inter-400.ttf
        resources/fonts/Inter-600.ttf
        resources/images/lotus.png)
    set_target_properties(cs_binary_data PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_sources(ChurchSynth PRIVATE ${CS_PLUGIN_SOURCES})

    target_compile_definitions(ChurchSynth PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    # Our own standalone app: identical to JUCE's, but opens all MIDI inputs automatically.
    target_sources(ChurchSynth_Standalone PRIVATE source/plugin/StandaloneApp.cpp)
    target_compile_definitions(ChurchSynth_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

    target_link_libraries(ChurchSynth
        PRIVATE
            cs_dsp
            cs_binary_data
            juce::juce_audio_utils
            juce::juce_audio_formats
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    if(CS_BUILD_TESTS)
        # Plugin-level state test: a console app compiling the processor sources directly.
        juce_add_console_app(cs_state_test PRODUCT_NAME "cs_state_test")
        target_sources(cs_state_test PRIVATE tests/state_test.cpp packs/Pack1.cpp packs/Pack1Notes.cpp packs/Pack2Worship.cpp packs/Pack2Notes.cpp packs/Pack3EPianos.cpp packs/Pack3Notes.cpp packs/Pack4Orchestral.cpp packs/Pack4Notes.cpp packs/Pack5Brass.cpp packs/Pack5Notes.cpp packs/Pack6ChorusPoly.cpp packs/Pack6Notes.cpp packs/Pack7GrandPoly.cpp packs/Pack7Notes.cpp packs/Pack8Ladder.cpp packs/Pack8Notes.cpp packs/Pack9VintagePoly.cpp packs/Pack9Notes.cpp packs/Pack10TwinLayer.cpp packs/Pack10Notes.cpp packs/Pack11Synthwave.cpp packs/Pack11Notes.cpp packs/Pack12EDM.cpp packs/Pack12Notes.cpp packs/Pack13Famous.cpp packs/Pack13Notes.cpp packs/Pack14SpecialFX.cpp packs/Pack14Notes.cpp ${CS_PLUGIN_SOURCES})
        target_include_directories(cs_state_test PRIVATE source .)
        target_compile_definitions(cs_state_test PRIVATE
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
            "JucePlugin_Name=\"Church Synth\""
            "JucePlugin_VersionString=\"${PROJECT_VERSION}\"")
        target_link_libraries(cs_state_test PRIVATE
            cs_dsp
            cs_binary_data
            juce::juce_audio_utils
            juce::juce_audio_formats
            juce::juce_recommended_config_flags)
        add_test(NAME plugin_state_test COMMAND cs_state_test)
        add_test(NAME plugin_thread_stress COMMAND cs_state_test --stress-threads 8)
    endif()
endif()
