#include "PluginEditor.h"

namespace
{
    const juce::Colour gold(0xffd4af37);
    const juce::Colour goldDark(0xff9a7b1e);
    const juce::Colour black(0xff0a0a0a);
    const juce::Colour panel(0xff141414);
    const juce::Colour cream(0xfff5e6c8);
}

PluginEditor::PluginEditor(FamousBeatsVocalCleanerProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(420, 320);

    titleLabel.setText("VOCAL CLEANER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, gold);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("FAMOUS BEATS STUDIO", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    brandLabel.setColour(juce::Label::textColourId, cream);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    amountKnob.setColour(juce::Slider::rotarySliderFillColourId, gold);
    amountKnob.setColour(juce::Slider::rotarySliderOutlineColourId, goldDark);
    amountKnob.setColour(juce::Slider::thumbColourId, gold);
    amountKnob.setColour(juce::Slider::textBoxTextColourId, cream);
    amountKnob.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    addAndMakeVisible(amountKnob);

    amountLabel.setText("AMOUNT", juce::dontSendNotification);
    amountLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    amountLabel.setColour(juce::Label::textColourId, cream);
    amountLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(amountLabel);

    gainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    gainKnob.setColour(juce::Slider::rotarySliderFillColourId, gold);
    gainKnob.setColour(juce::Slider::rotarySliderOutlineColourId, goldDark);
    gainKnob.setColour(juce::Slider::thumbColourId, gold);
    gainKnob.setColour(juce::Slider::textBoxTextColourId, cream);
    gainKnob.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    addAndMakeVisible(gainKnob);

    gainLabel.setText("OUTPUT", juce::dontSendNotification);
    gainLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    gainLabel.setColour(juce::Label::textColourId, cream);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, panel);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, goldDark);
    bypassButton.setColour(juce::TextButton::textColourOffId, cream);
    bypassButton.setColour(juce::TextButton::textColourOnId, black);
    addAndMakeVisible(bypassButton);

    hintLabel.setText("Put it first on a dry vocal track. 100% = full cleanup.",
                      juce::dontSendNotification);
    hintLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    hintLabel.setColour(juce::Label::textColourId, cream.withAlpha(0.7f));
    hintLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(hintLabel);

    auto& params = processor.getParametersState();
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, FamousBeatsVocalCleanerProcessor::kAmountParam, amountKnob);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, FamousBeatsVocalCleanerProcessor::kGainParam, gainKnob);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, FamousBeatsVocalCleanerProcessor::kBypassParam, bypassButton);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(black);
    g.setColour(goldDark);
    g.drawRect(getLocalBounds(), 2);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    titleLabel.setBounds(area.removeFromTop(40));
    brandLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(8);

    auto knobs = area.removeFromTop(160);
    auto amountArea = knobs.removeFromLeft(knobs.getWidth() / 2);
    amountKnob.setBounds(amountArea.removeFromTop(130));
    amountLabel.setBounds(amountArea.removeFromTop(20));
    gainKnob.setBounds(knobs.removeFromTop(130));
    gainLabel.setBounds(knobs.removeFromTop(20));

    area.removeFromTop(8);
    bypassButton.setBounds(area.removeFromTop(36).reduced(90, 0));
    area.removeFromTop(8);
    hintLabel.setBounds(area.removeFromTop(20));
}
