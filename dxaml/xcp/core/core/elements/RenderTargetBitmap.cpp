// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <GraphicsUtility.h>
#include <UIThreadScheduler.h>
#include <WindowsGraphicsDeviceManager.h>
#include <RenderTargetBitmapImplBase.h>
#include <RenderTargetBitmapWaitExecutor.h>
#include <RenderTargetBitmapImplUsingSpriteVisuals.h>
#include <DCompTreeHost.h>
#include <hwtexturemgr.h>
#include <hwwalk.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <MUX-ETWEvents.h>
#include <PixelFormat.h>

// TODO_WinRT: When the legacy RenderTargetBitmap is no longer necessary.  A lot of code can be removed:
//             - A lot of the CRenderTargetBitmapManager class that does drawing
//             - A lot of the CUIElement class that interacts with the manager and old drawing mechanisms

CRenderTargetBitmap::CRenderTargetBitmap(_In_ CCoreServices* core)
    : CImageSource(core)
    , m_currentState(RenderTargetElementState::Idle)
{
}

CRenderTargetBitmap::~CRenderTargetBitmap()
{
    // Cleanup hardware resources first because the CRenderTargetBitmapManager
    // checks the hardware resources during state change to determine if an
    // entry needs to be added to the idle list.
    if (m_pImageSurfaceWrapper != NULL &&
        m_pImageSurfaceWrapper->CheckForHardwareResources())
    {
        m_pImageSurfaceWrapper->ReleaseHardwareResources();
    }

    VERIFYHR(SetCurrentState(RenderTargetElementState::Idle));

    AbortPixelWaitExecutors(S_OK);
}

_Check_return_ HRESULT
CRenderTargetBitmap::PixelWidth(
    _In_ CDependencyObject* object,
    uint32_t argCount,
    _In_reads_(argCount) CValue* args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue* result
    )
{
    IFCPTR_RETURN(object);

    CRenderTargetBitmap* renderTargetBitmap = nullptr;
    IFC_RETURN(DoPointerCast(renderTargetBitmap, object));

    result->SetSigned(renderTargetBitmap->m_impl->GetPixelWidth());

    return S_OK;
}

_Check_return_ HRESULT
CRenderTargetBitmap::PixelHeight(
    _In_ CDependencyObject* object,
    uint32_t argCount,
    _In_reads_(argCount) CValue* args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue *result
    )
{
    IFCPTR_RETURN(object);

    CRenderTargetBitmap* renderTargetBitmap = nullptr;
    IFC_RETURN(DoPointerCast(renderTargetBitmap, object));

    result->SetSigned(renderTargetBitmap->m_impl->GetPixelHeight());

    return S_OK;
}

_Check_return_ HRESULT
CRenderTargetBitmap::InitInstance()
{
    xref_ptr<ImageSurfaceWrapper> imageSurfaceWrapper;
    IFC_RETURN(ImageSurfaceWrapper::Create(
        GetContext(),
        FALSE /* mustKeepSoftwareSurface */,
        imageSurfaceWrapper.ReleaseAndGetAddressOf()
        ));

    // Initialize a 1x1 transparent surface that will be used
    // when rendering an opacity mask for a shape.
    // The Silverlight rasterizer locks the software surface associated
    // with the fill/stroke brush, but then does nothing with it
    uint32_t transparentPixel = 0;
    xref_ptr<CMemorySurface> memorySurface;
    IFC_RETURN(CMemorySurface::Create(
        pixelColor32bpp_A8R8G8B8,
        1, // width
        1, // height
        sizeof(transparentPixel), // stride
        sizeof(transparentPixel), // buffer size
        &transparentPixel,
        memorySurface.ReleaseAndGetAddressOf()
        ));

    imageSurfaceWrapper->SetSoftwareSurface(memorySurface.get());

    // TODO: Detaching ref counted pointers is ugly and can lead to errors.
    //                 Fix this when m_pImageSurfaceWrapper is converted to an xref_ptr.
    m_pImageSurfaceWrapper = imageSurfaceWrapper.detach();

    return S_OK;
}

RenderTargetElementState
CRenderTargetBitmap::GetCurrentState() const
{
    return m_currentState;
}

// Sets current state to the given. This keeps list management in RenderTargetBitmapManager consistent.
_Check_return_ HRESULT
CRenderTargetBitmap::SetCurrentState(RenderTargetElementState currentState)
{
    ASSERT((currentState != RenderTargetElementState::Rendering ||
            m_currentState == RenderTargetElementState::Preparing) &&
           (currentState != RenderTargetElementState::Rendered ||
            m_currentState == RenderTargetElementState::Rendering) &&
           (currentState != RenderTargetElementState::Committed ||
            m_currentState == RenderTargetElementState::Rendered) &&
           (currentState != RenderTargetElementState::Drawing ||
            m_currentState == RenderTargetElementState::Committed));

    m_currentState = currentState;

    IFC_RETURN(GetContext()->GetRenderTargetBitmapManager()->OnSetCurrentState(this));

    return S_OK;
}

_Ret_notnull_ CRenderTargetElementData*
CRenderTargetBitmap::GetRenderTargetElementData() const
{
    return m_impl->GetRenderTargetElementData();
}

int32_t
CRenderTargetBitmap::GetPixelWidth() const
{
    return (m_impl != nullptr) ? m_impl->GetPixelWidth() : 0;
}

int32_t
CRenderTargetBitmap::GetPixelHeight() const
{
    return (m_impl != nullptr) ? m_impl->GetPixelHeight() : 0;
}

// RTB differentiates between surface bounds and layout bounds.
// This is because we create RTB surface appropriate for zoom scale.
// So if the zoom scaled surface bounds are used for layout as well
// then it gets scaled again during render, thus making things blurry.
// Hence we use unscaled dimension for layout.
uint32_t
CRenderTargetBitmap::GetLayoutWidth()
{
    return (m_impl != nullptr) ? m_impl->GetLayoutWidth() : 0;
}

uint32_t
CRenderTargetBitmap::GetLayoutHeight()
{
    return (m_impl != nullptr) ? m_impl->GetLayoutHeight() : 0;
}

_Check_return_ HRESULT
CRenderTargetBitmap::RequestRender(
    _In_ CUIElement* element,
    _In_ int32_t scaledWidth,
    _In_ int32_t scaledHeight,
    _In_ ICoreAsyncAction* asyncAction)
{
    auto core = GetContext();

    if (element == nullptr)
    {
        element = core->GetMainRootVisual();
    }
    ASSERT(element != nullptr);
    ASSERT(element->GetContext() == core);

    // Setting the state to Idle will ensure with the RenderTargetBitmapManager that any in-progress jobs
    // with this RTB will be effectively canceled which means the implementation can be switched.  This is
    // important because RTB using SpriteVisuals cannot be used if the UI element is in a BitmapCache subtree.
    IFC_RETURN(SetCurrentState(RenderTargetElementState::Idle));

    m_impl = std::make_unique<RenderTargetBitmapImplUsingSpriteVisuals>(
        *core->GetRenderTargetBitmapManager());

    auto hr = m_impl->RequestRender(element, scaledWidth, scaledHeight, asyncAction);

    if (SUCCEEDED(hr))
    {
        IFC_RETURN(SetCurrentState(RenderTargetElementState::Preparing));

        IXcpBrowserHost *pBH = core->GetBrowserHost();
        if (pBH != nullptr)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
            if (pFrameScheduler != nullptr)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(
                    0 /* immediate */,
                    RequestFrameReason::RTBRender));
            }
        }
    }

    return hr;
}

// PreCommit can also be thought of as PreDraw which prepares for any drawing operation.  However, PreCommit is
// more accurate since it runs before the composition commit and allows composition commands to be added to the
// batch prior to drawing.
_Check_return_ HRESULT
CRenderTargetBitmap::PreCommit(
    _In_ CWindowRenderTarget* renderTarget,
    _In_opt_ IPALEvent* completionEvent)
{
    auto renderTargetElementData = m_impl->GetRenderTargetElementData();
    ASSERT(renderTargetElementData != nullptr);

    auto oldWidth = m_impl->GetPixelWidth();
    auto oldHeight = m_impl->GetPixelHeight();

    auto newWidth = renderTargetElementData->GetPixelWidth();
    auto newHeight = renderTargetElementData->GetPixelHeight();

    // Reset and Create the hardware surface texture if needed.
    if (newWidth != oldWidth ||
        newHeight != oldHeight ||
        !m_pImageSurfaceWrapper->CheckForHardwareResources())
    {
        if (m_pImageSurfaceWrapper->HasHardwareSurfaces())
        {
            m_pImageSurfaceWrapper->ReleaseHardwareResources();
        }

        if (newWidth > 0 &&
            newHeight > 0)
        {
            HWTextureManager *pHWTextureMgrNoRef = renderTarget->GetHwWalk()->GetTextureManager();
            ASSERT(pHWTextureMgrNoRef != nullptr);

            HWTextureFlags flags = HWTextureFlags_None;
            // TODO: RTB Assume always non-opaque for now
            //if (IsOpaque())
            //{
            //    flags = static_cast<HWTextureFlags>(flags | HWTextureFlags_IsOpaque);
            //}

            IFC_RETURN(pHWTextureMgrNoRef->CreateTexture(
                pixelColor32bpp_A8R8G8B8,
                newWidth,
                newHeight,
                flags,
                m_rtbTexture.ReleaseAndGetAddressOf(),
                nullptr
                ));
        }
    }

    if (HasValidTexture())
    {
        IFC_RETURN(m_impl->PreCommit(
            renderTarget->GetDCompTreeHost(),
            m_rtbTexture.get(),
            completionEvent));
    }
    else if (completionEvent != nullptr)
    {
        // Complete the async operation immediately.  It isn't necessarily an error
        // to render an element with no size.
        completionEvent->Set();
    }

    return S_OK;
}

_Check_return_ HRESULT
CRenderTargetBitmap::PostDraw()
{
    auto hr = m_impl->PostDraw();

    if (hr == E_PENDING)
    {
        // CaptureAsync capture could not complete, such as for offered surfaces.
        // Signal the SurfaceContentsLost so the application knows to try again.
        IFCFAILFAST(GetContext()->EnqueueSurfaceContentsLostEvent());
    }

    IFC(hr);

    // Done with the hardware texture, setup the image surface wrapper so that it can be used for rendering.
    // If it is lost, set it to the ImageSurfaceWrapper anyway so that the render walk can detect the lost surface.
    if (m_rtbTexture != nullptr)
    {
        m_pImageSurfaceWrapper->SetHardwareSurface(m_rtbTexture.get());
        m_rtbTexture.reset();
    }

    // Flush any pending texture updates since the update may have been queued but it requires a flush to guarantee
    // that GetPixels can get the surface bits.
    IFC(GetContext()->GetHWTextureManagerNoRef()->SubmitTextureUpdates());

    // Invalidate the image on the next draw.
    m_fSourceNeedsMeasure = TRUE;
    SetDirty();

Cleanup:
    // Regardless of any error we might be bubbling out, we must guarantee this RTB transitions back to Idle state
    // in order for RenderTargetBitmapManager state machinery to work correctly.
    IFCFAILFAST(SetCurrentState(RenderTargetElementState::Idle));

    return hr;
}

// Fails the current render request with the given HR, and releases all the related data.
_Check_return_ HRESULT
CRenderTargetBitmap::FailRender(HRESULT failureHR)
{
    IFC_RETURN(m_impl->FailRender(failureHR));

    m_rtbTexture.reset();

    IFC_RETURN(SetCurrentState(RenderTargetElementState::Idle));

    return S_OK;
}

// Queues a texture copy and submit a work item to wait and complete the operation.
_Check_return_ HRESULT
CRenderTargetBitmap::RequestPixels(_In_ IRenderTargetBitmapGetPixelsAsyncOperation* asyncOperation)
{
    HRESULT hr = S_OK;
    bool lostResources = false;
    XBYTE* bytes = NULL;
    auto core = GetContext();

    if (m_pImageSurfaceWrapper->CheckForLostHardwareResources())
    {
        lostResources = TRUE;
    }
    else
    {
        IFC(core->DetermineDeviceLost(&lostResources));
    }

    if (!lostResources)
    {
        if (m_pImageSurfaceWrapper->CheckForHardwareResources())
        {
            HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
            ASSERT(pHWSurfaceNoRef && pHWSurfaceNoRef->GetCompositionSurface());

            xref_ptr<IPALByteAccessSurface> byteAccessSurface;
            IFC(pHWSurfaceNoRef->GetCompositionSurface()->CopyToSurface(
                byteAccessSurface.ReleaseAndGetAddressOf()));

            IFC(core->GetRenderTargetBitmapManager()->SubmitGetPixelsWorkItem(
                byteAccessSurface,
                asyncOperation,
                this));
        }
        else
        {
            bytes = new XBYTE[0];
            RenderTargetBitmapPixelData pixelData;
            pixelData.m_length = 0;
            pixelData.m_pBytes = bytes;
            IFC(asyncOperation->CoreSetResults(pixelData));
            asyncOperation->CoreFireCompletion();
            asyncOperation->CoreReleaseRef();
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        SAFE_DELETE_ARRAY(bytes);
    }

    if (GraphicsUtility::IsDeviceLostError(hr))
    {
        // Device could have been lost after the check
        // at the beginning of the method. Hence check again.
        // Calling DetermineDeviceLost would update the
        // state on core and request another frame if needed.
        VERIFYHR(core->DetermineDeviceLost(&lostResources));
        ASSERT(lostResources);

        // TODO: DEVICELOST: Trace
        hr = S_OK;
    }

    if (lostResources)
    {
        // TODO: RTB Set appropriate error for device lost.
        asyncOperation->CoreSetError(E_FAIL);
        asyncOperation->CoreFireCompletion();
        asyncOperation->CoreReleaseRef();
    }

    return hr;
}

// The virtual method which does the tree walk to clean up all
// the device related resources like brushes, textures,
// primitive composition data etc. in this subgraph.
void CRenderTargetBitmap::CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp)
{
    CImageSource::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    if (m_impl != nullptr)
    {
        m_impl->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    m_rtbTexture.reset();

    // TODO: RTB Set appropriate error for device lost.
    AbortPixelWaitExecutors(E_FAIL);
}

bool
CRenderTargetBitmap::HasValidTexture()
{
    return (m_rtbTexture != nullptr) && !m_rtbTexture->IsSurfaceLost();
}

_Check_return_ HRESULT
CRenderTargetBitmap::AddPixelWaitExecutor(_In_ RenderTargetBitmapPixelWaitExecutor* executor)
{
    if (m_pixelWaitExecutorList == nullptr)
    {
        m_pixelWaitExecutorList = wil::make_unique_failfast<CXcpList<RenderTargetBitmapPixelWaitExecutor>>();
    }

    IFC_RETURN(m_pixelWaitExecutorList->Add(executor));

    return S_OK;
}

_Check_return_ HRESULT
CRenderTargetBitmap::RemovePixelWaitExecutor(_In_ RenderTargetBitmapPixelWaitExecutor* executor)
{
    ASSERT(m_pixelWaitExecutorList != nullptr);

    return m_pixelWaitExecutorList->Remove(executor, FALSE);
}

void CRenderTargetBitmap::AbortPixelWaitExecutors(HRESULT resultHR)
{
    if (m_pixelWaitExecutorList != nullptr)
    {
        CXcpList<RenderTargetBitmapPixelWaitExecutor>::XCPListNode *pCurrent = m_pixelWaitExecutorList->GetHead();
        while (pCurrent)
        {
            pCurrent->m_pData->AbortAsyncOperation(resultHR);
            pCurrent = pCurrent->m_pNext;
        }
        m_pixelWaitExecutorList->Clean(FALSE);
        m_pixelWaitExecutorList.reset();
    }
}

bool
CRenderTargetBitmap::IsAncestorTypeEligible(_In_ CUIElement* element)
{
    if (element->OfTypeByIndex<KnownTypeIndex::TransitionRoot>() ||
        element->OfTypeByIndex<KnownTypeIndex::Popup>() ||
        element->OfTypeByIndex<KnownTypeIndex::PrintRoot>())
    {
        return false;
    }

    return true;
}
