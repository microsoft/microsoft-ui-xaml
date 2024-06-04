// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CUIElement;
class ICoreAsyncAction;
class CRenderTargetElementData;
class RenderTargetBitmapPixelWaitExecutor;
class CWindowRenderTarget;
class HWTexture;
class DCompTreeHost;
struct IPALEvent;

class RenderTargetBitmapImplBase
{
public:
    virtual ~RenderTargetBitmapImplBase() {}

    int32_t GetPixelWidth() const { return m_pixelWidth; }
    int32_t GetPixelHeight() const { return m_pixelHeight; }

    // RTB differentiates between surface bounds and layout bounds.
    // This is because we create RTB surface appropriate for zoom scale.
    // So if the zoom scaled surface bounds are used for layout as well
    // then it gets scaled again during render, thus making things blurry.
    // Hence we use unscaled dimension for layout.
    uint32_t GetLayoutWidth() const { return m_layoutWidth; }
    uint32_t GetLayoutHeight() const { return m_layoutHeight; }

    virtual _Check_return_ HRESULT RequestRender(
        _In_ CUIElement* pElement,
        int32_t scaledWidth,
        int32_t scaledHeight,
        _In_ ICoreAsyncAction *pAsyncAction);

    virtual _Check_return_ HRESULT PreCommit(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ HWTexture* hwTexture,
        _In_opt_ IPALEvent* completionEvent)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT PostDraw();

    virtual _Check_return_ HRESULT FailRender(
        HRESULT failureHR);

    virtual void CleanupDeviceRelatedResourcesRecursive(
        bool cleanupDComp);


    CRenderTargetElementData* GetRenderTargetElementData() const
    {
        return m_renderTargetElementData.get();
    }

    void SetRenderTargetElementData(_In_ CRenderTargetElementData* renderTargetElementData)
    {
        m_renderTargetElementData = renderTargetElementData;
    }

protected:

    _Check_return_ HRESULT CompleteOperation(HRESULT hr);

    int32_t m_pixelWidth = 0;
    int32_t m_pixelHeight = 0;
    uint32_t m_layoutWidth = 0;
    uint32_t m_layoutHeight = 0;

    xref_ptr<CRenderTargetElementData> m_renderTargetElementData;
};
