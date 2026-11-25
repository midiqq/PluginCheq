#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui::Theme {

// Apple-inspired Minimalist Dark + Light-Gray Palette
inline const juce::Colour backgroundDark   {0xff0d0e11}; // Deep obsidian canvas
inline const juce::Colour panelBg          {0xff14161b}; // Elevated card surface
inline const juce::Colour barBg            {0xff181b21}; // Top control bar
inline const juce::Colour controlSurface   {0xff22262f}; // Pill / button / combo fill
inline const juce::Colour controlHover     {0xff2c313c}; // Subtle hover state
inline const juce::Colour controlActive    {0xffd1d5db}; // Light-gray active pill fill
inline const juce::Colour controlActiveTxt {0xff0d0e11}; // High-contrast dark text on active pill

inline const juce::Colour borderSubtle     {0xff282c36}; // Hairline card border
inline const juce::Colour borderLightGray  {0xff3a404d}; // Light-gray control outline
inline const juce::Colour gridMajor        {0xff262a34};
inline const juce::Colour gridMinor        {0xff1b1e26};
inline const juce::Colour zeroLine         {0xff4b5263}; // 0 dB / 0 deg reference

inline const juce::Colour textPrimary      {0xfff2f4f8}; // Crisp off-white
inline const juce::Colour textSecondary    {0xff9ca3af}; // Apple-style light gray
inline const juce::Colour textMuted        {0xff6b7280};

// Scientific trace colors (refined saturation for dark canvas)
inline const juce::Colour traceMagnitude   {0xff5ac8fa}; // Sky Cyan
inline const juce::Colour traceH2          {0xffffd60a}; // Amber Yellow
inline const juce::Colour tracePhase       {0xffbf5af2}; // Soft Violet
inline const juce::Colour traceCoherence   {0xff30d158}; // Emerald Green
inline const juce::Colour traceSnapshotA   {0xffd1d5db}; // Light-gray Snapshot A
inline const juce::Colour traceSnapshotB   {0xfffb923c}; // Soft Coral Snapshot B

// Light-gray minimalist metering
inline const juce::Colour meterRef         {0xff9ca3af}; // Light gray
inline const juce::Colour meterMeas        {0xffe5e7eb}; // Brighter light gray
inline const juce::Colour meterClip        {0xffff453a}; // Apple Red

} // namespace ui::Theme
