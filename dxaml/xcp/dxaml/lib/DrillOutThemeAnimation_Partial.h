// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DrillOutThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DrillOutThemeAnimation)
    {
    public:
        _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;

    public:
        static const INT64 s_BackNavigatingAwayScaleDuration = 100;
        static const INT64 s_BackNavigatingAwayOpacityDuration = 100;
        static const INT64 s_BackNavigatingToBeginTime = 100;
        static const INT64 s_BackNavigatingToScaleDuration = 333 + s_BackNavigatingToBeginTime;
        static const INT64 s_BackNavigatingToOpacityDuration = 333 + s_BackNavigatingToBeginTime;
    };
}