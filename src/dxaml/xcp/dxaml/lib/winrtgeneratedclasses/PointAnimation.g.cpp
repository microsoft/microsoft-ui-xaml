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

#include "PointAnimation.g.h"
#include "EasingFunctionBase.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PointAnimation::PointAnimation()
{
}

DirectUI::PointAnimation::~PointAnimation()
{
}

HRESULT DirectUI::PointAnimation::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PointAnimation)))
    {
        *ppObject = static_cast<DirectUI::PointAnimation*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPointAnimation)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPointAnimation*>(this);
    }
    else
    {
        RRETURN(DirectUI::Timeline::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::PointAnimation::get_By(_Out_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Point>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointAnimation_By, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::put_By(ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Point>* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointAnimation_By, pValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::get_EasingFunction(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::Animation::IEasingFunctionBase** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointAnimation_EasingFunction, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::put_EasingFunction(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::IEasingFunctionBase* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointAnimation_EasingFunction, pValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::get_EnableDependentAnimation(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointAnimation_EnableDependentAnimation, pValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::put_EnableDependentAnimation(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointAnimation_EnableDependentAnimation, value));
}
IFACEMETHODIMP DirectUI::PointAnimation::get_From(_Out_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Point>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointAnimation_From, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::put_From(ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Point>* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointAnimation_From, pValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::get_To(_Out_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Point>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PointAnimation_To, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimation::put_To(ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Point>* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PointAnimation_To, pValue));
}

// Events.

// Methods.

HRESULT DirectUI::PointAnimationFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IPointAnimationStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IPointAnimationStatics*>(this);
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
IFACEMETHODIMP DirectUI::PointAnimationFactory::get_FromProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointAnimation_From, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimationFactory::get_ToProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointAnimation_To, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimationFactory::get_ByProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointAnimation_By, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimationFactory::get_EasingFunctionProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointAnimation_EasingFunction, ppValue));
}
IFACEMETHODIMP DirectUI::PointAnimationFactory::get_EnableDependentAnimationProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PointAnimation_EnableDependentAnimation, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PointAnimation()
    {
        RRETURN(ctl::ActivationFactoryCreator<PointAnimationFactory>::CreateActivationFactory());
    }
}
