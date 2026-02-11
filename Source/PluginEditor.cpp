/*
  ==============================================================================
    PluginEditor.cpp
    Step Sequencer UI - Grid-Based Design
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Macro for accessing steps (handles pointer)
#define STEPS (*audioProcessor.steps)

//==============================================================================
// Minimal "Null" Look - Clean Vector Knobs
//==============================================================================
StepSequencerAudioProcessorEditor::DarkLookAndFeel::DarkLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff121212));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffcccccc));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000)); // Transparent text box
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222222));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xff999999));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xffff9900));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222222));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0x00000000));
}

void StepSequencerAudioProcessorEditor::DarkLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = (float)juce::jmin(width, height) * 0.45f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    
    // Background arc
    auto angleRange = rotaryEndAngle - rotaryStartAngle;
    
    // Draw modern arc style
    g.setColour(juce::Colour(0xff2a2a2a));
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Active arc
    juce::Colour activeColor = juce::Colours::white;
    if (slider.getName().contains("Random")) activeColor = juce::Colour(0xffff6666);
    else if (slider.getName().contains("Mutate")) activeColor = juce::Colour(0xff6666ff);
    
    g.setColour(activeColor);
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryStartAngle + sliderPos * angleRange, true);
    g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Small pointer circle
    /*
    auto pointerAngle = rotaryStartAngle + sliderPos * angleRange;
    auto px = centreX + std::sin(pointerAngle) * radius;
    auto py = centreY - std::cos(pointerAngle) * radius;
    g.fillEllipse(px - 3, py - 3, 6, 6);
    */
    
    // Or just a minimal dot in center
    // g.setColour(juce::Colours::darkgrey);
    // g.fillEllipse(centreX - 2, centreY - 2, 4, 4);
}

void StepSequencerAudioProcessorEditor::DarkLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos);
    if (style == juce::Slider::LinearVertical)
    {
        // Modern wide bar design (User requested wider sliders)
        float trackWidth = (float)width * 0.9f; 
        auto trackX = x + width * 0.5f;
        
        // Track background
        g.setColour(juce::Colour(0xff181818));
        g.fillRect((float)x, (float)y, (float)width, (float)height); // darken background slightly
        
        g.setColour(juce::Colour(0xff222222));
        g.fillRoundedRectangle(trackX - trackWidth*0.5f, y + 2, trackWidth, height - 4, 3.0f);
        
        float fillHeight = height - 4 - (sliderPos - y - 2);
        if (fillHeight < 0) fillHeight = 0;
        
        auto name = slider.getName();
        juce::Colour fillColor = juce::Colour(0xff00ccff); // Cyan
        if (name.contains("Prob")) fillColor = juce::Colour(0xff00ff88); // Green
        
        // Use darker colors for unselected/inactive? 
        // Logic handled by opacity in component usually, but here fixed colors.
        
        // Filled bar
        g.setColour(fillColor.withAlpha(0.7f));
        g.fillRoundedRectangle(trackX - trackWidth*0.5f, sliderPos, trackWidth, fillHeight, 3.0f);
        
        // Handle "cap" active glow
        g.setColour(fillColor);
        g.fillRoundedRectangle(trackX - trackWidth*0.5f, sliderPos, trackWidth, 4.0f, 2.0f);
    }
    else
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}

//==============================================================================
StepSequencerAudioProcessorEditor::StepSequencerAudioProcessorEditor (StepSequencerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&darkLookAndFeel);
    
    // === CONTROL KNOBS (Top Row) ===
    
    // Steps Knob (preset positions: 4, 8, 12, 16, 20, 24, 28, 32)
    addAndMakeVisible(numStepsLabel);
    numStepsLabel.setText("Steps", juce::dontSendNotification);
    numStepsLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(numStepsKnob);
    numStepsKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    numStepsKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    numStepsKnob.setRange(4, 32, 4);
    numStepsKnob.setMouseDragSensitivity(100);
    numStepsAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(p.apvts, "numSteps", numStepsKnob));
    numStepsKnob.onValueChange = [this] {
        updateInspector(); // Update slider visibility when step count changes
    };

    // Rate Knob (preset positions mapped to combo)
    addAndMakeVisible(rateLabel);
    rateLabel.setText("Rate", juce::dontSendNotification);
    rateLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(rateKnob);
    rateKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    rateKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    rateKnob.setRange(0, 3, 1); // 0=1/4, 1=1/8, 2=1/16, 3=1/32
    rateKnob.setMouseDragSensitivity(100);
    rateKnob.textFromValueFunction = [](double value) {
        int v = (int)value;
        if (v == 0) return juce::String("1/4");
        if (v == 1) return juce::String("1/8");
        if (v == 2) return juce::String("1/16");
        if (v == 3) return juce::String("1/32");
        return juce::String("1/16");
    };
    rateAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(p.apvts, "rate", rateKnob));

    // Swing Knob (0-100%)
    addAndMakeVisible(swingLabel);
    swingLabel.setText("Swing", juce::dontSendNotification);
    swingLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(swingKnob);
    swingKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    swingKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    swingKnob.setTextValueSuffix("%");
    swingKnob.setMouseDragSensitivity(100);
    swingAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(p.apvts, "swing", swingKnob));

    // Key Combo
    addAndMakeVisible(keyLabel);
    keyLabel.setText("Key", juce::dontSendNotification);
    keyLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(keyCombo);
    keyCombo.addItemList(juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    keyAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(p.apvts, "key", keyCombo));
    keyCombo.onChange = [this] { repaint(); };

    // Scale Combo
    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale", juce::dontSendNotification);
    scaleLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(scaleCombo);
    scaleCombo.addItemList(juce::StringArray { "Chromatic", "Major", "Minor", "Dorian", "Phrygian", "Mixolydian", "Pentatonic" }, 1);
    scaleAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(p.apvts, "scale", scaleCombo));
    scaleCombo.onChange = [this] { repaint(); };

    // Randomize Knob
    addAndMakeVisible(randomizeLabel);
    randomizeLabel.setText("Randomize", juce::dontSendNotification);
    randomizeLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(randomizeKnob);
    randomizeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomizeKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    randomizeKnob.setRange(0.0, 100.0, 1.0);
    randomizeKnob.setValue(0.0);
    randomizeKnob.setTextValueSuffix("%");
    randomizeKnob.setMouseDragSensitivity(100);
    randomizeKnob.onValueChange = [this] { 
        float amount = (float)randomizeKnob.getValue() / 100.0f;
        audioProcessor.randomizePattern(amount); 
        updateInspector();
        repaint();
    };

    // Mutate Knob
    addAndMakeVisible(mutateLabel);
    mutateLabel.setText("Mutate", juce::dontSendNotification);
    mutateLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(mutateKnob);
    mutateKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mutateKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    mutateKnob.setRange(0.0, 100.0, 1.0);
    mutateKnob.setValue(0.0);
    mutateKnob.setTextValueSuffix("%");
    mutateKnob.setMouseDragSensitivity(100);
    mutateKnob.onValueChange = [this] { 
        float amount = (float)mutateKnob.getValue() / 100.0f;
        audioProcessor.mutatePattern(amount); 
        updateInspector();
        repaint();
    };
    
    // Octave Shift (Buttons)
    addAndMakeVisible(octaveLabel);
    octaveLabel.setText("Octave", juce::dontSendNotification);
    octaveLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(octaveMinusButton);
    octaveMinusButton.setButtonText("-");
    octaveMinusButton.onClick = [this] {
        if (auto* paramPtr = audioProcessor.apvts.getParameter("octave")) {
             // Best to use getRawParameterValue for logic.
             int currentVal = (int)*audioProcessor.apvts.getRawParameterValue("octave");
             paramPtr->setValueNotifyingHost(audioProcessor.apvts.getParameter("octave")->convertTo0to1(currentVal - 1));
             octaveValueLabel.setText(juce::String(currentVal - 1), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(octavePlusButton);
    octavePlusButton.setButtonText("+");
    octavePlusButton.onClick = [this] {
        if (auto* paramPtr = audioProcessor.apvts.getParameter("octave")) {
             int currentVal = (int)*audioProcessor.apvts.getRawParameterValue("octave");
             paramPtr->setValueNotifyingHost(audioProcessor.apvts.getParameter("octave")->convertTo0to1(currentVal + 1));
             octaveValueLabel.setText(juce::String(currentVal + 1), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(octaveValueLabel);
    octaveValueLabel.setJustificationType(juce::Justification::centred);
    octaveValueLabel.setText("0", juce::dontSendNotification); // Default
    // Init label from current state
    octaveValueLabel.setText(juce::String((int)*audioProcessor.apvts.getRawParameterValue("octave")), juce::dontSendNotification);

    // === PATTERN TRANSFORM BUTTONS ===
    addAndMakeVisible(clearButton);
    clearButton.setButtonText("Clear");
    clearButton.onClick = [this] {
        audioProcessor.clearPattern();
        updateInspector();
        repaint();
        // Force DAW to save
        audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
            audioProcessor.apvts.getParameter("swing")->getValue()
        );
    };

    addAndMakeVisible(invertButton);
    invertButton.setButtonText("Invert");
    invertButton.onClick = [this] {
        audioProcessor.invertPattern();
        updateInspector();
        repaint();
        // Force DAW to save
        audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
            audioProcessor.apvts.getParameter("swing")->getValue()
        );
    };

    addAndMakeVisible(reverseButton);
    reverseButton.setButtonText("Reverse");
    reverseButton.onClick = [this] {
        audioProcessor.reversePattern();
        updateInspector();
        repaint();
        // Force DAW to save
        audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
            audioProcessor.apvts.getParameter("swing")->getValue()
        );
    };

    addAndMakeVisible(euclideanLabel);
    euclideanLabel.setText("Euclidean", juce::dontSendNotification);
    euclideanLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(euclideanCombo);
    euclideanCombo.addItem("Off", 1);
    euclideanCombo.addItem("3/8", 2);
    euclideanCombo.addItem("5/8", 3);
    euclideanCombo.addItem("5/16", 4);
    euclideanCombo.addItem("7/16", 5);
    euclideanCombo.addItem("9/16", 6);
    euclideanCombo.setSelectedId(1, juce::dontSendNotification);
    euclideanCombo.onChange = [this] {
        int id = euclideanCombo.getSelectedId();
        int numSteps = (int)*audioProcessor.apvts.getRawParameterValue("numSteps");
        
        if (id == 2) audioProcessor.euclideanPattern(3, 8);
        else if (id == 3) audioProcessor.euclideanPattern(5, 8);
        else if (id == 4) audioProcessor.euclideanPattern(5, 16);
        else if (id == 5) audioProcessor.euclideanPattern(7, 16);
        else if (id == 6) audioProcessor.euclideanPattern(9, 16);
        
        if (id != 1) {
            updateInspector();
            repaint();
            // Force DAW to save
            audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                audioProcessor.apvts.getParameter("swing")->getValue()
            );
        }
    };

    // === TRACK CONTROL BUTTONS ===
    addAndMakeVisible(addTrackButton);
    addTrackButton.setButtonText("+");
    addTrackButton.setTooltip("Add Track");
    addTrackButton.onClick = [this] {
        audioProcessor.addTrack();
        rebuildTrackControls();
        updateTracksLabel();
        repaint();
    };

    addAndMakeVisible(removeTrackButton);
    removeTrackButton.setButtonText("-");
    removeTrackButton.setTooltip("Remove Last Track");
    removeTrackButton.onClick = [this] {
        audioProcessor.removeTrack();
        rebuildTrackControls();
        updateTracksLabel();
        repaint();
    };

    addAndMakeVisible(duplicateTrackButton);
    duplicateTrackButton.setButtonText("Dup");
    duplicateTrackButton.setTooltip("Duplicate Current Track");
    duplicateTrackButton.onClick = [this] {
        // Duplicate the current track
        if (audioProcessor.currentTrack >= 0 && audioProcessor.currentTrack < audioProcessor.getNumTracks()) {
            auto& currentTrack = audioProcessor.tracks[audioProcessor.currentTrack];
            
            // Create a copy of the current track
            std::vector<StepSequencerAudioProcessor::Step> newTrack = currentTrack;
            audioProcessor.tracks.push_back(newTrack);
            audioProcessor.trackRepeat.push_back(audioProcessor.trackRepeat[audioProcessor.currentTrack]);
            audioProcessor.trackEnabled.push_back(true);
            
            // Switch to the new duplicated track
            audioProcessor.switchToTrack(audioProcessor.getNumTracks() - 1);
            
            rebuildTrackControls();
            updateTracksLabel();
            updateInspector();
            repaint();
            
            // Force DAW to save
            audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                audioProcessor.apvts.getParameter("swing")->getValue()
            );
        }
    };

    // Initialize track controls
    rebuildTrackControls();

    // === PER-STEP SLIDERS (Vertical Bars) ===
    
    for (int i = 0; i < MAX_STEP_KNOBS; ++i) {
        // Velocity slider (vertical bar)
        addAndMakeVisible(stepVelocityKnobs[i]);
        stepVelocityKnobs[i].setName("Velocity");
        stepVelocityKnobs[i].setSliderStyle(juce::Slider::LinearVertical);
        stepVelocityKnobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        stepVelocityKnobs[i].setRange(0, 127, 1);
        stepVelocityKnobs[i].setValue(100);
        stepVelocityKnobs[i].setColour(juce::Slider::backgroundColourId, juce::Colours::black);
        stepVelocityKnobs[i].setColour(juce::Slider::trackColourId, juce::Colours::orange.darker(0.3f));
        stepVelocityKnobs[i].setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        stepVelocityKnobs[i].onValueChange = [this, i] {
             if (audioProcessor.steps && i < (int)audioProcessor.steps->size()) {
                (*audioProcessor.steps)[i].velocity = (int)stepVelocityKnobs[i].getValue();
                
                // Force DAW to save
                audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                    audioProcessor.apvts.getParameter("swing")->getValue()
                );
                
                repaint();
            }
        };
        
        // Probability slider (vertical bar)
        addAndMakeVisible(stepProbabilityKnobs[i]);
        stepProbabilityKnobs[i].setName("Probability");
        stepProbabilityKnobs[i].setSliderStyle(juce::Slider::LinearVertical);
        stepProbabilityKnobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        stepProbabilityKnobs[i].setRange(0.0, 1.0, 0.01);
        stepProbabilityKnobs[i].setValue(1.0);
        stepProbabilityKnobs[i].setColour(juce::Slider::backgroundColourId, juce::Colours::black);
        stepProbabilityKnobs[i].setColour(juce::Slider::trackColourId, juce::Colours::green.darker(0.3f));
        stepProbabilityKnobs[i].setColour(juce::Slider::thumbColourId, juce::Colours::green);
        stepProbabilityKnobs[i].onValueChange = [this, i] {
            if (audioProcessor.steps && i < (int)audioProcessor.steps->size()) {
                (*audioProcessor.steps)[i].prob = (float)stepProbabilityKnobs[i].getValue();
                
                // Force DAW to save
                audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                    audioProcessor.apvts.getParameter("swing")->getValue()
                );
                
                repaint();
            }
        };
    }
    
    updateTracksLabel();
    updateInspector(); // Sync sliders with current step data on startup
    setSize(1200, 800);
    startTimer(30);
}

StepSequencerAudioProcessorEditor::~StepSequencerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
    numStepsAttachment.reset();
    rateAttachment.reset();
    swingAttachment.reset();
    keyAttachment.reset();
    scaleAttachment.reset();
}

bool StepSequencerAudioProcessorEditor::isNoteInScale(int midiNote, int rootNote, int scaleType)
{
    int pitchClass = midiNote % 12;
    int relativePitch = (pitchClass - rootNote + 12) % 12;
    
    // Scale patterns (intervals from root)
    static const int chromatic[] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static const int major[] = {0,2,4,5,7,9,11};
    static const int minor[] = {0,2,3,5,7,8,10};
    static const int dorian[] = {0,2,3,5,7,9,10};
    static const int phrygian[] = {0,1,3,5,7,8,10};
    static const int mixolydian[] = {0,2,4,5,7,9,10};
    static const int pentatonic[] = {0,2,4,7,9};
    
    const int* scale = nullptr;
    int scaleSize = 0;
    
    switch(scaleType) {
        case 0: scale = chromatic; scaleSize = 12; break;
        case 1: scale = major; scaleSize = 7; break;
        case 2: scale = minor; scaleSize = 7; break;
        case 3: scale = dorian; scaleSize = 7; break;
        case 4: scale = phrygian; scaleSize = 7; break;
        case 5: scale = mixolydian; scaleSize = 7; break;
        case 6: scale = pentatonic; scaleSize = 5; break;
        default: return true;
    }
    
    for (int i = 0; i < scaleSize; ++i) {
        if (relativePitch == scale[i]) return true;
    }
    return false;
}

void StepSequencerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Modern Dark Background
    g.fillAll (juce::Colour(0xff121212));
    
    // Draw Vertical Step Lanes (WRAPPING GRID)
    if (!stepGridArea.isEmpty()) {
        int numSteps = (int)*audioProcessor.apvts.getRawParameterValue("numSteps");
        if (numSteps < 1) numSteps = 16;
        
        int stepsPerRow = 16;
        // Calculate how many rows we need
        int numRows = (numSteps + stepsPerRow - 1) / stepsPerRow;
        if (numRows < 1) numRows = 1;

        float laneWidth = stepGridArea.getWidth() / (float)stepsPerRow;
        float rowHeight = stepGridArea.getHeight() / (float)numRows;
        float buttonHeight = rowHeight * 0.3f; // Top 30% is button
        
        int beatsPerBar = audioProcessor.timeSignatureNumerator;
        int stepsPerBeat = (beatsPerBar > 0) ? (16 / beatsPerBar) : 4;
        
        for (int i = 0; i < numSteps && i < MAX_STEP_KNOBS; ++i) {
            // Calculate grid position
            int row = i / stepsPerRow;
            int col = i % stepsPerRow;
            
            float x = stepGridArea.getX() + col * laneWidth;
            float y = stepGridArea.getY() + row * rowHeight;
            
            // Draw lane background (alternating for readability)
            bool isBeatStart = (i % stepsPerBeat == 0);
            
            juce::Colour laneColor = (col % 2 == 0) ? juce::Colour(0xff1a1a1a) : juce::Colour(0xff1e1e1e);
            
            // Distinct darkness for alternate rows
            if (row % 2 != 0) laneColor = laneColor.darker(0.05f);

            if (isBeatStart) laneColor = laneColor.brighter(0.05f); // Subtle highlight for beat start
            
            g.setColour(laneColor);
            g.fillRect(x, y, laneWidth, rowHeight);
            
            // Draw vertical separator line
            if (col > 0) {
                g.setColour(isBeatStart ? juce::Colour(0xff444444) : juce::Colour(0xff2a2a2a));
                g.drawLine(x, y, x, y + rowHeight, 1.0f);
            }
            
            // Draw horizontal row separator
            if (row > 0) {
                 g.setColour(juce::Colour(0xff000000));
                 g.drawLine(x, y, x + laneWidth, y, 1.0f);
            }
            
            if (i >= (int)STEPS.size()) continue;
            
            bool isActive = STEPS[i].active;
            bool isPlayhead = (i == audioProcessor.currentStepIndex && audioProcessor.isPlaying);
            bool isSelected = false;
            for (int s : selectedSteps) if (s == i) { isSelected = true; break; }
            
            float velocity = (float)STEPS[i].velocity / 127.0f;
            
            // --- Step Button (Top Section) ---
            juce::Rectangle<float> buttonRect(x + 4, y + 4, laneWidth - 8, buttonHeight - 8);
            
            juce::Colour buttonBaseColor;
            if (isActive) {
                // Active: Orange with velocity brightness
                buttonBaseColor = juce::Colour(0xffff9900).withBrightness(0.6f + velocity * 0.4f);
            } else {
                // Inactive: Dark grey/glassy
                buttonBaseColor = juce::Colour(0xff333333);
            }
            
            // Button fill
            g.setColour(buttonBaseColor);
            g.fillRoundedRectangle(buttonRect, 4.0f);
            
            // Selection Ring
            if (isSelected) {
                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(buttonRect, 4.0f, 2.0f);
            }

            // Active glow/highlight
            if (isActive) {
                g.setColour(juce::Colours::white.withAlpha(0.2f));
                g.fillRoundedRectangle(buttonRect.removeFromTop(buttonRect.getHeight() * 0.5f), 4.0f);
            }
            
            // Playhead Indicator
            if (isPlayhead) {
                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(buttonRect.expanded(2.0f), 4.0f, 2.0f);
                
                // Also highlight the whole grid cell slightly
                g.setColour(juce::Colours::white.withAlpha(0.05f));
                g.fillRect(x, y, laneWidth, rowHeight);
            }
            
            // Note Name Text
            if (isActive) {
                g.setColour(juce::Colours::black);
                g.setFont(12.0f);
                g.setFont(g.getCurrentFont().boldened());
                juce::String noteName = juce::MidiMessage::getMidiNoteName(STEPS[i].note, true, true, 3);
                g.drawText(noteName, buttonRect, juce::Justification::centred);
            }
        }
        
        // Draw Beat Numbers
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(10.0f);
        for (int beat = 0; beat * stepsPerBeat < numSteps; ++beat) {
            int i = beat * stepsPerBeat;
            int row = i / stepsPerRow;
            int col = i % stepsPerRow;
            
            float x = stepGridArea.getX() + (col * laneWidth);
            float y = stepGridArea.getY() + (row * rowHeight);
            
            g.drawText(juce::String(beat + 1), x + 2, y + 2, 20, 10, juce::Justification::left);
        }
    }
    
    // Draw Piano Keyboard Background
    if (!pianoArea.isEmpty()) {
        g.setColour(juce::Colour(0xff000000));
        g.fillRect(pianoArea);
        
        int rootNote = (int)*audioProcessor.apvts.getRawParameterValue("key");
        int scaleType = (int)*audioProcessor.apvts.getRawParameterValue("scale");
        
        int startNote = 48; // C3
        int endNote = 72;   // C5
        
        // Count white keys
        int numWhite = 0;
        for (int n = startNote; n <= endNote; ++n) {
            int p = n % 12;
            if (p==0||p==2||p==4||p==5||p==7||p==9||p==11) numWhite++;
        }
        
        float keyW = (float)pianoArea.getWidth() / (float)numWhite;
        float h = (float)pianoArea.getHeight();
        float x = (float)pianoArea.getX();
        float y = (float)pianoArea.getY();
        
        int selectedNote = -1;
        if (!selectedSteps.empty()) selectedNote = STEPS[selectedSteps.back()].note;
        
        // Draw White Keys
        int wIdx = 0;
        for (int n = startNote; n <= endNote; ++n) {
            int p = n % 12;
            if (p==0||p==2||p==4||p==5||p==7||p==9||p==11) {
                bool inScale = isNoteInScale(n, rootNote, scaleType);
                juce::Rectangle<float> r(x + wIdx * keyW, y, keyW, h);
                
                if (n == selectedNote) g.setColour(juce::Colours::red);
                else if (!inScale) g.setColour(juce::Colour(220, 220, 220)); // Light grey for disabled keys
                else g.setColour(juce::Colours::white);
                
                g.fillRect(r.reduced(1.0f));
                
                if (p == 0) {
                     g.setColour(juce::Colours::black);
                     g.setFont(12.0f);
                     g.drawText("C" + juce::String(n/12 - 1), r.removeFromBottom(20), juce::Justification::centred);
                }
                
                wIdx++;
            }
        }
        
        // Draw Black Keys (Overlay)
        wIdx = 0;
        for (int n = startNote; n < endNote; ++n) {
            int p = n % 12;
            if (p==0||p==2||p==4||p==5||p==7||p==9||p==11) {
                wIdx++;
            } else {
                bool inScale = isNoteInScale(n, rootNote, scaleType);
                float kw = keyW * 0.6f;
                float kh = h * 0.6f;
                float kx = x + (wIdx * keyW) - (kw * 0.5f);
                
                juce::Rectangle<float> r(kx, y, kw, kh);
                
                if (n == selectedNote) g.setColour(juce::Colours::red);
                else if (!inScale) g.setColour(juce::Colour(140, 140, 140)); // Medium grey for disabled black keys
                else g.setColour(juce::Colours::black);
                
                g.fillRect(r);
                g.setColour(juce::Colours::grey);
                g.drawRect(r, 1.0f);
            }
        }
    }
}

void StepSequencerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // === TOP CONTROL ROW (Bigger knobs: 120px) ===
    auto controlRow = area.removeFromTop(120);
    
    // -- Left Group: Rhythm & Swing --
    auto stepsArea = controlRow.removeFromLeft(120);
    numStepsLabel.setBounds(stepsArea.removeFromTop(15));
    numStepsKnob.setBounds(stepsArea);
    
    controlRow.removeFromLeft(5);
    auto rateArea = controlRow.removeFromLeft(120);
    rateLabel.setBounds(rateArea.removeFromTop(15));
    rateKnob.setBounds(rateArea);
    
    controlRow.removeFromLeft(5);
    auto swingArea = controlRow.removeFromLeft(120);
    swingLabel.setBounds(swingArea.removeFromTop(15));
    swingKnob.setBounds(swingArea);
    
    // -- Center Group: Generation --
    // Flexible spacer? Or fixed offset? Let's use flexible if possible, or just push to right.
    // User wants Key/Scale/Octave on Right.
    // Let's put Random/Mutate in the middle or next to Swing.
    
    controlRow.removeFromLeft(20);
    auto randomArea = controlRow.removeFromLeft(120);
    randomizeLabel.setBounds(randomArea.removeFromTop(15));
    randomizeKnob.setBounds(randomArea);
    
    controlRow.removeFromLeft(5);
    auto mutateArea = controlRow.removeFromLeft(120);
    mutateLabel.setBounds(mutateArea.removeFromTop(15));
    mutateKnob.setBounds(mutateArea);
    
    // -- Right Group: PITCH (Key, Scale, Octave) --
    // Push to far right?
    // Let's take the remaining width and align right, or just position explicitly.
    // Since we are using removeFromLeft, the remainder is on the right.
    // But Key/Scale need to be WIDER.
    
    auto pitchArea = controlRow.removeFromRight(280); // Allocated right side space
    
    // Key & Scale (Row 1 of Right Side)
    auto keyScaleRow = pitchArea.removeFromTop(40);
    keyLabel.setBounds(keyScaleRow.removeFromLeft(30));
    keyCombo.setBounds(keyScaleRow.removeFromLeft(80)); // Wider Key
    keyScaleRow.removeFromLeft(10);
    scaleLabel.setBounds(keyScaleRow.removeFromLeft(40));
    scaleCombo.setBounds(keyScaleRow); // Takes remaining width (Scale)
    
    pitchArea.removeFromTop(10); // Spacing
    
    // Octave (Row 2 of Right Side)
    auto octaveRow = pitchArea.removeFromTop(30);
    octaveLabel.setBounds(octaveRow.removeFromLeft(50));
    
    // +/- Buttons and Value
    octaveMinusButton.setBounds(octaveRow.removeFromLeft(30));
    octaveRow.removeFromLeft(5);
    octaveValueLabel.setBounds(octaveRow.removeFromLeft(40));
    octaveRow.removeFromLeft(5);
    octavePlusButton.setBounds(octaveRow.removeFromLeft(30));

    area.removeFromTop(10);
    
    // === PATTERN TRANSFORM ROW ===
    auto transformRow = area.removeFromTop(35);
    
    clearButton.setBounds(transformRow.removeFromLeft(70));
    transformRow.removeFromLeft(5);
    
    invertButton.setBounds(transformRow.removeFromLeft(70));
    transformRow.removeFromLeft(5);
    
    reverseButton.setBounds(transformRow.removeFromLeft(70));
    transformRow.removeFromLeft(20);
    
    auto euclideanArea = transformRow.removeFromLeft(150);
    euclideanLabel.setBounds(euclideanArea.removeFromTop(15));
    euclideanCombo.setBounds(euclideanArea);

    area.removeFromTop(10);
    
    // === LEFT SIDEBAR ===
    auto leftSidebar = area.removeFromLeft(180);
    
    tracksLabel.setBounds(leftSidebar.removeFromTop(20));
    leftSidebar.removeFromTop(5);
    
    // +/- buttons to add/remove tracks
    auto addRemoveRow = leftSidebar.removeFromTop(25);
    addTrackButton.setBounds(addRemoveRow.removeFromLeft(40));
    addRemoveRow.removeFromLeft(5);
    removeTrackButton.setBounds(addRemoveRow.removeFromLeft(40));
    addRemoveRow.removeFromLeft(5);
    duplicateTrackButton.setBounds(addRemoveRow.removeFromLeft(50));
    leftSidebar.removeFromTop(10);
    
    // Show all track buttons with enable toggles and repeat dials (dynamic)
    int numTracks = audioProcessor.getNumTracks();
    for (int t = 0; t < numTracks; ++t) {
        if (t < (int)trackButtons.size()) {
            auto trackRow = leftSidebar.removeFromTop(40);
            
            // Track Select Button (1, 2, 3...)
            auto buttonArea = trackRow.removeFromLeft(40);
            trackButtons[t]->setBounds(buttonArea.removeFromTop(30));
            
            trackRow.removeFromLeft(5);
            
            // Toggle On/Off
            trackEnableButtons[t]->setBounds(trackRow.removeFromLeft(40).removeFromTop(30));
            
            trackRow.removeFromLeft(5);
            
            // Repeat Slider
            if (t < (int)trackRepeatSliders.size()) {
                trackRepeatSliders[t]->setBounds(trackRow.removeFromLeft(50));
            }
            
            leftSidebar.removeFromTop(5);
        }
    }
    
    area.removeFromLeft(8);
    
    // === BOTTOM PIANO ===
    pianoArea = area.removeFromBottom(70);
    area.removeFromBottom(8);
    
    // === VERTICAL STEP LANES (WRAPPING) ===
    stepGridArea = area;
    
    int numSteps = (int)*audioProcessor.apvts.getRawParameterValue("numSteps");
    if (numSteps < 1) numSteps = 16;
    
    int stepsPerRow = 16;
    int numRows = (numSteps + stepsPerRow - 1) / stepsPerRow;
    if (numRows < 1) numRows = 1;

    float laneWidth = stepGridArea.getWidth() / (float)stepsPerRow;
    float rowHeight = stepGridArea.getHeight() / (float)numRows;
    
    for (int i = 0; i < MAX_STEP_KNOBS; ++i) {
        if (i < numSteps && i < (int)STEPS.size()) {
            int row = i / stepsPerRow;
            int col = i % stepsPerRow;
            
            float x = stepGridArea.getX() + col * laneWidth;
            float y = stepGridArea.getY() + row * rowHeight;
            
            float buttonHeight = rowHeight * 0.3f;
            
            stepVelocityKnobs[i].setValue(STEPS[i].velocity, juce::dontSendNotification);
            stepProbabilityKnobs[i].setValue(STEPS[i].prob, juce::dontSendNotification);
            
            float slidersTotalH = rowHeight - buttonHeight - 10;
            float velHeight = slidersTotalH * 0.5f;
            float probHeight = slidersTotalH * 0.5f;
            
            float velY = y + buttonHeight + 5;
            stepVelocityKnobs[i].setBounds(x + 1, velY, laneWidth - 2, velHeight);
            stepVelocityKnobs[i].setVisible(true);
            
            float probY = velY + velHeight;
            stepProbabilityKnobs[i].setBounds(x + 1, probY, laneWidth - 2, probHeight);
            stepProbabilityKnobs[i].setVisible(true);
        } else {
            stepVelocityKnobs[i].setVisible(false);
            stepProbabilityKnobs[i].setVisible(false);
        }
    }
}

void StepSequencerAudioProcessorEditor::timerCallback() { repaint(); }

void StepSequencerAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    // 1. Piano Interaction
    if (pianoArea.contains(event.getPosition())) {
        int startNote = 48; // C3
        int endNote = 72;   // C5
        
        int numWhite = 0;
        for (int n = startNote; n <= endNote; ++n) {
            int p = n % 12;
            if (p==0||p==2||p==4||p==5||p==7||p==9||p==11) numWhite++;
        }
        
        float keyW = (float)pianoArea.getWidth() / (float)numWhite;
        float h = (float)pianoArea.getHeight();
        float x = (float)pianoArea.getX();
        float y = (float)pianoArea.getY();
        
        int clickedNote = -1;
        
        // Check Black Keys First
        int wIdx = 0;
        for (int n = startNote; n < endNote; ++n) {
            int p = n % 12;
            if (p==0||p==2||p==4||p==5||p==7||p==9||p==11) {
                wIdx++;
            } else {
                float kw = keyW * 0.6f;
                float kh = h * 0.6f;
                float kx = x + (wIdx * keyW) - (kw * 0.5f);
                juce::Rectangle<float> r(kx, y, kw, kh);
                
                if (r.contains(event.position)) {
                    clickedNote = n;
                    break;
                }
            }
        }
        
        // Check White Keys
        if (clickedNote == -1) {
            wIdx = 0;
            for (int n = startNote; n <= endNote; ++n) {
                int p = n % 12;
                if (p==0||p==2||p==4||p==5||p==7||p==9||p==11) {
                    float kx = x + (wIdx * keyW);
                    juce::Rectangle<float> r(kx, y, keyW, h);
                    
                    if (r.contains(event.position)) {
                        clickedNote = n;
                        break;
                    }
                    wIdx++;
                }
            }
        }
        
        if (clickedNote != -1) {
            // Apply to selected steps
            if (!selectedSteps.empty()) {
                for (int idx : selectedSteps) {
                    if (idx >= 0 && idx < (int)STEPS.size()) {
                        STEPS[idx].note = clickedNote;
                        STEPS[idx].active = true;
                    }
                }
                
                // Force DAW to save
                audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                    audioProcessor.apvts.getParameter("swing")->getValue()
                );
            }
            updateInspector();
            repaint();
        }
        return;
    }

    // 2. Step Lane Interaction
    if (stepGridArea.contains(event.getPosition())) {
        int numSteps = (int)*audioProcessor.apvts.getRawParameterValue("numSteps");
        if (numSteps < 1) numSteps = 16;
        
        int stepsPerRow = 16;
        int numRows = (numSteps + stepsPerRow - 1) / stepsPerRow;
        if (numRows < 1) numRows = 1;

        float laneWidth = stepGridArea.getWidth() / (float)stepsPerRow;
        float rowHeight = stepGridArea.getHeight() / (float)numRows;
        
        float relativeX = event.getPosition().x - stepGridArea.getX();
        float relativeY = event.getPosition().y - stepGridArea.getY();
        
        int col = (int)(relativeX / laneWidth);
        int row = (int)(relativeY / rowHeight);
        
        int laneIndex = row * stepsPerRow + col;
        float buttonHeight = rowHeight * 0.3f;
        float rowRelativeY = relativeY - (row * rowHeight);
        
        // Only respond if clicking in the button area (top 30%)
        if (laneIndex >= 0 && laneIndex < numSteps && laneIndex < (int)STEPS.size() && rowRelativeY < buttonHeight) {
            // Selection Logic
            if (!event.mods.isShiftDown()) selectedSteps.clear();
            
            // Add to selection if not present
            bool alreadySelected = false;
            for (int s : selectedSteps) if (s == laneIndex) alreadySelected = true;
            if (!alreadySelected) selectedSteps.push_back(laneIndex);
            
            // Activation Logic:
            // Single click activates if inactive.
            // Does NOT toggle off active steps (that requires double click).
            if (!STEPS[laneIndex].active) {
                STEPS[laneIndex].active = true;
            }
            
            // Force DAW to save by actually changing a parameter
            audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                audioProcessor.apvts.getParameter("swing")->getValue()
            );
            
            updateInspector();
            repaint();
        }
    }
}

void StepSequencerAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Double-click to clear a step
    if (stepGridArea.contains(event.getPosition())) {
        int numSteps = (int)*audioProcessor.apvts.getRawParameterValue("numSteps");
        if (numSteps < 1) numSteps = 16;
        
        int stepsPerRow = 16;
        int numRows = (numSteps + stepsPerRow - 1) / stepsPerRow;
        if (numRows < 1) numRows = 1;

        float laneWidth = stepGridArea.getWidth() / (float)stepsPerRow;
        float rowHeight = stepGridArea.getHeight() / (float)numRows;
        
        float relativeX = event.getPosition().x - stepGridArea.getX();
        float relativeY = event.getPosition().y - stepGridArea.getY();
        
        int col = (int)(relativeX / laneWidth);
        int row = (int)(relativeY / rowHeight);
        
        int laneIndex = row * stepsPerRow + col;
        float buttonHeight = rowHeight * 0.3f;
        float rowRelativeY = relativeY - (row * rowHeight);
        
        if (laneIndex >= 0 && laneIndex < numSteps && laneIndex < (int)STEPS.size() && rowRelativeY < buttonHeight) {
            STEPS[laneIndex].active = false;
            
            // Force DAW to save
            audioProcessor.apvts.getParameter("swing")->setValueNotifyingHost(
                audioProcessor.apvts.getParameter("swing")->getValue()
            );
            
            updateInspector();
            repaint();
        }
    }
}

void StepSequencerAudioProcessorEditor::updateTracksLabel()
{
    int enabledCount = 0;
    for (size_t i = 0; i < audioProcessor.trackEnabled.size(); ++i) {
        if (audioProcessor.trackEnabled[i]) enabledCount++;
    }
    tracksLabel.setText("Tracks: " + juce::String(enabledCount) + "/" + juce::String(audioProcessor.getNumTracks()), juce::dontSendNotification);
}

void StepSequencerAudioProcessorEditor::rebuildTrackControls()
{
    // Clear existing controls
    trackButtons.clear();
    trackEnableButtons.clear();
    trackRepeatSliders.clear();
    
    int numTracks = audioProcessor.getNumTracks();
    
    for (int t = 0; t < numTracks; ++t) {
        // Track select button
        auto trackBtn = std::make_unique<juce::TextButton>();
        trackBtn->setButtonText(juce::String(t + 1));
        trackBtn->setClickingTogglesState(true);
        trackBtn->setRadioGroupId(1001);
        trackBtn->onClick = [this, t] {
            audioProcessor.switchToTrack(t);
            updateInspector(); // Sync UI with new track's data
            updateTracksLabel();
            repaint();
        };
        addAndMakeVisible(trackBtn.get());
        if (t == audioProcessor.currentTrack) trackBtn->setToggleState(true, juce::dontSendNotification);
        trackButtons.push_back(std::move(trackBtn));
        
        // Track enable toggle button
        auto enableBtn = std::make_unique<juce::TextButton>();
        enableBtn->setButtonText("On");
        enableBtn->setClickingTogglesState(true);
        enableBtn->setToggleState(audioProcessor.trackEnabled[t], juce::dontSendNotification);
        auto* enableBtnPtr = enableBtn.get();
        enableBtn->onClick = [this, t, enableBtnPtr] {
            audioProcessor.trackEnabled[t] = enableBtnPtr->getToggleState();
            updateTracksLabel();
            repaint();
        };
        addAndMakeVisible(enableBtn.get());
        trackEnableButtons.push_back(std::move(enableBtn));
        
        // Track repeat slider
        auto repeatSlider = std::make_unique<juce::Slider>();
        repeatSlider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        repeatSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 30, 15);
        repeatSlider->setRange(1.0, 16.0, 1.0);
        if (t < (int)audioProcessor.trackRepeat.size()) {
            repeatSlider->setValue(audioProcessor.trackRepeat[t]);
        }
        repeatSlider->setMouseDragSensitivity(100);
        auto* repeatSliderPtr = repeatSlider.get();
        repeatSlider->onValueChange = [this, t, repeatSliderPtr] {
            if (t < (int)audioProcessor.trackRepeat.size()) {
                audioProcessor.trackRepeat[t] = (int)repeatSliderPtr->getValue();
            }
        };
        addAndMakeVisible(repeatSlider.get());
        trackRepeatSliders.push_back(std::move(repeatSlider));
    }
}

void StepSequencerAudioProcessorEditor::updateInspector()
{
    // Safety check
    if (!audioProcessor.steps || audioProcessor.steps->empty()) {
        return;
    }
    
    // Sync per-step sliders with current track's steps
    int numSteps = (int)*audioProcessor.apvts.getRawParameterValue("numSteps");
    for (int i = 0; i < MAX_STEP_KNOBS; ++i) {
        if (i < numSteps && i < (int)STEPS.size()) {
            stepVelocityKnobs[i].setValue(STEPS[i].velocity, juce::dontSendNotification);
            stepProbabilityKnobs[i].setValue(STEPS[i].prob, juce::dontSendNotification);
        }
    }
    
    // Trigger resized to show/hide controls based on step count
    resized();
}
