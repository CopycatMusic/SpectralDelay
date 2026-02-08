#!/bin/bash
# CopyCat Spectral Delay - PKG Signing and Notarization Script
# Run this script in Terminal to sign and notarize the PKG
#
# Usage: ./sign_and_notarize_pkg.sh [--version X.X.X] [--variant standard|lite]

set -e

# ===========================================================================
# Parse Arguments
# ===========================================================================
VERSION=""
VARIANT="standard"

while [[ $# -gt 0 ]]; do
    case $1 in
        --version)
            VERSION="$2"
            shift 2
            ;;
        --variant)
            VARIANT="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# ===========================================================================
# Auto-detect project root and version
# ===========================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Auto-extract version from CMakeLists.txt if not provided
if [ -z "$VERSION" ]; then
    VERSION=$(grep -oE 'project\(CopyCatSpectralDelay VERSION ([0-9.]+)' "$PROJECT_DIR/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    if [ -z "$VERSION" ]; then
        echo "❌ Error: Could not extract version from CMakeLists.txt"
        exit 1
    fi
    echo "📌 Auto-detected version: $VERSION"
fi

# ===========================================================================
# Set variant-specific variables
# ===========================================================================
if [ "$VARIANT" = "lite" ]; then
    PLUGIN_NAME="CopyCat Spectral Delay Lite"
    PKG_SUFFIX="Lite"
else
    PLUGIN_NAME="CopyCat Spectral Delay"
    PKG_SUFFIX="Standard"
fi

# ===========================================================================
# Set paths
# ===========================================================================
RELEASE_DIR="$PROJECT_DIR/release/v$VERSION/macOS/${PKG_SUFFIX}"
PKG_PATH="$RELEASE_DIR/CopyCatSpectralDelay-${VERSION}-macOS-${PKG_SUFFIX}.pkg"
AU_PATH="$RELEASE_DIR/${PLUGIN_NAME}.component"
VST3_PATH="$RELEASE_DIR/${PLUGIN_NAME}.vst3"

# Signing identities (IMPORTANT: PKG requires "Developer ID Installer", bundles require "Developer ID Application")
IDENTITY_INSTALLER="Developer ID Installer: Geng TIAN (WY964PPVM4)"
IDENTITY_APP="Developer ID Application: Geng TIAN (WY964PPVM4)"
PROFILE="AC_NOTARYTOOL"

echo "=================================================="
echo "🔐 Signing: $PLUGIN_NAME v$VERSION"
echo "=================================================="

# ===========================================================================
# Validate certificates exist
# ===========================================================================
if ! security find-identity -v | grep -q "$IDENTITY_INSTALLER"; then
    echo "❌ Error: Could not find installer certificate: $IDENTITY_INSTALLER"
    exit 1
fi

if ! security find-identity -v | grep -q "$IDENTITY_APP"; then
    echo "❌ Error: Could not find application certificate: $IDENTITY_APP"
    exit 1
fi

# ===========================================================================
# Check input files exist
# ===========================================================================
if [ ! -d "$AU_PATH" ]; then
    echo "❌ Error: AU component not found: $AU_PATH"
    echo "   Make sure to place artifacts in: $RELEASE_DIR"
    exit 1
fi

if [ ! -d "$VST3_PATH" ]; then
    echo "❌ Error: VST3 bundle not found: $VST3_PATH"
    echo "   Make sure to place artifacts in: $RELEASE_DIR"
    exit 1
fi

echo "🔍 Checking architectures of binaries..."
lipo -info "$AU_PATH/Contents/MacOS/$PLUGIN_NAME"
lipo -info "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME"

echo "🔐 Signing individual bundles..."
codesign --force --options runtime --timestamp --sign "$IDENTITY_APP" "$AU_PATH"
codesign --force --options runtime --timestamp --sign "$IDENTITY_APP" "$VST3_PATH"
echo "✅ Bundles signed successfully!"

echo "📦 Building PKG with signed bundles..."
"$SCRIPT_DIR/create_pkg.sh" --version "$VERSION" --variant "$VARIANT"

echo "🔐 Signing PKG..."
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
echo "🎉 Done! Your PKG and bundles are ready for distribution:"
echo "   PKG:  $PKG_PATH"
echo "   AU:   $AU_PATH"
echo "   VST3: $VST3_PATH"
