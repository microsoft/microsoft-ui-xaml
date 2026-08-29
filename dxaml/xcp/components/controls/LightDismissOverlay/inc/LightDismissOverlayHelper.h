// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Microsoft.UI.Xaml.h"
#include <XboxUtility.h>

class LightDismissOverlayHelper
{
public:
    static bool IsOverlayVisibleForMode(xaml_controls::LightDismissOverlayMode mode)
    {
        bool isOverlayVisible = false;

        if (mode == xaml_controls::LightDismissOverlayMode_Auto)
        {
            isOverlayVisible = XboxUtility::IsOnXbox();
        }
        else
        {
            isOverlayVisible = (mode == xaml_controls::LightDismissOverlayMode_On);
        }

        return isOverlayVisible;
    }

    // Controls that call this should have a get_LightDismissOverlayMode() method.
    template <typename T>
    static HRESULT ResolveIsOverlayVisibleForControl(T* control, _Out_ bool* isOverlayVisible)
    {
        auto overlayMode = xaml_controls::LightDismissOverlayMode_Off;
        IFC_RETURN(control->get_LightDismissOverlayMode(&overlayMode));

        *isOverlayVisible = IsOverlayVisibleForMode(overlayMode);

        return S_OK;
    }
};
