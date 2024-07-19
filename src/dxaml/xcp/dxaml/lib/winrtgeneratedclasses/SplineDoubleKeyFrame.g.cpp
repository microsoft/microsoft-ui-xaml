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

#include "SplineDoubleKeyFrame.g.h"
#include "KeySpline.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::SplineDoubleKeyFrame::SplineDoubleKeyFrame()
{
}

DirectUI::SplineDoubleKeyFrame::~SplineDoubleKeyFrame()
{
}

HRESULT DirectUI::SplineDoubleKeyFrame::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::SplineDoubleKeyFrame)))
    {
        *ppObject = static_cast<DirectUI::SplineDoubleKeyFrame*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::ISplineDoubleKeyFrame)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::ISplineDoubleKeyFrame*>(this);
    }
    else
    {
        RRETURN(DirectUI::DoubleKeyFrame::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::SplineDoubleKeyFrame::get_KeySpline(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::Animation::IKeySpline** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplineDoubleKeyFrame_KeySpline, ppValue));
}
IFACEMETHODIMP DirectUI::SplineDoubleKeyFrame::put_KeySpline(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::IKeySpline* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplineDoubleKeyFrame_KeySpline, pValue));
}

// Events.

// Methods.

HRESULT DirectUI::SplineDoubleKeyFrameFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::ISplineDoubleKeyFrameStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::ISplineDoubleKeyFrameStatics*>(this);
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
IFACEMETHODIMP DirectUI::SplineDoubleKeyFrameFactory::get_KeySplineProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplineDoubleKeyFrame_KeySpline, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_SplineDoubleKeyFrame()
    {
        RRETURN(ctl::ActivationFactoryCreator<SplineDoubleKeyFrameFactory>::CreateActivationFactory());
    }
}
