#!/bin/bash

# Usage: ./generate_audio.sh [VOICE_NAME] [OUTPUT_FILENAME]
# Default voice: Alex
# Default output: introduction_alex.m4a
# Script content: script.txt (must exist)

VOICE=${1:-Victoria}
OUTPUT=${2:-introduction.m4a}
SCRIPT="script.txt"

if [ ! -f "$SCRIPT" ]; then
    echo "Error: $SCRIPT not found."
    exit 1
fi

echo "Generating audio using voice: $VOICE"
echo "Output file: $OUTPUT"

say -v "$VOICE" -f "$SCRIPT" -o "$OUTPUT"

if [ $? -eq 0 ]; then
    echo "Success! Audio generated: $OUTPUT"
else
    echo "Error: Failed to generate audio."
    exit 1
fi
