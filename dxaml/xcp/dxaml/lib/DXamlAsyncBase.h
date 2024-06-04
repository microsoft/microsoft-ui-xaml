// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Dispatcher.h"
#include "Callback.h"
#include "wrl\async.h"

namespace DirectUI
{
    // -------------------------------------------------------------------------
    //
    // Internal class that implements IAsyncAction
    //
    // NOTES:
    // WRL::AsyncBase is thread-safe, but because of our thread affinity model or for simplicity, this class
    // has not been made thread-safe.  It could be refactored if we wanted to make some of our Async methods callable
    // from non-UI threads
    // -------------------------------------------------------------------------
    template<typename TComplete, typename TAsyncInterface, typename TAsyncBaseOptions = Microsoft::WRL::AsyncOptions<>, typename I1 = Microsoft::WRL::Details::Nil, unsigned int classFlags = Microsoft::WRL::RuntimeClassType::WinRt>
    class __declspec(novtable) DXamlAsyncBaseImpl: public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<classFlags>,
            TAsyncInterface,
            Microsoft::WRL::AsyncBase<TComplete, Microsoft::WRL::Details::Nil, Microsoft::WRL::SingleResult, TAsyncBaseOptions>,
            I1>
    {
        public:
            using RuntimeClassT = typename Microsoft::WRL::RuntimeClass<
                Microsoft::WRL::RuntimeClassFlags<classFlags>,
                TAsyncInterface,
                Microsoft::WRL::AsyncBase<TComplete, Microsoft::WRL::Details::Nil, Microsoft::WRL::SingleResult, TAsyncBaseOptions>,
                I1>::RuntimeClassT;

        InspectableClass(L"Windows.Foundation.IAsyncAction", BaseTrust);

        public:
            // Used by WRL to create an instance of the class
            STDMETHOD(RuntimeClassInitialize)(
                  UINT32 uId,
                  _In_opt_ DirectUI::IDispatcher* pDispatcher)
            {
                HRESULT hr = __super::put_Id(uId);
                m_spDispatcher = pDispatcher;
                return hr;
            }
            // AsyncBase hides the Start method from the public interface, so
            // we need this to be able to Hot Start ourselves
            HRESULT StartOperation(void)
            {
                return this->Start();
            }

        protected:
            // This class is marked novtable, so must not be instantiated directly.
            DXamlAsyncBaseImpl() = default;

            // AsyncBase overrides
            HRESULT OnStart(void) override
            {
                return S_OK;
            }
            void OnClose(void) override
            {
                return;
            }
            void OnCancel(void) override
            {
                return;
            }

            // ICoreAsyncAction Implementation
            bool CoreContinueAsyncActionImpl()
            {
                return __super::ContinueAsyncOperation();
            }
            void CoreFireCompletionImpl()
            {
                // If we have a dispatcher use it
                if (m_spDispatcher != nullptr)
                {
                    PostCompletion();
                }
                else
                {
                    __super::FireCompletion();
                }
            }
            void CoreReleaseRefImpl(void)
            {
                Release();
            }
            void CoreSetErrorImpl(HRESULT hr)
            {
                if (FAILED(hr))
                {
                    __super::TryTransitionToError(hr);
                }
            }

        private:
            // Called on the UI thread via the Dispatcher
            _Check_return_ HRESULT CompletionDispatcherCallback(void)
            {
                __super::FireCompletion();

                return S_OK;
            }

            // Call this to post completion to the dispatcher
            void PostCompletion(void)
            {
                ctl::ComPtr<ICallback> spCallback =
                    MakeCallback(
                        Microsoft::WRL::ComPtr<DXamlAsyncBaseImpl<TComplete, TAsyncInterface, TAsyncBaseOptions>>(this),
                        &DXamlAsyncBaseImpl::CompletionDispatcherCallback);

                m_spDispatcher->RunAsync(spCallback);
            }

            ctl::ComPtr<DirectUI::IDispatcher> m_spDispatcher;

        protected:
            ~DXamlAsyncBaseImpl() override {}
    };

    extern __declspec(selectany) const WCHAR BitmapSourceSetSourceAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Media.Imaging.BitmapSource.SetSourceAsync";
    extern __declspec(selectany) const WCHAR SoftwareBitmapSourceSetSourceAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Media.Imaging.SoftwareBitmapSource.SetSourceAsync";
    extern __declspec(selectany) const WCHAR SvgImageSourceSetSourceAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Media.Imaging.SvgImageSource.SetSourceAsync";

    template < typename TAsyncBaseOptions = Microsoft::WRL::AsyncOptions<> >
    class DXamlAsyncActionBase:
        public DXamlAsyncBaseImpl<wf::IAsyncActionCompletedHandler, wf::IAsyncAction, TAsyncBaseOptions>,
        public ICoreAsyncAction
    {
        using DXamlAsyncBaseImpl = DXamlAsyncBaseImpl<wf::IAsyncActionCompletedHandler, wf::IAsyncAction, TAsyncBaseOptions>;

    public:

        // wf::IAsyncAction overrides
        // This is the API the caller talks to (user code)
        IFACEMETHOD(put_Completed)(_In_ wf::IAsyncActionCompletedHandler *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  wf::IAsyncActionCompletedHandler **ppCompletedHandler) override
        {
            return __super::GetOnComplete(ppCompletedHandler);
        }
        IFACEMETHOD(GetResults)() override
        {
            return __super::CheckValidStateForResultsCall();
        }

        // ICoreAsyncAction overrides
        // This is the API the our code (Core and DXaml layer) talks to
        bool CoreContinueAsyncAction() override
        {
            return DXamlAsyncBaseImpl::CoreContinueAsyncActionImpl();
        }

        void CoreFireCompletion() override
        {
            DXamlAsyncBaseImpl::CoreFireCompletionImpl();
        }

        void CoreReleaseRef(void) override
        {
            DXamlAsyncBaseImpl::CoreReleaseRefImpl();
        }

        void CoreSetError(HRESULT hr) override
        {
            DXamlAsyncBaseImpl::CoreSetErrorImpl(hr);
        }
    };

    typedef DXamlAsyncActionBase<Microsoft::WRL::AsyncCausalityOptions<BitmapSourceSetSourceAsyncOperationName>> BitmapSourceSetSourceAsyncAction;
    typedef DXamlAsyncActionBase<Microsoft::WRL::AsyncCausalityOptions<SoftwareBitmapSourceSetSourceAsyncOperationName>> SoftwareBitmapSourceSetSourceAsyncAction;
}

