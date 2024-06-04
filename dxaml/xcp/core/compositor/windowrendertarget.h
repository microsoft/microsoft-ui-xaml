// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      A window render target is responsible for maintaining a rendering
//      area that allows presentation of content to an OS window surface.

#pragma once

#include <IPLMListener.h>
#include <XcpList.h>

class CCoreServices;
class WindowsGraphicsDeviceManager;
class VisualTree;
class DCompTreeHost;
class CompositorTreeHost;
class HWWalk;
class CompositorScheduler;
struct IPALGraphicsDeviceChangeListener;
class WindowsPresentTarget;

class CWindowRenderTarget final
    : public CXcpObjectBase<IObject>
    , public IPLMListener
{
private:
    CWindowRenderTarget(
        _In_ CCoreServices* pCore,
        bool isPrimaryWindowTarget,
        _In_ CompositorScheduler* compositorScheduler,
        _In_ WindowsGraphicsDeviceManager* pGraphicsDeviceManager,
        _In_ WindowsPresentTarget* initialPresentTarget);

    ~CWindowRenderTarget() override;

public:
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IObject>);

    static _Check_return_ HRESULT Create(
        _In_ CCoreServices* pCore,
        bool isPrimaryWindowTarget,
        _In_ CompositorScheduler* compositorScheduler,
        _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
        _In_ WindowsPresentTarget* initialPresentTarget,
        _Outptr_ CWindowRenderTarget **ppRenderTarget);

    // Register/unregister a listener for graphics device changes
    _Check_return_ HRESULT RegisterGraphicsDeviceChangeListener(_In_ IPALGraphicsDeviceChangeListener *pListener);

    _Check_return_ HRESULT UnregisterGraphicsDeviceChangeListener(_In_ IPALGraphicsDeviceChangeListener *pListener);

    _Check_return_ HRESULT RequestMainDCompDeviceCommit();

    DCompTreeHost *GetDCompTreeHost();

    CCoreServices* GetCoreServicesNoRef() const
    {
        return m_pCoreNoRef;
    }

    XUINT32 GetWidth() const;

    XUINT32 GetHeight() const;

    void Retarget(_In_ WindowsPresentTarget* presentTarget);

    xref_ptr<WindowsPresentTarget> GetPresentTarget();

    // Called by the host whenever a (full or partial) redraw of the scene is necessary. Also allows the window render
    // target to force a full redraw after recovering from device loss.
    _Check_return_ HRESULT Draw(
        _In_ CCoreServices *pCore,
        bool fForceRedraw,
        _Inout_ bool * pFrameDrawn);

    _Check_return_ HRESULT CleanupResources(_In_ bool cleanupDComp, _In_ bool isDeviceLost);

    _Check_return_ HRESULT RebuildResources();

    // Implement IPLMListener methods
    _Check_return_ HRESULT OnSuspend(_In_ bool isTriggeredByResourceTimer) override;
    _Check_return_ HRESULT OnResume() override;
    void OnLowMemory() override {}

    WindowsGraphicsDeviceManager *GetGraphicsDeviceManager() const;

    CompositorTreeHost* GetCompositorTreeHost() const;

    HWWalk* GetHwWalk();

private:
    // Ensures presence of an appropriate present method.
    _Check_return_ HRESULT Initialize();

    // Ensures presence of a present method and of a matching compositor.
    _Check_return_ HRESULT InitializeResources();

private:
    xref_ptr<CompositorScheduler> m_compositorScheduler;
    xref_ptr<WindowsGraphicsDeviceManager> m_graphicsDeviceManager;

    xref_ptr<WindowsPresentTarget> m_presentTarget;

    _Notnull_ HWWalk *m_pHwWalk;
    _Maybenull_ CompositorTreeHost *m_pCompositorTreeHost{};

    bool m_isPrimaryWindowTarget : 1;
    // TODO: these need to be tracked per-compositor now that the render target can have many.
    bool m_needsFullRedraw : 1;
    bool m_hardwareResourcesHaveBeenReset : 1;

    // The core owns the browser host, which then owns a render target.
    CCoreServices *m_pCoreNoRef;

    // List of listeners that will be called back when the window render target
    // graphics device is created, re-created or released...
    CXcpList<IPALGraphicsDeviceChangeListener> m_graphicsDeviceChangeListenersNoRef;
};
