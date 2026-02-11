#!/bin/bash
set -e

echo "🚀 Building Step Sequencer..."

cd "$(dirname "$0")"

# Create build directory
mkdir -p build
cd build

# Configure
cmake ..

# Build
make -j8

echo "✅ Build complete!"
echo ""
echo "Plugin locations:"
echo "  VST3: build/StepSequencer_artefacts/VST3/"
echo "  AU:   build/StepSequencer_artefacts/AU/"
echo ""
echo "To install locally for testing:"
echo "  cp -r build/StepSequencer_artefacts/VST3/Step\ Sequencer.vst3 ~/Library/Audio/Plug-Ins/VST3/"
echo "  cp -r build/StepSequencer_artefacts/AU/Step\ Sequencer.component ~/Library/Audio/Plug-Ins/Components/"
