// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <coreasyncaction.h>
#include <RenderTargetBitmapTypes.h>
#include <RenderTargetBitmapImplBase.h>

class RenderTargetBitmapPixelWaitExecutor;
class CRenderTargetElementData;
class HWTexture;
class RenderTargetBitmapBase;

class CRenderTargetBitmap final
    : public CImageSource
    , public IRenderTargetElement
    , public IRenderTargetBitmapExecutorControl
{
protected:
    CRenderTargetBitmap(_In_ CCoreServices* core);

public:

    ~CRenderTargetBitmap() override;
    DECLARE_CREATE(CRenderTargetBitmap);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRenderTargetBitmap>::Index;
    }

    _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager* textureManager,
        _In_ CWindowRenderTarget* renderTarget,
        _In_ SurfaceCache* surfaceCache) override
    {
        RRETURN(S_OK);
    }

    int32_t GetPixelWidth() const override;
    int32_t GetPixelHeight() const override;

    uint32_t GetLayoutWidth() override;
    uint32_t GetLayoutHeight() override;

    _Check_return_ HRESULT RequestRender(
        _In_ CUIElement* element,
        _In_ int32_t scaledWidth,
        _In_ int32_t scaledHeight,
        _In_ ICoreAsyncAction* asyncAction);
    _Check_return_ HRESULT RequestPixels(_In_ IRenderTargetBitmapGetPixelsAsyncOperation* asyncOperation);

    _Check_return_ HRESULT PreCommit(
        _In_ CWindowRenderTarget* renderTarget,
        _In_opt_ IPALEvent* completionEvent) override;

    _Check_return_ HRESULT PostDraw() override;
    _Check_return_ HRESULT OnDrawingCompleteForFrame() override { return S_OK; }
    _Check_return_ HRESULT FailRender(HRESULT failureHR) override;

    RenderTargetElementState GetCurrentState() const override;
    _Check_return_ HRESULT SetCurrentState(RenderTargetElementState currentState) override;

    _Ret_notnull_ CRenderTargetElementData* GetRenderTargetElementData() const override;

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    void AbortPixelWaitExecutors(HRESULT resultHR) override;

    _Check_return_ HRESULT AddPixelWaitExecutor(_In_ RenderTargetBitmapPixelWaitExecutor* executor) override;
    _Check_return_ HRESULT RemovePixelWaitExecutor(_In_ RenderTargetBitmapPixelWaitExecutor* executor) override;

    static _Check_return_ HRESULT PixelWidth(
        _In_ CDependencyObject* object,
        uint32_t argCount,
        _In_reads_(argCount) CValue* args,
        _In_opt_ IInspectable* valueOuter,
        _Out_ CValue* result
        );

    static _Check_return_ HRESULT PixelHeight(
        _In_ CDependencyObject *object,
        uint32_t argCount,
        _In_reads_(argCount) CValue *args,
        _In_opt_ IInspectable* valueOuter,
        _Out_ CValue *result
        );

    uint32_t GetWidth() const override { return m_pImageSurfaceWrapper->GetHardwareWidth(); }
    uint32_t GetHeight() const override { return m_pImageSurfaceWrapper->GetHardwareHeight(); }
    bool HasDimensions() override { return m_pImageSurfaceWrapper->CheckForHardwareResources(); }

    static bool IsAncestorTypeEligible(_In_ CUIElement* element);

    bool HasHardwareResources() override { return CheckForHardwareResources(); }
    bool HasLostHardwareResources() override { return CheckForLostHardwareResources(); }
    void CleanupHardwareResources(_In_ bool cleanupDComp) override { CleanupDeviceRelatedResourcesRecursive(cleanupDComp); }

    bool RequiresSurfaceContentsLostNotification() override { return true; }

protected:
    void RegisterForCleanupOnLeave() override
    {
        // RTB is already on another global list and gets specially cleaned
        // up through it. Hence to avoid duplicate tracking do nothing.
    }

    void UnregisterForCleanupOnEnter() override
    {
        // RTB is already on another global list and gets specially cleaned
        // up through it. Hence to avoid duplicate tracking do nothing.
    }

private:
    _Check_return_ HRESULT InitInstance() override;

    bool HasValidTexture();

    RenderTargetElementState m_currentState;
    std::unique_ptr<RenderTargetBitmapImplBase> m_impl;
    wistd::unique_ptr<CXcpList<RenderTargetBitmapPixelWaitExecutor>> m_pixelWaitExecutorList;
    xref_ptr<HWTexture> m_rtbTexture;
};
