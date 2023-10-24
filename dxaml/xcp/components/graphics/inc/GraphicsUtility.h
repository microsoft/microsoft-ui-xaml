// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class GraphicsUtility
{
public:
    static bool IsDeviceLostError(HRESULT hr)
    {
        return
            (hr == DXGI_ERROR_DEVICE_REMOVED ||
            hr == DXGI_ERROR_DEVICE_RESET ||
            hr == D2DERR_RECREATE_TARGET ||
            hr == DXGI_ERROR_DEVICE_HUNG ||
            hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR);
    }
};

enum class HWRenderVisibility : bool
{
    Invisible = false,
    Visible = true
};

// These macros are intended to fail gracefully in a DeviceLost scenario, otherwise it will fail fast
#define IFC_DEVICE_LOST_OTHERWISE_FAIL_FAST(x) \
    hr = (x); \
    if (SUCCEEDED(hr)) \
    { \
    } \
    else if (GraphicsUtility::IsDeviceLostError(hr))\
    { \
        IFC(hr); \
    } \
    else \
    { \
        IFCFAILFAST(hr); \
    }

#define IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(x) \
    {\
        HRESULT _deviceLostHR_ = (x); \
        if (SUCCEEDED(_deviceLostHR_)) \
        { \
        } \
        else if (GraphicsUtility::IsDeviceLostError(_deviceLostHR_))\
        { \
            IFC_RETURN(_deviceLostHR_); \
        } \
        else \
        { \
            IFCFAILFAST(_deviceLostHR_); \
        } \
    }