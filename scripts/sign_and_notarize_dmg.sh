#!/bin/bash
# CopyCat Spectral Delay - DMG Signing and Notarization Script
# Run this script in Terminal to sign and notarize the DMG

set -e

DMG_PATH="/Users/geng/Projects/CopyCatSpectralDelay/release/v1.0.0/CopyCatSpectralDelay-1.0.0-macOS.dmg"
IDENTITY="Developer ID Application: Geng TIAN (WY964PPVM4)"
PROFILE="AC_NOTARYTOOL"

echo "🔐 Signing DMG..."
codesign --force --sign "$IDENTITY" --timestamp "$DMG_PATH"
echo "✅ DMG signed successfully!"

echo ""
echo "📤 Submitting for notarization (this may take a few minutes)..."
xcrun notarytool submit "$DMG_PATH" --keychain-profile "$PROFILE" --wait

echo ""
echo "📎 Stapling notarization ticket..."
xcrun stapler staple "$DMG_PATH"

echo ""
echo "🎉 Done! Your DMG is ready for distribution:"
echo "   $DMG_PATH"
