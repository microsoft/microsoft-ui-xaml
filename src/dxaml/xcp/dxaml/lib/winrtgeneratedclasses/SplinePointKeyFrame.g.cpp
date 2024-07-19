// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "SplinePointKeyFrame.g.h"
#include "KeySpline.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::SplinePointKeyFrame::SplinePointKeyFrame()
{
}

DirectUI::SplinePointKeyFrame::~SplinePointKeyFrame()
{
}

HRESULT DirectUI::SplinePointKeyFrame::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::SplinePointKeyFrame)))
    {
        *ppObject = static_cast<DirectUI::SplinePointKeyFrame*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::ISplinePointKeyFrame)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::ISplinePointKeyFrame*>(this);
    }
    else
    {
        RRETURN(DirectUI::PointKeyFrame::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::SplinePointKeyFrame::get_KeySpline(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::Animation::IKeySpline** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplinePointKeyFrame_KeySpline, ppValue));
}
IFACEMETHODIMP DirectUI::SplinePointKeyFrame::put_KeySpline(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::IKeySpline* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplinePointKeyFrame_KeySpline, pValue));
}

// Events.

// Methods.

HRESULT DirectUI::SplinePointKeyFrameFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::ISplinePointKeyFrameStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::ISplinePointKeyFrameStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.

// Dependency properties.
IFACEMETHODIMP DirectUI::SplinePointKeyFrameFactory::get_KeySplineProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplinePointKeyFrame_KeySpline, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_SplinePointKeyFrame()
    {
        RRETURN(ctl::ActivationFactoryCreator<SplinePointKeyFrameFactory>::CreateActivationFactory());
    }
}
