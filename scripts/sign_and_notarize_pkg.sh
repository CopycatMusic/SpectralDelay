#!/bin/bash
# CopyCat Spectral Delay - PKG Signing and Notarization Script
# Run this script in Terminal to sign and notarize the PKG

set -e

PROJECT_DIR="/Users/geng/Projects/CopyCatSpectralDelay"
RELEASE_DIR="$PROJECT_DIR/release/macOS/v1.0.0"
PKG_PATH="$RELEASE_DIR/CopyCatSpectralDelay-1.0.0-macOS.pkg"
AU_PATH="$RELEASE_DIR/CopyCat Spectral Delay.component"
VST3_PATH="$RELEASE_DIR/CopyCat Spectral Delay.vst3"

# IMPORTANT: PKG signing requires a "Developer ID Installer" certificate
# Bundle signing requires a "Developer ID Application" certificate
IDENTITY_INSTALLER="Developer ID Installer: Geng TIAN (WY964PPVM4)"
IDENTITY_APP="Developer ID Application: Geng TIAN (WY964PPVM4)"
PROFILE="AC_NOTARYTOOL"

# Check if the installer identity exists
if ! security find-identity -v | grep -q "$IDENTITY_INSTALLER"; then
    echo "❌ Error: Could not find installer certificate: $IDENTITY_INSTALLER"
    exit 1
fi

if ! security find-identity -v | grep -q "$IDENTITY_APP"; then
    echo "❌ Error: Could not find application certificate: $IDENTITY_APP"
    exit 1
fi

echo "🔍 Checking architectures of binaries..."
lipo -info "$AU_PATH/Contents/MacOS/CopyCat Spectral Delay"
lipo -info "$VST3_PATH/Contents/MacOS/CopyCat Spectral Delay"

echo "🔐 Signing individual bundles..."
codesign --force --options runtime --timestamp --sign "$IDENTITY_APP" "$AU_PATH"
codesign --force --options runtime --timestamp --sign "$IDENTITY_APP" "$VST3_PATH"
echo "✅ Bundles signed successfully!"

echo "📦 Re-building PKG with signed bundles..."
./scripts/create_pkg.sh

echo "🔐 Signing PKG..."
# For PKG, we sign the actual file using productsign
VERSION="1.0.0"
SIGNED_PKG_PATH="${PKG_PATH%.pkg}_signed.pkg"
productsign --sign "$IDENTITY_INSTALLER" "$PKG_PATH" "$SIGNED_PKG_PATH"

# Replace original with signed version
mv "$SIGNED_PKG_PATH" "$PKG_PATH"
echo "✅ PKG signed successfully!"

echo ""
echo "📤 Submitting for notarization (this may take a few minutes)..."
xcrun notarytool submit "$PKG_PATH" --keychain-profile "$PROFILE" --wait

echo ""
echo "📎 Stapling notarization ticket..."
xcrun stapler staple "$PKG_PATH"
xcrun stapler staple "$AU_PATH"
xcrun stapler staple "$VST3_PATH"

echo ""
echo "🎉 Done! Your PKG and bundles are ready for distribution."
