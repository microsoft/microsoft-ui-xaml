// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CompositorTreeHost.h"
#include "HWTextureMgr.h"
#include "RenderTargetBitmapWaitExecutor.h"
#include <UIThreadScheduler.h>
#include <D3D11Device.h>
#include <WindowsGraphicsDeviceManager.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of a rendertargetbitmap manager object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::Create(_In_ CCoreServices *pCoreService, _Outptr_ CRenderTargetBitmapManager **ppRenderTargetElementManager )
{
    HRESULT hr = S_OK;
    CRenderTargetBitmapManager *pManager = NULL;

    pManager = new CRenderTargetBitmapManager(pCoreService);

    *ppRenderTargetElementManager = pManager;
    RRETURN(hr);//RRETURN_REMOVAL
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Ctor.
//
//-------------------------------------------------------------------------
CRenderTargetBitmapManager::CRenderTargetBitmapManager(_In_ CCoreServices *pCore)
    : m_pCoreNoRef(pCore)
    , m_pPendingListNoRef(NULL)
    , m_pRenderingListNoRef(NULL)
    , m_pDrawingListNoRef(NULL)
    , m_pDrawWaitDataList(NULL)
    , m_pIdleWithResourcesListNoRef(NULL)
    , m_needsSurfaceContentsLost(FALSE)
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Dtor.
//
//-------------------------------------------------------------------------
CRenderTargetBitmapManager::~CRenderTargetBitmapManager()
{
    if (m_pPendingListNoRef != NULL)
    {
        m_pPendingListNoRef->Clean(FALSE);
        SAFE_DELETE(m_pPendingListNoRef);
    }
    if (m_pRenderingListNoRef != NULL)
    {
        m_pRenderingListNoRef->Clean(FALSE);
        SAFE_DELETE(m_pRenderingListNoRef);
    }
    if (m_pDrawingListNoRef != NULL)
    {
        m_pDrawingListNoRef->Clean(FALSE);
        SAFE_DELETE(m_pDrawingListNoRef);
    }
    if (m_pDrawWaitDataList != NULL)
    {
        m_pDrawWaitDataList->Clean(TRUE);
        SAFE_DELETE(m_pDrawWaitDataList);
    }
    if (m_pIdleWithResourcesListNoRef != NULL)
    {
        m_pIdleWithResourcesListNoRef->Clean(FALSE);
        SAFE_DELETE(m_pIdleWithResourcesListNoRef);
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Gets executed when a RTB's state gets set.
//      This is not necessarily a state change.
//      Updates various lists accordingly.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::OnSetCurrentState(_In_ IRenderTargetElement *pItem)
{
    HRESULT hr = S_OK;
    switch (pItem->GetCurrentState())
    {
        case RenderTargetElementState::Idle:
        {
            IFC(RemoveRenderTargetElement(pItem));
            if (pItem->HasHardwareResources())
            {
                if (m_pIdleWithResourcesListNoRef == NULL)
                {
                    // Reserve based on values seen in perf runs.
                    m_pIdleWithResourcesListNoRef = new CXcpList<IRenderTargetElement>(16);
                }
                IFC(m_pIdleWithResourcesListNoRef->Add(pItem));
            }
            break;
        }
        case RenderTargetElementState::Preparing:
        {
            IFC(RemoveRenderTargetElement(pItem));
            if (m_pPendingListNoRef == NULL)
            {
                // Reserve based on values seen in perf runs.
                m_pPendingListNoRef = new CXcpList<IRenderTargetElement>(16);
            }
            IFC(m_pPendingListNoRef->Add(pItem));
            break;
        }
        case RenderTargetElementState::Rendering:
        {
            ASSERT(m_pPendingListNoRef != NULL);
            IFC(m_pPendingListNoRef->Remove(pItem, FALSE));
            ASSERT(hr == S_OK);
            if (m_pRenderingListNoRef == NULL)
            {
                // Reserve based on values seen in perf runs.
                m_pRenderingListNoRef = new CXcpList<IRenderTargetElement>(16);
            }
            IFC(m_pRenderingListNoRef->Add(pItem));
            break;
        }
        case RenderTargetElementState::Rendered:
        {
            break;
        }
        case RenderTargetElementState::Committed:
        {
            break;
        }
        case RenderTargetElementState::Drawing:
        {
            ASSERT(m_pRenderingListNoRef != NULL);
            IFC(m_pRenderingListNoRef->Remove(pItem, FALSE));
            ASSERT(hr == S_OK);
            if (m_pDrawingListNoRef == NULL)
            {
                // Reserve based on values seen in perf runs.
                m_pDrawingListNoRef = new CXcpList<IRenderTargetElement>(16);
            }
            IFC(m_pDrawingListNoRef->Add(pItem));
            break;
        }
        default:
            ASSERT(FALSE);
    }
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Remove from the list of RenderTargetBitmap.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::RemoveRenderTargetElement(_In_ IRenderTargetElement *pItem)
{
    if (m_pIdleWithResourcesListNoRef != NULL)
    {
        IFC_RETURN(m_pIdleWithResourcesListNoRef->Remove(pItem, FALSE));
    }

    IFC_RETURN(RemoveFromDrawingLists(pItem));

    if (m_pRenderingListNoRef != NULL)
    {
        IFC_RETURN(m_pRenderingListNoRef->Remove(pItem, FALSE));
    }

    if (m_pPendingListNoRef != NULL)
    {
        IFC_RETURN(m_pPendingListNoRef->Remove(pItem, FALSE));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Removes from drawing render target bitmap list. Also updates reference in wait list.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::RemoveFromDrawingLists(_In_ IRenderTargetElement *pItem)
{
    // 1) Search for the wait info in m_pDrawWaitDataList whose end marker matches the given RTB.
    // 2) Keep track of the previous wait info.
    // 3) Search for the passed in RTB in m_pDrawingListNoRef and find its previous.
    // 4) If previous RTB is NULL or if it is the end marker of previous wait info, then it means that
    //     there are no RTBs waiting on this wait info. Hence remove it from the list.
    // 5) Else update the end marker of the wait info.
    // 6) Finally remove the passed in RTB from m_pDrawingListNoRef.
    // NOTE: This is primarily necessary for the legacy RTB rendering code path which can cancel
    //       drawing which is handled entirely within XAML.  For SpriteVisuals mode, CaptureAsync does
    //       the drawing and is currently non-cancellable.  The SpriteVisuals code path also
    //       never uses the m_pDrawingListNoRef so it will bypass this check.  SpriteVisuals mode is
    //       handled by allowing the operations to complete and subsequent operations will overwrite the
    //       data in previous operations.
    if ((m_pDrawWaitDataList != nullptr) && (m_pDrawingListNoRef != nullptr))
    {
        RenderTargetElementWaitData *pPreviousWaitData = NULL;
        for (size_t wi = 0; wi < m_pDrawWaitDataList->Size(); ++wi)
        {
            auto* pCurrentWaitData = m_pDrawWaitDataList->At(wi);
            if (pItem == pCurrentWaitData->m_pRenderTargetElementNoRef)
            {
                ASSERT(m_pDrawingListNoRef != NULL);
                IRenderTargetElement *pPrevRenderTargetBitmap = NULL;
                for (size_t i = 1; i < m_pDrawingListNoRef->Size(); ++i)
                {
                    if (m_pDrawingListNoRef->At(i) == pItem)
                    {
                        pPrevRenderTargetBitmap = m_pDrawingListNoRef->At(i - 1);
                        break;
                    }
                }
                if (pPrevRenderTargetBitmap != NULL &&
                    (pPreviousWaitData == NULL ||
                    pPreviousWaitData->m_pRenderTargetElementNoRef != pPrevRenderTargetBitmap))
                {
                    pCurrentWaitData->m_pRenderTargetElementNoRef = pPrevRenderTargetBitmap;
                }
                else
                {
                    IFC_RETURN(m_pDrawWaitDataList->Remove(pCurrentWaitData, TRUE));
                }
                break;
            }
            pPreviousWaitData = pCurrentWaitData;
        }
    }

    if (m_pDrawingListNoRef != NULL)
    {
        IFC_RETURN(m_pDrawingListNoRef->Remove(pItem, FALSE));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if an element is suitable of being the render root for RTB.
//
//-------------------------------------------------------------------------
bool
CRenderTargetBitmapManager::CanPickupForRender(_In_ CUIElement *pElement)
{
    // If the element is not active and is not a root visual then do not pick it up.
    // Note: Root visual is never active, hence the special check.
    if (!pElement->IsActive() &&
        !pElement->OfTypeByIndex<KnownTypeIndex::RootVisual>())
    {
        return false;
    }
    if (pElement->OfTypeByIndex<KnownTypeIndex::RenderTargetBitmapRoot>())
    {
        return false;
    }
    CUIElement *pCurrent = pElement;
    CRootVisual *pMainRootVisual = m_pCoreNoRef->GetMainRootVisual();
    while (pCurrent)
    {
        CDependencyObject *pParent = pCurrent->GetParentInternal(false);
        if (!pCurrent->IsVisible())
        {
            return false;
        }
        if (pCurrent->OfTypeByIndex<KnownTypeIndex::RootVisual>() &&
            pCurrent != pMainRootVisual)
        {
            return false;
        }
        if (!CRenderTargetBitmap::IsAncestorTypeEligible(pCurrent))
        {
            return false;
        }
        if (pParent != NULL &&
            !pParent->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            return false;
        }

        pCurrent = static_cast<CUIElement *>(pParent);
    }
    return true;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Move RenderTargetBitmaps from pending list to ready list.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::PickupForRender()
{
    bool allowPickup = true;

    // Do not pickup RTBs if we are in the middle of
    // setting Window.Content.
    if (m_pCoreNoRef->GetDeployment() &&
        m_pCoreNoRef->GetDeployment()->m_pApplication &&
        m_pCoreNoRef->GetDeployment()->m_pApplication->m_fRootVisualSet)
    {
        allowPickup = FALSE;
    }

    // In background task allow pickup only if there are no pending decodes.
    // Note that we wait until the global pending decode count hits 0.
    // This means we could be waiting for pending decodes of images
    // which are not even in the subtree to be captured by a particular
    // RTB. This could have caused stravation in foreground apps but
    // this should be fine in background tasks where there is no on-screen tree.
    if (m_pCoreNoRef->IsInBackgroundTask() && m_pCoreNoRef->GetPendingDecodeCount() > 0)
    {
        allowPickup = FALSE;
    }

    if (allowPickup && m_pPendingListNoRef != NULL)
    {
        // Erase-while-iterate: SetCurrentState/FailRender may remove the current element
        // from this list (via OnSetCurrentState). We detect removal by comparing the list
        // size before and after, and only advance the index when no removal occurred.
        for (size_t i = 0; i < m_pPendingListNoRef->Size(); )
        {
            const size_t sizeBefore = m_pPendingListNoRef->Size();
            IRenderTargetElement *pRenderTargetElementNoRef = m_pPendingListNoRef->At(i);
            if (CanPickupForRender(pRenderTargetElementNoRef->GetRenderTargetElementData()->GetRenderElement()))
            {
                IFC_RETURN(pRenderTargetElementNoRef->SetCurrentState(RenderTargetElementState::Rendering));
            }
            else
            {
                // TODO: RTB: Appropriate HR?
                IFC_RETURN(pRenderTargetElementNoRef->FailRender(E_INVALIDARG));
            }

            if (m_pPendingListNoRef == NULL) { break; }
            if (m_pPendingListNoRef->Size() < sizeBefore) { /* removed, don't advance */ }
            else { ++i; }
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Render for all the batched RenderTargetBitmaps
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::RenderElements(
    _In_ HWWalk *pHWWalk,
    _In_ CWindowRenderTarget* pRenderTarget,
    _Out_ bool *pHasPendingDraws)
{
    *pHasPendingDraws = FALSE;
    if (m_pRenderingListNoRef != NULL)
    {
        const bool hadNodes = !m_pRenderingListNoRef->Empty();
        BOOLEAN isReadyForRenderTargetElementRender = TRUE;

        // Erase-while-iterate: SetCurrentState may remove the current element from
        // m_pRenderingListNoRef (via OnSetCurrentState). We detect removal by comparing
        // the list size before and after, and only advance the index when no removal occurred.
        for (size_t i = 0; i < m_pRenderingListNoRef->Size(); )
        {
            size_t sizeBefore = m_pRenderingListNoRef->Size();
            IRenderTargetElement *pRenderTargetElement = m_pRenderingListNoRef->At(i);
            if (pRenderTargetElement->GetCurrentState() == RenderTargetElementState::Rendering)
            {
                CRenderTargetElementData *pRenderTargetElementDataNoRef = pRenderTargetElement->GetRenderTargetElementData();
                ASSERT(pRenderTargetElementDataNoRef != NULL);

                CUIElement *pRenderElementNoRef = pRenderTargetElementDataNoRef->GetRenderElement();

                IFC_RETURN(pRenderElementNoRef->IsReadyForRenderTargetElementRender(&isReadyForRenderTargetElementRender));
                if (isReadyForRenderTargetElementRender)
                {
                    IFC_RETURN(pRenderTargetElement->SetCurrentState(RenderTargetElementState::Rendered));

                    *pHasPendingDraws = TRUE;
                }
                else
                {
                    // If the root element is not ready to rendered this
                    // frame and redo the render target capture from
                    // the beginning.
                    //
                    // In case of background tasks the responsibility to
                    // wait until the tree is ready is upon the framework.
                    // If the inner bounds are dirty at this moment (this
                    // could happen because of CCoreServices::UpdateDirtyState),
                    // then we might have used the inner bounds of the element
                    // too early. Hence retrying makes sense.

                    pRenderTargetElementDataNoRef->ResetState();
                    IFC_RETURN(pRenderTargetElement->SetCurrentState(RenderTargetElementState::Preparing));

                    // Request a new frame so that this can be
                    // processed again in the next frame.
                    IXcpBrowserHost *pBH = m_pCoreNoRef->GetBrowserHost();
                    if (pBH != NULL)
                    {
                        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
                        if (pFrameScheduler != NULL)
                        {
                            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::RTBRender));
                        }
                    }
                }
            }
            if (m_pRenderingListNoRef == NULL) { break; }
            if (m_pRenderingListNoRef->Size() < sizeBefore) { /* removed, don't advance */ }
            else { ++i; }
        }

        // Try cleaning up staging resources when in background task,
        // if there no RTBs under process.
        if ( hadNodes &&
            (m_pRenderingListNoRef == NULL || m_pRenderingListNoRef->Empty()) &&
            (m_pDrawingListNoRef == NULL || m_pDrawingListNoRef->Empty()))
        {
            IFC_RETURN(CleanupResourcesForBackgroundTask());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CRenderTargetBitmapManager::PreCommit(
    _In_ CWindowRenderTarget* renderTarget)
{
    // PreCommit operates on elements in the Rendered state prior to drawing them.
    // Go through all elements in the rendering list with the Rendered state and call their pre-commit function.
    // DrawCompTrees will be called shortly afterwards to update the state machine.
    if (m_pRenderingListNoRef != nullptr)
    {
        // In case of background tasks the responsibility to
        // wait until the tree is ready is upon the framework.
        // If new decode requests came up during the render
        // walk due to the "Decode To Render size" feature,
        // then we will have to wait until the new set of decodes
        // are done. Hence reset state and retry.
        bool resetToPreparing = false;
        if (m_pCoreNoRef->IsInBackgroundTask())
        {
            IFC_RETURN(m_pCoreNoRef->FlushImageDecodeRequests());
            if (m_pCoreNoRef->GetPendingDecodeCount() > 0)
            {
                resetToPreparing = true;
            }
        }

        if (resetToPreparing)
        {
            // Erase-while-iterate: SetCurrentState may remove the current element from
            // m_pRenderingListNoRef (via OnSetCurrentState). Detect removal via size check.
            for (size_t i = 0; i < m_pRenderingListNoRef->Size(); )
            {
                size_t sizeBefore = m_pRenderingListNoRef->Size();

                auto renderTargetElement = m_pRenderingListNoRef->At(i);

                if (renderTargetElement->GetCurrentState() == RenderTargetElementState::Rendered)
                {
                    auto renderTargetElementDataNoRef = renderTargetElement->GetRenderTargetElementData();
                    renderTargetElementDataNoRef->ResetState();
                    IFC_RETURN(renderTargetElement->SetCurrentState(RenderTargetElementState::Preparing));
                }

                if (m_pRenderingListNoRef == NULL) { break; }
                if (m_pRenderingListNoRef->Size() < sizeBefore) { /* removed, don't advance */ }
                else { ++i; }
            }
        }
        else
        {
            // Go through the rendering list and for each item that is in the Rendered state
            // Create an event and add it to the wait list.  Also call PreCommit on all those
            // items.
            // Erase-while-iterate: SetCurrentState may remove the current element from
            // m_pRenderingListNoRef (via OnSetCurrentState). Detect removal via size check.
            for (size_t i = 0; i < m_pRenderingListNoRef->Size(); )
            {
                size_t sizeBefore = m_pRenderingListNoRef->Size();

                auto renderTargetElement = m_pRenderingListNoRef->At(i);

                if (renderTargetElement->GetCurrentState() == RenderTargetElementState::Rendered)
                {
                    IPALEvent* completionEvent = nullptr;
                    auto closeEventOnFailure = wil::scope_exit([&]
                    {
                        if (completionEvent != nullptr)
                        {
                            VERIFYHR(completionEvent->Close());
                        }
                    });

                    // All SpriteVisual RTBs (requiring draw or not) need this event.
                    // RTB is not required to manage a draw operation in which case it will be managed
                    // by the renderTargetElement itself.  This should create an appropriate event and queue
                    // a wait operation for it.
                    IFC_RETURN(GetPALThreadingServices()->EventCreate(
                        &completionEvent,
                        FALSE /* bSignaled */,
                        FALSE /* bManual */));

                    // PreCommit may fail if the target element has left the visual tree
                    // (e.g., popup closed). Call the outer FailRender to properly transition
                    // to Idle and complete the async action with an error.
                    // scope_exit closes the event; no wait was submitted so no hang.
                    HRESULT preCommitHr = renderTargetElement->PreCommit(renderTarget, completionEvent);
                    if (FAILED(preCommitHr))
                    {
                        IGNOREHR(renderTargetElement->FailRender(preCommitHr));
                        continue;
                    }

                    AddWaitItem(renderTargetElement, completionEvent);

                    IFC_RETURN(SubmitWaitWorkItem(completionEvent));

                    // No failures occurred so submit the work item
                    completionEvent = nullptr;

                    IFC_RETURN(renderTargetElement->SetCurrentState(RenderTargetElementState::Committed));
                }

                if (m_pRenderingListNoRef == NULL) { break; }
                if (m_pRenderingListNoRef->Size() < sizeBefore) { /* removed, don't advance */ }
                else { ++i; }
            }
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//      Calls Draw on RenderTargetBitmaps which are already rendered.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::DrawCompTrees(
    _In_ CWindowRenderTarget *pRenderTarget)
{
    if (m_pRenderingListNoRef != NULL)
    {
        const bool hadNodes = !m_pRenderingListNoRef->Empty();

        // Erase-while-iterate: SetCurrentState may remove the current element from
        // m_pRenderingListNoRef (via OnSetCurrentState). Detect removal via size check.
        for (size_t i = 0; i < m_pRenderingListNoRef->Size(); )
        {
            size_t sizeBefore = m_pRenderingListNoRef->Size();

            IRenderTargetElement *pRenderTargetElement = m_pRenderingListNoRef->At(i);
            if (pRenderTargetElement->GetCurrentState() == RenderTargetElementState::Committed)
            {
                // The pRenderTargetElement doesn't require the RTB drawing management so put it
                // in the drawing state since it has reached the drawing stage.  Any event handling
                // should have been setup in PreCommit stage
                // PreCommit can also be considered the PreDraw since PreDraw makes this state
                // transition more intuitive but PreCommit is more accurate to the timing of the call.
                IFC_RETURN(pRenderTargetElement->SetCurrentState(RenderTargetElementState::Drawing));
            }

            if (m_pRenderingListNoRef == NULL) { break; }
            if (m_pRenderingListNoRef->Size() < sizeBefore) { /* removed, don't advance */ }
            else { ++i; }
        }

        // Try cleaning up staging resources when in background task,
        // if there no RTBs under process.
        if ( hadNodes &&
            (m_pRenderingListNoRef == NULL || m_pRenderingListNoRef->Empty()) &&
            (m_pDrawingListNoRef == NULL || m_pDrawingListNoRef->Empty()))
        {
            IFC_RETURN(CleanupResourcesForBackgroundTask());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Workitem callback for RenderTargetBitmap draw waits.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CRenderTargetBitmapManager::WorkCallback(
    _In_opt_ IObject *pData
    )
{
    RenderTargetBitmapWaitExecutor *pExecutor = static_cast<RenderTargetBitmapWaitExecutor*>(pData);
    ASSERT(pExecutor);
    IFC_RETURN(pExecutor->WaitAndExecuteOnUIThread());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to release staging resources on
//      D3D/D2D devices when in background tasks.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::CleanupResourcesForBackgroundTask()
{
    if (m_pCoreNoRef->IsInBackgroundTask())
    {
        CD3D11Device *pGraphicsDevice = m_pCoreNoRef->m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();
        IFC_RETURN(pGraphicsDevice->ReleaseScratchResources());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets called from UI thread asynchrnously when one of the wait handles is signaled.
//      Signals completion of corresponding async operations.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::NotifyDrawCompleted(_In_ IPALWaitable *pWaitHandle)
{
    // 1) Find the wait info matching pWaitHandle from m_pDrawWaitDataList. Also find its previous.
    // 2) Note the end marker RTBs for both wait infos.
    // 3) Search for the end marker of previous wait info in m_pDrawingListNoRef.
    // 4) Iterate from the next element in list until the end marker of the current wait info.
    // 5) Call PostDraw on all the RTBs in the range and remove them from m_pDrawingListNoRef.
    // 6) Finally remove the wait info from m_pDrawWaitDataList
    if (!m_pCoreNoRef->IsDestroyingCoreServices() &&
        m_pDrawWaitDataList != NULL)
    {
        bool deviceLost = false;
        IFC_RETURN(m_pCoreNoRef->DetermineDeviceLost(&deviceLost));
        if (deviceLost)
        {
            // We still have all the needed data to retry.
            // Hence leave the elements in drawing list
            // to be sent back to pending list by cleanup
            // code. And do nothing.
        }
        else
        {
            // Try cleaning up staging resources when in background task,
            // if there no RTBs under process. Do this before completing
            // the async actions which may queue up new RTBs.
            if ((m_pRenderingListNoRef == NULL || m_pRenderingListNoRef->Empty()) &&
                (m_pDrawWaitDataList->Size() <= 1))
            {
                IFC_RETURN(CleanupResourcesForBackgroundTask());
            }

            RenderTargetElementWaitData *pPreviousWaitData = NULL;
            for (size_t wi = 0; wi < m_pDrawWaitDataList->Size(); ++wi)
            {
                auto* pCurrentWaitData = m_pDrawWaitDataList->At(wi);
                if (pWaitHandle == pCurrentWaitData->m_pWaitHandleNoRef)
                {
                    IRenderTargetElement *pPreviousEnd = (pPreviousWaitData == NULL ? NULL : pPreviousWaitData->m_pRenderTargetElementNoRef);
                    IRenderTargetElement *pCurrentEnd = pCurrentWaitData->m_pRenderTargetElementNoRef;
                    IFC_RETURN(m_pDrawWaitDataList->Remove(pCurrentWaitData, TRUE));

                    ASSERT(pCurrentEnd != NULL);
                    ASSERT(m_pDrawingListNoRef != NULL);
                    // Find the starting index in the drawing list.
                    // If pPreviousEnd is not null, start after it; otherwise start from 0.
                    size_t startIdx = 0;
                    if (pPreviousEnd != NULL)
                    {
                        for (size_t i = 0; i < m_pDrawingListNoRef->Size(); ++i)
                        {
                            if (m_pDrawingListNoRef->At(i) == pPreviousEnd)
                            {
                                startIdx = i + 1;
                                break;
                            }
                        }
                    }
                    // Erase-while-iterate: PostDraw() calls SetCurrentState(Idle) which
                    // removes the current element from m_pDrawingListNoRef (via OnSetCurrentState).
                    // Detect removal via size check to avoid skipping the next element.
                    for (size_t i = startIdx; i < m_pDrawingListNoRef->Size(); )
                    {
                        size_t sizeBefore = m_pDrawingListNoRef->Size();
                        IRenderTargetElement *pRenderTargetElementNoRef = m_pDrawingListNoRef->At(i);
                        IFC_RETURN(pRenderTargetElementNoRef->PostDraw());
                        if (pRenderTargetElementNoRef == pCurrentEnd)
                        {
                            break;
                        }
                        if (m_pDrawingListNoRef->Size() < sizeBefore) { /* removed, don't advance */ }
                        else { ++i; }
                    }
                    break;
                }
                pPreviousWaitData = pCurrentWaitData;
            }
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Submits a workitem to wait for the pixels to be accessible
//      and then copy them to a byte array.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::SubmitGetPixelsWorkItem(
    _In_ IPALByteAccessSurface *pByteAccessSurface,
    _In_ IRenderTargetBitmapGetPixelsAsyncOperation *pAsyncOperation,
    _In_ IRenderTargetBitmapExecutorControl *pRenderTargetBitmapExecutorControl)
{
    HRESULT hr = S_OK;
    xref_ptr<RenderTargetBitmapPixelWaitExecutor> executor;
    IPALWaitable *pWaitHandle = NULL;
    IPALWorkItem *pWorkItem = NULL;

    CD3D11Device *pGraphicsDevice = m_pCoreNoRef->m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();

    IFC(pGraphicsDevice->CreateEventAndEnqueueWait(&pWaitHandle));

    executor = make_xref<RenderTargetBitmapPixelWaitExecutor>(
        m_pCoreNoRef,
        pWaitHandle,
        pByteAccessSurface,
        pAsyncOperation,
        pRenderTargetBitmapExecutorControl);

    pWaitHandle = NULL;
    IFC(m_pCoreNoRef->GetWorkItemFactory()->CreateWorkItem(
        &pWorkItem,
        &CRenderTargetBitmapManager::WorkCallback,
        executor.get() /*pData*/
        ));

    IFC(pWorkItem->Submit());
    IFC(pRenderTargetBitmapExecutorControl->AddPixelWaitExecutor(executor.get()));
Cleanup:
    if (pWaitHandle != NULL)
    {
        VERIFYHR(pWaitHandle->Close());
    }
    ReleaseInterface(pWorkItem);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if the surface contents are lost and cleans up accordingly.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::CheckForLostSurfaceContent()
{
    RRETURN(CleanupDeviceRelatedResourcesHelper(TRUE /* cleanupDiscardedOnly */, false /* cleanupDcomp */));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases all the device related resources from all
//      the RTBs.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::CleanupDeviceRelatedResources(_In_ bool cleanupDComp)
{
    RRETURN(CleanupDeviceRelatedResourcesHelper(FALSE /* cleanupDiscardedOnly */, cleanupDComp));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to cleanup device related resources on either all RTBs or
//      on RTBs with discarded resources.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::CleanupDeviceRelatedResourcesHelper(
    bool cleanupDiscardedOnly,
    bool cleanupDComp)
{
    // RTBs which are still preparing, rendering or
    // drawing through dcomp, can be retried in next
    // frame. This is because we still hold a reference
    // to their render roots. Hence move these RTBs
    // to preparing list so that they can be picked up
    // in next frame. Meanwhile get rid of their texture too.
    IFC_RETURN(CleanupDeviceRelatedResourcesOnList(
        m_pPendingListNoRef,
        FALSE /* addToPendingList */,
        FALSE /* cleanList */,
        FALSE /* setNeedsContentsLost */,
        cleanupDiscardedOnly,
        cleanupDComp));
    IFC_RETURN(CleanupDeviceRelatedResourcesOnList(
        m_pRenderingListNoRef,
        TRUE /* addToPendingList */,
        TRUE /* cleanList */,
        FALSE /* setNeedsContentsLost */,
        cleanupDiscardedOnly,
        cleanupDComp));
    IFC_RETURN(CleanupDeviceRelatedResourcesOnList(
        m_pDrawingListNoRef,
        TRUE /* addToPendingList */,
        TRUE /* cleanList */,
        FALSE /* setNeedsContentsLost */,
        cleanupDiscardedOnly,
        cleanupDComp));

    // RTBs in m_pIdleWithResourcesListNoRef cannot be
    // recreated without app intervention. This is because
    // we do not have any references to their original render
    // root. All we have is the render result in hardware texture
    // but we are about to throw it away. SurfaceContentsLost
    // event should be raised in these cases.
    IFC_RETURN(CleanupDeviceRelatedResourcesOnList(
        m_pIdleWithResourcesListNoRef,
        FALSE /* addToPendingList */,
        TRUE /* cleanList */,
        TRUE /* setNeedsContentsLost */,
        cleanupDiscardedOnly,
        cleanupDComp));

    if (m_pDrawWaitDataList != NULL &&
        !cleanupDiscardedOnly)
    {
        m_pDrawWaitDataList->Clean(TRUE);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to cleanup device related resources on a
//      given list of RTBs.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::CleanupDeviceRelatedResourcesOnList(
    _In_opt_ CXcpList<IRenderTargetElement> *pList,
    bool addToPendingList,
    bool cleanList,
    bool setNeedsContentsLost,
    bool cleanupDiscardedOnly,
    bool cleanupDComp)
{
    if (pList != NULL)
    {
        // Erase-while-iterate: SetCurrentState may remove the current element from
        // pList (via OnSetCurrentState). Detect removal via size check.
        for (size_t i = 0; i < pList->Size(); )
        {
            size_t sizeBefore = pList->Size();
            auto* item = pList->At(i);
            if (!cleanupDiscardedOnly ||
                item->HasLostHardwareResources())
            {
                if (setNeedsContentsLost
                    && item->RequiresSurfaceContentsLostNotification())
                {
                    m_needsSurfaceContentsLost = TRUE;
                }
                if (cleanupDiscardedOnly)
                {
                    // If we are to cleanup only the discarded resources,
                    // then this scenario is about failure to reclaim resources on resume.
                    // Cleanup only the cached device related resources for
                    // the current RTB request. But do not do a comprehensive walk
                    // as in the case of device loss.
                    CRenderTargetElementData *pRenderTargetElementDataNoRef =
                        item->GetRenderTargetElementData();
                    if (pRenderTargetElementDataNoRef != NULL)
                    {
                        pRenderTargetElementDataNoRef->CleanupDeviceRelatedResources();
                    }
                    item->AbortPixelWaitExecutors(E_FAIL);
                }
                else
                {
                    // If we are to cleanup all the resources (not just the discarded resources),
                    // then this scenario is for device loss. Hence do a extensive walk on the
                    // RTB to cleanup all the device related resources.
                    item->CleanupHardwareResources(cleanupDComp);
                }
                if (addToPendingList)
                {
                    IFC_RETURN(item->SetCurrentState(RenderTargetElementState::Preparing));
                }
            }
            if (pList->Size() >= sizeBefore) { ++i; }
        }
        if (cleanList)
        {
            pList->Clean(FALSE);
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CRenderTargetBitmapManager::UpdateMetrics()
{
    if (m_pPendingListNoRef != NULL)
    {
        for (auto it = m_pPendingListNoRef->OldestBegin(); it != m_pPendingListNoRef->OldestEnd(); ++it)
        {
            auto* pRenderTargetElement = *it;
            IFC_RETURN(pRenderTargetElement->GetRenderTargetElementData()->UpdateMetrics());
        }
    }
    return S_OK;
}

int
CRenderTargetBitmapManager::CountElementJobs(
    _In_ CUIElement* uiElement)
{
    int elementJobs = 0;

    auto countCallback = [&] (_In_ IRenderTargetElement* element)
    {
        if ((element->GetRenderTargetElementData() != nullptr) &&
            (element->GetRenderTargetElementData()->GetRenderElement() == uiElement))
        {
            elementJobs++;
        }
    };

    IterateList(m_pPendingListNoRef, countCallback);
    IterateList(m_pRenderingListNoRef, countCallback);
    IterateList(m_pDrawingListNoRef, countCallback);

    return elementJobs;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cancels RTB operations targeting the given element that have not
//      yet reached the capture phase (pending and rendering lists only).
//      RTBs in the drawing list have already completed CaptureAsync and
//      will finish safely without accessing the element's composition peer.
//      Called when an element leaves the visual tree (e.g., popup close)
//      to prevent PreCommit from accessing stale composition state.
//      Uses IterateList which safely captures 'next' before the callback,
//      so FailRender can safely modify the list during iteration.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapManager::CancelRenderForElement(
    _In_ CUIElement* pElement)
{
    auto cancelCallback = [&] (_In_ IRenderTargetElement* element)
    {
        if ((element->GetRenderTargetElementData() != nullptr) &&
            (element->GetRenderTargetElementData()->GetRenderElement() == pElement))
        {
            IGNOREHR(element->FailRender(E_ABORT));
        }
    };

    IterateList(m_pPendingListNoRef, cancelCallback);
    IterateList(m_pRenderingListNoRef, cancelCallback);

    return S_OK;
}

void
CRenderTargetBitmapManager::EnsureWaitList()
{
    if (m_pDrawWaitDataList == nullptr)
    {
        // Reserve based on values seen in perf runs.
        m_pDrawWaitDataList = new CXcpList<RenderTargetElementWaitData>(16);
    }
}

_Ret_notnull_ RenderTargetElementWaitData*
CRenderTargetBitmapManager::AddWaitItem(
    _In_opt_ IRenderTargetElement* renderTargetElement,
    _In_ IPALWaitable* waitableEvent
    )
{
    EnsureWaitList();

    // The element life time is managed in the list as a naked pointer.
    // TODO: Could be improved upon if it uses a more modern list.
    auto renderTargetElementWaitData = new RenderTargetElementWaitData();
    renderTargetElementWaitData->m_pRenderTargetElementNoRef = renderTargetElement;
    renderTargetElementWaitData->m_pWaitHandleNoRef = waitableEvent;

    // This can only fail on OOM which will fail fast anyway for an element of this size
    // so ignore checking the return code.
    m_pDrawWaitDataList->Add(renderTargetElementWaitData);

    return renderTargetElementWaitData;
}

_Check_return_ HRESULT
CRenderTargetBitmapManager::SubmitWaitWorkItem(
    _In_ IPALWaitable* waitableEvent
    )
{
    // TFS # 12007970:  First wait for device creation, to help prevent threadpool starvation during device lost recovery.
    WindowsGraphicsDeviceManager *deviceMgr = m_pCoreNoRef->GetBrowserHost()->GetGraphicsDeviceManager();
    IFC_RETURN(deviceMgr->WaitForD3DDependentResourceCreation());

    // Control flow explained:
    // 1) RenderTargetBitmapWaitExecutor instance gets created.
    // 2) WorkItem gets created with RenderTargetBitmapWaitExecutor instance as its data
    // 3) WorkItem callback (RenderTargetBitmapWorkCallback) calls WaitAndExecuteOnUIThread on RenderTargetBitmapWaitExecutor instance
    // 4) WaitAndExecuteOnUIThread waits on pWaitHandle and then enqueues self to be executed asycn on UI thread
    // 5) RenderTargetBitmapWaitExecutor::Execute gets called on UI thread.
    // 6) RenderTargetBitmapWaitExecutor::Execute calls CCoreServices::NotifyRenderTargetBitmaps
    // 7) CCoreServices::NotifyRenderTargetBitmaps marks completion of render/draw cycle for corresponding RTBs.
    auto renderTargetElementWaitExecutor = make_xref<RenderTargetBitmapWaitExecutor>(m_pCoreNoRef, waitableEvent);
    xref_ptr<IPALWorkItem> workItem;
    IFC_RETURN(m_pCoreNoRef->GetWorkItemFactory()->CreateWorkItem(
        workItem.ReleaseAndGetAddressOf(),
        &CRenderTargetBitmapManager::WorkCallback,
        renderTargetElementWaitExecutor.get() /*pData*/));
    IFC_RETURN(workItem->Submit());

    return S_OK;
}

template<class T>
void CRenderTargetBitmapManager::IterateList(
    _In_opt_ CXcpList<IRenderTargetElement>* elementList,
    T&& callbackFunc
    )
{
    // Check the list for null.  It can be null in case of FailRender or similar situation that causes an RTB to
    // exit early prior to each state being set for the first time, thus allocating all the lists.  It is rare, but it can happen.
    if (elementList != nullptr)
    {
        for (auto it = elementList->NewestBegin(); it != elementList->NewestEnd(); ++it)
        {
            callbackFunc(*it);
        }
    }
}
