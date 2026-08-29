// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ConnectedAnimationService.g.h"
#include "ConnectedAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ConnectedAnimationService)
    {
         public:
            static _Check_return_ HRESULT GetForCurrentViewImpl(_Outptr_ xaml_animation::IConnectedAnimationService** ppReturnValue);
            _Check_return_ HRESULT PrepareToAnimateImpl(_In_ HSTRING key, _In_ xaml::IUIElement* pSource, _Outptr_ xaml_animation::IConnectedAnimation** ppReturnValue);
            _Check_return_ HRESULT GetAnimationImpl(_In_ HSTRING key, _Outptr_opt_ xaml_animation::IConnectedAnimation** ppReturnValue);

            // Properties.
            _Check_return_ HRESULT get_DefaultDurationImpl(_Out_ wf::TimeSpan* pValue);
            _Check_return_ HRESULT put_DefaultDurationImpl(_In_ wf::TimeSpan value);
            _Check_return_ HRESULT get_DefaultEasingFunctionImpl(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** ppValue);
            _Check_return_ HRESULT put_DefaultEasingFunctionImpl(_In_opt_ WUComp::ICompositionEasingFunction* pValue);

        private:
    };
}
