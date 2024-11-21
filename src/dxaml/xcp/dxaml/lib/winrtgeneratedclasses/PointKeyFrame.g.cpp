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

#include "PointKeyFrame.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PointKeyFrame::PointKeyFrame()
{
}

DirectUI::PointKeyFrame::~PointKeyFrame()
{
}

HRESULT DirectUI::PointKeyFrame::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PointKeyFrame)))
    {
        *ppObject = static_cast<DirectUI::PointKeyFrame*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrame)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrame*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::PointKeyFrame::get_KeyTime(_Out_ ABI::Microsoft::UI::Xaml::Media::Animation::KeyTime* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointKeyFrame_KeyTime, pValue));
}
IFACEMETHODIMP DirectUI::PointKeyFrame::put_KeyTime(ABI::Microsoft::UI::Xaml::Media::Animation::KeyTime value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointKeyFrame_KeyTime, value));
}
IFACEMETHODIMP DirectUI::PointKeyFrame::get_Value(_Out_ ABI::Windows::Foundation::Point* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointKeyFrame_Value, pValue));
}
IFACEMETHODIMP DirectUI::PointKeyFrame::put_Value(ABI::Windows::Foundation::Point value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointKeyFrame_Value, value));
}

// Events.

// Methods.

HRESULT DirectUI::PointKeyFrameFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrameFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrameFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrameStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrameStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableAbstractCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::PointKeyFrameFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrame** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPointKeyFrame);
    const GUID metadataAPIGUID = MetadataAPI::GetClassInfoByIndex(GetTypeIndex())->GetGuid();
    const KnownTypeIndex typeIndex = GetTypeIndex();

    if(uuidofGUID != metadataAPIGUID)
    {
        XAML_FAIL_FAST();
    }
#endif

    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableAbstractCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::PointKeyFrameFactory::get_ValueProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointKeyFrame_Value, ppValue));
}
IFACEMETHODIMP DirectUI::PointKeyFrameFactory::get_KeyTimeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointKeyFrame_KeyTime, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PointKeyFrame()
    {
        RRETURN(ctl::ActivationFactoryCreator<PointKeyFrameFactory>::CreateActivationFactory());
    }
}
