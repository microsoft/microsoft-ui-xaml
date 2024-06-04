// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "wrl\wrappers\corewrappers.h"
#include <RefCounting.h>
#include <palgfx.h>

class CWindowRenderTarget;
class CD3D11Device;
class CD2DFactory;
class WindowsPresentTarget;
class DCompTreeHost;
class CompositorScheduler;
class RefreshRateInfo;

struct ID3D11Device;
struct ID2D1Device;
struct IDCompositionDesktopDevicePartner;
struct IPALWorkItemFactory;

class WindowsGraphicsDeviceManager final : public CXcpObjectBase<IObject>
{
private:
    WindowsGraphicsDeviceManager();

    ~WindowsGraphicsDeviceManager() override;

public:
    static _Check_return_ HRESULT Create(_Outptr_ WindowsGraphicsDeviceManager **ppGraphicsDeviceManager);

    _Check_return_ HRESULT GetGraphicsDeviceForVideo(_Outptr_ CD3D11Device **ppVideoGraphicsDevice);

    void CleanupCachedDeviceResources(_In_ bool cleanupDComp, _In_ bool isDeviceLost);

    _Check_return_ HRESULT StartResourceCreation();

    _Check_return_ HRESULT WaitForD3DDependentResourceCreation();
    _Check_return_ HRESULT EnsureDCompDevice();

    CD3D11Device* GetGraphicsDevice() { return m_cachedD3DDevice.get(); }

    CD3D11Device* GetVideoGraphicsDeviceNoRef() const;

    void SetRenderTarget(_In_ void *pIRenderTarget)
    {
        m_pRenderTargetNoRef = reinterpret_cast<CWindowRenderTarget*>(pIRenderTarget);
    }

    void SetCompositorScheduler(_In_ CompositorScheduler* compositorScheduler);

    CWindowRenderTarget* GetRenderTarget() const
    {
        return m_pRenderTargetNoRef;
    }

    CompositorScheduler* GetCompositorScheduler() const
    {
        return m_pCompositorSchedulerNoRef;
    }

    DCompTreeHost *GetDCompTreeHost() const
    {
        return m_pDCompTreeHost;
    }

    RefreshRateInfo* GetRefreshRateInfo() const
    {
        return m_refreshRateInfo.get();
    }

   _Check_return_ bool CanContinueOnInternalDriverError();
#if DBG
    _Check_return_ HRESULT CreateFinalReleaseAsserter(
        _Outptr_ IPALDebugDeviceFinalReleaseAsserter **ppAsserter);
#endif

    void SetAtlasSizeHint(XUINT32 width, XUINT32 height);
    void ResetAtlasSizeHint();
    void DisableAtlas();

    HRESULT GetResourceCreationHR() { return m_hrInitializeWorkItem; }

    bool IsInitialized() { return m_isInitialized; }

    _Check_return_ HRESULT EnsureD2DResources();

private:
    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT WaitForInitializationThreadCompletion();

    // Static entry point for initialize thread.
    // Matches the signature of PALWORKITEMCALLBACK required by IPALWorkItems
    static DWORD WINAPI InitializeCallback(
        _In_opt_ void *pData
        );

    _Check_return_ HRESULT InitializeExpensiveResources();

    _Check_return_ HRESULT DrawDummyText();

private:
    HANDLE m_initializationThreadHandle = NULL;
    HRESULT m_hrInitializeWorkItem;
    wil::critical_section m_InitializeWorkItemLock;

    xref_ptr<CD3D11Device> m_cachedD3DDevice;
    xref_ptr<CD3D11Device> m_videoGraphicsDevice;

    xref_ptr<RefreshRateInfo> m_refreshRateInfo;
    DCompTreeHost *m_pDCompTreeHost;

    CWindowRenderTarget* m_pRenderTargetNoRef;
    CompositorScheduler *m_pCompositorSchedulerNoRef;

    unsigned __int8 m_dxgiInternalErrorCount = 0;
    boolean m_dxgiUseWarpOnly = false;
    ULONGLONG m_dxgiLastInternalErrorTickCount = 0;

    bool m_isInitialized;
};

#if DBG
//------------------------------------------------------------------------------
//
//  Synopsis:
//      Class for helper object which can be used to assert
//      actual deletion of devices.
//
//------------------------------------------------------------------------------
class DebugDeviceFinalReleaseAsserter
    : public CXcpObjectBase<IPALDebugDeviceFinalReleaseAsserter>
{
public:
    DebugDeviceFinalReleaseAsserter(
        bool hasInteropCompositor,
        _In_opt_ ID3D11Device *pD3D11Device,
        _In_opt_ ID2D1Device *pD2DDevice,
        _In_opt_ IDCompositionDesktopDevicePartner *pDCompDevice);
    ~DebugDeviceFinalReleaseAsserter() override;
    void ReleaseAllWithAssert() override;

    _Maybenull_ ID3D11Device *m_pD3D11Device;
    _Maybenull_ ID2D1Device *m_pD2DDevice;
    _Maybenull_ IDCompositionDesktopDevicePartner *m_pDCompDevice;
    bool m_hasInteropCompositor;
};
#endif
