// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "d3d11device.h"
#include "D2DAccelerated.h"
#include <GraphicsUtility.h>
#include <DCompTreeHost.h>
#include <RuntimeEnabledFeatures.h>

//------------------------------------------------------------------------------
//
//  Synopsis:
//      WindowsGraphicsDeviceManager ctor
//
//------------------------------------------------------------------------------
WindowsGraphicsDeviceManager::WindowsGraphicsDeviceManager()
    : m_hrInitializeWorkItem(S_OK)
    , m_pDCompTreeHost(NULL)
    , m_pRenderTargetNoRef(NULL)
    , m_pCompositorSchedulerNoRef(NULL)
    , m_isInitialized(false)
{
    XCP_WEAK(&m_pCompositorSchedulerNoRef);
    XCP_WEAK(&m_pRenderTargetNoRef);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      WindowsGraphicsDeviceManager dtor
//
//------------------------------------------------------------------------------
WindowsGraphicsDeviceManager::~WindowsGraphicsDeviceManager()
{
    IGNOREHR(WaitForInitializationThreadCompletion());

    ReleaseInterface(m_pDCompTreeHost);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      WindowsGraphicsDeviceManager static factory method
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT WindowsGraphicsDeviceManager::Create(_Outptr_ WindowsGraphicsDeviceManager **ppGraphicsDeviceManager)
{
    HRESULT hr = S_OK;

    WindowsGraphicsDeviceManager *pGraphicsDeviceManager =  new WindowsGraphicsDeviceManager();


    IFC(pGraphicsDeviceManager->Initialize());

    *ppGraphicsDeviceManager = pGraphicsDeviceManager;
    pGraphicsDeviceManager = NULL; // pass ownership of the object to the caller

Cleanup:
    ReleaseInterfaceNoNULL(pGraphicsDeviceManager);

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Basic initialization
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
WindowsGraphicsDeviceManager::Initialize()
{
    HRESULT hr = S_OK;

    IFC(DCompTreeHost::Create(this, &m_pDCompTreeHost));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//     Get graphics devices for media playback. this will be used by all the
//     media elements in the visual tree.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT WindowsGraphicsDeviceManager::GetGraphicsDeviceForVideo(_Outptr_result_maybenull_ CD3D11Device **ppVideoGraphicsDevice)
{
    HRESULT hr = S_OK;

    bool allowVideoSpecificDevice = false;

    if (m_cachedD3DDevice != nullptr)
    {
        allowVideoSpecificDevice = TRUE;
    }

    if (allowVideoSpecificDevice)
    {
        if (m_videoGraphicsDevice == nullptr)
        {
            xref_ptr<CD3D11Device> videoDevice;
            IFC(CD3D11Device::Create(videoDevice.ReleaseAndGetAddressOf(), true /* useUniqueDevice */));
            IFC(videoDevice->InitializeVideoDevice());
            m_videoGraphicsDevice = videoDevice;
        }

        SetInterface(*ppVideoGraphicsDevice, m_videoGraphicsDevice.get());
    }
    else
    {
        *ppVideoGraphicsDevice = NULL;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the cached device after it's been lost.
//
//------------------------------------------------------------------------------
void WindowsGraphicsDeviceManager::CleanupCachedDeviceResources(_In_ bool cleanupDComp, _In_ bool isDeviceLost)
{
    IGNOREHR(WaitForInitializationThreadCompletion());

    //
    // If we have a shared static CD3D11DeviceInstance wrapper, then check it and clear out the D3D device inside.
    // We may find that it's no longer the shared static CD3D11DeviceInstance, because some other thread has come
    // along and reset the shared instance. In that case just release the CD3D11Device that we already have. When we
    // go through CD3D11Device::Create during recovery, we'll pick up the existing shared static CD3D11DeviceInstance.
    //
    if (isDeviceLost && m_cachedD3DDevice)
    {
        m_cachedD3DDevice->RecordDeviceAsLost();
    }

    m_cachedD3DDevice.reset();
    if (cleanupDComp)
    {
        m_pDCompTreeHost->ReleaseResources(true /* shouldDeferClosingInteropCompostior */);
    }
    else
    {
        m_pDCompTreeHost->ReleaseGraphicsResources();
    }
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Start off-thread resource creation.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT WindowsGraphicsDeviceManager::StartResourceCreation()
{
    // Only start device creation once.
    if (m_cachedD3DDevice == nullptr)
    {
        // Create the device wrapper here to signify that device creation has started.
        IFC_RETURN(CD3D11Device::Create(m_cachedD3DDevice.ReleaseAndGetAddressOf(), false /* useUniqueDevice */));

        ASSERT(m_initializationThreadHandle == NULL);

        m_hrInitializeWorkItem = S_OK;

        {
            // Take the critical section lock to synchronize the assignment of m_initializationThreadHandle.
            // If the thread finishes extremely quickly (can happen on VMs), it can finish before CreateThread returns,
            // which would cause m_initializationThreadHandle to be incorrectly overwritten.
            auto guard = m_InitializeWorkItemLock.lock();

            // We don't use a standard task here because that uses the thread pool which in turn may have tasks that are
            // dependent upon our initialization completing.  If there are enough of those we could get into a deadlock
            // situation where initialization is waiting on a threadpool thread to become available, while no threads
            // come available because they are waiting on initialization to finish.
            IFCHNDL_RETURN(m_initializationThreadHandle = CreateThread(nullptr /*threadattributes*/, 0 /*stacksize*/, InitializeCallback, reinterpret_cast<void*>(this), 0 /*creationflags*/, nullptr /*threadid*/));
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// Static entry point for initialize thread.
// Matches the signature of PALWORKITEMCALLBACK required by IPALWorkItems.
//
//------------------------------------------------------------------------
/* static */ DWORD WINAPI
WindowsGraphicsDeviceManager::InitializeCallback(
    _In_opt_ void *pData
    )
{
    HRESULT hr = S_OK;

    WindowsGraphicsDeviceManager *pThis = static_cast<WindowsGraphicsDeviceManager*>(pData);
    ASSERT(pThis != NULL);

    // On success or failure, clean-up the work item.
    auto workItemCleanupGuard = wil::scope_exit([&]
    {
        auto guard = pThis->m_InitializeWorkItemLock.lock();
        CloseHandle(pThis->m_initializationThreadHandle);
        pThis->m_initializationThreadHandle = NULL;
        pThis->m_hrInitializeWorkItem = hr;
        pThis->m_isInitialized = true;
    });

    // Hard fault if device initialization fails for any reason other than device lost.
    IFC_DEVICE_LOST_OTHERWISE_FAIL_FAST(pThis->InitializeExpensiveResources());

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// Synopsis:
//     Does the expensive initialization work for the wrapper objects.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WindowsGraphicsDeviceManager::InitializeExpensiveResources()
{
    HRESULT hr = S_OK;

    ASSERT(m_cachedD3DDevice != nullptr);
    IFC(m_cachedD3DDevice->EnsureResources());

    IFC(EnsureD2DResources());
    IFC(DrawDummyText());

    // Do not create the DComp device/compositor here. There are operations like animation completed callbacks that
    // need a message pump on the thread that creates the compositor, and this method is called from a worker thread.
    // We'll create the DComp device/compositor on the UI thread instead.

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
// Synopsis:
//     Does the work to create the D2D device and related resources.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WindowsGraphicsDeviceManager::EnsureD2DResources()
{
    return m_cachedD3DDevice->EnsureD2DResources();
}

_Check_return_ HRESULT
WindowsGraphicsDeviceManager::DrawDummyText()
{
    // This function front-loads the work of loading shaders used for drawing text to the background thread
    // by drawing a small amount of dummy text.  This saves ~70 ms of startup cost on xbox.
    D3D11_TEXTURE2D_DESC desc =
    {
        16,                             // Width
        16,                             // Height
        1,                              // MipLevels
        1,                              // ArraySize
        DXGI_FORMAT_B8G8R8A8_UNORM,     // Format
        { 1, 0 },                       // SampleDesc
        D3D11_USAGE_DEFAULT,            // Usage
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,       // BindFlags
        0,                              // CPUAccessFlags
        0                               // MiscFlags
    };

    // Create a scratch texture and device context to draw the text
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
    {
        CD3D11SharedDeviceGuard guard;
        IFC_RETURN(m_cachedD3DDevice->TakeLockAndCheckDeviceLost(&guard));
        IFC_RETURN(m_cachedD3DDevice->GetDevice(&guard)->CreateTexture2D(&desc, nullptr, &texture2D));
    }
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
    VERIFYHR(texture2D.As(&dxgiSurface));

    Microsoft::WRL::ComPtr<ID2D1DeviceContext> deviceContext;
    {
        CD3D11SharedDeviceGuard guard;
        IFC_RETURN(m_cachedD3DDevice->TakeLockAndCheckDeviceLost(&guard));
        IFC_RETURN(m_cachedD3DDevice->GetD2DDevice(&guard)->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
            &deviceContext));
    }

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    IFC_RETURN(deviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), nullptr, &bitmap));
    deviceContext->SetTarget(bitmap.Get());

    // Create a DWrite text format
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory;
    IFC_RETURN(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &dwriteFactory));

    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat1;
    IFC_RETURN(dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        20, // fontSize
        L"", //locale
        &textFormat1
        ));

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
    IFC_RETURN(deviceContext->CreateSolidColorBrush(
                    D2D1::ColorF(D2D1::ColorF::White),
                    &whiteBrush
                    ));

    deviceContext->BeginDraw();

    // First push a clip and clear, this causes D2D to load 2 of the shaders we always see loaded as it does the clear.
    deviceContext->PushAxisAlignedClip(
        D2D1::RectF(4, 4, 10, 10),
        D2D1_ANTIALIAS_MODE_ALIASED
        );

    deviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    D2D1_RECT_F rect = D2D1::RectF(0, 0, 16, 16);
    const WCHAR* text = L"A";

    // Draw the text
    deviceContext->DrawText(
        text,
        1,
        textFormat1.Get(),
        &rect,
        whiteBrush.Get()
        );

    deviceContext->PopAxisAlignedClip();

    IFC_RETURN(deviceContext->EndDraw());

    return S_OK;
}

_Check_return_ bool WindowsGraphicsDeviceManager::CanContinueOnInternalDriverError()
{
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    // Normally an internal driver error is a one off problem and so we can try to recover
    // it through our usual device lost code.  However, it is possible that the driver could
    // just be bad, or there is something in the hardware that causes it to continually fail.
    // In those cases, we want to try to fall back to the WARP driver and just use it.  If, for
    // some reason the WARP driver produces similar errors, there isn't much we can do, so we won't
    // be able to continue;
    ULONGLONG currentTickCount = ::GetTickCount64();
    if (currentTickCount > m_dxgiLastInternalErrorTickCount + (10 * 1000) /*ten seconds*/)
    {
        // It has been over ten seconds since our last internal error, so reset our counter.
        // This may seem like a short time frame, but really the scenario we are trying to
        // catch is where we get the error, reopen the device and the get the error on the
        // next frame.  Even if the device ends up resetting every 10+ seconds, it will
        // still be somewhat usable.
        m_dxgiInternalErrorCount = 0;
    }

    // See if we have had more than five consecutive errors
    if (++m_dxgiInternalErrorCount > 5)
    {
        if (m_dxgiUseWarpOnly)
        {
            // we have already forced warp, so there isn't anything we can do
            return false;
        }

        // Force the application to use WARP only.
        m_dxgiUseWarpOnly = true;
        m_dxgiInternalErrorCount = 0;
        runtimeEnabledFeatureDetector->SetFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceWarp, true);
    }
    m_dxgiLastInternalErrorTickCount = currentTickCount;

    return true;
}
#if DBG
//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a helper object which can be used to assert
//      actual deletion of devices.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT WindowsGraphicsDeviceManager::CreateFinalReleaseAsserter(
    _Outptr_ IPALDebugDeviceFinalReleaseAsserter **ppAsserter)
{
    HRESULT hr = S_OK;
    DebugDeviceFinalReleaseAsserter *pAsserter = NULL;
    ID3D11Device *pD3D11DeviceNoRef = NULL;
    ID2D1Device *pD2DDeviceNoRef = NULL;
    IDCompositionDesktopDevicePartner *pDCompDeviceNoRef = NULL;
    bool hasInteropCompositor = false;

    // We're breaking our own rules here about not holding on to the D3D device. We'll allow it here because
    // this is debug code during device lost recovery. The FinalReleaseAsserter will take a ref on the D3D
    // device and keep it alive, so we're not afraid of AVs. The final release asserter also releases its
    // refs before the end of cleanup, so it won't interfere with creating a new D3D device.
    CD3D11SharedDeviceGuard guard;
    if (m_cachedD3DDevice != NULL)
    {
        IFC_RETURN(m_cachedD3DDevice->TakeLockAndCheckDeviceLost(&guard))
        pD3D11DeviceNoRef = m_cachedD3DDevice->GetDevice(&guard);
        pD2DDeviceNoRef = m_cachedD3DDevice->GetD2DDevice(&guard);
    }

    if (m_pDCompTreeHost != NULL)
    {
        pDCompDeviceNoRef = m_pDCompTreeHost->GetMainDevice();
        hasInteropCompositor = m_pDCompTreeHost->HasInteropCompositorPartner();
    }

    pAsserter = new DebugDeviceFinalReleaseAsserter(
                                hasInteropCompositor,
                                pD3D11DeviceNoRef,
                                pD2DDeviceNoRef,
                                pDCompDeviceNoRef);
    *ppAsserter = pAsserter;

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Ctor.
//
//------------------------------------------------------------------------------
DebugDeviceFinalReleaseAsserter::DebugDeviceFinalReleaseAsserter(
    bool hasInteropCompositor,
    _In_opt_ ID3D11Device *pD3D11Device,
    _In_opt_ ID2D1Device *pD2DDevice,
    _In_opt_ IDCompositionDesktopDevicePartner *pDCompDevice)
{
    SetInterface(m_pD3D11Device, pD3D11Device);
    SetInterface(m_pD2DDevice, pD2DDevice);
    SetInterface(m_pDCompDevice, pDCompDevice);
    m_hasInteropCompositor = hasInteropCompositor;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Dtor.
//
//------------------------------------------------------------------------------
DebugDeviceFinalReleaseAsserter::~DebugDeviceFinalReleaseAsserter()
{
    ReleaseInterface(m_pDCompDevice);
    ReleaseInterface(m_pD2DDevice);
    ReleaseInterface(m_pD3D11Device);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Releases all the devices while asserting that this
//      release is the final realease for each device.
//
//------------------------------------------------------------------------------
void
DebugDeviceFinalReleaseAsserter::ReleaseAllWithAssert()
{
    // These have to be in this order for asserts to succeed.
    if (m_pDCompDevice != NULL)
    {
        // InteropCompositor's release was deferred so that we can run this asserter
        // before InteropCompositor::RealClose is called. That should be the single
        // outstanding reference on the device. Use LOG_LEAK_EX for now because
        // crashing here just causes tests to be unreliable in CatGates and there isn't
        // good reporting to know which test is considered unreliable.
#if 0
        ULONG remainingRefCount =
#endif
        m_pDCompDevice->Release();
        // When running under component UI, we hand off a visual to the CoreApplicationView, which takes
        // references to the device and don't give them up until a new visual is handed off.  Since there doesn't
        // seem to be anyway to "null" this out, for now we will  simply not check the DComp device

#if 0   // There are a bunch of references still holding onto the interop compositor.  See bug: Bug 23782596 : Renable or remove DebugDeviceFinalReleaseAsserter
        if (m_hasInteropCompositor && remainingRefCount != 1)
        {
            LOG_LEAK_EX(L"DebugDeviceFinalReleaseAsserter::ReleaseAllWithAssert: expected remainingRefCount = 1, actual = %d", remainingRefCount);
        }
        else if (!m_hasInteropCompositor && remainingRefCount != 0)
        {
            LOG_LEAK_EX(L"DebugDeviceFinalReleaseAsserter::ReleaseAllWithAssert: expected remainingRefCount = 0, actual = %d", remainingRefCount);
        }
#endif
        m_pDCompDevice = NULL;
    }

    if (m_pD2DDevice != NULL)
    {
        // Note: We can't assert the ref count here because if we have a shared device, there could be some other
        // thread that still holds a reference.
        m_pD2DDevice->Release();
        m_pD2DDevice = NULL;
    }

    if (m_pD3D11Device != NULL)
    {
        //
        // Note: We can't assert the ref count here. We could have passed our device to the app via CreateSurfacePresenter.
        // The app can then create surfaces on the device, which will hold references. This won't prevent us from recovering,
        // but it's up to the app to throw away any surfaces that it created.
        //
        m_pD3D11Device->Release();
        m_pD3D11Device = NULL;
    }
}
#endif

//----------------------------------------------------------------------------
//
// Passes the atlas size hint to the DCompTreeHost managed by this object.
//
//----------------------------------------------------------------------------
void WindowsGraphicsDeviceManager::SetAtlasSizeHint(XUINT32 width, XUINT32 height)
{
    ASSERT(m_pDCompTreeHost);
    m_pDCompTreeHost->SetAtlasSizeHint(width, height);
}

void WindowsGraphicsDeviceManager::ResetAtlasSizeHint()
{
    ASSERT(m_pDCompTreeHost);
    m_pDCompTreeHost->ResetAtlasSizeHint();
}


//----------------------------------------------------------------------------
//
// Passes the intent to the DCompTreeHost managed by this object.
//
//----------------------------------------------------------------------------
void WindowsGraphicsDeviceManager::DisableAtlas()
{
    ASSERT(m_pDCompTreeHost);
    m_pDCompTreeHost->DisableAtlas();
}

CD3D11Device* WindowsGraphicsDeviceManager::GetVideoGraphicsDeviceNoRef() const
{
    return m_videoGraphicsDevice.get();
}

void WindowsGraphicsDeviceManager::SetCompositorScheduler(_In_ CompositorScheduler* compositorScheduler)
{
    m_pCompositorSchedulerNoRef = compositorScheduler;
}
