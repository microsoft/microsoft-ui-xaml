// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include <NamespaceAliases.h>
#include <uielement.h>
#include <CompositionRequirement.h>
#include <RenderTargetElement.h>
#include <rendertargetbitmapmgr.h>
#include <palthread.h>
#include <hwcompnode.h>
#include <hwwalk.h>
#include <hwtexturemgr.h>
#include <dcompsurface.h>
#include <dcomptreehost.h>
#include <compositortree.h>
#include <GraphicsUtility.h>
#include <WicService.h>
#include <PixelFormat.h>
#include <windows.foundation.collections.h>
#include <windows.graphics.directx.h>
#include <transforms.h>
#include <winpal.h>
#include "RenderTargetBitmapImplUsingSpriteVisuals.h"
#include "LockableGraphicsPointer.h"
#include "D3D11.h"
#include "D3D11Device.h"
#include "D3D11SharedDeviceGuard.h"
#include <microsoft.ui.composition.internal.h>
#include <microsoft.ui.composition.internal.interop.h>
#include <microsoft.ui.composition.private.interop.h>

class CaptureAsyncCompletionState
{
public:

    CaptureAsyncCompletionState(
        _In_ xref_ptr<HWTexture> hwTexture,
        _In_ IPALEvent* completionEvent,
        HRESULT asyncErrorCode
        )
        : m_hwTexture(std::move(hwTexture))
    {
        // RS4 Bug #15505548:
        // IPALEvent does not support ref-counting, and the "real" owner of the event, RenderTargetBitmapWaitExecutor, can delete
        // the object before we access it in the destructor, specifically in a device-lost situation where the timing is not "normal".
        // Tactical fix: In order to avoid crashing accessing a deleted object, we duplicate the underlying event handle and use this instead.
        // TFS task #15568860 is tracking converting the usage of IPALEvent over to a safe pattern.
        CWinEvent* event = static_cast<CWinEvent*>(completionEvent);
        IFCW32FAILFAST(DuplicateHandle(GetCurrentProcess(), event->GetHandle(), GetCurrentProcess(), &m_completionEvent, 0 /*desiredaccess*/, FALSE/*inherithandle*/, DUPLICATE_SAME_ACCESS));
    }

    ~CaptureAsyncCompletionState()
    {
        // Signal the completion event to guarantee any outstanding RenderTargetBitmapWaitExecutor background task completes.
        ::SetEvent(m_completionEvent.get());
    }

    _Ret_notnull_ HWTexture* GetHwTexture() { return m_hwTexture.get(); }
    _Ret_notnull_ HANDLE GetCompletionEvent() { return m_completionEvent.get(); }

    void SetAsyncErrorCode(HRESULT hr) { m_asyncErrorCode = hr; }
    HRESULT GetAsyncErrorCode() { return m_asyncErrorCode; }

private:
    xref_ptr<HWTexture> m_hwTexture;
    wil::unique_handle m_completionEvent;
    HRESULT m_asyncErrorCode{};
};

RenderTargetBitmapImplUsingSpriteVisuals::RenderTargetBitmapImplUsingSpriteVisuals(
    _In_ CRenderTargetBitmapManager& rtbManager
    )
    : m_rtbManager(rtbManager)
{}

RenderTargetBitmapImplUsingSpriteVisuals::~RenderTargetBitmapImplUsingSpriteVisuals()
{
    Cleanup();
}

_Check_return_ HRESULT
RenderTargetBitmapImplUsingSpriteVisuals::RequestRender(
    _In_ CUIElement* uiElement,
    _In_ int32_t scaledWidth,
    _In_ int32_t scaledHeight,
    _In_ ICoreAsyncAction* asyncAction)
{
    IFC_RETURN(__super::RequestRender(uiElement, scaledWidth, scaledHeight, asyncAction));

    m_uiElement = uiElement;

    if (!m_uiElement->IsRenderTargetSource())
    {
        // Request a dedicated composition node for the render target UIElement
        IFCFAILFAST(m_uiElement->SetRequiresComposition(CompositionRequirement::RenderTargetBitmap, IndependentAnimationType::None));
    }

    return S_OK;
}

_Check_return_ HRESULT
RenderTargetBitmapImplUsingSpriteVisuals::PreCommit(
    _In_ DCompTreeHost* dcompTreeHost,
    _In_ HWTexture* hwTexture,
    _In_opt_ IPALEvent* completionEvent)
{
    IFC_RETURN(__super::PreCommit(dcompTreeHost, hwTexture, completionEvent));

    // Create the property visual to use for scaling the content independent of live tree manipulation.
    auto compositor = dcompTreeHost->GetCompositor();
    auto compositionDevice = dcompTreeHost->GetCompositionGraphicsDevice();

    HWCompTreeNode* elementCompNodeNoRef = m_uiElement->GetCompositionPeer();
    if (elementCompNodeNoRef == nullptr)
    {
        // The element is being rendered with a LayoutTransitionElement, and doesn't have a comp node. Instead, get the comp node
        // from the LTE itself. The LTE will contain all the SpriteVisual content in the element's subtree.
        ASSERT(m_uiElement->IsHiddenForLayoutTransition());
        elementCompNodeNoRef = m_uiElement->GetFirstLTETargetingThis()->GetCompositionPeer();
    }

    // Hand-off visual should be assured during PreCommit stage assuming the CompositionRequirement was set during RequestRender.
    // The Hand-off visual is actually the primary visual that we want to capture.
    auto primaryCompositionVisual2 = elementCompNodeNoRef->GetHandOffVisual();
    ASSERT(primaryCompositionVisual2 != nullptr);

    ctl::ComPtr<WUComp::Internal::IVisualInternal> primaryVisualInternal;
    IFCFAILFAST(ctl::do_query_interface(primaryVisualInternal, primaryCompositionVisual2));

    // Get the properties on the primary visual so they can be inverted for the property visual.
    ctl::ComPtr<WUComp::IVisual> primaryVisual;
    IFCFAILFAST(ctl::do_query_interface(primaryVisual, primaryCompositionVisual2));

    auto transformAndClipStack = m_renderTargetElementData->GetPrependTransformAndClip();

    CMILMatrix4x4 combinedTransform;
    transformAndClipStack->GetCombinedTransform(&combinedTransform);

    // Create the property visual here and use the calculated transform to adjust the
    // visual for scaling quality and so it is oriented at 0,0 in the texture.
    ctl::ComPtr<WUComp::IContainerVisual> propertyContainerVisual;
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(compositor->CreateContainerVisual(propertyContainerVisual.ReleaseAndGetAddressOf()));

    ctl::ComPtr<WUComp::IVisual> propertyVisual;
    IFCFAILFAST(propertyContainerVisual.As(&propertyVisual));

    wfn::Matrix4x4 wfnParentTransform;
    combinedTransform.ToMatrix4x4(&wfnParentTransform);
    IFCFAILFAST(propertyVisual->put_TransformMatrix(wfnParentTransform));

    // Execute CaptureAsync which won't actually run until Commit is executed on the main device.
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(primaryVisualInternal->CaptureAsync(
        propertyVisual.Get(),
        compositionDevice,
        m_renderTargetElementData->GetPixelWidth(),
        m_renderTargetElementData->GetPixelHeight(),
        ixp::DirectXPixelFormat_B8G8R8A8UIntNormalized,
        ixp::DirectXAlphaMode_Premultiplied,
        &m_captureAsyncOperation));

    // completionEvent is guaranteed to survive the lifetime since it has the lifetime of the entire XAML app
    // since it is managed by the CRenderTargetBitmapManager.

    // Construct a shared state object which the completion event can use to hold strong references to objects
    // that must survive until the completion event is finished and can be used to signal completion and return errors.

    m_captureAsyncCompletionState = std::make_shared<CaptureAsyncCompletionState>(
        xref_ptr<HWTexture>(hwTexture),
        completionEvent,
        E_PENDING);

    // Run an asynchronous operation to copy/scale the composition surface and complete the event
    // m_uiElement is unsafe to use on a background thread since it can be modified by the rest of XAML or the App on the UI thread.
    // The state object methods are safe to use on a background thread because launching the asynchronous operation is the last thing to happen
    // in this method and signaling the completion event will cause PostDraw to run on the UI thread afterward.
    // Therefore, no state modifications should happen after triggering the async operation or after signaling the completion event.
    using IVisualCaptureCompletedHandler = wf::IAsyncOperationCompletedHandler<WUComp::ICompositionSurface*>;
    auto captureAsyncCallback = wrl::Callback<
        wrl::Implements<wrl::RuntimeClassFlags<wrl::ClassicCom>, IVisualCaptureCompletedHandler, wrl::FtmBase>>(
        [captureAsyncCompletionState = m_captureAsyncCompletionState, graphicsDevice = dcompTreeHost->GetGraphicsDevice()]
        (_In_ CompositionSurfaceAsyncOperation* sender, wf::AsyncStatus) -> HRESULT
        {
            // When the capture is complete, get the returned surface
            ctl::ComPtr<WUComp::ICompositionSurface> capturedSurface;
            IFCFAILFAST(sender->GetResults(&capturedSurface));

            captureAsyncCompletionState->SetAsyncErrorCode(
                ProcessCaptureThreaded(
                    capturedSurface.Get(),
                    captureAsyncCompletionState->GetHwTexture(),
                    graphicsDevice));

            ::SetEvent(captureAsyncCompletionState->GetCompletionEvent());

            return S_OK;
        });

    m_captureAsyncOperation->put_Completed(captureAsyncCallback.Get());

    return S_OK;
}

_Check_return_ HRESULT
RenderTargetBitmapImplUsingSpriteVisuals::PostDraw()
{
    // m_captureAsyncCompletionState may be null if there was no capture due to a zero size element in which case
    // we should return S_OK if it got this far. It should also return S_OK in the unsupported capture scenario.
    HRESULT hr =
        (m_captureAsyncCompletionState == nullptr)
        ? S_OK
        : m_captureAsyncCompletionState->GetAsyncErrorCode();

    IFC_RETURN(CompleteOperation(hr));

    Cleanup();

    // Return the asynchronous error code to handle device lost scenarios
    return hr;
}

_Check_return_ HRESULT
RenderTargetBitmapImplUsingSpriteVisuals::FailRender(HRESULT failureHR)
{
    IFC_RETURN(__super::FailRender(failureHR));

    Cleanup();

    return S_OK;
}


void RenderTargetBitmapImplUsingSpriteVisuals::CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp)
{
    __super::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    m_captureAsyncOperation.Reset();
    m_captureAsyncCompletionState.reset();
}

void
RenderTargetBitmapImplUsingSpriteVisuals::Cleanup()
{
    if (m_uiElement != nullptr)
    {
        // If m_renderTargetElementData is available, then RequestRender was called and the element should have requested a composition node
        // already because it is using the SpriteVisuals code path.
        ASSERT(m_uiElement->IsRenderTargetSource());

        if (m_rtbManager.CountElementJobs(m_uiElement.get()) == 0)
        {
            m_uiElement->UnsetRequiresComposition(CompositionRequirement::RenderTargetBitmap, IndependentAnimationType::None);
        }

        m_uiElement.reset();
    }

    m_captureAsyncCompletionState.reset();
    m_captureAsyncOperation.Reset();
}

// ProcessCaptureThreaded method was designed to run on another thread and should
// only touch local non-pointer member variables.  completionEvent will prevent
// other methods from running until the event is signaled.
_Check_return_ HRESULT
RenderTargetBitmapImplUsingSpriteVisuals::ProcessCaptureThreaded(
    _In_ WUComp::ICompositionSurface* inputCompSurface,
    _In_ HWTexture* outputHwTexture,
    _In_ CD3D11Device* graphicsDevice)
{
    // Task 25523452: Stop calling IVisualInternal, ICompositionDrawingSurfaceInteropInternalDebug, and ICompositionDrawingSurfaceInteropPrivate - RTB doesn't work this way in lifted

    // Asynchronous code for handling CaptureAsync.  Everything used should be thread-safe.
    // Note that RenderTargetBitmap does not support source or destination content larger than max texture size
    // thus this algorithm assumes no tiling needs to be considered.  This could be added in the future by considering
    // the rectangles for CopySurface and LockRect.

    // TODO: It may be more efficient and possible to just use a CopySubResourceRegion to blt between DXGI surfaces.
    //                 Alternatively swapping the ICompositionSurface pointer into DCompSurface as a potential future optimization.

    // Setup the WIC scaling operation
    auto wicFactory = WicService::GetInstance().GetFactory();

    // Get the legacy partner interface to check for valid pixels.
    // This is necessary in case the surface has been offered (or lost) after CaptureAsync has been
    // called but before the completion handler is executed.  This check allows us to return a meaningful
    // error back to the application.
    // To test this code path, the following code must be added after setting put_Completed in PreCommit (for debug purposes).
    //    IFCFAILFAST(dcompTreeHost->OfferResources());
    ctl::ComPtr<WUComp::Internal::ICompositionDrawingSurfaceInteropPrivate> compPartner;
    IFCFAILFAST(do_query_interface(compPartner, inputCompSurface));

    BOOL isValid = TRUE;
    IFCFAILFAST(compPartner->HasValidPixels(&isValid));

    if (!isValid)
    {
        // Indicate to the application that it should try again.
        IFC_RETURN(E_PENDING);
    }

    // Get the DXGI surface
    ctl::ComPtr<WUComp::Internal::ICompositionDrawingSurfaceInteropInternalDebug> inputCompSurfaceDebug;
    IFCFAILFAST(do_query_interface(inputCompSurfaceDebug, inputCompSurface));

    ctl::ComPtr<IDXGISurface> dxgiSurface;
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(inputCompSurfaceDebug->CopySurface(nullptr, &dxgiSurface));

    // Map the source
    DXGI_SURFACE_DESC srcDesc;
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(dxgiSurface->GetDesc(&srcDesc));

    DXGI_MAPPED_RECT srcMappedRect;
    HRESULT hrMap = dxgiSurface->Map(&srcMappedRect, DXGI_MAP_READ);

    // VSTS Bug #11805892:
    // In some circumstances, Map may return E_OUTOFMEMORY while handling a device lost, even though
    // the system is not out of memory.  In this situation, we'll treat an E_OUTOFMEMORY as a device lost.
    if (hrMap == E_OUTOFMEMORY && graphicsDevice->IsDeviceLost())
    {
        hrMap = DXGI_ERROR_DEVICE_REMOVED;
    }
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(hrMap);

    auto unmapDxgiSurfaceOnFailure = wil::scope_exit([&]
    {
        IGNOREHR(dxgiSurface->Unmap());
    });

    // Verify the formats
    ASSERT(srcDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM);
    ASSERT(outputHwTexture->GetPixelFormat() == pixelColor32bpp_A8R8G8B8);

    // Map the destination
    XRECT destLockRegion =
    {
        0,
        0,
        outputHwTexture->GetWidth(),
        outputHwTexture->GetHeight(),
    };
    void* destAddress = nullptr;
    auto destStride = int32_t{ 0 };
    auto destWidth = uint32_t{ 0 };
    auto destHeight = uint32_t{ 0 };

    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(outputHwTexture->LockRect(
        destLockRegion,
        &destAddress,
        &destStride,
        &destWidth,
        &destHeight));

    auto unlockHwTextureOnFailure = wil::scope_exit([&]
    {
        IGNOREHR(outputHwTexture->Unlock());
    });

    if ((srcDesc.Width == destWidth) && (srcDesc.Height == destHeight))
    {
        // WIC doesn't do a straight source to destination copy well (even with nearest neighbor).  WIC will not guarantee an exact
        // pixel match and the performance is suboptimal.  This optimization simply copies source to destination by scanline if the
        // width and height match.
        uint8_t* srcLine = reinterpret_cast<uint8_t*>(srcMappedRect.pBits);
        uint8_t* dstLine = reinterpret_cast<uint8_t*>(destAddress);
        for (uint32_t line = 0; line < srcDesc.Height; line++)
        {
            memcpy(dstLine, srcLine, srcDesc.Width * 4);
            srcLine += srcMappedRect.Pitch;
            dstLine += destStride;
        }
    }
    else
    {
        // Setup a WICBitmap to source directly from the DXGI surface
        ctl::ComPtr<IWICBitmap> wicSrcBitmap;
        IFCFAILFAST(wicFactory->CreateBitmapFromMemory(
            srcDesc.Width,
            srcDesc.Height,
            GUID_WICPixelFormat32bppBGRA,
            srcMappedRect.Pitch,
            srcMappedRect.Pitch * srcDesc.Height,
            srcMappedRect.pBits,
            &wicSrcBitmap));

        // Make a decision on what scaling mode to use
        // Use highest quality scaling (Fant) if downscaling beyond a pre-defined threshold.
        const double FantScalingRatio = 0.5;
        WICBitmapInterpolationMode interpolationMode = WICBitmapInterpolationModeLinear;

        double widthRatio = static_cast<double>(srcDesc.Width) / static_cast<double>(destWidth);
        double heightRatio = static_cast<double>(srcDesc.Height) / static_cast<double>(destHeight);

        if ((widthRatio < FantScalingRatio) || (heightRatio < FantScalingRatio))
        {
            interpolationMode = WICBitmapInterpolationModeFant;
        }

        // Setup WIC to scale from source to destination
        ctl::ComPtr<IWICBitmapScaler> wicBitmapScaler;
        IFCFAILFAST(wicFactory->CreateBitmapScaler(&wicBitmapScaler));
        IFCFAILFAST(wicBitmapScaler->Initialize(
            wicSrcBitmap.Get(),
            destWidth,
            destHeight,
            interpolationMode));

        IFCFAILFAST(wicBitmapScaler->CopyPixels(
            nullptr,
            destStride,
            destStride * destHeight,
            reinterpret_cast<BYTE*>(destAddress)));
    }

    // Everything has been successful so far.  Unlock the textures explicitly for the following reason.
    // The return value is ignored previously if there was a failure that caused early execution
    // which is okay because we will preserve the original error code.  However, these methods could
    // still fail due to device lost (or other reasons).
    unlockHwTextureOnFailure.release();
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(outputHwTexture->Unlock());

    // Unmapping the surface may end up pushing data into the driver and some drivers are not
    // resilient to multiple thread access, so lock the D3D device.
    CD3D11SharedDeviceGuard guard;
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(graphicsDevice->TakeLockAndCheckDeviceLost(&guard));
    auto lockedContext = graphicsDevice->GetLockedDeviceContext(&guard);

    unmapDxgiSurfaceOnFailure.release();
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(dxgiSurface->Unmap());

    return S_OK;
}
