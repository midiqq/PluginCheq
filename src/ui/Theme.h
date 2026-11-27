#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui::Theme {

// ==============================================================================
// Apple Pro Dark Minimalist Palette (Precision Slate / Obsidian / Graphite)
// ==============================================================================

// Deep obsidian canvas & layered panel surfaces
inline const juce::Colour backgroundDark   {0xff0a0c10}; // Ultra-deep obsidian background
inline const juce::Colour panelBg          {0xff12151d}; // Elevated card background
inline const juce::Colour panelBgSubtle    {0xff0e1017}; // Recessed plot inner canvas
inline const juce::Colour barBg            {0xff151821}; // Top control bar / header
inline const juce::Colour drawerBg         {0xff10131a}; // Advanced settings drawer background

// Pill / control surfaces & interactive states
inline const juce::Colour controlSurface   {0xff1b1f2a}; // Default pill / button / combo fill
inline const juce::Colour controlHover     {0xff262c3b}; // Subtle hover state
inline const juce::Colour controlActive    {0xffe2e8f0}; // Apple light-slate active pill fill
inline const juce::Colour controlActiveTxt {0xff0a0c10}; // High-contrast dark obsidian text on active pill
inline const juce::Colour segmentedTrack   {0xff101219}; // Recessed track for segmented tab buttons

// Precision hairline borders and grid lines (Apple 1px micro-borders)
inline const juce::Colour borderSubtle     {0xff222735}; // Hairline card / separator border
inline const juce::Colour borderLightGray  {0xff323a4d}; // Control perimeter outline
inline const juce::Colour borderActive     {0xff4f5b75}; // Focused / highlighted border
inline const juce::Colour gridMajor        {0xff202534}; // Major grid lines
inline const juce::Colour gridMinor        {0xff161924}; // Minor grid lines
inline const juce::Colour zeroLine         {0xff3a4358}; // 0 dB / 0 deg / Unity reference line

// Typography (Apple-grade micro-typography)
inline const juce::Colour textPrimary      {0xfff8fafc}; // Crisp off-white primary text
inline const juce::Colour textSecondary    {0xff94a3b8}; // Apple cool slate gray for labels
inline const juce::Colour textMuted        {0xff64748b}; // Subdued gray for inactive items
inline const juce::Colour textDim          {0xff475569}; // Faintest labels / units

// Scientific measurement trace colors (Tuned saturation for dark canvas)
inline const juce::Colour traceMagnitude   {0xff38bdf8}; // Sky Cyan (Primary Transfer & Spectrum)
inline const juce::Colour traceH2          {0xfffbbf24}; // Amber Gold (H2 / Threshold & Ratio)
inline const juce::Colour tracePhase       {0xffc084fc}; // Soft Lavender / Violet (Phase)
inline const juce::Colour traceCoherence   {0xff34d399}; // Emerald Mint (Coherence / Safe status)
inline const juce::Colour traceSnapshotA   {0xffcbd5e1}; // Light Slate (Snapshot A reference)
inline const juce::Colour traceSnapshotB   {0xfffb923c}; // Tangerine Coral (Snapshot B reference)

// Harmonic markers
inline const juce::Colour harmonicH1       {0xff38bdf8}; // Sky Cyan (Fundamental)
inline const juce::Colour harmonicH2       {0xfffbbf24}; // Amber Gold
inline const juce::Colour harmonicH3       {0xfff87171}; // Soft Coral / Red
inline const juce::Colour harmonicHn       {0xffe879f9}; // Orchid Pink (H4..H8)

// Laboratory telemetry & metering colors
inline const juce::Colour meterRef         {0xff64748b}; // Subdued slate for Reference
inline const juce::Colour meterMeas        {0xff38bdf8}; // Sky Cyan for Measurement
inline const juce::Colour meterClip        {0xffef4444}; // Apple Red (Clip indicator)
inline const juce::Colour meterWarn        {0xfff59e0b}; // Amber warning
inline const juce::Colour statusOk         {0xff34d399}; // Mint (Delay locked / Sidechain OK)
inline const juce::Colour statusSearching  {0xfffbbf24}; // Amber (Searching / Calibrating)
inline const juce::Colour statusInactive   {0xff64748b}; // Slate (Disabled / Idle)

// Floating HUD / Badge card surfaces
inline const juce::Colour badgeBg          {0xff161a24}; // Badge pill background
inline const juce::Colour badgeBorder      {0xff282f42}; // Badge perimeter

} // namespace ui::Theme
