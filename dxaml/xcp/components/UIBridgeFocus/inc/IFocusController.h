// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class FocusObserver;
XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Hosting {

typedef
    wf::ITypedEventHandler<
        IInspectable*,
        IInspectable*>
    FocusDepartingEventHandler;

typedef
    wf::ITypedEventHandler<
        IInspectable*,
        IInspectable*>
    FocusNavigatedEventHandler;

MIDL_INTERFACE("e2cc466e-22dc-4774-9337-ee839aaa0969")
IFocusController : public IInspectable
{
public:

    IFACEMETHOD(get_HasFocus)(_Out_ boolean* pValue) = 0;

    IFACEMETHOD(NavigateFocus)(
        _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request,
        _In_ FocusObserver* pFocusObserver,
        _Outptr_ xaml_hosting::IXamlSourceFocusNavigationResult** ppResult) = 0;

    IFACEMETHOD(add_GotFocus)(
        _In_ FocusNavigatedEventHandler* handler,
        _Out_ EventRegistrationToken* token) = 0;
    IFACEMETHOD(remove_GotFocus)(
        _In_ EventRegistrationToken token) = 0;

    IFACEMETHOD(add_LosingFocus)(
        _In_ FocusDepartingEventHandler* handler,
        _Out_ EventRegistrationToken* token) = 0;
    IFACEMETHOD(remove_LosingFocus)(
        _In_ EventRegistrationToken token) = 0;
        
    IFACEMETHOD(DepartFocus)(
        _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request) = 0;
};

} } } } XAML_ABI_NAMESPACE_END

