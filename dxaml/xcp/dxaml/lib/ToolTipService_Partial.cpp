// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToolTipService.g.h"
#include "PropertyMetadata.g.h"
#include "ToolTip.g.h"
#include "DispatcherTimer.g.h"
#include "Window.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "PointerEventArgs.h"
#include "RoutedEventArgs.h"
#include "focusmgr.h"
#include "InputServices.h"
#include "XboxUtility.h"
#include <CStaticLock.h>
#include <algorithm>
#include "UIElement.g.h"
#include "TextElement.g.h"
#include <XamlOneCoreTransforms.h>
#include <powrprof.h>
#include <WRLHelper.h>
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ToolTipServiceMetadata::ToolTipServiceMetadata()
{
    m_nestedOwners = NULL;

    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS notification = {};
    notification.Callback = DisplayStateNotification;
    notification.Context = this;
    IGNOREHR(HRESULT_FROM_WIN32(PowerSettingRegisterNotification(
        &GUID_SESSION_DISPLAY_STATUS, DEVICE_NOTIFY_CALLBACK,
        &notification, &m_displayStateHandle)));
}

_Check_return_ HRESULT  ToolTipServiceMetadata::EnsureNestedOwnersInstance()
{
    if (!m_nestedOwners)
    {
        m_nestedOwners = new std::list<ctl::WeakRefPtr>();
    }
    return m_nestedOwners != NULL ? S_OK : E_OUTOFMEMORY;
}

_Check_return_ HRESULT  ToolTipServiceMetadata::DeleteElementFromNestedOwners(_Inout_ std::list<ctl::WeakRefPtr>::iterator &it)
{
    HRESULT hr = S_OK;
    if (m_isErasingNestedOwners)
    {
        // nested erase is detected and we don't expect it. so throw E_UNEXPECTED exception
        IFCEXPECT(false);
    }
    else
    {
        m_isErasingNestedOwners = true;
        it = m_nestedOwners->erase(it);
        m_isErasingNestedOwners = false;
    }

Cleanup:
    return hr;
}

ToolTipServiceMetadata::~ToolTipServiceMetadata()
{
    if (m_displayStateHandle != nullptr)
    {
        PowerSettingUnregisterNotification(m_displayStateHandle);
        m_displayStateHandle = nullptr;
    }

    m_tpOwner.Clear();
    m_tpContainer.Clear();
    m_tpCurrentPopup.Clear();
    m_tpOpenTimer.Clear();
    m_tpCloseTimer.Clear();
    m_tpSafeZoneCheckTimer.Clear();

    // avoid nested destroy if deleting m_nestedOwners trig another ToolTipService call.
    std::list<ctl::WeakRefPtr>* nestedOwners = m_nestedOwners;
    m_nestedOwners = nullptr;
    delete nestedOwners;
}

_Use_decl_annotations_
ULONG CALLBACK ToolTipServiceMetadata::DisplayStateNotification(PVOID pvContext, ULONG, PVOID pvSetting)
{
    ToolTipServiceMetadata* pThis = reinterpret_cast<ToolTipServiceMetadata*>(pvContext);

    POWERBROADCAST_SETTING* powerSetting = reinterpret_cast<POWERBROADCAST_SETTING*>(pvSetting);
    DWORD* pValue = reinterpret_cast<DWORD*>(powerSetting->Data);

    // Non-zero value indicates the display is on.
    pThis->m_displayOn = (*pValue != 0);

    return S_OK;
}

BOOLEAN ToolTipService::s_bOpeningAutomaticToolTip = FALSE;
AutomaticToolTipInputMode ToolTipService::s_lastEnterInputMode = AutomaticToolTipInputMode::None;
wf::Point ToolTipService::s_lastPointerEnteredPoint = {};
POINT ToolTipService::s_pointerPointWhenSafeZoneTimerStart = {};
INT64 ToolTipService::s_lastToolTipOpenedTime = 0;


_Check_return_
HRESULT
ToolTipService::RegisterToolTip(
    _In_ xaml::IDependencyObject* pOwner,
    _In_ xaml::IFrameworkElement* pContainer,
    _In_ IInspectable* pToolTipAsIInspectable,
    _In_ const bool isKeyboardAcceleratorToolTip)
{
    ASSERT(pOwner != NULL, L"ToolTip must have an owner");
    ASSERT(pContainer != NULL, L"ToolTip must have an container");
    ASSERT(pToolTipAsIInspectable != NULL, L"ToolTip can not be null");

    ctl::ComPtr<IToolTip> spToolTipObject;
    ctl::ComPtr<IToolTip> spIToolTip;
    ctl::ComPtr<IPointerEventHandler> spPointerEnteredHandler;
    ctl::ComPtr<IPointerEventHandler> spPointerExitedHandler;
    ctl::ComPtr<IPointerEventHandler> spPointerCaptureLostHandler;
    ctl::ComPtr<IPointerEventHandler> spPointerCanceledHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spGotFocusHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLostFocusHandler;
    bool inputEventsAlreadyHookedUp = false;

    ToolTip* pToolTipNoRef;

    if (isKeyboardAcceleratorToolTip)
    {
        IFC_RETURN(ToolTipServiceFactory::GetKeyboardAcceleratorToolTipObjectStatic(pOwner, &spToolTipObject));
    }
    else
    {
        IFC_RETURN(ToolTipServiceFactory::GetToolTipObjectStatic(pOwner, &spToolTipObject));
    }

    if (spToolTipObject)
    {
        inputEventsAlreadyHookedUp = spToolTipObject.Cast<ToolTip>()->m_bInputEventsHookedUp;
    }

    // Set the tooltip before applying the delegates, otherwise the owner
    // will try to call into the tool tip services.
    IFC_RETURN(ConvertToToolTip(pToolTipAsIInspectable, &spIToolTip));

    pToolTipNoRef = spIToolTip.Cast<ToolTip>();
    IFC_RETURN(pToolTipNoRef->SetOwner(pOwner));
    IFC_RETURN(pToolTipNoRef->SetContainer(pContainer));

    if (isKeyboardAcceleratorToolTip)
    {
        IFC_RETURN(ToolTipServiceFactory::SetKeyboardAcceleratorToolTipObjectStatic(pOwner, pToolTipNoRef));
    }
    else
    {
        IFC_RETURN(ToolTipServiceFactory::SetToolTipObjectStatic(pOwner, pToolTipNoRef));
    }

    // If the owner is also the container, then we'll want to attach pointer events,
    // since nothing will be already listening to pointer events for us.
    if (ctl::are_equal(pOwner, pContainer) && !inputEventsAlreadyHookedUp)
    {
        ctl::ComPtr<IFrameworkElement> ownerAsFE;

        IFC_RETURN(ctl::do_query_interface(ownerAsFE, pOwner));

        FrameworkElement* pOwnerAsFENoRef = static_cast<FrameworkElement*>(ownerAsFE.Get());

        spPointerEnteredHandler.Attach(
            new StaticMemberEventHandler<
                IPointerEventHandler,
                IInspectable,
                IPointerRoutedEventArgs>(&ToolTipService::OnOwnerPointerEntered));
        IFC_RETURN(pOwnerAsFENoRef->add_PointerEntered(spPointerEnteredHandler.Get(), &pToolTipNoRef->m_ownerPointerEnteredToken));

        spPointerExitedHandler.Attach(
            new StaticMemberEventHandler<
                IPointerEventHandler,
                IInspectable,
                IPointerRoutedEventArgs>(&ToolTipService::OnOwnerPointerExitedOrLostOrCanceled));
        IFC_RETURN(pOwnerAsFENoRef->add_PointerExited(spPointerExitedHandler.Get(), &pToolTipNoRef->m_ownerPointerExitedToken));

        spPointerCaptureLostHandler.Attach(
            new StaticMemberEventHandler<
                IPointerEventHandler,
                IInspectable,
                IPointerRoutedEventArgs>(&ToolTipService::OnOwnerPointerExitedOrLostOrCanceled));
        IFC_RETURN(pOwnerAsFENoRef->add_PointerCaptureLost(spPointerCaptureLostHandler.Get(), &pToolTipNoRef->m_ownerPointerCaptureLostToken));

        spPointerCanceledHandler.Attach(
            new StaticMemberEventHandler<
                IPointerEventHandler,
                IInspectable,
                IPointerRoutedEventArgs>(&ToolTipService::OnOwnerPointerExitedOrLostOrCanceled));
        IFC_RETURN(pOwnerAsFENoRef->add_PointerCanceled(spPointerCanceledHandler.Get(), &pToolTipNoRef->m_ownerPointerCanceledToken));

        spGotFocusHandler.Attach(
            new StaticMemberEventHandler<
                xaml::IRoutedEventHandler,
                IInspectable,
                IRoutedEventArgs>(&ToolTipService::OnOwnerGotFocus));
        IFC_RETURN(pOwnerAsFENoRef->add_GotFocus(spGotFocusHandler.Get(), &pToolTipNoRef->m_ownerGotFocusToken));

        spLostFocusHandler.Attach(
            new StaticMemberEventHandler<
                xaml::IRoutedEventHandler,
                IInspectable,
                IRoutedEventArgs>(&ToolTipService::OnOwnerLostFocus));
        IFC_RETURN(pOwnerAsFENoRef->add_LostFocus(spLostFocusHandler.Get(), &pToolTipNoRef->m_ownerLostFocusToken));

        pToolTipNoRef->m_bInputEventsHookedUp = TRUE;
    }

    return S_OK;
}

_Check_return_
HRESULT
ToolTipService::RegisterToolTipFromCore(
    _In_ CDependencyObject* owner,
    _In_ CFrameworkElement* container)
{
    ctl::ComPtr<DependencyObject> ownerPeer;
    ctl::ComPtr<DependencyObject> containerPeer;
    CValue toolTipValue;
    ctl::ComPtr<IInspectable> toolTip;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(owner, &ownerPeer));

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(container, &containerPeer));
    IFC_RETURN(owner->GetValue(owner->GetPropertyByIndexInline(KnownPropertyIndex::ToolTipService_ToolTip), &toolTipValue));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(&toolTipValue, /* pTargetType */ nullptr, &toolTip));

    if (ownerPeer && containerPeer && toolTip)
    {
        ctl::ComPtr<xaml::IFrameworkElement> containerPeerAsFE;

        IFC_RETURN(containerPeer.As(&containerPeerAsFE));
        IFC_RETURN(RegisterToolTip(ownerPeer.Get(), containerPeerAsFE.Get(), toolTip.Get(), false /*isKeyboardAcceleratorToolTip*/));
    }

    return S_OK;
}

_Check_return_
HRESULT
ToolTipService::UnregisterToolTip(
    _In_ xaml::IDependencyObject* pOwner,
    _In_ xaml::IFrameworkElement* pContainer,
    _In_ const bool isKeyboardAcceleratorToolTip)
{
    ASSERT(pOwner, L"owner element is required");
    ASSERT(pContainer, L"container element is required");

    ctl::ComPtr<DependencyObject> spOwnerConcrete = static_cast<DependencyObject*>(pOwner);
    ctl::ComPtr<FrameworkElement> spContainerConcrete = static_cast<FrameworkElement*>(pContainer);
    ctl::ComPtr<IToolTip> spToolTipObject;
    ctl::ComPtr<ToolTip> spToolTipObjectConcrete;

    if (isKeyboardAcceleratorToolTip)
    {
        IFC_RETURN(ToolTipServiceFactory::GetKeyboardAcceleratorToolTipObjectStatic(pOwner, &spToolTipObject));
    }
    else
    {
        IFC_RETURN(ToolTipServiceFactory::GetToolTipObjectStatic(pOwner, &spToolTipObject));
    }

    spToolTipObjectConcrete = spToolTipObject.Cast<ToolTip>();
    ASSERT(spToolTipObjectConcrete);

    if (spToolTipObjectConcrete->m_bInputEventsHookedUp)
    {
        IFC_RETURN(spContainerConcrete->remove_PointerEntered(spToolTipObjectConcrete->m_ownerPointerEnteredToken));
        IFC_RETURN(spContainerConcrete->remove_PointerExited(spToolTipObjectConcrete->m_ownerPointerExitedToken));
        IFC_RETURN(spContainerConcrete->remove_PointerCaptureLost(spToolTipObjectConcrete->m_ownerPointerCaptureLostToken));
        IFC_RETURN(spContainerConcrete->remove_PointerCanceled(spToolTipObjectConcrete->m_ownerPointerCanceledToken));
        IFC_RETURN(spContainerConcrete->remove_GotFocus(spToolTipObjectConcrete->m_ownerGotFocusToken));
        IFC_RETURN(spContainerConcrete->remove_LostFocus(spToolTipObjectConcrete->m_ownerLostFocusToken));
    }

    IFC_RETURN(spToolTipObjectConcrete->SetOwner(NULL));
    IFC_RETURN(spToolTipObjectConcrete->SetContainer(NULL));

    // Close the ToolTip if it's open, or cancel it from opening if it's in the process of opening
    IFC_RETURN(OnOwnerLeaveInternal(pOwner));

    KnownPropertyIndex toolTipPropertyIndex = isKeyboardAcceleratorToolTip ? KnownPropertyIndex::ToolTipService_KeyboardAcceleratorToolTipObject :
                                                                             KnownPropertyIndex::ToolTipService_ToolTipObject;
    IFC_RETURN(spOwnerConcrete->ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(toolTipPropertyIndex)));

    return S_OK;
}

void ToolTipService::GetActualToolTipObjectStatic(
    _In_ xaml::IDependencyObject* pElement,
    _Outptr_result_maybenull_ xaml_controls::IToolTip** ppValue)
{
    // Try to get the actual public tooltip object
    IFCFAILFAST(DependencyObject::GetAttachedValueByKnownIndex(
        static_cast<DirectUI::DependencyObject*>(pElement),
        KnownPropertyIndex::ToolTipService_ToolTipObject,
        ppValue));

    // If public tooltip doesn't exist, then look for keyboard accelerator tooltip.
    if (*ppValue == nullptr)
    {
        IFCFAILFAST(DependencyObject::GetAttachedValueByKnownIndex(
            static_cast<DirectUI::DependencyObject*>(pElement),
            KnownPropertyIndex::ToolTipService_KeyboardAcceleratorToolTipObject,
            ppValue));
    }
}

_Check_return_
HRESULT
ToolTipService::UnregisterToolTipFromCore(
    _In_ CDependencyObject* owner,
    _In_ CFrameworkElement* container)
{
    ctl::ComPtr<DependencyObject> ownerPeer;
    ctl::ComPtr<DependencyObject> containerPeer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(owner, &ownerPeer));
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(container, &containerPeer));

    if (ownerPeer && containerPeer)
    {
        ctl::ComPtr<xaml::IFrameworkElement> containerPeerAsFE;

        IFC_RETURN(containerPeer.As(&containerPeerAsFE));
        IFC_RETURN(UnregisterToolTip(ownerPeer.Get(), containerPeerAsFE.Get(),false  /*isKeyboardAcceleratorToolTip*/));
    }

    return S_OK;
}

// ToolTip will not be closed until Pointer moves out of safe zone
// A timer is started when ToolTip is open, and then check if pointer is in the safe zone periodically.
// Because Keyboard also opens ToolTip, s_pointerPointWhenSafeZoneTimerStart is used to determine if pointer is moved or not after.
_Check_return_ HRESULT
ToolTipService::OnSafeZoneCheck()
{

    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;
    IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    // When screen is off, stop the scheduled timer to avoid battery drain like BUG 1735672 and BUG 1735672
    if (!pToolTipServiceMetadataNoRef->m_displayOn && pToolTipServiceMetadataNoRef->m_tpSafeZoneCheckTimer)
    {
        pToolTipServiceMetadataNoRef->m_tpSafeZoneCheckTimer->Stop();
        IFC_RETURN(ToolTipService::CancelAutomaticToolTip());
        return S_OK;
    }

    if (auto tooltip = static_cast<ToolTip*>(pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get()))
    {
        // We don't not use PointerPointStatic get_Position here, since it is based on the current window.
        // This prevents us from getting the right position in out of window ToolTip cases.
        // We call GetCursorPos() to get the screen based position instead of each window based position.
        POINT pointerPosition;
        if (GetCursorPos(&pointerPosition))
        {
            auto startPosition = ToolTipService::s_pointerPointWhenSafeZoneTimerStart;
            if (fabs(startPosition.x - pointerPosition.x) < 0.1 && fabs(startPosition.y - pointerPosition.y) < 0.1)
            {
                // Pointer not moved, avoid to dismiss keyboard opened ToolTip
                return S_OK;
            }
            IFC_RETURN(tooltip->HandlePointInSafeZone(pointerPosition));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
ToolTipService::StartSafeZoneCheckTimer(
    _In_ ToolTipServiceMetadata* pToolTipServiceMetadataNoRef)
{
    if (!pToolTipServiceMetadataNoRef->m_tpSafeZoneCheckTimer.Get())
    {
        wf::TimeSpan interval;
        interval.Duration = s_safeZoneCheckTimerDuration;

        ctl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;
        ctl::ComPtr<msy::IDispatcherQueueTimer> dispatcherQueueTimer;

        IFC_RETURN(GetDispatcherQueueForCurrentThread(&dispatcherQueue));

        IFC_RETURN(dispatcherQueue->CreateTimer(&dispatcherQueueTimer));

        EventRegistrationToken token;

        IFC_RETURN(dispatcherQueueTimer->add_Tick(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<msy::DispatcherQueueTimer*, IInspectable*>>(
                [](msy::IDispatcherQueueTimer* sender, IInspectable* args) -> HRESULT
                {
                    IFC_RETURN(OnSafeZoneCheck());
                    return S_OK;
                }).Get(), &token));

        IFC_RETURN(dispatcherQueueTimer->put_Interval(interval));

        pToolTipServiceMetadataNoRef->SetSafeZoneTimer(dispatcherQueueTimer.Get());
    }

    IFC_RETURN(pToolTipServiceMetadataNoRef->m_tpSafeZoneCheckTimer->Start());

    GetCursorPos(&ToolTipService::s_pointerPointWhenSafeZoneTimerStart);

    return S_OK;
}

_Check_return_
HRESULT
ToolTipService::OpenAutomaticToolTip(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IToolTip> spToolTipObject;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spCloseTimerTickEventHandler;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;
    ULONG showDurationSeconds = 0;
    wf::TimeSpan showDurationTimeSpan = {};

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_tpOpenTimer)
    {
        IFC(pToolTipServiceMetadataNoRef->m_tpOpenTimer->Stop());
    }

    ASSERT(pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get() == NULL);

    // ToolTipService does not open ToolTips automatically on Xbox.
    if (XboxUtility::IsOnXbox())
    {
        goto Cleanup;
    }

    if (!::SystemParametersInfo(SPI_GETMESSAGEDURATION, 0, &showDurationSeconds, 0))
    {
#ifdef DBG
        Trace(L"Failed to fetch SPI_GETMESSAGEDURATION. Falling back to DEFAULT_SHOW_DURATION_SECONDS.");
        WCHAR szTrace[32];
        IFCEXPECT(swprintf_s(szTrace, 32, L"Last Error: 0x%08X", HRESULT_FROM_WIN32(GetLastError())));
        Trace(szTrace);
#endif
        showDurationSeconds = DEFAULT_SHOW_DURATION_SECONDS;
    }
    showDurationTimeSpan.Duration = (showDurationSeconds * 1000) * TICKS_PER_MILLISECOND;

    {
        CStaticLock lock;

        // If m_tpOwner or m_tpContainer is null, we received a Tick when the timer was already stopped.
        // Can't just check if timer was stopped because we sometimes call this directly.
        if (!pToolTipServiceMetadataNoRef->m_tpOwner || !pToolTipServiceMetadataNoRef->m_tpContainer)
        {
            goto Cleanup;
        }

        if (!static_cast<FrameworkElement*>(pToolTipServiceMetadataNoRef->m_tpContainer.Get())->IsInLiveTree())
        {
            goto Cleanup;
        }

        ToolTipService::GetActualToolTipObjectStatic(pToolTipServiceMetadataNoRef->m_tpOwner.Get(), &spToolTipObject);
        ASSERT(spToolTipObject.Get(), L"ToolTip must have been registered");
        IFCPTR(spToolTipObject);

        spToolTipObject.Cast<ToolTip>()->m_inputMode = s_lastEnterInputMode;

        s_bOpeningAutomaticToolTip = TRUE;
        hr = spToolTipObject->put_IsOpen(TRUE);
        s_bOpeningAutomaticToolTip = FALSE;
        IFC(hr);
    }

    IFC(StartSafeZoneCheckTimer(pToolTipServiceMetadataNoRef));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::CloseAutomaticToolTip(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_tpCloseTimer.Get() != nullptr)
    {
        IFC(pToolTipServiceMetadataNoRef->m_tpCloseTimer->Stop());
    }

    if (pToolTipServiceMetadataNoRef->m_tpSafeZoneCheckTimer)
    {
        IFC(pToolTipServiceMetadataNoRef->m_tpSafeZoneCheckTimer->Stop());
    }

    if (pToolTipServiceMetadataNoRef->m_tpCurrentToolTip)
    {
        ASSERT(pToolTipServiceMetadataNoRef->m_tpCurrentPopup != nullptr);
        pToolTipServiceMetadataNoRef->m_tpCurrentToolTip->put_IsOpen(FALSE);

        s_lastToolTipOpenedTime = GetTickCount();
    }

    if (pToolTipServiceMetadataNoRef->m_lastToolTipOwnerInSafeZone)
    {
        ctl::ComPtr<xaml::IDependencyObject> owner;
        IFC(pToolTipServiceMetadataNoRef->m_lastToolTipOwnerInSafeZone.As(&owner));
        pToolTipServiceMetadataNoRef->m_lastToolTipOwnerInSafeZone = nullptr;
        IFC(ToolTipService::RemoveFromNestedOwners(owner.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::ConvertToToolTip(
    _In_ IInspectable* pObjectIn,
    _Outptr_ IToolTip** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IToolTip> spIToolTip;
    ctl::ComPtr<xaml::IDependencyObject> spObjectInParent;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    spIToolTip = ctl::query_interface_cast<IToolTip>(pObjectIn);
    if (!spIToolTip)
    {
        ctl::ComPtr<IFrameworkElement> spObjectInAsIFE;
        ctl::ComPtr<ToolTip> spNewToolTip;

        if (SUCCEEDED(ctl::do_query_interface(spObjectInAsIFE, pObjectIn)))
        {
            IFC(spObjectInAsIFE->get_Parent(&spObjectInParent));
            if (spObjectInParent)
            {
                ctl::ComPtr<IToolTip> spObjectInParentAsIToolTip;
                spObjectInParentAsIToolTip = spObjectInParent.AsOrNull<IToolTip>();
                if (spObjectInParentAsIToolTip)
                {
                    IFC(spObjectInParentAsIToolTip.MoveTo(ppReturnValue));
                    goto Cleanup;
                }
            }
        }

        IFC(ctl::make<ToolTip>(&spNewToolTip));

        IFC(spNewToolTip->put_Content(pObjectIn));
        spIToolTip = spNewToolTip;
    }

    IFC(spIToolTip.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::OnOwnerEnterInternal(
    _In_ IInspectable* pSender,
    _In_ IInspectable* pSource,
    _In_ AutomaticToolTipInputMode mode)
{
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;

    EventRegistrationToken openTimerTickToken;
    wf::TimeSpan initialShowDelayTimeSpan = {};
    bool isSenderLastEnterSource = false;

    IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    IFC_RETURN(ctl::are_equal(pToolTipServiceMetadataNoRef->m_tpLastEnterSource.Get(), pSource, &isSenderLastEnterSource));
    if (pToolTipServiceMetadataNoRef->m_tpLastEnterSource &&
        isSenderLastEnterSource)
    {
        // ToolTipService had processed this event once before, when it fired on the child
        // skip it now
        return S_OK;
    }

    if (pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get() != nullptr)
    {
        ctl::ComPtr<IToolTip> spToolTipObject;
        ctl::ComPtr<xaml::IDependencyObject> spSenderAsIDO;

        IFC_RETURN(ctl::do_query_interface(spSenderAsIDO, pSender));
        ToolTipService::GetActualToolTipObjectStatic(spSenderAsIDO.Get(), &spToolTipObject);

        if (spToolTipObject.Get() != pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get())
        {
            // first close the previous ToolTip if entering nested elements with tooltips
            IFC_RETURN(CloseAutomaticToolTip(nullptr, nullptr));
        }
        else
        {
            // reentering the same element
            return S_OK;
        }
    }

    {
        CStaticLock lock;
        ctl::ComPtr<xaml::IDependencyObject> spSenderAsDO;
        ctl::ComPtr<xaml::IFrameworkElement> spContainer;

        IFC_RETURN(ctl::do_query_interface(spSenderAsDO, pSender));
        IFC_RETURN(GetContainerFromOwner(spSenderAsDO.Get(), &spContainer));

        pToolTipServiceMetadataNoRef->SetOwner(spSenderAsDO.Get());
        pToolTipServiceMetadataNoRef->SetContainer(spContainer.Get());
        pToolTipServiceMetadataNoRef->SetLastEnterSource(pSource);
    }

    ASSERT(pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get() == nullptr);

    // open the ToolTip after the InitialShowDelay interval expires
    if (pToolTipServiceMetadataNoRef->m_tpOpenTimer.Get() == nullptr)
    {
        ctl::ComPtr<DispatcherTimer> spNewDispatcherTimer;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spOpenTimerTickEventHandler;

        IFC_RETURN(ctl::make<DispatcherTimer>(&spNewDispatcherTimer));
        pToolTipServiceMetadataNoRef->SetOpenTimer(spNewDispatcherTimer.Get());

        spOpenTimerTickEventHandler.Attach(
            new StaticMemberEventHandler<
                wf::IEventHandler<IInspectable*>,
                IInspectable,
                IInspectable>(&ToolTipService::OpenAutomaticToolTip));
        IFC_RETURN(pToolTipServiceMetadataNoRef->m_tpOpenTimer->add_Tick(spOpenTimerTickEventHandler.Get(), &openTimerTickToken));
    }

    s_lastEnterInputMode = mode;

    BOOLEAN useReshowTimer = GetTickCount() - s_lastToolTipOpenedTime < BETWEEN_SHOW_DELAY_MS;
    IFC_RETURN(GetInitialShowDelay(mode, useReshowTimer, &initialShowDelayTimeSpan));
    IFC_RETURN(pToolTipServiceMetadataNoRef->m_tpOpenTimer->put_Interval(initialShowDelayTimeSpan));
    IFC_RETURN(pToolTipServiceMetadataNoRef->m_tpOpenTimer->Start());

    return S_OK;
}

// Used to handle MouseLeave on a ToolTip's owner FrameworkElement.
_Check_return_
HRESULT
ToolTipService::OnOwnerLeaveInternal(
    _In_ IInspectable* pSender)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spSenderAsDO;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;
    bool areEqual = false;

    IFC(ctl::do_query_interface(spSenderAsDO, pSender));
    ASSERT(spSenderAsDO.Get());
    IFCPTR(spSenderAsDO);

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    IFC(ctl::are_equal(spSenderAsDO.Get(), pToolTipServiceMetadataNoRef->m_tpOwner.Get(), &areEqual));
    if (areEqual)
    {
        // No need to call RemoveFromNestedOwners() since CancelAutomaticToolTip calls it.
        IFC(CancelAutomaticToolTip());
    }
    else
    {
        IFC(RemoveFromNestedOwners(spSenderAsDO.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// If there is an automatic ToolTip in the process of opening, stop it from opening.
// If one is already open, close it.
// Clear any state associated with the current automatic ToolTip and its owner.
_Check_return_ HRESULT
ToolTipService::CancelAutomaticToolTip()
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (!pToolTipServiceMetadataNoRef->m_tpCurrentToolTip)
    {
        // ToolTip had not been opened yet

        // There are some strange cases where the owner will get a leave but not an enter.
        // The _openTimer is initialized in the enter, so we need to make sure it is there
        // before we try to stop it.

        if (pToolTipServiceMetadataNoRef->m_tpOpenTimer)
        {
            IFC(pToolTipServiceMetadataNoRef->m_tpOpenTimer->Stop());
        }
    }
    else
    {
        IFC(CloseAutomaticToolTip(NULL, NULL));
    }

    {
        CStaticLock lock;

        if (pToolTipServiceMetadataNoRef->m_tpOwner)
        {
            IFC(RemoveFromNestedOwners(pToolTipServiceMetadataNoRef->m_tpOwner.Get()));
            pToolTipServiceMetadataNoRef->m_tpOwner.Clear();
        }
        pToolTipServiceMetadataNoRef->m_tpContainer.Clear();
        pToolTipServiceMetadataNoRef->m_tpLastEnterSource.Clear();
    }

Cleanup:
    RRETURN(hr);
}

/* static */ _Check_return_ HRESULT ToolTipService::EnsureHandlersAttachedToRootElement(_In_ VisualTree* visualTree)
{
    ToolTipServiceMetadata* toolTipServiceMetadataNoRef = nullptr;

    CUIElement* coreRootElement = visualTree->GetPublicRootVisual();

    DependencyObject* rootElementDO = coreRootElement->GetDXamlPeer();
    ASSERT(ctl::is<IUIElement>(rootElementDO));
    IUIElement* rootElement = static_cast<DirectUI::UIElement*>(rootElementDO);

    IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(toolTipServiceMetadataNoRef));

    if (std::find(
        toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.begin(),
        toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.end(),
        rootElement) == toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.end())
    {
        // These event handlers are never detached, but they will not leak the root element. They take references on the core
        // CUIElement. The DXaml layer's peer DirectUI::UIElement has a separate ref count, and when the DXaml peer is released,
        // it calls into the core to detach all event handlers. See DirectUI::DependencyObject::DisconnectFrameworkPeerCore's
        // loop over its m_pEventMap.

        ctl::ComPtr<IPointerEventHandler> spRootVisualPointerMovedHandler;
        EventRegistrationToken pointerMoveToken;
        spRootVisualPointerMovedHandler.Attach(
            new StaticMemberEventHandler<
                IPointerEventHandler,
                IInspectable,
                IPointerRoutedEventArgs>(&ToolTipService::OnRootVisualPointerMoved));

        IFC_RETURN(rootElement->add_PointerMoved(spRootVisualPointerMovedHandler.Get(), &pointerMoveToken));

        ctl::ComPtr<xaml::ISizeChangedEventHandler> spRootVisualSizeChangedHandler;
        EventRegistrationToken sizeChangedToken;
        spRootVisualSizeChangedHandler.Attach(
            new StaticMemberEventHandler<
                xaml::ISizeChangedEventHandler,
                IInspectable,
                ISizeChangedEventArgs>(&ToolTipService::OnRootVisualSizeChanged));

        ASSERT(ctl::is<IFrameworkElement>(rootElement));
        IFrameworkElement* rootFrameworkElement = static_cast<DirectUI::FrameworkElement*>(rootElement);
        IFC_RETURN(rootFrameworkElement->add_SizeChanged(spRootVisualSizeChangedHandler.Get(), &sizeChangedToken));

        toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.emplace_back(rootElement);
    }

    return S_OK;
}

/* static */ void ToolTipService::OnPublicRootRemoved(_In_ CUIElement* publicRoot)
{
    // This function can be called in a shutdown path after the ToolTipService has already been destroyed.
    // No need to (re)create the ToolTipService here since we only get it to remove handlers.
    ToolTipServiceMetadata* toolTipServiceMetadataNoRef = nullptr;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(
        toolTipServiceMetadataNoRef,
        false /*createIfNeeded*/));

    if (toolTipServiceMetadataNoRef)
    {
        IUIElement* rootElement = static_cast<DirectUI::UIElement*>(publicRoot->GetDXamlPeer());

        if (rootElement != nullptr)
        {
            toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.erase(
                std::remove(
                    toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.begin(),
                    toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.end(),
                    rootElement),
                toolTipServiceMetadataNoRef->m_rootElementsWithHandlersNoRef.end());
        }
    }
}

_Check_return_ HRESULT
ToolTipService::GetToolTipOwnersBoundary(
    _In_ ctl::ComPtr<xaml::IDependencyObject> const& ownerDO, _Out_ XRECTF_RB *ownerBounds)
{
    XRECTF_RB bounds;
    auto owner = ownerDO.AsOrNull<xaml::IUIElement>();
    if (owner)
    {
        IFC_RETURN(ToolTipService::GetGlobalBoundsLogical(owner, &bounds));
    }
    else
    {
        ctl::ComPtr<xaml::Documents::ITextElement> ownerAsTextElement = ownerDO.AsOrNull<xaml::Documents::ITextElement>();

        if (ownerAsTextElement)
        {
            CCoreServices *pCore = static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle());

            XRECTF boundingRect;
            IFC_RETURN(pCore->GetTextElementBoundingRect(ownerAsTextElement.Cast<TextElement>()->GetHandle(), &boundingRect));
            bounds = ToXRectFRB(boundingRect);
        }
        else
        {
            return E_FAIL;
        }
    }

    if (DoubleUtil::IsInfinity(bounds.left) || DoubleUtil::IsNaN(bounds.left) ||
        DoubleUtil::IsInfinity(bounds.top) || DoubleUtil::IsNaN(bounds.top) ||
        DoubleUtil::IsInfinity(bounds.right) || DoubleUtil::IsNaN(bounds.right) ||
        DoubleUtil::IsInfinity(bounds.bottom) || DoubleUtil::IsNaN(bounds.bottom))
    {
        *ownerBounds = {};
    }
    else
    {
        *ownerBounds = bounds;
    }
    return S_OK;
}

_Check_return_ HRESULT
ToolTipService::HandleToolTipSafeZone(
    _In_ wf::Point point,
    _In_ ctl::ComPtr<xaml::IUIElement> const& toolTip,
    _In_ ctl::ComPtr<xaml::IDependencyObject> const& ownerDO)
{
    // On WindowsCore, because message event queue, even if ToolTip is unhooked from CoreWindow during the same PointerMove event handling,
    // CoreWindows.PointerMove event is still be received by ToolTip, and it makes the current ToolTip doesn't match with ToolTip from the event.
    // so we should only handle the tooltip if it's the current ToolTip.
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;
    IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    auto current = pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get();
    if (!current || !ctl::are_equal(ctl::iinspectable_cast(current), ctl::iinspectable_cast(toolTip.Get())))
    {
        return S_OK;
    }

    // owner bounds in global space
    XRECTF_RB ownerBounds = {};
    IFC_RETURN(GetToolTipOwnersBoundary(ownerDO, &ownerBounds));

    // tooltip bounds in global space
    XRECTF_RB toolTipBounds = {};
    IFC_RETURN(ToolTipService::GetGlobalBoundsLogical(toolTip, &toolTipBounds));

    // outside of safe zone, close ToolTip
    if (!ToolTipService::IsToolTipInSafeZone(point, ownerBounds, toolTipBounds))
    {
        IFC_RETURN(ToolTipService::CancelAutomaticToolTip());
    }
    return S_OK;
}

_Check_return_
HRESULT
ToolTipService::CloseToolTipInternal(
    _In_opt_ IKeyRoutedEventArgs* pIKeyRoutedEventArgs)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_tpOpenTimer.Get() == NULL)
    {
        goto Cleanup;
    }

    // close the opened ToolTip or cancel mouse hover
    if (pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get() == NULL)
    {
        pToolTipServiceMetadataNoRef->m_tpOpenTimer.Get()->Stop();
        goto Cleanup;
    }

    if (pIKeyRoutedEventArgs != NULL)
    {
        IFC(pIKeyRoutedEventArgs->get_Key(&key));
        if (IsSpecialKey(key))
        {
            goto Cleanup;
        }
    }

    IFC(CloseAutomaticToolTip(NULL, NULL));

Cleanup:
    RRETURN(hr);
}

BOOLEAN
ToolTipService::IsSpecialKey(
    _In_ wsy::VirtualKey key)
{
    switch (key)
    {
        case wsy::VirtualKey_Menu:
        case wsy::VirtualKey_Back:
        case wsy::VirtualKey_Delete:
        case wsy::VirtualKey_Down:
        case wsy::VirtualKey_End:
        case wsy::VirtualKey_Home:
        case wsy::VirtualKey_Insert:
        case wsy::VirtualKey_Left:
        case wsy::VirtualKey_PageDown:
        case wsy::VirtualKey_PageUp:
        case wsy::VirtualKey_Right:
        case wsy::VirtualKey_Space:
        case wsy::VirtualKey_Up:
            return TRUE;
        default:
            return FALSE;
    }
}

_Check_return_
HRESULT
ToolTipService::GetOwner(
    _Outptr_ xaml::IDependencyObject** ppOwner)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;

    IFCPTR(ppOwner);
    *ppOwner = NULL;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    IFC(pToolTipServiceMetadataNoRef->m_tpOwner.CopyTo(ppOwner));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::AddToNestedOwners(
    _In_ xaml::IDependencyObject* pOwner)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;
    std::list<ctl::WeakRefPtr>::iterator it;
    std::list<ctl::WeakRefPtr>::iterator insertionIndex;
    BOOLEAN bIsAncestor = FALSE;
    ctl::WeakRefPtr wrNewWeakRef;

    IFCPTR(pOwner);

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_isRemovingFromNestedOwners ||
        pToolTipServiceMetadataNoRef->m_isPurgingInvalidNestedOwners)
    {
        IFC(ctl::AsWeak(pOwner, &wrNewWeakRef));
        pToolTipServiceMetadataNoRef->m_objectsToAdd.push_back(wrNewWeakRef);
        goto Cleanup;
    }

    pToolTipServiceMetadataNoRef->m_isAddingToNestedOwners = true;
    IFC(pToolTipServiceMetadataNoRef->EnsureNestedOwnersInstance());

    insertionIndex = pToolTipServiceMetadataNoRef->m_nestedOwners->begin();

    // Don't add if already in the list
    for (it = pToolTipServiceMetadataNoRef->m_nestedOwners->begin(); it != pToolTipServiceMetadataNoRef->m_nestedOwners->end(); ++it)
    {
        ctl::ComPtr<xaml::IDependencyObject> spCurrentDO;
        ctl::WeakRefPtr wrCurrent = *it;
        bool areEqual = false;

        IFC(wrCurrent.As(&spCurrentDO));
        IFC(ctl::are_equal(pOwner, spCurrentDO.Get(), &areEqual));
        if (areEqual)
        {
            goto Cleanup;
        }
    }

    // Add to list, which is increasingly sorted by ancestry
    for (it = pToolTipServiceMetadataNoRef->m_nestedOwners->begin(); it != pToolTipServiceMetadataNoRef->m_nestedOwners->end(); ++it)
    {
        ctl::ComPtr<xaml::IDependencyObject> spCurrentDO;
        ctl::WeakRefPtr wrCurrent = *it;

        IFC(wrCurrent.As(&spCurrentDO));
        if (spCurrentDO)
        {
            ctl::ComPtr<xaml::IFrameworkElement> spContainer;
            ctl::ComPtr<xaml::IFrameworkElement> spCurrentContainer;

            IFC(GetContainerFromOwner(pOwner, &spContainer));
            IFC(GetContainerFromOwner(spCurrentDO.Get(), &spCurrentContainer));

            IFC(spCurrentContainer.Cast<FrameworkElement>()->IsAncestorOf(static_cast<FrameworkElement*>(spContainer.Get()), &bIsAncestor));
            if (bIsAncestor)
            {
                // Found insertion point
                insertionIndex = it;
                break;
            }
        }
    }

    IFC(ctl::AsWeak(pOwner, &wrNewWeakRef));
    pToolTipServiceMetadataNoRef->m_nestedOwners->insert(insertionIndex, wrNewWeakRef);
    pToolTipServiceMetadataNoRef->m_isAddingToNestedOwners = false;
    IFC(RunPendingOwnerListOperations(pToolTipServiceMetadataNoRef));

Cleanup:
    if (pToolTipServiceMetadataNoRef)
    {
        pToolTipServiceMetadataNoRef->m_isAddingToNestedOwners = false;
    }

    RRETURN(hr);
}

// The point is in safe zone if the point is in bounds of owner or tooltip, or if it's within the bounds of the convex
// hull created by the bounds of the owner plus tooltip.
bool ToolTipService::IsToolTipInSafeZone(
    _In_ wf::Point const& point,
    _In_ XRECTF_RB const& ownerBounds,
    _In_ XRECTF_RB const& toolTipBounds)
{
    if (ToolTipService::IsPointInRect(point, ownerBounds) || ToolTipService::IsPointInRect(point, toolTipBounds))
    {
        return true;
    }

    XPOINTF polygonPoints[] = {
        {ownerBounds.left, ownerBounds.top},
        {ownerBounds.left, ownerBounds.bottom},
        {ownerBounds.right, ownerBounds.bottom},
        {ownerBounds.right, ownerBounds.top},
        {toolTipBounds.left, toolTipBounds.top},
        {toolTipBounds.left, toolTipBounds.bottom},
        {toolTipBounds.right, toolTipBounds.bottom},
        {toolTipBounds.right, toolTipBounds.top}
    };

    XUINT32 numHullPoints {};

    // It's ok to pass in the same point buffer for input and output, the function will update the buffer in place.
    ComputeConvexHull<XPOINTF>(
        ARRAYSIZE(polygonPoints),
        polygonPoints,
        &numHullPoints,
        polygonPoints);

    const XPOINTF testPoint {point.X, point.Y};
    const bool isPointInsideConvexHull = IsPointInsidePolygon(
        testPoint,
        numHullPoints,
        polygonPoints);

    return isPointInsideConvexHull;
}

bool ToolTipService::IsPointInRect(
    _In_ wf::Point const& point,
    _In_ XRECTF_RB const& rect)
{
    return (point.Y >= rect.top && point.Y <= rect.bottom && point.X >= rect.left && point.X <= rect.right);
}

_Check_return_ HRESULT ToolTipService::GetGlobalBoundsLogical(
    _In_ ctl::ComPtr<xaml::IUIElement> const& element,
    _Out_ XRECTF_RB* bounds)
{
    return (static_cast<CUIElement*>(element.Cast<UIElement>()->GetHandle()))->GetGlobalBoundsLogical(bounds);
}

_Check_return_ HRESULT
ToolTipService::RemoveFromNestedOwners(
    _In_ xaml::IDependencyObject* pOwner)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;

    ASSERT(pOwner);

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_isAddingToNestedOwners ||
        pToolTipServiceMetadataNoRef->m_isPurgingInvalidNestedOwners)
    {
        ctl::WeakRefPtr weakRef;
        IFC(ctl::AsWeak(pOwner, &weakRef));
        pToolTipServiceMetadataNoRef->m_objectsToRemove.push_back(weakRef);
        goto Cleanup;
    }

    pToolTipServiceMetadataNoRef->m_isRemovingFromNestedOwners = true;

    // Remove from list of nested owners
    if (pToolTipServiceMetadataNoRef->m_nestedOwners != NULL)
    {
        std::list<ctl::WeakRefPtr>::iterator it = pToolTipServiceMetadataNoRef->m_nestedOwners->begin();
        while (it != pToolTipServiceMetadataNoRef->m_nestedOwners->end())
        {
            ctl::ComPtr<xaml::IDependencyObject> spCurrentDO;
            ctl::WeakRefPtr wrCurrent = *it;

            IFC(wrCurrent.As(&spCurrentDO));
            if (spCurrentDO)
            {
                bool areEqual = false;

                IFC(ctl::are_equal(pOwner, spCurrentDO.Get(), &areEqual));
                if (areEqual)
                {
                    IFC(pToolTipServiceMetadataNoRef->DeleteElementFromNestedOwners(it));
                    break;
                }
            }

            ++it;
        }
    }

    pToolTipServiceMetadataNoRef->m_isRemovingFromNestedOwners = false;
    IFC(RunPendingOwnerListOperations(pToolTipServiceMetadataNoRef));

Cleanup:
    if (pToolTipServiceMetadataNoRef)
    {
        pToolTipServiceMetadataNoRef->m_isRemovingFromNestedOwners = false;
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::PurgeInvalidNestedOwners()
{
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;
    std::list<ctl::WeakRefPtr>::iterator it;
    BOOLEAN bIsHitTestVisible = FALSE;

    IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    auto guard = wil::scope_exit([&pToolTipServiceMetadataNoRef]()
    {
        if (pToolTipServiceMetadataNoRef)
        {
            pToolTipServiceMetadataNoRef->m_isPurgingInvalidNestedOwners = false;
        }
    });

    const bool isListOperationInProgress =
        pToolTipServiceMetadataNoRef->m_isAddingToNestedOwners ||
        pToolTipServiceMetadataNoRef->m_isRemovingFromNestedOwners;

    pToolTipServiceMetadataNoRef->m_isPurgingInvalidNestedOwners = true;

    if (pToolTipServiceMetadataNoRef->m_nestedOwners == nullptr)
    {
        return S_OK;
    }

    it = pToolTipServiceMetadataNoRef->m_nestedOwners->begin();
    while (it != pToolTipServiceMetadataNoRef->m_nestedOwners->end())
    {
        ctl::ComPtr<xaml::IDependencyObject> spCurrentDO;
        ctl::WeakRefPtr wrCurrent = *it;

        BOOLEAN shouldErase = FALSE;

        IFC_RETURN(wrCurrent.As(&spCurrentDO));

        if (!spCurrentDO)
        {
            shouldErase = TRUE;
        }
        else
        {
            ctl::ComPtr<xaml::IFrameworkElement> spCurrentContainer;

            IFC_RETURN(GetContainerFromOwner(spCurrentDO.Get(), &spCurrentContainer));
            IFC_RETURN(spCurrentContainer.Cast<FrameworkElement>()->get_IsHitTestVisible(&bIsHitTestVisible));

            shouldErase = !bIsHitTestVisible || !spCurrentContainer.Cast<FrameworkElement>()->IsInLiveTree();
        }

        if (shouldErase)
        {
            if (isListOperationInProgress)
            {
                pToolTipServiceMetadataNoRef->m_objectsToRemove.push_back(*it);
            }
            else
            {
                IFC_RETURN(pToolTipServiceMetadataNoRef->DeleteElementFromNestedOwners(it));
            }
        }
        else
        {
            ++it;
        }
    }

    if (!isListOperationInProgress)
    {
        pToolTipServiceMetadataNoRef->m_isPurgingInvalidNestedOwners = false;
        IFC_RETURN(RunPendingOwnerListOperations(pToolTipServiceMetadataNoRef));
    }

    return S_OK;
}

_Check_return_ HRESULT
ToolTipService::RunPendingOwnerListOperations(
    _In_ ToolTipServiceMetadata* pToolTipServiceMetadataNoRef)
{
    const static std::size_t weakRefPtrArenaSize = 16 * sizeof(ctl::WeakRefPtr);
    Jupiter::arena<weakRefPtrArenaSize> arena;
    typedef Jupiter::stack_allocator<ctl::WeakRefPtr, weakRefPtrArenaSize> Alloc;

    while (!pToolTipServiceMetadataNoRef->m_objectsToAdd.empty())
    {
        // Cache the list so we don't modify it while iterating over it.
        std::vector<ctl::WeakRefPtr, Alloc> objectsToAdd{ Alloc(arena) };
        objectsToAdd.reserve(pToolTipServiceMetadataNoRef->m_objectsToAdd.size());

        for (const auto& objectToAdd : pToolTipServiceMetadataNoRef->m_objectsToAdd)
        {
            objectsToAdd.push_back(std::move(objectToAdd));
        }

        pToolTipServiceMetadataNoRef->m_objectsToAdd.clear();

        for (auto& objectToAdd : objectsToAdd)
        {
            ctl::ComPtr<xaml::IDependencyObject> objectToAddDO;
            IFC_RETURN(objectToAdd.As(&objectToAddDO));

            if (objectToAddDO)
            {
                IFC_RETURN(AddToNestedOwners(objectToAddDO.Get()));
            }
        }
    }

    while (!pToolTipServiceMetadataNoRef->m_objectsToRemove.empty())
    {
        // Cache the list so we don't modify it while iterating over it.
        std::vector<ctl::WeakRefPtr, Alloc> objectsToRemove{ Alloc(arena) };
        objectsToRemove.reserve(pToolTipServiceMetadataNoRef->m_objectsToRemove.size());

        for (const auto& objectToRemove : pToolTipServiceMetadataNoRef->m_objectsToRemove)
        {
            objectsToRemove.push_back(std::move(objectToRemove));
        }

        pToolTipServiceMetadataNoRef->m_objectsToRemove.clear();

        for (auto& objectToRemove : objectsToRemove)
        {
            ctl::ComPtr<xaml::IDependencyObject> objectToRemoveDO;
            IFC_RETURN(objectToRemove.As(&objectToRemoveDO));

            if (objectToRemoveDO)
            {
                IFC_RETURN(RemoveFromNestedOwners(objectToRemoveDO.Get()));
            }
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
ToolTipService::GetFirstNestedOwner(
    _Outptr_ xaml::IDependencyObject** ppFirstNestedOwner)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;

    IFCPTR(ppFirstNestedOwner);
    *ppFirstNestedOwner = NULL;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_nestedOwners != NULL)
    {
        std::list<ctl::WeakRefPtr>::iterator it = pToolTipServiceMetadataNoRef->m_nestedOwners->begin();
        while (it != pToolTipServiceMetadataNoRef->m_nestedOwners->end())
        {
            ctl::ComPtr<xaml::IDependencyObject> spCurrentAsDO;
            ctl::WeakRefPtr wrCurrent = *it;

            IFC(wrCurrent.As(&spCurrentAsDO));
            if (spCurrentAsDO)
            {
                IFC(spCurrentAsDO.MoveTo(ppFirstNestedOwner));
                break;
            }

            ++it;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Used to handle PointerEntered on a ToolTip's owner FrameworkElement.
_Check_return_
HRESULT
ToolTipService::OnOwnerPointerEntered(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spSenderAsDO;
    ctl::ComPtr<IToolTip> spToolTipObject;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;
    BOOLEAN isInPointerMode = FALSE;
    BOOLEAN isAlreadyOpen = FALSE;

    IFC(ctl::do_query_interface(spSenderAsDO, pSender));
    ToolTipService::GetActualToolTipObjectStatic(spSenderAsDO.Get(), &spToolTipObject);
    IFCEXPECT(spToolTipObject);
    IFC(spToolTipObject->get_IsOpen(&isAlreadyOpen));

    if (!isAlreadyOpen)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<IInspectable> spOriginalSource;

        IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_PointerDeviceType(&pointerDeviceType));
        isInPointerMode = pointerDeviceType == mui::PointerDeviceType_Touch;

        IFC(spPointerPoint->get_Position(&s_lastPointerEnteredPoint));

        //
        // Account for offsets on windowed popups
        // The incoming spPointerPoint is relative to an InputSite. The main Xaml tree has an InputSite, but so do all
        // windowed popups. If the ToolTip owner element is in a windowed popup, then the point we get will be relative
        // to that windowed popup rather than relative to the main Xaml tree. We need to account for the difference.
        //
        ctl::ComPtr<DirectUI::DependencyObject> senderAsDO;
        IFC(ctl::do_query_interface(senderAsDO, pSender));
        CDependencyObject* dependencyObject = senderAsDO->GetHandle();
        // Optimization - if there are no open popups, then the sender can't be in an open windowed popup, then there's
        // no windowed popup offset to account for.
        VisualTree* visualTree = VisualTree::GetForElementNoRef(dependencyObject);
        if (visualTree->GetPopupRoot()->HasOpenOrUnloadingPopups())
        {
            // Getting the first ancestor popup is enough. Even for nested popups, Xaml will create the PopupSiteBridge
            // from the DesktopChildSiteBridge for the main tree.
            CPopup* ancestorPopup = dependencyObject->GetFirstAncestorPopup(true /* windowedPopupOnly */);

            if (ancestorPopup)
            {
                XPOINTF_COORDS popupOffset = {};
                IFC(ancestorPopup->GetScreenOffsetFromOwner(&popupOffset));
                XPOINTF popupOffsetLogical = popupOffset.ToXPointF_Logical(ancestorPopup->GetWindowedPopupRasterizationScale());
                s_lastPointerEnteredPoint.X += popupOffsetLogical.x;
                s_lastPointerEnteredPoint.Y += popupOffsetLogical.y;
            }
        }

        // Add to list of nested owners
        IFC(ToolTipService::AddToNestedOwners(spSenderAsDO.Get()));
        IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_OriginalSource(&spOriginalSource));

        IFC(ToolTipService::OnOwnerEnterInternal(
                spSenderAsDO.Get(),
                spOriginalSource.Get(),
                isInPointerMode ? AutomaticToolTipInputMode::Touch : AutomaticToolTipInputMode::Mouse));
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_
HRESULT
ToolTipService::OnOwnerPointerEnteredFromCore(
    _In_ CDependencyObject* sender,
    _In_ CPointerEventArgs* args)
{
    ctl::ComPtr<DependencyObject> senderPeer;
    ctl::ComPtr<IInspectable> argsPeer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(sender, &senderPeer));
    IFC_RETURN(args->CreateFrameworkPeer(&argsPeer));

    if (senderPeer && argsPeer)
    {
        ctl::ComPtr<xaml_input::IPointerRoutedEventArgs> argsPeerAsPointerRoutedEventArgs;

        IFC_RETURN(argsPeer.As(&argsPeerAsPointerRoutedEventArgs));
        IFC_RETURN(OnOwnerPointerEntered(senderPeer.Cast<xaml::IDependencyObject>(), argsPeerAsPointerRoutedEventArgs.Get()));
    }

    return S_OK;
}

// Used to handle PointerExited, PointerCaptureLost, and PointerCanceled on a ToolTip's owner FrameworkElement.
_Check_return_
HRESULT
ToolTipService::OnOwnerPointerExitedOrLostOrCanceled(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    // Opened ToolTip will be kept and will be closed when Pointer is out side of safe zone
    // Not opened ToolTip will be cancelled
    if (!pToolTipServiceMetadataNoRef->m_tpCurrentToolTip)
    {
        // Cancel the ToolTip if it had not been opened yet
        if (pToolTipServiceMetadataNoRef->m_tpOpenTimer)
        {
            IFC(ToolTipService::CancelAutomaticToolTip());
        }
    }
    else
    {
        ctl::ComPtr<xaml::IDependencyObject> owner;
        IFC(ctl::do_query_interface(owner, pSender));

        if (owner)
        {
            pToolTipServiceMetadataNoRef->m_lastToolTipOwnerInSafeZone = nullptr;
            IFC(ctl::AsWeak(owner.Get(), &pToolTipServiceMetadataNoRef->m_lastToolTipOwnerInSafeZone));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::OnOwnerPointerExitedOrLostOrCanceledFromCore(
    _In_ CDependencyObject* sender,
    _In_ CPointerEventArgs* args)
{
    ctl::ComPtr<DependencyObject> senderPeer;
    ctl::ComPtr<IInspectable> argsPeer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(sender, &senderPeer));
    IFC_RETURN(args->CreateFrameworkPeer(&argsPeer));

    if (senderPeer && argsPeer)
    {
        ctl::ComPtr<xaml_input::IPointerRoutedEventArgs> argsPeerAsPointerRoutedEventArgs;

        IFC_RETURN(argsPeer.As(&argsPeerAsPointerRoutedEventArgs));
        IFC_RETURN(OnOwnerPointerExitedOrLostOrCanceled(senderPeer.Cast<xaml::IDependencyObject>(), argsPeerAsPointerRoutedEventArgs.Get()));
    }

    return S_OK;
}

// Used to handle PointerMoved on the application root visual FrameworkElement.
_Check_return_ HRESULT ToolTipService::OnRootVisualPointerMoved(
    _In_ IInspectable* pSender,
    _In_ IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml::IDependencyObject> spOwner;
    BOOLEAN isInPointerMode = FALSE;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    // If the pointer is over a nested owner, and there is no current
    // owner, notify the next nested owner that it is current owner.
    // This supports the pointer coming back to an ancestor after
    // it enters and leaves a nested descendant element. Although
    // the pointer never left the ancestor in this scenario, the ancestor
    // needs to re-open its tooltip, because the pointer came back to the
    // ancestor from the nested descendant.
    IFC_RETURN(ToolTipService::GetOwner(&spOwner));
    if (!spOwner)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        wf::Point position{};

        IFC_RETURN(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
        IFCPTR_RETURN(spPointerPoint);
        IFC_RETURN(spPointerPoint->get_Position(&position));

        ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;
        IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

        ctl::ComPtr<xaml::IUIElement> rootElement;
        if (SUCCEEDED(ctl::do_query_interface(rootElement, pSender)))
        {
            ctl::ComPtr<wfc::IIterable<xaml::UIElement*> > elementsAtPosition;

            IFC_RETURN(VisualTreeHelper::FindElementsInHostCoordinatesPointStatic(
                position,
                rootElement.Get(),
                FALSE /* canHitDisabledElements */,
                FALSE /* canHitInvisibleElements */,
                &elementsAtPosition));

            IFC_RETURN(ToolTipService::PurgeInvalidNestedOwners());

            if (pToolTipServiceMetadataNoRef->m_nestedOwners != NULL)
            {
                auto nestedOwnersIterator = pToolTipServiceMetadataNoRef->m_nestedOwners->begin();
                while (nestedOwnersIterator != pToolTipServiceMetadataNoRef->m_nestedOwners->end())
                {
                    ctl::ComPtr<xaml::IDependencyObject> nestedOwner;
                    ctl::WeakRefPtr wrNestedOwner = *nestedOwnersIterator;

                    IFC_RETURN(wrNestedOwner.As(&nestedOwner));
                    if (!nestedOwner)
                    {
                        IFC_RETURN(pToolTipServiceMetadataNoRef->DeleteElementFromNestedOwners(nestedOwnersIterator));
                        continue;
                    }

                    bool ownerIsAtPosition = false;
                    ctl::ComPtr<wfc::IIterator<xaml::UIElement*> > elementsAtPositionIterator;

                    IFC_RETURN(elementsAtPosition->First(&elementsAtPositionIterator));

                    BOOLEAN hasCurrent = FALSE;
                    IFC_RETURN(elementsAtPositionIterator->get_HasCurrent(&hasCurrent));

                    while (hasCurrent)
                    {
                        ctl::ComPtr<xaml::IUIElement> elementAtPosition;
                        IFC_RETURN(elementsAtPositionIterator->get_Current(&elementAtPosition));

                        if (ctl::iinspectable_cast(elementAtPosition.Get()) == ctl::iinspectable_cast(nestedOwner.Get()))
                        {
                            ownerIsAtPosition = true;
                            break;
                        }

                        IFC_RETURN(elementsAtPositionIterator->MoveNext(&hasCurrent));
                    }

                    if (!ownerIsAtPosition)
                    {
                        IFC_RETURN(pToolTipServiceMetadataNoRef->DeleteElementFromNestedOwners(nestedOwnersIterator));
                        continue;
                    }

                    ctl::ComPtr<xaml::IFrameworkElement> spContainer;

                    IFC_RETURN(spPointerPoint->get_PointerDeviceType(&pointerDeviceType));
                    isInPointerMode = pointerDeviceType == mui::PointerDeviceType_Touch;

                    IFC_RETURN(GetContainerFromOwner(nestedOwner.Get(), &spContainer));

                    IFC_RETURN(ToolTipService::OnOwnerEnterInternal(
                        nestedOwner.Get(),
                        spContainer.Get(),
                        isInPointerMode ? AutomaticToolTipInputMode::Touch : AutomaticToolTipInputMode::Mouse));

                    break;
                }
            }
        }
    }

    return S_OK;
}

// Used to handle SizeChanged on the application root visual FrameworkElement.
_Check_return_ HRESULT ToolTipService::OnRootVisualSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = NULL;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

    if (pToolTipServiceMetadataNoRef->m_tpCurrentToolTip)
    {
        IFC(static_cast<ToolTip*>(pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get())->OnRootVisualSizeChanged());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTipService::OnToolTipChanged(
    _In_ xaml::IDependencyObject* pSender,
    _In_ const PropertyChangedParams& args)
{
    ctl::ComPtr<IFrameworkElement> spSenderAsFE;
    if (SUCCEEDED(ctl::do_query_interface(spSenderAsFE, pSender)))
    {
        const bool isKeyboardAcceleratorToolTip = args.m_pDP->GetIndex() == KnownPropertyIndex::ToolTipService_KeyboardAcceleratorToolTip;
        if (!args.m_pOldValue->IsNullOrUnset())
        {
            IFC_RETURN(ToolTipService::UnregisterToolTip(pSender, spSenderAsFE.Get(), isKeyboardAcceleratorToolTip));
        }

        ctl::ComPtr<IInspectable> spToolTip;
        IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spToolTip));
        if (spToolTip != nullptr)
        {
            IFC_RETURN(ToolTipService::RegisterToolTip(pSender, spSenderAsFE.Get(), spToolTip.Get(), isKeyboardAcceleratorToolTip));
        }
    }

    return S_OK;
}

// Used to handle GotFocus on a ToolTip's owner FrameworkElement.
_Check_return_
HRESULT
ToolTipService::OnOwnerGotFocus(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    ctl::ComPtr<IToolTip> spToolTipObject;
    ctl::ComPtr<xaml::IDependencyObject> spSenderAsDO;
    BOOLEAN isAlreadyOpen = FALSE;

    IFC_RETURN(ctl::do_query_interface(spSenderAsDO, pSender));
    ToolTipService::GetActualToolTipObjectStatic(spSenderAsDO.Get(), &spToolTipObject);
    IFCEXPECT_RETURN(spToolTipObject);
    IFC_RETURN(spToolTipObject->get_IsOpen(&isAlreadyOpen));

    if (!isAlreadyOpen)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(spSenderAsDO.Cast<DependencyObject>()->GetHandle());

        auto focusState = contentRoot->GetFocusManagerNoRef()->GetRealFocusStateForFocusedElement();

        // If the source of a programmatic focus was UIA, we should show the tooltip:
        bool shouldShowToolTip = (focusState == DirectUI::FocusState::Keyboard) ||
                                 (focusState == DirectUI::FocusState::Programmatic && spSenderAsDO && contentRoot->GetInputManager().GetWasUIAFocusSetSinceLastInput());

        if (shouldShowToolTip)
        {
            ctl::ComPtr<IInspectable> spOriginalSource;

            IFC_RETURN(pArgs->get_OriginalSource(&spOriginalSource));
            IFC_RETURN(ToolTipService::OnOwnerEnterInternal(pSender, spOriginalSource.Get(), AutomaticToolTipInputMode::Keyboard));
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
ToolTipService::OnOwnerGotFocusFromCore(
    _In_ CDependencyObject* sender,
    _In_ CRoutedEventArgs* args)
{
    ctl::ComPtr<DependencyObject> senderPeer;
    ctl::ComPtr<IInspectable> argsPeer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(sender, &senderPeer));
    IFC_RETURN(args->CreateFrameworkPeer(&argsPeer));

    if (senderPeer && argsPeer)
    {
        ctl::ComPtr<xaml::IRoutedEventArgs> argsPeerAsRoutedEventArgs;

        IFC_RETURN(argsPeer.As(&argsPeerAsRoutedEventArgs));
        IFC_RETURN(OnOwnerGotFocus(senderPeer.Cast<xaml::IDependencyObject>(), argsPeerAsRoutedEventArgs.Get()));
    }

    return S_OK;
}

 // Used to handle LostFocus on a ToolTip's owner FrameworkElement.
 _Check_return_
HRESULT
ToolTipService::OnOwnerLostFocus(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(OnOwnerLeaveInternal(pSender));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ToolTipService::OnOwnerLostFocusFromCore(
    _In_ CDependencyObject* sender,
    _In_ CRoutedEventArgs* args)
{
    ctl::ComPtr<DependencyObject> senderPeer;
    ctl::ComPtr<IInspectable> argsPeer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(sender, &senderPeer));
    IFC_RETURN(args->CreateFrameworkPeer(&argsPeer));

    if (senderPeer && argsPeer)
    {
        ctl::ComPtr<xaml::IRoutedEventArgs> argsPeerAsRoutedEventArgs;

        IFC_RETURN(argsPeer.As(&argsPeerAsRoutedEventArgs));
        IFC_RETURN(OnOwnerLostFocus(senderPeer.Cast<xaml::IDependencyObject>(), argsPeerAsRoutedEventArgs.Get()));
    }

    return S_OK;
}

 // For a given input mode, returns the initial delay before the ToolTip shows according to spec.
//
// There are normal and reshow timers.  The normal timer is used when first opening a ToolTip.
// The reshow timer is used when a previous ToolTip has been shown within BETWEEN_SHOW_DELAY_MS
// of invoking this one.
//
//          Touch   Mouse   Keyboard
//  --------------------------------
//  Normal     1x      2x         2x
//  Reshow      0    1.5x         2x
//
//  where x = SPI_GETMOUSEHOVERTIME (400 ms by default)
_Check_return_ HRESULT
ToolTipService::GetInitialShowDelay(
    _In_ AutomaticToolTipInputMode mode,
    _In_ BOOLEAN isReshow,
    _Out_ wf::TimeSpan* pDelay)
{
    HRESULT hr = S_OK;
    ULONG ulSPIGetMouseHoverTimeMS = 0;
    INT64 ulSPIGetMouseHoverTimeTicks = 0;
    wf::TimeSpan initialShowDelayTimeSpan = {};

    IFCPTR(pDelay);

    if (!::SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &ulSPIGetMouseHoverTimeMS, 0))
    {
#ifdef DBG
        Trace(L"Failed to fetch SPI_GETMOUSEHOVERTIME. Falling back to DEFAULT_SPI_GETMOUSEHOVERTIME.");
        WCHAR szTrace[32];
        IFCEXPECT(swprintf_s(szTrace, 32, L"Last Error: 0x%08X", HRESULT_FROM_WIN32(GetLastError())));
        Trace(szTrace);
#endif
        ulSPIGetMouseHoverTimeMS = DEFAULT_SPI_GETMOUSEHOVERTIME;
    }

    ulSPIGetMouseHoverTimeTicks = ulSPIGetMouseHoverTimeMS * TICKS_PER_MILLISECOND;

    switch (mode)
    {
    case AutomaticToolTipInputMode::Touch:
        ulSPIGetMouseHoverTimeTicks *= isReshow ? 0 : 1;
        break;
    case AutomaticToolTipInputMode::Mouse:
        ulSPIGetMouseHoverTimeTicks *= static_cast<INT64>(isReshow ? 1.5 : 2);
        break;
    case AutomaticToolTipInputMode::Keyboard:
        ulSPIGetMouseHoverTimeTicks *= 2;
        break;
    }

    initialShowDelayTimeSpan.Duration = ulSPIGetMouseHoverTimeTicks;
    *pDelay = initialShowDelayTimeSpan;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToolTipService::GetContainerFromOwner(
    _In_ xaml::IDependencyObject *owner,
    _Outptr_ xaml::IFrameworkElement** container)
{
    ctl::ComPtr<IToolTip> toolTipObject;

    *container = nullptr;

    ToolTipService::GetActualToolTipObjectStatic(owner, &toolTipObject);

    if (toolTipObject)
    {
        ctl::ComPtr<ToolTip> toolTipObjectConcrete;
        toolTipObjectConcrete = toolTipObject.Cast<ToolTip>();

        IFC_RETURN(toolTipObjectConcrete->GetContainer(container));
    }

    return S_OK;
}

_Check_return_ HRESULT
ToolTipService::GetDispatcherQueueForCurrentThread(
    _Outptr_ msy::IDispatcherQueue** value)
{
    ctl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
        &dispatcherQueueStatics));
    IFC_RETURN(dispatcherQueueStatics->GetForCurrentThread(value));

    return S_OK;
}

SIZE
ToolTipPositioning::MakeMultipleOfUnit(
    _In_ const CConstraint &constraint,
    _In_ SIZE size,
    _In_range_(>, 0) INT unit)
{
    ASSERT("The given size must fit within the constraint",
        size.cx <= RECTWIDTH(constraint) && size.cy <= RECTHEIGHT(constraint));

    SIZE sizeUnit;

    INT cxLarger = ((size.cx % unit) != 0) ? size.cx + unit - (size.cx % unit) : size.cx;
    if (cxLarger > RECTWIDTH(constraint))
    {
        sizeUnit.cx = size.cx - (size.cx % unit);
    }
    else
    {
        sizeUnit.cx = cxLarger;
    }

    INT cyLarger = ((size.cy % unit) != 0) ? size.cy + unit - (size.cy % unit) : size.cy;
    if (cyLarger > RECTHEIGHT(constraint))
    {
        sizeUnit.cy = size.cy - (size.cy % unit);
    }
    else
    {
        sizeUnit.cy = cyLarger;
    }

    return sizeUnit;
}

ToolTipPositioning::CConstraint::CConstraint(
    _In_ INT iLeft,
    _In_ INT iTop,
    _In_ INT iRight,
    _In_ INT iBottom)
{
    left = iLeft;
    top = iTop;
    right = iRight;
    bottom = iBottom;
}

void ToolTipPositioning::CConstraint::SetRect(
    _In_ const RECT &rc)
{
    left = rc.left;
    right = rc.right;
    top = rc.top;
    bottom = rc.bottom;
}

void ToolTipPositioning::CConstraint::SetRect(
    _In_ INT iLeft,
    _In_ INT iTop,
    _In_ INT iRight,
    _In_ INT iBottom)
{
    left = iLeft;
    top = iTop;
    right = iRight;
    bottom = iBottom;
}

BOOL
ToolTipPositioning::IsLefthandedUser()
{
    // Get the menu drop alignment setting
    // On right-handed systems menus are right aligned
    BOOL fRightAlignMenu = TRUE;
    BOOL fRet = ::SystemParametersInfo(SPI_GETMENUDROPALIGNMENT, 0, reinterpret_cast<LPVOID>(&fRightAlignMenu), 0);
    if (!fRet)
    {
#ifdef DBG
        Trace(L"Failed to fetch SPI_GETMENUDROPALIGNMENT. Falling back to right-aligned menu.");
        WCHAR szTrace[32];
        VERIFYHR(swprintf_s(szTrace, 32, L"Last Error: 0x%08X", HRESULT_FROM_WIN32(GetLastError())));
        Trace(szTrace);
#endif
        // Continue with right-aligned menu...
        fRightAlignMenu = TRUE;
    }
    return fRightAlignMenu;
}

SIZE
ToolTipPositioning::ConstrainSize(
    _In_ const SIZE &size,
    _In_ const INT xMax,
    _In_ const INT yMax)
{
    // Apply constraint
    SIZE sizeShrunk = size;

    // Calc horizontal constraint
    if (size.cx > xMax)
    {
        sizeShrunk.cx = xMax;
    }
    // Calc vertical constraint
    if (size.cy > yMax)
    {
        sizeShrunk.cy = yMax;
    }
    return sizeShrunk;
}

RECT
ToolTipPositioning::HorizontallyCenterRect(
    _In_ const CConstraint &container,
    _In_ const RECT &rcToCenter)
{
    INT dx = ((container.left - rcToCenter.left) + (container.right - rcToCenter.right)) / 2;
    RECT rcCentered = rcToCenter;
    OffsetRect(&rcCentered, dx, 0);
    return rcCentered;
}

RECT
ToolTipPositioning::VerticallyCenterRect(
    _In_ const CConstraint &container,
    _In_ const RECT &rcToCenter)
{
    INT dy = ((container.top - rcToCenter.top) + (container.bottom - rcToCenter.bottom)) / 2;
    RECT rcCentered = rcToCenter;
    OffsetRect(&rcCentered, 0, dy);
    return rcCentered;
}

RECT
ToolTipPositioning::MoveNearRect(
    _In_ const RECT &rcWindow,
    _In_ const RECT &rcWindowToTract,
    _In_ const xaml_primitives::PlacementMode nSide)
{
    POINT ptOffset = {0};
    switch (nSide)
    {
    case xaml_primitives::PlacementMode_Left:
        ptOffset.y = 0;
        ptOffset.x = rcWindowToTract.left - rcWindow.right;
        break;
    case xaml_primitives::PlacementMode_Right:
        ptOffset.y = 0;
        ptOffset.x = rcWindowToTract.right - rcWindow.left;
        break;
    case xaml_primitives::PlacementMode_Top:
        ptOffset.y = rcWindowToTract.top - rcWindow.bottom;
        ptOffset.x = 0;
        break;
    case xaml_primitives::PlacementMode_Bottom:
        ptOffset.y = rcWindowToTract.bottom - rcWindow.top;
        ptOffset.x = 0;
    }
    RECT rcOffset = rcWindow;
    OffsetRect(&rcOffset, ptOffset);
    return rcOffset;
}

BOOL
ToolTipPositioning::IsContainedInRect(
    _In_ const CConstraint &container,
    _In_ const RECT &rc)
{
    bool isContained = true;
    if (rc.left > rc.right || rc.top > rc.bottom)
    {
        ASSERT(false, "This rect is ill formed.");
        isContained = false;
    }
    if (rc.left < container.left || rc.right > container.right || rc.top < container.top || rc.bottom > container.bottom)
    {
        isContained = false;
    }

    return isContained;
}

RECT
ToolTipPositioning::MoveRectToPoint(
    _In_ const RECT &rc,
    _In_ INT x,
    _In_ INT y)
{
    RECT rcMoved = rc;
    OffsetRect(&rcMoved, x - rcMoved.left, y - rcMoved.top);
    return rcMoved;
}

RECT
ToolTipPositioning::ShiftRectIntoContainer(
    _In_ const CConstraint &container,
    _In_ const RECT &rcToShift)
{
    ASSERT("A rect must be fit in a container in order to be shifted into it",
        (RECTWIDTH(rcToShift) <= RECTWIDTH(container)) && (RECTHEIGHT(rcToShift) <= RECTHEIGHT(container)));

    RECT rcShifted = rcToShift;
    if (rcShifted.left < container.left)
    {
        rcShifted = MoveRectToPoint(rcShifted, container.left, rcShifted.top);
    }
    else if (rcShifted.right > container.right)
    {
        rcShifted = MoveRectToPoint(rcShifted, container.right - RECTWIDTH(rcShifted), rcShifted.top);
    }

    if (rcShifted.top < container.top)
    {
        rcShifted = MoveRectToPoint(rcShifted, rcShifted.left, container.top);
    }
    else if (rcShifted.bottom > container.bottom)
    {
        rcShifted = MoveRectToPoint(rcShifted, rcShifted.left, container.bottom - RECTHEIGHT(rcShifted));
    }
    ASSERT(IsContainedInRect(container, rcShifted));
    return rcShifted;
}

BOOL
ToolTipPositioning::CanPositionRelativeOnSide(
    _In_ const CConstraint &windowToTrack,
    _In_ const RECT &rcWindow,
    _In_ xaml_primitives::PlacementMode nSide,
    _In_ const CConstraint &constraint)
{
    INT nAvailableWidth = 0;
    INT nAvailableHeight = 0;
    switch (nSide)
    {
    case xaml_primitives::PlacementMode_Left:
        {
            nAvailableWidth = windowToTrack.left - constraint.left;
            nAvailableHeight = RECTHEIGHT(constraint);
        }
        break;

    case xaml_primitives::PlacementMode_Top:
        {
            nAvailableWidth = RECTWIDTH(constraint);
            nAvailableHeight = windowToTrack.top - constraint.top;
        }
        break;

    case xaml_primitives::PlacementMode_Right:
        {
            nAvailableWidth = constraint.right - windowToTrack.right;
            nAvailableHeight = RECTHEIGHT(constraint);
        }
        break;

    case xaml_primitives::PlacementMode_Bottom:
        {
            nAvailableWidth = RECTWIDTH(constraint);
            nAvailableHeight = constraint.bottom - windowToTrack.bottom;
        }
        break;
    }
    return ((nAvailableWidth >= RECTWIDTH(rcWindow)) &&
            (nAvailableHeight >= RECTHEIGHT(rcWindow)));
}

RECT
ToolTipPositioning::PositionRelativeOnSide(
    _In_ const CConstraint &windowToTrack,
    _In_ const RECT &rcWindow,
    _In_ xaml_primitives::PlacementMode nSide,
    _In_ const CConstraint &constraint)
{
    RECT rcAdjusted = {};
    switch (nSide)
    {
    case xaml_primitives::PlacementMode_Left:
    case xaml_primitives::PlacementMode_Right:
        {
            rcAdjusted = VerticallyCenterRect(windowToTrack, MoveNearRect(rcWindow, windowToTrack, nSide));
        }
        break;

    case xaml_primitives::PlacementMode_Top:
    case xaml_primitives::PlacementMode_Bottom:
        {
            rcAdjusted = HorizontallyCenterRect(windowToTrack, MoveNearRect(rcWindow, windowToTrack, nSide));
        }
        break;
    }

    if (!IsContainedInRect(constraint, rcAdjusted))
    {
        rcAdjusted = ShiftRectIntoContainer(constraint, rcAdjusted);
    }
    return rcAdjusted;
}

// Adjusts a window to use relative positioning.
// -  constraint is the rectangle which should fully contain the Flyout, in screen coordinates
// -  sizeFlyout is the proposed SIZE of the Flyout, in screen coordinates
// -  rcDockTo is the RECT Flyout should not obscure, in screen coordinates
// -  sidePreferred is the side the Flyout should appear on
//
// Returns a RECT which should be used for the Flyout, in screen coordinates
RECT
ToolTipPositioning::QueryRelativePosition(
    _In_ const CConstraint &constraint,
    _In_ SIZE sizeFlyout,
    _In_ RECT rcDockTo,
    _In_ xaml_primitives::PlacementMode nSidePreferred,
    _In_ POPUP_SNAPPING popupSnapping,
    _Out_ xaml_primitives::PlacementMode* pSideChosen)
{
    ASSERT(pSideChosen, "pSideChosen must be non-null");

    CConstraint typographic = constraint;
    xaml_primitives::PlacementMode nSideChosen = ToolTip::DefaultPlacementMode;

    if (popupSnapping == PS_TYPOGRAPHIC_GRID)
    {
        InflateRect(&typographic, -TYPOGRAPHIC_LARGE, -TYPOGRAPHIC_LARGE);
    }

    SIZE sizeUnit = ConstrainSize(sizeFlyout, RECTWIDTH(typographic), RECTHEIGHT(typographic));
    if (popupSnapping == PS_TYPOGRAPHIC_GRID)
    {
        sizeUnit = MakeMultipleOfUnit(typographic, sizeUnit, TYPOGRAPHIC_SMALL);
    }

    RECT rcFlyout = {};
    rcFlyout.right = sizeUnit.cx;
    rcFlyout.bottom = sizeUnit.cy;

    CConstraint constraintDockTo;
    constraintDockTo.SetRect(rcDockTo);

    RECT rcAdjusted = {};
    if (CanPositionRelativeOnSide(constraintDockTo, rcFlyout, nSidePreferred, typographic))
    {
        rcAdjusted = PositionRelativeOnSide(constraintDockTo, rcFlyout, nSidePreferred, typographic);
        nSideChosen = nSidePreferred;
    }
    else
    {
        // fTriedRight isnt needed since if we know the other three are true then it must be that
        // we didn't try the right side
        bool fTriedLeft = false;
        bool fTriedAbove = false;
        bool fTriedBelow = false;

        xaml_primitives::PlacementMode nSideOpposite = xaml_primitives::PlacementMode_Top;
        switch (nSidePreferred)
        {
        case xaml_primitives::PlacementMode_Top:
            nSideOpposite = xaml_primitives::PlacementMode_Bottom;
            fTriedAbove = true;
            fTriedBelow = true;
            break;

        case xaml_primitives::PlacementMode_Bottom:
            nSideOpposite = xaml_primitives::PlacementMode_Top;
            fTriedAbove = true;
            fTriedBelow = true;
            break;

        case xaml_primitives::PlacementMode_Left:
            nSideOpposite = xaml_primitives::PlacementMode_Right;
            fTriedLeft = true;
            break;

        case xaml_primitives::PlacementMode_Right:
            nSideOpposite = xaml_primitives::PlacementMode_Left;
            fTriedLeft = true;
            break;
        }

        if (CanPositionRelativeOnSide(constraintDockTo, rcFlyout, nSideOpposite, typographic))
        {
            rcAdjusted = PositionRelativeOnSide(constraintDockTo, rcFlyout, nSideOpposite, typographic);
            nSideChosen = nSideOpposite;
        }
        else
        {
            xaml_primitives::PlacementMode nSideAxialBest;
            if ((nSidePreferred == xaml_primitives::PlacementMode_Left) || (nSidePreferred == xaml_primitives::PlacementMode_Right))
            {
                nSideAxialBest = xaml_primitives::PlacementMode_Top;
                fTriedAbove = true;
            }
            else
            {
                if (IsLefthandedUser())
                {
                    nSideAxialBest = xaml_primitives::PlacementMode_Right;
                }
                else
                {
                    nSideAxialBest = xaml_primitives::PlacementMode_Left;
                    fTriedLeft = true;
                }
            }

            if (CanPositionRelativeOnSide(constraintDockTo, rcFlyout, nSideAxialBest, typographic))
            {
                rcAdjusted = PositionRelativeOnSide(constraintDockTo, rcFlyout, nSideAxialBest, typographic);
                nSideChosen = nSideAxialBest;
            }
            else
            {
                xaml_primitives::PlacementMode nSideRemaining;
                if (!fTriedAbove)
                {
                    nSideRemaining = xaml_primitives::PlacementMode_Top;
                }
                else if (!fTriedBelow)
                {
                    nSideRemaining = xaml_primitives::PlacementMode_Bottom;
                }
                else if (!fTriedLeft)
                {
                    nSideRemaining = xaml_primitives::PlacementMode_Left;
                }
                else
                {
                    nSideRemaining = xaml_primitives::PlacementMode_Right;
                }

                if (CanPositionRelativeOnSide(constraintDockTo, rcFlyout, nSideRemaining, typographic))
                {
                    rcAdjusted = PositionRelativeOnSide(constraintDockTo, rcFlyout, nSideRemaining, typographic);
                    nSideChosen = nSideRemaining;
                }
                else
                {
                    switch (nSidePreferred)
                    {
                    case xaml_primitives::PlacementMode_Left:
                        {
                            POINT pt;
                            pt.x = typographic.left;
                            pt.y = rcFlyout.top;
                            rcAdjusted = MoveRectToPoint(rcFlyout, pt.x, pt.y);
                            rcAdjusted = VerticallyCenterRect(constraintDockTo, rcAdjusted);
                        }
                        break;

                    case xaml_primitives::PlacementMode_Top:
                        {
                            POINT pt;
                            pt.x = rcFlyout.left;
                            pt.y = typographic.top;
                            rcAdjusted = MoveRectToPoint(rcFlyout, pt.x, pt.y);
                            rcAdjusted = HorizontallyCenterRect(constraintDockTo, rcAdjusted);
                        }
                        break;

                    case xaml_primitives::PlacementMode_Right:
                        {
                            POINT pt;
                            pt.x = typographic.right - RECTWIDTH(rcFlyout);
                            pt.y = rcFlyout.top;
                            rcAdjusted = MoveRectToPoint(rcFlyout, pt.x, pt.y);
                            rcAdjusted = VerticallyCenterRect(constraintDockTo, rcAdjusted);
                        }
                        break;

                    case xaml_primitives::PlacementMode_Bottom:
                        {
                            POINT pt;
                            pt.x = rcFlyout.left;
                            pt.y = typographic.bottom - RECTHEIGHT(rcFlyout);
                            rcAdjusted = MoveRectToPoint(rcFlyout, pt.x, pt.y);
                            rcAdjusted = HorizontallyCenterRect(constraintDockTo, rcAdjusted);
                        }
                        break;
                    }

                    rcAdjusted = ShiftRectIntoContainer(typographic, rcAdjusted);
                    nSideChosen = nSidePreferred;
                }
            }
        }
    }

    *pSideChosen = nSideChosen;
    return rcAdjusted;
}
