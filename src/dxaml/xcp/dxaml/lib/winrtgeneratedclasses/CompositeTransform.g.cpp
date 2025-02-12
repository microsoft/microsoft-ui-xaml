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

#include "CompositeTransform.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::CompositeTransform::CompositeTransform()
{
}

DirectUI::CompositeTransform::~CompositeTransform()
{
}

HRESULT DirectUI::CompositeTransform::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::CompositeTransform)))
    {
        *ppObject = static_cast<DirectUI::CompositeTransform*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::ICompositeTransform)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::ICompositeTransform*>(this);
    }
    else
    {
        RRETURN(DirectUI::Transform::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::CompositeTransform::get_CenterX(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterX, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_CenterX(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterX, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_CenterXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterXAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_CenterXAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterXAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_CenterY(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterY, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_CenterY(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterY, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_CenterYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterYAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_CenterYAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_CenterYAnimation, pValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_RotateAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_RotateAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_RotateAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_RotateAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_Rotation(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_Rotation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_Rotation(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_Rotation, value));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_ScaleX(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleX, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_ScaleX(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleX, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_ScaleXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleXAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_ScaleXAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleXAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_ScaleY(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleY, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_ScaleY(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleY, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_ScaleYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleYAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_ScaleYAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_ScaleYAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_SkewX(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewX, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_SkewX(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewX, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_SkewXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewXAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_SkewXAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewXAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_SkewY(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewY, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_SkewY(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewY, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_SkewYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewYAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_SkewYAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_SkewYAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_TranslateX(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateX, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_TranslateX(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateX, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_TranslateXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateXAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_TranslateXAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateXAnimation, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::get_TranslateY(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateY, pValue));
}
IFACEMETHODIMP DirectUI::CompositeTransform::put_TranslateY(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateY, value));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::get_TranslateYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateYAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::CompositeTransform::put_TranslateYAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CompositeTransform_TranslateYAnimation, pValue));
}

// Events.

// Methods.

HRESULT DirectUI::CompositeTransformFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::ICompositeTransformStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::ICompositeTransformStatics*>(this);
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
IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_CenterXProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_CenterX, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_CenterYProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_CenterY, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_ScaleXProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_ScaleX, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_ScaleYProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_ScaleY, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_SkewXProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_SkewX, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_SkewYProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_SkewY, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_RotationProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_Rotation, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_TranslateXProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_TranslateX, ppValue));
}

IFACEMETHODIMP DirectUI::CompositeTransformFactory::get_TranslateYProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::CompositeTransform_TranslateY, ppValue));
}


// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_CompositeTransform()
    {
        RRETURN(ctl::ActivationFactoryCreator<CompositeTransformFactory>::CreateActivationFactory());
    }
}
