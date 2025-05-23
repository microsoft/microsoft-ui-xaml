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

#include "TransitionTarget.g.h"
#include "CompositeTransform.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::TransitionTarget::TransitionTarget()
{
}

DirectUI::TransitionTarget::~TransitionTarget()
{
}

HRESULT DirectUI::TransitionTarget::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::TransitionTarget)))
    {
        *ppObject = static_cast<DirectUI::TransitionTarget*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
_Check_return_ HRESULT DirectUI::TransitionTarget::get_ClipTransform(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::ICompositeTransform** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_ClipTransform, ppValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::put_ClipTransform(_In_opt_ ABI::Microsoft::UI::Xaml::Media::ICompositeTransform* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_ClipTransform, pValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::get_ClipTransformOrigin(_Out_ ABI::Windows::Foundation::Point* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_ClipTransformOrigin, pValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::put_ClipTransformOrigin(ABI::Windows::Foundation::Point value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_ClipTransformOrigin, value));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::get_CompositeTransform(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::ICompositeTransform** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_CompositeTransform, ppValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::put_CompositeTransform(_In_opt_ ABI::Microsoft::UI::Xaml::Media::ICompositeTransform* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_CompositeTransform, pValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::get_Opacity(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_Opacity, pValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::put_Opacity(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_Opacity, value));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::get_OpacityAnimation(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_OpacityAnimation, ppValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::put_OpacityAnimation(_In_opt_ IInspectable* pValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_OpacityAnimation, pValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::get_TransformOrigin(_Out_ ABI::Windows::Foundation::Point* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_TransformOrigin, pValue));
}
_Check_return_ HRESULT DirectUI::TransitionTarget::put_TransformOrigin(ABI::Windows::Foundation::Point value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_TransformOrigin, value));
}

// Events.

// Methods.


