// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PLMIntegration.h"
#include "FrameworkApplication.g.h"
#include "Callback.h"
#include <WRLHelper.h>
#include <MetadataAPI.h>
#include <windows.ui.core.corewindow-defs.h>

//  from RuntimeProfiler.cpp
STDAPI_(void) SendTelemetryOnSuspend();

using DirectUI::DXamlCore;

namespace XAML
{

namespace PLM
{
    template <class THandlerType, class TArg1Type, class TArg2Type>
    _Check_return_ HRESULT MTAEventSource<THandlerType, TArg1Type, TArg2Type>::AddHandler(_In_ THandlerType* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        return m_handlers.Add(pHandler, pToken);
    }

    template <class THandlerType, class TArg1Type, class TArg2Type>
    _Check_return_ HRESULT MTAEventSource<THandlerType, TArg1Type, TArg2Type>::RemoveHandler(EventRegistrationToken token)
    {
        return m_handlers.Remove(token);
    }

    template <class THandlerType, class TArg1Type, class TArg2Type>
    _Check_return_ HRESULT MTAEventSource<THandlerType, TArg1Type, TArg2Type>::InvokeHandlers(_In_ TArg1Type* pArg1, _In_ TArg2Type* pArg2)
    {
        return m_handlers.Raise(pArg1, pArg2);
    }

    template <class THandlerType, class TArg1Type, class TArg2Type>
    _Check_return_ HRESULT ASTAEventSource<THandlerType, TArg1Type, TArg2Type>::AddHandler(_In_ THandlerType* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        return m_handlers.Add(pHandler, pToken);
    }

    template <class THandlerType, class TArg1Type, class TArg2Type>
    _Check_return_ HRESULT ASTAEventSource<THandlerType, TArg1Type, TArg2Type>::RemoveHandler(EventRegistrationToken token)
    {
        return m_handlers.Remove(token);
    }

    template <class THandlerType, class TArg1Type, class TArg2Type>
    _Check_return_ HRESULT ASTAEventSource<THandlerType, TArg1Type, TArg2Type>::InvokeHandlers(_In_ TArg1Type* pArg1, _In_ TArg2Type* pArg2)
    {
        m_handlers.InvokeAll(pArg1, pArg2);
        RRETURN(S_OK);
    }

    // Creates a PLMHandler suitable for use in an ASTA.
    _Check_return_ HRESULT PLMHandler::CreateForASTA(_In_opt_ IPLMHandlerCallbacks* pCallbacks, _Outptr_ PLMHandler** ppHandler)
    {
        return Create(TRUE /* fForASTA */, pCallbacks, ppHandler);
    }

    // Creates a PLMHandler suitable for use in the ASTA.
    _Check_return_ HRESULT PLMHandler::CreateForMTA(_In_opt_ IPLMHandlerCallbacks* pCallbacks, _Outptr_ PLMHandler** ppHandler)
    {
        return Create(FALSE /* fForASTA */, pCallbacks, ppHandler);
    }

    _Check_return_ HRESULT PLMHandler::Create(bool fForASTA, _In_opt_ IPLMHandlerCallbacks* pCallbacks, _Outptr_ PLMHandler** ppHandler)
    {
        HRESULT hr = S_OK;
        PLMHandler* pHandler = NULL;

        *ppHandler = NULL;

        pHandler = new PLMHandler(pCallbacks);

        // Create the event sources, either ASTA- or MTA-compatible, depending on the parameter.
        if (fForASTA)
        {
            pHandler->m_pAppSuspendHandlers = std::unique_ptr<IEventSource<xaml::ISuspendingEventHandler, IInspectable, wa::ISuspendingEventArgs>>(new
                ASTAEventSource<xaml::ISuspendingEventHandler, IInspectable, wa::ISuspendingEventArgs>());
        }
        else
        {
            pHandler->m_pAppSuspendHandlers = std::unique_ptr<IEventSource<xaml::ISuspendingEventHandler, IInspectable, wa::ISuspendingEventArgs>>( new
                MTAEventSource<xaml::ISuspendingEventHandler, IInspectable, wa::ISuspendingEventArgs>());
        }

        if (fForASTA)
        {
            pHandler->m_pAppResumeHandlers = std::unique_ptr<IEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>>(new
                ASTAEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>());
        }
        else
        {
            pHandler->m_pAppResumeHandlers = std::unique_ptr<IEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>>(new
                MTAEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>());
        }

        // Create the event sources, either ASTA- or MTA-compatible, depending on the parameter.
        if (fForASTA)
        {
            pHandler->m_pAppLeavingBackgroundHandlers = std::unique_ptr<IEventSource<xaml::ILeavingBackgroundEventHandler, xaml::IApplication, wa::ILeavingBackgroundEventArgs>>(new
                ASTAEventSource<xaml::ILeavingBackgroundEventHandler, xaml::IApplication, wa::ILeavingBackgroundEventArgs>());
        }
        else
        {
            pHandler->m_pAppLeavingBackgroundHandlers = std::unique_ptr<IEventSource<xaml::ILeavingBackgroundEventHandler, xaml::IApplication, wa::ILeavingBackgroundEventArgs>>(new
                MTAEventSource<xaml::ILeavingBackgroundEventHandler, xaml::IApplication, wa::ILeavingBackgroundEventArgs>());
        }

        // Create the event sources, either ASTA- or MTA-compatible, depending on the parameter.
        if (fForASTA)
        {
            pHandler->m_pAppEnteredBackgroundHandlers = std::unique_ptr<IEventSource<xaml::IEnteredBackgroundEventHandler, xaml::IApplication, wa::IEnteredBackgroundEventArgs>>(new
                ASTAEventSource<xaml::IEnteredBackgroundEventHandler, xaml::IApplication, wa::IEnteredBackgroundEventArgs>());
        }
        else
        {
            pHandler->m_pAppEnteredBackgroundHandlers = std::unique_ptr<IEventSource<xaml::IEnteredBackgroundEventHandler, xaml::IApplication, wa::IEnteredBackgroundEventArgs>>(new
                MTAEventSource<xaml::IEnteredBackgroundEventHandler, xaml::IApplication, wa::IEnteredBackgroundEventArgs>());
        }
        pHandler->m_pAppEnteredBackgroundHandlers;

        // Register for PLM callbacks.
        IFC(pHandler->RegisterWithPLM());

        *ppHandler = pHandler;
        pHandler = NULL;

    Cleanup:
        delete pHandler;
        RRETURN(hr);
    }

    PLMHandler::PLMHandler(_In_opt_ IPLMHandlerCallbacks* pCallbacks) :
        m_pCallbacks(pCallbacks),
        m_cAppSuspendActivityCount(0),
        m_pDeferral(nullptr)
    {
        m_suspendingToken.value = 0;
        m_resumingToken.value = 0;
        m_enteredBackgroundToken.value = 0;
        m_leavingBackgroundToken.value = 0;
    }

    PLMHandler::~PLMHandler()
    {
        VERIFYHR(UnregisterWithPLM());
        m_pCallbacks = nullptr;
    }

    _Check_return_ HRESULT PLMHandler::AddAppSuspendHandler(_In_ xaml::ISuspendingEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        return m_pAppSuspendHandlers->AddHandler(pHandler, pToken);
    }

    _Check_return_ HRESULT PLMHandler::RemoveAppSuspendHandler(EventRegistrationToken token)
    {
        return m_pAppSuspendHandlers->RemoveHandler(token);
    }

    _Check_return_ HRESULT PLMHandler::AddAppResumeHandler(_In_ wf::IEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        return m_pAppResumeHandlers->AddHandler(pHandler, pToken);
    }

    _Check_return_ HRESULT PLMHandler::RemoveAppResumeHandler(EventRegistrationToken token)
    {
        return m_pAppResumeHandlers->RemoveHandler(token);
    }

    _Check_return_ HRESULT PLMHandler::AddAppLeavingBackgroundHandler(_In_ xaml::ILeavingBackgroundEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        return m_pAppLeavingBackgroundHandlers->AddHandler(pHandler, pToken);
    }

    _Check_return_ HRESULT PLMHandler::RemoveAppLeavingBackgroundHandler(EventRegistrationToken token)
    {
        return m_pAppLeavingBackgroundHandlers->RemoveHandler(token);
    }

    _Check_return_ HRESULT PLMHandler::AddAppEnteredBackgroundHandler(_In_ xaml::IEnteredBackgroundEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        return m_pAppEnteredBackgroundHandlers->AddHandler(pHandler, pToken);
    }

    _Check_return_ HRESULT PLMHandler::RemoveAppEnteredBackgroundHandler(EventRegistrationToken token)
    {
        return m_pAppEnteredBackgroundHandlers->RemoveHandler(token);
    }

    _Check_return_ HRESULT PLMHandler::IncrementAppSuspendActivityCount()
    {
        HRESULT hr = S_OK;

        InterlockedIncrement(&m_cAppSuspendActivityCount);

        RRETURN(hr);
    }

    _Check_return_ HRESULT PLMHandler::DecrementAppSuspendActivityCount()
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;

        LONG count = InterlockedDecrement(&m_cAppSuspendActivityCount);
        if (count > 0)
        {
            goto Cleanup;
        }

        // App suspend activity is done. If we have callbacks, we need to
        // invoke OnAfterAppSuspend (possibly posting to get on the correct thread).

        if (m_pCallbacks)
        {
            IFC(m_pCallbacks->GetXamlDispatcher(&spDispatcher));

            if (spDispatcher)
            {
                if (spDispatcher->OnDispatcherThread())
                {
                    IFC(InvokeAfterAppSuspendCallback());
                }
                else
                {
                    IFC(spDispatcher->RunAsync(
                        DirectUI::MakeCallback(
                            this,
                            &PLMHandler::InvokeAfterAppSuspendCallback)));
                }

                goto Cleanup;
            }
        }

        // We don't have callbacks, so we need to complete suspension directly.
        CompleteXamlSuspendActivity();

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT PLMHandler::RegisterWithPLM()
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HStringReference coreApplicationAcid(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
        ctl::ComPtr<wac::ICoreApplication> spCoreApplication = NULL;
        ctl::ComPtr<wac::ICoreApplication2> spCoreApplication2 = NULL;

        IFC(ctl::GetActivationFactory(coreApplicationAcid.Get(), &spCoreApplication));
        IFC(spCoreApplication.As(&spCoreApplication2));

        IFC(spCoreApplication->add_Suspending(
            Microsoft::WRL::Callback<wf::IEventHandler<wa::SuspendingEventArgs*>>(
                this, &PLMHandler::OnSuspending).Get(),
            &m_suspendingToken));

        IFC(spCoreApplication->add_Resuming(
            Microsoft::WRL::Callback<wf::IEventHandler<IInspectable*>>(
                this, &PLMHandler::OnResuming).Get(),
            &m_resumingToken));

        IFC(spCoreApplication2->add_LeavingBackground(
            Microsoft::WRL::Callback<wf::IEventHandler<wa::LeavingBackgroundEventArgs*>>(
                this, &PLMHandler::OnLeavingBackground).Get(),
                &m_leavingBackgroundToken));

        IFC(spCoreApplication2->add_EnteredBackground(
            Microsoft::WRL::Callback<wf::IEventHandler<wa::EnteredBackgroundEventArgs*>>(
                this, &PLMHandler::OnEnteredBackground).Get(),
                &m_enteredBackgroundToken));
    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT PLMHandler::UnregisterWithPLM()
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HStringReference coreApplicationAcid(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
        ctl::ComPtr<wac::ICoreApplication> spCoreApplication;
        ctl::ComPtr<wac::ICoreApplication2> spCoreApplication2;

        IFC(ctl::GetActivationFactory(coreApplicationAcid.Get(), &spCoreApplication));
        IFC(spCoreApplication.As(&spCoreApplication2));

        if (m_suspendingToken.value)
        {
            IFC(spCoreApplication->remove_Suspending(m_suspendingToken));
            m_suspendingToken.value = 0;
        }

        if (m_resumingToken.value)
        {
            IFC(spCoreApplication->remove_Resuming(m_resumingToken));
            m_resumingToken.value = 0;
        }

        if (m_enteredBackgroundToken.value)
        {
            IFC(spCoreApplication2->remove_EnteredBackground(m_enteredBackgroundToken));
            m_enteredBackgroundToken.value = 0;
        }

        if (m_leavingBackgroundToken.value)
        {
            IFC(spCoreApplication2->remove_LeavingBackground(m_leavingBackgroundToken));
            m_leavingBackgroundToken.value = 0;
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT PLMHandler::OnSuspending(_In_ IInspectable* pSender, _In_ wa::ISuspendingEventArgs* pArgs)
    {
        ctl::ComPtr<wa::ISuspendingOperation> spSuspendingOperation;
        ctl::ComPtr<wa::ISuspendingEventArgs> spWrappedArgs;
        wrl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;
        wrl::ComPtr<msy::IDispatcherQueue> spDispatcherQueue;
        ctl::ComPtr<wf::IAsyncOperation<bool>> spAsyncOperation;
        bool decrementSynchronously = false;

        TraceApplicationSuspendBegin();

        auto guard = wil::scope_exit([]()
        {
            TraceApplicationSuspendEnd();
        });


        // take out a deferral that will keep this ASTA alive until we've completed our work
        IFC_RETURN(pArgs->get_SuspendingOperation(&spSuspendingOperation));
        IFC_RETURN(spSuspendingOperation->GetDeferral(&m_pDeferral));

        // start off the app activity count at 1 - this represents the activity of invoking the app handlers
        m_cAppSuspendActivityCount = 1;

        // Notify the IPLMHandlerCallbacks that we're about to execute app code for suspend.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnBeforeAppSuspend());
        }

        // Invoke app suspend handlers.
        // Use Application, not CoreApplication, as the sender. Wrap the args so we can intercept deferral requests.
        IInspectable* pAppSender = ctl::as_iinspectable(DirectUI::FrameworkApplication::GetCurrentNoRef());
        IFC_RETURN(SuspendingEventArgsWrapper::Wrap(pArgs, this, &spWrappedArgs));
        IFC_RETURN(m_pAppSuspendHandlers->InvokeHandlers(pAppSender, spWrappedArgs.Get()));

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            &spDispatcherQueueStatics));

        IFC_RETURN(spDispatcherQueueStatics->GetForCurrentThread(&spDispatcherQueue));

        if (spDispatcherQueue != nullptr)
        {
            // If none of the Suspending event handlers had called GetDeferral then decrementing the internal count
            // will lead to completion of a deferral object (and sets suspending event to exit wait loop).
            //
            // However if an app has scheduled a callback in Suspending event handler and calls GetDeferral from within,
            // it will lead to AV when CoreDispatcher dispatches its pending queue before exiting Suspend wait loop.
            //
            // Thus scheduling a low (i.e. Idle) priority callback on CoreDispatcher which will be dispatched after all the pending tasks/
            // messages gets flushed and then safely decrement the internal count. We will make a call to TryEnqueueWithPriority as
            // there could be a potential chance that Window is already destroyed in which case async task will not be invoked
            // and we have to decrement the count synchronously.
            boolean enqueued;

            IFC_RETURN(spDispatcherQueue->TryEnqueueWithPriority(
                msy::DispatcherQueuePriority::DispatcherQueuePriority_Low,
                WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([this]() mutable -> HRESULT
                    {
                        // this decrement corresponds with the initial set of the count to 1
                        return this->DecrementAppSuspendActivityCount();
                    }).Get(),
                &enqueued));

            // 'enqueued' will be set to false synchronously if the above task fails to
            // schedule. This could be because CoreWindow is already destroyed. In that case it is safe and necessary to
            // decrement the count synchronously.
            if (!enqueued)
            {
                decrementSynchronously = true;
            }
        }
        else
        {
            // If no dispatcher is running on this thread that implies that this is invoked for MTA handlers and they
            // don't use dispatcher pump to wait for outstanding deferrals(instead they use CoWaitForMultipleHandles).
            // So decrement the count synchronously.
            decrementSynchronously = true;
        }

        if (decrementSynchronously)
        {
            IFC_RETURN(this->DecrementAppSuspendActivityCount());
        }

        // Calling telemetry code that will fire telemetry event on profiled
        // classes.
        ::SendTelemetryOnSuspend();         //  Internal to this binary
        ExtensionTelemetryProc(NULL, nullptr);  //  MUX Extensions, NULL HModule -> not caching, so suspending.

        return S_OK;
    }

    _Check_return_ HRESULT PLMHandler::InvokeAfterAppSuspendCallback()
    {
        HRESULT hr = S_OK;

        if (m_pCallbacks)
        {
            IFC(m_pCallbacks->OnAfterAppSuspend());
        }

        CompleteXamlSuspendActivity();

    Cleanup:
        //
        // If we hit an error during suspend that would prevent us from completing the deferral,
        // fail fast now.
        //
        // If we don't fail fast, PLM will terminate us anyway. If we fail fast, the ErrorContext
        // mechanism can be used to obtain a failure callstack of the original error that led to
        // a failure at this point.
        //
        if (FAILED(hr))
        {
            FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hr);
        }

        RRETURN(hr);
    }

    void PLMHandler::CompleteXamlSuspendActivity()
    {
        if (m_pDeferral != nullptr)
        {
            ErrorContextLogAccumulatedErrors(ErrorContextLog::ErrorContextLogType::Suspend);

            //
            // We're now completely finished with suspend work on this thread, so complete and release our deferral.
            //
            HRESULT hr = m_pDeferral->Complete();

            //
            // If completing the deferral fails, fail fast.
            //
            // If we don't fail fast, PLM will terminate us anyway. If we fail fast, the ErrorContext
            // mechanism will generate a failure callstack that points to this location.
            //
            if (FAILED(hr))
            {
                FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hr);
            }

            ReleaseInterface(m_pDeferral);
        }
    }

    typedef void (STDAPICALLTYPE *PFNSENDTELEMETRY)(void);

    void PLMHandler::ExtensionTelemetryProc(_In_opt_ const HMODULE hModule, _In_ const wchar_t * modulename)
    {
        //  List of exports to MUX Extension binaries
        static PFNSENDTELEMETRY     apfnSendTelemetry[WUXExtensionListSize] = { 0 };  //  nullptr initialize this static array

        //  Cycle through extensions
        if (NULL == hModule)
        {
            //  We're doing a suspension run.  Call each loaded extension
            for (int ii = 0; ii < ARRAYSIZE(apfnSendTelemetry); ii++)
            {
                if (nullptr != apfnSendTelemetry[ii])
                {
                    (apfnSendTelemetry[ii])();
                }
            }
        }
        else
        {
            //  Sanity check the module name...
            for (int ii = 0; ii < ARRAYSIZE(apfnSendTelemetry); ii++)
            {
                if (0 == lstrcmpiW(s_pWUXExtensions[ii], modulename))
                {
                    PFNSENDTELEMETRY    pfn = (PFNSENDTELEMETRY)(GetProcAddress(hModule, "SendTelemetryOnSuspend"));

                    if (NULL != pfn)
                    {
                        apfnSendTelemetry[ii] = pfn;
                    }

                    return;
                }
            }
        }
    }

    _Check_return_ HRESULT PLMHandler::OnResuming(_In_ IInspectable* pSender, _In_ IInspectable* pArg)
    {
        TraceApplicationResumeBegin();

        auto guard = wil::scope_exit([]()
        {
            TraceApplicationResumeEnd();
        });

        // Notify the IPLMHandlerCallbacks that we're about to execute app code for resume.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnBeforeAppResume());
        }

        // Invoke app suspend handlers.
        // Use Application, not CoreApplication, as the sender.
        IInspectable* pAppSender = ctl::as_iinspectable(DirectUI::FrameworkApplication::GetCurrentNoRef());
        IFC_RETURN(m_pAppResumeHandlers->InvokeHandlers(pAppSender, pArg));

        // Notify the IPLMHandlerCallbacks that we've finished executing app code for resume.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnAfterAppResume());
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SuspendingDeferralWrapper::Complete()
    {
        HRESULT hr = S_OK;

        // Complete the wrapped deferral
        IFC(GetWrapped()->Complete());

        // If that succeeded, notify the PLM handler that the app completed a deferral
        IFC(m_pHandler->DecrementAppSuspendActivityCount());

    Cleanup:
        RRETURN(hr);
    }

   _Check_return_ HRESULT PLMHandler::OnLeavingBackground(_In_ IInspectable* pSender, _In_ wa::ILeavingBackgroundEventArgs* pArgs)
   {
        // Return immediately if there is no DXamlCore. This is a work-around for RS1 Bug #6940053 where
        // our event handlers are called after the PLMHandler instance has been destructed.
        if (!DXamlCore::GetCurrentNoCreate())
        {
            return S_OK;
        }

        // Notify the IPLMHandlerCallbacks that we're about to execute app code for LeavingBackground.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnBeforeAppLeavingBackground());
        }

        // Use Application, not CoreApplication, as the sender.
        IFC_RETURN(m_pAppLeavingBackgroundHandlers->InvokeHandlers(DirectUI::FrameworkApplication::GetCurrentNoRef(), pArgs));

        // Notify the IPLMHandlerCallbacks that we've finished executing app code for LeavingBackground.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnAfterAppLeavingBackground());
        }

        return S_OK;
   }

   _Check_return_ HRESULT PLMHandler::OnEnteredBackground(_In_ IInspectable* pSender, _In_ wa::IEnteredBackgroundEventArgs* pArgs)
   {
        // Return immediately if there is no DXamlCore. This is a work-around for RS1 Bug #6940053 where
        // our event handlers are called after the PLMHandler instance has been destructed.
        if (!DXamlCore::GetCurrentNoCreate())
        {
            return S_OK;
        }

        // Notify the IPLMHandlerCallbacks that we're about to execute app code for EnteredBackground.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnBeforeAppEnteredBackground());
        }

        // Use Application, not CoreApplication, as the sender.
        IFC_RETURN(m_pAppEnteredBackgroundHandlers->InvokeHandlers(DirectUI::FrameworkApplication::GetCurrentNoRef(), pArgs));

        // Notify the IPLMHandlerCallbacks that we've finished executing app code for EnteredBackground.
        if (m_pCallbacks)
        {
            IFC_RETURN(m_pCallbacks->OnAfterAppEnteredBackground());
        }

        return S_OK;
   }

    HRESULT STDMETHODCALLTYPE SuspendingOperationWrapper::GetDeferral(
        /* [out][retval] */ __RPC__deref_out_opt wa::ISuspendingDeferral** ppDeferral)
    {
        HRESULT hr = S_OK;
        wa::ISuspendingDeferral* pDeferral = NULL;
        wa::ISuspendingDeferral* pWrappedDeferral = NULL;

        // Request a deferral from our wrapped SuspendingOperation
        IFC(GetWrapped()->GetDeferral(&pDeferral));

        // Wrap the deferral so we can intercept when the app completes it
        IFC(SuspendingDeferralWrapper::Wrap(pDeferral, m_pHandler, &pWrappedDeferral));

        // Notify the PLM handler that the app has requested a deferral
        IFC(m_pHandler->IncrementAppSuspendActivityCount());

        *ppDeferral = pWrappedDeferral;
        pWrappedDeferral = NULL;

    Cleanup:
        if (FAILED(hr) && pDeferral)
        {
            VERIFYHR(pDeferral->Complete());
        }
        ReleaseInterface(pDeferral);
        ReleaseInterface(pWrappedDeferral);

        RRETURN(hr);
    }

    HRESULT STDMETHODCALLTYPE SuspendingEventArgsWrapper::get_SuspendingOperation(
        /* [out][retval] */ __RPC__deref_out_opt wa::ISuspendingOperation** ppValue)
    {
        HRESULT hr = S_OK;
        wa::ISuspendingOperation* pSuspendingOperation = NULL;

        IFC(GetWrapped()->get_SuspendingOperation(&pSuspendingOperation));
        IFC(SuspendingOperationWrapper::Wrap(pSuspendingOperation, m_pHandler, ppValue));

    Cleanup:
        ReleaseInterface(pSuspendingOperation);

        RRETURN(hr);
    }

}

}

void STDAPICALLTYPE CacheExtensionSuspensionCallback(_In_opt_ const HMODULE hModule, _In_ const wchar_t * modulename)
{
    XAML::PLM::PLMHandler::ExtensionTelemetryProc(hModule, modulename);  //  non-NULL hModule -> caching...
}