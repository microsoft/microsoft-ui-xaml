// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "SwapChainElement.h"
#include "DCompSurface.h"
#include "DCompSurfaceFactoryManager.h"
#include "WindowsGraphicsDeviceManager.h"
#include "D3D11SharedDeviceGuard.h"
#include "D3D11Device.h"


_Check_return_ HRESULT DCompSurface::Create(
    _In_ CD3D11Device *pDevice,
    _In_ DCompTreeHost *pDCompHost,
    bool isOpaque,
    bool fAlphaMask,
    bool isVirtual,
    bool isHDR,
    bool requestAtlas,
    XUINT32 widthWithoutGutters,
    XUINT32 heightWithoutGutters,
    _Outptr_ DCompSurface** ppTexture
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DCompSurface::Create(
    _In_ DCompTreeHost *pDCompHost,
    _In_ IDCompositionSurfaceFactory *pSurfaceFactory,
    bool isOpaque,
    bool isVirtual,
    bool isHDR,
    bool requestAtlas,
    XUINT32 widthWithoutGutters,
    XUINT32 heightWithoutGutters,
    _Outptr_ DCompSurface** ppTexture
    )
{
    return E_NOTIMPL;
}

/* static */ xref_ptr<DCompSurface>
DCompSurface::CreateWithNoHardware(
    _In_ DCompTreeHost *dcompTreeHost,
    bool isVirtual
    )
{
    xref_ptr<DCompSurface> nullSurface;

    return nullSurface;
}

_Check_return_ HRESULT DCompSurface::InitializeSurface(
    _In_ CD3D11Device *pDevice,
    _In_ DCompTreeHost *pDCompTreeHost,
    bool isVirtual
    )
{
    return E_NOTIMPL;
}

bool DCompSurface::IsVirtual()
{
    return false;
}

_Check_return_ HRESULT DCompSurface::BeginDraw(
    _In_ const XRECT *pUpdateRect,
    _In_ REFIID iid,
    _Outptr_ IUnknown **ppSurface,
    _Out_ XPOINT *pOffsetIntoSurface
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DCompSurface::EndDraw()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DCompSurface::Resize(
    XUINT32 width,
    XUINT32 height
)
{
    return E_NOTIMPL;
}

DCompSurfaceFactoryManager* DCompSurfaceFactoryManager::Instance()
{
    return nullptr;
}

_Check_return_ HRESULT DCompSurfaceFactoryManager::ObtainSurfaceFactory(
    _In_ DCompTreeHost *pDCompTreeHost,
    _In_ IUnknown *pIUnk,
    _In_ IDCompositionDesktopDevice*pDCompDevice,
    _Outptr_ DCompSurfaceFactory **ppSurfaceFactory
    )
{
    return E_NOTIMPL;
}

void DCompSurfaceFactoryManager::GetSurfaceFactoriesForCurrentThread(
    _Inout_ std::vector<IDCompositionSurfaceFactory*>* surfaceFactoryVector
)
{
    return;
}

_Check_return_ HRESULT DCompSurfaceFactoryManager::OnSurfaceFactoryDestroyed(
    _In_ DCompSurfaceFactory* pSurfaceFactory
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT DCompSurfaceFactoryManager::CleanupSurfaceFactoryMapForCurrentThread()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CD3D11Device::TakeLockAndCheckDeviceLost(_In_ CD3D11SharedDeviceGuard* guard)
{
    return E_NOTIMPL;
}

_Ret_notnull_ ID3D11Device * CD3D11Device::GetDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    #pragma warning(suppress: 6387) // test function, return nullptr
    return nullptr;
}

ID3D11Device * CD3D11Device::TestHook_GetDevice() const
{
    return nullptr;
}

ID2D1Device * CD3D11Device::GetD2DDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    return nullptr;
}

ID2D1DeviceContext * CD3D11Device::GetD2DDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    return nullptr;
}

CD2DFactory * CD3D11Device::GetD2DFactory() const
{
    return nullptr;
}

_Check_return_ HRESULT CD3D11Device::EnsureD2DResources()
{
    return E_NOTIMPL;
}

bool CD3D11Device::ShouldAttemptToUseA8Textures() const
{
    return false;
}

bool CD3D11Device::IsDeviceLost() const
{
    return false;
}

void CD3D11Device::TestHook_LoseDevice()
{
    return;
}

_Check_return_ HRESULT WindowsGraphicsDeviceManager::EnsureD2DResources()
{
    return E_NOTIMPL;
}

EncodedPtr<IPlatformServices> gps; // will stay null
