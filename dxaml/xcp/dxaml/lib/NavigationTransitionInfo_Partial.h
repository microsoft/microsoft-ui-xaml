// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationTransitionInfo.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(NavigationTransitionInfo)
    {
    public:
        _Check_return_ HRESULT CreateStoryboardsCoreImpl(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) { RRETURN(S_OK); };
        _Check_return_ HRESULT CreateStoryboardsImpl(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
        { return CreateStoryboardsCore(element, trigger, storyboards); }

        _Check_return_ HRESULT GetNavigationStateCoreImpl(_Out_ HSTRING* string) { RRETURN(S_OK); };
        _Check_return_ HRESULT SetNavigationStateCoreImpl(_In_ HSTRING string) { RRETURN(S_OK); };
    };
}
