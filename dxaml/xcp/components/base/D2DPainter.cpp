// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <D2DPainter.h>

// Direct2D immediate painter shared by D2D-HW and D2D-SW; the device decides hardware versus WARP.
// Owned by the immediate-rendering-d2d-sw branch.
//
// Device ownership
//   The painter draws with the ID3D11Device it is given (CreateD3DDeviceForMode picks WARP or a hardware adapter) and
//   never creates its own. GetDeviceIdentity reports that device's adapter LUID, WARP flag, and feature level.
//
// Copy accounting: ResourceInventory::uploadBytesThisFrame (test plan metric W5)
//   Reset at BeginFrame; describes the frame completed by the last EndFrame.
//   D2D-SW: WARP rasterizes on the CPU straight into the target bitmap, which lives in system memory, so drawing
//     itself uploads nothing. Making the frame visible needs a copy of the damaged pixels from that target into the
//     presentation surface (the attached swap chain's back buffer), which DWM then consumes. The painter counts that
//     copy: for a presented frame, uploadBytesThisFrame == sum over damage rects (clipped to the target) of
//     width * height * 4 == FrameStats::damagedPixels * 4 for non-overlapping damage. A frame painted with no swap
//     chain attached (offscreen-only: tests, fidelity capture) reports 0. ReadPixels is a diagnostic readback and is
//     never counted. Copies DWM makes on its own side are outside the process and are measured with the WPR Gpu
//     profile instead.
//   D2D-HW: defined by the immediate-rendering-d2d-hw branch (CPU-to-GPU uploads such as glyph and bitmap uploads; a
//     same-adapter target-to-back-buffer copy is GPU-local).
//
// Device removal
//   When the device is removed or reset, BeginFrame, EndFrame, Resize, and ReadPixels return the failure
//   (DXGI_ERROR_DEVICE_REMOVED or the device's removed reason) instead of crashing, and the painter emits DeviceLost.
//   Drawing calls between a failed BeginFrame and EndFrame are no-ops. Recovery (new device, recreate resources) is the
//   caller's decision.
namespace RenderingBackend
{
    HRESULT CreateD2DPainter(Mode mode, ID3D11Device* device, const PainterOptions& options, std::unique_ptr<ID2DImmediatePainter>& painter) noexcept
    {
        painter.reset();
        if ((mode != Mode::ImmediateD2DHardware && mode != Mode::ImmediateD2DSoftware) || device == nullptr ||
            options.width == 0 || options.height == 0 || !(options.dpiScale > 0))
        {
            return E_INVALIDARG;
        }
        return E_NOTIMPL;
    }

    HRESULT CreateD2DPainter(Mode mode, ID3D11Device* device, const PainterOptions& options, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        std::unique_ptr<ID2DImmediatePainter> d2dPainter;
        const HRESULT hr = CreateD2DPainter(mode, device, options, d2dPainter);
        painter = std::move(d2dPainter);
        return hr;
    }
}
