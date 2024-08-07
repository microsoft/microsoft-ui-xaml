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
#include "CalendarViewSelectedDatesChangedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::CalendarViewSelectedDatesChangedEventArgs::CalendarViewSelectedDatesChangedEventArgs()
{
}

DirectUI::CalendarViewSelectedDatesChangedEventArgs::~CalendarViewSelectedDatesChangedEventArgs()
{
}

HRESULT DirectUI::CalendarViewSelectedDatesChangedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::CalendarViewSelectedDatesChangedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::CalendarViewSelectedDatesChangedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICalendarViewSelectedDatesChangedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICalendarViewSelectedDatesChangedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::CalendarViewSelectedDatesChangedEventArgs::get_AddedDates(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Foundation::DateTime>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pAddedDates.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::CalendarViewSelectedDatesChangedEventArgs::put_AddedDates(_In_opt_ ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Foundation::DateTime>* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pAddedDates, pValue);
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::CalendarViewSelectedDatesChangedEventArgs::get_RemovedDates(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Foundation::DateTime>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pRemovedDates.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::CalendarViewSelectedDatesChangedEventArgs::put_RemovedDates(_In_opt_ ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Foundation::DateTime>* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pRemovedDates, pValue);
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateCalendarViewSelectedDatesChangedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::CalendarViewSelectedDatesChangedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_CalendarViewSelectedDatesChangedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
