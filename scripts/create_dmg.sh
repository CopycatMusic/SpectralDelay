#!/bin/bash
# CopyCat Spectral Delay - Simple DMG Creator
# Creates a clean DMG without background image, with clear installation guidance

set -e

PROJECT_DIR="/Users/geng/Projects/CopyCatSpectralDelay"
RELEASE_DIR="$PROJECT_DIR/release/v1.0.0"
DMG_SOURCE="$RELEASE_DIR/dmg_source"
DMG_OUTPUT="$RELEASE_DIR/CopyCatSpectralDelay-1.0.0-macOS.dmg"
VOLNAME="CopyCat Spectral Delay v1.0.0 Installer"

echo "🧹 Cleaning up previous build..."
rm -rf "$DMG_SOURCE"
rm -f "$DMG_OUTPUT"

echo "📁 Preparing DMG source folder..."
mkdir -p "$DMG_SOURCE"

# Copy signed plugins with clear names
cp -R "$RELEASE_DIR/macOS/CopyCat Spectral Delay.vst3" "$DMG_SOURCE/"
cp -R "$RELEASE_DIR/macOS/CopyCat Spectral Delay.component" "$DMG_SOURCE/"

# Create symlinks with matching names for clarity
# VST3 -> VST3 folder, Component -> Components folder
ln -s "/Library/Audio/Plug-Ins/VST3" "$DMG_SOURCE/→ Install VST3 Here"
ln -s "/Library/Audio/Plug-Ins/Components" "$DMG_SOURCE/→ Install AU Here"

echo "💿 Creating DMG with clear layout..."
# Using create-dmg for simple layout
# Left side: plugins, Right side: installation folders
create-dmg \
  --volname "$VOLNAME" \
  --window-pos 200 120 \
  --window-size 600 480 \
  --icon-size 72 \
  --text-size 12 \
  --icon "CopyCat Spectral Delay.vst3" 140 140 \
  --icon "→ Install VST3 Here" 460 140 \
  --icon "CopyCat Spectral Delay.component" 140 300 \
  --icon "→ Install AU Here" 460 300 \
  --hide-extension "CopyCat Spectral Delay.vst3" \
  --hide-extension "CopyCat Spectral Delay.component" \
  --no-internet-enable \
  "$DMG_OUTPUT" \
  "$DMG_SOURCE"

# Clean up source folder
rm -rf "$DMG_SOURCE"

echo ""
echo "✅ DMG created: $DMG_OUTPUT"
echo ""
echo "📋 Layout:"
echo "   Left side:  Plugin files"
echo "   Right side: Installation folders (drag plugins here)"
echo ""
echo "📝 Next: Run ./scripts/sign_and_notarize_dmg.sh to sign and notarize"
