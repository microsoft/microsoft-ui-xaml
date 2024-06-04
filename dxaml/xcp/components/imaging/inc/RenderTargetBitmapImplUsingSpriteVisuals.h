// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RenderTargetBitmapImplBase.h>

class CRenderTargetElementData;
class CUIElement;
class ICoreAsyncAction;
class DCompTreeHost;
class HWTexture;
class CaptureAsyncCompletionState;
class CRenderTargetBitmapManager;
class CD3D11Device;
struct IPALEvent;

using CompositionSurfaceAsyncOperation = wf::IAsyncOperation<WUComp::ICompositionSurface*>;

class RenderTargetBitmapImplUsingSpriteVisuals
    : public RenderTargetBitmapImplBase
{
public:

    RenderTargetBitmapImplUsingSpriteVisuals(
        _In_ CRenderTargetBitmapManager& rtbManager);
    ~RenderTargetBitmapImplUsingSpriteVisuals() override;

    _Check_return_ HRESULT RequestRender(
        _In_ CUIElement* element,
        int32_t scaledWidth,
        int32_t scaledHeight,
        _In_ ICoreAsyncAction* asyncAction
        ) override;

    _Check_return_ HRESULT PreCommit(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ HWTexture* hwTexture,
        _In_opt_ IPALEvent* completionEvent
        ) override;

    _Check_return_ HRESULT PostDraw() override;

    _Check_return_ HRESULT FailRender(
        HRESULT failureHR
        ) override;

    void CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp) override;

private:

    void Cleanup();

    static _Check_return_ HRESULT ProcessCaptureThreaded(
        _In_ WUComp::ICompositionSurface* inputCompSurface,
        _In_ HWTexture* outputHwTexture,
        _In_ CD3D11Device* graphicsDevice
        );

    CRenderTargetBitmapManager& m_rtbManager; // Managed by core services and guaranteed to survive the lifetime of the process
    xref_ptr<CUIElement> m_uiElement; // TODO: Can be removed in the future in favor of m_renderTargetElementData->GetRenderElement()
    ctl::ComPtr<CompositionSurfaceAsyncOperation> m_captureAsyncOperation;
    std::shared_ptr<CaptureAsyncCompletionState> m_captureAsyncCompletionState;
};
