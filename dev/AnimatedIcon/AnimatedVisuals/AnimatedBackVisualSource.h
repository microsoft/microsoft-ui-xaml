﻿#pragma once
//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//       LottieGen version:
//           7.0.0-build.101+g12769c43d3
//       
//       Command:
//           LottieGen -Language Cppwinrt -WinUIVersion 2.4 -InputFile AnimatedBackVisualSource.json
//       
//       Input file:
//           AnimatedBackVisualSource.json (19124 bytes created 12:28-08:00 Feb 8 2021)
//       
//       LottieGen source:
//           http://aka.ms/Lottie
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------
#include "common.h"
#include "AnimatedVisuals\AnimatedBackVisualSource.g.h"

// Name:        AnimatedBackVisualSource
// Frame rate:  60 fps
// Frame count: 50
// Duration:    833.3 mS
// ____________________________________________________________________________________________
// |           Marker           |           Constant           | Frame |  mS   |   Progress   |
// |____________________________|______________________________|_______|_______|______________|
// | NormalToPointerOver_Start  | M_NormalToPointerOver_Start  |     0 |   0.0 | 0.0F         |
// | NormalToPointerOver_End    | M_NormalToPointerOver_End    |     9 | 150.0 | 0.180999994F |
// | NormalToPressed_Start      | M_NormalToPressed_Start      |    10 | 166.7 | 0.201000005F |
// | NormalToPressed_End        | M_NormalToPressed_End        |    14 | 233.3 | 0.280999988F |
// | PointerOverToNormal_Start  | M_PointerOverToNormal_Start  |    15 | 250.0 | 0.300999999F |
// | PointerOverToNormal_End    | M_PointerOverToNormal_End    |    24 | 400.0 | 0.481000006F |
// | PointerOverToPressed_Start | M_PointerOverToPressed_Start |    25 | 416.7 | 0.500999987F |
// | PointerOverToPressed_End   | M_PointerOverToPressed_End   |    29 | 483.3 | 0.58099997F  |
// | PressedToNormal_Start      | M_PressedToNormal_Start      |    30 | 500.0 | 0.601000011F |
// | PressedToNormal_End        | M_PressedToNormal_End        |    44 | 733.3 | 0.880999982F |
// | PressedToPointerOver_Start | M_PressedToPointerOver_Start |    45 | 750.0 | 0.901000023F |
// | PressedToPointerOver_End   | M_PressedToPointerOver_End   |    49 | 816.7 | 0.981000006F |
// --------------------------------------------------------------------------------------------
// _______________________________________________________
// | Theme property |  Accessor  | Type  | Default value |
// |________________|____________|_______|_______________|
// | Foreground     | Foreground | Color |   #FF060808   |
// -------------------------------------------------------
class AnimatedBackVisualSource
    : public winrt::implementation::AnimatedBackVisualSourceT<AnimatedBackVisualSource>
{
    winrt::Windows::UI::Composition::CompositionPropertySet _themeProperties{ nullptr };
    winrt::Windows::UI::Color _themeForeground{ 0xFF, 0x06, 0x08, 0x08 };
    winrt::Windows::UI::Composition::CompositionPropertySet EnsureThemeProperties(winrt::Windows::UI::Composition::Compositor compositor);

    static winrt::Windows::Foundation::Numerics::float4 ColorAsVector4(winrt::Windows::UI::Color color);
public:
    // Animation duration: 0.833 seconds.
    static constexpr int64_t c_durationTicks{ 8333333L };

    // Marker: NormalToPointerOver_Start.
    static constexpr float M_NormalToPointerOver_Start{ 0.0F };

    // Marker: NormalToPointerOver_End.
    static constexpr float M_NormalToPointerOver_End{ 0.180999994F };

    // Marker: NormalToPressed_Start.
    static constexpr float M_NormalToPressed_Start{ 0.201000005F };

    // Marker: NormalToPressed_End.
    static constexpr float M_NormalToPressed_End{ 0.280999988F };

    // Marker: PointerOverToNormal_Start.
    static constexpr float M_PointerOverToNormal_Start{ 0.300999999F };

    // Marker: PointerOverToNormal_End.
    static constexpr float M_PointerOverToNormal_End{ 0.481000006F };

    // Marker: PointerOverToPressed_Start.
    static constexpr float M_PointerOverToPressed_Start{ 0.500999987F };

    // Marker: PointerOverToPressed_End.
    static constexpr float M_PointerOverToPressed_End{ 0.58099997F };

    // Marker: PressedToNormal_Start.
    static constexpr float M_PressedToNormal_Start{ 0.601000011F };

    // Marker: PressedToNormal_End.
    static constexpr float M_PressedToNormal_End{ 0.880999982F };

    // Marker: PressedToPointerOver_Start.
    static constexpr float M_PressedToPointerOver_Start{ 0.901000023F };

    // Marker: PressedToPointerOver_End.
    static constexpr float M_PressedToPointerOver_End{ 0.981000006F };

    // Theme property: Foreground.
    static inline const winrt::Windows::UI::Color c_themeForeground{ 0xFF, 0x06, 0x08, 0x08 };


    winrt::Windows::UI::Color Foreground();
    void Foreground(winrt::Windows::UI::Color value);

    winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual TryCreateAnimatedVisual(
        winrt::Windows::UI::Composition::Compositor const& compositor);

    winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual TryCreateAnimatedVisual(
        winrt::Windows::UI::Composition::Compositor const& compositor,
        winrt::Windows::Foundation::IInspectable& diagnostics);

    // Gets the number of frames in the animation.
    double FrameCount();

    // Gets the framerate of the animation.
    double Framerate();

    // Gets the duration of the animation.
    winrt::Windows::Foundation::TimeSpan Duration();

    // Converts a zero-based frame number to the corresponding progress value denoting the
    // start of the frame.
    double FrameToProgress(double frameNumber);

    // Returns a map from marker names to corresponding progress values.
    winrt::Windows::Foundation::Collections::IMapView<hstring, double> Markers();

    // Sets the color property with the given name, or does nothing if no such property
    // exists.
    void SetColorProperty(hstring const& propertyName, winrt::Windows::UI::Color value);

    // Sets the scalar property with the given name, or does nothing if no such property
    // exists.
    void SetScalarProperty(hstring const& propertyName, double value);
};
