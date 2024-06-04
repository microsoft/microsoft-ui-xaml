// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines all core DXaml classes.

#pragma once

#include "TrackerPtr.h"
#include "DependencyObject.h"
#include "FrameworkEventArgs.h" // needed by RoutedEventArgs.g.h
#include "RoutedEventArgs.g.h"
#include "ErrorHelper.h"
#include "EnumDefs.g.h" // uses DirectUI::ManagedEvent
#include <bit_vector.h>
#include "LifetimeUtils.h"
#include "CoreImports.h"

namespace DirectUI
{
    class Binding;
    class FrameworkElement;

    //------------------------------------------------------------------------
    // internal enum used across appbarservice and appbar
    //------------------------------------------------------------------------
    enum AppBarMode
    {
        AppBarMode_Floating,
        AppBarMode_Top,
        AppBarMode_Bottom,
        AppBarMode_Inline,   // Similar to floating, except it doesn't register with appbarservice.
    };

    MIDL_INTERFACE("82ca158e-f647-444b-9251-4d9bb875f0be")
    IModernCollectionBasePanel: IInspectable
    {
    };

    MIDL_INTERFACE("b1e8b97b-81e8-44e0-9ca4-953ef45e596a")
    ICalendarViewBaseItem: IInspectable
    {
    };

    MIDL_INTERFACE("7358abac-10f9-447c-b773-6bc3d9905079")
    ITextAdapter: IInspectable
    {
    };

    MIDL_INTERFACE("fbd4119d-01c0-40ed-9c21-723cb77ebba0")
    ITextRangeAdapter: IInspectable
    {
    };

    MIDL_INTERFACE("cf14860c-5248-4214-997f-aa576543583c")
    ICorePropertyChangedEventArgs : public IUnknown
    {
        virtual _Check_return_ HRESULT get_Property(_Out_ KnownPropertyIndex* pValue) = 0;
    };

    MIDL_INTERFACE("34ed8377-28f3-4af8-9107-2b62408b483e")
    ICorePropertyChangedEventHandler : public IUnknown
    {
        virtual _Check_return_ HRESULT __stdcall Invoke(_In_ IInspectable* pSender, _In_ ICorePropertyChangedEventArgs* pArgs) = 0;
    };

    MIDL_INTERFACE("b8d4644a-48e6-4e93-9834-a998d3ae1f60")
    IInheritanceContextChangedEventHandler: public IUnknown
    {
        virtual _Check_return_ HRESULT Invoke(_In_ DependencyObject* pSender, _In_ IInspectable* pArgs) = 0;
    };


    MIDL_INTERFACE("4f6651bd-34e6-4e23-b336-733104c5cc1b")
    IInheritanceContextChangedEventSource: public IUnknown
    {
        virtual _Check_return_ HRESULT AddHandler(_In_ IInheritanceContextChangedEventHandler* pHandler) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(_In_ IInheritanceContextChangedEventHandler* pHandler) = 0;
    };

    enum DataContextChangedReason
    {
        NewDataContext,
        EnteringLiveTree
    };

    struct DataContextChangedParams
    {
        DataContextChangedParams(_In_ FrameworkElement* pOriginalSource, _In_ DataContextChangedReason reason)
            : m_pOriginalSourceNoRef(pOriginalSource)
            , m_dataContextChangedReason(reason)
            , m_pNewDataContext(nullptr)
            , m_pNewDataContextOuterNoRef(nullptr)
            , m_fResolvedNewDataContext(FALSE)
        {
        }

        DataContextChangedParams(_In_ FrameworkElement* pOriginalSource, _In_ DataContextChangedReason reason, _In_ const CValue& newDataContext, _In_ IInspectable* newDataContextOuterNoRef)
            : m_pOriginalSourceNoRef(pOriginalSource)
            , m_dataContextChangedReason(reason)
            , m_pNewDataContext(&newDataContext)
            , m_pNewDataContextOuterNoRef(newDataContextOuterNoRef)
            , m_fResolvedNewDataContext(TRUE)
        {
        }

        _Check_return_ HRESULT GetNewDataContext(_COM_Outptr_result_maybenull_ IInspectable** ppNewValue) const;

        DataContextChangedReason m_dataContextChangedReason;
        FrameworkElement* m_pOriginalSourceNoRef;
        const CValue* m_pNewDataContext;
        IInspectable* m_pNewDataContextOuterNoRef;
        bool m_fResolvedNewDataContext;
    };

    MIDL_INTERFACE("72c90a23-f35d-4b1a-8a4a-4eb1e092c0b4")
    IDataContextChangedHandler: public IUnknown
    {
        virtual _Check_return_ HRESULT Invoke(_In_ DependencyObject* pSender, _In_ const DataContextChangedParams* pArgs) = 0;
    };

    MIDL_INTERFACE("fdab99f7-0951-430b-b415-2fb246b4123a")
    IDataContextChangedEventSource: public IUnknown
    {
        virtual _Check_return_ HRESULT AddHandler(_In_ IDataContextChangedHandler* pHandler) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(_In_ IDataContextChangedHandler* pHandler) = 0;
    };

    MIDL_INTERFACE("a4baffc4-8cf8-461e-88c1-8518c1255863")
    ICVSViewChangedHandler: public IUnknown
    {
        virtual _Check_return_ HRESULT Invoke(_In_ IInspectable *pSender, _In_ IInspectable *pArgs) = 0;
    };

    MIDL_INTERFACE("fea6faae-4d8b-4aea-97da-a2a1d6672b77")
    ICVSViewChangedEventSource: public IUnknown
    {
        virtual _Check_return_ HRESULT AddHandler(_In_ ICVSViewChangedHandler *pHandler) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(_In_ ICVSViewChangedHandler *pHandler) = 0;
    };

    MIDL_INTERFACE("b279a506-6907-430a-9023-a3537035e509")
    ICurrentChangedEventSource: public IUnknown
    {
        virtual _Check_return_ HRESULT AddHandler(_In_ wf::IEventHandler<IInspectable*> *pHandler) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(_In_ wf::IEventHandler<IInspectable*> *pHandler) = 0;
    };

    MIDL_INTERFACE("1fa1e14a-d2b4-426e-a207-48dff531ccb9")
    ICurrentChangingEventSource: public IUnknown
    {
        virtual _Check_return_ HRESULT AddHandler(_In_ xaml_data::ICurrentChangingEventHandler *pHandler) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(_In_ xaml_data::ICurrentChangingEventHandler *pHandler) = 0;
    };

    MIDL_INTERFACE("080c5e03-0ab6-4ed0-98a6-e4a75f5fba61")
    IUntypedEventSource: public IInspectable,  public PReferenceTrackerInternal
    {
        virtual _Check_return_ HRESULT UntypedRaise(_In_opt_ IInspectable* pSender, _In_opt_ IInspectable* pArgs) = 0;
        virtual _Check_return_ HRESULT Disconnect() = 0;
        virtual ctl::ComBase* GetTargetNoRef() = 0;
        virtual KnownEventIndex GetHandle() = 0;
    };

    // This interface is used by property path listener hosts
    interface IPropertyPathListenerHost
    {
        virtual _Check_return_ HRESULT GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString) = 0;
        virtual _Check_return_ HRESULT SourceChanged() = 0;
    };

    interface IScrollInfo;

    template<class TEVENT, class THANDLER, class TSOURCE, class TARGS>
    class __declspec(novtable) CEventSourceBase :
        public TEVENT,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(CEventSourceBase, ctl::ComBase)
            INTERFACE_ENTRY(CEventSourceBase, TEVENT)
        END_INTERFACE_MAP(CEventSourceBase, ctl::ComBase)


    public:
        typedef THANDLER HandlerType;
        typedef TSOURCE SenderType;
        typedef TARGS ArgsType;

        // Methods with NOLINT below in some cases are declared in base class as virtual or not declared at all.  It means that they could be virtual or override
        // here.  Leaving as is until it's fixed.

        virtual _Check_return_ HRESULT AddHandler(_In_ THANDLER* pHandler)  // NOLINT(modernize-use-override)
        {
            HRESULT hr = S_OK;

            // TODO (12320763): if we just remove this IFCPTR or convert to failfast, then every AddHandler can't fail...
            IFCPTR(pHandler);

            #if DBG
            m_delegates.Append(pHandler, m_bStatic);
            #else
            m_delegates.Append(pHandler);
            #endif

        Cleanup:

            RRETURN(hr);
        }

        virtual _Check_return_ HRESULT RemoveHandler(_In_ THANDLER* pHandler)  // NOLINT(modernize-use-override)
        {
            HRESULT hr = S_OK;

            IFCPTR(pHandler);
            IFC(m_delegates.Remove(pHandler));

        Cleanup:
            RRETURN(hr);
        }

        // Event Source's just forward the RTW, so only need to define ReferenceTrackerWalk to participate
        bool ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = FALSE)  // NOLINT(modernize-use-override)
        {
            m_delegates.ReferenceTrackerWalk(walkType);
            return true;
        }

        _Check_return_ HRESULT UntypedRaise(_In_opt_ IInspectable* pSource, _In_opt_ IInspectable* pArgs)  // NOLINT(modernize-use-override)
        {
            HRESULT hr = S_OK;
            TSOURCE* pSourceConverted = NULL;
            TARGS* pArgsConverted = NULL;

            if (pSource)
            {
                IFC(pSource->QueryInterface(__uuidof(TSOURCE), (void**)&pSourceConverted));
            }
            if (pArgs)
            {
                IFC(pArgs->QueryInterface(__uuidof(TARGS), (void**)&pArgsConverted));
            }

            IFC(Raise(pSourceConverted, pArgsConverted));

        Cleanup:

            ReleaseInterface(pSourceConverted);
            ReleaseInterface(pArgsConverted);
            RRETURN(hr);
        }


        _Check_return_ HRESULT Raise(_In_opt_ TSOURCE* pSource, _In_opt_ TARGS* pArgs)
        {
            HRESULT hr = S_OK;
            HRESULT invokeHR = S_OK;

            switch (m_delegates.Size())
            {
            case 0:
                // No handlers... Short-circuit.
                break;
            case 1:
                {
                    // One handler... No need to copy things to a temporary list.
                    auto itrDelegate = m_delegates.Begin();
                    ASSERT(itrDelegate != m_delegates.End());
                    ctl::ComPtr<THANDLER> spHandler = (*itrDelegate).Get();

                    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
                    invokeHR = spHandler->Invoke(pSource, pArgs);

                    if (invokeHR == RPC_E_DISCONNECTED || invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                    {
                        // If the event has been disconnected, we swallow the error and remove the handler.
                        // This is consistent with the rest of the WinRT event source implementations.
                        IFC(ErrorHelper::ClearError());
                        IFC(RemoveHandler(spHandler.Get()));
                        invokeHR = S_OK;
                    }

                    IFC(invokeHR);
                }
                break;
            default:
                {
                    // Multiple handlers... Copy to temporary list to avoid re-entrancy issues.
                    std::vector<ctl::ComPtr<THANDLER>> handlers;

                    // Copy the list first to prevent re-entrancy problems
                    std::for_each(m_delegates.Begin(), m_delegates.End(),
                        [&handlers](const TrackerPtr<THANDLER> &tracker) {
                            handlers.push_back(tracker.Get());
                    });


                    // Invoke the handlers
                    for ( auto iterCopy = handlers.cbegin();
                          iterCopy != handlers.cend();
                          ++iterCopy)
                    {
                        auto& spHandler = (*iterCopy);

                        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
                        invokeHR = spHandler->Invoke(pSource, pArgs);

                        if (invokeHR == RPC_E_DISCONNECTED || invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                        {
                            // If the event has been disconnected, we swallow the error and remove the handler.
                            // This is consistent with the rest of the WinRT event source implementations.
                            IFC(ErrorHelper::ClearError());
                            IFC(RemoveHandler(spHandler.Get()));
                            invokeHR = S_OK;
                        }

                        IFC(invokeHR);
                    }
                }
                break;
            }

        Cleanup:

            RRETURN(hr);
        }


        // Gets a value indicating whether the event source has any handlers
        // attached.
        BOOLEAN HasHandlers()
        {
            return m_delegates.Size() > 0;
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CEventSourceBase( )
        {
            #if DBG
            m_bStatic = FALSE;
            #endif
        }

        ~CEventSourceBase() override
        { }

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(TEVENT)))
            {
                *ppObject = static_cast<TEVENT*>(this);
            }
            else
            {
                return ctl::ComBase::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        // The listener delegates are wrapped in a TrackerPtr, so that we can
        // incorporate into the IReferenceTracker lifetime logic.
        TrackerPtrVector<THANDLER> m_delegates;

        #if DBG
        // Indicates if this is a static event
        bool m_bStatic;
        #endif

    };

    // Wrapping this work decouples this header (widely included) from DXamlCore.h (much less commonly needed)
    _Check_return_ HRESULT RegisterUntypedEventSourceInCore(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager);
    _Check_return_ HRESULT UnregisterUntypedEventSourceInCore(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager);

    template<class THANDLER, class TSOURCE, class TARGS>
    class __declspec(novtable) CEventSource:
        public CEventSourceBase<IUntypedEventSource, THANDLER, TSOURCE, TARGS>
    {
        using base_type = CEventSourceBase<IUntypedEventSource, THANDLER, TSOURCE, TARGS>;

    public:
        void Initialize(_In_ KnownEventIndex nEventIndex, _In_ ctl::ComBase* pTarget, _In_ bool bUseEventManager = true
            #if DBG
            , _In_ bool bStatic = false
            #endif
            )
        {
            m_hEventId = nEventIndex;
            m_pTarget = pTarget;
            m_bUseEventManager = bUseEventManager;

            #if DBG
            m_bStatic = bStatic;
            #endif
        }

        void Shutdown()
        {
            this->m_delegates.Clear();
        }

        _Check_return_ HRESULT AddHandler(_In_ THANDLER* pHandler) override
        {
            HRESULT hr = S_OK;

            // Only register our event source when we're adding the first handler.
            // Our target may not be alive anymore when we're shutting down, and our target has been disconnected
            // from its core object.
            if (!this->HasHandlers())
            {
                IFC(RegisterUntypedEventSourceInCore(this, m_bUseEventManager));
            }

            IFC(base_type::AddHandler(pHandler));

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT RemoveHandler(_In_ THANDLER* pHandler) override
        {
            HRESULT hr = S_OK;

            if (this->m_delegates.Size() == 1)
            {
                IFC(Disconnect());
            }

            IFC(base_type::RemoveHandler(pHandler));

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT Disconnect() override
        {
            HRESULT hr = S_OK;

            if (this->HasHandlers())
            {
                IFC(UnregisterUntypedEventSourceInCore(this, m_bUseEventManager));
            }

        Cleanup:
            RRETURN(hr);
        }

        ctl::ComBase* GetTargetNoRef() override
        {
            return m_pTarget;
        }

        KnownEventIndex GetHandle() override
        {
            return m_hEventId;
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CEventSource(  )
        {
        }

        ~CEventSource() override
        {
        }

        using ctl::ComBase::Initialize;

    private:
        ctl::ComBase* m_pTarget;
        KnownEventIndex m_hEventId;
        bool m_bUseEventManager; // Whether the event is a core event that requires AddEventListener/RemoveEventListener registration.
    };

    template <class TEVENT, class THANDLER, class TSOURCE, class TARGS>
    class __declspec(novtable) CFrameworkEventSource:
        public CEventSourceBase<TEVENT, THANDLER, TSOURCE, TARGS>
    {
    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CFrameworkEventSource()
        {
        }
    };

    MIDL_INTERFACE("3fb62e4a-81eb-48e0-bb1d-d5549b91dc54")
    IDPChangedEventHandler: public IUnknown
    {
        IFACEMETHOD(Invoke)(_In_ xaml::IDependencyObject* pSender, _In_ const CDependencyProperty* pDP) = 0;
    };

    MIDL_INTERFACE("af3da0dd-b20e-41cd-a51b-375dcb0b3422")
    IDPChangedEventSource : public virtual IUnknown
    {
        virtual _Check_return_ HRESULT AddHandler(_In_ IDPChangedEventHandler* pHandler) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(_In_ IDPChangedEventHandler* pHandler) = 0;
    };

    // Internal class for the DPChangedEventSource
    class __declspec(novtable) DPChangedEventSource : public CFrameworkEventSource<IDPChangedEventSource, IDPChangedEventHandler, xaml::IDependencyObject, const CDependencyProperty>
    {
    protected:
        // This class is marked novtable, so must not be instantiated directly.
        DPChangedEventSource() = default;
    };

    class __declspec(novtable) InheritanceContextChangedEventSource:
          public CFrameworkEventSource<
            IInheritanceContextChangedEventSource,
            IInheritanceContextChangedEventHandler,
            DependencyObject,
            IInspectable>
    {
        using base_type = CFrameworkEventSource<IInheritanceContextChangedEventSource, IInheritanceContextChangedEventHandler, DependencyObject, IInspectable>;

    public:
        void Initialize(_In_ DependencyObject* pTarget)
        {
            m_pTarget = pTarget;
        }

        _Check_return_ HRESULT AddHandler(_In_ IInheritanceContextChangedEventHandler* pHandler) override
        {
            HRESULT hr = S_OK;

            IFC(base_type::AddHandler(pHandler));

            // If we added our first handler then
            // remember that we want the event
            if (m_delegates.Size() == 1)
            {
                IFC(CoreImports::WantsEvent(m_pTarget->GetHandle(), DirectUI::ManagedEvent::ManagedEventInheritanceContextChanged, true));
            }

        Cleanup:

            RRETURN(hr);
        }

        _Check_return_ HRESULT RemoveHandler(_In_ IInheritanceContextChangedEventHandler* pHandler) override
        {
            HRESULT hr = S_OK;
            int oldSize = m_delegates.Size();

            IFC(base_type::RemoveHandler(pHandler));

            // We removed the last delegate
            if (m_delegates.Size() == 0 && oldSize > 0)
            {
                IFC(CoreImports::WantsEvent(m_pTarget->GetHandle(), DirectUI::ManagedEvent::ManagedEventInheritanceContextChanged, false));
            }

        Cleanup:

            RRETURN(hr);
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        InheritanceContextChangedEventSource()
            : m_pTarget(NULL)
        {
        }

        using ctl::ComBase::Initialize;

    private:
        DependencyObject* m_pTarget;    // This is a weak reference
    };

    template<class TCLASS, class TINTERFACE, class THANDLER, class TSENDER, class TARGS>
    class ClassMemberEventHandler final : public ctl::implements<THANDLER>
    {
    public:
        // Pointer to an event handler member on TCLASS.
        typedef _Check_return_ HRESULT (TCLASS::*FnEventHandler)(TSENDER* pSender, TARGS* pArgs);

    private:
        // If the subscriber is the same instance as the event publisher,
        // we use a raw reference to the subscriber. This not only prevent a circular
        // reference but we also avoid QI-ing for IWeakReferenceSource which can be
        // problematic during construction if ther an outer instance.
        // If the subscriber is different from the publisher, we use a weak reference
        // because it prevents a circular reference (e.g. a control subscribes to
        // an event on a template part).
        // The way we distinguish between the two mode is to look at the least significant
        // bit. If it's 1, we are holding a raw reference. If it's 0, we are holding a weak
        // reference.
        union
        {
            IWeakReference* m_pWeakRef;
            TCLASS* m_eventHandler;
            uintptr_t m_subscribingToSelf;
        };
        static const uintptr_t SubscribingToSelfMask = 0x01;
        static const uintptr_t ExtractEventHandlerMask = ~SubscribingToSelfMask;

        // The event handler that will respond to the event.
        FnEventHandler m_pfnEventHandler;

    public:
        // Initializes a new instance of the ClassMemberEventHandler class.
        ClassMemberEventHandler(
            _In_ ctl::WeakReferenceSourceNoThreadId* pReferenceSource,
            _In_ FnEventHandler pfnEventHandler,
            _In_ bool subscribingToSelf = false)
            : m_pfnEventHandler(pfnEventHandler)
        {
            if (subscribingToSelf)
            {
                // Set the least significant bit to 1 so that we know that we are subscribing on self.
                // This is safe to do because pointers are aligned (4 or 8 bytes aligned) and the least
                // significant bit is guaranteed to be zero.
                m_eventHandler = static_cast<TCLASS*>(pReferenceSource);
                m_subscribingToSelf |= SubscribingToSelfMask;
            }
            else
            {
                VERIFYHR(ctl::as_weakref(m_pWeakRef, ctl::as_iinspectable(pReferenceSource)));
            }
        }

        // Destroys an intstance of the ClassMemberEventHandler class.
        ~ClassMemberEventHandler() override
        {
            if (!IsSubscribingToSelf())
            {
                ReleaseInterface(m_pWeakRef);
            }
        }

        // Handle the event by invoking the TCLASS member.
        IFACEMETHODIMP Invoke(
            _In_ TSENDER* pSender,
            _In_ TARGS* pArgs) final
        {
            HRESULT hr = S_OK;
            TINTERFACE* pInterface = NULL;
            TCLASS* pClass = NULL;

            IFCPTR(m_pfnEventHandler);

            if (IsSubscribingToSelf())
            {
                pClass = reinterpret_cast<TCLASS*>(m_subscribingToSelf & ExtractEventHandlerMask);
            }
            else
            {
                IFC(ctl::resolve_weakref(m_pWeakRef, pInterface));
                pClass = static_cast<TCLASS*>(pInterface);
            }

            {
                auto pegClass = try_make_autopeg(pClass);

                if (pegClass)
                {
                    // Call the virtual OnEvent method
                    IFC((pClass->*m_pfnEventHandler)(pSender, pArgs));
                }
            }

        Cleanup:
            ReleaseInterface(pInterface);
            RRETURN(hr);
        }

    private:
        bool IsSubscribingToSelf() const
        {
            return static_cast<bool>(m_subscribingToSelf & SubscribingToSelfMask);
        }
    };


//REVIEW - see usage in BitmapSource_Partial.cpp line 178
    template<class TCLASS, class TINTERFACE, class THANDLER, class TARGS, class TARGS2>
    class ClassMemberCallback2 final : public ctl::implements<THANDLER>
    {
    public:
        // Pointer to an event handler member on TCLASS.
        // this version, e.g. supports async Completed handlers with 2 args
        typedef _Check_return_ HRESULT (TCLASS::*FnEventHandler)(TARGS* pArgs, TARGS2 arg2);

    protected:
        // The weak reference with the event handler.
        IWeakReference* m_pWeakRef;

        // The event handler that will respond to the event.
        FnEventHandler m_pfnEventHandler;

    public:
        ClassMemberCallback2(
            _In_ ctl::WeakReferenceSourceNoThreadId* pReferenceSource,
            _In_ FnEventHandler pfnEventHandler)
            : m_pfnEventHandler(pfnEventHandler)
        {
            VERIFYHR(ctl::as_weakref(m_pWeakRef, ctl::as_iinspectable(pReferenceSource)));
        }

        ~ClassMemberCallback2() override
        {
            ReleaseInterface(m_pWeakRef);
        }


        // Handle the event by invoking the TCLASS member.
        IFACEMETHODIMP Invoke(
            _In_ TARGS* pArgs,
            _In_ TARGS2 arg2) final
        {
            HRESULT hr = S_OK;
            TINTERFACE* pInterface = NULL;
            TCLASS* pClass = NULL;

            IFCPTR(m_pfnEventHandler);

            IFC(ctl::resolve_weakref(m_pWeakRef, pInterface));
            pClass = static_cast<TCLASS*>(pInterface);

            {
                auto pegClass = try_make_autopeg(pClass);

                if (pegClass)
                {
                    IFC((pClass->*m_pfnEventHandler)(pArgs, arg2));
                }
            }

        Cleanup:
            ReleaseInterface(pInterface);
            RRETURN(hr);
        }
    };

    template<class THANDLER, class TSENDER, class TARGS>
    class StaticMemberEventHandler final : public ctl::implements<THANDLER>
    {
    public:
        // Pointer to an event handler.
        typedef _Check_return_ HRESULT (*FnEventHandler)(TSENDER* pSender, TARGS* pArgs);

    protected:
        // The event handler that will respond to the event.
        FnEventHandler m_pfnEventHandler;

    public:
        // Initializes a new instance of the StaticMemberEventHandler class.
        StaticMemberEventHandler(
            _In_ FnEventHandler pfnEventHandler)
        {
            m_pfnEventHandler = pfnEventHandler;
        }

        // Handle the event by invoking the handler.
        IFACEMETHODIMP Invoke(
            _In_ TSENDER* pSender,
            _In_ TARGS* pArgs) final
        {
            HRESULT hr = S_OK;

            IFCPTR(m_pfnEventHandler);

            IFC((*m_pfnEventHandler)(pSender, pArgs));

        Cleanup:
            RRETURN(hr);
        }
    };

    class CIsEnabledChangedEventSource :
        public CEventSource<xaml::IDependencyPropertyChangedEventHandler,
                            IInspectable,
                            xaml::IDependencyPropertyChangedEventArgs>
    {
        typedef CEventSource<xaml::IDependencyPropertyChangedEventHandler,
                            IInspectable,
                            xaml::IDependencyPropertyChangedEventArgs> BASE;

    public:
        using BASE::Initialize;

        // Overrides CEventSource::UntypedRaise to handle raising IsEnabledChanged specially.
        // pArgs are safely converted from IsEnabledChangedEventArgs to DependencyPropertyChangedEventArgs before
        // calling Raise().
        //
        // CEventSource::UntypedRaise fails for the IsEnabledChanged event because cannot QI from
        // IsEnabledChangedEventArgs, the internal args type, to DependencyPropertyChangedEventArgs.  IsEnabledChanged
        // uses these internal event args which have no COM interface, but the public hander for the IsEnabledChanged
        // event is a normal DependencyPropertyChangedHandler that expects DependencyPropertyChangedEventArgs.
        // Therefore, we need to transmute the IsEnabledChangedEventArgs to DependencyPropertyChangedEventArgs before
        // calling Raise().
        _Check_return_ HRESULT UntypedRaise(
            _In_opt_ IInspectable* source,
            _In_opt_ IInspectable* args) final;

    protected:
        CIsEnabledChangedEventSource()
        {
        }

        using ctl::ComBase::Initialize;
    };

    template<class TEVENT, class THANDLER, class TSOURCE, class TARGS>
    class __declspec(novtable) CRoutedEventSourceBase:
        public TEVENT,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(CRoutedEventSourceBase, ctl::ComBase)
            INTERFACE_ENTRY(CRoutedEventSourceBase, TEVENT)
        END_INTERFACE_MAP(CRoutedEventSourceBase, ctl::ComBase)

    public:
        typedef THANDLER HandlerType;
        typedef TSOURCE SenderType;
        typedef TARGS ArgsType;

        struct HandlerInfo
        {
            ctl::ComPtr<THANDLER> m_spHandler;
            bool m_bHandledToo;
        };

        virtual _Check_return_ HRESULT AddHandler(_In_ THANDLER* pHandler)
        {
            RRETURN(AddHandler(pHandler, FALSE));
        }

        virtual _Check_return_ HRESULT AddHandler(_In_ THANDLER* pHandler, _In_ BOOLEAN bHandledToo)
        {
            m_delegates.Append(pHandler);
            m_handledTooValues.push_back(!!bHandledToo);

            ASSERT(m_delegates.Size() == m_handledTooValues.size());

            //TODO: this function can't fail, can we guarantee that for all derived types to allow this to return void?
            return S_OK; //RRETURN_REMOVAL
        }

        virtual _Check_return_ HRESULT RemoveHandler(_In_ THANDLER* pHandler)
        {
            HRESULT hr = S_OK;
            UINT index = 0;
            auto itrHandler = std::find_if(m_delegates.Begin(), m_delegates.End(),
                [pHandler](const TrackerPtr<THANDLER>& tracker) { return tracker.Get() == pHandler; });

            if (itrHandler == m_delegates.End())
            {
                goto Cleanup;
            }

            index = static_cast<UINT>(itrHandler - m_delegates.Begin());

            ASSERT(index < m_handledTooValues.size());

            IFC(m_delegates.RemoveAt(index));
            m_handledTooValues.erase(index);

            ASSERT(m_delegates.Size() == m_handledTooValues.size());

        Cleanup:
            RRETURN(hr);
        }

        // Event Source's just forward the RTW, so only need to define ReferenceTrackerWalk to participate
        bool ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = FALSE) override
        {
            m_delegates.ReferenceTrackerWalk(walkType);
            return true;
        }

        _Check_return_ HRESULT UntypedRaise(_In_opt_ IInspectable* pSource, _In_opt_ IInspectable* pArgs) override
        {
            HRESULT hr = S_OK;
            TSOURCE* pSourceConverted = NULL;
            TARGS* pArgsConverted = NULL;

            if (pSource)
            {
                IFC(pSource->QueryInterface(__uuidof(TSOURCE), (void**)&pSourceConverted));
            }
            if (pArgs)
            {
                IFC(pArgs->QueryInterface(__uuidof(TARGS), (void**)&pArgsConverted));
            }

            IFC(Raise(pSourceConverted, pArgsConverted));

        Cleanup:

            ReleaseInterface(pSourceConverted);
            ReleaseInterface(pArgsConverted);
            RRETURN(hr);
        }


        _Check_return_ HRESULT Raise(_In_opt_ TSOURCE* pSource, _In_opt_ TARGS* pArgs)
        {
            HRESULT hr = S_OK;
            HRESULT invokeHR = S_OK;
            ctl::ComPtr<xaml::IRoutedEventArgs> spRoutedEventArgs;
            bool bHandled = false;

            if (!m_isEnabled)
            {
                goto Cleanup;
            }

            switch (m_delegates.Size())
            {
            case 0:
                // No handlers... Short-circuit.
                break;
            case 1:
                {
                    // One handler... No need to copy things to a temporary list.
                    auto itrDelegate = m_delegates.Begin();
                    size_t itrHandledValue = 0;
                    ASSERT(itrDelegate != m_delegates.End());
                    ASSERT(itrHandledValue < m_handledTooValues.size());
                    ctl::ComPtr<THANDLER> spHandler = (*itrDelegate).Get();

                    if (!m_handledTooValues[itrHandledValue])
                    {
                        IFC(ctl::do_query_interface(spRoutedEventArgs, pArgs));
                        IFC(spRoutedEventArgs.Cast<RoutedEventArgs>()->IsHandled(&bHandled));
                        if (bHandled)
                        {
                            goto Cleanup;
                        }
                    }

                    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
                    invokeHR = spHandler->Invoke(pSource, pArgs);

                    if (invokeHR == RPC_E_DISCONNECTED || invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                    {
                        // If the event has been disconnected, we swallow the error and remove the handler.
                        // This is consistent with the rest of the WinRT event source implementations.
                        IFC(ErrorHelper::ClearError());
                        IFC(RemoveHandler(spHandler.Get()));
                        invokeHR = S_OK;
                    }

                    IFC(invokeHR);
                }
                break;
            default:
                {
                    // Multiple handlers... Copy to temporary list to avoid re-entrancy issues.
                    std::vector<HandlerInfo> handlers;

                    size_t itrHandledValue = 0;

                    for (auto itrDelegate = m_delegates.Begin();
                         itrDelegate != m_delegates.End();
                         itrDelegate++, itrHandledValue++)
                    {
                        ASSERT(itrHandledValue < m_handledTooValues.size());
                        HandlerInfo handlerInfo = {(*itrDelegate).Get(), m_handledTooValues[itrHandledValue]};
                        handlers.push_back(handlerInfo);
                    }

                    // Invoke the handlers
                    for ( auto iterCopy = handlers.begin();
                          iterCopy != handlers.end();
                          ++iterCopy)
                    {
                        HandlerInfo handlerInfo = *iterCopy;
                        THANDLER *pHandlerNoRef = handlerInfo.m_spHandler.Get();

                        IFCPTR(pHandlerNoRef);

                        if (!(handlerInfo.m_bHandledToo))
                        {
                            if (!spRoutedEventArgs)
                            {
                                IFC(ctl::do_query_interface(spRoutedEventArgs, pArgs));
                            }
                            IFC(spRoutedEventArgs.Cast<RoutedEventArgs>()->IsHandled(&bHandled));
                            if (bHandled)
                            {
                                continue;
                            }
                        }

                        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
                        invokeHR = pHandlerNoRef->Invoke(pSource, pArgs);

                        if (invokeHR == RPC_E_DISCONNECTED || invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                        {
                            // If the event has been disconnected, we swallow the error and remove the handler.
                            // This is consistent with the rest of the WinRT event source implementations.
                            IFC(ErrorHelper::ClearError());
                            IFC(RemoveHandler(pHandlerNoRef));
                            invokeHR = S_OK;
                        }

                        IFC(invokeHR);
                    }
                }
                break;
            }

        Cleanup:

            RRETURN(hr);
        }


        // Gets a value indicating whether the event source has any handlers
        // attached.
        BOOLEAN HasHandlers()
        {
            return m_delegates.Size() > 0;
        }

        // Return True if the requested handler is available in the delegate list.
        BOOLEAN HasHandler(_In_ THANDLER* pHandler)
        {
            auto itrHandler = std::find_if(m_delegates.Begin(), m_delegates.End(),
                [pHandler](const TrackerPtr<THANDLER>& tracker) { return tracker.Get() == pHandler; });

            return itrHandler != m_delegates.End();
        }

        void SetIsEnabled(bool isEnabled)
        {
            m_isEnabled = isEnabled;
        }

    protected:
        ~CRoutedEventSourceBase() override
        { }

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) final
        {
            if (InlineIsEqualGUID(iid, __uuidof(TEVENT)))
            {
                *ppObject = static_cast<TEVENT*>(this);
            }
            else
            {
                return ctl::ComBase::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        // The listener delegates are wrapped in a TrackerPtr, so that we can
        // incorporate into the IReferenceTracker lifetime logic.

        TrackerPtrVector<THANDLER> m_delegates;
        containers::bit_vector m_handledTooValues;

        // Don't raise events if disabled
        bool m_isEnabled = true;
    };

    template<class THANDLER, class TSOURCE, class TARGS>
    class __declspec(novtable) CRoutedEventSource :
        public CRoutedEventSourceBase<IUntypedEventSource, THANDLER, TSOURCE, TARGS>
    {
        using base_type = CRoutedEventSourceBase<IUntypedEventSource, THANDLER, TSOURCE, TARGS>;

    public:
        void Initialize(_In_ KnownEventIndex nEventIndex, _In_ ctl::ComBase* pTarget, _In_ bool bUseEventManager = true)
        {
            m_hEventId = nEventIndex;
            m_pTarget = pTarget;
            m_bUseEventManager = bUseEventManager;
        }

        _Check_return_ HRESULT AddHandler(_In_ THANDLER* pHandler) final
        {
            HRESULT hr = S_OK;

            if (!this->HasHandlers())
            {
                IFC(RegisterUntypedEventSourceInCore(this, m_bUseEventManager));
            }

            IFC(base_type::AddHandler(pHandler, FALSE));

        Cleanup:
            return hr;
        }

        _Check_return_ HRESULT AddHandler(_In_ THANDLER* pHandler, _In_ BOOLEAN bHandledToo) final
        {
            HRESULT hr = S_OK;

            if (!this->HasHandlers())
            {
                IFC(RegisterUntypedEventSourceInCore(this, m_bUseEventManager));
            }

            IFC(base_type::AddHandler(pHandler, bHandledToo));

        Cleanup:
            return hr;
        }

        _Check_return_ HRESULT RemoveHandler(_In_ THANDLER* pHandler) final
        {
            HRESULT hr = S_OK;

            // Unregister the event source when the current delegate size is one
            // and the requested handler is available in the delegate list.
            if (this->m_delegates.Size() == 1 && this->HasHandler(pHandler))
            {
                IFC(Disconnect());
            }

            IFC(base_type::RemoveHandler(pHandler));

        Cleanup:
            return hr;
        }

        _Check_return_ HRESULT Disconnect() override
        {
            HRESULT hr = S_OK;

            if (this->HasHandlers())
            {
                IFC(UnregisterUntypedEventSourceInCore(this, m_bUseEventManager));
            }

        Cleanup:
            RRETURN(hr);
        }

        ctl::ComBase* GetTargetNoRef() override
        {
            return m_pTarget;
        }

        KnownEventIndex GetHandle() override
        {
            return m_hEventId;
        }

    protected:
        CRoutedEventSource()
        {
        }

        ~CRoutedEventSource() override
        {
        }

        using ctl::ComBase::Initialize;

    private:
        ctl::ComBase* m_pTarget;
        KnownEventIndex m_hEventId;
        bool m_bUseEventManager{}; // Whether the event is an event that uses CEventManager.
    };

    template<class THANDLER, class TSENDER, class TARGS>
    class CForwardEventSource final : public ctl::implements<THANDLER>
    {
    public:
        typedef THANDLER HandlerType;
        typedef TSENDER SenderType;
        typedef TARGS ArgsType;

        HRESULT Initialize(_In_ HandlerType* pHandler, _In_ SenderType* pSender)
        {
            m_spHandler = pHandler;
            m_spSender = pSender;
            return S_OK;
        }

        IFACEMETHODIMP Invoke(_In_ SenderType*, _In_ ArgsType* pArgs) final
        {
            return m_spHandler->Invoke(m_spSender.Get(), pArgs);
        }
    private:
        ctl::ComPtr<HandlerType> m_spHandler;
        ctl::ComPtr<SenderType> m_spSender;
    };
}
