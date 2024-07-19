// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FocusController.h"
#include "FocusObserver.h"
#include "NavigationFocusEventArgs.h"
#include "NavigateFocusResult.h"
#include <WRLHelper.h>
#include <windows.ui.core.corewindow-defs.h>

using namespace xaml_hosting;
using FocusChangedEventHandler = wf::ITypedEventHandler<
        ixp::InputFocusController*, ixp::FocusChangedEventArgs*>;

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_XamlSourceFocusNavigationRequest();
}

using namespace DirectUI;

FocusController::FocusController(_In_ wuc::ICoreComponentFocusable* pFocusable)
{
    m_coreComponentFocusable = pFocusable;
}

FocusController::FocusController(_In_ ixp::IInputFocusController* pFocusable)
{
    m_inputObjectFocusable = pFocusable;
}

FocusController::~FocusController()
{
    // This can return E_CLOSED, which is safe to ignore.
    IGNOREHR(DeInit());
}

_Check_return_
HRESULT FocusController::Init()
{
    if (m_coreComponentFocusable != nullptr)
    {
        IFC_RETURN(m_coreComponentFocusable->add_GotFocus(
            Microsoft::WRL::Callback<wuc::GotFocusEventHandler>(
                this, &FocusController::OnCoreInputGotFocus).Get(),
            &m_gotFocusToken));
    }
    else
    {
        wrl::WeakRef wrThis;
        IFCFAILFAST(wrl::AsWeak(this, &wrThis));

        IFCFAILFAST(m_inputObjectFocusable->add_GotFocus(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputFocusController*, ixp::FocusChangedEventArgs*>>(
                [wrThis](ixp::IInputFocusController* focusController, ixp::IFocusChangedEventArgs* args) -> HRESULT
        {
            wrl::ComPtr<FocusController> spThis;
            if (SUCCEEDED(wrThis.As(&spThis)) && spThis)
            {
                args->put_Handled(true);
                return spThis->OnGotFocusCommon();
            }
            return S_OK;
        }).Get(), &m_gotFocusToken));
    }
    return S_OK;
}

_Check_return_
HRESULT FocusController::DeInit()
{
    if (m_gotFocusToken.value != 0)
    {
        if (m_coreComponentFocusable != nullptr)
        {
            IFC_RETURN(m_coreComponentFocusable->remove_GotFocus(m_gotFocusToken));
        }
        else
        {
            IFC_RETURN(m_inputObjectFocusable->remove_GotFocus(m_gotFocusToken));
        }
        m_gotFocusToken.value = 0;
    }
    return S_OK;
}


_Check_return_
HRESULT FocusController::Create(_In_ wuc::ICoreComponentFocusable* pFocusable, _Outptr_ FocusController** ppValue)
{
    wrl::ComPtr<FocusController> focusController = wrl::Make<FocusController>(pFocusable);
    IFC_RETURN(focusController->Init());
    IFC_RETURN(focusController.CopyTo(ppValue));
    return S_OK;
}


_Check_return_
HRESULT FocusController::Create(_In_ ixp::IInputFocusController* pFocusable, _Outptr_ FocusController** ppValue)
{
    wrl::ComPtr<FocusController> focusController = wrl::Make<FocusController>(pFocusable);
    IFC_RETURN(focusController->Init());
    IFC_RETURN(focusController.CopyTo(ppValue));
    return S_OK;
}


_Check_return_
HRESULT FocusController::NavigateFocus(
        _In_ IXamlSourceFocusNavigationRequest* pRequest,
        _In_ FocusObserver* pFocusObserver,
        _Outptr_ IXamlSourceFocusNavigationResult** ppResult)
{
    *ppResult = nullptr;

    m_currentRequest = pRequest;
    auto scopeExit = wil::scope_exit([&]
    {
        m_currentRequest = nullptr;
    });

    boolean isHandled = false;
    IFC_RETURN(pFocusObserver->ProcessNavigateFocusRequest(pRequest, &isHandled));

    wrl::ComPtr<NavigateFocusResult> result = wrl::Make<NavigateFocusResult>(!!isHandled);
    IFC_RETURN(result.CopyTo(ppResult));

    return S_OK;
}

_Check_return_
HRESULT FocusController::add_LosingFocus(
        _In_ FocusDepartingEventHandler* handler,
        _Out_ EventRegistrationToken* token)
{
    IFC_RETURN(m_focusDepartingEvent.Add(handler, token));
    return S_OK;
}

_Check_return_
HRESULT FocusController::remove_LosingFocus(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_focusDepartingEvent.Remove(token));
    return S_OK;
}

_Check_return_
HRESULT FocusController::add_GotFocus(
    _In_ FocusNavigatedEventHandler* handler,
    _Out_ EventRegistrationToken* token)
{
    IFC_RETURN(m_gotFocusEvent.Add(handler, token));
    return S_OK;
}

_Check_return_
HRESULT FocusController::remove_GotFocus(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_gotFocusEvent.Remove(token));
    return S_OK;
}

_Check_return_
HRESULT FocusController::DepartFocus(
    _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request)
{
    wrl::ComPtr<xaml_hosting::NavigationLosingFocusEventArgs> spLosingFocusRequest = wrl::Make<xaml_hosting::NavigationLosingFocusEventArgs>(request);
    wrl::ComPtr<IDesktopWindowXamlSourceTakeFocusRequestedEventArgs> departingEventArgs;
    IFC_RETURN(spLosingFocusRequest.As(&departingEventArgs));
    
    wrl::ComPtr<FocusController> spThis(this);
    wrl::ComPtr<IInspectable> spThat;
    IFC_RETURN(spThis.As(&spThat));
    
    IFC_RETURN(m_focusDepartingEvent.InvokeAll(spThat.Get(), departingEventArgs.Get()));

    // Trigger IXP focus navigation
    ixp::FocusNavigationReason ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_First;
    xaml_hosting::XamlSourceFocusNavigationReason xamlReason;
    IFC_RETURN(request->get_Reason(&xamlReason));

    switch(xamlReason)
    {
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Programmatic:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Programmatic;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Restore:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Restore;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_First:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_First;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Last:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Last;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Up:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Up;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Down:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Down;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Left:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Left;
            break;
        case xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Right:
            ixpReason = ixp::FocusNavigationReason::FocusNavigationReason_Right;
            break;
    }

    ABI::Windows::Foundation::Rect hintRect;
    IFC_RETURN(request->get_HintRect(&hintRect));
    GUID correlationId;
    IFC_RETURN(request->get_CorrelationId(&correlationId));
    wrl::ComPtr<ixp::IFocusNavigationRequest> ixpRequest;

    wrl::ComPtr<ixp::IFocusNavigationRequestStatics> focusNavigationRequestStatics;
    wf::GetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_FocusNavigationRequest).Get(), &focusNavigationRequestStatics);
    IFCFAILFAST(focusNavigationRequestStatics->CreateWithHintRectAndId(ixpReason, hintRect, correlationId, &ixpRequest));

    wrl::ComPtr<ixp::IInputFocusController2> inputFocusController2;
    IFC_RETURN(m_inputObjectFocusable.As(&inputFocusController2));
    ixp::FocusNavigationResult result;
    IFC_RETURN(inputFocusController2->DepartFocus(ixpRequest.Get(), &result));

    // WinUI may wish to respond to the result (Moved/NotMoved/NoFocusableElements) here in some way

    return S_OK;
}

template<typename TDelegateInterface, typename TCallback>
::Microsoft::WRL::ComPtr<TDelegateInterface> MakeAgileCallback(TCallback callback) throw()
{
    return ::Microsoft::WRL::Callback<
        ::Microsoft::WRL::Implements<
        ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>,
        TDelegateInterface,
        ::Microsoft::WRL::FtmBase>>(callback);
}

_Check_return_
HRESULT FocusController::OnCoreInputGotFocus(_In_ IInspectable* pInspectable, _In_ wuc::ICoreWindowEventArgs* e)
{
    return OnGotFocusCommon();
}

_Check_return_
HRESULT FocusController::OnGotFocusCommon()
{
    wrl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
        &spDispatcherQueueStatics));

    wrl::ComPtr<msy::IDispatcherQueue> spDispatcherQueue;
    IFC_RETURN(spDispatcherQueueStatics->GetForCurrentThread(&spDispatcherQueue));

    wrl::ComPtr<FocusController> spThis(this);
    auto currentRequest = m_currentRequest;
    m_currentRequest = nullptr;
    auto callback = MakeAgileCallback<msy::IDispatcherQueueHandler>(
        [spThis, currentRequest]() mutable
        {
            IFC_RETURN(spThis->FireGotFocus(currentRequest.Get()));
            return S_OK;
        });
    boolean enqueued = false;
    IFC_RETURN(spDispatcherQueue->TryEnqueue(callback.Get(), &enqueued));
    return S_OK;
}

_Check_return_
HRESULT FocusController::get_HasFocus(_Out_ boolean* pValue)
{
    if (m_coreComponentFocusable != nullptr)
    {
        IFC_RETURN(m_coreComponentFocusable->get_HasFocus(pValue));
    }
    else
    {
        IFC_RETURN(m_inputObjectFocusable->get_HasFocus(pValue));
    }
    return S_OK;
}

_Check_return_
HRESULT FocusController::FireGotFocus(_In_opt_ xaml_hosting::IXamlSourceFocusNavigationRequest* pCurrentRequest)
{
    wrl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> spRequest(pCurrentRequest);
    if (spRequest.Get() == nullptr)
    {
        wrl::ComPtr<IActivationFactory> activationFactory(CreateActivationFactory_XamlSourceFocusNavigationRequest());
        wrl::ComPtr<IXamlSourceFocusNavigationRequestFactory> requestFactory;
        IFC_RETURN(activationFactory.As(&requestFactory));
        IFC_RETURN(requestFactory->CreateInstance(XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Restore, &spRequest));
    }

    auto spArgs = wrl::Make<xaml_hosting::NavigationGotFocusEventArgs>(spRequest.Get());
    wrl::ComPtr<IDesktopWindowXamlSourceGotFocusEventArgs> gotFocusEventArgs;
    IFC_RETURN(spArgs.As(&gotFocusEventArgs));

    wrl::ComPtr<FocusController> spThis(this);
    wrl::ComPtr<IInspectable> spThat;
    IFC_RETURN(spThis.As(&spThat));
    IFC_RETURN(m_gotFocusEvent.InvokeAll(spThat.Get(), gotFocusEventArgs.Get()));

    return S_OK;
}

