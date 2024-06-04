// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Brush.g.h"
#include <FeatureFlags.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT Brush::StartAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation)
{
    CBrush* brush = static_cast<CBrush*>(GetHandle());
    return brush->StartAnimation(animation);
}

_Check_return_ HRESULT Brush::StopAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation)
{
    CBrush* brush = static_cast<CBrush*>(GetHandle());
    return brush->StopAnimation(animation);
}

_Check_return_ HRESULT STDMETHODCALLTYPE Brush::PopulatePropertyInfo(
    _In_ HSTRING propertyName,
    _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
    )
{
    return PopulatePropertyInfoOverrideProtected(propertyName, animationPropertyInfo);
}

_Check_return_ HRESULT Brush::PopulatePropertyInfoOverrideImpl(
    _In_ HSTRING propertyName,
    _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
    )
{
    IFC_RETURN(E_NOTIMPL);
    return S_OK;
}

