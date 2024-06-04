// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FTMEventSource.h"

namespace XAML
{

namespace PLM
{
    // IEventSource - defines basic add/remove/invoke event source functionality
    template <class THandlerType, class TArg1Type, class TArg2Type> class IEventSource
    {
    public:
        virtual _Check_return_ HRESULT AddHandler(_In_ THandlerType* pHandler, _Out_ EventRegistrationToken* pToken) = 0;
        virtual _Check_return_ HRESULT RemoveHandler(EventRegistrationToken token) = 0;
        virtual _Check_return_ HRESULT InvokeHandlers(_In_ TArg1Type* pArg1, _In_ TArg2Type* pArg2) = 0;
        virtual ~IEventSource() { }
    };

    // implementation of IEventSource suitable for use in an ASTA
    template <class THandlerType, class TArg1Type, class TArg2Type> class ASTAEventSource : public IEventSource<THandlerType, TArg1Type, TArg2Type>
    {
    public:
        _Check_return_ HRESULT AddHandler(_In_ THandlerType* pHandler, _Out_ EventRegistrationToken* pToken) override;
        _Check_return_ HRESULT RemoveHandler(EventRegistrationToken token) override;
        _Check_return_ HRESULT InvokeHandlers(_In_ TArg1Type* pArg1, _In_ TArg2Type* pArg2) override;
    private:
        Microsoft::WRL::EventSource<THandlerType> m_handlers;
    };

    // implementation of IEventSource suitable for use in the MTA
    template <class THandlerType, class TArg1Type, class TArg2Type> class MTAEventSource : public IEventSource<THandlerType, TArg1Type, TArg2Type>
    {
    public:
        _Check_return_ HRESULT AddHandler(_In_ THandlerType* pHandler, _Out_ EventRegistrationToken* pToken) override;
        _Check_return_ HRESULT RemoveHandler(EventRegistrationToken token) override;
        _Check_return_ HRESULT InvokeHandlers(_In_ TArg1Type* pArg1, _In_ TArg2Type* pArg2) override;
    private:
        DirectUI::CFTMEventSource<THandlerType, TArg1Type, TArg2Type> m_handlers;
    };

    // This is implemented to receive callbacks at various points in the PLM suspend/resume sequence.
    class IPLMHandlerCallbacks
    {
    public:
        
        // Called during suspend before any app code has run.
        virtual _Check_return_ HRESULT OnBeforeAppSuspend()
        {
            RRETURN(S_OK);
        }

        // Called during suspend after all app code has finished.
        virtual _Check_return_ HRESULT OnAfterAppSuspend()
        {
            RRETURN(S_OK);
        }

        // Called during resume before any app code has run.
        virtual _Check_return_ HRESULT OnBeforeAppResume()
        {
            RRETURN(S_OK);
        }

        // Called during resume after all app code has finished.
        virtual _Check_return_ HRESULT OnAfterAppResume()
        {
            RRETURN(S_OK);
        }

        // Called during LeavingBackground before any app code has run.
        virtual _Check_return_ HRESULT OnBeforeAppLeavingBackground()
        {
            RRETURN(S_OK);
        }

        // Called during LeavingBackground after all app code has finished.
        virtual _Check_return_ HRESULT OnAfterAppLeavingBackground()
        {
            RRETURN(S_OK);
        }

        // Called during EnteredBackground before any app code has run.
        virtual _Check_return_ HRESULT OnBeforeAppEnteredBackground()
        {
            RRETURN(S_OK);
        }

        // Called during EnteredBackground after all app code has finished.
        virtual _Check_return_ HRESULT OnAfterAppEnteredBackground()
        {
            RRETURN(S_OK);
        }

        // Used to ensure that all of the above methods are invoked on the same thread.
        virtual _Check_return_ HRESULT GetXamlDispatcher(_Out_ ctl::ComPtr<DirectUI::IDispatcher>* pspDispatcher) = 0;
    };

    // PLMHandler - implements PLM coordination for a single apartment.
    class PLMHandler
    {
    public:
        static _Check_return_ HRESULT CreateForASTA(_In_opt_ IPLMHandlerCallbacks* pCallbacks, _Outptr_ PLMHandler** ppHandler);
        static _Check_return_ HRESULT CreateForMTA(_In_opt_ IPLMHandlerCallbacks* pCallbacks, _Outptr_ PLMHandler** ppHandler);
        ~PLMHandler();
        
        _Check_return_ HRESULT AddAppSuspendHandler(_In_ xaml::ISuspendingEventHandler* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveAppSuspendHandler(EventRegistrationToken token);
        _Check_return_ HRESULT AddAppResumeHandler(_In_ wf::IEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveAppResumeHandler(EventRegistrationToken token);

        _Check_return_ HRESULT AddAppLeavingBackgroundHandler(_In_ xaml::ILeavingBackgroundEventHandler* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveAppLeavingBackgroundHandler(EventRegistrationToken token);
        _Check_return_ HRESULT AddAppEnteredBackgroundHandler(_In_ xaml::IEnteredBackgroundEventHandler* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT RemoveAppEnteredBackgroundHandler(EventRegistrationToken token);

        _Check_return_ HRESULT IncrementAppSuspendActivityCount();
        _Check_return_ HRESULT DecrementAppSuspendActivityCount();

        static void ExtensionTelemetryProc(_In_opt_ const HMODULE hModule, _In_ const wchar_t * modulename);
        
    private:
        static _Check_return_ HRESULT Create(bool fForASTA, _In_opt_ IPLMHandlerCallbacks* pCallbacks, _Outptr_ PLMHandler** ppHandler);

        PLMHandler(_In_opt_ IPLMHandlerCallbacks* pCallbacks);

        _Check_return_ HRESULT RegisterWithPLM();
        _Check_return_ HRESULT UnregisterWithPLM();

        _Check_return_ HRESULT OnSuspending(_In_ IInspectable* pSender, _In_ wa::ISuspendingEventArgs* pArgs);
        _Check_return_ HRESULT OnResuming(_In_ IInspectable* pSender, _In_ IInspectable* pArg);

        _Check_return_ HRESULT OnLeavingBackground(_In_ IInspectable* pSender, _In_ wa::ILeavingBackgroundEventArgs* pArgs);
        _Check_return_ HRESULT OnEnteredBackground(_In_ IInspectable* pSender, _In_ wa::IEnteredBackgroundEventArgs* pArgs);

        _Check_return_ HRESULT InvokeAfterAppSuspendCallback();
        void CompleteXamlSuspendActivity();

        IPLMHandlerCallbacks* m_pCallbacks;
        EventRegistrationToken m_suspendingToken;
        EventRegistrationToken m_resumingToken;
        EventRegistrationToken m_leavingBackgroundToken;
        EventRegistrationToken m_enteredBackgroundToken;
        XUINT32 m_cAppSuspendActivityCount;
        std::unique_ptr<IEventSource<xaml::ISuspendingEventHandler, IInspectable, wa::ISuspendingEventArgs>> m_pAppSuspendHandlers;
        std::unique_ptr<IEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>> m_pAppResumeHandlers;
        std::unique_ptr<IEventSource<xaml::ILeavingBackgroundEventHandler, xaml::IApplication, wa::ILeavingBackgroundEventArgs>> m_pAppLeavingBackgroundHandlers;
        std::unique_ptr<IEventSource<xaml::IEnteredBackgroundEventHandler, xaml::IApplication, wa::IEnteredBackgroundEventArgs>> m_pAppEnteredBackgroundHandlers;

        wa::ISuspendingDeferral* m_pDeferral;
    };

    // Implements a generic wrapper for an IInspectable-derived type.
    // The wrapper is free-threaded so is suitable for wrapping free-threaded objects.
    template <class TWrapped> class Wrapper : public ctl::implements_inspectable<TWrapped>, public IAgileObject
    {
    public:

        // IUnknown

        HRESULT STDMETHODCALLTYPE QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
        {
            HRESULT hr = S_OK;
            
            if (IID_IMarshal == riid)
            {
                if (!m_pUnkFTM)
                {
                    IUnknown* pThisUnk = static_cast<ctl::implements_inspectable<TWrapped>*>(this);
                    hr = CoCreateFreeThreadedMarshaler(pThisUnk, &m_pUnkFTM);
                    if (FAILED(hr))
                    {
                        return hr;
                    }
                }
                
                return m_pUnkFTM->QueryInterface(riid, ppvObject);
            }

            return ctl::implements_inspectable<TWrapped>::QueryInterface(riid, ppvObject);
        }
        
        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return ctl::implements_inspectable<TWrapped>::AddRef();
        }
        
        ULONG STDMETHODCALLTYPE Release() override
        {
            return ctl::implements_inspectable<TWrapped>::Release();
        }
        
        // IInspectable
        
        HRESULT STDMETHODCALLTYPE GetIids( 
            /* [out] */ __RPC__out ULONG *iidCount,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids) override
        {
            return m_pWrapped->GetIids(iidCount, iids);
        }
        
        HRESULT STDMETHODCALLTYPE GetRuntimeClassName( 
            /* [out] */ __RPC__deref_out_opt HSTRING *className) override
        {
            return m_pWrapped->GetRuntimeClassName(className);
        }
        
        HRESULT STDMETHODCALLTYPE GetTrustLevel( 
            /* [out] */ __RPC__out TrustLevel *trustLevel) override
        {
            return m_pWrapped->GetTrustLevel(trustLevel);
        }

    protected:
        Wrapper(_In_ TWrapped* pWrapped) :
            m_pWrapped(pWrapped),
            m_pUnkFTM(NULL)
        {
            AddRefInterface(m_pWrapped);
        }
            
        ~Wrapper() override
        {
            ReleaseInterface(m_pWrapped);
            ReleaseInterface(m_pUnkFTM);
        }

        TWrapped* GetWrapped()
        {
            return m_pWrapped;
        }

    private:
        TWrapped* m_pWrapped;
        IUnknown* m_pUnkFTM;
    };

    // Wraps ISuspendingDeferral so we can intercept deferral completion.
    class SuspendingDeferralWrapper : public Wrapper<wa::ISuspendingDeferral>
    {
    public:
        static _Check_return_ HRESULT Wrap(
            _In_ wa::ISuspendingDeferral* pWrapped,
            _In_ PLMHandler* pHandler,
            _Outptr_ wa::ISuspendingDeferral** ppWrapper)
        {
            HRESULT hr = S_OK;

            *ppWrapper = new SuspendingDeferralWrapper(pWrapped, pHandler);

            RRETURN(hr);//RRETURN_REMOVAL
        }

        // ISuspendingDeferral
        HRESULT STDMETHODCALLTYPE Complete() override;

    private:
        PLMHandler* m_pHandler;

        SuspendingDeferralWrapper(_In_ wa::ISuspendingDeferral* pWrapped, _In_ PLMHandler* pHandler) :
            Wrapper<wa::ISuspendingDeferral>(pWrapped),
            m_pHandler(pHandler)
        {
        }
    };

    // Wraps ISuspendingOperation so we can intercept deferral requests.
    class SuspendingOperationWrapper : public Wrapper<wa::ISuspendingOperation>
    {
    public:
        static _Check_return_ HRESULT Wrap(
            _In_ wa::ISuspendingOperation* pWrapped,
            _In_ PLMHandler* pHandler,
            _Outptr_ wa::ISuspendingOperation** ppWrapper)
        {
            HRESULT hr = S_OK;

            *ppWrapper = new SuspendingOperationWrapper(pWrapped, pHandler);

            RRETURN(hr);//RRETURN_REMOVAL
        }

        // ISuspendingOperation
        HRESULT STDMETHODCALLTYPE GetDeferral( 
            /* [out][retval] */ __RPC__deref_out_opt wa::ISuspendingDeferral **deferral) override;
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE get_Deadline( 
            /* [out][retval] */ __RPC__out wf::DateTime *value) override
        {
            return GetWrapped()->get_Deadline(value);
        }

    private:
        PLMHandler* m_pHandler;

        SuspendingOperationWrapper(_In_ wa::ISuspendingOperation* pWrapped, _In_ PLMHandler* pHandler) :
            Wrapper<wa::ISuspendingOperation>(pWrapped),
            m_pHandler(pHandler)
        {
        }
    };

    // Wraps ISuspendingEventArgs so we can intercept deferral requests.
    class SuspendingEventArgsWrapper : public Wrapper<wa::ISuspendingEventArgs>
    {
    public:
        static _Check_return_ HRESULT Wrap(
            _In_ wa::ISuspendingEventArgs* pWrapped,
            _In_ PLMHandler* pHandler,
            _Outptr_ wa::ISuspendingEventArgs** ppWrapper)
        {
            HRESULT hr = S_OK;

            *ppWrapper = new SuspendingEventArgsWrapper(pWrapped, pHandler);

            RRETURN(hr);//RRETURN_REMOVAL
        }

        // ISuspendingEventArgs
        /* [propget] */ HRESULT STDMETHODCALLTYPE get_SuspendingOperation( 
            /* [out][retval] */ __RPC__deref_out_opt wa::ISuspendingOperation **value) override;
        
    private:
        PLMHandler* m_pHandler;

        SuspendingEventArgsWrapper(_In_ wa::ISuspendingEventArgs* pWrapped, _In_ PLMHandler* pHandler) :
            Wrapper<wa::ISuspendingEventArgs>(pWrapped),
            m_pHandler(pHandler)
        {
        }
    };
}

}
