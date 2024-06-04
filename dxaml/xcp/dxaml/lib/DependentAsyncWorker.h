// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DependentAsyncWorker is a helper class for building an async action
//      or operation (the parent worker) that depends on the execution
//      and completion of another action or operation (the child worker).
//      The child worker may execute on any thread but the parent worker
//      is bound to the thread that started it (usually the UI thread).

#pragma once

#include "DXamlAsyncBase.h"

namespace DirectUI
{
    template<typename TAsyncOptions = wrl::AsyncOptions<>>
    struct AsyncActionWorkerTraits
    {
        typedef wf::IAsyncAction TAsyncInterface;
        typedef wf::IAsyncActionCompletedHandler TComplete;
        typedef void TCompleteResult;
        typedef TAsyncOptions TAsyncBaseOptions;
    };

    template<typename TResult, typename TAsyncOptions>
    struct AsyncOperationWorkerTraitsBase
    {
        typedef wf::IAsyncOperation<TResult> TAsyncInterface;
        typedef wf::IAsyncOperationCompletedHandler<TResult> TComplete;
        typedef TResult TCompleteResult;
        typedef typename std::remove_pointer<TResult>::type TResultNoPointer;
        typedef TAsyncOptions TAsyncBaseOptions;
    };

    template<typename TResult, typename TAsyncOptions = wrl::AsyncOptions<>, typename Enable = void>
    struct AsyncOperationWorkerTraits
        : public AsyncOperationWorkerTraitsBase<TResult, TAsyncOptions>
    {
        typedef TResult TCompleteResultStorage;

        static _Check_return_ TResult get_from_storage(_In_ TCompleteResultStorage storage) { return storage; }
        static _Check_return_ TResult copy_from_storage(_In_ TCompleteResultStorage storage) { return storage; }
    };

    template<typename TResult, typename TAsyncOptions>
    struct AsyncOperationWorkerTraits
        <TResult,
         TAsyncOptions,
         typename std::enable_if<std::is_base_of<IUnknown, typename std::remove_pointer<TResult>::type>::value>::type>
         : public AsyncOperationWorkerTraitsBase<TResult, TAsyncOptions>
    {
        typedef wrl::ComPtr<typename AsyncOperationWorkerTraitsBase<TResult, TAsyncOptions>::TResultNoPointer> TCompleteResultStorage;

        static _Check_return_ TResult get_from_storage(_In_ TCompleteResultStorage& storage) { return storage.Get(); }
        static _Check_return_ TResult copy_from_storage(_In_ TCompleteResultStorage& storage) { storage.Get()->AddRef(); return storage.Get(); }
    };

    #pragma region DependentAsyncWorkerTraits
    // DependentAsyncWorker supports only actions and operations that doesn't report progress.
    // Support for those cases can be added later, altough it really makes sense only for the child async worker.

    // Both parent and child workers are operations.
    template<typename TParentCompleteResult, typename TChildCompleteResult>
    struct DependentAsyncWorkerTraits
    {
        typedef std::function<HRESULT(wf::AsyncStatus, TChildCompleteResult, /* (__deref)_out */ TParentCompleteResult*)> DependentAsyncWorkerCompletedCallback;
    };

    // Both parent and child workers are actions.
    template<>
    struct DependentAsyncWorkerTraits<void, void>
    {
        typedef std::function<HRESULT(wf::AsyncStatus)> DependentAsyncWorkerCompletedCallback;
    };

    // Parent worker is an action and child worker is an operation.
    template<typename TChildCompleteResult>
    struct DependentAsyncWorkerTraits<void, TChildCompleteResult>
    {
        typedef std::function<HRESULT(wf::AsyncStatus, TChildCompleteResult)> DependentAsyncWorkerCompletedCallback;
    };

    // Parent worker is an operation and child worker is an action.
    template<typename TParentCompleteResult>
    struct DependentAsyncWorkerTraits<TParentCompleteResult, void>
    {
        typedef std::function<HRESULT(wf::AsyncStatus, /* (__deref)_out */ TParentCompleteResult*)> DependentAsyncWorkerCompletedCallback;
    };

    #pragma endregion

    template<typename TPARENT_ASYNC_WORKER_TRAITS, typename TCHILD_ASYNC_WORKER_TRAITS>
    class ChildAsyncWorkerCompleteHandler;

    template<typename TPARENT_ASYNC_WORKER_TRAITS, typename TCHILD_ASYNC_WORKER_TRAITS>
    class DependentAsyncWorkerBase
        : public DXamlAsyncBaseImpl<
            typename TPARENT_ASYNC_WORKER_TRAITS::TComplete,
            typename TPARENT_ASYNC_WORKER_TRAITS::TAsyncInterface,
            typename TPARENT_ASYNC_WORKER_TRAITS::TAsyncBaseOptions>
    {
    protected:

        typedef typename TPARENT_ASYNC_WORKER_TRAITS::TComplete TParentComplete;
        typedef typename TPARENT_ASYNC_WORKER_TRAITS::TCompleteResult TParentCompleteResult;
        typedef typename TPARENT_ASYNC_WORKER_TRAITS::TAsyncInterface TParentAsyncInterface;

        typedef typename TCHILD_ASYNC_WORKER_TRAITS::TCompleteResult TChildCompleteResult;
        typedef typename TCHILD_ASYNC_WORKER_TRAITS::TAsyncInterface TChildAsyncInterface;
        typedef ChildAsyncWorkerCompleteHandler<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS> TChildAsyncWorkerCompleteHandler;

        typedef typename DependentAsyncWorkerTraits<typename TPARENT_ASYNC_WORKER_TRAITS::TCompleteResult, typename TCHILD_ASYNC_WORKER_TRAITS::TCompleteResult>::DependentAsyncWorkerCompletedCallback TDependentAsyncWorkerCompletedCallback;

        friend class ChildAsyncWorkerCompleteHandler<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>;

    public:

        STDMETHOD(RuntimeClassInitialize)()
        {
            return this->put_Id(s_ulUniqueAsyncActionId++);
        }

        _Check_return_ HRESULT InitializeAsyncWorkers(
            _In_ std::function<HRESULT(TChildAsyncInterface**)> runChildAsyncWorkerFunc,
            _In_ TDependentAsyncWorkerCompletedCallback parentAsyncWorkerCompletedCallback)
        {
            ASSERT(runChildAsyncWorkerFunc);
            ASSERT(parentAsyncWorkerCompletedCallback);

            // DependentAsyncWorker is not designed to be
            // thread-safe. Workers should be started and
            // interacted with from the same thread
            // (usually the UI thread).

            m_runChildAsyncWorkerFunc = std::move(runChildAsyncWorkerFunc);
            m_parentAsyncWorkerCompletedCallback = std::move(parentAsyncWorkerCompletedCallback);

            return S_OK;
        }

    protected:

        virtual _Check_return_ HRESULT CompleteParentAsyncWorkerImpl(
            _In_ wf::AsyncStatus status)
        {
            HRESULT hr = S_OK;

            switch(status)
            {
                case wf::AsyncStatus::Canceled:
                    IFC(this->Cancel());
                    break;

                case wf::AsyncStatus::Error:
                    this->TryTransitionToError(E_FAIL);
                    break;
            }

            IFC(this->FireCompletion());

        Cleanup:
            m_spChildAsyncInfo = nullptr;
            m_runChildAsyncWorkerFunc = nullptr;
            m_spThis = nullptr;
            RRETURN(hr);
        }

        virtual _Check_return_ HRESULT CompleteParentAsyncWorkerSynchronously() = 0;

    private:

        IFACEMETHOD(put_Completed)(_In_ TParentComplete *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_ TParentComplete **ppCompletedHandler) override
        {
            return __super::GetOnComplete(ppCompletedHandler);
        }

        _Check_return_ HRESULT OnStart() override
        {
            HRESULT hr = S_OK;
            wrl::ComPtr<TChildAsyncInterface> spChildAsyncInterface;
            wrl::ComPtr<TChildAsyncWorkerCompleteHandler> spChildAsyncWorkerCompleteHandler;

            IFC(m_runChildAsyncWorkerFunc(&spChildAsyncInterface));

            if (spChildAsyncInterface)
            {
                IFC(wrl::MakeAndInitialize<TChildAsyncWorkerCompleteHandler>(
                    &spChildAsyncWorkerCompleteHandler,
                    this));

                IFC(spChildAsyncInterface->put_Completed(spChildAsyncWorkerCompleteHandler.Get()));

                IFC(spChildAsyncInterface.As(&m_spChildAsyncInfo));

                m_spThis = this;
            }
            else
            {
                // Complete the parent async worker right away.
                IFC(CompleteParentAsyncWorkerSynchronously());
            }

        Cleanup:
            RRETURN(hr);
        }

        void OnClose() override
        {
            if(m_spChildAsyncInfo)
            {
                IGNOREHR(m_spChildAsyncInfo->Close());
            }
        }

        void OnCancel() override
        {
            if(m_spChildAsyncInfo)
            {
                IGNOREHR(m_spChildAsyncInfo->Cancel());
            }
        }

    private:
        // Unique id for DependentAsyncWorker instances.
        static UINT32 s_ulUniqueAsyncActionId;

        // Used to hold a ref to self while invoking completion handler
        wrl::ComPtr<DependentAsyncWorkerBase> m_spThis;

        // Function that will run the child async worker.
        std::function<HRESULT(TChildAsyncInterface**)> m_runChildAsyncWorkerFunc;

        // Stores the result of the m_runChildAsyncWorkerFunc call.
        wrl::ComPtr<wf::IAsyncInfo> m_spChildAsyncInfo;

    protected:
        // Callback function when the parent async worker completes.
        TDependentAsyncWorkerCompletedCallback m_parentAsyncWorkerCompletedCallback;
    };

    template <typename U, typename V>
    UINT32 DependentAsyncWorkerBase<U, V>::s_ulUniqueAsyncActionId = 1;

    template<typename TPARENT_ASYNC_WORKER_TRAITS, typename TCHILD_ASYNC_WORKER_TRAITS, typename Enable = void>
    class DependentAsyncWorker
        : public DependentAsyncWorkerBase<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>
    {
    };

    template<typename TPARENT_ASYNC_WORKER_TRAITS, typename TCHILD_ASYNC_WORKER_TRAITS>
    class DependentAsyncWorker
        <TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS,
         typename std::enable_if<std::is_same<TPARENT_ASYNC_WORKER_TRAITS, AsyncActionWorkerTraits<typename TPARENT_ASYNC_WORKER_TRAITS::TAsyncBaseOptions>>::value>::type>
        : public DependentAsyncWorkerBase<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>
    {
        using base_type = DependentAsyncWorkerBase<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>;

    public:
        IFACEMETHOD(GetResults)() override
        {
            // Consider: return an error if the operation hasn't completed yet.
            RRETURN(S_OK);
        };

    private:
        // Called from the completed handler of the child async operation.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<!std::is_same<TChildCompleteResult, void>::value, HRESULT>::type
        CompleteParentAsyncWorker(
            _In_ wf::AsyncStatus status,
            _In_ TChildCompleteResult results)
        {
            HRESULT hr = S_OK;

            if (status == wf::AsyncStatus::Completed)
            {
                IFC(this->m_parentAsyncWorkerCompletedCallback(status, results));
            }
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        // Called from the completed handler of the child async action.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<std::is_same<TChildCompleteResult, void>::value, HRESULT>::type
        CompleteParentAsyncWorker(
            _In_ wf::AsyncStatus status)
        {
            HRESULT hr = S_OK;

            if (status == wf::AsyncStatus::Completed)
            {
                IFC(this->m_parentAsyncWorkerCompletedCallback(status));
            }
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        // Completes the parent async worker synchronously without invoking the child async operation.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<!std::is_same<TChildCompleteResult, void>::value &&
                                               std::is_pointer<TChildCompleteResult>::value, HRESULT>::type
        CompleteParentAsyncWorkerSynchronouslyImpl()
        {
            HRESULT hr = S_OK;
            wf::AsyncStatus status = wf::AsyncStatus::Completed;
            TChildCompleteResult results = nullptr;

            IFC(this->m_parentAsyncWorkerCompletedCallback(status, results));
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<!std::is_same<TChildCompleteResult, void>::value &&
                                              !std::is_pointer<TChildCompleteResult>::value, HRESULT>::type
        CompleteParentAsyncWorkerSynchronouslyImpl()
        {
            HRESULT hr = S_OK;
            wf::AsyncStatus status = wf::AsyncStatus::Completed;
            TChildCompleteResult results = TChildCompleteResult();

            IFC(this->m_parentAsyncWorkerCompletedCallback(status, results));
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        // Completes the parent async worker synchronously without invoking the child async action.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<std::is_same<TChildCompleteResult, void>::value, HRESULT>::type
        CompleteParentAsyncWorkerSynchronouslyImpl()
        {
            HRESULT hr = S_OK;
            wf::AsyncStatus status = wf::AsyncStatus::Completed;

            IFC(this->m_parentAsyncWorkerCompletedCallback(status));
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT CompleteParentAsyncWorkerSynchronously() override
        {
            RRETURN(CompleteParentAsyncWorkerSynchronouslyImpl<typename base_type::TChildCompleteResult>());
        }

        friend class ChildAsyncWorkerCompleteHandler<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>;
    };

    template<typename TPARENT_ASYNC_WORKER_TRAITS, typename TCHILD_ASYNC_WORKER_TRAITS>
    class DependentAsyncWorker
        <TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS,
         typename std::enable_if<std::is_same<TPARENT_ASYNC_WORKER_TRAITS, AsyncActionWorkerTraits<typename TPARENT_ASYNC_WORKER_TRAITS::TAsyncBaseOptions>>::value == false>::type>
        : public DependentAsyncWorkerBase<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>
    {
        typedef typename TPARENT_ASYNC_WORKER_TRAITS::TCompleteResultStorage TParentCompleteResultStorage;

        InspectableClass(TParentAsyncInterface::z_get_rc_name_impl(), BaseTrust);

    public:
        IFACEMETHOD(GetResults)(
            typename DependentAsyncWorkerBase<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>::TParentCompleteResult* results) override
        {
            // Consider: return an error if the operation hasn't completed yet.
            *results = TPARENT_ASYNC_WORKER_TRAITS::copy_from_storage(m_parentResults);
            RRETURN(S_OK);
        };

    private:
        // Called from the completed handler of the child async operation.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<!std::is_same<TChildCompleteResult, void>::value, HRESULT>::type
        CompleteParentAsyncWorker(
            _In_ wf::AsyncStatus status,
            _In_ TChildCompleteResult childResults)
        {
            HRESULT hr = S_OK;

            if (status == wf::AsyncStatus::Completed)
            {
                IFC(this->m_parentAsyncWorkerCompletedCallback(status, childResults, &m_parentResults));
            }
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        // Called from the completed handler of the child async action.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<std::is_same<TChildCompleteResult, void>::value, HRESULT>::type
        CompleteParentAsyncWorker(
            _In_ wf::AsyncStatus status)
        {
            HRESULT hr = S_OK;

            if (status == wf::AsyncStatus::Completed)
            {
                IFC(this->m_parentAsyncWorkerCompletedCallback(status, &m_parentResults));
            }
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        // Completes the parent async worker synchronously without invoking the child async operation.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<!std::is_same<TChildCompleteResult, void>::value &&
                                               std::is_pointer<TChildCompleteResult>::value, HRESULT>::type
        CompleteParentAsyncWorkerSynchronouslyImpl()
        {
            HRESULT hr = S_OK;
            wf::AsyncStatus status = wf::AsyncStatus::Completed;
            TChildCompleteResult childResults = nullptr;

            IFC(this->m_parentAsyncWorkerCompletedCallback(status, childResults, &m_parentResults));
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<!std::is_same<TChildCompleteResult, void>::value &&
                                              !std::is_pointer<TChildCompleteResult>::value, HRESULT>::type
        CompleteParentAsyncWorkerSynchronouslyImpl()
        {
            HRESULT hr = S_OK;
            wf::AsyncStatus status = wf::AsyncStatus::Completed;
            TChildCompleteResult childResults = TChildCompleteResult();

            IFC(this->m_parentAsyncWorkerCompletedCallback(status, childResults, &m_parentResults));
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        // Completes the parent async worker synchronously without invoking the child async action.
        template<typename TChildCompleteResult>
        _Check_return_ typename std::enable_if<std::is_same<TChildCompleteResult, void>::value, HRESULT>::type
        CompleteParentAsyncWorkerSynchronouslyImpl()
        {
            HRESULT hr = S_OK;
            wf::AsyncStatus status = wf::AsyncStatus::Completed;

            IFC(this->m_parentAsyncWorkerCompletedCallback(status, &m_parentResults));
            IFC(this->CompleteParentAsyncWorkerImpl(status));

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT CompleteParentAsyncWorkerSynchronously() override
        {
            RRETURN(CompleteParentAsyncWorkerSynchronouslyImpl<TChildCompleteResult>());
        }

    private:
        TParentCompleteResultStorage m_parentResults;
        friend class ChildAsyncWorkerCompleteHandler<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS>;
    };

    template<typename TPARENT_ASYNC_WORKER_TRAITS, typename TCHILD_ASYNC_WORKER_TRAITS>
    class ChildAsyncWorkerCompleteHandler
        : public wrl::RuntimeClass<
            wrl::RuntimeClassFlags<wrl::ClassicCom>,
            typename TCHILD_ASYNC_WORKER_TRAITS::TComplete>
    {

        typedef typename TCHILD_ASYNC_WORKER_TRAITS::TAsyncInterface TChildAsyncInterface;
        typedef typename TCHILD_ASYNC_WORKER_TRAITS::TCompleteResult TChildCompleteResult;

        typedef DependentAsyncWorkerBase<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS> TParentAsyncWorkerBase;
        typedef DependentAsyncWorker<TPARENT_ASYNC_WORKER_TRAITS, TCHILD_ASYNC_WORKER_TRAITS> TParentAsyncWorker;

    public:

        STDMETHOD(RuntimeClassInitialize)(TParentAsyncWorkerBase* pParentAsyncWorker)
        {
            ASSERT(pParentAsyncWorker);

            m_spParentAsyncWorker = static_cast<TParentAsyncWorker*>(pParentAsyncWorker);
            RRETURN(S_OK);
        }

        // Called when the child async worker completes.
        // We call back into the parent async worker to let it know.
        IFACEMETHOD(Invoke)(
            _In_ TChildAsyncInterface* asyncInfo,
            _In_ wf::AsyncStatus status) override
        {
            HRESULT hr = S_OK;

            ASSERT(m_spParentAsyncWorker);

            // Notify the parent async worker that the child async worker is done.
            IFC(CompleteParentAsyncWorker<TChildAsyncInterface>(status, asyncInfo));

            // We won't be using the child async worker
            // from now on, so close it.
            IFC(CloseChildAsyncWorker(asyncInfo));

        Cleanup:
            RRETURN(hr);
        }

    private:

        template<typename TChildAsyncInterface>
        _Check_return_ typename std::enable_if<!std::is_same<TChildAsyncInterface, wf::IAsyncAction>::value, HRESULT>::type
        CompleteParentAsyncWorker(
            _In_ wf::AsyncStatus status,
            _In_ TChildAsyncInterface* asyncInfo)
        {
            typedef typename TCHILD_ASYNC_WORKER_TRAITS::TCompleteResultStorage TChildCompleteResultStorage;

            HRESULT hr = S_OK;
            TChildCompleteResultStorage results;

            if (status == wf::AsyncStatus::Completed)
            {
                IFC(asyncInfo->GetResults(&results));
            }
            IFC(m_spParentAsyncWorker->template CompleteParentAsyncWorker<TChildCompleteResult>(
                status,
                TCHILD_ASYNC_WORKER_TRAITS::get_from_storage(results)));

        Cleanup:
            RRETURN(hr);
        }

        template<typename TChildAsyncInterface>
        _Check_return_ typename std::enable_if<std::is_same<TChildAsyncInterface, wf::IAsyncAction>::value, HRESULT>::type
        CompleteParentAsyncWorker(
            _In_ wf::AsyncStatus status,
            _In_ TChildAsyncInterface* asyncInfo)
        {
            HRESULT hr = S_OK;
            UNREFERENCED_PARAMETER(asyncInfo);

            IFC(m_spParentAsyncWorker->CompleteParentAsyncWorker<TChildCompleteResult>(status));

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT
        CloseChildAsyncWorker(
            _In_ TChildAsyncInterface* asyncInfo)
        {
            HRESULT hr = S_OK;
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;

            IFC(asyncInfo->QueryInterface(
                __uuidof(wf::IAsyncInfo),
                &spAsyncInfo));
            IFCPTR(spAsyncInfo);

            IFC(spAsyncInfo->Close());
            m_spParentAsyncWorker = nullptr;

        Cleanup:
            RRETURN(hr);
        }

    private:
        wrl::ComPtr<TParentAsyncWorker> m_spParentAsyncWorker;
    };
}

