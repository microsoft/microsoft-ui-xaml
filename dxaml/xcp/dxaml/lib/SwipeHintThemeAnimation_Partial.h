// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Need this for TimingFunctionDescription. It isn't included in time, and it requires the class def'ns
// in DXaml.g.h. SwipeHintThemeAnimation's header (this file) is included within DXaml.g.h, which creates
// a bit of a nasty loop.
#include "ThemeGenerator.h"
#include "SwipeHintThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(SwipeHintThemeAnimation)
    {
    public:

        struct SwipeHintTimingFunctionDescription : TimingFunctionDescription
        {
            SwipeHintTimingFunctionDescription()
            {
                // todo: should these come from pvl?? We have no way of doing this currently
                cp1.X = 0.0f;
                cp1.Y = 0.0f;
                cp2.X = 0.1f;
                cp2.Y = 0.9f;
                cp3.X = 0.2f;
                cp3.Y = 1.0f;
                cp4.X = 1.0f;
                cp4.Y = 1.0f;
            }
        };

        _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;
    };
}
