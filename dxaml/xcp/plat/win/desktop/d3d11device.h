// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LockableGraphicsPointer.h"

#include <d3d11.h>
#include <palgfx.h>

#pragma warning(push)
#pragma warning(disable:28718)
#pragma warning(disable:28722)

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIAdapter;
struct IDXGIDevice2;
struct ID2D1Device;
struct ID2D1DeviceContext;
struct ID2D1SolidColorBrush;
struct IMFDXGIDeviceManager;
class CD3D11DeviceInstance;
class CD2DFactory;
class CD3D11SharedDeviceGuard;

class SystemMemoryBits;

#pragma warning(pop)

/*

    The CD3D11Device class is an object that is used in the rendering code to
    get back to the D3D device/device context, the D2D device/device context and
    the DXGI Device.  There is at least one of these per UI thread (Core Window).
    One driving the standard UI and additional ones that are created for media to
    use to embed content in the UI.

    Prior to Threshold, each instance of this class created its own instances of
    of the D2D and D3D devices. During Threshold, this was modified to allow
    multiple UI threads to share a single D3D Device/D2D Device combination.  To
    accomplish this, all the sharable objects were moved out to CD3D11DeviceInstance
    and this class (CD3D11Device) now acts as an encapsulation wrapper around
    a shared instance of CD3D11DeviceInstance.

    The thread unique objects (e.g. ID2DDeviceContext, DXGIDeviceManager) still
    continue to be handled by this class.

    Note also, that we don't always use a shared version CD3D11DeviceInstance.  For
    example, media creates it own CD3D11DeviceInstance, but it sort of does its own
    thing, we go ahead and give them a new underlying CD3D11DeviceInstance
    each time.

*/

class CD3D11Device final : public CXcpWeakObjectBase
{
public:
#if DBG
    FORWARD_ADDREF_RELEASE(CXcpWeakObjectBase);
#endif /* DBG */
    static _Check_return_ HRESULT Create(
        _Outptr_ CD3D11Device **ppD3D11Device,
        _In_ bool useUniqueDevice
        );

    _Check_return_ HRESULT EnsureResources();

    _Check_return_ HRESULT GetTextureMemoryUsage(
        _Out_ UINT32 *puTextureMemoryUsage,
        _Out_ UINT32 *puTextureMemoryUsageNPOT
        );

    //
    // CD3D11Device methods
    //
    _Check_return_ HRESULT EnsureD2DResources();

    _Ret_notnull_ ID3D11Device *GetDevice(_In_ const CD3D11SharedDeviceGuard* guard) const;

    ID3D11Device *TestHook_GetDevice() const;

    CLockedGraphicsPointer<ID3D11DeviceContext> GetLockedDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard);

    ID2D1Device* GetD2DDevice(_In_ const CD3D11SharedDeviceGuard* guard) const;

    ID2D1DeviceContext* GetD2DDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard) const;

    CD2DFactory* GetD2DFactory() const;

    ID2D1SolidColorBrush* GetD2DSolidColorBrush(_In_ const CD3D11SharedDeviceGuard* guard) const;

    _Check_return_  HRESULT InitializeVideoDevice( );

    _Check_return_ HRESULT CreateEventAndEnqueueWait(
        _Outptr_ IPALWaitable **ppWaitHandle
        );

    _Check_return_ HRESULT EnqueueSetEvent(
        _In_ IPALEvent *pEvent,
        _Out_ bool *pSetEventEnqueued
        );

    _Check_return_ HRESULT RegisterDeviceRemovedEvent(_In_ HANDLE event, _Out_ DWORD* cookie);
    void UnregisterDeviceRemoved(DWORD cookie);

    _Check_return_ HRESULT InitializeWithD3DDeviceAndContext(
                            _In_ ID3D11Device *pD3DDevice,
                            _In_ ID3D11DeviceContext *pD3DDeviceContext);

    void TrimMemory();

    _Check_return_ HRESULT ReleaseScratchResources();

    bool ShouldAttemptToUseA8Textures() const;

    void ReturnToSysMemBitsPool(_In_ SystemMemoryBits *pSysMemBits);

    _Check_return_ HRESULT AllocateFromSysMemBitsPool(
        UINT32 width,
        UINT32 height,
        DXGI_FORMAT pixelFormat,
        bool hasNative8BitSupport,
        _Outptr_ SystemMemoryBits **ppOut
        );

    ID3D11Texture2D *GetIntermediateUploadSurfaceNoRef(DXGI_FORMAT format) const;

    void NotifyLargestTextureInFrame(
        DXGI_FORMAT format,
        UINT32 uMaxWidth,
        UINT32 uMaxHeight
        );

    void RecordDeviceAsLost();
    bool IsDeviceLost() const;

    _Check_return_ HRESULT IsHdrOutput(_In_ HMONITOR monitor, _Out_ bool* isHDR) const;

    _Check_return_ HRESULT TakeLockAndCheckDeviceLost(_In_ CD3D11SharedDeviceGuard* guard);

    void TestHook_LoseDevice();

private:
    CD3D11Device();

    ~CD3D11Device() override;

private:
    std::shared_ptr<CD3D11DeviceInstance> m_deviceInstance;

    UINT                  m_uResetToken;

    static const UINT32 sc_stagingTextureReleaseFrameDelayMilliseconds = 1000;

    struct IntermediateTextureUploadParams
    {
        ID3D11Texture2D *pTexture;
        UINT32          width;
        UINT32          height;
        UINT64          lastUse;
    };

    // Intermediate texture for DXGI_FORMAT_B8G8R8A8_UNORM texture uploads.
    IntermediateTextureUploadParams m_UploadIntermediate;
};

_Check_return_ HRESULT
GetDeviceAdapterLuid(
    _In_ ID3D11Device *pDevice,
    _Out_ LUID *pLuid
    );
