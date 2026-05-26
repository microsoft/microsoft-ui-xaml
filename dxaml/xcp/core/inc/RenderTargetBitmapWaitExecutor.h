// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CRenderTargetBitmap;

//------------------------------------------------------------------------
//
//  Synopsis:
//      UIThread execution operation.
//      This gets passed on to a thread pool work item
//      which waits on the given event and then
//      queues this to the UI thread.
//      This class gets used signaling the end of RenderAsync operation.
//
//------------------------------------------------------------------------
class RenderTargetBitmapWaitExecutor :
    public CXcpObjectBase<IPALExecuteOnUIThread, CXcpObjectThreadSafeAddRefPolicy>,
    public IObject
{
public:
    RenderTargetBitmapWaitExecutor(CCoreServices* core, IPALWaitable* waitBeforeExecuteHandle)
        : m_coreNoRef(core)
        , m_waitBeforeExecuteHandleNoRef(waitBeforeExecuteHandle)
    {
        XCP_WEAK(&m_coreNoRef);
    }

    uint32_t AddRef() final
    {
        return CXcpObjectBase<IPALExecuteOnUIThread, CXcpObjectThreadSafeAddRefPolicy>::AddRef();
    }
    uint32_t Release() final
    {
        return CXcpObjectBase<IPALExecuteOnUIThread, CXcpObjectThreadSafeAddRefPolicy>::Release();
    }
    _Check_return_ HRESULT Execute() override;
    virtual _Check_return_ HRESULT WaitAndExecuteOnUIThread();

protected:
    ~RenderTargetBitmapWaitExecutor() override
    {
        VERIFYHR(m_waitBeforeExecuteHandleNoRef->Close());
    }

    CCoreServices* m_coreNoRef;
    IPALWaitable* m_waitBeforeExecuteHandleNoRef;
};

//------------------------------------------------------------------------
//
//  Synopsis:
//      UIThread execution operation.
//      This gets passed on to a thread pool workitem
//      which waits on the given event and then
//      queues this to the UI thread.
//      This class gets used signaling the end of GetPixelsAsync operation.
//
//------------------------------------------------------------------------
class RenderTargetBitmapPixelWaitExecutor :
    public RenderTargetBitmapWaitExecutor
{
public:
    RenderTargetBitmapPixelWaitExecutor(
        CCoreServices* core,
        IPALWaitable* waitBeforeExecuteHandle,
        _In_ IPALByteAccessSurface* byteAccessSurface,
        _In_ IRenderTargetBitmapGetPixelsAsyncOperation* asyncOperation,
        _In_ IRenderTargetBitmapExecutorControl* renderTargetBitmapExecutorControl)
        : RenderTargetBitmapWaitExecutor(core, waitBeforeExecuteHandle)
        , m_byteAccessSurface(byteAccessSurface)
        , m_asyncOperation(asyncOperation)
        , m_renderTargetBitmapExecutorControlNoRef(renderTargetBitmapExecutorControl)
    {
        XCP_WEAK(&m_coreNoRef);
    }

    ~RenderTargetBitmapPixelWaitExecutor() override;

    _Check_return_ HRESULT Execute() override;
    void AbortAsyncOperation(HRESULT resultHR);

protected:

    xref_ptr<IPALByteAccessSurface> m_byteAccessSurface;
    IRenderTargetBitmapGetPixelsAsyncOperation* m_asyncOperation;
    IRenderTargetBitmapExecutorControl* m_renderTargetBitmapExecutorControlNoRef;
};

