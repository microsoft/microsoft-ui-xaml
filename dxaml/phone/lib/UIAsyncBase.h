// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Internal classes that implements IAsyncAction.
//
// Uses assumpted baked into our threading model to simplify the
// Async callback model to only run callbacks on the UI thread.
//
// NOTES:
//   This class was created due to DXamlAsyncBase's reliance on
//   IDispatcher, an internal interface that is part of the core and
//   provides a lightweight, x-platform,  alternative to CoreDispatch.

#pragma once

#include "IPickerFlyoutAsyncOperation.h"
#include "ValueHelpers.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

    // Because this is only a template class for TAsyncInterface (which should be of type
    // IAsyncOperation<T>) it does not implement the interface methods needed. This must be
    // done by a class deriving from this one.
    template<typename TComplete, typename TAsyncInterface, typename TAsyncBaseOptions = wrl::AsyncOptions<>>
    class UIAsyncBase :
        public wrl::RuntimeClass<
            TAsyncInterface, wrl::AsyncBase<TComplete, wrl::Details::Nil, wrl::SingleResult, TAsyncBaseOptions>>
    {
        public:
            using RuntimeClassT = typename wrl::RuntimeClass<TAsyncInterface, wrl::AsyncBase<TComplete, wrl::Details::Nil, wrl::SingleResult, TAsyncBaseOptions>>::RuntimeClassT;
            InspectableClass(L"Windows.Foundation.IAsyncAction", BaseTrust);

        public:
            UIAsyncBase() {}

            // AsyncBase hides the Start method from the public interface, so
            // we need this to be able to Hot Start ourselves
            _Check_return_ HRESULT StartOperation(void)
            {
                return this->Start();
            }

            // Call this method to put the operation in the completed
            // state and fire off a callback.
            _Check_return_ HRESULT FireCompletionImpl()
            {
                HRESULT hr = S_OK;

                ASYNCTRACE(L"Running the completion operation synchronously.");
                IFC(__super::FireCompletion());

            Cleanup:
                return hr;
            }

        protected:

            // AsyncBase overrides
            virtual HRESULT OnStart(void) override
            {
                return S_OK;
            }
            virtual void OnClose(void) override
            {
                return;
            }
            virtual void OnCancel(void) override
            {
                return;
            }

        protected:
            virtual ~UIAsyncBase() {}
    };

#pragma region ABI type helper

    template<typename T>
    struct AbiTypeHelper
    {
        typedef typename T T_abi;
    };

    template<>
    struct AbiTypeHelper<bool>
    {
        typedef boolean T_abi;
    };

#pragma endregion ABI type helper

#pragma region Results storage

    template<typename TResult>
    class ResultsStorage
    {
    };

    template<typename TResult>
    class ResultsStorage<TResult *>
    {
    protected:
        void SetResult(_In_ TResult* pResult)
        {
            m_spResult = pResult;
        }

        HRESULT GetResult(_Outptr_ TResult** ppResult)
        {
            return m_spResult.CopyTo(ppResult);
        }

        void Reset()
        {
            m_spResult.Reset();
        }

    private:
        wrl::ComPtr<TResult> m_spResult;
    };

    template<>
    class ResultsStorage<bool>
    {
    protected:
        ResultsStorage() : m_result(false)
        { }

        void SetResult(_In_ bool result)
        {
            m_result = result;
        }

        HRESULT GetResult(_Out_ bool* pResult)
        {
           *pResult = m_result;
           return S_OK;
        }

        void Reset()
        {
            m_result = false;
        }

    private:
        bool m_result;
    };

#pragma endregion Results storage

#pragma region PickerFlyoutAsyncOperation declaration

    template<typename TResult, PCWSTR OpName>
    class PickerFlyoutAsyncOperation :
        public UIAsyncBase<
            wf::IAsyncOperationCompletedHandler<TResult>,
            wf::IAsyncOperation<TResult>,
            ::Microsoft::WRL::AsyncCausalityOptions<OpName>>,
        public xaml_controls::IPickerFlyoutAsyncOperation<TResult>,
        xaml_controls::AbiTypeHelper<TResult>,
        xaml_controls::ResultsStorage<TResult>
    {
        using base_type = UIAsyncBase<
            wf::IAsyncOperationCompletedHandler<TResult>,
            wf::IAsyncOperation<TResult>,
            ::Microsoft::WRL::AsyncCausalityOptions<OpName>>;

        using RuntimeClassT = typename base_type::RuntimeClassT;

        InspectableClass(wf::IAsyncOperation<TResult>::z_get_rc_name_impl(), BaseTrust);

    public:
        using T_abi = xaml_controls::AbiTypeHelper<TResult>::T_abi;

        PickerFlyoutAsyncOperation() { }
        ~PickerFlyoutAsyncOperation()
        {
            ASYNCTRACE(L"AsyncOperation destrctor called.");
        }

         // wf::IAsyncOperation<TRESULT> forwarders
        IFACEMETHOD(put_Completed)(_In_ wf::IAsyncOperationCompletedHandler<TResult>* pCompletedHandler) override;
        IFACEMETHOD(get_Completed)(_Outptr_ wf::IAsyncOperationCompletedHandler<TResult>** ppCompletedHandler) override;
        IFACEMETHOD(Cancel)() override;

#pragma warning(suppress: 6504) // the annotation here would differ based on T_abi. Not worth creating a template specialization
        IFACEMETHOD(GetResults) (T_abi* result) override;

        _Check_return_ HRESULT StartOperation(_In_ xaml_primitives::IFlyoutBase* pAssociatedFlyout) override;

        _Check_return_ HRESULT CompleteOperation(_In_ TResult result) override;

    protected:

        virtual void OnClose() override;

    private:
        wrl::ComPtr<xaml_primitives::IFlyoutBase> m_spPendingFlyout;
    };

#pragma endregion PickerFlyoutAsyncOperation declaration

#pragma region PickerFlyoutAsyncOperation definitions

    template<typename TResult, PCWSTR OpName>
    IFACEMETHODIMP PickerFlyoutAsyncOperation<TResult, OpName>::put_Completed(
        _In_ wf::IAsyncOperationCompletedHandler<TResult>* pCompletedHandler)
    {
        return __super::PutOnComplete(pCompletedHandler);
    }

    template<typename TResult, PCWSTR OpName>
    IFACEMETHODIMP PickerFlyoutAsyncOperation<TResult, OpName>::get_Completed(
        _Outptr_ wf::IAsyncOperationCompletedHandler<TResult>** ppCompletedHandler)
    {
        return __super::GetOnComplete(ppCompletedHandler);
    }

    template<typename TResult, PCWSTR OpName>
    IFACEMETHODIMP PickerFlyoutAsyncOperation<TResult, OpName>::Cancel()
    {
        HRESULT hr = S_OK;

        IFC(__super::Cancel());

        if (m_spPendingFlyout)
        {
            IFC(m_spPendingFlyout->Hide());
        }

    Cleanup:
         return hr;
    }

    template<typename TResult, PCWSTR OpName>
    IFACEMETHODIMP PickerFlyoutAsyncOperation<TResult, OpName>::GetResults(T_abi* result)
    {
        HRESULT hr = S_OK;
        TResult tmp;

        IFC(this->GetResult(&tmp));
        *result = static_cast<T_abi>(tmp);

    Cleanup:
        return S_OK;
    }

    template<typename TResult, PCWSTR OpName>
    _Check_return_ HRESULT PickerFlyoutAsyncOperation<TResult, OpName>::StartOperation(_In_ xaml_primitives::IFlyoutBase* pAssociatedFlyout)
    {
        HRESULT hr = S_OK;
        ASSERT(!m_spPendingFlyout, "StartOperation should never be called on an operation already associated with a flyout.");

        m_spPendingFlyout = pAssociatedFlyout;
        IFC(base_type::StartOperation());

    Cleanup:
        return hr;
    }

    template<typename TResult, PCWSTR OpName>
    _Check_return_ HRESULT PickerFlyoutAsyncOperation<TResult, OpName>::CompleteOperation(
        _In_ TResult result)
    {
        HRESULT hr = S_OK;
        ASSERT(m_spPendingFlyout, "Expected operation to have a ref to the associated open flyout.");

        m_spPendingFlyout.Reset();
        this->SetResult(result);
        IFC(this->FireCompletionImpl());

    Cleanup:
        return hr;
    }

    template<typename TResult, PCWSTR OpName>
    void PickerFlyoutAsyncOperation<TResult, OpName>::OnClose()
    {
        this->Reset();
    }

#pragma endregion PickerFlyoutAsyncOperation definitions

    class ListPickerGetSelectedIndexAsyncOperation :
        public UIAsyncBase<wf::IAsyncOperationCompletedHandler<INT32>, wf::IAsyncOperation<INT32>, wrl::DisableCausality>
    {
        InspectableClass(wf::IAsyncOperation<INT32>::z_get_rc_name_impl(), BaseTrust);

        public:
            ListPickerGetSelectedIndexAsyncOperation() :
                m_isCompleted(false),
                m_closedToken()
            {
            }

            ~ListPickerGetSelectedIndexAsyncOperation()
            {
                ASYNCTRACE(L"AsyncOperation destrctor called.");
            }

            // wf::IAsyncOperation<TRESULT> forwarders
            IFACEMETHOD(put_Completed)(_In_ wf::IAsyncOperationCompletedHandler<INT32> *pCompletedHandler) override
            {
                return __super::PutOnComplete(pCompletedHandler);
            }

            IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  wf::IAsyncOperationCompletedHandler<INT32> **ppCompletedHandler) override
            {
                return __super::GetOnComplete(ppCompletedHandler);
            }

            IFACEMETHOD(GetResults) (_Out_ INT32* results) override
            {
                *results = m_selectedIndex;
                return S_OK;
            }

            IFACEMETHOD(Cancel)() override
            {
                HRESULT hr = S_OK;

                IFC(__super::Cancel());
                IFC(m_spInnerAsyncInfo->Cancel());

            Cleanup:
                return hr;
            }

            _Check_return_ HRESULT StartOperation(
                _In_ xaml_controls::IListPickerFlyout* pAssociatedFlyout,
                _In_ wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>* pInnerGetSelectionOperation)
            {
                HRESULT hr = S_OK;
                wrl::ComPtr<wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>> spInnerGetSelectionOperation(pInnerGetSelectionOperation);
                wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;
                wrl::WeakRef wrThis;

                ASSERT(!m_spPendingFlyout, "StartOperation should never be called on an operation already associated with a flyout.");

                IFC(spInnerGetSelectionOperation.As(&m_spInnerAsyncInfo));
                m_spPendingFlyout = pAssociatedFlyout;

                // There is also no guarantee that this operation will outlive the flyout or the inner operation.
                // Hence we protect the following callbacks with a weak ref.
                IFC(wrl::AsWeak(this, &wrThis));

                IFC(spInnerGetSelectionOperation->put_Completed(
                    wrl::Callback<wf::IAsyncOperationCompletedHandler<wfc::IVectorView<IInspectable*>*>>(
                    [wrThis](wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>* pAsyncInfo, wf::AsyncStatus asyncStatus) mutable
                {
                    HRESULT hr = S_OK;
                    wrl::ComPtr<wf::IAsyncOperation<INT32>> spThis;

                    IGNOREHR(wrThis.As(&spThis));
                    if (spThis)
                    {
                        IFC(static_cast<ListPickerGetSelectedIndexAsyncOperation*>(spThis.Get())->
                            OnSelectionCompleted(pAsyncInfo, asyncStatus));
                    }

                Cleanup:
                    return hr;
                }).Get()));

                IFC(m_spPendingFlyout.As(&spFlyoutBase));
                IFC(spFlyoutBase->add_Closed(
                    wrl::Callback<wf::IEventHandler<IInspectable*>>(
                    [wrThis](IInspectable* pSender, IInspectable* pArgs) mutable
                    {
                        HRESULT hr = S_OK;
                        wrl::ComPtr<wf::IAsyncOperation<INT32>> spThis;

                        IGNOREHR(wrThis.As(&spThis));
                        if (spThis)
                        {
                            IFC(static_cast<ListPickerGetSelectedIndexAsyncOperation*>(spThis.Get())->OnFlyoutClosed(pSender, pArgs));
                        }

                    Cleanup:
                        return hr;
                    }).Get(),
                    &m_closedToken));

                IFC(UIAsyncBase::StartOperation());

            Cleanup:
                return hr;
            }

        private:

            _Check_return_ HRESULT OnFlyoutClosed(_In_ IInspectable* /*pSender*/, _In_ IInspectable* /*pArgs*/)
            {
                return CompleteOperation();
            }

            // This method handles the completion of the ListPickerFlyout ShowAtAsync operation.
            // It pulls the selected index out of the ListPickerFlyout and completes this async
            // operation.
            _Check_return_ HRESULT OnSelectionCompleted(
                _In_ wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>* pAsyncInfo,
                _In_ wf::AsyncStatus asyncStatus)
            {
                HRESULT hr = S_OK;
                bool doWaitForFlyoutClose = false;

                ASSERT(!m_isCompleted, "GetSelectedIndex wrapper operation completed before inner operation, or completed multiple times.");
#ifdef DBG
                // Validate results of the inner operation
                ASSERT(asyncStatus == wf::AsyncStatus::Completed || asyncStatus == wf::AsyncStatus::Canceled);

                if (asyncStatus == wf::AsyncStatus::Completed)
                {
                    wrl::ComPtr<wfc::IVectorView<IInspectable*>> spSelectedItems;
                    IFC(pAsyncInfo->GetResults(&spSelectedItems));
                    if (spSelectedItems)
                    {
                        UINT32 numSelected = 0;
                        wrl::ComPtr<IInspectable> spSelectedItemFromResults;
                        wrl::ComPtr<IInspectable> spSelectedItem;
                        BOOLEAN areEqual = FALSE;

                        spSelectedItems->get_Size(&numSelected);
                        ASSERT(numSelected == 1, "ListPickerFlyout for ComboBox is expected to return exactly one selected item.");

                        IFC(spSelectedItems->GetAt(0, &spSelectedItemFromResults));
                        IFC(m_spPendingFlyout->get_SelectedItem(&spSelectedItem));

                        IFC(Private::ValueComparisonHelpers::AreEqual(spSelectedItem.Get(), spSelectedItemFromResults.Get(), &areEqual));
                        ASSERT(areEqual, "The result of the async selection operation does not match the state of the ListPickerFlyout.");
                    }
                }
#else
                UNREFERENCED_PARAMETER(pAsyncInfo);
#endif // DBG

                if (asyncStatus == wf::AsyncStatus::Completed)
                {
                    wrl::ComPtr<wfc::IVectorView<IInspectable*>> spSelectedItems;
                    IFC(pAsyncInfo->GetResults(&spSelectedItems));
                    if (spSelectedItems)
                    {
                        wrl::ComPtr<IInspectable> spSelectedItemFromResults;
                        wrl::ComPtr<xaml::IUIElement> spSelectedItemAsUI;

                        IFC(spSelectedItems->GetAt(0, &spSelectedItemFromResults));
                        if (SUCCEEDED(spSelectedItemFromResults.As(&spSelectedItemAsUI)))
                        {
                            // Selected item is a UIElement. An error will occur if the consumer
                            // of this operation adds the selected item to the visual tree, as
                            // it is already present in the flyout, so if the flyout is still open,
                            // wait for it to close before completing the operation.
                            doWaitForFlyoutClose = true;
                        }
                    }

                    IFC(m_spPendingFlyout->get_SelectedIndex(&m_selectedIndex));
                }
                else
                {
                    m_selectedIndex = -1;
                }

                if (!doWaitForFlyoutClose)
                {
                    IFC(CompleteOperation());
                }

            Cleanup:
                return hr;
            }

            _Check_return_ HRESULT CompleteOperation()
            {
                HRESULT hr = S_OK;

                if (!m_isCompleted)
                {
                    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

                    IFC(m_spPendingFlyout.As(&spFlyoutBase));
                    IFC(spFlyoutBase->remove_Closed(m_closedToken));

                    m_isCompleted = true;
                    m_spInnerAsyncInfo.Reset();
                    m_spPendingFlyout.Reset();
                    IFC(FireCompletionImpl());
                }

            Cleanup:
                return hr;
            }

            EventRegistrationToken m_closedToken;
            wrl::ComPtr<wf::IAsyncInfo> m_spInnerAsyncInfo;
            wrl::ComPtr<xaml_controls::IListPickerFlyout> m_spPendingFlyout;
            bool m_isCompleted;
            INT32 m_selectedIndex{};
    };

} } } } XAML_ABI_NAMESPACE_END