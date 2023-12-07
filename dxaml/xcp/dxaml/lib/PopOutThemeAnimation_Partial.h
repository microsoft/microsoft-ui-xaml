// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PopOutThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(PopOutThemeAnimation)
    {
        public:
            _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;
    };
}
