// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitOpenThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(SplitOpenThemeAnimation)
    {
        public:
            _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;

        public:
            static const INT64 s_OpacityChangeDuration = 83;
            static const INT64 s_OpenDuration = 250;    
    };
}
