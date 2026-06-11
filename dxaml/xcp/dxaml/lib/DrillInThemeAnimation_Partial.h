// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DrillInThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DrillInThemeAnimation)
    {
    public:
        _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;

    public:
        static const INT64 s_NavigatingAwayScaleDuration = 100;
        static const INT64 s_NavigatingAwayOpacityDuration = 100;
        static const INT64 s_NavigatingToBeginTime = 100;
        static const INT64 s_NavigatingToScaleDuration = 783 + s_NavigatingToBeginTime;
        static const INT64 s_NavigatingToOpacityDuration = 333 + s_NavigatingToBeginTime;
    };
}