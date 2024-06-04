// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Timeline.g.h"
#include "DoubleKeyFrame.g.h"
#include "PointKeyFrame.g.h"
#include "ColorKeyFrame.g.h"
#include "ObjectKeyFrame.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_
HRESULT
Timeline::OnInheritanceContextChanged()
{
    HRESULT hr = S_OK;
    HRESULT xr = S_OK;

    // First call up to base to process this object
    IFC(TimelineGenerated::OnInheritanceContextChanged());

    // Now process the children of this object, if any
    // TODO: Add more composite animation types here
    switch (GetTypeIndex())
    {
    case KnownTypeIndex::DoubleAnimationUsingKeyFrames:
        xr = ProcessKeyFrames<
            xaml_animation::IDoubleAnimationUsingKeyFrames,
            xaml_animation::IDoubleKeyFrame,
            xaml_animation::DoubleKeyFrame,
            DoubleKeyFrame>(this);
        IFC(xr);
        break;

    case KnownTypeIndex::ColorAnimationUsingKeyFrames:
        xr = ProcessKeyFrames<
            xaml_animation::IColorAnimationUsingKeyFrames,
            xaml_animation::IColorKeyFrame,
            xaml_animation::ColorKeyFrame,
            ColorKeyFrame>(this);
        IFC(xr);
        break;

    case KnownTypeIndex::PointAnimationUsingKeyFrames:
        xr = ProcessKeyFrames<
            xaml_animation::IPointAnimationUsingKeyFrames,
            xaml_animation::IPointKeyFrame,
            xaml_animation::PointKeyFrame,
            PointKeyFrame>(this);
        IFC(xr);
        break;

    case KnownTypeIndex::ObjectAnimationUsingKeyFrames:
        xr = ProcessKeyFrames<
            xaml_animation::IObjectAnimationUsingKeyFrames,
            xaml_animation::IObjectKeyFrame,
            xaml_animation::ObjectKeyFrame,
            ObjectKeyFrame>(this);
        IFC(xr);
        break;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a value indicating that dependent animations are allowed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TimelineFactory::get_AllowDependentAnimationsImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    bool allowDependentAnimations = false;

    IFCPTR(pValue);

    IFC(CoreImports::Timeline_GetAllowDependentAnimations(&allowDependentAnimations));
    *pValue = static_cast<BOOLEAN>(allowDependentAnimations);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a value indicating that dependent animations are allowed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TimelineFactory::put_AllowDependentAnimationsImpl(_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;

    IFC(CoreImports::Timeline_SetAllowDependentAnimations(!!value));

Cleanup:
    RRETURN(hr);
}
