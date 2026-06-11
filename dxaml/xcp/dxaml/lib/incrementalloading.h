// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides the async operation implementation required for ListViewBase's
//      incremental loading functionality.

#pragma once

#include "DXamlAsyncBase.h"

namespace DirectUI
{
    typedef wf::IAsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult> LoadMoreItemsOperationCompletedHandler;
    class LoadMoreItemsOperation:
        public DXamlAsyncBaseImpl<LoadMoreItemsOperationCompletedHandler, wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>>
    {
        friend class ListViewBase;

        InspectableClass(wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>::z_get_rc_name_impl(), BaseTrust);

        private:
            unsigned m_count;
            ctl::WeakRefPtr m_wrListViewBase;
            // We can't use a TrackerPtr here since this isn't a WeakReferenceSourceNoThreadId.
            ctl::ComPtr<IAsyncInfo> m_spDataSourceAsyncInfo;
            // Used to hold a ref to self while invoking completion handler
            Microsoft::WRL::ComPtr<LoadMoreItemsOperation> m_spThis;
            // Skip raising event if data is yet processing
            BOOLEAN m_isDataProcessing;

        public:

                // wf::IAsyncAction<LoadMoreItemsResults> forwarders
                IFACEMETHOD(put_Completed)(_In_ LoadMoreItemsOperationCompletedHandler *pCompletedHandler) override
                {
                    return __super::PutOnComplete(pCompletedHandler);
                }

                IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  LoadMoreItemsOperationCompletedHandler **ppCompletedHandler) override
                {
                    return AsyncBase::GetOnComplete(ppCompletedHandler);
                }

                IFACEMETHOD(GetResults) (
                    _Inout_ xaml_data::LoadMoreItemsResult *results) override;

                BOOLEAN HasStatus() const { return !!m_spDataSourceAsyncInfo; }

                _Check_return_ HRESULT GetStatus(_Out_ wf::AsyncStatus* pStatus)
                {
                    RRETURN(m_spDataSourceAsyncInfo->get_Status(pStatus));
                }

            public:
                LoadMoreItemsOperation();

                _Check_return_ HRESULT Init(_In_ ListViewBase* pOwner, _In_ BOOLEAN isDataProcessing);

                STDMETHOD(RuntimeClassInitialize)(void)
                {
                    HRESULT hr = S_OK;
                    auto id = z_ulUniqueAsyncActionId;

                    InterlockedIncrement(&z_ulUniqueAsyncActionId);

                    AsyncBase::put_Id(id);
                    m_spThis = this;
                    return hr;
                }
            private:
                // Used to assign unique ids to AsyncActions
                static UINT32 z_ulUniqueAsyncActionId;

            protected:
                // wf::IAsyncAction overrides
                HRESULT OnStart(void) override;
                void OnClose(void) override;
                void OnCancel(void) override;

            public:
            // Raises completed event to the caller of ListViewBase.LoadMoreItemsAsync
// REVIEW: should this have status arg?
            _Check_return_ HRESULT RaisedCompleteWithResult(
                _In_ unsigned result,
                _In_ wf::AsyncStatus status);

    };

    class IncrementalLoadAsyncCompleteHandler
        : public wf::IAsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>
        , public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(IncrementalLoadAsyncCompleteHandler, ctl::ComBase)
            INTERFACE_ENTRY(IncrementalLoadAsyncCompleteHandler, wf::IAsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult> )
        END_INTERFACE_MAP(IncrementalLoadAsyncCompleteHandler, ctl::ComBase)

        private:
            Microsoft::WRL::ComPtr<LoadMoreItemsOperation> m_spChainedAsyncOperation;
            bool m_chainedInvokeCalled;

        public:
            // Called by data source when the incremental data load is
            // completed.
            STDMETHODIMP Invoke(
                _In_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>* asyncInfo,
                _In_ wf::AsyncStatus status) override;

            void SetChainedAsyncOperation(
                _In_ LoadMoreItemsOperation* pChainedAsyncOperation);

        protected:
            IncrementalLoadAsyncCompleteHandler();

            ~IncrementalLoadAsyncCompleteHandler() override;

            _Check_return_ HRESULT QueryInterfaceImpl(
                _In_ REFIID iid,
                _Outptr_ void** ppObject) override;
    };
}
