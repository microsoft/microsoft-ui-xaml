// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <GdiPlusPainter.h>
#include <cmath>

// GDI+ headers call min/max unqualified; precomp.h undefines the macros.
namespace Gdiplus
{
    using std::min;
    using std::max;
}
#include <objidl.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// GDI+ immediate painter (CPU, antialiased, PARGB). Owned by the immediate-rendering-gdiplus branch.
//
// Contract for the implementation (tests: unittests\GdiPlusPainterTests.cpp):
// - Runtime: every painter holds a GdiPlus::RuntimeReference. GdiplusStartup/GdiplusShutdown are refcounted across
//   painters; destroying the last painter shuts GDI+ down, and a later painter starts it again.
// - Target: a Gdiplus::Bitmap in PixelFormat32bppPARGB. ReadPixels returns its bytes unchanged (already premultiplied
//   BGRA), so a 50% alpha fill on a transparent clear reads back with color channels scaled by alpha.
// - Quality: SmoothingModeAntiAlias for geometry; PixelOffsetModeHalf so pixel centers are at +0.5 like D2D and
//   integer-aligned rectangles have crisp edges. Text is drawn through DirectWrite with grayscale antialiasing, never
//   ClearType, because the target has per-pixel alpha (ClearType's per-channel coverage cannot be stored in it).
// - Resources: brushes, pens, fonts, and text formats are cached by value; an identical second frame creates none
//   (GetLastFrameResourceChurn reports 0 created / 0 destroyed). Steady-state frames must not grow the process GDI
//   object count or ResourceInventory::cachedObjectCount.
// - Upload/copy accounting: GDI+ cannot draw to a presentable surface while preserving alpha, so each frame paints into
//   the CPU bitmap and then copies ONLY the damaged region(s) to the presentable surface. uploadBytesThisFrame is that
//   copy, DamageCopyBytes(width, height, damage) == FrameStats::damagedPixels * 4. A full-target frame copies the whole
//   bitmap. When no presentable surface is attached (offscreen tests), the copy that would happen is still reported, so
//   the metric is comparable with D2D-SW's upload bytes.
namespace RenderingBackend
{
    namespace GdiPlus
    {
        namespace
        {
            std::mutex s_runtimeLock;
            uint32_t s_runtimeReferences = 0;
            ULONG_PTR s_runtimeToken = 0;

            HRESULT HResultFromStatus(Gdiplus::Status status) noexcept
            {
                switch (status)
                {
                    case Gdiplus::Ok: return S_OK;
                    case Gdiplus::InvalidParameter: return E_INVALIDARG;
                    case Gdiplus::OutOfMemory: return E_OUTOFMEMORY;
                    case Gdiplus::NotImplemented: return E_NOTIMPL;
                    case Gdiplus::Win32Error: return HRESULT_FROM_WIN32(GetLastError());
                    default: return E_FAIL;
                }
            }

            uint32_t ClampEdge(float value, uint32_t limit, bool roundUp) noexcept
            {
                const float rounded = roundUp ? std::ceil(value) : std::floor(value);
                if (!(rounded > 0))
                {
                    return 0;
                }
                return rounded >= static_cast<float>(limit) ? limit : static_cast<uint32_t>(rounded);
            }
        }

        HRESULT RuntimeReference::Acquire() noexcept
        {
            if (m_held)
            {
                return S_OK;
            }
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            if (s_runtimeReferences == 0)
            {
                Gdiplus::GdiplusStartupInput input;
                ULONG_PTR token = 0;
                const HRESULT hr = HResultFromStatus(Gdiplus::GdiplusStartup(&token, &input, nullptr));
                if (FAILED(hr))
                {
                    return hr;
                }
                s_runtimeToken = token;
            }
            ++s_runtimeReferences;
            m_held = true;
            return S_OK;
        }

        void RuntimeReference::Release() noexcept
        {
            if (!m_held)
            {
                return;
            }
            m_held = false;
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            if (--s_runtimeReferences == 0)
            {
                Gdiplus::GdiplusShutdown(s_runtimeToken);
                s_runtimeToken = 0;
            }
        }

        uint32_t GetRuntimeReferenceCount() noexcept
        {
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            return s_runtimeReferences;
        }

        bool IsRuntimeStarted() noexcept
        {
            std::lock_guard<std::mutex> lock(s_runtimeLock);
            return s_runtimeToken != 0;
        }

        uint64_t DamagedPixelCount(uint32_t width, uint32_t height, const RectF* damageRects, uint32_t count) noexcept
        {
            if (damageRects == nullptr || count == 0)
            {
                return static_cast<uint64_t>(width) * height;
            }
            uint64_t total = 0;
            for (uint32_t index = 0; index < count; ++index)
            {
                const RectF& rect = damageRects[index];
                const uint32_t left = ClampEdge(rect.left, width, false);
                const uint32_t top = ClampEdge(rect.top, height, false);
                const uint32_t right = ClampEdge(rect.right, width, true);
                const uint32_t bottom = ClampEdge(rect.bottom, height, true);
                if (right > left && bottom > top)
                {
                    total += static_cast<uint64_t>(right - left) * (bottom - top);
                }
            }
            return total;
        }

        uint64_t DamageCopyBytes(uint32_t width, uint32_t height, const RectF* damageRects, uint32_t count) noexcept
        {
            constexpr uint64_t c_bytesPerPixel = 4; // PARGB
            return DamagedPixelCount(width, height, damageRects, count) * c_bytesPerPixel;
        }

        HRESULT GetLastFrameResourceChurn(const IImmediatePainter& painter, ResourceChurn* churn) noexcept
        {
            *churn = {};
            if (painter.GetDeviceIdentity().mode != Mode::ImmediateGdiPlus)
            {
                return E_INVALIDARG;
            }
            return E_NOTIMPL;
        }
    }

    HRESULT CreateGdiPlusPainter(const PainterOptions&, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        painter.reset();
        return E_NOTIMPL;
    }
}
