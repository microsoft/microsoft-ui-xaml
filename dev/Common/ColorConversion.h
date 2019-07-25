// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CppWinRTHelpers.h"
#include "DispatcherHelper.h"

// Helper classes used for converting between RGB, HSV, and hex.
class Rgb
{
public:
    double r{};
    double g{};
    double b{};
    Rgb() = default;
    Rgb(double r, double g, double b);
};

struct Rgba
{
    Rgb rgb{};
    double a{};
};

class Hsv
{
public:
    double h{};
    double s{};
    double v{};
    Hsv() = default;
    Hsv(double h, double s, double v);
};

std::optional<unsigned long> TryParseInt(const wstring_view& s);
std::optional<unsigned long> TryParseInt(const PCWSTR& str, int base);

Hsv RgbToHsv(const Rgb &rgb);
Rgb HsvToRgb(const Hsv &hsv);

Rgb HexToRgb(const wstring_view& input);
winrt::hstring RgbToHex(const Rgb &rgb);

Rgba HexToRgba(const wstring_view& input);
winrt::hstring RgbaToHex(const Rgb &rgb, double alpha);

winrt::Color ColorFromRgba(const Rgb &rgb, double alpha = 1.0);
Rgb RgbFromColor(const winrt::Color &color);

// We represent HSV and alpha using a Vector4 (float4 in C++/WinRT).
// We'll use the following helper methods to convert between the four dimensions and HSVA.
namespace hsv
{
    inline float GetHue(const winrt::float4 &hsva) { return hsva.x; }
    inline void SetHue(winrt::float4 &hsva, float hue) { hsva.x = hue; }
    inline float GetSaturation(const winrt::float4 &hsva) { return hsva.y; }
    inline void SetSaturation(winrt::float4 &hsva, float saturation) { hsva.y = saturation; }
    inline float GetValue(const winrt::float4 &hsva) { return hsva.z; }
    inline void SetValue(winrt::float4 &hsva, float value) { hsva.z = value; }
    inline float GetAlpha(const winrt::float4 &hsva) { return hsva.w; }
    inline void SetAlpha(winrt::float4 &hsva, float alpha) { hsva.w = alpha; }
}
