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

#include "SkewTransform.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::SkewTransform::SkewTransform()
{
}

DirectUI::SkewTransform::~SkewTransform()
{
}

HRESULT DirectUI::SkewTransform::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::SkewTransform)))
    {
        *ppObject = static_cast<DirectUI::SkewTransform*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::ISkewTransform)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::ISkewTransform*>(this);
    }
    else
    {
        RRETURN(DirectUI::Transform::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::SkewTransform::get_AngleX(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleX, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::put_AngleX(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleX, value));
}
_Check_return_ HRESULT DirectUI::SkewTransform::get_AngleXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleXAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::SkewTransform::put_AngleXAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleXAnimation, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::get_AngleY(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleY, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::put_AngleY(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleY, value));
}
_Check_return_ HRESULT DirectUI::SkewTransform::get_AngleYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleYAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::SkewTransform::put_AngleYAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_AngleYAnimation, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::get_CenterX(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterX, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::put_CenterX(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterX, value));
}
_Check_return_ HRESULT DirectUI::SkewTransform::get_CenterXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterXAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::SkewTransform::put_CenterXAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterXAnimation, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::get_CenterY(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterY, pValue));
}
IFACEMETHODIMP DirectUI::SkewTransform::put_CenterY(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterY, value));
}
_Check_return_ HRESULT DirectUI::SkewTransform::get_CenterYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterYAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::SkewTransform::put_CenterYAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SkewTransform_CenterYAnimation, pValue));
}

// Events.

// Methods.

HRESULT DirectUI::SkewTransformFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::ISkewTransformStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::ISkewTransformStatics*>(this);
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
IFACEMETHODIMP DirectUI::SkewTransformFactory::get_CenterXProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SkewTransform_CenterX, ppValue));
}

IFACEMETHODIMP DirectUI::SkewTransformFactory::get_CenterYProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SkewTransform_CenterY, ppValue));
}

IFACEMETHODIMP DirectUI::SkewTransformFactory::get_AngleXProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SkewTransform_AngleX, ppValue));
}

IFACEMETHODIMP DirectUI::SkewTransformFactory::get_AngleYProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SkewTransform_AngleY, ppValue));
}


// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_SkewTransform()
    {
        RRETURN(ctl::ActivationFactoryCreator<SkewTransformFactory>::CreateActivationFactory());
    }
}
