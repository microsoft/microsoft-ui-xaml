// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RenderTargetElement.h>
#include "RenderTargetBitmapImplBase.h"

_Check_return_ HRESULT
RenderTargetBitmapImplBase::RequestRender(
    _In_ CUIElement* uiElement,
    _In_ int32_t scaledWidth,
    _In_ int32_t scaledHeight,
    _In_ ICoreAsyncAction* asyncAction)
{
    auto releaseAsyncActionOnExit = wil::scope_exit([&]
    {
        if (asyncAction != nullptr)
        {
            asyncAction->CoreReleaseRef();
        }
    });

    if (m_renderTargetElementData != nullptr)
    {
        m_renderTargetElementData->ReleaseRenderAsyncAction();
        m_renderTargetElementData.reset();
    }

    IFC_RETURN(CRenderTargetElementData::Create(
        uiElement,
        scaledWidth,
        scaledHeight,
        asyncAction,
        this,
        m_renderTargetElementData.ReleaseAndGetAddressOf()));
    asyncAction = nullptr;

    return S_OK;
}

_Check_return_ HRESULT
RenderTargetBitmapImplBase::PostDraw()
{
    return CompleteOperation(S_OK);
}

_Check_return_ HRESULT
RenderTargetBitmapImplBase::CompleteOperation(HRESULT hr)
{
    ASSERT(m_renderTargetElementData != nullptr);
    m_renderTargetElementData->CompleteRenderAsyncAction(hr);

    m_pixelWidth = m_renderTargetElementData->GetPixelWidth();
    m_pixelHeight = m_renderTargetElementData->GetPixelHeight();

    m_layoutWidth = m_renderTargetElementData->GetLayoutWidth();
    m_layoutHeight = m_renderTargetElementData->GetLayoutHeight();

    m_renderTargetElementData.reset();

    return S_OK;
}

_Check_return_ HRESULT
RenderTargetBitmapImplBase::FailRender(HRESULT failureHR)
{
    ASSERT(m_renderTargetElementData != nullptr);
    m_renderTargetElementData->CompleteRenderAsyncAction(failureHR);

    m_renderTargetElementData.reset();

    return S_OK;
}

void RenderTargetBitmapImplBase::CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp)
{
    if (m_renderTargetElementData != nullptr)
    {
        m_renderTargetElementData->CleanupDeviceRelatedResources();
    }
}
