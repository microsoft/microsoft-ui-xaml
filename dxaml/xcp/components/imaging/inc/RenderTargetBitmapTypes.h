// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CoreAsyncAction.h"

enum class RenderTargetElementState
{
    //
    //  No render operation is under process.
    //  Both yet-to-start and completed states
    //  are captured with this state.
    //
    Idle,
    //
    //  A render request has arrived.
    //  It will be picked up in subsequent batches.
    //
    Preparing,
    //
    //  The RenderTargetBitmap has been picked up
    //  render walk.
    //
    Rendering,
    //
    //  Render walk has completed. About to draw the comp tree.
    //
    Rendered,
    //
    //  Content has been rendered and commit has been called.
    //
    Committed,
    //
    //  Draw has been submitted.
    //
    Drawing
};

struct RenderTargetBitmapPixelData
{
    RenderTargetBitmapPixelData()
    {
        m_length = 0;
        m_pBytes = NULL;
    }
    XUINT32 m_length;
    _Field_size_(m_length) XBYTE *m_pBytes;
};

class IRenderTargetBitmapGetPixelsAsyncOperation : public ICoreAsyncOperation<RenderTargetBitmapPixelData>
{};

class RenderTargetBitmapPixelWaitExecutor;

struct IRenderTargetBitmapExecutorControl
{
    virtual _Check_return_ HRESULT AddPixelWaitExecutor(_In_ RenderTargetBitmapPixelWaitExecutor *pExecutor) = 0;
    virtual _Check_return_ HRESULT RemovePixelWaitExecutor(_In_ RenderTargetBitmapPixelWaitExecutor *pExecutor) = 0;
};