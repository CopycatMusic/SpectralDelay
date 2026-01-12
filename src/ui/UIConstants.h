#pragma once

/**
 * Centralized UI constants for the CopyCat Spectral Delay plugin.
 * All UI-related dimensions, sizes, and fonts should be defined here.
 */
namespace UIConstants {

// ===== Window Dimensions =====
constexpr int kDefaultWindowWidth = 950;
constexpr int kDefaultWindowHeight = 450;
constexpr int kMinWindowWidth = 700;
constexpr int kMinWindowHeight = 450;
constexpr int kMaxWindowWidth = 1600;
constexpr int kMaxWindowHeight = 1000;

// ===== Layout =====
constexpr int kControlPanelWidth = 290;
constexpr int kTitleBarHeight = 40;
constexpr int kTitleAreaWidth = 600;

// ===== Margins and Spacing =====
constexpr int kMargin = 10;
constexpr int kSpacing = 8;
constexpr int kRowHeight = 25;
constexpr int kLabelHeight = 15;
constexpr int kTextBoxHeight = 15;

// ===== Knob Sizes (square) =====
constexpr int kLargeKnobSize = 90;  // Multiply knob
constexpr int kMediumKnobSize = 65; // LFO Speed/Amount/Smooth
constexpr int kSmallKnobSize = 55;  // FFT, Mix, Stereo, LowCut, Grid

// ===== Font Sizes =====
constexpr float kTitleFontSize = 32.0f;   // Brand and title labels
constexpr float kDefaultFontSize = 14.0f; // Buttons, knob text, general UI

} // namespace UIConstants
