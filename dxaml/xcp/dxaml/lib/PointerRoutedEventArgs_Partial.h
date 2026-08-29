// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PointerRoutedEventArgs.g.h"

namespace DirectUI
{
    class PointerPointTransform;

    PARTIAL_CLASS(PointerRoutedEventArgs)
    {
    public:
        // Implement GetCurrentPoint and GetIntermediatePoints
        _Check_return_ HRESULT GetCurrentPointImpl(_In_opt_ xaml::IUIElement* relativeTo, _Outptr_ ixp::IPointerPoint** returnValue);
        _Check_return_ HRESULT GetIntermediatePointsImpl(_In_opt_ xaml::IUIElement* relativeTo, _Outptr_ wfc::IVector<ixp::PointerPoint*>** returnValue);
    };
}
