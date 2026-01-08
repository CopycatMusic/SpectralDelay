#!/bin/bash
# CopyCat Spectral Delay - PKG Installer Creator
# This script bundles VST3 and AU plugins into a single macOS .pkg installer.

set -e

PROJECT_DIR="/Users/geng/Projects/CopyCatSpectralDelay"
RELEASE_DIR="$PROJECT_DIR/release/macOS/v1.0.0"
BUILD_DIR="$RELEASE_DIR/pkg_build_temp"
PKG_OUTPUT="$RELEASE_DIR/CopyCatSpectralDelay-1.0.0-macOS.pkg"

# Bundle IDs
BUNDLE_ID="com.copycat.pkg.SpectralDelay"
VST3_ID="com.copycat.vst3.SpectralDelay"
AU_ID="com.copycat.au.SpectralDelay"

echo "🧹 Cleaning up previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "📦 Building VST3 component package..."
pkgbuild --component "$RELEASE_DIR/CopyCat Spectral Delay.vst3" \
         --install-location "/Library/Audio/Plug-Ins/VST3" \
         --identifier "$VST3_ID" \
         --version "1.0.0" \
         "$BUILD_DIR/vst3.pkg"

echo "📦 Building AU component package..."
pkgbuild --component "$RELEASE_DIR/CopyCat Spectral Delay.component" \
         --install-location "/Library/Audio/Plug-Ins/Components" \
         --identifier "$AU_ID" \
         --version "1.0.0" \
         "$BUILD_DIR/au.pkg"

echo "🏗️ Synthesizing distribution package..."
# 1. Generate a temporary distribution.xml
DIST_XML="$BUILD_DIR/distribution.xml"
productbuild --synthesize \
             --package "$BUILD_DIR/vst3.pkg" \
             --package "$BUILD_DIR/au.pkg" \
             "$DIST_XML"

# 2. Inject the title into the distribution.xml (using sed)
# We add it right after the <installer-gui-script> tag
TITLE="CopyCat Spectral Delay v1.0.0"
sed -i '' "s|<installer-gui-script[^>]*>|&\n    <title>$TITLE</title>|" "$DIST_XML"

# 3. Build the final package using the distribution file
productbuild --distribution "$DIST_XML" \
             --package-path "$BUILD_DIR" \
             "$PKG_OUTPUT"

echo ""
echo "✅ PKG created: $PKG_OUTPUT"
echo "⚠️ Note: This package is NOT yet signed or notarized."
echo "📝 Next: Run ./scripts/sign_and_notarize_pkg.sh to sign and notarize."

# Clean up build temp files
# rm -rf "$BUILD_DIR"
