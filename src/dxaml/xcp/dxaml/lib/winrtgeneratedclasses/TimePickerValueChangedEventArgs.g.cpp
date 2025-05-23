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
#include "TimePickerValueChangedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::TimePickerValueChangedEventArgs::TimePickerValueChangedEventArgs(): m_oldTime(), m_newTime()
{
}

DirectUI::TimePickerValueChangedEventArgs::~TimePickerValueChangedEventArgs()
{
}

HRESULT DirectUI::TimePickerValueChangedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::TimePickerValueChangedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::TimePickerValueChangedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ITimePickerValueChangedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ITimePickerValueChangedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::TimePickerValueChangedEventArgs::get_OldTime(_Out_ ABI::Windows::Foundation::TimeSpan* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_oldTime, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::TimePickerValueChangedEventArgs::put_OldTime(ABI::Windows::Foundation::TimeSpan value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_oldTime));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::TimePickerValueChangedEventArgs::get_NewTime(_Out_ ABI::Windows::Foundation::TimeSpan* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_newTime, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::TimePickerValueChangedEventArgs::put_NewTime(ABI::Windows::Foundation::TimeSpan value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_newTime));
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateTimePickerValueChangedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::TimePickerValueChangedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_TimePickerValueChangedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
