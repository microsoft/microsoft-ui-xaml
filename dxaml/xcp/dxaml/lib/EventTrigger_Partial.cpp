// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EventTrigger.g.h"
#include "RoutedEvent.g.h"

using namespace DirectUI;
using namespace xaml;

_Check_return_ HRESULT DirectUI::EventTrigger::get_RoutedEventImpl(_Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;
    HSTRING hValue = nullptr;
    IFC(GetValueByKnownIndex(KnownPropertyIndex::EventTrigger_RoutedEvent, &hValue));

    if (hValue)
    {
        // Currently we only support the FrameworkElement.Loaded event.
        ctl::ComPtr<RoutedEvent> spInstance;
        IFC(ctl::make(KnownEventIndex::FrameworkElement_Loaded, wrl_wrappers::HStringReference(L"FrameworkElement.Loaded").Get(), &spInstance));
        *ppValue = spInstance.Detach();
    }
    else
    {
        *ppValue = nullptr;
    }

Cleanup:
    WindowsDeleteString(hValue);
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::EventTrigger::put_RoutedEventImpl(_In_ xaml::IRoutedEvent* pValue)
{
    HRESULT hr = S_OK;
    
    IFC(SetValueByKnownIndex(KnownPropertyIndex::EventTrigger_RoutedEvent, static_cast<RoutedEvent*>(pValue)->m_strNativeRepresentation.Get()));

Cleanup:
    RRETURN(hr);
}
