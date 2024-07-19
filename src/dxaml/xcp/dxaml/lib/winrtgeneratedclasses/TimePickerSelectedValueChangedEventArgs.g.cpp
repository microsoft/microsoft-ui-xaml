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
#include "TimePickerSelectedValueChangedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::TimePickerSelectedValueChangedEventArgs::TimePickerSelectedValueChangedEventArgs()
{
}

DirectUI::TimePickerSelectedValueChangedEventArgs::~TimePickerSelectedValueChangedEventArgs()
{
}

HRESULT DirectUI::TimePickerSelectedValueChangedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::TimePickerSelectedValueChangedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::TimePickerSelectedValueChangedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ITimePickerSelectedValueChangedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ITimePickerSelectedValueChangedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::TimePickerSelectedValueChangedEventArgs::get_OldTime(_Out_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_oldTime.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::TimePickerSelectedValueChangedEventArgs::put_OldTime(_In_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_oldTime, pValue);
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::TimePickerSelectedValueChangedEventArgs::get_NewTime(_Out_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_newTime.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::TimePickerSelectedValueChangedEventArgs::put_NewTime(_In_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::TimeSpan>* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_newTime, pValue);
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateTimePickerSelectedValueChangedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::TimePickerSelectedValueChangedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TimePickerSelectedValueChangedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
