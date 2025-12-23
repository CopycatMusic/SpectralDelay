# Preset Browser Feature Specification

## Overview
Add a built-in preset browser to the plugin for saving/loading parameter states.

## UI Design

Location: **Top center of plugin window, in the title bar area**

```
[SPECTRAL DELAY]  [◀] [Shimmer ▼] [▶] [⟲] [💾]  [by Author]
                   │      │       │    │    │
                   │      │       │    │    └── Save button
                   │      │       │    └─────── Init button (reset to defaults)
                   │      │       └──────────── Next preset
                   │      └──────────────────── Preset dropdown (with categories)
                   └─────────────────────────── Previous preset
```

## Preset File Location

| Platform | Path |
|----------|------|
| macOS | `~/Documents/CopyCatSpectralDelay/Presets/` |
| Windows | `%USERPROFILE%\Documents\CopyCatSpectralDelay\Presets\` |

### Directory Structure
```
Presets/
├── Factory/          # Read-only factory presets
│   ├── Metallic.json
│   └── Shimmer.json
└── User/             # User-created presets
    └── My Preset.json
```

### JUCE API for Cross-Platform Path
```cpp
juce::File presetsFolder = juce::File::getSpecialLocation(
    juce::File::userDocumentsDirectory)
    .getChildFile("CopyCatSpectralDelay")
    .getChildFile("Presets");
```

## Preset File Format (JSON)

```json
{
  "name": "Shimmer",
  "version": 1,
  "parameters": {
    "fftSize": 2048,
    "overlap": 4,
    "mix": 50,
    "delayMode": false
  },
  "curves": {
    "gain": "20:100;1000:150;10000:80",
    "phase": "20:0;20000:180",
    "feedback": "",
    "delay": ""
  }
}
```

## Features

### 1. Preset Selector (Dropdown)
- Lists all presets from Factory and User folders
- Shows category headers (Factory / User)
- Selecting a preset loads it immediately

### 2. Previous/Next Buttons (◀ ▶)
- Navigate through presets in order
- Wrap around at ends

### 3. Init Button (⟲)
- Reset all parameters to default values:
  - FFT Size: 2048
  - Overlap: 4x
  - Mix: 50%
  - Delay Mode: Phase
- Clear all curve points (flat curves)

### 4. Save Button (💾)
- Opens a dialog to enter preset name
- Saves to User folder as JSON
- Overwrites if name already exists (with confirmation)

## Implementation Files to Create/Modify

### New Files
- `src/ui/PresetBrowser.h` - PresetBrowser component class
- `src/ui/PresetBrowser.cpp` - Implementation
- `src/utils/PresetManager.h` - Preset loading/saving logic
- `src/utils/PresetManager.cpp` - Implementation

### Files to Modify
- `src/PluginEditor.cpp` - Add PresetBrowser to UI layout
- `src/PluginEditor.h` - Add PresetBrowser member
- `CMakeLists.txt` - Add new source files

## Notes
- Factory presets should be embedded in the plugin or copied on first run
- Create Presets directory if it doesn't exist on first save
- Handle edge cases: missing files, corrupted JSON, etc.
