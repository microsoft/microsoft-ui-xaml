// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LockableGraphicsPointer.h"
#include "PalThread.h"
#include "xcplist.h"
#include <d3d11.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIAdapter;
struct ID2D1Device;
struct ID2D1Multithread;
struct ID2D1DeviceContext;
struct ID2D1SolidColorBrush;
struct ID3D10Multithread;
struct IDXGIDevice2;
class SystemMemoryBits;
class CD2DFactory;
class CD3D11Device;
class CD3D11SharedDeviceGuard;
static const char D3D11DeviceName[] = "XAML Graphics Device";
static const UINT WARP_VENDOR_ID = 0x1414;
static const UINT WARP_DEVICE_ID = 0x8c;
/*

    The CD3D11Device class is an object that wraps objects that can be shared
    across multiple UI Threads (Core Windows).  See comment in CD3DDevice.h for
    more information.

*/

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {
    class D3D11DeviceInstanceUnitTests;
} } } } }

//
// DirectX device management is tricky:
//   1. We can have multiple UI threads all using the same DX objects.
//   2. We can receive device lost errors anytime with things like a video driver upgrade or a remote desktop connection.
//   3. When recovering from a device loss, we need to release all old DX objects before creating any new ones.
//
// Xaml internally caches the D3D device (and other DX resources) in a CD3D11DeviceInstance that's shared among all
// UI threads. Condition 3 above means when one thread tries to recover a device lost error, it needs to clear out
// the D3D device (and other DX resources) immediately. This creates potential AVs for other threads currently using
// the same D3D device. Checking for a lost device before accessing a resource is no good because of condition 2. The
// device can be lost between the check and the access, so that the check passes but the access still returns null
// because the D3D device has been cleaned out.
//
// So we have to lock around accessing the D3D device. It's not enough to just take a lock before checking and returning
// the D3D device pointer, though. We'd have to hold the lock as long as the thread is using the D3D device. If we release
// the lock while still holding the D3D device, then another UI thread can come in to do device lost recovery and fail
// because an old D3D device still lives.
//
// The rules for D3D device (and other DX resource) access become:
//   1. Take the lock and check for device lost. If the device is already lost then stop.
//   2. Access and use the D3D device.
//   3. Do not save the D3D device anywhere.
//   4. Release the lock once you have the D3D device*
//
// Rules 1 and 2 can be enforced by the API by having the caller pass in the lock, proving that they've taken it. Rules 3
// and 4 are conventions that must be maintained in the code.
//
// *Rule 4 should be "once you're completely done with the D3D device", but creates contention by extending the duration
// that someone needs to hold the lock. We'll try to avoid contention by letting callers release the lock once they have
// the device, provided they don't save the device anywhere. It's possible for the caller to be swapped out while still
// using the device, then for another thread to fail device lost recovery because the device can't be fully released. In
// that case, we'll wait for the caller's thread to be swapped back in and for the caller to release the device. Meanwhile
// the thread doing the recovery will encounter an error during recovery and will try again next tick.
//
// It's important to check for device lost in step 1. Once the device has been lost, threads must keep their hands off
// of the D3D device so that it can be fully released and device lost recovery can succeed. It also helps make code
// cleaner. Xaml often has code like pDeviceInstance->GetD3DDevice()->Call_D3D_Method() that gets the D3D device from
// the CD3D11DeviceInstance and directly makes calls to D3D. This code assumes that a D3D device is always available
// in the CD3D11DeviceInstance, which isn't true when we're still cleaning up from device lost. Making a device lost
// check after taking the lock ensures the caller that the D3D device will always be available inside the CD3D11DeviceInstance
// that they just locked, and lets us skip a lot of null checks.
//
// The matching rules for device lost recovery become:
//   1. Take the lock and confirm that the device is lost. If it's not lost then stop.
//   2. Clear out the D3D device and other DX resources.
//   3. Release the lock.
//   4. Create a new set of DX resources.
//
// Holding the lock between steps 1 and 3 mean that there are no current users of the D3D device that we're clearing out.
// Holding the lock also means users of the D3D device don't have to check for null once they own the lock, because the
// device can't be cleaned out by someone else without them getting the lock first.
//
// Another thing we want to prevent is multiple UI threads detecting device lost before any of them start recovering.
// Then they all try to recover and release and re-create the D3D device multiple times. This is covered by confirming
// that the device is lost in step 1. If it isn't, that means another thread has recovered the device before us, and we
// can no-op. This check is made in WindowsGraphicsDeviceManager::CleanupCachedDeviceResources and CD3D11DeviceInstance::
// RecordDeviceAsLost.
//
class CD3D11DeviceInstance
{
    friend ::Windows::UI::Xaml::Tests::Graphics::D3D11DeviceInstanceUnitTests;

public:
    static HRESULT GetInstance(_Out_ std::shared_ptr<CD3D11DeviceInstance> & sharedDevice, bool useUniqueDevice);

    CD3D11DeviceInstance(bool isSharedDevice);
    ~CD3D11DeviceInstance();

    _Check_return_ HRESULT EnsureResources();
    _Check_return_ HRESULT EnsureD2DResources();
    _Check_return_ HRESULT EnsureDeviceSpecificD2DResources(_In_ const CD3D11Device* deviceWrapper);
    _Check_return_ HRESULT EnsureD2DFactory();

    _Check_return_ HRESULT GetTextureMemoryUsage(
        _Out_ UINT32 *puTextureMemoryUsage,
        _Out_ UINT32 *puTextureMemoryUsageNPOT
        );

    //
    // CD3D11DeviceInstance methods
    //
    _Check_return_ HRESULT TakeLockAndCheckDeviceLost(_In_ CD3D11SharedDeviceGuard* guard);

    _Ret_notnull_ ID3D11Device *GetDevice(_In_ const CD3D11SharedDeviceGuard* guard) const;

    CLockedGraphicsPointer<ID3D11DeviceContext> GetLockedDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard) const;

    _Ret_maybenull_ IDXGIDevice2* GetDXGIDevice(_In_ const CD3D11SharedDeviceGuard* guard) const;

    _Ret_maybenull_ ID2D1Device *GetD2DDevice(_In_ const CD3D11SharedDeviceGuard* guard) const;

    _Ret_maybenull_ ID2D1DeviceContext* GetD2DDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard, _In_ const CD3D11Device* deviceWrapper) const;

    _Ret_maybenull_ ID2D1SolidColorBrush* GetD2DSolidColorBrush(_In_ const CD3D11SharedDeviceGuard* guard, _In_ const CD3D11Device* deviceWrapper) const;

    _Ret_maybenull_ CD2DFactory* GetD2DFactory() const;

    _Check_return_  HRESULT InitializeVideoDevice();

    void ReleaseResourcesOnDeviceLost();

    void ReturnToSysMemBitsPool(_In_ SystemMemoryBits *pSysMemBits);
    _Check_return_ HRESULT AllocateFromSysMemBitsPool(
        UINT32 width,
        UINT32 height,
        DXGI_FORMAT pixelFormat,
        bool hasNative8BitSupport,
        _Outptr_ SystemMemoryBits **ppOut
        );

    _Check_return_ HRESULT ReleaseScratchResources();

    _Check_return_ HRESULT IsHdrOutput(_In_ HMONITOR monitor, _Out_ bool* isHDR);
    bool ShouldAttemptToUseA8Textures() const { return !m_fIsWarpDevice || !m_fIsHardwareOutput; }
    bool UseIntermediateUploadSurface() const { return m_fUseIntermediateUploadSurface; }

    void TrimMemory(const UINT64 currentTime);

    void RecordDeviceAsLost();
    bool IsDeviceLost();
    static INT32 ReleaseD2DResources(_In_ UINT8 * pData);

    void TestHook_LoseDevice() { m_testHook_IsDeviceLost = true; }
    bool TestHook_IsDeviceLost() { return m_testHook_IsDeviceLost == true; }
    _Ret_maybenull_ ID3D11Device *TestHook_GetDevice() const;

private:
    void EnsureLockTaken(_In_ const CD3D11SharedDeviceGuard* guard) const;

    void ReleaseD2DResources(const long millisecondDelay);

    _Check_return_ HRESULT InitializeState();

    _Check_return_ HRESULT EnsureDXGIAdapters();
    _Check_return_ HRESULT CheckForStaleD3DDevice();
    _Check_return_ HRESULT CreateGraphicsDeviceFromAdapter(_In_ IDXGIAdapter1* adapter, bool isWarpAdapter, bool isVideo, bool isDebugDevice);

    CLockableGraphicsPointer<ID3D11DeviceContext> GetLockableDeviceContext()
    {
        ASSERT(m_pDeviceContext != NULL);
        CLockableGraphicsPointer<ID3D11DeviceContext> pointerWithLock(m_pDeviceContext, m_pD3DMultithread, m_pD2DMultithread);
        return pointerWithLock;
    }

    bool IsWarpDevice() { return m_fIsWarpDevice; }
    D3D_FEATURE_LEVEL GetFeatureLevel() { return m_featureLevel; }
    bool IsOnSameAdapter(_In_ const LUID *pOtherLuid);

    static std::shared_ptr<CD3D11DeviceInstance> GetSharedDeviceInstance();

private:
    // The singleton shared CD3D11DeviceInstance and the lock protecting it
    // Callers must not take any other locks while holding this one.
    static std::weak_ptr<CD3D11DeviceInstance> s_sharedDeviceWeak;
    static wil::critical_section s_sharedInstanceCriticalSection;

    // Multiple threads can access, and potentially create, the primary adapter.
    wil::critical_section m_lock;

    _Maybenull_impl_ ID3D11Device *m_pDevice;
    _Maybenull_impl_ ID3D11DeviceContext *m_pDeviceContext;
    _Maybenull_impl_ CD2DFactory *m_pD2DFactory;
    _Maybenull_impl_ ID2D1Device * m_pD2DDevice;
    _Maybenull_impl_ ID2D1Multithread* m_pD2DMultithread;
    _Maybenull_impl_ ID3D10Multithread* m_pD3DMultithread;

    // These are separated per CD3D11Device (i.e. one per UI thread), as opposed to one shared among all threads.
    // This is so multiple UI threads won't interfere with each other if they do something like render text at
    // the same time.
    std::map<const CD3D11Device*, wrl::ComPtr<ID2D1DeviceContext>> m_d2dDeviceContexts;
    std::map<const CD3D11Device*, wrl::ComPtr<ID2D1SolidColorBrush>> m_d2dSolidColorBrushes;

    D3D_FEATURE_LEVEL m_featureLevel;

    IDXGIDevice2* m_pDXGIDevice2;

    bool m_fIsWarpDevice : 1;
    bool m_fIsHardwareOutput : 1;
    bool m_fUseIntermediateUploadSurface : 1;
    bool m_isSharedDevice : 1;

    // Written on the render thread, read on the UI thread
    xinit_cross_thread<LUID> m_adapterLuid;

    IntrusiveList<SystemMemoryBits> m_sysMemBitsPool;

    // Fields protected by the m_lock
    Microsoft::WRL::ComPtr<IDXGIFactory1> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_hardwareAdapter;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_warpAdapter;

    bool m_wasLostDeviceReleased {false};
    bool m_testHook_IsDeviceLost {false};
};
