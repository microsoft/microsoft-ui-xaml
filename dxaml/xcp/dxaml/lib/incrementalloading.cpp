// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides the async operation implementation required for ListViewBase's
//      incremental loading functionality.

#include "precomp.h"
#include "incrementalloading.h"
#include "ListViewBase.g.h"
#include "ListViewBaseItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

IncrementalLoadAsyncCompleteHandler::IncrementalLoadAsyncCompleteHandler()
{
    m_chainedInvokeCalled = false;
}

IncrementalLoadAsyncCompleteHandler::~IncrementalLoadAsyncCompleteHandler()
{
    if(m_spChainedAsyncOperation && !m_chainedInvokeCalled)
    {
        m_spChainedAsyncOperation->Close();
    }

    m_spChainedAsyncOperation = nullptr;
}

_Check_return_ HRESULT IncrementalLoadAsyncCompleteHandler::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wf::IAsyncOperationCompletedHandler<LoadMoreItemsResult>)))
    {
        *ppObject = static_cast<wf::IAsyncOperationCompletedHandler<LoadMoreItemsResult>*>(this);
    }
    else
    {
        return ComBase::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Called by data source when the incremental data load is completed.
IFACEMETHODIMP IncrementalLoadAsyncCompleteHandler::Invoke(
    _In_ wf::IAsyncOperation<LoadMoreItemsResult>* asyncInfo,
    _In_ wf::AsyncStatus status)
{
    HRESULT hr = S_OK;
    LoadMoreItemsResult result;

    // Notify the chained async operation of data load completion
    if (m_spChainedAsyncOperation)
    {
        IFC(asyncInfo->GetResults(&result));
        m_chainedInvokeCalled = true;
        IFC(m_spChainedAsyncOperation->RaisedCompleteWithResult(result.Count, status));
        m_spChainedAsyncOperation = nullptr;
    }
    else
    {
        ctl::ComPtr<IAsyncInfo> spAsyncInfo = NULL;
        spAsyncInfo.Attach(ctl::query_interface<IAsyncInfo>(asyncInfo));
        IFCPTR(spAsyncInfo.Get());
        IFC(spAsyncInfo.Get()->Close());
    }

Cleanup:
    RRETURN(hr);
}

void IncrementalLoadAsyncCompleteHandler::SetChainedAsyncOperation(
    _In_ LoadMoreItemsOperation* pChainedAsyncOperation)
{
    ASSERT(!m_spChainedAsyncOperation);
    m_spChainedAsyncOperation = pChainedAsyncOperation;
}


UINT32 LoadMoreItemsOperation::z_ulUniqueAsyncActionId = 1;

LoadMoreItemsOperation::LoadMoreItemsOperation()
    : m_count(0)
    , m_isDataProcessing(FALSE)
{}

_Check_return_
HRESULT
LoadMoreItemsOperation::Init(_In_ ListViewBase* pOwner, _In_ BOOLEAN isDataProcessing)
{
    m_isDataProcessing = isDataProcessing;
    RRETURN(ctl::AsWeak(pOwner, &m_wrListViewBase));
}

STDMETHODIMP LoadMoreItemsOperation::GetResults(
    _Inout_ LoadMoreItemsResult *results)
{
    results->Count = m_count;
    RRETURN(S_OK);
}

HRESULT LoadMoreItemsOperation::OnStart()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spListViewBase;

    // Application has called Start on the AsyncOperation we returned via ListViewBase.LoadMoreItemsAsync API.
    // Initiate incremental load on the data source.
    m_spDataSourceAsyncInfo = nullptr;
    spListViewBase = m_wrListViewBase.AsOrNull<IListViewBase>();
    if (spListViewBase)
    {
        IFC(spListViewBase.Cast<ListViewBase>()->LoadMoreItemsAsyncInternal(this, m_spDataSourceAsyncInfo.ReleaseAndGetAddressOf()));
    }

    // if we have an associated ListView and this listview does not generate an async info that is
    // because there's nothing else to load, we complete this operation synchronously
    if (!m_isDataProcessing && spListViewBase)
    {
        if (m_spDataSourceAsyncInfo)
        {
            wf::AsyncStatus status = wf::AsyncStatus::Completed;
            ctl::ComPtr<wf::IAsyncOperation<LoadMoreItemsResult>> spAsyncOperation;

            IFC(m_spDataSourceAsyncInfo->get_Status(&status));

            if (status == wf::AsyncStatus::Completed)
            {
                LoadMoreItemsResult result = {0};

                // Since the task is completed then we're done, just raise our completed handler
                // and be done
                IFC(m_spDataSourceAsyncInfo.As<wf::IAsyncOperation<LoadMoreItemsResult>>(&spAsyncOperation));

                IFC(spAsyncOperation->GetResults(&result));

                IFC(RaisedCompleteWithResult(result.Count, wf::AsyncStatus::Completed));
            }
        }
        else
        {
            IFC(RaisedCompleteWithResult(0, wf::AsyncStatus::Completed));
        }
    }

Cleanup:
    RRETURN(hr);
}

void LoadMoreItemsOperation::OnCancel()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    // Application has called Cancel on the AsyncOperation we returned via ListViewBase.LoadMoreItemsAsync API.
    // Call Cancel on the AsyncOperation of the DataSource.
    if (m_spDataSourceAsyncInfo)
    {
        IFC(m_spDataSourceAsyncInfo->Cancel());
    }


Cleanup:
    m_isDataProcessing = FALSE;
    m_spThis = nullptr;
}

void LoadMoreItemsOperation::OnClose()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    // Application has called Close on the AsyncOperation we returned via ListViewBase.LoadMoreItemsAsync API.
    // Call Close on the AsyncOperation of the DataSource.
    if (m_spDataSourceAsyncInfo)
    {
        IFC(m_spDataSourceAsyncInfo->Close());
    }

Cleanup:
    m_isDataProcessing = FALSE;
    m_spDataSourceAsyncInfo = nullptr;
    m_spThis = nullptr;
}

// Raises completed event to the caller of ListViewBase.LoadMoreItemsAsync
_Check_return_ HRESULT LoadMoreItemsOperation::RaisedCompleteWithResult(
    _In_ unsigned result,
    _In_ wf::AsyncStatus status)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spListViewBase;

    if (m_isDataProcessing)
    {
        m_count = result;
        switch (status)
        {
        case wf::AsyncStatus::Canceled:
            Cancel();
            break;
        case wf::AsyncStatus::Error:
            TryTransitionToError(E_FAIL);
            break;
        }

        // do not raise completion event if ListViewBase destroyed or marker as ready for GC.
        spListViewBase = m_wrListViewBase.AsOrNull<IListViewBase>();
        if (spListViewBase)
        {
            IFC(AsyncBase::FireCompletion());
        }
    }

    m_spThis = nullptr;

Cleanup:
    RRETURN(hr);
}
