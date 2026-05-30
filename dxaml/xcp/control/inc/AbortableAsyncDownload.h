// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Synopsis:
//      An abortable download to return when an async download request
//      is placed on the queue.

#pragma once

class CAbortableAsyncDownload final : public IPALAbortableOperation
{
public:
    _Check_return_ static HRESULT Create(
        _In_ CDownloadRequest * pDownloadRequest,
        _In_ IAsyncDownloadRequestManager* pAsyncDownloadRequestManager,
        _Outptr_ CAbortableAsyncDownload** ppAbortableDownload);

    void Abort() override;

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT SetAbortable(_In_ IPALAbortableOperation* pAbortable);

    void SetAsyncDownloadRequestManager(_In_opt_ IAsyncDownloadRequestManager* pManager)
    {
        m_pAsyncDownloadRequestManager = pManager;
    }

    IAsyncDownloadRequestManager* GetAsyncDownloadRequestManager()
    {
        return m_pAsyncDownloadRequestManager;
    }

protected:
    CAbortableAsyncDownload()
    {
        m_cRef = 1;
        m_pAbortable = NULL;
        m_pAsyncDownloadRequestManager = NULL;
        m_pRequest = NULL;
    }

private:
    ~CAbortableAsyncDownload() override
    {
        //if it still exists, tell the manager we're leaving
        if(m_pAsyncDownloadRequestManager)
        {
            if (FAILED(m_pAsyncDownloadRequestManager->RemoveAbortableAsyncDownload(this)))
            {
                if (m_pAbortable)
                {
                    // If we have an abortable set, the ADRM should always have us,
                    // so remove should have succeeded. Assert here to help find a
                    // stress/Watson bug (see Silverlight bug 26749).
                    ASSERT(FALSE);
                }
            }
        }
        ReleaseInterface(m_pAbortable);
    }

    CDownloadRequest *          m_pRequest;
    IAsyncDownloadRequestManager* m_pAsyncDownloadRequestManager;
    XUINT32                     m_cRef;
    IPALAbortableOperation*     m_pAbortable;
};

