// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "palnetwork.h"
#include <xref_ptr.h>
#include <vector_map.h>
#include <vector>
#include <CompactInlineVector.h>

// Need to forward reference the args
class CEventArgs;
class CControl;
struct IPALQueue;
struct REQUEST;

enum class RoutingStrategy
{
    Bubbling = 0,
    Tunnelling = 1,
};

//------------------------------------------------------------------------
//
//  Class:  CEventManager
//
//  Synopsis:
//      CEventManager class
//
//------------------------------------------------------------------------
class CRoutedEventArgs;
class CXcpDispatcher;

class CEventManager final : public IObject, public IPALExecuteOnUIThread
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices* pCore,
        _Outptr_ CEventManager **ppEventManager);

// IObject overrides

    XUINT32 AddRef() override;
    XUINT32 Release() override;

// IPALExecuteOnUIThread

    _Check_return_ HRESULT Execute() override;

// CEventManager methods

    _Check_return_ HRESULT EnableEvents(
        _In_ CDependencyObject *pObject,
        _In_ std::vector<REQUEST> *pEventList);

    _Check_return_ HRESULT DisableEvents(
        _In_ CDependencyObject *pObject,
        _In_ std::vector<REQUEST> *pEventList);

    _Check_return_ HRESULT ClearRequests();

    void Raise(
        _In_ EventHandle hEvent,
        _In_ XINT32 bRefire,
        _In_opt_ CDependencyObject *pSender,
        _In_opt_ CEventArgs *pArgs = NULL,
        _In_ bool fRaiseSync  = false,
        _In_ bool fInputEvent = false,
        _In_ bool bAllowErrorFallback = true,
        _In_opt_ CDependencyObject *pSenderOverride = NULL);

    bool RaiseRoutedEvent(
        _In_ EventHandle hEvent,
        _In_ CDependencyObject *pSource,
        _In_ CRoutedEventArgs *pArgs = nullptr,
        _In_ bool bIgnoreVisibility = false,
        _In_ bool fRaiseSync  = false,
        _In_ bool fInputEvent = false,
        _In_opt_ CDependencyObject* coerceToHandledAtElement = nullptr,
        _In_ RoutingStrategy strategy = RoutingStrategy::Bubbling);

    void ThreadSafeRaise(
        _In_ EventHandle hEvent,
        _In_ XINT32 bRefire,
        _In_opt_ CDependencyObject *pSender,
        _Inout_opt_ CEventArgs **ppArgs = NULL,
        _In_ bool fAllowErrorFallback = true,
        _In_ bool fUseFastPath = false);

    void RaiseCallback(
        _In_ CEventArgs *pArgs);

private:
    void TraceInputEventBegin(
        _In_ const CDependencyObject *pSender,
        _In_ CEventArgs *pInputArgs,
        _In_ const EventHandle &hEvent);

    void TraceInputEventEnd(
        _In_ CEventArgs *pInputArgs,
        _In_ const EventHandle &hEvent);

    void AddToLoadedEventListIfNeeded(
        _In_ CDependencyObject* pDO,
        _In_ EventHandle hEvent);

    void RemoveFromLoadedEventList(_In_ CDependencyObject* pElement);

public:
    _Check_return_ HRESULT ProcessQueue(_In_ bool fProcessFastQueue = false);

    _Check_return_ HRESULT FlushQueue();

    _Check_return_ HRESULT Disable();

    _Check_return_ bool ShouldRaiseLoadedEvent()
    {
        return m_fRaiseLoadedEventNeeded;
    }

    _Check_return_ HRESULT RequestRaiseLoadedEventOnNextTick()
    {
        m_fRaiseLoadedEventNeeded = TRUE;
        return RequestAdditionalFrame();
    }

    _Check_return_ HRESULT RaiseLoadedEvent();


    void Reset()
    {
        m_topDownPathToSource.clear();
        m_topDownPathToSource.shrink_to_fit();
    }

// Static methods
    _Check_return_ HRESULT static AddEventListener(_In_ CDependencyObject *pDO,
                                    _Inout_ std::unique_ptr<std::vector<REQUEST>>& pEventList,
                                    _In_ EventHandle hEvent,
                                    _In_ CValue *pValue,
                                    _In_ XINT32 iListenerType,
                                    _In_ bool fHandledEventsToo = false,
                                    _In_ bool fSkipIsActiveCheck = false);

    _Check_return_ HRESULT static RemoveEventListener(_In_ CDependencyObject *pDO,
                                    _In_ std::vector<REQUEST> *pEventList,
                                    _In_ EventHandle hEvent,
                                    _In_ CValue *pValue);

    bool IsLoadedEventPending(_In_ CDependencyObject* pElement);

// CEventManager fields
public:
    EVENTPFN m_pfnScriptCallbackAsync;
    EVENTPFN m_pfnScriptCallbackSync;
    // this is for the condition where we want to call into a
    // global function and we need to ensure that
    // we prevent cross domain hacks since we send in an instance
    // of the object to the DOM.
    void *   m_pControl;

private:
    typedef std::vector<xref_ptr<CDependencyObject>> CDependencyObjectVector;
    using TempRequests = CompactInlineVector<REQUEST, 2>;

    CEventManager()
    {
        m_cRef = 1;
        XCP_WEAK(&m_pControl);
        m_pControl =  NULL;
        m_pfnScriptCallbackAsync = NULL;
        m_pfnScriptCallbackSync = NULL;
        m_pSlowQueue = NULL;
        m_pFastQueue = NULL;
        XCP_WEAK(&m_pCore);
        m_pCore = NULL;
        m_fRaiseLoadedEventNeeded = FALSE;
    }

   ~CEventManager();

   bool RaiseRoutedEventBubbling(
       _In_ EventHandle hEvent,
       _In_opt_ CDependencyObject* pSource,
       _In_opt_ CRoutedEventArgs* pArgs,
       _In_ bool bIgnoreVisibility,
       _In_ bool fRaiseSync,
       _In_ bool fInputEvent,
       _In_opt_ CDependencyObject* coerceToHandledAtElement);

   void RaiseRoutedEventTunnelling(
       _In_ EventHandle hEvent,
       _In_ CDependencyObject *pSource,
       _In_ CRoutedEventArgs *pArgs,
       _In_ bool bIgnoreVisibility,
       _In_ bool fRaiseSync,
       _In_ bool fInputEvent);

    inline void RaiseControlEvents(
        _In_ EventHandle hEvent,
        _In_opt_ CControl *pSender,
        _In_opt_ CEventArgs *pArgs,
        _In_ EVENTPFN pfnScriptCallback);

    inline void RaiseUIElementEvents(
        _In_ EventHandle hEvent,
        _In_opt_ CUIElement *pSender,
        _In_opt_ CEventArgs *pArgs,
        _In_ EVENTPFN pfnScriptCallback);

    inline void RaiseStaticEvents(
        _In_ EventHandle hEvent,
        _In_opt_ CEventArgs *pArgs,
        _In_ EVENTPFN pfnScriptCallback);

    HRESULT RaiseHelper(const gsl::span<const REQUEST>& requests,
                        _In_ CDependencyObject *pSender,
                        CEventArgs *pArgs,
                        EVENTPFN pfnScriptCallback,
                        bool& bFired,
                        _In_opt_ CDependencyObject *pSenderOverride = nullptr);

    static void FilterEligibleHandlers(
        const gsl::span<REQUEST>& handlers,
        EventHandle hEvent,
        bool bRefire,
        _Inout_ TempRequests* pMatches);

    static bool IsLoadedEvent(_In_ EventHandle hEvent) { return hEvent.index == KnownEventIndex::FrameworkElement_Loaded; }
    static bool IsLoadingEvent(_In_ EventHandle hEvent) { return hEvent.index == KnownEventIndex::FrameworkElement_Loading; }
    static bool IsUnloadedEvent(_In_ EventHandle hEvent) { return hEvent.index == KnownEventIndex::FrameworkElement_Unloaded; }

    static bool IsSyncInputEvent(_In_ EventHandle hEvent)
    {
        // These input events are fired synchronously to the application, without taking
        // the reentrancy guard, because the code path to fire the event has been hardened
        // against reentrancy. However if the reentrancy guard has already been taken, the
        // application will be halted to prevent reentrancy in XAML components that have not
        // been hardened against reentrancy. (See  WM_SCRIPT_SYNC_INPUT_CALL_BACK handler.)
        // When adding a new event to this list, you must verify that the code path that
        // fires the new event has been hardened.
        return hEvent.index == KnownEventIndex::UIElement_KeyDown ||
            hEvent.index == KnownEventIndex::UIElement_KeyUp ||
            hEvent.index == KnownEventIndex::UIElement_TabProcessing ||
            hEvent.index == KnownEventIndex::UIElement_CharacterReceived ||
            hEvent.index == KnownEventIndex::UIElement_PointerEntered ||
            hEvent.index == KnownEventIndex::UIElement_PointerPressed ||
            hEvent.index == KnownEventIndex::UIElement_PointerMoved ||
            hEvent.index == KnownEventIndex::UIElement_PointerReleased ||
            hEvent.index == KnownEventIndex::UIElement_PointerExited ||
            hEvent.index == KnownEventIndex::UIElement_PointerWheelChanged ||
            hEvent.index == KnownEventIndex::UIElement_PointerCanceled ||
            hEvent.index == KnownEventIndex::UIElement_PointerCaptureLost ||
            hEvent.index == KnownEventIndex::UIElement_Tapped ||
            hEvent.index == KnownEventIndex::UIElement_DoubleTapped ||
            hEvent.index == KnownEventIndex::UIElement_RightTappedUnhandled ||
            hEvent.index == KnownEventIndex::UIElement_Holding ||
            hEvent.index == KnownEventIndex::UIElement_RightTapped ||
            hEvent.index == KnownEventIndex::UIElement_ManipulationStarting ||
            hEvent.index == KnownEventIndex::UIElement_ManipulationInertiaStarting ||
            hEvent.index == KnownEventIndex::UIElement_ManipulationStarted ||
            hEvent.index == KnownEventIndex::UIElement_ManipulationDelta ||
            hEvent.index == KnownEventIndex::UIElement_ManipulationCompleted ||
            hEvent.index == KnownEventIndex::UIElement_PreviewKeyDown ||
            hEvent.index == KnownEventIndex::UIElement_PreviewKeyUp ||
            hEvent.index == KnownEventIndex::KeyboardAccelerator_Invoked;
    }

    static bool IsValidSyncInputEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent, _In_ bool fRaiseSync)
    {
        // PointerEntered and PointerExited can be raised as the async if the sync input event
        // is in the middle of processing.
        return ((IsSyncInputEvent(hEvent)) &&
                (fInputEvent) &&
                (fRaiseSync || hEvent.index == KnownEventIndex::UIElement_PointerEntered || hEvent.index == KnownEventIndex::UIElement_PointerExited));
    }

    static bool IsKeyEvent(_In_ EventHandle hEvent)
    {
        return hEvent.index == KnownEventIndex::UIElement_KeyDown ||
            hEvent.index == KnownEventIndex::UIElement_KeyUp ||
            hEvent.index == KnownEventIndex::UIElement_PreviewKeyDown ||
            hEvent.index == KnownEventIndex::UIElement_PreviewKeyUp;
    }

    _Check_return_ HRESULT ProcessQueueImpl(_In_ IPALQueue *pQueue);

    static bool DoesElementAllowHandlerWhenNotLive(CDependencyObject* pDO);

    _Check_return_ HRESULT RequestAdditionalFrame();

    static bool IsValidStaticEventDelegate(_In_ EventHandle hEvent);

    _Check_return_ HRESULT RaiseLoadedEventForObject(_In_ CDependencyObject* pLoadedEventObject, _In_ CEventArgs* loadedArgs);

    CXcpDispatcher* GetXcpDispatcher(_In_ CDependencyObject* pLoadedEventObject);

    _Check_return_ HRESULT GetParentForRoutedEventBubbling(EventHandle hEvent, _In_ CDependencyObject* currentObject, _In_ CDependencyObject** parentObject);

private:
    std::vector<xref_ptr<CUIElement>> m_topDownPathToSource;

    XUINT32                     m_cRef;
    IPALQueue                  *m_pSlowQueue;
    IPALQueue                  *m_pFastQueue;
    CDependencyObjectVector     m_loadedEventObjects;
    CCoreServices*              m_pCore;
    bool                       m_fRaiseLoadedEventNeeded;

private:
    struct RaiseArguments
    {
        EventHandle              m_hEvent;
        XINT32                   m_bRefire;
        CDependencyObject       *m_pSender;
        CEventArgs              *m_pArgs;
        bool                    m_fAllowErrorFallback;

        RaiseArguments(
            EventHandle hEvent,
            XINT32 bRefire,
            _In_opt_ CDependencyObject *pSender,
            _Inout_opt_ CEventArgs **pArgs,
            _In_ bool fAllowErrorFallback
            );

        ~RaiseArguments();
    };
};
