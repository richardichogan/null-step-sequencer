/*
  ==============================================================================
    PluginProcessor.cpp
    Step Sequencer - MIDI Step Sequencer to Component Spec
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>

//==============================================================================
StepSequencerAudioProcessor::StepSequencerAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::disabled(), false)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true) // Dummy output for Live VST3 compatibility
                       ),
       apvts (*this, nullptr, "Parameters", createParams())
{
    // Initialize all tracks
    // Initialize with 1 track by default
    tracks.resize(1);
    trackRepeat.resize(1, 1);
    trackEnabled.resize(1, true);
    
    tracks[0].resize(32);
    for (auto& s : tracks[0]) {
        s.active = false; // Default to silence
        s.note = 60; // C3
        s.velocity = 100;
        s.gate = 0.5f;
    }
    steps = &tracks[0]; // Point to first track
}

StepSequencerAudioProcessor::~StepSequencerAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout StepSequencerAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Global Parameters
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("numSteps", 1), "Steps", 1, 32, 16));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("rate", 1), "Rate",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2)); // Default 1/16
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("swing", 1), "Swing", 0.0f, 100.0f, 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("key", 1), "Key",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0)); // Default C
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("scale", 1), "Scale",
        juce::StringArray { "Chromatic", "Major", "Minor", "Dorian", "Phrygian", "Mixolydian", "Pentatonic" }, 0)); // Default Chromatic

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("octave", 1), "Octave", -3, 3, 0)); // Default 0

    // Hidden parameter to force DAW to detect state changes
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("_stateVersion", 1), "_StateVersion", 0, 999999, 0));

    return { params.begin(), params.end() };
}

const juce::String StepSequencerAudioProcessor::getName() const { return "Step Sequencer"; }
bool StepSequencerAudioProcessor::acceptsMidi() const { return true; }
bool StepSequencerAudioProcessor::producesMidi() const { return true; }
bool StepSequencerAudioProcessor::isMidiEffect() const { return false; }
double StepSequencerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int StepSequencerAudioProcessor::getNumPrograms() { return 1; }
int StepSequencerAudioProcessor::getCurrentProgram() { return 0; }
void StepSequencerAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String StepSequencerAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void StepSequencerAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

//==============================================================================
void StepSequencerAudioProcessor::prepareToPlay (double sRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    sampleRate = (sRate > 0.0) ? sRate : 44100.0;
    accumulatedSamples = 0.0;
    currentStepIndex = 0;
    samplesRemainingForGate = 0;
    lastNote = -1;
}

void StepSequencerAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool StepSequencerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // IS_SYNTH logic: 0 Input, Stereo Output
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()) return false;
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled()) return false;
    return true;
}
#endif

void StepSequencerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear dummy audio buffer to silence
    buffer.clear();

    juce::AudioPlayHead* playHead = getPlayHead();
    if (!playHead) return;

    bool hostIsPlaying = false;
    double hostBPM = 120.0;

    // Modern PlayHead API
    if (auto positionOpt = playHead->getPosition())
    {
        const auto& pos = *positionOpt;
        
        if (auto ts = pos.getTimeSignature()) {
            timeSignatureNumerator = ts->numerator;
            timeSignatureDenominator = ts->denominator;
        }
        
        if (auto bpm = pos.getBpm()) {
            hostBPM = *bpm;
        }
        
        hostIsPlaying = pos.getIsPlaying();
    }
    
    // Check state change
    if (!hostIsPlaying) {
        // Send All Notes Off if we just stopped
        if (isPlaying) {
            midiMessages.addEvent(juce::MidiMessage::allNotesOff(1), 0);
            isPlaying = false;
            samplesRemainingForGate = 0;
            accumulatedSamples = 0;
        }
        return;
    }
    
    isPlaying = true;
    currentBPM = hostBPM > 0 ? hostBPM : 120.0;
    
    // Calculate Timing
    double samplesPerMinute = sampleRate * 60.0;
    double currentSamplesPerBeat = samplesPerMinute / currentBPM;
    
    // Rate Multiplier (simplified for 1/16 default)
    int rateIndex = (int) *apvts.getRawParameterValue("rate");
    double rateMult = 1.0;
    if (rateIndex == 0) rateMult = 1.0; // 1/4 (1 beat)
    else if (rateIndex == 1) rateMult = 0.5; // 1/8
    else if (rateIndex == 2) rateMult = 0.25; // 1/16
    else if (rateIndex == 3) rateMult = 0.125; // 1/32
    
    double samplesPerStep = currentSamplesPerBeat * rateMult;
    
    // Safety check
    if (samplesPerStep < 32.0) samplesPerStep = 32.0;

    int numSamples = buffer.getNumSamples();
    int sampleOffset = 0;
    
    while (sampleOffset < numSamples)
    {
        // 1. Handle Note Offs
        if (samplesRemainingForGate > 0) {
            samplesRemainingForGate--;
            if (samplesRemainingForGate == 0 && lastNote != -1) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNote), sampleOffset);
                lastNote = -1;
            }
        }
        
        // 2. Scheduler logic
        // If we are at the start of a new step
        accumulatedSamples += 1.0;
        
        if (accumulatedSamples >= samplesPerStep) {
            accumulatedSamples -= samplesPerStep;
            
            // Advance Step
            int numSteps = (int) *apvts.getRawParameterValue("numSteps");
            
            // Check if we finished a full sequence loop
            if (currentStepIndex >= numSteps - 1) {
                // We reached the end of the sequence
                barsPlayedOnCurrentTrack++;
                
                // Track Switch Logic: Count loops, not bars
                if (barsPlayedOnCurrentTrack >= trackRepeat[currentTrack]) {
                    barsPlayedOnCurrentTrack = 0;
                    
                    // Find next enabled track
                    int nextTrack = currentTrack;
                    int numTracks = getNumTracks();
                    
                    // Safety break loop
                    int attempts = 0;
                    while(attempts < numTracks) {
                        nextTrack = (nextTrack + 1) % numTracks;
                        if (trackEnabled[nextTrack]) {
                            switchToTrack(nextTrack);
                            break;
                        }
                        attempts++;
                    }
                }
                currentStepIndex = 0;
            } else {
                currentStepIndex++;
            }
            
            // Trigger New Note
            Step& s = (*steps)[currentStepIndex];
            if (s.active) {
                // If it's a TIED step, we do NOT trigger a new note. 
                // We just let the previous note continue ringing (because its gate was long enough).
                if (!s.isTied) {
                    // Determine velocity and probability
                    if (juce::Random::getSystemRandom().nextFloat() <= s.prob) {
                         // Kill previous note if still ringing (monophonic sequencer)
                         if (lastNote != -1) {
                             midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNote), sampleOffset);
                         }
                         
                         int octaveShift = (int)*apvts.getRawParameterValue("octave");
                         lastNote = juce::jlimit(0, 127, s.note + (octaveShift * 12));
                         
                         midiMessages.addEvent(juce::MidiMessage::noteOn(1, lastNote, (juce::uint8)s.velocity), sampleOffset);
                         
                         // Calculate Gate Length
                         samplesRemainingForGate = (long)(samplesPerStep * s.gate);
                         
                         // DEBUG: if gate is large, print it? No console for VST.
                         // But logic assumes s.gate is e.g. 5.0 for a long tie.
                    }
                }
                // Important: If it IS tied, we don't do anything. 
                // We rely on samplesRemainingForGate from the previous "START" step to keep counting down.
                // NOTE: If the user ties steps but the "start" step wasn't set to a long gate, the note will cut off early.
                // The Editor logic handles setting the start step gate.
            }
        }
        
        sampleOffset++;
    }
}

//==============================================================================
bool StepSequencerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* StepSequencerAudioProcessor::createEditor() { return new StepSequencerAudioProcessorEditor (*this); }

//==============================================================================
void StepSequencerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Ensure all params are consistent before saving
    apvts.state.setProperty("numTracks", getNumTracks(), nullptr);
    
    // Save Track Data manually or as a child tree
    juce::ValueTree tracksTree("TRACKS");
    for (int t = 0; t < getNumTracks(); ++t) {
        juce::ValueTree trackNode("TRACK");
        trackNode.setProperty("index", t, nullptr);
        trackNode.setProperty("repeat", trackRepeat[t], nullptr);
        trackNode.setProperty("enabled", (bool)trackEnabled[t], nullptr);
        
        // Save Steps
        juce::ValueTree stepsTree("STEPS");
        for (int s = 0; s < (int)tracks[t].size(); ++s) {
            const auto& step = tracks[t][s];
            // Save ALL steps (not just non-default ones) to ensure velocity/prob changes on inactive steps are preserved
            juce::ValueTree stepNode("STEP");
            stepNode.setProperty("i", s, nullptr);
            stepNode.setProperty("n", step.note, nullptr);
            stepNode.setProperty("v", step.velocity, nullptr);
            stepNode.setProperty("g", step.gate, nullptr);
            stepNode.setProperty("p", step.prob, nullptr);
            stepNode.setProperty("a", step.active, nullptr);
            stepNode.setProperty("t", step.isTied, nullptr);
            stepsTree.addChild(stepNode, -1, nullptr);
        }
        trackNode.addChild(stepsTree, -1, nullptr);
        tracksTree.addChild(trackNode, -1, nullptr);
    }
    
    // Copy current state from APVTS
    juce::ValueTree currentState = apvts.copyState();
    
    // Remove ALL existing TRACKS nodes to prevent accumulation of history
    while (currentState.getChildWithName("TRACKS").isValid()) {
        currentState.removeChild(currentState.getChildWithName("TRACKS"), nullptr);
    }
        
    currentState.addChild(tracksTree, -1, nullptr);
    
    std::unique_ptr<juce::XmlElement> xml (currentState.createXml());
    
    copyXmlToBinary (*xml, destData);
}

void StepSequencerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName (apvts.state.getType())) {
            juce::ValueTree newState = juce::ValueTree::fromXml (*xmlState);
            apvts.replaceState (newState);
            
            // Restore Tracks
            juce::ValueTree tracksTree = newState.getChildWithName("TRACKS");
            if (tracksTree.isValid()) {
                int numTracksSaved = tracksTree.getNumChildren();
                
                // Clear and resize to match saved state
                tracks.clear();
                trackRepeat.clear();
                trackEnabled.clear();
                tracks.resize((size_t)numTracksSaved);
                trackRepeat.resize((size_t)numTracksSaved, 1);
                trackEnabled.resize((size_t)numTracksSaved, true);
                
                for (int t = 0; t < numTracksSaved; ++t) {
                    juce::ValueTree trackNode = tracksTree.getChild(t);
                    trackRepeat[(size_t)t] = (int)trackNode.getProperty("repeat", 1);
                    trackEnabled[(size_t)t] = (bool)trackNode.getProperty("enabled", true);
                    
                    // Initialize steps default
                    tracks[(size_t)t].resize(32);
                     for (auto& s : tracks[(size_t)t]) { s.active = false; s.isTied = false; s.note = 60; s.velocity = 100; s.prob = 1.0f; }

                    juce::ValueTree stepsTree = trackNode.getChildWithName("STEPS");
                    
                    for (int s = 0; s < stepsTree.getNumChildren(); ++s) {
                        juce::ValueTree stepNode = stepsTree.getChild(s);
                        int idx = (int)stepNode.getProperty("i", -1);
                        bool active = (bool)stepNode.getProperty("a", false);
                        int note = (int)stepNode.getProperty("n", 60);
                        
                        if (idx >= 0 && idx < (int)tracks[(size_t)t].size()) {
                            tracks[(size_t)t][(size_t)idx].note = note;
                            tracks[(size_t)t][(size_t)idx].velocity = (int)stepNode.getProperty("v", 100);
                            tracks[(size_t)t][(size_t)idx].gate = (float)stepNode.getProperty("g", 0.5f);
                            tracks[(size_t)t][(size_t)idx].prob = (float)stepNode.getProperty("p", 1.0f);
                            tracks[(size_t)t][(size_t)idx].active = active;
                            tracks[(size_t)t][(size_t)idx].isTied = (bool)stepNode.getProperty("t", false);
                        }
                    }
                }
                
                // Reset pointers
                currentTrack = 0;
                if (!tracks.empty()) steps = &tracks[0];
            }
        }
    }
    
    // Notify editor to rebuild UI if it exists
    if (auto* editor = dynamic_cast<StepSequencerAudioProcessorEditor*>(getActiveEditor())) {
        editor->rebuildTrackControls();
        editor->updateTracksLabel();
        editor->updateInspector();
        editor->repaint();
    }
}

//==============================================================================
void StepSequencerAudioProcessor::randomizePattern(float amount)
{
    if (amount <= 0.0f) return;
    
    int rootNote = (int)*apvts.getRawParameterValue("key");
    int scaleType = (int)*apvts.getRawParameterValue("scale");
    
    auto& random = juce::Random::getSystemRandom();
    for (auto& s : *steps) {
        // Randomize: Chaos generator - completely replaces values
        if (random.nextFloat() < amount) {
            // Structural changes
            s.active = random.nextFloat() > 0.3f; // 70% chance to be active
            
            // Note changes (Total replacement)
            s.note = getRandomNoteInScale(rootNote, scaleType, 3, 5); // C3-C5
            
            // Timbre changes (Total replacement)
            s.velocity = random.nextInt(60) + 60; // 60-119 range
            s.gate = 0.2f + (random.nextFloat() * 0.8f);
            s.prob = 0.7f + (random.nextFloat() * 0.3f);
            
            // Clear ties usually in randomization to avoid broken chains
            s.isTied = false;
        }
    }
    
    // Safety pass for ties? 
    // Ideally we should validate ties, but for now simple random is fine.
}

void StepSequencerAudioProcessor::mutatePattern(float amount)
{
    if (amount <= 0.0f) return;
    
    int rootNote = (int)*apvts.getRawParameterValue("key");
    int scaleType = (int)*apvts.getRawParameterValue("scale");
    
    auto& random = juce::Random::getSystemRandom();
    for (auto& s : *steps) {
        // Mutate: Evolution - Shifts existing values slightly
        // Apply to ACTIVE steps mostly to preserve structure
        if (s.active && random.nextFloat() < amount) {
            int type = random.nextInt(4);
            
            if (type == 0) {
                // Pitch Shift (Small Interval)
                // Find index in scale and move +/- 1 or 2
                // Simplified: just try to find nearby scale note
                int offset = random.nextBool() ? 1 : -1;
                if (random.nextFloat() > 0.7f) offset *= 2; // Occasional larger jump
                
                // We don't have a "getIndexOfNote" easily, so just brute force search nearby
                int tries = 0;
                int candidate = s.note + offset;
                while (!isNoteInScale(candidate, rootNote, scaleType) && tries < 5) {
                    candidate += (offset > 0 ? 1 : -1);
                    tries++;
                }
                if (isNoteInScale(candidate, rootNote, scaleType)) {
                    s.note = juce::jlimit(0, 127, candidate);
                }
            }
            else if (type == 1) {
                // Velocity Nudge (+/- 15)
                int diff = random.nextInt(30) - 15;
                s.velocity = juce::jlimit(1, 127, s.velocity + diff);
            }
            else if (type == 2) {
                // Gate Nudge (+/- 10%)
                float diff = (random.nextFloat() * 0.2f) - 0.1f;
                s.gate = juce::jlimit(0.1f, 1.0f, s.gate + diff);
            }
            else if (type == 3) {
                 // Probability Nudge
                 float diff = (random.nextFloat() * 0.2f) - 0.1f;
                 s.prob = juce::jlimit(0.0f, 1.0f, s.prob + diff);
            }
        }
        else if (!s.active && random.nextFloat() < (amount * 0.1f)) {
             // Very rare chance to revive a dead step
             s.active = true;
             s.velocity = 80;
        }
    }
}

void StepSequencerAudioProcessor::switchToTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < getNumTracks() && trackIndex != currentTrack) {
        currentTrack = trackIndex;
        steps = &tracks[currentTrack]; // Update pointer
        currentStepIndex = 0; // Reset step position when switching tracks
    }
}

void StepSequencerAudioProcessor::clearPattern()
{
    // Reset all steps in current track to default state
    for (auto& s : *steps) {
        s.active = false;
        s.isTied = false;
        s.note = 60;
        s.velocity = 100;
        s.gate = 0.5f;
        s.prob = 1.0f;
    }
}

void StepSequencerAudioProcessor::invertPattern()
{
    // Flip the active/inactive state of all steps
    for (auto& s : *steps) {
        s.active = !s.active;
    }
}

void StepSequencerAudioProcessor::reversePattern()
{
    // Reverse the order of steps (only within the active numSteps range)
    int numSteps = (int)*apvts.getRawParameterValue("numSteps");
    if (numSteps > 0 && numSteps <= (int)steps->size()) {
        std::reverse(steps->begin(), steps->begin() + numSteps);
    }
}

void StepSequencerAudioProcessor::euclideanPattern(int hits, int steps)
{
    // Bjorklund algorithm for Euclidean rhythm generation
    // Distribute 'hits' evenly across 'steps'
    
    if (hits <= 0 || steps <= 0 || hits > steps) return;
    
    // Clear pattern first
    for (auto& s : *this->steps) {
        s.active = false;
        s.isTied = false;
    }
    
    // Simple Euclidean distribution using Bresenham-like algorithm
    int bucket = 0;
    for (int i = 0; i < steps && i < (int)this->steps->size(); ++i) {
        bucket += hits;
        if (bucket >= steps) {
            bucket -= steps;
            (*this->steps)[i].active = true;
        }
    }
}

bool StepSequencerAudioProcessor::isNoteInScale(int midiNote, int rootNote, int scaleType)
{
    // Scale intervals (semitones from root)
    static const int chromatic[] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static const int major[] = {0,2,4,5,7,9,11};
    static const int minor[] = {0,2,3,5,7,8,10};
    static const int dorian[] = {0,2,3,5,7,9,10};
    static const int phrygian[] = {0,1,3,5,7,8,10};
    static const int mixolydian[] = {0,2,4,5,7,9,10};
    static const int pentatonic[] = {0,2,4,7,9};
    
    const int* intervals = nullptr;
    int count = 0;
    
    switch(scaleType) {
        case 0: intervals = chromatic; count = 12; break;
        case 1: intervals = major; count = 7; break;
        case 2: intervals = minor; count = 7; break;
        case 3: intervals = dorian; count = 7; break;
        case 4: intervals = phrygian; count = 7; break;
        case 5: intervals = mixolydian; count = 7; break;
        case 6: intervals = pentatonic; count = 5; break;
        default: return true;
    }
    
    int noteClass = midiNote % 12;
    int offset = (noteClass - rootNote + 12) % 12;
    
    for (int i = 0; i < count; ++i) {
        if (intervals[i] == offset) return true;
    }
    return false;
}

int StepSequencerAudioProcessor::getRandomNoteInScale(int rootNote, int scaleType, int minOctave, int maxOctave)
{
    // Scale intervals (semitones from root)
    static const int chromatic[] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static const int major[] = {0,2,4,5,7,9,11};
    static const int minor[] = {0,2,3,5,7,8,10};
    static const int dorian[] = {0,2,3,5,7,9,10};
    static const int phrygian[] = {0,1,3,5,7,8,10};
    static const int mixolydian[] = {0,2,4,5,7,9,10};
    static const int pentatonic[] = {0,2,4,7,9};
    
    const int* intervals = nullptr;
    int count = 0;
    
    switch(scaleType) {
        case 0: intervals = chromatic; count = 12; break;
        case 1: intervals = major; count = 7; break;
        case 2: intervals = minor; count = 7; break;
        case 3: intervals = dorian; count = 7; break;
        case 4: intervals = phrygian; count = 7; break;
        case 5: intervals = mixolydian; count = 7; break;
        case 6: intervals = pentatonic; count = 5; break;
        default: intervals = chromatic; count = 12; break;
    }
    
    auto& random = juce::Random::getSystemRandom();
    
    // Pick random octave in range
    int octave = random.nextInt(juce::Range<int>(minOctave, maxOctave + 1));
    
    // Pick random scale degree
    int degree = random.nextInt(count);
    int interval = intervals[degree];
    
    // Calculate MIDI note: octave * 12 + root + interval
    int midiNote = (octave * 12) + rootNote + interval;
    
    // Clamp to valid MIDI range
    return juce::jlimit(0, 127, midiNote);
}

void StepSequencerAudioProcessor::addTrack()
{
    std::vector<Step> newTrack(32);
    for (auto& s : newTrack) {
        s.active = false;
        s.note = 60;
        s.velocity = 100;
        s.gate = 0.5f;
        s.prob = 1.0f;
    }
    tracks.push_back(newTrack);
    trackRepeat.push_back(1);
    trackEnabled.push_back(true);
    
    // Update steps pointer after reallocation
    if (currentTrack < (int)tracks.size()) {
        steps = &tracks[currentTrack];
    }
}

void StepSequencerAudioProcessor::removeTrack()
{
    if (tracks.size() > 1) {
        tracks.pop_back();
        trackRepeat.pop_back();
        trackEnabled.pop_back();
        
        // Make sure currentTrack is still valid
        if (currentTrack >= (int)tracks.size()) {
            currentTrack = (int)tracks.size() - 1;
        }
        
        // Update steps pointer after removal
        steps = &tracks[currentTrack];
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StepSequencerAudioProcessor();
}
#endif
