// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RenderTargetBitmapTypes.h"

class CRenderTargetElementData;
class CompositorTreeHost;
class CWindowRenderTarget;
class CUIElement;
class ICoreAsyncAction;
class HWRealization;
class HWCompNode;
class HWCompTreeNode;
class TransformAndClipStack;
class RenderTargetBitmapImplBase;
struct IPALEvent;

struct IRenderTargetElement
{
    virtual _Check_return_ HRESULT FailRender(HRESULT failureHR) =0;

    virtual _Check_return_ HRESULT PreCommit(
        _In_ CWindowRenderTarget* renderTarget,
        _In_opt_ IPALEvent* completionEvent) =0;
    virtual _Check_return_ HRESULT PostDraw() =0;
    virtual _Check_return_ HRESULT OnDrawingCompleteForFrame() =0;

    virtual _Check_return_ HRESULT SetCurrentState(RenderTargetElementState currentState) =0;
    virtual RenderTargetElementState GetCurrentState() const =0;

    virtual CRenderTargetElementData* GetRenderTargetElementData() const =0;

    virtual bool HasHardwareResources() =0;
    virtual bool HasLostHardwareResources() =0;
    virtual void CleanupHardwareResources(_In_ bool cleanupDComp) =0;

    virtual void AbortPixelWaitExecutors(HRESULT resultHR) =0;

    virtual bool RequiresSurfaceContentsLostNotification() =0;
};


class CRenderTargetElementData : public CXcpObjectBase<IObject>
{
public:

#if DBG
        FORWARD_ADDREF_RELEASE(CXcpObjectBase<IObject>);
#endif /* DBG */

    static _Check_return_ HRESULT Create(
        _In_ CUIElement* renderElement,
        int32_t scaledWidth,
        int32_t scaledHeight,
        _In_ ICoreAsyncAction* renderAction,
        _In_ RenderTargetBitmapImplBase* renderTargetBitmapImpl,
        _Outptr_ CRenderTargetElementData** renderTargetBitmapData
        );

    CompositorTreeHost* GetCompositorTreeHost() const { return m_pCompositorTreeHost; }
    _Check_return_ HRESULT GetCompositionPeer(_In_ CUIElement *pElement, _Outptr_ HWCompTreeNode **ppCompositionPeer) const;
    _Check_return_ HRESULT EnsureCompositionPeer(
        _In_ CUIElement *pElement,
        _In_ HWCompTreeNode *pCompositionPeer,
        _In_opt_ HWCompTreeNode *pParentNode,
        _In_ HWCompNode *pPreviousSibling);
    CUIElement* GetRenderElement() const { return m_pRenderElement; }
    TransformAndClipStack* GetPrependTransformAndClip() const { return m_pPrependTransformAndClip; }
    _Check_return_ HRESULT UpdateMetrics();


    XUINT32 GetLayoutWidth() const { return m_layoutWidth; }
    XUINT32 GetLayoutHeight() const { return m_layoutHeight; }

    XINT32 GetPixelWidth() const { return m_pixelWidth; }
    XINT32 GetPixelHeight() const { return m_pixelHeight; }

    void SetPixelWidth(XUINT32 width) { m_pixelWidth = width; }
    void SetPixelHeight(XUINT32 height) { m_pixelHeight = height; }

    ICoreAsyncAction* GetRenderAsyncAction() const { return m_pRenderAsyncAction; };
    void CompleteRenderAsyncAction(HRESULT resultHR);
    void ReleaseRenderAsyncAction();

    void SaveHWRealization(_In_ HWRealization* const pRealization);
    void ClearHWRealizations();
    void CleanupDeviceRelatedResources();

    void ResetState();

protected:

    CRenderTargetElementData(
        _In_ CUIElement *pRenderElement,
        _In_ ICoreAsyncAction *pRenderAction,
        _In_ XINT32 scaledWidth,
        _In_ XINT32 scaledHeight,
        _In_ RenderTargetBitmapImplBase* renderTargetBitmapImpl);
    ~CRenderTargetElementData() override;

    _Check_return_ HRESULT Initialize();
    void ClearCompNodeMap();

    CUIElement *m_pRenderElement;
    CUIElement *m_pConnectionElement;
    CompositorTreeHost *m_pCompositorTreeHost{};
    TransformAndClipStack *m_pPrependTransformAndClip;
    xchainedmap<CUIElement*, HWCompTreeNode*> m_mapUIElementNoRefToCompTreeNode;
    ICoreAsyncAction *m_pRenderAsyncAction;

    std::vector<HWRealization*> m_realizations;

    XUINT32 m_pixelWidth;
    XUINT32 m_pixelHeight;
    XUINT32 m_layoutWidth;
    XUINT32 m_layoutHeight;
    XINT32 m_scaleWidth;
    XINT32 m_scaleHeight;

    RenderTargetBitmapImplBase* m_renderTargetBitmapImplNoRef;
};
