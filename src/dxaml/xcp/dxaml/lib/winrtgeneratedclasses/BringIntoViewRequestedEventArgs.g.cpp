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

#include "precomp.h"
#include "BringIntoViewRequestedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::BringIntoViewRequestedEventArgs::BringIntoViewRequestedEventArgs()
{
}

DirectUI::BringIntoViewRequestedEventArgs::~BringIntoViewRequestedEventArgs()
{
}

HRESULT DirectUI::BringIntoViewRequestedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::BringIntoViewRequestedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::BringIntoViewRequestedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::IBringIntoViewRequestedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::IBringIntoViewRequestedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::RoutedEventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_TargetElement(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    CUIElement* pValueCore = nullptr;

    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_TargetElement(&pValueCore));

    IFC(CValueBoxer::ConvertToFramework(pValueCore, ppValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::put_TargetElement(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    CUIElement* pValueCore = static_cast<CUIElement*>(pValue ? static_cast<DirectUI::UIElement*>(pValue)->GetHandle() : nullptr);

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->put_TargetElement(pValueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_AnimationDesired(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_AnimationDesired(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::put_AnimationDesired(BOOLEAN value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->put_AnimationDesired(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_TargetRect(_Out_ ABI::Windows::Foundation::Rect* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    ABI::Windows::Foundation::Rect valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_TargetRect(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::put_TargetRect(ABI::Windows::Foundation::Rect value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    ABI::Windows::Foundation::Rect valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->put_TargetRect(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_HorizontalAlignmentRatio(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_HorizontalAlignmentRatio(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_VerticalAlignmentRatio(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_VerticalAlignmentRatio(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_HorizontalOffset(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_HorizontalOffset(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::put_HorizontalOffset(DOUBLE value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->put_HorizontalOffset(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_VerticalOffset(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_VerticalOffset(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::put_VerticalOffset(DOUBLE value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->put_VerticalOffset(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::get_Handled(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->get_Handled(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::BringIntoViewRequestedEventArgs::put_Handled(BOOLEAN value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CBringIntoViewRequestedEventArgs*>(pCoreEventArgsNoRef)->put_Handled(valueCore));


Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateBringIntoViewRequestedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::BringIntoViewRequestedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_BringIntoViewRequestedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
