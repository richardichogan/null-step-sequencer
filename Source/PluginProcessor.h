/*
  ==============================================================================
    PluginProcessor.h
    Step Sequencer - MIDI Step Sequencer for Ableton Live
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class StepSequencerAudioProcessor : public juce::AudioProcessor
{
public:
    StepSequencerAudioProcessor();
    ~StepSequencerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Buses
    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    #endif

    // APVTS
    juce::AudioProcessorValueTreeState apvts;
    
    // Data structures for UI
    struct Step { 
        bool active = true; 
        bool isTied = false;
        int note = 60; 
        int velocity = 100; 
        float gate = 0.5f; // 0.1 to ~0.9
        float prob = 1.0f; 
    };
    
    // Multi-track system (dynamic)
    std::vector<std::vector<Step>> tracks; // Dynamic list of tracks
    std::vector<int> trackRepeat; // How many times to repeat each track before moving to next
    std::vector<bool> trackEnabled; // Which tracks are active
    int currentTrack = 0;
    std::vector<Step>* steps = nullptr; // Pointer to current track for easy switching
    int currentStepIndex = 0;
    bool isPlaying = false;
    
    // Helper to add/remove tracks
    void addTrack();
    void removeTrack();
    int getNumTracks() const { return (int)tracks.size(); }
    
    // Time Signature from Host
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    
    // Track switching
    int barsPlayedOnCurrentTrack = 0;
    int beatsPlayedInCurrentBar = 0;
    
    // Generative Functions
    void randomizePattern(float amount = 1.0f); // amount: 0.0 to 1.0
    void mutatePattern(float amount = 0.2f);    // amount: 0.0 to 1.0
    void clearPattern();                        // Reset all steps to default
    void invertPattern();                       // Flip active/inactive states
    void reversePattern();                      // Reverse step order
    void euclideanPattern(int hits, int steps); // Euclidean rhythm generator
    void switchToTrack(int trackIndex);
    
    // Helper Functions
    bool isNoteInScale(int midiNote, int rootNote, int scaleType);
    int getRandomNoteInScale(int rootNote, int scaleType, int minOctave, int maxOctave);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    
    // Timing state
    double sampleRate = 44100.0;
    double currentBPM = 120.0;
    double samplesPerBeat = 22050.0;
    
    // Phasor / Accumulator concept (simplified for reliability)
    double accumulatedSamples = 0.0;
    
    // Note State
    int lastNote = -1;
    long samplesRemainingForGate = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencerAudioProcessor)
};
