# Step Sequencer

A MIDI step sequencer plugin for Ableton Live.

## Features

- 16-step sequencer with adjustable length (1-16 steps)
- Multiple rate divisions: 1/4, 1/8, 1/16, 1/32, triplets
- Swing control (0-100%)
- Gate length control (1-100%)
- Visual step grid with playback indicator
- Click steps to toggle on/off
- Sync to DAW tempo

## Building

```bash
cd step-sequencer
mkdir build && cd build
cmake ..
make
```

## Usage in Ableton Live

1. Add the plugin to a MIDI track
2. Route the output to an instrument
3. Press play and the sequencer will generate MIDI notes
4. Click on steps in the grid to enable/disable them
5. Adjust parameters for different rhythms

## Controls

- **Steps**: Number of active steps (1-16)
- **Rate**: Note division for step timing
- **Swing**: Adds swing to odd-numbered steps
- **Gate**: Length of each note (percentage of step duration)

## Todo

- [ ] Per-step note editing
- [ ] Per-step velocity editing
- [ ] Per-step probability
- [ ] Pattern save/load
- [ ] Multiple patterns
- [ ] Randomization
