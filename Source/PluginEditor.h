#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ChazLookAndFeel.h"

class MacroStrip final : public juce::Component
{
public:
    explicit MacroStrip (juce::AudioProcessorValueTreeState& s);
    void resized() override;
private:
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> atts;
};

class ChazVelvetEditor final : public juce::AudioProcessorEditor,
                               public juce::Timer
{
public:
    explicit ChazVelvetEditor (ChazVelvetProcessor&);
    ~ChazVelvetEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void buildKnob (juce::Slider& s, juce::Label& l, const char* name, const char* paramId,
                    juce::Colour fill = chazui::col::accent);
    void refreshPresetBox();
    void syncLayerButtons();
    void bindLayerToggle (juce::ToggleButton& btn, const char* paramId);

    ChazVelvetProcessor& proc;
    chazui::ChazLookAndFeel lnf;
    MacroStrip macros;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::ToggleButton epBtn { "EP" }, padBtn { "PAD" }, sparkBtn { "SPARK" }, subBtn { "SUB" };
    juce::Label title, subtitle, status, sectionEp, sectionPad, sectionAmp;

    juce::Slider epTone, epDecay, epDrive, epTrem, epFm, epBell;
    juce::Label  epToneL, epDecayL, epDriveL, epTremL, epFmL, epBellL;

    juce::Slider padAtk, padCut, padDet, padShape;
    juce::Label  padAtkL, padCutL, padDetL, padShapeL;

    juce::Slider ampA, ampD, ampS, ampR, cutoff, reso, master;
    juce::Label  ampAL, ampDL, ampSL, ampRL, cutoffL, resoL, masterL;

    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtts;

    float meterSmooth = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChazVelvetEditor)
};
