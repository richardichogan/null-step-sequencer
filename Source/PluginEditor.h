/*
  ==============================================================================
    PluginEditor.h
    Step Sequencer UI
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
//==============================================================================
class StepSequencerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    StepSequencerAudioProcessorEditor (StepSequencerAudioProcessor&);
    ~StepSequencerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    
    // Interactions
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    
    // Helper (public so processor can call on state restore)
    void updateInspector();
    void rebuildTrackControls();
    void updateTracksLabel();
    
    // Custom LookAndFeel
    class DarkLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkLookAndFeel();
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider& slider) override;
        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float minSliderPos, float maxSliderPos,
                            juce::Slider::SliderStyle style, juce::Slider& slider) override;
    };

private:
    StepSequencerAudioProcessor& audioProcessor;      // Declare ref first
    std::vector<int> selectedSteps; // Multi-selection support

    // Per-step controls (32 steps max = 2 rows of 16)
    static const int MAX_STEP_KNOBS = 32;
    juce::Slider stepVelocityKnobs[MAX_STEP_KNOBS];
    juce::Slider stepGateKnobs[MAX_STEP_KNOBS];
    juce::Slider stepProbabilityKnobs[MAX_STEP_KNOBS];
    
    // Global control knobs
    juce::Slider numStepsKnob;      // Rotary with preset positions
    juce::Slider rateKnob;          // Rotary with preset positions
    juce::Slider swingKnob;         // Rotary 0-100
    juce::Slider randomizeKnob;     // Rotary 0-100
    juce::Slider mutateKnob;        // Rotary 0-100
    
    juce::Label numStepsLabel;
    juce::Label rateLabel;
    juce::Label swingLabel;
    juce::Label randomizeLabel;
    juce::Label mutateLabel;
    juce::Label keyLabel;
    juce::Label scaleLabel;
    juce::Label octaveLabel;
    
    juce::ComboBox keyCombo;
    juce::ComboBox scaleCombo;
    
    // Octave Control (Buttons)
    juce::TextButton octavePlusButton;
    juce::TextButton octaveMinusButton;
    juce::Label octaveValueLabel;
    
    // Pattern Transform Buttons
    juce::TextButton clearButton;
    juce::TextButton invertButton;
    juce::TextButton reverseButton;
    juce::ComboBox euclideanCombo;
    juce::Label euclideanLabel;

    // Track System (left sidebar) - dynamic
    juce::Label tracksLabel;        // Shows "Tracks: 2" or similar
    std::vector<std::unique_ptr<juce::TextButton>> trackButtons; // Radio buttons to switch view
    std::vector<std::unique_ptr<juce::TextButton>> trackEnableButtons; // Toggle track on/off
    std::vector<std::unique_ptr<juce::Slider>> trackRepeatSliders; // Repeat count for each track (1-16)
    juce::TextButton addTrackButton; // "+" button to add tracks
    juce::TextButton duplicateTrackButton; // "Dup" button to duplicate current track
    juce::TextButton removeTrackButton; // "-" button to remove tracks
    
    juce::Rectangle<int> stepGridArea;
    juce::Rectangle<int> pianoArea;

    // Helper function to check if note is in scale
    bool isNoteInScale(int midiNote, int rootNote, int scaleType);
    
    // Custom styling
    DarkLookAndFeel darkLookAndFeel;
    
    // UI Attachments (Last for safety)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> numStepsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> swingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencerAudioProcessorEditor)
};
