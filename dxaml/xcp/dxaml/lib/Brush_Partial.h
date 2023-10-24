// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Brush.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Brush)
    {
    public:
        _Check_return_ HRESULT StartAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation);
        _Check_return_ HRESULT StopAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation);

        _Check_return_ HRESULT STDMETHODCALLTYPE PopulatePropertyInfo(
            _In_ HSTRING propertyName,
            _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
            ) override;

        _Check_return_ HRESULT PopulatePropertyInfoOverrideImpl(
            _In_ HSTRING propertyName,
            _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
            );

    };
}

