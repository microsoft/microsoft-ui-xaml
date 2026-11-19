// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <ImmediatePainter.h>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

// Helpers shared by the data-driven painter conformance suites (ImmediatePainterTests.cpp and
// ImmediatePainterExtendedTests.cpp). The "Mode" test data selects the backend.
namespace RenderingBackendTests::PainterTestHelpers
{
    using namespace RenderingBackend;

    constexpr uint32_t c_size = 64;
    constexpr ColorF c_white{ 1, 1, 1, 1 };
    constexpr ColorF c_black{ 0, 0, 0, 1 };
    constexpr ColorF c_red{ 1, 0, 0, 1 };
    constexpr ColorF c_blue{ 0, 0, 1, 1 };

    struct Pixel
    {
        uint8_t b, g, r, a;
    };

    struct Image
    {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> bytes;

        Pixel At(uint32_t x, uint32_t y) const
        {
            const uint8_t* p = &bytes[(static_cast<size_t>(y) * width + x) * 4];
            return { p[0], p[1], p[2], p[3] };
        }
    };

    inline bool Near(uint8_t actual, int expected, int tolerance)
    {
        return std::abs(static_cast<int>(actual) - expected) <= tolerance;
    }

    inline bool IsColor(const Pixel& p, const ColorF& c, int tolerance = 3)
    {
        return Near(p.r, static_cast<int>(std::lround(c.r * c.a * 255)), tolerance) &&
            Near(p.g, static_cast<int>(std::lround(c.g * c.a * 255)), tolerance) &&
            Near(p.b, static_cast<int>(std::lround(c.b * c.a * 255)), tolerance) &&
            Near(p.a, static_cast<int>(std::lround(c.a * 255)), tolerance);
    }

    // Ink on a white background: any channel noticeably darker than white.
    inline bool IsInk(const Pixel& p)
    {
        return p.r < 200 || p.g < 200 || p.b < 200;
    }

    inline void LogPixel(const wchar_t* label, uint32_t x, uint32_t y, const Pixel& p)
    {
        WEX::Logging::Log::Comment(WEX::Common::String().Format(L"%s (%u,%u) = B%u G%u R%u A%u", label, x, y, p.b, p.g, p.r, p.a));
    }

    inline Mode CurrentMode()
    {
        WEX::Common::String text;
        VERIFY_SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"Mode", text));
        Mode mode{};
        VERIFY_IS_TRUE(TryParseMode(static_cast<const wchar_t*>(text), mode));
        return mode;
    }

    inline void LogNoHardwareSkip()
    {
        WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"No hardware D3D adapter on this machine; D2D-HW cannot be exercised.");
    }

    // Returns nullptr after logging Skipped when the mode needs hardware this machine does not have.
    inline std::unique_ptr<IImmediatePainter> CreatePainter(uint32_t width = c_size, uint32_t height = c_size, float dpiScale = 1.0f)
    {
        const Mode mode = CurrentMode();
        std::unique_ptr<IImmediatePainter> painter;
        const HRESULT hr = CreateImmediatePainter(mode, PainterOptions{ width, height, dpiScale }, painter);
        if (mode == Mode::ImmediateD2DHardware && hr == DXGI_ERROR_NOT_FOUND)
        {
            LogNoHardwareSkip();
            return nullptr;
        }
        VERIFY_SUCCEEDED(hr, WEX::Common::String().Format(L"CreateImmediatePainter(%s)", ModeName(mode)));
        VERIFY_IS_NOT_NULL(painter.get());
        return painter;
    }

    inline Image Read(IImmediatePainter& painter, uint32_t width = c_size, uint32_t height = c_size)
    {
        Image image{ width, height, std::vector<uint8_t>(static_cast<size_t>(width) * height * 4) };
        VERIFY_SUCCEEDED(painter.ReadPixels(image.bytes.data(), width * 4, static_cast<uint32_t>(image.bytes.size())));
        return image;
    }

    template <typename PaintFn>
    FrameStats Paint(IImmediatePainter& painter, PaintFn&& paint, const RectF* damage = nullptr, uint32_t damageCount = 0)
    {
        VERIFY_SUCCEEDED(painter.BeginFrame(damage, damageCount));
        paint(painter);
        FrameStats stats{};
        VERIFY_SUCCEEDED(painter.EndFrame(&stats));
        return stats;
    }
}
