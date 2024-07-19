// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ConnectedAnimation.g.h"
#include <fwd/windows.ui.composition.h>

namespace DirectUI
{
    PARTIAL_CLASS(ConnectedAnimation)
    {
    public:
        _Check_return_ HRESULT TryStartImpl(_In_ xaml::IUIElement* pDestination, _Out_ BOOLEAN* pReturnValue);
        _Check_return_ HRESULT TryStartWithCoordinatedElementsImpl(_In_ xaml::IUIElement* pDestination, _In_ wfc::IIterable<xaml::UIElement*>* pCoordinatedElements, _Out_ BOOLEAN* pReturnValue);
        _Check_return_ HRESULT CancelImpl();
        _Check_return_ HRESULT SetAnimationComponentImpl(_In_ xaml_animation::ConnectedAnimationComponent component, _In_opt_ WUComp::ICompositionAnimationBase *animation);
        _Check_return_ HRESULT get_IsScaleAnimationEnabledImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsScaleAnimationEnabledImpl(_In_ BOOLEAN pValue);
        _Check_return_ HRESULT get_ConfigurationImpl(_Out_ xaml_animation::IConnectedAnimationConfiguration** value);
        _Check_return_ HRESULT put_ConfigurationImpl(_In_ xaml_animation::IConnectedAnimationConfiguration* value);

    private:
    };
}
