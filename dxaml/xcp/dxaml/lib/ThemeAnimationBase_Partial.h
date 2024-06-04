// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ThemeAnimationBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ThemeAnimationBase)
    {
    public:
        _Check_return_ HRESULT CreateTimelines(
            _In_ BOOLEAN bOnlyGenerateSteadyState,
            _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection) override
        {
            RRETURN(CreateTimelinesInternalProtected(bOnlyGenerateSteadyState, timelineCollection));
        }

        IFACEMETHOD(CreateTimelinesInternal)(_In_ BOOLEAN onlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection)
        {
            RRETURN(E_NOTIMPL);
        }

        _Check_return_ HRESULT CreateTimelinesInternalImpl(_In_ BOOLEAN onlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection)
        {
            RRETURN(E_NOTIMPL);
        }
    };
}
