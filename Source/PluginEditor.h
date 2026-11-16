#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Simple black-and-gold editor: a big Amount knob, Bypass, Output Gain.
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(FamousBeatsVocalCleanerProcessor&);
    ~PluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    FamousBeatsVocalCleanerProcessor& processor;

    juce::Slider amountKnob;
    juce::Slider gainKnob;
    juce::TextButton bypassButton;
    juce::Label titleLabel;
    juce::Label brandLabel;
    juce::Label amountLabel;
    juce::Label gainLabel;
    juce::Label hintLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
