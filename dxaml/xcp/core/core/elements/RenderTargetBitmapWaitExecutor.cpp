// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RenderTargetBitmapWaitExecutor.h"

_Check_return_ HRESULT
RenderTargetBitmapWaitExecutor::WaitAndExecuteOnUIThread()
{
    while (!m_coreNoRef->IsDestroyingCoreServices())
    {
        if (WAIT_OBJECT_0 == gps->WaitForObjects(1, &m_waitBeforeExecuteHandleNoRef, TRUE, 200 /* nTimeout */))
        {
            // Things could have changed during the wait. Hence check again.
            if (!m_coreNoRef->IsDestroyingCoreServices())
            {
                IFC_RETURN(m_coreNoRef->ExecuteOnUIThread(this, ReentrancyBehavior::AllowReentrancy));
            }
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
RenderTargetBitmapWaitExecutor::Execute()
{
    IFC_RETURN(m_coreNoRef->GetRenderTargetBitmapManager()->NotifyDrawCompleted(m_waitBeforeExecuteHandleNoRef));

    return S_OK;
}

RenderTargetBitmapPixelWaitExecutor::~RenderTargetBitmapPixelWaitExecutor()
{
    if (m_asyncOperation != nullptr)
    {
        m_asyncOperation->CoreReleaseRef();
        m_asyncOperation = nullptr;
    }

    if (m_renderTargetBitmapExecutorControlNoRef != nullptr)
    {
        VERIFYHR(m_renderTargetBitmapExecutorControlNoRef->RemovePixelWaitExecutor(this));
    }
}

_Check_return_ HRESULT
RenderTargetBitmapPixelWaitExecutor::Execute()
{
    HRESULT hr = S_OK;

    uint8_t* bytes = nullptr;
    if (!m_coreNoRef->IsDestroyingCoreServices())
    {
        // The surface could have been reclaimed due to
        // the device loss. In such cases do nothing.
        if (m_byteAccessSurface != nullptr)
        {
            uint32_t length = 0;
            IFC(m_byteAccessSurface->ReadBytes(length, &bytes));
            RenderTargetBitmapPixelData pixelData;
            pixelData.m_length = length;
            pixelData.m_pBytes = bytes;

            // Complete the async operation with appropriate results.
            IFC(m_asyncOperation->CoreSetResults(pixelData));
            m_asyncOperation->CoreFireCompletion();
            m_asyncOperation->CoreReleaseRef();
            m_asyncOperation = nullptr;

            // Unlink self from the RTB.
            IFC(m_renderTargetBitmapExecutorControlNoRef->RemovePixelWaitExecutor(this));
            m_renderTargetBitmapExecutorControlNoRef = nullptr;
        }
    }
Cleanup:
    if (FAILED(hr))
    {
        SAFE_DELETE_ARRAY(bytes);
    }

    if (GraphicsUtility::IsDeviceLostError(hr))
    {
        // Device could have been lost after the check
        // at the beginning of the method. Hence check again.
        // Calling DetermineDeviceLost would update the
        // state on core and request another frame if needed.
        bool deviceLost = false;
        VERIFYHR(m_coreNoRef->DetermineDeviceLost(&deviceLost));
        ASSERT(deviceLost);

        VERIFYHR(m_renderTargetBitmapExecutorControlNoRef->RemovePixelWaitExecutor(this));
        // TODO: RTB Set appropriate error for device lost.
        AbortAsyncOperation(E_FAIL);

        // TODO: DEVICELOST: Trace
        hr = S_OK;
    }
    return hr;
}

void
RenderTargetBitmapPixelWaitExecutor::AbortAsyncOperation(HRESULT resultHR)
{
    ASSERT(m_asyncOperation != nullptr);
    m_asyncOperation->CoreSetError(resultHR);
    m_asyncOperation->CoreFireCompletion();
    m_asyncOperation->CoreReleaseRef();
    m_asyncOperation = nullptr;
    m_byteAccessSurface.reset();
    m_renderTargetBitmapExecutorControlNoRef = nullptr;
}
