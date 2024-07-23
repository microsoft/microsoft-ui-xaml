// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IFocusController.h"
#include <microsoft.ui.input.experimental.h>
#include "NavigationFocusEventArgs.h"

class __declspec(uuid("078216f2-d1b8-4395-bc8d-da6699be028a")) FocusController : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
    xaml_hosting::IFocusController>
{
    InspectableClass(L"Microsoft.UI.Xaml.Hosting.FocusController", TrustLevel::BaseTrust);
private:
    _Check_return_ HRESULT Init();
    _Check_return_ HRESULT DeInit();
    ~FocusController() override;

public:
    FocusController(_In_ wuc::ICoreComponentFocusable* pFocusable);
    FocusController(_In_ ixp::IInputFocusController* pFocusable);

    static _Check_return_ HRESULT Create(_In_ wuc::ICoreComponentFocusable* pFocusable, _Outptr_ FocusController** pValue);
    static _Check_return_ HRESULT Create(_In_ ixp::IInputFocusController* pFocusable, _Outptr_ FocusController** pValue);

    // IFocusController

    _Check_return_ IFACEMETHOD(get_HasFocus)(_Out_ boolean* pValue) override;

    _Check_return_ IFACEMETHOD(NavigateFocus)(
        _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request,
        _In_ FocusObserver* pFocusObserver,
        _Outptr_ xaml_hosting::IXamlSourceFocusNavigationResult** ppResult);

    _Check_return_ IFACEMETHOD(add_GotFocus)(
        _In_ xaml_hosting::FocusNavigatedEventHandler* handler,
        _Out_ EventRegistrationToken* token) override;
    _Check_return_ IFACEMETHOD(remove_GotFocus)(
        _In_ EventRegistrationToken token) override;

    _Check_return_ IFACEMETHOD(add_LosingFocus)(
        _In_ xaml_hosting::FocusDepartingEventHandler* handler,
        _Out_ EventRegistrationToken* token) override;
    _Check_return_ IFACEMETHOD(remove_LosingFocus)(
        _In_ EventRegistrationToken token) override;

    _Check_return_ IFACEMETHOD(DepartFocus)(
        _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request) override;

private:
    _Check_return_ HRESULT OnCoreInputGotFocus(_In_ IInspectable* pInspectable, _In_ wuc::ICoreWindowEventArgs* e);
    _Check_return_ HRESULT OnGotFocusCommon();

    _Check_return_ HRESULT FireGotFocus(_In_opt_ xaml_hosting::IXamlSourceFocusNavigationRequest* pCurrentRequest);

private:
    Microsoft::WRL::EventSource<xaml_hosting::FocusDepartingEventHandler> m_focusDepartingEvent;
    Microsoft::WRL::EventSource<xaml_hosting::FocusNavigatedEventHandler> m_gotFocusEvent;

    Microsoft::WRL::ComPtr<wuc::ICoreComponentFocusable>   m_coreComponentFocusable;
    Microsoft::WRL::ComPtr<ixp::IInputFocusController> m_inputObjectFocusable;

    EventRegistrationToken m_gotFocusToken = {};
    Microsoft::WRL::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> m_currentRequest;
};

