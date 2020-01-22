#pragma once

class ProgressRingLoading :
    public winrt::implements<ProgressRingLoading, winrt::IAnimatedVisualSource>
{
private:
    // This file was auto-generated then modified to update animation visuals with inherited properties below
    float m_strokeThickness{ 4 };
    winrt::Size m_size{ 40, 40 };
    winrt::Color m_foreground{ winrt::ColorHelper::FromArgb(0xFF, 0x00, 0x78, 0xD7) };
    winrt::Color m_background{ winrt::ColorHelper::FromArgb(0xFF, 0xD3, 0xD3, 0xD3) };

public:
    ProgressRingLoading(float strokeThickness, winrt::Size size, winrt::Color foreground, winrt::Color background);

    winrt::IAnimatedVisual TryCreateAnimatedVisual(
        const winrt::Compositor& compositor,
        winrt::IInspectable& diagnostics);
};
