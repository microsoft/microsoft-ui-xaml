// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// IInputManager implementation
//
// Synchronous Input events are fired without taking the reentrancy guard,
// to allow the application's input event handlers to call API like the
// following, which pump messages and cause reentrancy:
// CoreDispatcher.ProcessEvents
// CoWaitForMultipleHandles
//
// For example, tick or other input events may be pumped while InputManager
// is in a synchronous callout. So InputManager needs to be hardened against
// reentrancy. It should ensure that objects are alive and that state is
// re-validated after these callouts return.

#include "precomp.h"
#include "InputServices.h"
#include "PointerAnimationUsingKeyFrames.h"
#include "DirectManipulationServiceSharedState.h"
#include "XboxUtility.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "Timer.h"
#include "TimeSpan.h"
#include "ContextRequestedEventArgs.h"
#include <KeyboardUtility.h>
#include <UIThreadScheduler.h>
#include "EnumDefs.h"
#include "XamlIslandRoot.h"
#include "TextCommon.h"
#include "KeyboardAcceleratorUtility.h"
#include <XamlOneCoreTransforms.h>
#include "InitialFocusSIPSuspender.h"
#include "FocusLockOverrideGuard.h"
#include "JupiterWindow.h"
#include <DXamlServices.h>

#include <FocusManagerLostFocusEventArgs.h>
#include <FocusManagerGotFocusEventArgs.h>
#include <CaretBrowsingGlobal.h>
#include <FeatureFlags.h>
#include <FocusSelection.h>
#include "RootScale.h"

#include "DirectManipulationService.h"
#include "isapipresent.h"

#include <ReentrancyGuard.h>
#include <Windowing.h>

#undef max
#undef min

using namespace DirectUI;
using namespace Focus;

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation debug outputs, and 0 otherwise
#define DMIM_DBG 0

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation verbose debug outputs, and 0 otherwise
#define DMIMv_DBG 0

//#define TIEIM_DBG

#define ExitOnSetContactFailure(x) if (x) { goto Cleanup; }

using namespace RuntimeFeatureBehavior;

CInputServices::CInputServices(_In_ CCoreServices *pCoreService)
    : m_pVisualTree(pCoreService->GetMainVisualTree())
{
    Init(pCoreService);
}

void
CInputServices::Init(_In_ CCoreServices *pCoreService)
{
    m_qpcFirstPointerUpSinceLastFrame = 0;

    m_pTextCompositionTargetDO = NULL;

    XCP_WEAK(&m_pCoreService);
    m_pCoreService = static_cast<CCoreServices*>(pCoreService);

    m_pEventManager = m_pCoreService->GetEventManager();
    m_pEventManager->AddRef();

    m_bStylusInvertedOnDown = FALSE;
    m_ptStylusPosLast.x = 0.0;
    m_ptStylusPosLast.y = 0.0;
    m_fStylusPressureFactorLast = 0.0;

    m_pViewports = NULL;
    m_pCrossSlideViewports = NULL;
    m_pSecondaryContentRelationshipsToBeApplied = NULL;

    m_pDMServices = NULL;
    m_DMServiceSharedState = std::make_shared<DirectManipulationServiceSharedState>();
    m_cCrossSlideContainers = 0;

    if (!static_cast<CCoreServices*>(pCoreService)->IsTSF3Enabled())
    {
        // initialize to current input language
        m_inputLang = GetKeyboardLayout(0);
    }

#ifdef DM_DEBUG
    EvaluateInfoTracingStatuses();
#endif // DM_DEBUG
}

CInputServices::~CInputServices()
{
    Reset();
}

void CInputServices::Reset()
{
    ReleaseInterface(m_pEventManager);

    ResetAllCrossSlideServices();
    m_islandInputSiteRegistrations.clear();
    IGNOREHR(DeleteDMViewports());
    IGNOREHR(DeleteDMCrossSlideViewports());
    IGNOREHR(DeleteSecondaryContentRelationshipsToBeApplied());
    IGNOREHR(DeleteDMServices());
    DeleteDMContainersNeedingInitialization();

    // Cleanup all create pointer objects
    DestroyPointerObjects();

    m_pCoreService = nullptr;
}

void CInputServices::ResetAllCrossSlideServices()
{
    for (auto& islandInputSiteRegistration : m_islandInputSiteRegistrations)
    {
        auto cpDMCrossSlideService = islandInputSiteRegistration.DMCrossSlideService();
        if (nullptr != cpDMCrossSlideService)
        {
            IFCFAILFAST(cpDMCrossSlideService->DeactivateDirectManipulationManager());
            islandInputSiteRegistration.DMCrossSlideService(nullptr);
        }
    }
}

void CInputServices::RegisterIslandInputSite(_In_ ixp::IIslandInputSitePartner* pIslandInputSite)
{
    auto iter = std::find_if(
        m_islandInputSiteRegistrations.cbegin(),
        m_islandInputSiteRegistrations.cend(),
        [&pIslandInputSite](IslandInputSiteRegistration const& entry) { return entry.Match(pIslandInputSite); });

    if (iter == m_islandInputSiteRegistrations.end())
    {
        // Not yet registered.
        m_islandInputSiteRegistrations.push_back(IslandInputSiteRegistration{ pIslandInputSite });
        if (m_shouldRegisterPrimaryDMViewportCallback)
        {
            // This will only be true if we had no IslandInputSites registered when a CUIElement tried to register a CrossSlide DManip container.
            // Transfer the pending callback to the registration entry we just added.
            m_islandInputSiteRegistrations.at(0).ShouldRegisterDMViewportCallback(true);
            m_shouldRegisterPrimaryDMViewportCallback = false;
        }
    }
}

void CInputServices::UnregisterIslandInputSite(_In_ ixp::IIslandInputSitePartner* pIslandInputSite)
{
    auto iter = std::find_if(
        m_islandInputSiteRegistrations.begin(),
        m_islandInputSiteRegistrations.end(),
        [&pIslandInputSite](IslandInputSiteRegistration const& entry) { return entry.Match(pIslandInputSite); });

    if (iter != m_islandInputSiteRegistrations.end())
    {
        // Found a match, clean up the DMCrossSlideService, if any.
        auto cpDMCrossSlideService = iter->DMCrossSlideService();
        if (nullptr != cpDMCrossSlideService)
        {
            IFCFAILFAST(cpDMCrossSlideService->DeactivateDirectManipulationManager());
            iter->DMCrossSlideService(nullptr);
        }

        // Delete the registration entry.
        m_islandInputSiteRegistrations.erase(iter);
    }
}

wrl::ComPtr<ixp::IIslandInputSitePartner> CInputServices::GetPrimaryRegisteredIslandInputSite() const
{
    if (m_islandInputSiteRegistrations.empty())
    {
        return nullptr;
    }

    // Return the oldest registered IslandInputSite with input services that is still valid for this thread.
    return m_islandInputSiteRegistrations.at(0).IslandInputSite();
}

CInputServices::IslandInputSiteRegistration& CInputServices::GetIslandInputSiteRegistrationForUIElement(CUIElement* pUIElement)
{
    auto iter = std::find_if(
        m_islandInputSiteRegistrations.begin(),
        m_islandInputSiteRegistrations.end(),
        [&pUIElement](IslandInputSiteRegistration const& entry) { return entry.Match(pUIElement); });

    // If the CUIElement is not yet on a ContentIsland, CUIElement->GetElementIslandInputSite (used in the Match function) will return the
    // "Primary" IslandInputSite as a fallback which corresponds to the "main", "first", or "oldest valid" IslandInputSite
    // registered with InputServices (i.e. the first element in m_islandInputSiteRegistrations).
    // It calls the CInputServices::GetPrimaryRegisteredIslandInputSite() just above. See depends.cpp for CDependencyObject::GetElementIslandInputSite.
    // If m_islandInputSiteRegistrations is empty, we handle this as a special case, see CInputServices::RegisterDirectManipulationCrossSlideContainer.
    //
    // This is fragile, since it does mean the CUIElement could be placed on a different ContentIsland leading to potentially
    // wonky DManip behavior. However, it does match existing behavior.
    //
    // We should *never* be in a situation where a CUIElement *has* an ElementIslandInputSite via a XamlIslandRoot that has somehow not already been registered.
    FAIL_FAST_IF(iter == m_islandInputSiteRegistrations.end());

    return *iter;
}

IPALDirectManipulationService* CInputServices::GetDMCrossSlideServiceNoRefForUIElement(_In_ CUIElement* pUIElement) const
{
    auto iter = std::find_if(
        m_islandInputSiteRegistrations.cbegin(),
        m_islandInputSiteRegistrations.cend(),
        [&pUIElement](IslandInputSiteRegistration const& entry) { return entry.Match(pUIElement); });

    if (iter != m_islandInputSiteRegistrations.cend())
    {
        return iter->DMCrossSlideService().Get();
    }

    return nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   DestroyPointerObjects
//
//  Synopsis:
//      Destroy all created pointer objects.
//
//------------------------------------------------------------------------
void
CInputServices::DestroyPointerObjects()
{
    // Destroy all interaction engines
    m_interactionManager.DestroyAllInteractionEngine();

    // Clean up the interaction map chain
    for (xchainedmap<XUINT32, CUIElement*>::const_iterator it = m_mapInteraction.begin();
        it != m_mapInteraction.end();
        ++it)
    {
        CUIElement* pInteractionElement = (*it).second;
        if (pInteractionElement)
        {
            ReleaseInterface(pInteractionElement);
        }
    }
    m_mapInteraction.Clear();

    m_mapPointerDownTracker.clear();

    // Clean up the manipulation container map chain
    m_mapManipulationContainer.Clear();

    // Clean up pointer state
    for (auto pair : m_mapPointerState)
    {
        std::shared_ptr<CPointerState> pointerState = pair.second;
        if (pointerState)
        {
            pointerState->Reset();
        }
    }
    m_mapPointerState.Clear();

    // Clean up PointerEnter state chained map
    m_mapPointerEnterFromElement.clear();
    m_mapPointerNodeDirtyFromElement.clear();

    // Clean up pointer exited state.
    for (xchainedmap<PointerExitedStateKey, CPointerExitedState*>::const_iterator it = m_mapPointerExitedState.begin();
        it != m_mapPointerExitedState.end();
        ++it)
    {
        CPointerExitedState* pPointerExitedState = (*it).second;
        if (pPointerExitedState)
        {
            IGNOREHR(pPointerExitedState->SetExitedDO(NULL));
            IGNOREHR(pPointerExitedState->SetEnteredDO(NULL));
            delete pPointerExitedState;
        }
    }
    m_mapPointerExitedState.Clear();

    for (const xref_ptr<CContentRoot>& contentRoot: m_pCoreService->GetContentRootCoordinator()->GetContentRoots())
    {
        contentRoot->GetInputManager().GetPointerInputProcessor().SetPrimaryPointerId(-1);

        // Unregister InputPane that clean InputPane handler, interaction and root SV.
        contentRoot->GetInputManager().DestroyInputPaneHandler();
    }
}

//------------------------------------------------------------------------
//
//  Method:   ObjectLeavingTree
//
//  Synopsis:
//      Currently called when the provided DO leaves the tree.
//      Handles pointer and drag drop cleanup
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ObjectLeavingTree(_In_ CDependencyObject *object)
{
    // Handle pointer state cleanup.
    IFC_RETURN(CleanPointerElementObject(object));

    // If we have an active drag, then we need to see if this "leave" invalidates
    // our dragEnterDO.  The dragEnterDO is the lowest element in the ancestor that
    // has recorded a drag enter.  If this item is leaving the tree, then we need
    // fix up the dragEnterDO.  This is complicated by the fact that it may not be
    // our dragEnterDO that is leaving the tree, but one of its ancestors.

    // So, if we need to fix up the dragEnterDO and this element was in the drag enter
    // chain and its parent is still alive, we will set the current dragEnterDO to the
    // parent.

    if (CContentRoot* contentRoot = VisualTree::GetContentRootForElement(object))
    {
        xref_ptr<CDependencyObject>& dragEnterDO = contentRoot->GetInputManager().GetDragDropProcessor().GetDragEnterDONoRef();
        if (dragEnterDO && object->HasDragEnter())
        {
            CDependencyObject* parent = object->GetParentInternal();
            if (parent)
            {
                if (parent->IsActive())
                {
                    // Our parent is still in the tree so it becomes the new dragEnterDO
                    dragEnterDO = parent;
                    ASSERT(parent->HasDragEnter());
                }
                else if (parent->OfTypeByIndex<KnownTypeIndex::RootVisual>())
                {
                    // Our parent is the root visual, so we don't need a dragEnterDO
                    // Note, RootVisual is never active so it won't get included above.
                    dragEnterDO = nullptr;
                }
            }
        }
        ASSERT(dragEnterDO != object);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CleanPointerElementObject
//
//  Synopsis:
//      Currently called when the provided DO leaves the tree.
//      Cleans cached structures used for raising raw pointer, gestures
//      manipulation events.
//      Conditionally raises PointerCaptureLost, PointerEnter and
//      PointerLeave events.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CleanPointerElementObject(_In_ CDependencyObject *pObject)
{
    std::shared_ptr<CPointerState> pointerState;
    XUINT32 pointerId = 0;
    CUIElement* pUIElement = NULL;

    for (auto it = m_mapPointerState.begin();
        it != m_mapPointerState.end();
        ++it)
    {
        pointerState = (*it).second;
        if (pointerState)
        {
            CDependencyObject*  pPointerEnterDO = NULL;
            CDependencyObject*  pPointerCaptureDO = NULL;

            pointerId = pointerState->GetPointerId();

            pPointerEnterDO = pointerState->GetEnterDO();
            pPointerCaptureDO = pointerState->GetCaptureDO();

            if (pObject == pPointerEnterDO || pObject == pPointerCaptureDO)
            {
                if (pObject == pPointerCaptureDO)
                {
                    // Do not fire the PointerCaptureLost event while processing Tick.
                    // The processing tick can clean up the elements that is collected by GC.
                    if (m_pCoreService)
                    {
                        IXcpBrowserHost *pBrowserHost = m_pCoreService->GetBrowserHost();
                        ITickableFrameScheduler *pFrameScheduler = pBrowserHost->GetFrameScheduler();

                        if (pFrameScheduler != NULL && pFrameScheduler->IsInTick() == FALSE)
                        {
                            CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pPointerCaptureDO);
                            IFC_RETURN(contentRoot->GetInputManager().GetPointerInputProcessor().ReleasePointerCapture(pPointerCaptureDO, pointerState->GetCapturePointer()));
                        }
                    }
                }

                if (pObject == pPointerEnterDO)
                {
                    IFC_RETURN(ProcessPointerExitedEventByPointerEnteredElementStateChange(pPointerEnterDO, pointerState));
                }
            }

            m_mapPointerEnterFromElement.erase(pObject);
            m_mapPointerNodeDirtyFromElement.erase(pObject);

            if (m_mapInteraction.ContainsKey(pointerId))
            {
                CUIElement *pInteractionElement = NULL;
                IFC_RETURN(m_mapInteraction.Get(pointerId, pInteractionElement));
                if (pObject == pInteractionElement)
                {
                    IFC_RETURN(m_mapInteraction.Remove(pointerId, pInteractionElement));
                    ReleaseInterface(pInteractionElement);
                }
            }

            PointerDownTrackerMap::iterator itFind = m_mapPointerDownTracker.find(pointerId);
            if (itFind != m_mapPointerDownTracker.end())
            {
                CUIElement *pTrackedElement = itFind->second;
                if (pObject == pTrackedElement)
                {
                    m_mapPointerDownTracker.erase(itFind);
                }
            }

            // Do not remove pointerState from m_mapPointerState that will be removed when
            // the specified pointer Id is completed by XCP_POINTERLEAVE, XCP_POINTERCAPTURECHANGED or XCP_POINTERSUSPENDED.

            pointerId = 0;
            pointerState = nullptr;
        }
    }

    pUIElement = do_pointer_cast<CUIElement>(pObject);
    if (pUIElement)
    {
        // Remove the manipulation container that is associated with the leaving element.
        if (m_mapManipulationContainer.ContainsKey(pUIElement))
        {
            CUIElement *pManipulationContainer = NULL;
            IFC_RETURN(m_mapManipulationContainer.Remove(pUIElement, pManipulationContainer));
        }

        // Remove the potential interaction engine associated with the leaving element
        // so that it is no longer pegged.
        m_interactionManager.DestroyInteractionEngine(pUIElement);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessPointerExitedEventByPointerEnteredElementStateChange
//
//  Synopsis:
//      Process PointerExited event when the pointer entered element is leaving
//      the tree, disabled or collapsed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessPointerExitedEventByPointerEnteredElementStateChange(
    _In_ CDependencyObject* pElementDO,
    _In_opt_ std::shared_ptr<CPointerState> pointerState)
{
    HRESULT hr = S_OK;
    const CRootVisual *pRootVisual = NULL;
    CDependencyObject*  pElementRoot = NULL;
    CDependencyObject*  pElementParent = NULL;
    CDependencyObject*  pPointerEnteredDO = NULL;
    CDependencyObject*  pTemplatedParent = NULL;
    CDependencyObject*  pNewPointerEnteredDO = NULL;
    CFrameworkElement*  pPointerEnteredFE = NULL;
    CUIElement*  pPointerEnteredUIE = NULL;
    CUIElement*  pNewPointerEnteredUIE = NULL;
    CUIElement*  pElementUIE = NULL;
    CPointerExitedState* pPointerExitedState = NULL;
    CPointer *pPointer = NULL;
    CPointerEventArgs *pPointerArgs = NULL;
    XUINT32 pointerId = 0;
    XUINT32 modifierKeys = 0;
    bool bIsAncestor = false;
    bool bFoundPointerState = pointerState != nullptr;
    CREATEPARAMETERS cp(m_pCoreService);
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pElementDO);

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElementDO);
    IFCPTR(m_pEventManager);

    pElementParent = pElementDO;
    pRootVisual = VisualTree::GetRootForElement(pElementDO);
    if (pRootVisual)
    {
        while (pElementParent && pElementParent != pRootVisual)
        {
            pElementRoot = pElementParent;
            pElementParent = pElementParent->GetParentInternal();
        }
    }

    // Do not process if it is in the middle of reset visual tree.
    if (m_pCoreService->IsInResetVisualTree() || !pElementRoot || !pElementRoot->IsActive() || pElementRoot->IsProcessingEnterLeave())
    {
        goto Cleanup;
    }

    // Get the current pointer state that hold the pointer entered element.
    if (pointerState == nullptr)
    {
        for (auto it = m_mapPointerState.begin();
            it != m_mapPointerState.end();
            ++it)
        {
            pointerState = (*it).second;
            if (pointerState)
            {
                bIsAncestor = false;
                pointerId = pointerState->GetPointerId();
                pPointerEnteredDO = pointerState->GetEnterDO();

                if (pPointerEnteredDO)
                {
                    pPointerEnteredUIE = do_pointer_cast<CUIElement>(pPointerEnteredDO);
                    pElementUIE = do_pointer_cast<CUIElement>(pElementDO);

                    if (pPointerEnteredUIE && pElementUIE)
                    {
                        bIsAncestor = pElementUIE->IsAncestorOf(pPointerEnteredUIE);
                    }
                    if (pElementDO == pPointerEnteredDO || bIsAncestor)
                    {
                        bFoundPointerState = TRUE;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        pointerId = pointerState->GetPointerId();
        pPointerEnteredDO = pointerState->GetEnterDO();
    }

    if (bFoundPointerState)
    {
        ASSERT(pPointerEnteredDO && pointerState && pointerId);

        // Note: If at some future point lifted input can come from a non-UI thread, we'll need an actual check here.
        const bool onCorrectThread = true;

        bool isPointerInfoValid = false;
        PointerInfo pointerInfo = {};
        wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
        if (onCorrectThread)
        {
            CXamlIslandRoot* pIslandRoot = contentRoot->GetXamlIslandRootNoRef();
            if (pIslandRoot)
            {
                pointerPoint = pIslandRoot->GetPreviousPointerPoint();
            }
            else
            {
                CJupiterWindow* jupiterWindow = DirectUI::DXamlServices::GetCurrentJupiterWindow();
                pointerPoint = jupiterWindow->GetInputSiteAdapterPointerPoint();
            }

            if (pointerPoint)
            {
                isPointerInfoValid = SUCCEEDED(GetPointerInfoFromPointerPoint(pointerPoint.Get(), &pointerInfo));
            }
        }

        // Get the current pointer information and fire PointerExited event.

        // Get the new entered element to stop the bubbling PointerExit.
        pPointerEnteredFE = do_pointer_cast<CFrameworkElement>(pPointerEnteredDO);
        if (pPointerEnteredFE)
        {
            pTemplatedParent = pPointerEnteredFE->GetTemplatedParent();

            while (pTemplatedParent)
            {
                pPointerEnteredFE = do_pointer_cast<CFrameworkElement>(pTemplatedParent);
                if (pPointerEnteredFE && pPointerEnteredFE->GetTemplatedParent())
                {
                    pTemplatedParent = pPointerEnteredFE->GetTemplatedParent();
                }
                else
                {
                    break;
                }
            }

            if (pTemplatedParent)
            {
                pNewPointerEnteredDO = pTemplatedParent->GetParentInternal();
            }
        }

        // Set the new entered element as the current entered element's parent.
        if (!pNewPointerEnteredDO)
        {
            pNewPointerEnteredDO = pPointerEnteredDO->GetParentInternal();
        }

        if (pNewPointerEnteredDO)
        {
            // Ensure the new entered element is enabled and hit-test visible.
            pNewPointerEnteredUIE = do_pointer_cast<CUIElement>(pNewPointerEnteredDO);
            while (pNewPointerEnteredUIE)
            {
                if (!(pNewPointerEnteredUIE->IsHitTestVisible()) ||
                    !(pNewPointerEnteredUIE->IsEnabled()))
                {
                    pNewPointerEnteredDO = static_cast<CDependencyObject*>(pNewPointerEnteredDO)->GetParentInternal();
                    pNewPointerEnteredUIE = do_pointer_cast<CUIElement>(pNewPointerEnteredDO);
                }
                else
                {
                    break;
                }
            }
        }

        if (isPointerInfoValid && pointerInfo.m_pointerInputType == XcpPointerInputTypeMouse && !contentRoot->GetInputManager().GetPointerInputProcessor().IsProcessingPointerInput())
        {
            // If the entered element is changing the visual state that used with the mouse pointer input device and
            // it is not the input stack call, we need to process PointerExited now asynchronously.
            //
            // Raise PointerExited event on the pointer entered element that leave the tree, visibility collapsed or disabled.
            // Raise PointerEntered event if the pointer is positioned to the new contact element. For example, collapse of
            // the original pointer entered element.

            // Create the pointer event arg.
            pPointerArgs = new CPointerEventArgs(pCoreService);

            pPointerArgs->SetGlobalPoint(pointerState->GetLastPosition());

            // Set the original source element
            IFC(pPointerArgs->put_Source(pPointerEnteredDO));

            // Set the Pointer object
            IFC(CPointer::Create((CDependencyObject**)&pPointer, &cp));
            IFC(pPointer->SetPointerFromPointerInfo(pointerInfo));
            pPointerArgs->m_pPointer = pPointer;
            pPointerArgs->m_pPointerPoint = pointerPoint.Get();

            pPointer = NULL;

            // Get the current key modifiers and set to PointerArgs.
            IFC(gps->GetKeyboardModifiersState(&modifierKeys));
            IFC(ContentRootInput::PointerInputProcessor::SetPointerKeyModifiers(modifierKeys, pPointerArgs));

            if (pNewPointerEnteredDO)
            {
                IFC(contentRoot->GetInputManager().GetPointerInputProcessor().ProcessPointerEnterLeave(
                    pNewPointerEnteredDO,
                    pPointerEnteredDO,
                    pointerId,
                    pPointerArgs,
                    FALSE /* bSkipLeave */,
                    FALSE /* bForceRaisePointerEntered */,
                    TRUE /* bIgnoreHitTestVisibleForPointerExited */,
                    TRUE /*bAsyncEvent*/));
            }
            else
            {
                IFC(contentRoot->GetInputManager().GetPointerInputProcessor().ProcessPointerLeave(pPointerEnteredDO, pointerId, pPointerArgs, TRUE /*bAsyncEvent*/));
            }
        }
        else
        {
            // Save the pointer exited state information to process PointerExited event
            // on the next WM_POINTERXXX input stack.
            PointerExitedStateKey key = { pointerId, pPointerEnteredDO };
            if (m_mapPointerExitedState.ContainsKey(key))
            {
                IFC(m_mapPointerExitedState.Get(key, pPointerExitedState));
            }
            else
            {
                pPointerExitedState = new CPointerExitedState(pointerId);
                IFC(m_mapPointerExitedState.Add(key, pPointerExitedState));
            }

            if (pPointerExitedState->GetExitedDONoRef() == NULL)
            {
                IFC(pPointerExitedState->SetExitedDO(pPointerEnteredDO));
            }

            if (pNewPointerEnteredDO && pPointerExitedState->GetEnteredDONoRef() == NULL)
            {
                IFC(pPointerExitedState->SetEnteredDO(static_cast<CDependencyObject*>(pNewPointerEnteredDO)));
            }

            // In case of having pNewPointerEnteredDO, we need to ensure the PointerExited firing element's
            // managed object life time by calling PegManagedPeer() while processing PointerExited event.
            // If pNewPointerEnteredDO is null, we will only fire PointerExited event once to the current
            // pointer exited DO so we don't need to call PegManagedPeer().
            // The pointer exited DO doesn't call PegManagedPeer() here since it is already pegged by
            // calling SetEnteredDO() above.
            if (pNewPointerEnteredDO)
            {
                auto peggedPointerExitedDOs = pPointerExitedState->GetPeggedPointerExitedDOs();
                pElementParent = pPointerEnteredDO;
                while (pElementParent)
                {
                    IFC(pElementParent->PegManagedPeer(TRUE /* isShutdownException */));
                    peggedPointerExitedDOs->push_back(xref::get_weakref(pElementParent));
                    pElementParent = pElementParent->GetParentInternal();
                    if (pElementParent == pNewPointerEnteredDO)
                    {
                        break;
                    }
                }
            }
        }

        // Set the pointer entered element.
        IFC(pointerState->SetEnterDO(pNewPointerEnteredDO ? static_cast<CDependencyObject*>(pNewPointerEnteredDO) : NULL));
    }

Cleanup:
    ReleaseInterface(pPointer);
    ReleaseInterface(pPointerArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifyWindowDestroyed
//
//  Synopsis:
//      Update the destroyed window handle on PointerState.
//
//------------------------------------------------------------------------
void
CInputServices::NotifyWindowDestroyed(
    _In_ XHANDLE hDestroyedWindow)
{
    for (auto it = m_mapPointerState.begin();
        it != m_mapPointerState.end();
        ++it)
    {
        std::shared_ptr<CPointerState> pointerState = (*it).second;
        if (pointerState && pointerState->GetWindowHandle() == hDestroyedWindow)
        {
            pointerState->SetWindowHandle(NULL);
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   DestroyInteractionEngine
//
//  Synopsis:
//      Destroy the specified interaction engine on the element.
//
//------------------------------------------------------------------------
void
CInputServices::DestroyInteractionEngine(
    _In_ CUIElement* pDestroyElement)
{
   m_interactionManager.DestroyInteractionEngine(pDestroyElement);
}

XUINT32 CInputServices::AddRef()
{
    return ++m_ref;
}

XUINT32 CInputServices::Release()
{
    int ref = --m_ref;

    if (ref == 0)
    {
        delete this;
    }

    return ref;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessInput
//
//  Synopsis:
//      This is what handles the actual input.
//------------------------------------------------------------------------
_Check_return_
HRESULT CInputServices::ProcessInput(_In_ InputMessage *pMsg, _In_ CContentRoot* contentRoot, _Out_ XINT32 *handled)
{
    //validate pointers
    IFCPTR_RETURN(pMsg);
    IFCPTR_RETURN(handled);

    // Initialize handled
    *handled = FALSE;
    bool shouldPlayInteractionSound = false;

    switch (pMsg->m_msgID)
    {
    case XCP_DEACTIVATE:
        // If we lose activation, then we need to reset the m_fKeyDownHandled flag
        // so that if we receive a XCP_CHAR it isn't ignored
        contentRoot->GetInputManager().SetKeyDownHandled(false);
        contentRoot->GetInputManager().SetNoCandidateDirectionPerTick(FocusNavigationDirection::None);
        __fallthrough;
    case XCP_ACTIVATE:
        {
            const bool shiftPressed = ((pMsg->m_modifierKeys & KEY_MODIFIER_SHIFT) != 0);
            const bool isActivating = (pMsg->m_msgID == XCP_ACTIVATE);
            IFC_RETURN(contentRoot->GetInputManager().ProcessWindowActivation(shiftPressed, isActivating));
        }
        break;
    case XCP_POINTERDOWN:
    case XCP_POINTERUPDATE:
    case XCP_POINTERUP:
    case XCP_POINTERENTER:
    case XCP_POINTERLEAVE:
    case XCP_POINTERWHEELCHANGED:
    case XCP_POINTERCAPTURECHANGED:
    case XCP_POINTERSUSPENDED:
        contentRoot->GetInputManager().SetShouldAllRequestFocusSound(true);
        IFC_RETURN(contentRoot->GetInputManager().GetPointerInputProcessor().ProcessPointerInput(pMsg, handled));
        contentRoot->GetInputManager().SetShouldAllRequestFocusSound(false);
        shouldPlayInteractionSound = true;
        break;
    case XCP_DMPOINTERHITTEST:
        IFC_RETURN(ProcessDirectManipulationPointerHitTest(pMsg, contentRoot, handled));
        break;
    case XCP_KEYUP:
    case XCP_KEYDOWN:
    case XCP_CHAR:
    case XCP_DEADCHAR:
    {
        bool bHandled = false;
        IFC_RETURN(contentRoot->GetInputManager().ProcessKeyboardInput(
            pMsg->m_platformKeyCode,
            pMsg->m_physicalKeyStatus,
            pMsg->m_msgID,
            nullptr /* deviceId */,
            pMsg->m_bIsSecondaryMessage,
            pMsg->m_hPlatformPacket,
            &bHandled));
        *handled = bHandled;
        break;
    }
    case XCP_GOTFOCUS:
    case XCP_LOSTFOCUS:
        IFC_RETURN(contentRoot->GetInputManager().ProcessFocusInput(pMsg, handled));
        break;
    case XCP_CONTEXTMENU:
    {
        bool bHandled = false;
        IFC_RETURN(contentRoot->GetInputManager().RaiseRightTappedEventFromContextMenu(&bHandled));
        *handled = bHandled;
        break;
    }
    case XCP_INPUTLANGCHANGE:
        IFC_RETURN(ProcessInputLanguageChange(pMsg, contentRoot, handled));
        break;
    case XCP_WINDOWMOVE:
        IFC_RETURN(ProcessWindowMove(pMsg, contentRoot));
        break;
    case XCP_NULL:
    default:
        ASSERT(FALSE);
        break;
    }

    if (shouldPlayInteractionSound)
    {
        // Play the interaction sound if there is a requested sound during processing input
        IFC_RETURN(FxCallbacks::ElementSoundPlayerService_PlayInteractionSound());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreatePointerCaptureLostEventArgs
//
//  Synopsis:
//  Create the argument we will be sending to listeners of the
//  PointerCaptureLost event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CreatePointerCaptureLostEventArgs(
    _In_ CDependencyObject *pSenderObject,
    _In_ XPOINTF pointLast,
    _In_ CPointer* pPointer,
    _Out_ CPointerEventArgs **ppPointerEventArgs)
{
    HRESULT hr = S_OK;
    CPointerEventArgs*  pArgs = NULL;

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pSenderObject);
    IFCPTR(pPointer);
    IFCPTR(ppPointerEventArgs);
    *ppPointerEventArgs = NULL;

    pArgs = new CPointerEventArgs(pCoreService);

    pArgs->SetGlobalPoint(pointLast);

    // Set the source
    IFC(pArgs->put_Source(pSenderObject));

    // Set Pointer object
    pArgs->m_pPointer = pPointer;
    pPointer->AddRef();

    *ppPointerEventArgs = pArgs;
    pArgs = NULL;

Cleanup:
    ReleaseInterface(pArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   SetCursor
//
//  Synopsis:
//      If the InputCursor is null, create an InputCursor
//      with a specific MouseCursor. If InputCursor is not null,
//      update the XamlIslandRoot cursor with InputCursor.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetCursor(MouseCursor eMouseCursor,
                          _In_opt_ mui::IInputCursor* inputCursor,
                          _In_ wrl::ComPtr<mui::IInputPointerSource> inputPointerSource)
{
    if (eMouseCursor == MouseCursorUnset)
    {
        // Windows doesn't need to do anything in this case
        return S_OK;
    }

    wrl::ComPtr<mui::IInputSystemCursor> pCursor;
    if (!inputCursor)
    {
        if (!m_inputSystemCursorStatics)
        {
            m_inputSystemCursorStatics = ActivationFactoryCache::GetActivationFactoryCache()->GetInputSystemCursorStatics();
        }

        switch (eMouseCursor)
        {
        case MouseCursorArrow:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_Arrow, &pCursor);
            break;
        case MouseCursorHand:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_Hand, &pCursor);
            break;
        case MouseCursorWait:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_Wait, &pCursor);
            break;
        case MouseCursorIBeam:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_IBeam, &pCursor);
            break;
        case MouseCursorSizeNS:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_SizeNorthSouth, &pCursor);
            break;
        case MouseCursorSizeWE:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_SizeWestEast, &pCursor);
            break;
        case MouseCursorSizeNESW:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_SizeNortheastSouthwest, &pCursor);
            break;
        case MouseCursorSizeNWSE:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_SizeNorthwestSoutheast, &pCursor);
            break;
        case MouseCursorPin:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_Pin, &pCursor);
            break;
        case MouseCursorPerson:
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_Person, &pCursor);
            break;
        case MouseCursorNone:
            pCursor = nullptr;
            break;
        case MouseCursorDefault:
            // fall back to IDC_ARROW as the default cursor
            m_inputSystemCursorStatics->Create(mui::InputSystemCursorShape_Arrow, &pCursor);
            break;
        }
    }

    wrl::ComPtr<mui::IInputCursor> inputCursorToSet;
    if (inputCursor)
    {
        inputCursorToSet = inputCursor;
    }
    else
    {
        pCursor.As(&inputCursorToSet);
    }

    inputPointerSource->put_Cursor(inputCursorToSet.Get());

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   UpdateCursor
//
//  Synopsis:
//      Set the cursor to whatever would be appropriate given the element
//      that the mouse is over, or the element that has captured the mouse
//      taking into account whether the element has inherited a Cursor value
//      from an ancestor.
//------------------------------------------------------------------------

_Check_return_ HRESULT
CInputServices::UpdateCursor(_In_ CDependencyObject* pVisualInTargetIsland, _In_ XINT32 bUnset)
{
    MouseCursor eNewMouseCursor = MouseCursorDefault;
    wrl::ComPtr<mui::IInputPointerSource> pInputPointerSource = nullptr;
    CDependencyObject *pVisual = NULL;
    auto lastDeviceType = (m_lastInputDeviceType != XcpPointerInputTypePen) ? XcpPointerInputTypeMouse : XcpPointerInputTypePen;

    for (auto it = m_mapPointerState.begin();
        it != m_mapPointerState.end();
        ++it)
    {
        std::shared_ptr<CPointerState>& pointerState = it->second;
        if (pointerState->GetPointerInputType() == lastDeviceType)
        {
            pVisual = pointerState->m_pPointerCaptureDO ? pointerState->m_pPointerCaptureDO : pointerState->m_pPointerEnterDO;
            break;
        }
    }

    if (m_pCoreService->GetInitializationType() == InitializationType::IslandsOnly && pVisualInTargetIsland)
    {
        if (CXamlIslandRoot* pIslandRoot = m_pVisualTree->GetXamlIslandRootForElement(pVisualInTargetIsland))
        {
            pInputPointerSource = pIslandRoot->GetInputPointerSource();
        }
    }

    // If XamlIslandRoot does not exist, set InputPointerSource.Cursor on JupiterWindow's InputSiteAdapter
    // TODO: It is unclear if this plays well with app
    // setting their own cursor. This task tracks this:
    // https://microsoft.visualstudio.com/OS/_workitems/edit/28797081
    else if (m_pCoreService->GetInitializationType() != InitializationType::IslandsOnly)
    {
        auto jupiterWindow = DirectUI::DXamlServices::GetCurrentJupiterWindow();
        pInputPointerSource = jupiterWindow->GetInputSiteAdapterInputPointerSource();
    }

    // If pInputPointerSource is nullptr and there is no exit,
    // we will hit an access violation due to pInputPointerSource being null in SetCursor()
    if (pInputPointerSource == nullptr)
    {
        return E_INVALIDARG;
    }

    if (bUnset)
    {
        // Unset any cursor-related state
        IFC_RETURN(SetCursor(MouseCursorUnset, nullptr, pInputPointerSource));
        return S_OK;
    }

    wrl::ComPtr<mui::IInputCursor> newProtectedCursor = nullptr;
    while (pVisual)
    {
        if (eNewMouseCursor == MouseCursorDefault
            && pVisual->OfTypeByIndex<KnownTypeIndex::FrameworkElement>()
            && static_cast<CUIElement*>(pVisual)->IsHitTestVisible())
        {
            // If ProtectedCursor is set on the current element, we stop the
            // walk and update the cursor with ProtectedCursor
            if (static_cast<CUIElement*>(pVisual)->IsProtectedCursorSet())
            {
                newProtectedCursor = static_cast<CUIElement*>(pVisual)->GetProtectedCursor();
                break;
            }
            else
            {
                eNewMouseCursor = (static_cast<CFrameworkElement*>(pVisual))->m_eMouseCursor;
            }
        }

        pVisual = pVisual->GetParentInternal();
    }

    IFC_RETURN(SetCursor(eNewMouseCursor, newProtectedCursor.Get(), pInputPointerSource));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationPointerHitTest
//
//  Synopsis:
//      Processes DM_POINTERHITTEST message. Does a hit-test and
//      calls DManip's SetContact on hit-tested viewports.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationPointerHitTest(
    _In_ InputMessage *pMsg,
    _In_ CContentRoot* contentRoot,
    _Out_ XINT32 *pfHandled)
{
    HRESULT hr = S_OK;
    CDependencyObject *pDOContact = NULL;
    CUIElement *pContactElement = NULL;
    CCoreServices *pCoreService = m_pCoreService;

    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pMsg);
    IFCPTR(pfHandled);
    *pfHandled = FALSE;

    ASSERT(pMsg->m_msgID == XCP_DMPOINTERHITTEST);

    TraceProcessPointerInputBegin(pMsg->m_pointerInfo.m_pointerId, static_cast<XUINT32>(pMsg->m_msgID), pMsg->m_pointerInfo.m_pointerLocation.x, pMsg->m_pointerInfo.m_pointerLocation.y);

    // Do the hit-test
    IFC(contentRoot->GetInputManager().GetPointerInputProcessor().HitTestHelper(pMsg->m_pointerInfo.m_pointerLocation, contentRoot->GetXamlIslandRootNoRef(), &pDOContact));

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessDirectManipulationPointerHitTest - Hit-tested pDOContact=0x%p, pointerId=%d.",
            this, pDOContact, pMsg->m_pointerInfo.m_pointerId));
    }
#endif // DM_DEBUG

    if (!pDOContact)
    {
        // No hit-testable element was found on the walk up. Set the current contact to the visual root.
        pDOContact = contentRoot->GetVisualTreeNoRef()->GetPublicRootVisual();

        if (pDOContact)
        {
            pDOContact->AddRef();
        }
        else
        {
            goto Cleanup;
        }
    }

    ASSERT(pDOContact);

    pContactElement = do_pointer_cast<CUIElement>(pDOContact);
    if (pContactElement)
    {
        bool unused;
        IFC(InitializeDirectManipulationForPointerId(pMsg->m_pointerInfo.m_pointerId, TRUE /*fIsForDMHitTest*/, pContactElement, &unused /*pContactSuccess*/));
    }

    *pfHandled = TRUE;

Cleanup:
    ReleaseInterface(pCoreService);
    ReleaseInterface(pDOContact);

    TraceProcessPointerInputEnd();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RegisterDMViewportsOnPointerDown
//
//  Synopsis:
//      Process Pointer Down message
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RegisterDMViewportsOnPointerDown(
    _In_ XUINT32 pointerId,
    _In_ bool fIsForDMHitTest,
    _In_ CUIElement* pPointedElement)
{
    // Create DM viewports walking up the parent chain, looking for a CUIDMContainer.
    bool unused;
    RRETURN(InitializeDirectManipulationForPointerId(pointerId, fIsForDMHitTest, pPointedElement, &unused /*pContactSuccess*/));
}

//------------------------------------------------------------------------
//
//  Method:   UnRegisterDMViewportsOnPointerUp
//
//  Synopsis:
//      Process Pointer Up message
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UnRegisterDMViewportsOnPointerUp(
    _In_ XUINT32 pointerId)
{
    // Unregister DM viewports that might be associated with this pointer Id
    IFC_RETURN(UnregisterContactId(pointerId, (CDMViewport*)NULL /*pExclusiveViewport*/, TRUE /*fCompleteManipulation*/));
    IFC_RETURN(UnregisterCrossSlideViewportContactId(pointerId));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   HasPointerEnter
//
//  Synopsis:
//      TRUE if pointer input is entered into the specified element
//------------------------------------------------------------------------
_Check_return_ bool
CInputServices::HasPointerEnter(
    _In_ CDependencyObject* pElement,
    _In_ UINT32 uiPointerId) const
{
    auto iterElement = m_mapPointerEnterFromElement.find(pElement);
    if (iterElement != m_mapPointerEnterFromElement.end())
    {
        auto iterPointer = iterElement->second.find(uiPointerId);
        if (iterPointer != iterElement->second.end())
        {
            return iterPointer->second;
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   SetPointerEnter
//
//  Synopsis:
//      Specify the pointer enter state(TRUE/FALSE) on the specified element
//------------------------------------------------------------------------
void CInputServices::SetPointerEnter(
    _In_ CDependencyObject* pElement,
    _In_ UINT32 uiPointerId,
    _In_ bool bValue)
{
    m_mapPointerEnterFromElement[pElement][uiPointerId] = bValue;
}

//------------------------------------------------------------------------
//
//  Method:   RemovePointerEnter
//
//  Synopsis:
//      Removed pointer enter state from the specified element
//------------------------------------------------------------------------
void CInputServices::RemovePointerEnter(
    _In_ CDependencyObject* pElement,
    _In_ UINT32 uiPointerId)
{
    auto iterElement = m_mapPointerEnterFromElement.find(pElement);
    if (iterElement != m_mapPointerEnterFromElement.end())
    {
        iterElement->second.erase(uiPointerId);
    }
}

//------------------------------------------------------------------------
//
//  Method:   IsInputPointerNodeDirty
//
//  Synopsis:
//      Return the pointer input node state from the specified element
//------------------------------------------------------------------------
_Check_return_ bool
CInputServices::IsInputPointerNodeDirty(
    _In_ CDependencyObject* pElement,
    _In_ XUINT32 uiPointerId) const
{
    auto iterElement = m_mapPointerNodeDirtyFromElement.find(pElement);
    if (iterElement != m_mapPointerNodeDirtyFromElement.end())
    {
        auto iterPointer = iterElement->second.find(uiPointerId);
        if (iterPointer != iterElement->second.end())
        {
            return iterPointer->second;
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   SetInputPointerNodeDirty
//
//  Synopsis:
//      Specify pointer input node state(TRUE/FALSE) on the specified element
//------------------------------------------------------------------------
void CInputServices::SetInputPointerNodeDirty(
    _In_ CDependencyObject* pElement,
    _In_ UINT32 uiPointerId,
    _In_ bool bValue)
{
    m_mapPointerNodeDirtyFromElement[pElement][uiPointerId] = bValue;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveInputPointerNodeDirty
//
//  Synopsis:
//      Removed the pointer enter state from the specified element
//------------------------------------------------------------------------
void CInputServices::RemoveInputPointerNodeDirty(
    _In_ CDependencyObject* pElement,
    _In_ UINT32 uiPointerId)
{
    auto iterElement = m_mapPointerNodeDirtyFromElement.find(pElement);
    if (iterElement != m_mapPointerNodeDirtyFromElement.end())
    {
        iterElement->second.erase(uiPointerId);
    }
}

//------------------------------------------------------------------------
//
//  Method:   ConvertPixelsToDips
//
//  Synopsis:
//      Convert the physical pixels to DIPs
//------------------------------------------------------------------------
XFLOAT CInputServices::ConvertPixelsToDips(_In_ float scale, _In_ float pixelValue)
{
    ASSERT(scale > 0);
    return pixelValue / scale;
}

XPOINTF CInputServices::ConvertPixelsToDips(_In_ float scale, _In_ XPOINTF pixelValue)
{
    ASSERT(scale > 0);
    return XPOINTF
    {
        pixelValue.x / scale,
        pixelValue.y / scale
    };
}

//------------------------------------------------------------------------
//
//  Method:   ConvertDipsToPixels
//
//  Synopsis:
//      Convert the Device Independent Pixels to the physical pixels
//------------------------------------------------------------------------
XFLOAT CInputServices::ConvertDipsToPixels(_In_ float scale, _In_ float dipsValue)
{
    ASSERT(scale > 0);
    return dipsValue * scale;
}

//------------------------------------------------------------------------
//
//  Method:   ConvertTransformPointToLocal
//
//  Synopsis:
//      Convert the transform point from the global to the local point.
//------------------------------------------------------------------------
_Check_return_ HRESULT CInputServices::ConvertTransformPointToLocal(
    _In_ CUIElement *pUIElement,
    _Inout_ XPOINTF *ppt)
{
    HRESULT hr = S_OK;
    XPOINTF pt = *ppt;
    ITransformer* pTransformer = NULL;

    IFCPTR(pUIElement);

    // Get the transform from the root visual to the element
    IFC(pUIElement->TransformToRoot(&pTransformer));

    // Transform the point
    IFC(pTransformer->ReverseTransform(&pt, &pt, 1));

    ASSERT(ppt);
    *ppt = pt;

Cleanup:
    ReleaseInterface(pTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ConvertTransformPointToGlobal
//
//  Synopsis:
//      Convert the transform point from the local to the global point.
//------------------------------------------------------------------------
_Check_return_ HRESULT CInputServices::ConvertTransformPointToGlobal(
    _In_ CUIElement *pUIElement,
    _Inout_ XPOINTF *ppt)
{
    HRESULT hr = S_OK;
    XPOINTF pt = *ppt;
    ITransformer* pTransformer = NULL;

    IFCPTR(pUIElement);

    // Get the transform from the root visual to the element
    IFC(pUIElement->TransformToRoot(&pTransformer));

    // Transform the point
    IFC(pTransformer->Transform(&pt, &pt, 1));

    ASSERT(ppt);
    *ppt = pt;

Cleanup:
    ReleaseInterface(pTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   IsInteractionSupported
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::IsInteractionSupported(
    _In_ CUIElement *pContactElement,
    _In_ CUIElement **ppInteractionElement,
    _Out_ bool *pbInteractionSupported)
{
    IFCPTR_RETURN(pContactElement);
    IFCPTR_RETURN(ppInteractionElement);
    IFCPTR_RETURN(pbInteractionSupported);

    *pbInteractionSupported = FALSE;

    while (pContactElement)
    {
        //  Pick the first element encountered interested in any gesture, or has a ManipulationMode property different from ManipulationModes.System.
        if (pContactElement->IsTapEnabled() ||
            pContactElement->IsDoubleTapEnabled() ||
            pContactElement->IsRightTapEnabled() ||
            pContactElement->IsHoldEnabled() ||
            pContactElement->GetManipulationMode() != DirectUI::ManipulationModes::System)
        {
            *pbInteractionSupported = TRUE;
            *ppInteractionElement = pContactElement;
            break;
        }
        pContactElement = static_cast<CUIElement*>(pContactElement->GetParentInternal());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetManipulationElement
//
//  Synopsis: Get the manipulation element that enables manipulation mode
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetManipulationElement(
    _In_ CUIElement* pContactElement,
    _Out_ CUIElement** ppManipulationElement)
{
    IFCPTR_RETURN(pContactElement);
    IFCPTR_RETURN(ppManipulationElement);

    *ppManipulationElement = NULL;

    while (pContactElement)
    {
        // Pick the first element encountered with a ManipulationMode property different from ManipulationModes.System.
        if (pContactElement->GetManipulationMode() != DirectUI::ManipulationModes::System)
        {
            *ppManipulationElement = pContactElement;
            break;
        }
        pContactElement = static_cast<CUIElement*>(pContactElement->GetParentInternal());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   AddPointerIdToInteractionElement
//
//  Synopsis:
//      Add the pointer id and interaction element to the map chain.
//      Pointer message will be captured to the associated interaction
//      element until leave the contact.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::AddPointerIdToInteractionElement(
    _In_ XUINT32 pointerId,
    _In_ CUIElement  *pContactElement)
{
    bool bInteractionSupported = false;
    CUIElement *pInteractionElement = NULL;

    IFCPTR_RETURN(pContactElement);

    IFC_RETURN(IsInteractionSupported(pContactElement, &pInteractionElement, &bInteractionSupported));

    if (bInteractionSupported)
    {
        IFCPTR_RETURN(pInteractionElement);
        if (!m_mapInteraction.ContainsKey(pointerId))
        {
            IFC_RETURN(m_mapInteraction.Add(pointerId, pInteractionElement));
            pInteractionElement->AddRef();

#ifdef POINTER_TRACING
            gps->DebugOutputSzNoEndl(L"[InputPointer:IM]: Interaction: Added PointerId to the interaction element! \r\n");
#endif // POINTER_TRACING
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RemovePointerIdFromInteractionElement
//
//  Synopsis:
//      Remove the pointer id and interaction element from the map chain.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RemovePointerIdFromInteractionElement(
    _In_ XUINT32 pointerId)
{
    if (m_mapInteraction.ContainsKey(pointerId))
    {
        CUIElement *pInteractionElement = NULL;

        IFC_RETURN(m_mapInteraction.Get(pointerId, pInteractionElement));
        IFC_RETURN(m_mapInteraction.Remove(pointerId, pInteractionElement));

#ifdef POINTER_TRACING
        gps->DebugOutputSzNoEndl(L"[InputPointer:IM]: Interaction: XcpMsgLeave PointerId=%d is Removed Element=0x%p \r\n", pointerId, pInteractionElement);
#endif // POINTER_TRACING

        ReleaseInterface(pInteractionElement);
    }

    return S_OK;
}

// Add a mapping from pointerId -> CUIElement to begin tracking after it was hit tested in WM_POINTERDOWN
_Check_return_ HRESULT
CInputServices::AddEntryToPointerDownTrackerMap(
    _In_ UINT32 pointerId,
    _In_ CUIElement* pElement)
{
    HRESULT hr = S_OK;

    if (m_mapPointerDownTracker.find(pointerId) == m_mapPointerDownTracker.end())
    {
        xref_ptr<CUIElement> spElement(pElement);
        m_mapPointerDownTracker.insert(std::make_pair(pointerId, spElement));

#ifdef POINTER_TRACING
        gps->DebugOutputSzNoEndl(L"[InputPointer:IM]: Interaction: Added entry to PointerDownTrackerMap, pointerId=%d, Element=0x%p \r\n", pointerId, pElement);
#endif // POINTER_TRACING

    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Remove the mapping from pointerId -> CUIElement to end tracking this interaction that started in WM_POINTERDOWN
_Check_return_ HRESULT
CInputServices::RemoveEntryFromPointerDownTrackerMap(
    _In_ UINT32 pointerId)
{
    HRESULT hr = S_OK;

    PointerDownTrackerMap::iterator itFind = m_mapPointerDownTracker.find(pointerId);
    if (itFind != m_mapPointerDownTracker.end())
    {
#ifdef POINTER_TRACING
        CUIElement* pTrackedElement = itFind->second;
        gps->DebugOutputSzNoEndl(L"[InputPointer:IM]: Interaction: Removing entry from PointerDownTrackerMap, pointerId=%d, Element=0x%p \r\n", pointerId, pTrackedElement);
#endif // POINTER_TRACING

        m_mapPointerDownTracker.erase(itFind);
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   CleanPointerProcessingState
//
//  Synopsis:
//      Clean up the pointer processing state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CleanPointerProcessingState(
    _In_ CContentRoot* contentRoot,
    _In_ InputMessage *pMsg,
    _In_ XUINT32 pointerId,
    _In_ std::shared_ptr<CPointerState> pointerState,
    _In_opt_ CDependencyObject* pDOContact)
{
    HRESULT hr = S_OK;
    ContentRootInput::PointerInputProcessor& pointerInputProcessor = contentRoot->GetInputManager().GetPointerInputProcessor();

    if (pMsg->m_msgID == XCP_POINTERLEAVE || pMsg->m_msgID == XCP_POINTERCAPTURECHANGED || pMsg->m_msgID == XCP_POINTERSUSPENDED)
    {
        if (pointerInputProcessor.GetPrimaryPointerId() == pointerId)
        {
            pointerInputProcessor.SetPrimaryPointerId(-1);
        }
        if (m_mapPointerState.ContainsKey(pointerId))
        {
            IFC(m_mapPointerState.Remove(pointerId, pointerState));
            pointerState->Reset();
        }
        if (pMsg->m_msgID == XCP_POINTERCAPTURECHANGED || pMsg->m_msgID == XCP_POINTERSUSPENDED)
        {
            m_interactionManager.StopInteraction(static_cast<CUIElement*>(pDOContact), false /*bCallbackForManipulationCompleted*/);
        }
        if (pMsg->m_msgID == XCP_POINTERLEAVE)
        {
            m_interactionManager.DestroyInteractionIdlingEngine();
        }
        IFC(RemovePointerIdFromInteractionElement(pointerId));
        IFC(RemoveEntryFromPointerDownTrackerMap(pointerId));

        if (pMsg->m_msgID == XCP_POINTERSUSPENDED &&
            pointerInputProcessor.IsInputTypeTreatedLikeTouch(pMsg->m_pointerInfo.m_pointerInputType) &&
            do_pointer_cast<CUIElement>(pDOContact))
        {
            // Unregister DM cross-slide viewports that might be associated with this pointer Id
            IFC(UnregisterCrossSlideViewportContactId(pointerId));
        }
    }

    if (pMsg->m_msgID == XCP_POINTERUP)
    {
        contentRoot->GetInputManager().SetBarrelButtonPressed(pMsg->m_pointerInfo.m_bBarrelButtonPressed);
        xref_ptr<CPointerEventArgs>& pendingPointerEventArgs = pointerInputProcessor.GetPendingPointerEventArgs();

        if (pendingPointerEventArgs && !pendingPointerEventArgs->m_bFiredDelayedPointerUp)
        {
            IFC(RaiseDelayedPointerUpEvent(NULL /* pMsgGesture */, pDOContact));
        }
        if (pointerInputProcessor.IsInputTypeTreatedLikeTouch(pMsg->m_pointerInfo.m_pointerInputType) && do_pointer_cast<CUIElement>(pDOContact))
        {
            // Unregister DM viewports that might be associated with this pointer Id
            IFC(UnRegisterDMViewportsOnPointerUp(pointerId));
        }
        if (pendingPointerEventArgs && m_mapPointerState.ContainsKey(pointerId))
        {
            CDependencyObject* pPointerCaptureDO = pointerState->GetCaptureDO();
            CDependencyObject* pPointerEnterDO = pointerState->GetCaptureDO();
            if (pPointerCaptureDO && pMsg->m_pointerInfo.m_pointerInputType == XcpPointerInputTypeMouse)
            {
                if (pPointerCaptureDO != pPointerEnterDO  && pPointerEnterDO == NULL || !HasPointerEnter(pPointerEnterDO, pointerId))
                {
                    pointerState->SetEnterDO(pPointerCaptureDO);
                    IFC(pointerInputProcessor.ProcessPointerEnterLeave(pDOContact, pPointerCaptureDO, pointerId, pendingPointerEventArgs, TRUE /* bSkipLeave */,  FALSE /* bForceRaisePointerEntered*/));
                }
                IFC(pointerInputProcessor.ReleasePointerCapture(pPointerCaptureDO, pendingPointerEventArgs->m_pPointer));
            }
        }
        if (pMsg->m_pointerInfo.m_pointerInputType == XcpPointerInputTypeMouse)
        {
            //
            // Keep the mouse PointerState since it always has the same pointer id.
            //

            m_interactionManager.DestroyInteractionIdlingEngine();
            IFC(RemovePointerIdFromInteractionElement(pointerId));
        }
    }

Cleanup:
    if (pMsg->m_msgID == XCP_POINTERUP)
    {
        pointerInputProcessor.ResetPendingPointerEventArgs();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   StopInteraction
//
//  Synopsis: Stop the interaction engine
//
//------------------------------------------------------------------------
void
CInputServices::StopInteraction(
    _In_ CUIElement* pContactElement,
    _In_ bool bCallbackForManipulationCompleted)
{
    m_interactionManager.StopInteraction(pContactElement, bCallbackForManipulationCompleted);
}

// Process the pointer messages with interaction context.
_Check_return_ HRESULT
CInputServices::ProcessInteractionPointerMessages(
    _In_ XUINT32 pointerId,
    _In_ InputMessage *pMsg)
{
    HRESULT hr = S_OK;
    bool isTouchInteractionUnpeggingRequired = false;
    bool isManipulationInteractionUnpeggingRequired = false;
    CUIElement *pInteractionElement = nullptr;
    CUIElement *pManipulationElement = nullptr;
    ElementGestureTracker *pTouchInteraction = nullptr;
    ElementGestureTracker *pManipulationInteraction = nullptr;

    ASSERT(pMsg);

    // Get the interaction element and interaction context for the current contact
    IFC(m_mapInteraction.Get(pointerId, pInteractionElement));
    ASSERT(pInteractionElement);
    IFC(m_interactionManager.GetInteractionEngine(pInteractionElement, nullptr /*pActiveInteractionElement*/, FALSE /*fManipulationOnly*/, &pTouchInteraction));
    ASSERT(pTouchInteraction);

    // Get the manipulation element and manipulation interaction context if manipulation
    // mode is enabled.
    IFC(GetManipulationElement(pInteractionElement, &pManipulationElement));
    if (pManipulationElement)
    {
        IFC(m_interactionManager.GetInteractionEngine(pManipulationElement, pInteractionElement, TRUE /*fManipulationOnly*/, &pManipulationInteraction));
        ASSERT(pManipulationInteraction);

        // Peg pManipulationElement's interaction engine so that it does not get destroyed inside CInputServices::CleanPointerElementObject
        // while processing the ProcessPointerMessagesWithInteractionEngine call below.
        ASSERT(!pManipulationInteraction->IsPegged());
        pManipulationInteraction->UpdatePeg(true);
        isManipulationInteractionUnpeggingRequired = true;
    }

    // When pManipulationElement == pInteractionElement, then pManipulationInteraction == pTouchInteraction and pTouchInteraction is already pegged above.
    if (!pTouchInteraction->IsPegged())
    {
        // Peg pInteractionElement's interaction engine so that it does not get destroyed inside CInputServices::CleanPointerElementObject
        // while processing the ProcessPointerMessagesWithInteractionEngine calls below.
        pTouchInteraction->UpdatePeg(true);
        isTouchInteractionUnpeggingRequired = true;
    }

    // This is for gesture or both gesture/manipulation on the current contact pointer
    IFC(ProcessPointerMessagesWithInteractionEngine(
        pointerId,
        pMsg,
        pInteractionElement,
        pManipulationElement,
        TRUE /*bIgnoreManipulationElement*/,
        pTouchInteraction,
        FALSE /*bForceDisableGesture*/));

    if (pManipulationElement && pManipulationElement != pInteractionElement)
    {
        ASSERT(pManipulationInteraction);
        // Manipulation specific pointer processing with the specified interaction context
        // that only recognize the manipulation result.
        IFC(ProcessPointerMessagesWithInteractionEngine(
            pointerId,
            pMsg,
            pInteractionElement,
            pManipulationElement,
            FALSE /*bIgnoreManipulationElement*/,
            pManipulationInteraction,
            TRUE /*bForceDisableGesture*/));
    }

    // Now that ProcessPointerMessagesWithInteractionEngine completed,
    // pManipulationInteraction and pTouchInteraction no longer needs protection against premature destruction.
    // Note the pManipulationInteraction and pTouchInteraction objects may be deleted when we unpeg them.
    if (isManipulationInteractionUnpeggingRequired)
    {
        ASSERT(pManipulationInteraction);
        pManipulationInteraction->UpdatePeg(false);
        isManipulationInteractionUnpeggingRequired = false;
    }
    if (isTouchInteractionUnpeggingRequired)
    {
        ASSERT(pTouchInteraction);
        pTouchInteraction->UpdatePeg(false);
        isTouchInteractionUnpeggingRequired = false;
    }

    // A ProcessPointerMessagesWithInteractionEngine above can result in the destruction of the pManipulationInteraction
    // or pTouchInteraction interaction engines, so let's null them to prevent their usage.
    pManipulationInteraction = nullptr;
    pTouchInteraction = nullptr;

    // The destruction of pManipulationInteraction may have been skipped because of the pegging
    // above. Destroy it now if it's no longer required.
    if (pManipulationElement && !pManipulationElement->IsInteractionEngineRequired(false /*ignoreActiveState*/))
    {
        m_interactionManager.DestroyInteractionEngine(pManipulationElement);
    }

    // The destruction of pInteractionElement may have been skipped because of the pegging
    // above. Destroy it now if it's no longer required.
    if (!pInteractionElement->IsInteractionEngineRequired(false /*ignoreActiveState*/))
    {
        m_interactionManager.DestroyInteractionEngine(pInteractionElement);
    }

Cleanup:
    if (isManipulationInteractionUnpeggingRequired)
    {
        ASSERT(pManipulationInteraction);
        pManipulationInteraction->UpdatePeg(false);
    }

    if (isTouchInteractionUnpeggingRequired)
    {
        ASSERT(pTouchInteraction);
        pTouchInteraction->UpdatePeg(false);
    }

    RRETURN(hr);
}

void
CInputServices::RaiseManipulationInertiaProcessingEvent()
{
    if (m_pVisualTree && m_interactionManager.IsManipulationInertiaProcessing())
    {
        m_pEventManager->Raise(
            EventHandle(KnownEventIndex::UIElement_ManipulationInertiaProcessing),
            TRUE,
            static_cast<CDependencyObject*>(m_pVisualTree->GetRootVisual()),
            NULL /*pArgs*/,
            FALSE /* fRaiseSync */,
            TRUE /* fInputEvent */);
    }
}

//------------------------------------------------------------------------
//
//  Method:   ProcessManipulationInertiaInteraction
//
//  Synopsis:
//      Process the manipulation inertia that notify the inertia processing
//      to the interaction context manager.
//
//------------------------------------------------------------------------
void
CInputServices::ProcessManipulationInertiaInteraction()
{
    m_interactionManager.ProcessManipulationInertiaInteraction();
}

//------------------------------------------------------------------------
//
//  Method:   ProcessPointerMessagesWithInteractionEngine
//
//  Synopsis:
//      Process the pointer messages for recognizing gestures and manipulation
//      with Touch Interaction Engine.
//  WARNING: The object pointed to by pInteractionContext may be deleted during this call.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessPointerMessagesWithInteractionEngine(
    _In_ XUINT32 pointerId,
    _In_ InputMessage *pMsg,
    _In_ CUIElement *pInteractionElement,
    _In_opt_ CUIElement *pManipulationElement,
    _In_ bool bIgnoreManipulationElement,
    _In_ ElementGestureTracker *pInteractionContext,
    _In_ bool bForceDisableGesture)
{
    ASSERT(pMsg);
    ASSERT(pInteractionElement);
    ASSERT(pInteractionContext);

    // Set ICM setting with the primary contact on the specified interaction element.
    if (pMsg->m_msgID == XCP_POINTERDOWN)
    {
        XUINT32 countInteractionPointer = 0;

        for (xchainedmap<XUINT32, CUIElement*>::const_iterator it = m_mapInteraction.begin();
            it != m_mapInteraction.end();
            ++it)
        {
            CUIElement* pActiveInteractionElement = (*it).second;

            if (!bIgnoreManipulationElement)
            {
                CUIElement* pActiveManipulationElement = NULL;
                ASSERT(pManipulationElement);
                IFC_RETURN(GetManipulationElement(pActiveInteractionElement, &pActiveManipulationElement));
                if (pActiveManipulationElement == pManipulationElement)
                {
                    countInteractionPointer++;
                }
            }
            else
            {
                if (pActiveInteractionElement == pInteractionElement)
                {
                    countInteractionPointer++;
                }
            }
            if (countInteractionPointer >= 2)
            {
                // Found the active interaction points more than 2.
                // The setting don't need to change if active interaction points more than 2.
                break;
            }
        }

        // Replace the interaction element with the manipulation element in case of the specified
        // manipulation only interaction.
        if (!bIgnoreManipulationElement)
        {
            ASSERT(pManipulationElement);
            pInteractionElement = pManipulationElement;
        }

        // Initialize interaction context setting with pointer contact message in case of the primary pointer down
        // on the specified interaction element.
        if (countInteractionPointer == 1)
        {
            XPOINTF ptPivotCenter;
            bool bTapEnabled = pInteractionElement->IsTapEnabled();
            bool bDoubleTapEnabled = pInteractionElement->IsDoubleTapEnabled();
            bool bRightTapEnabled = pInteractionElement->IsRightTapEnabled();
            bool bHoldEnabled = pInteractionElement->IsHoldEnabled();
            DirectUI::ManipulationModes uiManipulationMode = pInteractionElement->GetManipulationMode();
            XFLOAT fPivotRadius = static_cast<XFLOAT>(XDOUBLE_NAN);

            ptPivotCenter.x = static_cast<XFLOAT>(XDOUBLE_NAN);
            ptPivotCenter.y = static_cast<XFLOAT>(XDOUBLE_NAN);

            // Stop the current interaction if there is still remaining interaction(like Inertia).
            if (!pInteractionContext->IsIdle())
            {
                // DEAD_CODE_REMOVAL ?
                // Stop the interaction instead of Reset to complete the events.
                // A first call with bCallbackForManipulationCompleted=True is triggering the UIElement.ManipulationCompleted event, while
                // a second call with bCallbackForManipulationCompleted=False makes sure the ElementGestureTracker::m_bIsInteractionStopped
                // member is set to True.
                pInteractionContext->Stop(true /*bCallbackForManipulationCompleted*/);
                pInteractionContext->Stop(false /*bCallbackForManipulationCompleted*/);
            }

            if ((CustomManipulationModes(uiManipulationMode) != DirectUI::ManipulationModes::None) && !pInteractionContext->IsManipulationStarting())
            {
                // Raise ManipulationStarting event to retrieve the manipulation settings
                IFC_RETURN(RaiseManipulationStartingEvent(pInteractionElement, &uiManipulationMode, &ptPivotCenter, &fPivotRadius));
                pInteractionContext->SetManipulationStarting(true/* bManipulationStarting */);
            }

            // Disable the gesture setting if the interaction is for only manipulation recognition
            if (bForceDisableGesture)
            {
                bTapEnabled = FALSE;
                bDoubleTapEnabled = FALSE;
                bRightTapEnabled = FALSE;
                bHoldEnabled = FALSE;
            }

            // Update the interaction setting if the setting property is changed
            IFC_RETURN(pInteractionContext->SetConfiguration(bTapEnabled,
                                                      bDoubleTapEnabled,
                                                      bRightTapEnabled,
                                                      bHoldEnabled,
                                                      CustomManipulationModes(uiManipulationMode),
                                                      ptPivotCenter,
                                                      fPivotRadius));
        }
    }

#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:IM]: ProcessInteraction: XcpMsg=%d PointerId=%d Element=0x%p \r\n", pMsg->m_msgID, pointerId, pInteractionElement);
#endif // POINTER_TRACING

    // Non-client pointer messages don't have the pointer point information that the
    // interaction engine requires to properly detect gestures.
    if (!pMsg->m_isNonClientPointerMessage)
    {
        // Feed the pointer messages into the interaction engine.
        // WARNING: This function may delete its own "this" pointer.
        IFC_RETURN(pInteractionContext->ProcessPointerMessage(*pMsg));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   RaiseManipulationStartingEvent
//
//  Synopsis: Raise a manipulation starting event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RaiseManipulationStartingEvent(
    _In_ CUIElement* pElement,
    _Inout_ DirectUI::ManipulationModes* puiManipulationMode,
    _Inout_ XPOINTF* pPointPivotCenter,
    _Inout_ XFLOAT* pfPivotRadius)
{
    HRESULT hr = S_OK;
    CManipulationStartingEventArgs *pManipulationStartingArgs = NULL;

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElement);
    IFCPTR(puiManipulationMode);
    IFCPTR(pPointPivotCenter);
    IFCPTR(pfPivotRadius);

    // Create CManipulationStartingEventArgs
    pManipulationStartingArgs = new CManipulationStartingEventArgs(pCoreService);

    // Set the original source element
    IFC(pManipulationStartingArgs->put_Source(pElement));

    // Set the current manipulation settings
    pManipulationStartingArgs->m_uiManipulationMode = *puiManipulationMode;

    // Set the manipulation container
    IFC(pManipulationStartingArgs->put_Container(pElement));

    // Set the Pivot object as null
    IFC(pManipulationStartingArgs->put_Pivot(NULL));

    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
    // objects are alive and state is re-validated after return.
    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_ManipulationStarting),
        static_cast<CDependencyObject *>(pElement),
        pManipulationStartingArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    // Retrieve the manipulation settings after raise ManipulationStarting event
    *puiManipulationMode = pManipulationStartingArgs->m_uiManipulationMode;
    if (pManipulationStartingArgs->m_pPivot)
    {
        pPointPivotCenter->x = pManipulationStartingArgs->m_pPivot->m_ptCenter.x;
        pPointPivotCenter->y = pManipulationStartingArgs->m_pPivot->m_ptCenter.y;

        if (pManipulationStartingArgs->m_pManipulationContainer)
        {
            // Convert the pivot point to the global coordinate from the manipulation container.
            IFC(ConvertTransformPointToGlobal(pManipulationStartingArgs->m_pManipulationContainer, pPointPivotCenter));
            // Convert the pivot point to the manipulated element's local coordinate that won't be changed during the manipulation.
            IFC(ConvertTransformPointToLocal(pElement, pPointPivotCenter));
        }

        *pfPivotRadius = pManipulationStartingArgs->m_pPivot->m_fRadius;
    }

    // Add the manipulation container into the map chain to keep to use it during ManipulationStarted/Delta/Completed events
    if (pManipulationStartingArgs->m_pManipulationContainer)
    {
        if (m_mapManipulationContainer.ContainsKey(pElement))
        {
            CUIElement *pOldManipulationContainer = NULL;
            IFC(m_mapManipulationContainer.Remove(static_cast<CUIElement*>(pElement), pOldManipulationContainer));
        }
        IFC(m_mapManipulationContainer.Add(pElement, pManipulationStartingArgs->m_pManipulationContainer));
    }

Cleanup:
    ReleaseInterface(pManipulationStartingArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RaiseManipulationInertiaStartingEvent
//
//  Synopsis: Raise a manipulation inertia starting event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RaiseManipulationInertiaStartingEvent(
    _In_ CUIElement* pElement,
    _In_ TouchInteractionMsg *pMsg)
{
    HRESULT hr = S_OK;
    CManipulationInertiaStartingEventArgs *pManipulationArgs = NULL;
    CInertiaExpansionBehavior *pInertiaExpansion = NULL;
    CInertiaRotationBehavior *pInertiaRotation = NULL;
    CInertiaTranslationBehavior *pInertiaTranslation = NULL;
    CUIElement *pManipulationContainer = NULL;
    CManipulationDelta *pDelta = NULL;
    CManipulationDelta *pCumulative = NULL;
    CManipulationVelocities *pVelocities = NULL;
    CREATEPARAMETERS cp(m_pCoreService);

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElement);
    IFCPTR(pMsg);

    // Create CManipulationInertiaStartingEventArgs
    pManipulationArgs = new CManipulationInertiaStartingEventArgs(pCoreService);

    // Set the original source element
    IFC(pManipulationArgs->put_Source(pElement));

    // Set the pointer device type
    pManipulationArgs->m_pointerDeviceType = pManipulationArgs->GetPointerDeviceType(pMsg->m_pointerInputType);

    // Get the manipulation container that is specified on ManipulationStarting
    if (m_mapManipulationContainer.ContainsKey(pElement))
    {
        IFC(m_mapManipulationContainer.Get(pElement, pManipulationContainer));
    }
    // Set the manipulation container
    if (pManipulationContainer)
    {
        pManipulationArgs->m_pManipulationContainer = pManipulationContainer;
        pManipulationArgs->m_pManipulationContainer->AddRef();
    }

    const auto scale = RootScale::GetRasterizationScaleForElement(pElement);

    // Create pInertiaExpansion object and set on inertia expansion behavior
    IFC(CInertiaExpansionBehavior::Create((CDependencyObject**)&pInertiaExpansion, &cp));
    pInertiaExpansion->m_fDeceleration = ConvertPixelsToDips(scale, pMsg->m_fInertiaExpansionDeceleration);
    pInertiaExpansion->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_fInertiaExpansionExpansion);
    pManipulationArgs->m_pExpansion = pInertiaExpansion;
    pManipulationArgs->m_pExpansion->AddRef();

    // Create pInertiaRotation object and set on inertia rotation behavior
    IFC(CInertiaRotationBehavior::Create((CDependencyObject**)&pInertiaRotation, &cp));
    pInertiaRotation->m_fDeceleration = pMsg->m_fInertiaRotationDeceleration;
    pInertiaRotation->m_fRotation = pMsg->m_fInertiaRotationAngle;
    pManipulationArgs->m_pRotation = pInertiaRotation;
    pManipulationArgs->m_pRotation->AddRef();

    // Create pInertiaTranslation object and set on inertia translation behavior
    IFC(CInertiaTranslationBehavior::Create((CDependencyObject**)&pInertiaTranslation, &cp));
    pInertiaTranslation->m_fDeceleration = ConvertPixelsToDips(scale, pMsg->m_fInertiaTranslationDeceleration);
    pInertiaTranslation->m_fDisplacement = ConvertPixelsToDips(scale, pMsg->m_fInertiaTranslationDisplacement);

    pManipulationArgs->m_pTranslation = pInertiaTranslation;
    pManipulationArgs->m_pTranslation->AddRef();

    // Delta
    IFC(CManipulationDelta::Create((CDependencyObject**)&pDelta, &cp));

    pDelta->m_ptTranslation.x = ConvertPixelsToDips(scale, pMsg->m_delta.m_pointTranslation.x);
    pDelta->m_ptTranslation.y = ConvertPixelsToDips(scale, pMsg->m_delta.m_pointTranslation.y);
    pDelta->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_delta.m_floatExpansion);

    pDelta->m_fScale = pMsg->m_delta.m_floatScale;
    pDelta->m_fRotation = pMsg->m_delta.m_floatRotation;

    pManipulationArgs->m_pDelta = pDelta;
    pManipulationArgs->m_pDelta->AddRef();

    // Cumulative
    IFC(CManipulationDelta::Create((CDependencyObject**)&pCumulative, &cp));

    pCumulative->m_ptTranslation.x = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.x);
    pCumulative->m_ptTranslation.y = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.y);
    pCumulative->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_floatExpansion);

    pCumulative->m_fScale = pMsg->m_cumulative.m_floatScale;
    pCumulative->m_fRotation = pMsg->m_cumulative.m_floatRotation;

    pManipulationArgs->m_pCumulative = pCumulative;
    pManipulationArgs->m_pCumulative->AddRef();

    // Velocities
    IFC(CManipulationVelocities::Create((CDependencyObject**)&pVelocities, &cp));

    pVelocities->m_ptLinear.x = ConvertPixelsToDips(scale, pMsg->m_velocity.m_pointLinear.x);
    pVelocities->m_ptLinear.y = ConvertPixelsToDips(scale, pMsg->m_velocity.m_pointLinear.y);
    pVelocities->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_velocity.m_floatExpansion);

    pVelocities->m_fAngular = pMsg->m_velocity.m_floatAngular;

    pManipulationArgs->m_pVelocities = pVelocities;
    pManipulationArgs->m_pVelocities->AddRef();

    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
    // objects are alive and state is re-validated after return.
    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_ManipulationInertiaStarting),
        static_cast<CDependencyObject *>(pElement),
        pManipulationArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    // Retrieve the specified inertia settings
    // Rotation value is degrees. It'll be converted from degrees value to radian
    // when inertia setting is set to ICM.
    pMsg->m_fInertiaRotationDeceleration = pInertiaRotation->m_fDeceleration;
    pMsg->m_fInertiaRotationAngle = pInertiaRotation->m_fRotation;
    // Inertia expansion and deceleration need to be converted from device independent pixels
    // to physical pixels for ICM's consumption.
    pMsg->m_fInertiaExpansionDeceleration = ConvertDipsToPixels(scale, pInertiaExpansion->m_fDeceleration);
    pMsg->m_fInertiaExpansionExpansion = ConvertDipsToPixels(scale, pInertiaExpansion->m_fExpansion);

    // Inertia translation deceleration and displacement need to be converted from device independent pixels
    // to physical pixels for ICM's consumption.
    pMsg->m_fInertiaTranslationDeceleration = ConvertDipsToPixels(scale, pInertiaTranslation->m_fDeceleration);
    pMsg->m_fInertiaTranslationDisplacement = ConvertDipsToPixels(scale, pInertiaTranslation->m_fDisplacement);

Cleanup:
    ReleaseInterface(pInertiaExpansion);
    ReleaseInterface(pInertiaRotation);
    ReleaseInterface(pInertiaTranslation);
    ReleaseInterface(pDelta);
    ReleaseInterface(pCumulative);
    ReleaseInterface(pVelocities);
    ReleaseInterface(pManipulationArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

_Check_return_ HRESULT CInputServices::ProcessTouchInteractionCallback(
    _In_ const xref_ptr<CUIElement> &element,
    _In_ TouchInteractionMsg *message)
{
    IFCPTR_RETURN(element);
    IFCPTR_RETURN(message);

    switch (message->m_msgID)
    {
    case XCP_GESTURETAP:
    case XCP_GESTUREDOUBLETAP:
    case XCP_GESTUREHOLD:
    case XCP_GESTURERIGHTTAP:
        IFC_RETURN(ProcessGestureInput(element, message));
        break;
    case XCP_MANIPULATIONSTARTED:
        IFC_RETURN(ProcessManipulationStartedInput(element, message));
        break;
    case XCP_MANIPULATIONDELTA:
        IFC_RETURN(ProcessManipulationDeltaInput(element, message));
        break;
    case XCP_MANIPULATIONCOMPLETED:
        IFC_RETURN(ProcessManipulationCompletedInput(element, message));
        break;
    case XCP_MANIPULATIONINERTIASTARTING:
        IFC_RETURN(RaiseManipulationInertiaStartingEvent(element, message));
        break;
    default:
        IFC_RETURN(E_UNEXPECTED);
        break;
    }

    if (message->m_bPivotEnabled)
    {
        IFC_RETURN(ConvertTransformPointToGlobal(element, &message->m_pointPivot));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessGestureInput
//
//  Synopsis:
//      This is what handles the actual Gesture input from Touch Interaction
//      Engine callback.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessGestureInput(
    _In_ CDependencyObject *pElement,
    _In_ TouchInteractionMsg *pMsg)
{
    HRESULT hr = S_OK;

    CInputPointEventArgs *pArgs = NULL;
    CREATEPARAMETERS cp(m_pCoreService);
    CUIElement* pGestureElement = NULL;

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElement);
    IFCPTR(pMsg);

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pElement);

    pGestureElement = static_cast<CUIElement*>(pElement);

    switch (pMsg->m_msgID)
    {
    case XCP_GESTURETAP:
        if (pGestureElement->IsTapEnabled())
        {
            pArgs = new CTappedEventArgs(pCoreService);
        }
        break;
    case XCP_GESTUREDOUBLETAP:
        if (pGestureElement->IsDoubleTapEnabled())
        {
            pArgs = new CDoubleTappedEventArgs(pCoreService);
        }
        break;
    case XCP_GESTUREHOLD:
        if (pGestureElement->IsHoldEnabled())
        {
            pArgs = new CHoldingEventArgs(pCoreService);
        }
        break;
    case XCP_GESTURERIGHTTAP:
        if (pGestureElement->IsRightTapEnabled())
        {
            pArgs = new CRightTappedEventArgs(pCoreService);
        }
        break;
    default:
        IFC(E_UNEXPECTED);
        break;
    }

    if (pArgs == NULL)
    {
        // Current gesture result is disabled
        goto Cleanup;
    }

    // Set the original source element
    IFC(pArgs->put_Source(pElement));

    // Set gesture position
    pArgs->SetGlobalPoint(pMsg->m_pointInteraction);

    if (pMsg->m_msgID == XCP_GESTURETAP || pMsg->m_msgID == XCP_GESTURERIGHTTAP)
    {
        IFC(RaiseDelayedPointerUpEvent(pMsg, pElement));
    }

    // Set gesture device type
    pArgs->m_pointerDeviceType = pArgs->GetPointerDeviceType(pMsg->m_pointerInputType);

    switch (pMsg->m_msgID)
    {
    case XCP_GESTURETAP:
        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_pEventManager->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_Tapped),
            static_cast<CDependencyObject *>(pElement),
            pArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
        break;
    case XCP_GESTUREDOUBLETAP:
        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_pEventManager->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_DoubleTapped),
            static_cast<CDependencyObject *>(pElement),
            pArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
        break;
    case XCP_GESTUREHOLD:
        if (static_cast<DirectUI::HoldingState>(pMsg->m_holdingState) == DirectUI::HoldingState::Completed)
        {
            contentRoot->GetInputManager().SetIsContextMenuOnHolding(false);
        }

        // Set the gesture holding state
        static_cast<CHoldingEventArgs*>(pArgs)->m_holdingState = static_cast<DirectUI::HoldingState>(pMsg->m_holdingState);

        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_pEventManager->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_Holding),
            static_cast<CDependencyObject *>(pElement),
            pArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);

        // We can fire a ShowContextMenu event if the element is not draggable/pannable.
        // If the element is draggable/pannable, start a timer for 500 ms. In the timer expiration handler, we fire ShowContextMenu in this case.
        if (static_cast<CHoldingEventArgs*>(pArgs)->m_holdingState == DirectUI::HoldingState::Started)
        {
            wf::Point point = { pMsg->m_pointInteraction.x, pMsg->m_pointInteraction.y };
            contentRoot->GetInputManager().GetContextMenuProcessor().SetContextMenuOnHoldingTouchPoint(point);
            IFC(contentRoot->GetInputManager().GetContextMenuProcessor().ProcessContextRequestOnHoldingGesture(pElement));
        }
        break;
    case XCP_GESTURERIGHTTAP:
        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_pEventManager->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_RightTapped),
            static_cast<CDependencyObject *>(pElement),
            pArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
        if (!pArgs->m_bHandled)
        {
            IFC(RaiseRightTappedUnhandledEvent(pMsg, pElement));
        }

        // On mouse right-click or pen + barrel button, fire a ContextRequested event.
        if (pMsg->m_pointerInputType == XcpPointerInputTypeMouse || pMsg->m_pointerInputType == XcpPointerInputTypePen)
        {
            IFC(contentRoot->GetInputManager().RaiseContextRequestedEvent(
                static_cast<CDependencyObject*>(pElement),
                { pMsg->m_pointInteraction.x, pMsg->m_pointInteraction.y },
                false /* isTouchInput */));
        }
        break;
    default:
        IFC(E_UNEXPECTED);
        break;
    }

Cleanup:
    ReleaseInterface(pArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RaiseRightTappedUnhandledEvent
//
//  Synopsis:
//      Raise the RightTappedUnhandled event just after raising gesture
//      RightTapped event if it isn't handled.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RaiseRightTappedUnhandledEvent(
    _In_ TouchInteractionMsg* pMsg,
    _In_ CDependencyObject* pElement)
{
    HRESULT hr = S_OK;

    CInputPointEventArgs *pArgs = NULL;
    CREATEPARAMETERS cp(m_pCoreService);
    CUIElement* pGestureElement = NULL;

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pMsg);
    IFCPTR(pElement);

    pGestureElement = static_cast<CUIElement*>(pElement);

    if (pGestureElement->IsRightTapEnabled())
    {
        pArgs = new CRightTappedEventArgs(pCoreService);
    }

    if (pArgs == NULL)
    {
        goto Cleanup;
    }

    // Set the original source element
    IFC(pArgs->put_Source(pElement));

    // Set gesture position
    pArgs->SetGlobalPoint(pMsg->m_pointInteraction);

    // Set gesture device type for RightTapped
    pArgs->m_pointerDeviceType = pArgs->GetPointerDeviceType(pMsg->m_pointerInputType);

    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_RightTappedUnhandled),
        static_cast<CDependencyObject *>(pElement),
        pArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

Cleanup:
    ReleaseInterface(pArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RaiseDelayedPointerUpEvent
//
//  Synopsis:
//      Raise the delayed PointerUp event just before raising gesture Tapped
//      or RightTapped event.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RaiseDelayedPointerUpEvent(
    _In_opt_ TouchInteractionMsg* pMsgGesture,
    _In_ CDependencyObject* pElement)
{
    HRESULT hr = S_OK;
    std::shared_ptr<CPointerState> pointerState;
    CDependencyObject* pPointerCaptureDO = NULL;
    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pElement);

    IFCPTR(pElement);

    // Gesture recognition engine can notify the gesture result at WM_POINTERUPDATE after
    // fired WM_POINTERUP in case of touch inject simulation.
    xref_ptr<CPointerEventArgs>& pendingPointerEventArgs = contentRoot->GetInputManager().GetPointerInputProcessor().GetPendingPointerEventArgs();

    if (pendingPointerEventArgs)
    {
        if (pendingPointerEventArgs->m_pPointer &&
            m_mapPointerState.ContainsKey(pendingPointerEventArgs->m_pPointer->m_uiPointerId))
        {
            IFC(m_mapPointerState.Get(pendingPointerEventArgs->m_pPointer->m_uiPointerId, pointerState))
            if (pointerState)
            {
                pPointerCaptureDO = pointerState->GetCaptureDO();
            }
        }
        else
        {
            goto Cleanup;
        }

        if (pMsgGesture)
        {
            // Set GestureFollowing property.
            if (pMsgGesture->m_msgID == XCP_GESTURETAP)
            {
                pendingPointerEventArgs->m_uiGestureFollowing = DirectUI::GestureModes::Tapped;
            }
            else if (pMsgGesture->m_msgID == XCP_GESTURERIGHTTAP)
            {
                pendingPointerEventArgs->m_uiGestureFollowing = DirectUI::GestureModes::RightTapped;
            }
            else
            {
                ASSERT(FALSE);
            }
        }

        // Set m_bFiredDelayedPointerUp to TRUE just before raising PointerReleased event
        // since someone (Win32, RichEdit, SpellCheck or ContextMenu) can pump the message and
        // cause pendingPointerEventArgs to be cleaned up.
        pendingPointerEventArgs->m_bFiredDelayedPointerUp = TRUE;

        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_pEventManager->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_PointerReleased),
            pPointerCaptureDO ? pPointerCaptureDO : static_cast<CDependencyObject *>(pElement),
            pendingPointerEventArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
    }

Cleanup:
    ReleaseInterface(pCoreService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RaiseRightTappedEvent
//
//  Synopsis:
//      Raise RightTapped event for Programmatic invocation
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RaiseRightTappedEvent(_In_ CDependencyObject* pElement, _In_ DirectUI::PointerDeviceType deviceType)
{
    HRESULT hr = S_OK;
    XPOINTF localPoint;
    localPoint.x = 0.0f;
    localPoint.y = 0.0f;
    CUIElement *pInvokeElement = NULL;
    CRightTappedEventArgs *pRightTappedEventArgs = NULL;

    pRightTappedEventArgs = new CRightTappedEventArgs(m_pCoreService);

    // Set the original source element
    IFC(pRightTappedEventArgs->put_Source(pElement));

    pInvokeElement = static_cast<CUIElement*>(pElement);
    if (pInvokeElement)
    {
        IFC(ConvertTransformPointToGlobal(pInvokeElement, &localPoint));

        static_cast<CInputPointEventArgs*>(pRightTappedEventArgs)->SetGlobalPoint(localPoint);
    }
    else
    {
        // Set the default position
        static_cast<CInputPointEventArgs*>(pRightTappedEventArgs)->SetGlobalPoint(XPOINTF{});
    }

    // Set the device type
    pRightTappedEventArgs->m_pointerDeviceType = deviceType;

    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
    // objects are alive and state is re-validated after return.
    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_RightTapped),
        static_cast<CDependencyObject *>(pElement),
        pRightTappedEventArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

Cleanup:
    ReleaseInterface(pRightTappedEventArgs);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessManipulationStartedInput
//
//  Synopsis:
//      This is what handles the actual Manipulation input from Touch Interaction
//      Engine callback.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessManipulationStartedInput(
    _In_ CDependencyObject *pElement,
    _In_ TouchInteractionMsg *pMsg)
{
    HRESULT hr = S_OK;
    CManipulationStartedEventArgs *pManipulationArgs = NULL;
    CUIElement *pManipulationContainer = NULL;
    CManipulationDelta *pCumulative = NULL;
    CREATEPARAMETERS cp(m_pCoreService);

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElement);
    IFCPTR(pMsg);

    pManipulationArgs = new CManipulationStartedEventArgs(pCoreService);

    // Set the original source element
    IFC(pManipulationArgs->put_Source(pElement));

    // Set the pointer device type
    pManipulationArgs->m_pointerDeviceType = pManipulationArgs->GetPointerDeviceType(pMsg->m_pointerInputType);

    // Get the manipulation container that is specified on ManipulationStarting
    if (m_mapManipulationContainer.ContainsKey(static_cast<CUIElement*>(pElement)))
    {
        IFC(m_mapManipulationContainer.Get(static_cast<CUIElement*>(pElement), pManipulationContainer));
    }
    // Set the manipulation container
    if (pManipulationContainer)
    {
        pManipulationArgs->m_pManipulationContainer = pManipulationContainer;
        pManipulationArgs->m_pManipulationContainer->AddRef();
    }

    // Set manipulation origin
    IFC(pManipulationArgs->SetManipulationOriginPosition(pMsg->m_pointInteraction.x, pMsg->m_pointInteraction.y));

#ifdef TIEIM_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"TIEIM: Raising ManipulationStarted - OriginX=%4.2lf, OriginY=%4.2lf\r\n",
        pManipulationArgs->m_ptManipulation.x, pManipulationArgs->m_ptManipulation.y));
#endif // TIEIM_DBG

    // Cumulative
    IFC(CManipulationDelta::Create((CDependencyObject**)&pCumulative, &cp));
    const auto scale = RootScale::GetRasterizationScaleForElement(pElement);

    pCumulative->m_ptTranslation.x = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.x);
    pCumulative->m_ptTranslation.y = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.y);
    pCumulative->m_fScale = pMsg->m_cumulative.m_floatScale;
    pCumulative->m_fRotation = pMsg->m_cumulative.m_floatRotation;
    pCumulative->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_floatExpansion);

    pManipulationArgs->m_pCumulative = pCumulative;
    pManipulationArgs->m_pCumulative->AddRef();

    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
    // objects are alive and state is re-validated after return.
    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_ManipulationStarted),
        static_cast<CDependencyObject *>(pElement),
        pManipulationArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    // Raise the ManipulationCompleted event immediately if pManipulationArgs->Complete() is called
    if (pManipulationArgs->m_bRequestedComplete)
    {
        IFC(ProcessManipulationCompletedInput(pElement, pMsg));
    }

Cleanup:
    ReleaseInterface(pCumulative);
    ReleaseInterface(pManipulationArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessManipulationDeltaInput
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessManipulationDeltaInput(
    _In_ CDependencyObject *pElement,
    _In_ TouchInteractionMsg *pMsg)
{
    HRESULT hr = S_OK;
    CManipulationDeltaEventArgs *pManipulationArgs = NULL;
    CManipulationDelta *pDelta = NULL;
    CManipulationDelta *pCumulative = NULL;
    CManipulationVelocities *pVelocities = NULL;
    CUIElement *pManipulationContainer = NULL;
    CREATEPARAMETERS cp(m_pCoreService);

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElement);
    IFCPTR(pMsg);

    pManipulationArgs = new CManipulationDeltaEventArgs(pCoreService);
    const auto scale = RootScale::GetRasterizationScaleForElement(pElement);

    // Set the original source element
    IFC(pManipulationArgs->put_Source(pElement));

    // Set the pointer device type
    pManipulationArgs->m_pointerDeviceType = pManipulationArgs->GetPointerDeviceType(pMsg->m_pointerInputType);

    // Get the manipulation container that is specified on ManipulationStarting
    if (m_mapManipulationContainer.ContainsKey(static_cast<CUIElement*>(pElement)))
    {
        IFC(m_mapManipulationContainer.Get(static_cast<CUIElement*>(pElement), pManipulationContainer));
    }
    // Set the manipulation container
    if (pManipulationContainer)
    {
        pManipulationArgs->m_pManipulationContainer = pManipulationContainer;
        pManipulationArgs->m_pManipulationContainer->AddRef();
    }

    // Set manipulation origin
    IFC(pManipulationArgs->SetManipulationOriginPosition(pMsg->m_pointInteraction.x, pMsg->m_pointInteraction.y));

    // Set manipulation inertial state
    pManipulationArgs->m_bInertial = pMsg->m_bInertial;

    // Delta
    IFC(CManipulationDelta::Create((CDependencyObject**)&pDelta, &cp));

    pDelta->m_ptTranslation.x = ConvertPixelsToDips(scale, pMsg->m_delta.m_pointTranslation.x);
    pDelta->m_ptTranslation.y = ConvertPixelsToDips(scale, pMsg->m_delta.m_pointTranslation.y);
    pDelta->m_fScale = pMsg->m_delta.m_floatScale;
    pDelta->m_fRotation = pMsg->m_delta.m_floatRotation;
    pDelta->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_delta.m_floatExpansion);

    pManipulationArgs->m_pDelta = pDelta;
    pManipulationArgs->m_pDelta->AddRef();

    // Cumulative
    IFC(CManipulationDelta::Create((CDependencyObject**)&pCumulative, &cp));

    pCumulative->m_ptTranslation.x = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.x);
    pCumulative->m_ptTranslation.y = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.y);
    pCumulative->m_fScale = pMsg->m_cumulative.m_floatScale;
    pCumulative->m_fRotation = pMsg->m_cumulative.m_floatRotation;
    pCumulative->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_floatExpansion);

    pManipulationArgs->m_pCumulative = pCumulative;
    pManipulationArgs->m_pCumulative->AddRef();

    // Velocities
    IFC(CManipulationVelocities::Create((CDependencyObject**)&pVelocities, &cp));

    pVelocities->m_ptLinear.x = ConvertPixelsToDips(scale, pMsg->m_velocity.m_pointLinear.x);
    pVelocities->m_ptLinear.y = ConvertPixelsToDips(scale, pMsg->m_velocity.m_pointLinear.y);
    pVelocities->m_fAngular = pMsg->m_velocity.m_floatAngular;
    pVelocities->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_velocity.m_floatExpansion);

    pManipulationArgs->m_pVelocities = pVelocities;
    pManipulationArgs->m_pVelocities->AddRef();

#ifdef TIEIM_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"TIEIM: Raising ManipulationDelta - OriginX=%4.2lf, OriginY=%4.2lf, Inertial=%d\r\n",
        pManipulationArgs->m_ptManipulation.x, pManipulationArgs->m_ptManipulation.y, pManipulationArgs->m_bInertial));
    IGNOREHR(gps->DebugOutputSzNoEndl(L"       Delta TranslationX=%4.2lf, TranslationY=%4.2lf, FactorZ=%4.2lf\r\n",
        pDelta->m_ptTranslation.x, pDelta->m_ptTranslation.y, pDelta->m_fScale));
    IGNOREHR(gps->DebugOutputSzNoEndl(L"       Cumulative TranslationX=%4.2lf, TranslationY=%4.2lf, FactorZ=%4.2lf\r\n",
        pCumulative->m_ptTranslation.x, pCumulative->m_ptTranslation.y, pCumulative->m_fScale));
#endif // TIEIM_DBG

    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
    // objects are alive and state is re-validated after return.
    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_ManipulationDelta),
        static_cast<CDependencyObject *>(pElement),
        pManipulationArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    // Raise the ManipulationCompleted event immediately if pManipulationArgs->Complete() is called
    if (pManipulationArgs->m_bRequestedComplete)
    {
        IFC(ProcessManipulationCompletedInput(pElement, pMsg));
    }
    else if (pMsg->m_bInertial)
    {
        // Request the addition frame to process the inertia through OnTick().
        IFC(RequestAdditionalFrame());
    }

Cleanup:
    ReleaseInterface(pDelta);
    ReleaseInterface(pCumulative);
    ReleaseInterface(pVelocities);
    ReleaseInterface(pManipulationArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessManipulationCompletedInput
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessManipulationCompletedInput(
    _In_ CDependencyObject *pElement,
    _In_ TouchInteractionMsg *pMsg)
{
    HRESULT hr = S_OK;
    CManipulationCompletedEventArgs *pManipulationArgs = NULL;
    CManipulationDelta *pManipulationTotal = NULL;
    CManipulationVelocities *pVelocitiesFinal = NULL;
    CUIElement *pManipulationContainer = NULL;
    CREATEPARAMETERS cp(m_pCoreService);

    CCoreServices *pCoreService = m_pCoreService;
    IFCPTR(pCoreService);
    pCoreService->AddRef();

    IFCPTR(pElement);
    IFCPTR(pMsg);

    pManipulationArgs = new CManipulationCompletedEventArgs(pCoreService);

    // Set the original source element
    IFC(pManipulationArgs->put_Source(pElement));

    // Set the pointer device type
    pManipulationArgs->m_pointerDeviceType = pManipulationArgs->GetPointerDeviceType(pMsg->m_pointerInputType);

    // Get the manipulation container that is specified on ManipulationStarting
    if (m_mapManipulationContainer.ContainsKey(static_cast<CUIElement*>(pElement)))
    {
        IFC(m_mapManipulationContainer.Get(static_cast<CUIElement*>(pElement), pManipulationContainer));
    }
    // Set the manipulation container
    if (pManipulationContainer)
    {
        pManipulationArgs->m_pManipulationContainer = pManipulationContainer;
        pManipulationArgs->m_pManipulationContainer->AddRef();
    }

    // Set manipulation origin
    IFC(pManipulationArgs->SetManipulationOriginPosition(pMsg->m_pointInteraction.x, pMsg->m_pointInteraction.y));

    pManipulationArgs->m_bInertial = pMsg->m_bInertial;

    // Cumulative
    IFC(CManipulationDelta::Create((CDependencyObject**)&pManipulationTotal, &cp));
    const auto scale = RootScale::GetRasterizationScaleForElement(pElement);

    pManipulationTotal->m_ptTranslation.x = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.x);
    pManipulationTotal->m_ptTranslation.y = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_pointTranslation.y);
    pManipulationTotal->m_fScale = pMsg->m_cumulative.m_floatScale;
    pManipulationTotal->m_fRotation = pMsg->m_cumulative.m_floatRotation;
    pManipulationTotal->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_cumulative.m_floatExpansion);

    pManipulationArgs->m_pManipulationTotal = pManipulationTotal;
    pManipulationArgs->m_pManipulationTotal->AddRef();

    // Velocities
    IFC(CManipulationVelocities::Create((CDependencyObject**)&pVelocitiesFinal, &cp));

    pVelocitiesFinal->m_ptLinear.x = ConvertPixelsToDips(scale, pMsg->m_velocity.m_pointLinear.x);
    pVelocitiesFinal->m_ptLinear.y = ConvertPixelsToDips(scale, pMsg->m_velocity.m_pointLinear.y);

    pVelocitiesFinal->m_fAngular = pMsg->m_velocity.m_floatAngular;
    pVelocitiesFinal->m_fExpansion = ConvertPixelsToDips(scale, pMsg->m_velocity.m_floatExpansion);

    pManipulationArgs->m_pVelocitiesFinal = pVelocitiesFinal;
    pManipulationArgs->m_pVelocitiesFinal->AddRef();

#ifdef TIEIM_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"TIEIM: Raising ManipulationCompleted - OriginX=%4.2lf, OriginY=%4.2lf, Inertial=%d\r\n",
        pManipulationArgs->m_ptManipulation.x, pManipulationArgs->m_ptManipulation.y, pManipulationArgs->m_bInertial));
    IGNOREHR(gps->DebugOutputSzNoEndl(L"       TotalTranslationX=%4.2lf, TotalTranslationY=%4.2lf, TotalFactorZ=%4.2lf\r\n",
        pManipulationTotal->m_ptTranslation.x, pManipulationTotal->m_ptTranslation.y, pManipulationTotal->m_fScale));
#endif // TIEIM_DBG

    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
    // objects are alive and state is re-validated after return.
    m_pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_ManipulationCompleted),
        static_cast<CDependencyObject *>(pElement),
        pManipulationArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    // Remove the manipulation container from map chain
    if (m_mapManipulationContainer.ContainsKey(static_cast<CUIElement*>(pElement)))
    {
        CUIElement* unused = nullptr;
        IFC(m_mapManipulationContainer.Remove(static_cast<CUIElement*>(pElement), unused));
    }

Cleanup:
    ReleaseInterface(pManipulationTotal);
    ReleaseInterface(pVelocitiesFinal);
    ReleaseInterface(pManipulationArgs);
    ReleaseInterface(pCoreService);

    RRETURN(hr);
}

// Called when processing a WM_INPUTLANGCHANGE message.
_Check_return_ HRESULT
CInputServices::ProcessInputLanguageChange(_In_ InputMessage* pMsg, _In_ CContentRoot* contentRoot, _Out_ XINT32* pHandled)
{
    m_inputLang = reinterpret_cast<HKL>(static_cast<uintptr_t>(pMsg->m_langID));

    if (CTextBoxBase* pTextBoxBase = do_pointer_cast<CTextBoxBase>(contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef()))
    {
        IFC_RETURN(pTextBoxBase->OnInputLanguageChange(m_inputLang));
    }

    *pHandled = TRUE;

    return S_OK;
}

// Called when processing a WM_MOVE message.
_Check_return_ HRESULT
CInputServices::ProcessWindowMove(_In_ InputMessage* pMsg, _In_ CContentRoot* contentRoot)
{
    IFC_RETURN(contentRoot->GetAKExport().ExitAccessKeyMode());

    if (CTextBoxBase* pTextBoxBase = do_pointer_cast<CTextBoxBase>(contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef()))
    {
        IFC_RETURN(pTextBoxBase->OnWindowMoved());
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Method:   IsTextEditableControl
//
//  Synopsis:
//      Returns TRUE if an object is the control of type TextBox,
//      RichTextBox or PasswordBox and is also editable
//---------------------------------------------------------------------------
BOOL
CInputServices::IsTextEditableControl(_In_ const CDependencyObject* const pObject)
{
    BOOL isEditableControl = FALSE;

    if (const CTextBoxBase* pTextBoxBase = do_pointer_cast<CTextBoxBase>(pObject))
    {
        isEditableControl = !(pTextBoxBase->IsReadOnly());
    }

    return isEditableControl;
}

// static
HWND CInputServices::GetUnderlyingInputHwndFromIslandInputSite(_In_opt_ ixp::IIslandInputSitePartner* pIslandInputSite)
{
    if (nullptr != pIslandInputSite)
    {
        ABI::Microsoft::UI::WindowId inputWindowId;
        if (S_OK == pIslandInputSite->get_UnderlyingInputWindowId(&inputWindowId))
        {
            // The IslandInputSite might already be closed, in which case we should just return nullptr since there is no valid HWND.
            HWND inputHwnd;
            IFCFAILFAST(Windowing_GetWindowFromWindowId(inputWindowId, &inputHwnd));
            return inputHwnd;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   RegisterDirectManipulationContainer
//
//  Synopsis:
//    Called by UIElement_SetIsDirectManipulationContainer when an UIElement
//    declares itself as a CUIDMContainer implementer.
//    Initializes the provided element or puts it into a waiting queue for
//    future initialization.
//    Later when the CUIDMContainer implementer is destroyed
//    and a manipulation handler was not assigned to it yet, it calls this
//    method again with fIsDirectManipulationContainer==False.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RegisterDirectManipulationContainer(
    _In_ CUIElement* pDMContainer,
    _In_ bool fIsDirectManipulationContainer)
{
    IFCPTR_RETURN(pDMContainer);

    if (fIsDirectManipulationContainer)
    {
        // Make sure we don't register a DM container twice.
        const bool registerDMContainer =
            !m_pDMContainersNeedingInitialization ||
            std::find_if(
                m_pDMContainersNeedingInitialization->begin(),
                m_pDMContainersNeedingInitialization->end(),
                [=](const xref::weakref_ptr<CUIElement>& elem)
                {
                    ASSERT(elem);
                    return elem.lock() == pDMContainer;
                }) == m_pDMContainersNeedingInitialization->end();

        if (registerDMContainer)
        {
            // Registering a new CUIDMContainer implementer
            if (!m_pDMContainersNeedingInitialization)
            {
                m_pDMContainersNeedingInitialization.reset(new std::vector<xref::weakref_ptr<CUIElement>>);
            }
            m_pDMContainersNeedingInitialization->emplace_back(pDMContainer);

            // Request a UI thread tick to initialize the DM container.
            IFC_RETURN(RequestAdditionalFrame());
        }
    }
    else if (m_pDMContainersNeedingInitialization)
    {
        // Unregistering an old CUIDMContainer implementer
        auto position = std::find_if(m_pDMContainersNeedingInitialization->begin(), m_pDMContainersNeedingInitialization->end(),
            [=](const xref::weakref_ptr<CUIElement>& elem)
        {
            ASSERT(elem);
            return elem.lock() == pDMContainer;
        });

        if (position != m_pDMContainersNeedingInitialization->end())
        {
            pDMContainer->SetIsDirectManipulationContainer(FALSE);
            m_pDMContainersNeedingInitialization->erase(position);
        }

        if (m_pDMContainersNeedingInitialization->empty())
        {
            m_pDMContainersNeedingInitialization.reset();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::RegisterDirectManipulationCrossSlideContainer
//
//  Synopsis:
//    Called by UIElement_SetIsDirectManipulationCrossSlideContainer when an
//    UIElement declares itself as an element that requires a cross-slide DM
//    viewport in order to get a chance to recognize its own gestures/
//    manipulations. fIsDirectManipulationCrossSlideContainer is False when
//    the element no longer needs a cross-slide viewport.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RegisterDirectManipulationCrossSlideContainer(
    _In_ CUIElement* pDMCrossSlideContainer,
    _In_ bool fIsDirectManipulationCrossSlideContainer)
{
    HRESULT hr = S_OK;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: RegisterDirectManipulationCrossSlideContainer. pDMCrossSlideContainer=0x%p, fIsDirectManipulationCrossSlideContainer=%d, m_cCrossSlideContainers=%d.",
            this, pDMCrossSlideContainer, fIsDirectManipulationCrossSlideContainer, m_cCrossSlideContainers));
    }
#endif // DM_DEBUG

    IFCPTR(pDMCrossSlideContainer);

    if (pDMCrossSlideContainer->GetIsDirectManipulationCrossSlideContainer() != fIsDirectManipulationCrossSlideContainer)
    {
        bool fIsDraggable = false;

        // UIElement's m_fIsDirectManipulationCrossSlideContainer flag is changing
        pDMCrossSlideContainer->SetIsDirectManipulationCrossSlideContainer(fIsDirectManipulationCrossSlideContainer);

        IFC(pDMCrossSlideContainer->CanDrag(&fIsDraggable));

        if (fIsDraggable && m_islandInputSiteRegistrations.empty())
        {
            // No IslandInputSites have been registered yet! That means this CUIElement isn't yet in a tree connected to any ContentIsland.
            // Warning: potential fragility - continuing the existing assumption that the CUIElement will eventually be placed on the first
            // ContentIsland that will register its IslandInputSite with InputServices. If it is placed on a different ContentIsland, DManip
            // will likely not work.
            m_shouldRegisterPrimaryDMViewportCallback = true;
        }
        else
        {
            auto& islandInputSiteRegistration = GetIslandInputSiteRegistrationForUIElement(pDMCrossSlideContainer);
            if (fIsDraggable && !islandInputSiteRegistration.ShouldRegisterDMViewportCallback())
            {
                if (islandInputSiteRegistration.DMCrossSlideService() == nullptr)
                {
                    // Register DMViewportCallback when DMCrossSlideService is set up.
                    islandInputSiteRegistration.ShouldRegisterDMViewportCallback(true);
                }
                else
                {
                    // if DMViewportCallback is not already registered on DMCrossSlideService when DMCrossSlideService is already set up.
                    xref_ptr<IXcpDirectManipulationViewportEventHandler> spDirectManipulationViewportEventHandler;

                    IFC(GetDirectManipulationViewportEventHandler(spDirectManipulationViewportEventHandler.ReleaseAndGetAddressOf()));
                    IFC(islandInputSiteRegistration.DMCrossSlideService()->RegisterViewportEventHandler(spDirectManipulationViewportEventHandler.get()));
                }
            }
        }

        if (!fIsDirectManipulationCrossSlideContainer)
        {
            ASSERT(m_cCrossSlideContainers > 0);
            m_cCrossSlideContainers--;

            // Discard the cross-slide viewport that might currently be associated with this element
            IFC(GetCrossSlideViewport(pDMCrossSlideContainer, &pCrossSlideViewport));
            if (pCrossSlideViewport)
            {
                IFC(DirectManipulationCrossSlideContainerCompleted(pDMCrossSlideContainer, pCrossSlideViewport));
            }
        }
        else
        {
            m_cCrossSlideContainers++;
        }

#ifdef DM_DEBUG
        if (m_fIsDMVerboseInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                L"                   new m_cCrossSlideContainers=%d.", m_cCrossSlideContainers));
        }
#endif // DM_DEBUG

    }

Cleanup:
    ReleaseInterface(pCrossSlideViewport);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DirectManipulationCrossSlideContainerCompleted
//
//  Synopsis:
//    Called by UIElement_DirectManipulationCrossSlideContainerCompleted when the
//    provided UIElement no longer requires a cross-slide viewport. This may be
//    because a DM manipulation needs to start for the owning ScrollViewer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DirectManipulationCrossSlideContainerCompleted(
    _In_ CUIElement* pDMCrossSlideContainer,
    _In_opt_ CDMCrossSlideViewport* pCrossSlideViewport)
{
    HRESULT hr = S_OK;
    XUINT32 cContactIds = 0;
    CDMCrossSlideViewport* pCrossSlideViewportTmp = NULL;

    TraceDmCrossSlideContainerCompletedBegin();

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  DirectManipulationCrossSlideContainerCompleted. pDMCrossSlideContainer=0x%p.",
            this, pDMCrossSlideContainer));
    }
#endif // DM_DEBUG

    IFCPTR(pDMCrossSlideContainer);

    if (!pCrossSlideViewport)
    {
        IFC(GetCrossSlideViewport(pDMCrossSlideContainer, &pCrossSlideViewportTmp));
        pCrossSlideViewport = pCrossSlideViewportTmp;
    }

    // Our container will be null if this function has been called already for this container (without an intervening
    // RegisterDirectManipulationCrossSlideContainer(TRUE)). Practically, this should never happen as controls that deal with
    // cross slide viewports are coded to only call this when necessary. This is here to guard against bugs.
    ASSERT(pCrossSlideViewport);
    if (pCrossSlideViewport)
    {
        auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pDMCrossSlideContainer);
        ASSERT(pDMCrossSlideService);

        IFC(pCrossSlideViewport->GetContactIdCount(&cContactIds));
        if (cContactIds > 0)
        {
            IFC(pDMCrossSlideService->ReleaseAllContacts(pCrossSlideViewport));
        }

        IFC(UnregisterCrossSlideViewport(pCrossSlideViewport));
    }

Cleanup:
    ReleaseInterface(pCrossSlideViewportTmp);

    TraceDmCrossSlideContainerCompletedEnd();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::InitializeDirectManipulationContainer
//
//  Synopsis:
//    Creates a CUIDMContainer and IDirectManipulationContainerHandler
//    instances for the provided element and sets them up for future usage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::InitializeDirectManipulationContainer(
    _In_ CUIElement* pDMContainer)
{
    HRESULT hr = S_OK;
    bool fCanManipulateElementsByTouch = false;
    bool fCanManipulateElementsNonTouch = false;
    bool fCanManipulateElementsWithBringIntoViewport = false;
    IXcpDirectManipulationViewportEventHandler* pDirectManipulationViewportEventHandler = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pNewDirectManipulationService = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    ASSERT(CanDMContainerInitialize(pDMContainer));

    IFCPTR(pDMContainer);

    // Create a DM Service for this DM container.
    IFC(gps->GetDirectManipulationService(m_DMServiceSharedState, &pNewDirectManipulationService));
    ASSERT(pNewDirectManipulationService);

    IFC(EnsureDMServices());
    AddRefInterface(pDMContainer);
    IFC(m_pDMServices->Add(pDMContainer, pNewDirectManipulationService));
    pDirectManipulationService = pNewDirectManipulationService;
    pNewDirectManipulationService = NULL;

    IFC(pDirectManipulationService->EnsureDirectManipulationManager(pDMContainer->GetElementIslandInputSite().Get(), FALSE /*fIsForCrossSlideViewports*/));

    IFC(GetDirectManipulationViewportEventHandler(&pDirectManipulationViewportEventHandler));
    IFC(pDirectManipulationService->RegisterViewportEventHandler(pDirectManipulationViewportEventHandler));

    IFC(pDMContainer->CreateDirectManipulationContainer());
    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
    IFC(pDirectManipulationContainer->GetCanManipulateElements(&fCanManipulateElementsByTouch, &fCanManipulateElementsNonTouch, &fCanManipulateElementsWithBringIntoViewport));
    if (fCanManipulateElementsByTouch || fCanManipulateElementsNonTouch || fCanManipulateElementsWithBringIntoViewport)
    {
        // Activate DM manager for this container.
        IFC(UpdateDirectManipulationManagerActivation(
            pDMContainer,
            FALSE /*fCancelManipulations*/,
            fCanManipulateElementsByTouch,
            fCanManipulateElementsNonTouch,
            fCanManipulateElementsWithBringIntoViewport,
            FALSE /*fRefreshViewportStatus*/));
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pNewDirectManipulationService);
    ReleaseInterface(pDirectManipulationViewportEventHandler);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::UpdateCrossSlideViewportConfigurations
//
//  Synopsis:
//    Updates the parent viewport configuration or combined parent viewports
//    configurations for the cross-slide viewports that still need to be started
//    through a call to OnDirectManipulationCrossSlideContainerStart. The
//    active configuration of the provided viewport is used.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateCrossSlideViewportConfigurations(
    _In_ XUINT32 pointerId,
    _In_ CDMViewport* pViewport,
    _Out_ bool* pfContactFailure)
{
    bool fCausedRunningStatus = false;
    XUINT32 cCrossSlideViewports = 0;
    XDMConfigurations configuration = XcpDMConfigurationNone;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

    IFCPTR_RETURN(pViewport);
    IFCPTR_RETURN(pfContactFailure);
    *pfContactFailure = false;

    if (m_pCrossSlideViewports)
    {
        cCrossSlideViewports = m_pCrossSlideViewports->size();
        for (XUINT32 iCrossSlideViewport = 0; iCrossSlideViewport < cCrossSlideViewports; iCrossSlideViewport++)
        {
            IFC_RETURN(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewport));
            ASSERT(pCrossSlideViewport);

            if (pCrossSlideViewport->GetNeedsStart())
            {
                ASSERT(!pCrossSlideViewport->GetIsRejectionViewport());
                configuration = pViewport->GetTouchConfiguration();
                ASSERT(configuration != XcpDMConfigurationNone);
                if (pCrossSlideViewport->HasParentViewportConfiguration())
                {
                    pCrossSlideViewport->CombineParentViewportsConfigurations(configuration);
                }
                else
                {
                    pCrossSlideViewport->SetParentViewportConfiguration(configuration);

                    // Now it's time to setup the cross-slide viewport based on the parent's configuration.
                    configuration = static_cast<XDMConfigurations>(configuration & (XcpDMConfigurationPanX | XcpDMConfigurationPanY));
                    if (configuration == XcpDMConfigurationPanX || configuration == XcpDMConfigurationPanY)
                    {
                        auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pViewport->GetDMContainerNoRef());
                        ASSERT(pDMCrossSlideService);
                        IFC_RETURN(pDMCrossSlideService->AddViewportConfiguration(
                            pCrossSlideViewport,
                            TRUE /*fIsCrossSlideViewport*/,
                            FALSE /*fIsDragDrop*/,
                            configuration));
                        // fCausedRunningStatus is ignored because we don't track cross-slide viewport statuses.
                        IFC_RETURN(pDMCrossSlideService->EnableViewport(pCrossSlideViewport, fCausedRunningStatus));
                        IFC_RETURN(AddCrossSlideViewportContactId(pointerId, pCrossSlideViewport, pfContactFailure));
                        if (*pfContactFailure)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::StartDirectManipulationCrossSlideContainer
//
//  Synopsis:
//    Notifies the provided element that a cross-slide viewport was created on its
//    behalf. The element is then supposed to call DirectManipulationCrossSlideContainerCompleted
//    when a DM manipulation is recognized.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::StartDirectManipulationCrossSlideContainer(
    _In_ CUIElement* pDMCrossSlideContainer,
    _In_ XDMConfigurations parentViewportConfiguration,
    _In_ XDMConfigurations parentViewportsCombinedConfigurations)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  StartDirectManipulationCrossSlideContainer. pDMCrossSlideContainer=0x%p.",
            this, pDMCrossSlideContainer));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pDMCrossSlideContainer);
    IFCPTR_RETURN(m_pCoreService);

    IFC_RETURN(DirectManipulationCrossSlideContainerCompleted(pDMCrossSlideContainer, NULL /*pCrossSlideViewport*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::StartDirectManipulationCrossSlideContainers
//
//  Synopsis:
//    Loops through all existing cross-slide viewports and checks which ones
//    need a call to OnDirectManipulationCrossSlideContainerStart to kick-start
//    the manipulation recognition.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::StartDirectManipulationCrossSlideContainers()
{
    HRESULT hr = S_OK;
    XINT32 cCrossSlideViewports = 0;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

    TraceDmStartCrossSlideContainersBegin();

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  StartDirectManipulationCrossSlideContainers. m_pCrossSlideViewports=0x%p.",
            this, m_pCrossSlideViewports));
    }
#endif // DM_DEBUG

    if (m_pCrossSlideViewports)
    {
        cCrossSlideViewports = static_cast<XINT32>(m_pCrossSlideViewports->size());
        for (XINT32 iCrossSlideViewport = 0; iCrossSlideViewport < cCrossSlideViewports; iCrossSlideViewport++)
        {
            IFC(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewport));
            ASSERT(pCrossSlideViewport);

            if (pCrossSlideViewport->GetNeedsStart())
            {
                ASSERT(!pCrossSlideViewport->GetIsRejectionViewport());
                pCrossSlideViewport->AddRef();
                IFC(StartDirectManipulationCrossSlideContainer(
                    pCrossSlideViewport->GetDMContainerNoRef(),
                    pCrossSlideViewport->GetParentViewportConfiguration(),
                    pCrossSlideViewport->GetParentViewportsCombinedConfigurations()));
                pCrossSlideViewport->SetNeedsStart(FALSE);
                ASSERT(static_cast<XINT32>(m_pCrossSlideViewports->size()) == cCrossSlideViewports ||
                    static_cast<XINT32>(m_pCrossSlideViewports->size()) == cCrossSlideViewports - 1);
                if (static_cast<XINT32>(m_pCrossSlideViewports->size()) == cCrossSlideViewports - 1)
                {
                    // Cross-slide viewport was discarded because OnDirectManipulationCrossSlideContainerStart returned fStarted==FALSE
                    cCrossSlideViewports--;
                    iCrossSlideViewport--;
                }
                ReleaseInterface(pCrossSlideViewport);
            }
            pCrossSlideViewport = NULL;
        }
    }

Cleanup:
    ReleaseInterface(pCrossSlideViewport);

    TraceDmStartCrossSlideContainersEnd();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::CompleteDirectManipulationCrossSlideContainers
//
//  Synopsis:
//    Loops through all existing cross-slide viewports and checks which ones
//    were meant to be started via a call to OnDirectManipulationCrossSlideContainerStart.
//    Discard those cross-slide viewports instead.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CompleteDirectManipulationCrossSlideContainers()
{
    XINT32 cCrossSlideViewports = 0;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  CompleteDirectManipulationCrossSlideContainers. m_pCrossSlideViewports=0x%p.",
            this, m_pCrossSlideViewports));
    }
#endif // DM_DEBUG

    if (m_pCrossSlideViewports)
    {
        cCrossSlideViewports = static_cast<XINT32>(m_pCrossSlideViewports->size());
        for (XINT32 iCrossSlideViewport = 0; iCrossSlideViewport < cCrossSlideViewports; iCrossSlideViewport++)
        {
            IFC_RETURN(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewport));
            ASSERT(pCrossSlideViewport);

            if (pCrossSlideViewport->GetNeedsStart())
            {
                ASSERT(!pCrossSlideViewport->GetIsRejectionViewport());
                IFC_RETURN(DirectManipulationCrossSlideContainerCompleted(pCrossSlideViewport->GetDMContainerNoRef(), pCrossSlideViewport));
                ASSERT(static_cast<XINT32>(m_pCrossSlideViewports->size()) == cCrossSlideViewports - 1);
                cCrossSlideViewports--;
                iCrossSlideViewport--;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::InitializeDirectManipulationContainers
//
//  Synopsis:
//    Creates a CUIDMContainer and IDirectManipulationContainerHandler
//    implementations for all elements in the waiting queue.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::InitializeDirectManipulationContainers()
{
    if (CanDMContainerInitialize() && m_pDMContainersNeedingInitialization)
    {
#ifdef DM_DEBUG
        if (m_fIsDMVerboseInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                L"DMIMv[0x%p]: InitializeDirectManipulationContainers. Initializing DM containers.", this));
        }
#endif // DM_DEBUG

        // Only pre-create the DM manager dedicated to cross-slide viewports when a DManip container
        // and real viewport is instantiated.
        for (auto& islandInputSiteRegistration : m_islandInputSiteRegistrations)
        {
            if (!islandInputSiteRegistration.DMCrossSlideService())
            {
                // Pre-create a DM manager for potential cross-slide handling, using cross-slide viewports
                wrl::ComPtr<IPALDirectManipulationService> cpDMCrossSlideService{ nullptr };
                IFC_RETURN(gps->GetDirectManipulationService(m_DMServiceSharedState, &cpDMCrossSlideService));
                ASSERT(cpDMCrossSlideService);

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  InitializeDirectManipulationContainers. Created cpDMCrossSlideService=0x%p. m_cCrossSlideContainers=%d.",
                        this, cpDMCrossSlideService.Get(), m_cCrossSlideContainers));
                }
#endif // DM_DEBUG

                IFC_RETURN(cpDMCrossSlideService->EnsureDirectManipulationManager(islandInputSiteRegistration.IslandInputSite().Get(), TRUE /*fIsForCrossSlideViewports*/));
                IFC_RETURN(cpDMCrossSlideService->ActivateDirectManipulationManager());

                if (islandInputSiteRegistration.ShouldRegisterDMViewportCallback())
                {
                    xref_ptr<IXcpDirectManipulationViewportEventHandler> spDirectManipulationViewportEventHandler;

                    IFC_RETURN(GetDirectManipulationViewportEventHandler(spDirectManipulationViewportEventHandler.ReleaseAndGetAddressOf()));
                    IFC_RETURN(cpDMCrossSlideService->RegisterViewportEventHandler(spDirectManipulationViewportEventHandler.get()));
                }

                // Success! Store our IslandInputSite specific DMCrossSlideService.
                islandInputSiteRegistration.DMCrossSlideService(cpDMCrossSlideService.Get());
            }
        }

        // Shuffle all the elements we're about to remove to the end
        // Avoids a potential n^2 shuffle
        auto position = std::partition(m_pDMContainersNeedingInitialization->begin(), m_pDMContainersNeedingInitialization->end(),
            [](const xref::weakref_ptr<CUIElement>& elem)
        {
            // Only keep containers that are inactive or do not have a valid IslandInputSite.
            auto pDMContainer = elem.lock();
            return pDMContainer && (!pDMContainer->IsActive() || !pDMContainer->CanDMContainerInitialize());
        });

        // Process all the remaining active containers
        for (auto iter = position; iter != m_pDMContainersNeedingInitialization->end(); ++iter)
        {
            auto pDMContainer = iter->lock();
            if (pDMContainer)
            {
                IFC_RETURN(InitializeDirectManipulationContainer(iter->lock()));
            }
        }

        m_pDMContainersNeedingInitialization->erase(position, m_pDMContainersNeedingInitialization->end());
        if (m_pDMContainersNeedingInitialization->empty())
        {
            m_pDMContainersNeedingInitialization.reset();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::EnsureViewports
//
//  Synopsis:
//    Allocates the m_pViewports xvector if needed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::EnsureViewports()
{
    HRESULT hr = S_OK;

    if (!m_pViewports)
    {
        m_pViewports = new xvector<CDMViewport*>();
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::EnsureCrossSlideViewports
//
//  Synopsis:
//    Allocates the m_pCrossSlideViewports xvector if needed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::EnsureCrossSlideViewports()
{
    HRESULT hr = S_OK;

    if (!m_pCrossSlideViewports)
    {
        m_pCrossSlideViewports = new xvector<CDMCrossSlideViewport*>();
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetCrossSlideViewport
//
//  Synopsis:
//    Returns the existing cross-slide viewport for the provided element, NULL otherwise.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetCrossSlideViewport(
    _In_ CUIElement* pDMCrossSlideContainer,
    _Outptr_ CDMCrossSlideViewport** ppCrossSlideViewport)
{
    XUINT32 cCrossSlideViewports = 0;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

    IFCPTR_RETURN(pDMCrossSlideContainer);
    IFCPTR_RETURN(ppCrossSlideViewport);
    *ppCrossSlideViewport = NULL;

    if (m_pCrossSlideViewports)
    {
        cCrossSlideViewports = m_pCrossSlideViewports->size();
        for (XUINT32 iCrossSlideViewport = 0; iCrossSlideViewport < cCrossSlideViewports; iCrossSlideViewport++)
        {
            IFC_RETURN(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewport));
            if (pCrossSlideViewport->GetDMContainerNoRef() == pDMCrossSlideContainer)
            {
                AddRefInterface(pCrossSlideViewport);
                *ppCrossSlideViewport = pCrossSlideViewport;
                break;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetViewportForContentElement
//
//  Synopsis:
//    Returns the CDMViewport and CDMContent instances associated with
//    the provided secondary content element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetViewportForContentElement(
    _In_ CUIElement* pContentElement,
    _Outptr_ CDMViewport** ppViewport,
    _Outptr_ CDMContent** ppContent)
{
    XUINT32 iViewport = 0;
    XUINT32 cViewports = 0;
    CDMViewport* pViewport = NULL;
    CDMContent* pContent = NULL;

    IFCPTR_RETURN(pContentElement);
    IFCPTR_RETURN(ppViewport);
    *ppViewport = NULL;
    IFCPTR_RETURN(ppContent);
    *ppContent = NULL;

    if (m_pViewports)
    {
        cViewports = m_pViewports->size();
        for (iViewport = 0; iViewport < cViewports; iViewport++)
        {
            IFC_RETURN(m_pViewports->get_item(iViewport, pViewport));
            IFC_RETURN(pViewport->GetContentNoRef(pContentElement, &pContent, NULL /*pContentIndex*/));
            if (pContent)
            {
                AddRefInterface(pViewport);
                AddRefInterface(pContent);
                *ppViewport = pViewport;
                *ppContent = pContent;
                break;
            }

            IFC_RETURN(pViewport->GetClipContentNoRef(pContentElement, &pContent, NULL /*pContentIndex*/));
            if (pContent)
            {
                AddRefInterface(pViewport);
                AddRefInterface(pContent);
                *ppViewport = pViewport;
                *ppContent = pContent;
                break;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetViewport
//
//  Synopsis:
//    Returns the CDMViewport instance associated with the provided
//    manipulated element (i.e. DM content)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetViewport(
    _In_opt_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _Outptr_ CDMViewport** ppViewport)
{
    XUINT32 iViewport = 0;
    XUINT32 cViewports = 0;
    CDMViewport* pViewport = NULL;

    IFCPTR_RETURN(pManipulatedElement);
    IFCPTR_RETURN(ppViewport);
    *ppViewport = NULL;

    if (m_pViewports)
    {
        cViewports = m_pViewports->size();
        for (iViewport = 0; iViewport < cViewports; iViewport++)
        {
            IFC_RETURN(m_pViewports->get_item(iViewport, pViewport));
            if (pViewport->GetManipulatedElementNoRef() == pManipulatedElement &&
                (!pDMContainer || pViewport->GetDMContainerNoRef() == pDMContainer))
            {
                *ppViewport = pViewport;
                break;
            }
        }

        if (!pDMContainer && *ppViewport && (*ppViewport)->GetNeedsUnregistration())
        {
            for (iViewport = iViewport + 1; iViewport < cViewports; iViewport++)
            {
                IFC_RETURN(m_pViewports->get_item(iViewport, pViewport));
                if (pViewport->GetManipulatedElementNoRef() == pManipulatedElement)
                {
                    // The manipulated element actually belongs to this viewport that is not unregistering.
                    ASSERT(!pViewport->GetNeedsUnregistration());
                    *ppViewport = pViewport;
                    break;
                }
            }
        }

        if (*ppViewport)
        {
            AddRefInterface(*ppViewport);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetViewport
//
//  Synopsis:
//    Returns the CDMViewport instance associated with the provided
//    DManip container, irrespective of its manipulatable element.
//    Only non-unregistering viewports qualify.
//
//------------------------------------------------------------------------
_Maybenull_ CDMViewport*
CInputServices::GetViewport(_In_ CUIElement* pDMContainer) const
{
    if (m_pViewports)
    {
        XUINT32 cViewports = m_pViewports->size();
        for (XUINT32 iViewport = 0; iViewport < cViewports; iViewport++)
        {
            CDMViewport* pViewport = nullptr;
            VERIFYHR(m_pViewports->get_item(iViewport, pViewport));
            if (pViewport && pViewport->GetDMContainerNoRef() == pDMContainer && !pViewport->GetNeedsUnregistration())
            {
                return pViewport;
            }
        }
    }

    return nullptr;
}

// Returns True when the provided element is:
// - declared manipulatable for DirectManipulation
// - the primary content of a viewport
_Check_return_ HRESULT
CInputServices::IsManipulatablePrimaryContent(
    _In_ CUIElement* pUIElement,
    _Out_ bool* pIsManipulatablePrimaryContent)
{
    *pIsManipulatablePrimaryContent = false;

    if (pUIElement->IsManipulatable())
    {
        xref_ptr<CDMViewport> spViewport;

        IFC_RETURN(GetViewport(
            nullptr /*pDMContainer*/,
            pUIElement /*pManipulatedElement*/,
            spViewport.ReleaseAndGetAddressOf()));

        *pIsManipulatablePrimaryContent = spViewport != nullptr;
    }

    return S_OK;
}

// Returns the viewport for pCandidateElement if this element is primary or secondary content
_Check_return_ HRESULT
CInputServices::GetViewportForPrimaryOrSecondaryContent(
    _In_ CUIElement* pCandidateElement,
    _Outptr_result_maybenull_ CDMViewport** ppViewport)
{
    HRESULT hr = S_OK;
    CDMViewport *pViewport = nullptr;

    *ppViewport = nullptr;

    if (m_pViewports != nullptr)
    {
        UINT cViewports = m_pViewports->size();
        for (UINT32 iViewport = 0; iViewport < cViewports; iViewport++)
        {
            IFC(m_pViewports->get_item(iViewport, pViewport));

            bool found = SearchViewportForPrimaryOrSecondaryContent(pViewport, pCandidateElement);
            if (found)
            {
                *ppViewport = pViewport;
                if (!pViewport->GetNeedsUnregistration())
                {
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:
    AddRefInterface(*ppViewport);
    RRETURN(hr);
}

// Searches a viewport's primary and secondary content for the given pCandidateElement as its manipulatable element
bool CInputServices::SearchViewportForPrimaryOrSecondaryContent(
    _In_ CDMViewport* pViewport,
    _In_ CUIElement* pCandidateElement)
{
    xref_ptr<CDMContent> spContent;

    if (pViewport->GetManipulatedElementNoRef() == pCandidateElement)
    {
        // Aha!  The candidate is the primary content.
        return true;
    }

    // Now search through all secondary content for this viewport
    UINT32 cContents = pViewport->GetContentsCount();

    for (UINT32 iContent = 0; iContent < cContents; iContent++)
    {
        IFCFAILFAST(pViewport->GetContent(iContent, spContent.ReleaseAndGetAddressOf()));

        if (spContent->GetContentElementNoRef() == pCandidateElement)
        {
            // Aha!  The candidate is secondary content
            return true;
        }
    }

    // Now search through all secondary clip content for this viewport
    UINT32 cClipContents = pViewport->GetClipContentsCount();

    for (UINT32 iClipContent = 0; iClipContent < cClipContents; iClipContent++)
    {
        IFCFAILFAST(pViewport->GetClipContent(iClipContent, spContent.ReleaseAndGetAddressOf()));

        if (spContent->GetContentElementNoRef() == pCandidateElement)
        {
            // Aha!  The candidate is secondary clip content
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetContentForContentElement
//
//  Synopsis:
//    Returns the CDMContent instance associated with the provided
//    secondary content relationship.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetContentForContentElement(
    _In_ CUIElement *pContentElement,
    _In_ bool targetClip,
    _Outptr_ CDMContent** ppContent)
{
    HRESULT hr = S_OK;
    XUINT32 cViewports = 0;
    CDMViewport *pViewport = NULL;
    CDMContent *pContent = NULL;

    IFCPTR(pContentElement);
    IFCPTR(ppContent);
    *ppContent = NULL;

    if (m_pViewports)
    {
        cViewports = m_pViewports->size();
        for (XUINT32 iViewport = 0; iViewport < cViewports && (*ppContent) == NULL; iViewport++)
        {
            XUINT32 cContents = 0;
            IFC(m_pViewports->get_item(iViewport, pViewport));

            cContents = targetClip ? pViewport->GetClipContentsCount() : pViewport->GetContentsCount();

            for (XUINT32 iContent = 0; iContent < cContents && (*ppContent) == NULL; iContent++)
            {
                IFC(targetClip ? pViewport->GetClipContent(iContent, &pContent) : pViewport->GetContent(iContent, &pContent));

                if (pContent->GetContentElementNoRef() == pContentElement)
                {
                    *ppContent = pContent;

                    // GetContent() or GetClipContent() add-refs, so we'll set pContent to NULL when we want to return the value
                    // since we want to keep that reference around.
                    pContent = NULL;
                }

                ReleaseInterface(pContent);
            }
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::FilterChainedMotions
//
//  Synopsis:
//    Filters out the chained motions that are not included in the provided
//    configuration.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::FilterChainedMotions(
    _In_ XDMConfigurations configuration,
    _Inout_ XDMMotionTypes& chainedMotionTypes)
{
    if ((configuration & XcpDMConfigurationPanX) == XcpDMConfigurationNone &&
        (chainedMotionTypes & XcpDMMotionTypePanX) == XcpDMMotionTypePanX)
    {
        chainedMotionTypes = (XDMMotionTypes)(chainedMotionTypes - XcpDMMotionTypePanX);
    }

    if ((configuration & XcpDMConfigurationPanY) == XcpDMConfigurationNone &&
        (chainedMotionTypes & XcpDMMotionTypePanY) == XcpDMMotionTypePanY)
    {
        chainedMotionTypes = (XDMMotionTypes)(chainedMotionTypes - XcpDMMotionTypePanY);
    }

    if ((configuration & XcpDMConfigurationZoom) == XcpDMConfigurationNone &&
        (chainedMotionTypes & XcpDMMotionTypeZoom) == XcpDMMotionTypeZoom)
    {
        chainedMotionTypes = (XDMMotionTypes)(chainedMotionTypes - XcpDMMotionTypeZoom);
    }
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::HasChainingChildViewport
//
//  Synopsis:
//    Checks if the provided pViewport viewport has a child viewport with chaining turned on.
//    fCheckState == TRUE:  looking for a child viewport in the ManipulationStarting,
//    ManipulationStarted or ManipulationDelta state.
//    fCheckState == FALSE: looking for a child viewport in the XcpDMViewportReady status.
//    Chaining only kicks in when
//      - both viewports have the touch configuration activated.
//      - the child viewport is in Running status.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::HasChainingChildViewport(
    _In_ CDMViewport* pViewport,
    _In_ bool fCheckState,
    _Out_ bool& fHasChainingChildViewport)
{
    HRESULT hr = S_OK;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMMotionTypes chainedMotionTypes = XcpDMMotionTypeNone;
    CDependencyObject* pParentDO = NULL;
    CDMViewport* pViewportTmp = NULL;
    CUIElement* pDMContainer = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  HasChainingChildViewport entry. pViewport=0x%p, IM state=%d.",
            this, pViewport, pViewport->GetState()));
    }
#endif // DM_DEBUG

    fHasChainingChildViewport = FALSE;

    IFCPTR(pViewport);

    // The parent viewport, pViewport, must have the touch configuration activated.
    if (!pViewport->GetIsTouchConfigurationActivated())
    {
        goto Cleanup;
    }

    if (m_pViewports)
    {
        for (XUINT32 iViewport = 0; iViewport < m_pViewports->size(); iViewport++)
        {
            IFC(m_pViewports->get_item(iViewport, pViewportTmp));
            ASSERT(pViewportTmp);

            if (pViewportTmp != pViewport)
            {
                IFC(pViewportTmp->GetCurrentStatus(currentStatus));

                if (fCheckState)
                {
                    // Check#1: is pViewportTmp viewport in ManipulationStarting/ManipulationStarted/ManipulationDelta state?
                    if (pViewportTmp->GetState() != ManipulationStarting &&
                        pViewportTmp->GetState() != ManipulationStarted &&
                        pViewportTmp->GetState() != ManipulationDelta &&
                        pViewportTmp->GetState() != ManipulationLastDelta)
                    {
#ifdef DM_DEBUG
                        if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewport))
                        {
                            XDMViewportStatus statusDbg = XcpDMViewportBuilding;
                            IPALDirectManipulationService* pDirectManipulationServiceDbg = NULL;
                            IGNOREHR(GetDMService(pViewportTmp->GetDMContainerNoRef(), &pDirectManipulationServiceDbg));
                            if (pDirectManipulationServiceDbg)
                            {
                                IGNOREHR(pDirectManipulationServiceDbg->GetViewportStatus(pViewportTmp, statusDbg));
                                ReleaseInterface(pDirectManipulationServiceDbg);

                                if (statusDbg != XcpDMViewportBuilding)
                                {
                                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                        L"                   Non-qualifying child pViewport=0x%p, DM status=%d, IM state=%d, IM status=%d.",
                                        pViewportTmp, statusDbg, pViewportTmp->GetState(), currentStatus));
                                }
                            }
                        }
#endif // DM_DEBUG

                        // Viewport is not active, ignore it.
                        continue;
                    }
                }
                else
                {
                    // Check#1: is pViewportTmp viewport in XcpDMViewportRunning status?
                    if (currentStatus != XcpDMViewportRunning)
                    {
#ifdef DM_DEBUG
                        if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewport))
                        {
                            XDMViewportStatus statusDbg = XcpDMViewportBuilding;
                            IPALDirectManipulationService* pDirectManipulationServiceDbg = NULL;
                            IGNOREHR(GetDMService(pViewportTmp->GetDMContainerNoRef(), &pDirectManipulationServiceDbg));
                            if (pDirectManipulationServiceDbg)
                            {
                                IGNOREHR(pDirectManipulationServiceDbg->GetViewportStatus(pViewportTmp, statusDbg));
                                ReleaseInterface(pDirectManipulationServiceDbg);

                                if (statusDbg != XcpDMViewportBuilding)
                                {
                                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                        L"                   Non-qualifying child pViewport=0x%p, DM status=%d, IM state=%d, IM status=%d != running.",
                                        pViewportTmp, statusDbg, pViewportTmp->GetState(), currentStatus));
                                }
                            }
                        }
#endif // DM_DEBUG

                        // Viewport is not running, ignore it.
                        continue;
                    }
                }

                // Check#2: does pViewportTmp have its touch configuration activated?
                if (!pViewportTmp->GetIsTouchConfigurationActivated())
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewport))
                    {
                        XDMViewportStatus statusDbg = XcpDMViewportBuilding;
                        IPALDirectManipulationService* pDirectManipulationServiceDbg = NULL;
                        IGNOREHR(GetDMService(pViewportTmp->GetDMContainerNoRef(), &pDirectManipulationServiceDbg));
                        if (pDirectManipulationServiceDbg)
                        {
                            IGNOREHR(pDirectManipulationServiceDbg->GetViewportStatus(pViewportTmp, statusDbg));
                            ReleaseInterface(pDirectManipulationServiceDbg);

                            if (statusDbg != XcpDMViewportBuilding)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                    L"                   Non-qualifying child pViewport=0x%p, DM status=%d, IM state=%d, IM status=%d has not activated touch configuration.",
                                    pViewportTmp, statusDbg, pViewportTmp->GetState(), currentStatus));
                            }
                        }
                    }
#endif // DM_DEBUG

                    // Viewport's touch configuration is not active, ignore it.
                    continue;
                }

                // Check#3: has pViewportTmp viewport chaining turned on?
                pDMContainer = pViewportTmp->GetDMContainerNoRef();
                ASSERT(pDMContainer);

                IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));

                IFC(pDirectManipulationContainer->GetManipulationViewport(
                    pViewportTmp->GetManipulatedElementNoRef(),
                    NULL /*pBounds*/,
                    NULL /*pInputTransform*/,
                    NULL /*pTouchConfiguration*/,
                    NULL /*pNonTouchConfiguration*/,
                    NULL /*pBringIntoViewportConfiguration*/,
                    NULL /*pcConfigurations*/,
                    NULL /*ppConfigurations*/,
                    &chainedMotionTypes,
                    NULL /*pHorizontalOverpanMode*/,
                    NULL /*pVerticalOverpanMode*/));
                if (chainedMotionTypes == XcpDMMotionTypeNone)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewport))
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Non-qualifying child pViewport=0x%p: not chaining.", pViewportTmp));
                    }
#endif // DM_DEBUG

                    // Viewport is not chaining, ignore it.
                    ReleaseInterface(pDirectManipulationContainer);
                    continue;
                }

                // Check#4: are the child's chaining motions and manipulatable motions overlapping?
                IFC(FilterChainedMotions(pViewportTmp->GetTouchConfiguration(), chainedMotionTypes));
                if (chainedMotionTypes == XcpDMMotionTypeNone)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewport))
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Non-qualifying child pViewport=0x%p: chaining/configuration non-overlapping.", pViewportTmp));
                    }
#endif // DM_DEBUG

                    // Child's chaining and configuration do not overlap, ignore it.
                    ReleaseInterface(pDirectManipulationContainer);
                    continue;
                }

                // Check#5: are the child's chaining motions overlapping with the parent's manipulatable motions?
                IFC(FilterChainedMotions(pViewport->GetTouchConfiguration(), chainedMotionTypes));
                if (chainedMotionTypes == XcpDMMotionTypeNone)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewport))
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Non-qualifying child pViewport=0x%p: remaining chaining/parent configuration non-overlapping.", pViewportTmp));
                    }

#endif // DM_DEBUG

                    // Child's chaining and parent's configuration do not overlap, ignore it.
                    ReleaseInterface(pDirectManipulationContainer);
                    continue;
                }

                // Check#6: is pViewportTmp viewport a child of the provided one?
                pParentDO = pDMContainer;
                do
                {
                    pParentDO = pParentDO->GetParentInternal();
                    if (pParentDO == pViewport->GetManipulatedElementNoRef())
                    {
#ifdef DM_DEBUG
                        if (m_fIsDMInfoTracingEnabled)
                        {
                            XDMViewportStatus statusDbg = XcpDMViewportBuilding;
                            IPALDirectManipulationService* pDirectManipulationServiceDbg = NULL;
                            IGNOREHR(GetDMService(pViewportTmp->GetDMContainerNoRef(), &pDirectManipulationServiceDbg));
                            if (pDirectManipulationServiceDbg)
                            {
                                IGNOREHR(pDirectManipulationServiceDbg->GetViewportStatus(pViewportTmp, statusDbg));
                                ReleaseInterface(pDirectManipulationServiceDbg);

                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                    L"                   Found a chaining child viewport pViewport=0x%p, DM status=%d, IM state=%d, IM status=%d.",
                                    pViewportTmp, statusDbg, pViewportTmp->GetState(), currentStatus));
                            }
                        }
#endif // DM_DEBUG

                        fHasChainingChildViewport = TRUE;
                        break;
                    }
                }
                while (pParentDO);

                ReleaseInterface(pDirectManipulationContainer);
            }
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::CompleteParentViewports
//
//  Synopsis:
//    Completes the manipulation of any parent viewport with an active state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CompleteParentViewports(
    _In_ CDMViewport* pViewport)
{
    XDMViewportStatus oldStatus = XcpDMViewportBuilding;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    CDependencyObject* pParentDO = NULL;
    CDMViewport* pViewportTmp = NULL;
    CUIElement* pDMContainer = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  CompleteParentViewports entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pViewport);

    if (m_pViewports)
    {
        pDMContainer = pViewport->GetDMContainerNoRef();
        ASSERT(pDMContainer);

        for (XUINT32 iViewport = 0; iViewport < m_pViewports->size(); iViewport++)
        {
            IFC_RETURN(m_pViewports->get_item(iViewport, pViewportTmp));
            ASSERT(pViewportTmp);

            if (pViewportTmp != pViewport)
            {
                IFC_RETURN(pViewportTmp->GetCurrentStatus(currentStatus));

                // Check#0: did the pViewportTmp viewport skip its ManipulationCompleted state?
                if (pViewportTmp->GetIsCompletedStateSkipped())
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && currentStatus != XcpDMViewportBuilding && IsElementInViewport(pDMContainer, pViewportTmp))
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Non-qualifying parent pViewport=0x%p, IM state=%d, IM status=%d: skipped completed state.",
                            pViewportTmp, pViewportTmp->GetState(), currentStatus));
                    }
#endif // DM_DEBUG

                    // pViewportTmp is about to begin a BringIntoViewport operation. Let's not complete it.
                    continue;
                }

                // Check#1: is the pViewportTmp viewport in active state?
                if (pViewportTmp->GetState() != ManipulationStarting &&
                    pViewportTmp->GetState() != ManipulationStarted &&
                    pViewportTmp->GetState() != ManipulationDelta &&
                    pViewportTmp->GetState() != ManipulationLastDelta)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && currentStatus != XcpDMViewportBuilding && IsElementInViewport(pDMContainer, pViewportTmp))
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Non-qualifying parent pViewport=0x%p, IM state=%d, IM status=%d: no active state.",
                            pViewportTmp, pViewportTmp->GetState(), currentStatus));
                    }
#endif // DM_DEBUG

                    // Viewport has not raised manipulation starting, ignore it.
                    continue;
                }

                // Check#2: is the pViewportTmp viewport in XcpDMViewportReady or XcpDMViewportEnabled status?
                if (currentStatus != XcpDMViewportReady &&
                    currentStatus != XcpDMViewportEnabled)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled && currentStatus != XcpDMViewportBuilding && IsElementInViewport(pDMContainer, pViewportTmp))
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Non-qualifying parent pViewport=0x%p, IM state=%d, IM status=%d: not Ready or Enabled status.",
                            pViewportTmp, pViewportTmp->GetState(), currentStatus));
                    }
#endif // DM_DEBUG

                    // Viewport is not in ready or enabled status, ignore it.
                    continue;
                }

                // Check#3: if the pViewportTmp is in the XcpDMViewportReady status but has an Active-to-Inactive status change queued up,
                // the completion is about to be processed in ProcessDirectManipulationViewportStatusUpdate instead.
                if (currentStatus == XcpDMViewportReady)
                {
                    IFC_RETURN(pViewportTmp->GetOldStatus(oldStatus));
                    if (IsViewportActive(oldStatus))
                    {
#ifdef DM_DEBUG
                        if (m_fIsDMInfoTracingEnabled && IsElementInViewport(pDMContainer, pViewportTmp))
                        {
                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                L"                   Non-qualifying parent pViewport=0x%p, IM state=%d, current IM status=%d, old IM status=%d: completion queued up.",
                                pViewportTmp, pViewportTmp->GetState(), currentStatus, oldStatus));
                        }
#endif // DM_DEBUG

                        continue;
                    }
                }

                // Check#4: is the provided pViewport viewport within this pViewportTmp viewport?
                pParentDO = pDMContainer;
                do
                {
                    pParentDO = pParentDO->GetParentInternal();
                    if (pParentDO == pViewportTmp->GetManipulatedElementNoRef())
                    {
                        // Complete manipulation of pViewportTmp
#ifdef DM_DEBUG
                        if (m_fIsDMInfoTracingEnabled)
                        {
                            XUINT32 cContactIdsDbg = 0;
                            IGNOREHR(pViewportTmp->GetContactIdCount(&cContactIdsDbg));
                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                L"                   Found a parent viewport pViewport=0x%p, state=%d, cContactIdsDbg=%d. Completing manipulation.",
                                pViewportTmp, pViewportTmp->GetState(), cContactIdsDbg));
                        }
#endif // DM_DEBUG

                        IFC_RETURN(UnregisterContactIds(pViewportTmp, FALSE /*fReleaseAllContactsAndDisableViewport*/));
                        break;
                    }
                }
                while (pParentDO);
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::EnsureDMServices
//
//  Synopsis:
//    Allocates the m_pDMServices xchainedmap if needed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::EnsureDMServices()
{
    HRESULT hr = S_OK;

    if (!m_pDMServices)
    {
        m_pDMServices = new xchainedmap<CUIElement*, IPALDirectManipulationService*>();
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetDMService
//
//  Synopsis:
//    Returns the DM service for the provided DM container
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetDMService(
    _In_ CUIElement* pDMContainer,
    _Outptr_ IPALDirectManipulationService** ppDirectManipulationService)
{
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    IFCPTR_RETURN(pDMContainer);
    IFCPTR_RETURN(ppDirectManipulationService);
    *ppDirectManipulationService = NULL;

    if (m_pDMServices)
    {
        for (xchainedmap<CUIElement*, IPALDirectManipulationService*>::const_iterator it = m_pDMServices->begin();
            it != m_pDMServices->end();
            ++it)
        {
            if (pDMContainer == (*it).first)
            {
                pDirectManipulationService = (*it).second;
                AddRefInterface(pDirectManipulationService);
                *ppDirectManipulationService = pDirectManipulationService;
                break;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CInputServices::EnsureElementIslandInputSiteForDManipService(_In_ CUIElement* pDMContainer)
{
    wrl::ComPtr<IPALDirectManipulationService> dmanipService;
    IFC_RETURN(GetDMService(pDMContainer, dmanipService.ReleaseAndGetAddressOf()));

    ASSERT(dmanipService);
    IFC_RETURN(dmanipService->EnsureElementIslandInputSite(pDMContainer->GetElementIslandInputSite().Get()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetDirectManipulationViewports
//
//  Synopsis:
//    Called by the UI thread in ::NWDraw to become aware of new
//    DirectManipulation-driven manipulations.
//    Or all existing manipulations when fReturnAllActiveViewports is True.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetDirectManipulationViewports(
    _In_ bool fReturnAllActiveViewports,
    _Out_ xvector<CDMCViewport*>& compositorViewports)
{
    HRESULT hr = S_OK;
    CUIElement* pDMContainer = NULL;
    CUIElement* pContentElement = NULL;
    CDMContent* pContent = NULL;
    CDMViewport* pViewport = NULL;
    CDMCViewport* pCViewport = NULL;
    IObject* pNewCompositorContent = NULL;
    IObject* pNewCompositorViewport = NULL;
    IObject* pCompositorViewport = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;
    IPALDirectManipulationCompositorService* pDirectManipulationCompositorService = NULL;
    xvector<CDMViewport*>::const_iterator itV;
    xvector<CDMViewport*>::const_iterator endV;

    ASSERT(compositorViewports.size() == 0);

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: GetDirectManipulationViewports entry.", this));
    }
#endif // DM_DEBUG

    if (m_pViewports)
    {
        endV = m_pViewports->end();
        for (itV = m_pViewports->begin(); itV != endV; ++itV)
        {
            pViewport = (*itV);
            ASSERT(pViewport);

            if (pViewport->GetHasNewManipulation() ||
                (fReturnAllActiveViewports && !pViewport->GetHasOldManipulation() && pViewport->GetIsCompositorAware()))
            {
                pDMContainer = pViewport->GetDMContainerNoRef();
                ASSERT(pDMContainer);

                IFC(GetDMService(pDMContainer, &pDirectManipulationService));
                ASSERT(pDirectManipulationService);

                IFC(pDirectManipulationService->GetCompositorService(&pDirectManipulationCompositorService));

                if (pViewport->GetHasNewManipulation())
                {
                    // We expect the render walk to visit the manipulated element
                    // well before a manipulation begins, this walk will cause the primary content to be pushed
                    // into the viewport, in GetDirectManipulationServiceAndContent().
                    // In the rare case where a BringIntoViewport call is made right after the viewport is registered,
                    // the primary content can still be null until the next render walk.

                    IFC(pDirectManipulationService->GetCompositorViewport(pViewport, &pNewCompositorViewport));
                    ASSERT(pNewCompositorViewport);
                    ASSERT(!pViewport->GetCompositorViewportNoRef());
                    pViewport->SetCompositorViewport(pNewCompositorViewport);
                    pCompositorViewport = pNewCompositorViewport;
                    ReleaseInterface(pNewCompositorViewport);

                    TraceDmCompositorViewportAddedInfo(
                        (UINT64)pViewport,
                        (UINT64)pCompositorViewport,
                        (UINT64)pDMContainer);

                    pViewport->SetHasNewManipulation(FALSE);

#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"DMIM[0x%p]:  GetDirectManipulationViewports. Calling SetRequiresCompositionNode for pViewport=0x%p, pManipulatedElement=0x%p.", this, pViewport, pViewport->GetManipulatedElementNoRef()));
                    }
#endif // DM_DEBUG

                    IFC(pViewport->GetManipulatedElementNoRef()->SetRequiresComposition(
                        CompositionRequirement::IndependentManipulation,
                        IndependentAnimationType::None
                        ));

                    for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
                    {
                        IFC(pViewport->GetContentElementNoRef(iContent, &pContentElement));
                        ASSERT(pContentElement);
                        IFC(pContentElement->SetRequiresComposition(
                            CompositionRequirement::IndependentManipulation,
                            IndependentAnimationType::None));
                    }

                    for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
                    {
                        IFC(pViewport->GetClipContentElementNoRef(iClipContent, &pContentElement));
                        ASSERT(pContentElement);
                        IFC(pContentElement->SetRequiresComposition(
                            CompositionRequirement::IndependentClipManipulation,
                            IndependentAnimationType::None));
                    }
                }
                else
                {
#ifdef DM_DEBUG
                    if (m_fIsDMVerboseInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                            L"DMIMv[0x%p]: GetDirectManipulationViewports re-submitting viewport to new compositor.", this));
                    }
#endif // DM_DEBUG

                    pCompositorViewport = pViewport->GetCompositorViewportNoRef();
                    ASSERT(pCompositorViewport);

                    // Manipulation might have started before a header is added on the fly.
                    for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
                    {
                        IFC(pViewport->GetContent(iContent, &pContent));
                        if (pContent)
                        {
                            if (!pContent->GetCompositorSecondaryContentNoRef())
                            {
                                IFC(pDirectManipulationService->GetCompositorSecondaryContent(pViewport, pContent, &pNewCompositorContent));
                                ASSERT(pNewCompositorContent);
                                pContent->SetCompositorSecondaryContent(pNewCompositorContent);
                                ReleaseInterface(pNewCompositorContent);
                            }
                            ReleaseInterface(pContent);
                        }
                    }

                    for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
                    {
                        IFC(pViewport->GetClipContent(iClipContent, &pContent));
                        if (pContent)
                        {
                            if (!pContent->GetCompositorSecondaryContentNoRef())
                            {
                                IFC(pDirectManipulationService->GetCompositorSecondaryClipContent(pViewport, pContent, &pNewCompositorContent));
                                ASSERT(pNewCompositorContent);
                                pContent->SetCompositorSecondaryContent(pNewCompositorContent);
                                ReleaseInterface(pNewCompositorContent);
                            }
                            ReleaseInterface(pContent);
                        }
                    }
                }

                pViewport->SetIsCompositorAware(TRUE);

                ReleaseInterface(pDirectManipulationCompositorService);
                ReleaseInterface(pDirectManipulationService);
            }
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationCompositorService);
    ReleaseInterface(pDirectManipulationService);
    ReleaseInterface(pNewCompositorContent);
    ReleaseInterface(pNewCompositorViewport);
    ReleaseInterface(pCViewport);
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetOldDirectManipulationViewport
//
//  Synopsis:
//    Called by the UI thread's ::NWDraw to become aware of
//    DirectManipulation-driven viewports that no longer have a manipulation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetOldDirectManipulationViewport(
    _Out_ IObject** ppCompositorViewport)
{
    HRESULT hr = S_OK;
    XFLOAT translationX = 0.0f;
    XFLOAT translationY = 0.0f;
    XFLOAT uncompressedZoomFactor = 1.0f;
    CUIElement* pContentElement = NULL;
    CDMViewport* pViewport = NULL;
    IObject* pCompositorViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    xvector<CDMViewport*>::const_iterator itV;
    xvector<CDMViewport*>::const_iterator endV;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: GetOldDirectManipulationViewport entry.", this));
    }
#endif // DM_DEBUG

    IFCPTR(ppCompositorViewport);
    *ppCompositorViewport = NULL;

    if (m_pViewports)
    {
        endV = m_pViewports->end();
        for (itV = m_pViewports->begin(); itV != endV; ++itV)
        {
            pViewport = (*itV);
            ASSERT(pViewport);
            if (pViewport->GetHasOldManipulation())
            {
                pViewport->SetHasOldManipulation(FALSE);
                pViewport->SetIsCompositorAware(FALSE);

                SetInterface(pCompositorViewport, pViewport->GetCompositorViewportNoRef());
                break;
            }
        }

        if (pCompositorViewport)
        {
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMIM_DBG) /*traceType*/,
                    L"DMIM[0x%p]:  GetOldDirectManipulationViewport. ThreadID=%d, handling pCompositorViewport=0x%p.", this, pCompositorViewport));
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"DMIM[0x%p]:  GetOldDirectManipulationViewport. Calling UnsetRequiresCompositionNode for pViewport=0x%p, pManipulatedElement=0x%p.", this, pViewport, pViewport->GetManipulatedElementNoRef()));
            }
#endif // DM_DEBUG

            pViewport->GetManipulatedElementNoRef()->UnsetRequiresComposition(
                CompositionRequirement::IndependentManipulation,
                IndependentAnimationType::None
                );
            for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
            {
                IFC(pViewport->GetContentElementNoRef(iContent, &pContentElement));
                ASSERT(pContentElement);
                pContentElement->UnsetRequiresComposition(
                    CompositionRequirement::IndependentManipulation,
                    IndependentAnimationType::None);
            }

            for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
            {
                IFC(pViewport->GetClipContentElementNoRef(iClipContent, &pContentElement));
                ASSERT(pContentElement);
                pContentElement->UnsetRequiresComposition(
                    CompositionRequirement::IndependentClipManipulation,
                    IndependentAnimationType::None);
            }

            if (pViewport->GetNeedsUnregistration())
            {
                IFC(UnregisterViewport(pViewport));
            }
            else
            {
                pViewport->SetCompositorViewport(NULL);

                IFC(pViewport->GetDMContainerNoRef()->GetDirectManipulationContainer(&pDirectManipulationContainer));
                ASSERT(pDirectManipulationContainer);

                TraceDmCompositorViewportRemovedInfo(
                    (UINT64)pViewport,
                    (UINT64)pCompositorViewport,
                    (UINT64)pViewport->GetDMContainerNoRef());

                IFC(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
                    pViewport->GetManipulatedElementNoRef(),
                    FALSE /*fInManipulation*/,
                    FALSE /*fForInitialTransformationAdjustment*/,
                    FALSE /*fForMargins*/,
                    &translationX,
                    &translationY,
                    &uncompressedZoomFactor));

                pViewport->SetCompositorTransformationValues(translationX, translationY, uncompressedZoomFactor);
            }

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"DMIM[0x%p]:  GetOldDirectManipulationViewport returns pCompositorViewport=0x%p.", this, pCompositorViewport));
            }
#endif // DM_DEBUG

            ASSERT(pCompositorViewport);
            *ppCompositorViewport = pCompositorViewport;
            pCompositorViewport = NULL;
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pCompositorViewport);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::InitializeDirectManipulationForPointerId
//
//  Synopsis:
//    Creates viewports as needed, for the provided contact Id
//    Walks up the logical parent tree looking for CUIDMContainer
//    implementations.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::InitializeDirectManipulationForPointerId(
    _In_ XUINT32 pointerId,
    _In_ bool fIsForDMHitTest,
    _In_ CUIElement* pPointedElement,
    _Out_ bool* pContactSuccess)
{
    HRESULT hr = S_OK;
    bool fContactFailure = false;
    bool fUseDM = false;
    CUIElement* pChildElement = pPointedElement;
    CUIElement* pElement = NULL;
    CDependencyObject* pParentDO = pPointedElement;

    *pContactSuccess = false;                       // Will be set true if SetDirectManipulationSetContact succeeds

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  InitializeDirectManipulationForPointerId entry with pointerId=%d, fIsForDMHitTest=%d, pPointedElement=0x%p.",
            this, pointerId, fIsForDMHitTest, pPointedElement));
    }
#endif // DM_DEBUG

    IFCPTR(pPointedElement);

    TraceDmInitializeBegin();

    if (!fIsForDMHitTest)
    {
        IFC(SetDirectManipulationCrossSlideContainer(pointerId, pPointedElement, &fContactFailure));
        ExitOnSetContactFailure(fContactFailure);
    }

    fUseDM = (SystemManipulationModes(pPointedElement->GetManipulationMode()) != DirectUI::ManipulationModes::None);
    if (!fUseDM && fIsForDMHitTest)
    {
        goto Cleanup;
    }

    do
    {
        pParentDO = pParentDO->GetParentInternal();
        if (pParentDO)
        {
            pElement = do_pointer_cast<CUIElement>(pParentDO);
            if (pElement)
            {
                if (fUseDM)
                {
                    if (pElement->GetIsDirectManipulationContainer())
                    {
                        IFC(SetDirectManipulationContact(pointerId, fIsForDMHitTest, pPointedElement, pChildElement, pElement, &fContactFailure));
                        ExitOnSetContactFailure(fContactFailure);
                        *pContactSuccess = true;
                    }

                    fUseDM = (SystemManipulationModes(pElement->GetManipulationMode()) != DirectUI::ManipulationModes::None);

                    // Even if fUseDM is now FALSE, continue the parent walk anyways
                    // in order to detect the potential remaining cross-slide containers.
                    // Unless fIsForDMHitTest is TRUE, in which case the walk can be stopped.
                    if (!fUseDM && fIsForDMHitTest)
                    {
                        break;
                    }
                }

                if (!fIsForDMHitTest)
                {
                    IFC(SetDirectManipulationCrossSlideContainer(pointerId, pElement, &fContactFailure));
                    ExitOnSetContactFailure(fContactFailure);
                }

                pChildElement = pElement;
            }
        }
    }
    while (pParentDO);

    if (!fIsForDMHitTest)
    {
        IFC(StartDirectManipulationCrossSlideContainers());
    }

Cleanup:
    if (!fIsForDMHitTest && (FAILED(hr) || fContactFailure))
    {
        // If an error occurred, discard all the new cross-slide viewports.
        IGNOREHR(CompleteDirectManipulationCrossSlideContainers());
    }

    TraceDmInitializeEnd();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::SetDirectManipulationContact
//
//  Synopsis:
//    Initiates a DirectManipulation manip for the provided CUIDMContainer
//    if the associated viewport is ready.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetDirectManipulationContact(
    _In_ XUINT32 pointerId,
    _In_ bool fIsForDMHitTest,
    _In_ CDependencyObject* pPointedElement,
    _In_ CUIElement* pChildElement,
    _In_ CUIElement* pDMContainer,
    _Out_ bool* pfContactFailure
    )
{
    HRESULT hr = S_OK;
    bool fCanManipulateElementsByTouch = false;
    bool fCanManipulateElementsNonTouch = false;
    bool fCanManipulateElementsWithBringIntoViewport = false;
    bool fProcessedViewportChanges = false;
    XUINT32 cContactIds = 0;
    CUIElement* pManipulatedElement = NULL;
    CDMViewport* pViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;
    XDMViewportStatus oldStatus = XcpDMViewportBuilding;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMViewportStatus status = XcpDMViewportBuilding;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  SetDirectManipulationContact entry with pointerId=%d, fIsForDMHitTest=%d, pDMContainer=0x%p.",
            this, pointerId, fIsForDMHitTest, pDMContainer));
    }
#endif // DM_DEBUG

    IFCPTR(pPointedElement);
    IFCPTR(pChildElement);
    IFCPTR(pDMContainer);
    IFCPTR(pfContactFailure);
    *pfContactFailure = FALSE;

    do
    {
        fProcessedViewportChanges = FALSE;
        // Check if there is a live CDMViewport for this CUIDMContainer
        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
            ASSERT(pDirectManipulationContainer);

            IFC(pDirectManipulationContainer->GetCanManipulateElements(&fCanManipulateElementsByTouch, &fCanManipulateElementsNonTouch, &fCanManipulateElementsWithBringIntoViewport));
            if (!fCanManipulateElementsByTouch)
            {
                goto Cleanup;
            }

            IFC(pDirectManipulationContainer->GetManipulatedElement(
                pPointedElement,
                pChildElement,
                &pManipulatedElement));
            ASSERT(pManipulatedElement);

            if ((pManipulatedElement->GetManipulationMode() & DirectUI::ManipulationModes::System) == DirectUI::ManipulationModes::None)
            {
                // Do not allow a touch-based manipulation when the manipulatable element's ManipulationMode does not include System
                goto Cleanup;
            }

            IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
            ASSERT(pViewport);
            ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

            IFC(pDirectManipulationService->GetViewportStatus(pViewport, status));
            IFC(pViewport->GetOldStatus(oldStatus));
            IFC(pViewport->GetCurrentStatus(currentStatus));

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                XUINT32 cContactIdsDbg = 0;
                IGNOREHR(pViewport->GetContactIdCount(&cContactIdsDbg));
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   pViewport=0x%p, DM status=%d, current IM status=%d, old IM status=%d, IM state=%d, cContactIds=%d.", pViewport, status, currentStatus, oldStatus, pViewport->GetState(), cContactIdsDbg));

                if (status != currentStatus)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   status != currentStatus with IsCompletedStateSkipped=%d.", pViewport->GetIsCompletedStateSkipped()));
                }
            }
#endif // DM_DEBUG

            XDMViewportInteractionType interactionType = XcpDMViewportInteractionBegin;
            if (pViewport->HasQueuedInteractionType())
            {
                interactionType = pViewport->GetBackInteractionType();
            }

            // DM status might be ahead of currentStatus when Ready status is skipped
            if (status == currentStatus &&
                ((status != oldStatus) ||
                 (status != XcpDMViewportReady && pViewport->GetIsCompletedStateSkipped()) ||
                 (interactionType == XcpDMViewportInteractionEnd)))
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Processing changes for ongoing manipulation."));
                }
#endif // DM_DEBUG

                IFC(ProcessDirectManipulationViewportChanges(pViewport));
                // Setting a flag that forces the fCanManipulateElements re-evaluation because the
                // synchronously raised events might have altered the element's manipulability.
                fProcessedViewportChanges = TRUE;

                ReleaseInterface(pManipulatedElement);
                ReleaseInterface(pViewport);
                ReleaseInterface(pDirectManipulationContainer);
                ReleaseInterface(pDirectManipulationService);
            }
        }
    }
    while (fProcessedViewportChanges);

    if (pDirectManipulationService)
    {
        IFC(pViewport->GetContactIdCount(&cContactIds));
        if (cContactIds == 0 && !IsViewportActive(status) && !IsViewportActive(currentStatus))
        {
            if (pViewport->GetState() != ManipulationNone &&
                pViewport->GetState() != ManipulationCompleted &&
                pViewport->GetState() != ConstantVelocityScrollStopped)
            {
                // Complete the ongoing manipulation. This situation arises when a manipulation
                // completion was previously delayed because of a chaining child viewport.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Completing manipulation on new pointer down."));
                }
#endif // DM_DEBUG
                IFC(CompleteDirectManipulation(pViewport, FALSE /*fDisableViewport*/));
            }

            ASSERT(pDirectManipulationContainer);
            IFC(pDirectManipulationContainer->SetPointedElement(pPointedElement));

            IFC(SetupDirectManipulationViewport(
                pDMContainer,
                pManipulatedElement,
                pViewport,
                pDirectManipulationContainer,
                pDirectManipulationService,
                TRUE  /*fIsForTouch*/,
                FALSE /*fIsForBringIntoViewport*/,
                FALSE /*fIsForAnimatedBringIntoViewport*/,
                TRUE  /*fIsForFullSetup*/));

            if (!pViewport->GetIsTouchConfigurationActivated())
            {
                IFC(UpdateManipulationTouchConfiguration(
                    pDirectManipulationService,
                    pViewport,
                    pViewport->GetTouchConfiguration()));
            }

            if (pViewport->GetTouchConfiguration() == XcpDMConfigurationNone)
            {
                // User is touching a header and neither panning nor zooming is allowed.
                goto Cleanup;
            }

            if (pViewport->GetTouchConfiguration() == XcpDMConfigurationInteraction)
            {
                // User is touching an element that is too small compared to its viewport
                // for allowing any panning. The viewport is declared touch-manipulatable
                // anyways so a non-touch manipulation may increase the element's extent(s)
                // and allow touch-base manipulations on the fly.
                goto Cleanup;
            }

            if (!fIsForDMHitTest)
            {
                IFC(UpdateCrossSlideViewportConfigurations(pointerId, pViewport, pfContactFailure));
                ExitOnSetContactFailure(*pfContactFailure);
            }

            IFC(EnableViewport(pDirectManipulationService, pViewport));
            IFC(pDirectManipulationService->SetContact(pViewport, pointerId, pfContactFailure));
            ExitOnSetContactFailure(*pfContactFailure);

            if (fIsForDMHitTest)
            {
                pViewport->SetHasDMHitTestContactId(TRUE /*fHasDMHitTestContactId*/);
            }
            IFC(pViewport->AddContactId(pointerId));
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                XUINT32 cContactIdsDbg = 0;
                IGNOREHR(pViewport->GetContactIdCount(&cContactIdsDbg));
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Added pointerId=%d. New pointerId count is %d.", pointerId, cContactIdsDbg));
            }
#endif // DM_DEBUG

            if (status == XcpDMViewportInertia)
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Added pointerId in Inertia."));
                }
#endif // DM_DEBUG
                pViewport->SetHasReceivedContactIdInInertia(TRUE /*fHasReceivedContactIdInInertia*/);
            }

            IFC(DeclareNewViewportForCompositor(pViewport, pDirectManipulationContainer));

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Calling NotifyManipulationProgress(ManipulationStarting) for pViewport=0x%p, state=%d.", pViewport, pViewport->GetState()));
            }
#endif // DM_DEBUG

            ASSERT(pViewport->GetState() == ManipulationNone ||
                pViewport->GetState() == ManipulationCompleted ||
                pViewport->GetState() == ConstantVelocityScrollStopped);
            pViewport->SetState(ManipulationStarting);
            IFC(pDirectManipulationContainer->NotifyManipulationProgress(
                pManipulatedElement,
                ManipulationStarting,
                0.0f /*xCumulativeTranslation*/,
                0.0f /*yCumulativeTranslation*/,
                1.0f /*zCumulativeFactor*/,
                0.0f /*xInertiaEndTranslation*/,
                0.0f /*yInertiaEndTranslation*/,
                1.0f /*zInertiaEndFactor*/,
                0.0f /*xCenter*/,
                0.0f /*yCenter*/,
                FALSE /*fIsInertiaEndTransformValid*/,
                FALSE /*fIsInertial*/,
                TRUE  /*fIsTouchConfigurationActivated*/,
                FALSE /*fIsBringIntoViewportConfigurationActivated*/));
        }
        else
        {
            if (!fIsForDMHitTest)
            {
                IFC(UpdateCrossSlideViewportConfigurations(pointerId, pViewport, pfContactFailure));
                ExitOnSetContactFailure(*pfContactFailure);
            }

            // Only proceed with setting the contact if we're not in a constant-velocity pan.
            if (status != XcpDMViewportAutoRunning)
            {
                if (!pViewport->GetIsTouchConfigurationActivated())
                {
                    if (pViewport->GetTouchConfiguration() == XcpDMConfigurationInteraction)
                    {
                        // User is touching an element that is too small compared to its viewport for allowing
                        // any touch-based interaction. Do not interrupt the current inertia in this case.
                        // The viewport is declared touch-manipulatable anyways so a non-touch manipulation may
                        // sufficiently increase the element's extent(s) and allow touch-base manipulations on the fly.
                        goto Cleanup;
                    }

                    // Activate the touch configuration since another one was activated
                    IFC(UpdateManipulationTouchConfiguration(
                        pDirectManipulationService,
                        pViewport,
                        pViewport->GetTouchConfiguration()));
                }

                IFC(EnableViewport(pDirectManipulationService, pViewport));
                IFC(pDirectManipulationService->SetContact(pViewport, pointerId, pfContactFailure));
                ExitOnSetContactFailure(*pfContactFailure);

                if (fIsForDMHitTest)
                {
                    pViewport->SetHasDMHitTestContactId(TRUE /*fHasDMHitTestContactId*/);
                }
                IFC(pViewport->AddContactId(pointerId));
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    XUINT32 cContactIdsDbg = 0;
                    IGNOREHR(pViewport->GetContactIdCount(&cContactIdsDbg));
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Added pointerId=%d. New pointerId count is %d.", pointerId, cContactIdsDbg));
                }
#endif // DM_DEBUG

                if (status == XcpDMViewportInertia)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Added pointerId in Inertia."));
                    }
#endif // DM_DEBUG
                    pViewport->SetHasReceivedContactIdInInertia(TRUE /*fHasReceivedContactIdInInertia*/);
                }
            }
#ifdef DM_DEBUG
            else if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Skipping SetContact due to a running constant-velocity pan."));
            }
#endif // DM_DEBUG
        }
    }

Cleanup:
    ReleaseInterface(pManipulatedElement);
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::SetDirectManipulationCrossSlideContainer
//
//  Synopsis:
//    Checks if the provided element requires a cross-slide or rejection
//    cross-slide viewport, and creates it if needed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetDirectManipulationCrossSlideContainer(
    _In_ XUINT32 pointerId,
    _In_ CUIElement* pElement,
    _Out_ bool* pfContactFailure)
{
    XDMConfigurations rejectionViewportConfiguration = XcpDMConfigurationNone;
    bool fIsDraggable = false;

    IFCPTR_RETURN(pfContactFailure);
    *pfContactFailure = FALSE;

    IFC_RETURN(pElement->CanDrag(&fIsDraggable));

    auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pElement);
    if (pElement->GetIsDirectManipulationCrossSlideContainer()
        && !(fIsDraggable && ((pDMCrossSlideService && pDMCrossSlideService->GetHasDragDropViewport()) || !pElement->IsEnabled())))
    {
        // This element needs a cross-slide viewport - it needs a first shot at recognizing gestures or manipulations.
        // Once the first DragDrop CrossSlideContainer is set up, we will treat the rest as regular containers.
        IFC_RETURN(SetDirectManipulationCrossSlideContainer(pointerId, pElement, XcpDMConfigurationNone, pfContactFailure));
    }
    else
    {
        // pElement->GetManipulationMode() is ignored when pElement is a cross-slide container
        if ((pElement->GetManipulationMode() & (DirectUI::ManipulationModes::System | DirectUI::ManipulationModes::Scale)) > DirectUI::ManipulationModes::System)
        {
            // Create a rejection viewport for the provided element, dedicated to blocking DManip zoom manipulations.
            rejectionViewportConfiguration = XcpDMConfigurationZoom;
        }
        else
        {
            if ((pElement->GetManipulationMode() & (DirectUI::ManipulationModes::System | DirectUI::ManipulationModes::TranslateX | DirectUI::ManipulationModes::TranslateRailsX)) > DirectUI::ManipulationModes::System)
            {
                rejectionViewportConfiguration = XcpDMConfigurationPanY;
            }
            if ((pElement->GetManipulationMode() & (DirectUI::ManipulationModes::System | DirectUI::ManipulationModes::TranslateY | DirectUI::ManipulationModes::TranslateRailsY)) > DirectUI::ManipulationModes::System)
            {
                rejectionViewportConfiguration = static_cast<XDMConfigurations>(rejectionViewportConfiguration | XcpDMConfigurationPanX);
            }
        }
        if (rejectionViewportConfiguration != XcpDMConfigurationNone)
        {
            IFC_RETURN(SetDirectManipulationCrossSlideContainer(pointerId, pElement, rejectionViewportConfiguration, pfContactFailure));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::SetDirectManipulationCrossSlideContainer
//
//  Synopsis:
//    Creates and sets up a PAL service and a cross-slide viewport for the element
//    if needed. Records the contact on the viewport.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetDirectManipulationCrossSlideContainer(
    _In_ XUINT32 pointerId,
    _In_ CUIElement* pDMCrossSlideContainer,
    _In_ XDMConfigurations configuration,
    _Out_ bool* pfContactFailure)
{
    HRESULT hr = S_OK;
    bool fCausedRunningStatus = false;
    XUINT32 cContactIds = 0;
    CDMCrossSlideViewport* pNewCrossSlideViewport = NULL;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

    TraceDmSetCrossSlideContainerBegin();

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  SetDirectManipulationCrossSlideContainer entry with pointerId=%d, pDMCrossSlideContainer=0x%p, configuration=%d.",
            this, pointerId, pDMCrossSlideContainer, configuration));
    }
#endif // DM_DEBUG

    IFCPTR(pDMCrossSlideContainer);
    IFCPTR(pfContactFailure);
    *pfContactFailure = FALSE;

    auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pDMCrossSlideContainer);
    if (pDMCrossSlideService)
    {
        // Check if a DM cross-slide viewport was already created for this element
        IFC(GetCrossSlideViewport(pDMCrossSlideContainer, &pCrossSlideViewport));

        if (!pCrossSlideViewport)
        {
            bool fIsDraggable = false;
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Creating new cross-slide viewport."));
            }
#endif // DM_DEBUG

            IFC(CDMCrossSlideViewport::Create(&pNewCrossSlideViewport, pDMCrossSlideContainer));
            ASSERT(pNewCrossSlideViewport);
            IFC(EnsureCrossSlideViewports());
            IFC(m_pCrossSlideViewports->push_back(pNewCrossSlideViewport));
            pCrossSlideViewport = pNewCrossSlideViewport;
            pNewCrossSlideViewport = NULL;

            AddRefInterface(pCrossSlideViewport);

            IFC(pDMCrossSlideContainer->CanDrag(&fIsDraggable));

            if (configuration != XcpDMConfigurationNone || fIsDraggable)
            {
                // Set up a new rejection cross-slide viewport for DManip mixed modes
                pCrossSlideViewport->SetNeedsStart(FALSE);
                pCrossSlideViewport->SetIsRejectionViewport(!fIsDraggable);
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Setting configuration %d of new rejection cross-slide viewport 0x%p.", configuration, pCrossSlideViewport));
                }
#endif // DM_DEBUG

                IFC(pDMCrossSlideService->AddViewportConfiguration(
                    pCrossSlideViewport,
                    TRUE /*fIsCrossSlideViewport*/,
                    fIsDraggable /*fIsDragDrop*/,
                    configuration));
                // fCausedRunningStatus is ignored because we don't track cross-slide viewport statuses.
                IFC(pDMCrossSlideService->EnableViewport(pCrossSlideViewport, fCausedRunningStatus));
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Adding pointerId to new rejection cross-slide viewport."));
                }
#endif // DM_DEBUG

                IFC(AddCrossSlideViewportContactId(pointerId, pCrossSlideViewport, pfContactFailure));
            }
            // Else configuration == XcpDMConfigurationNone ==>
            // Do not setup that new cross-slide viewport yet since we don't know
            // if it's for a horizontal or vertical gesture. The setup and SetContact
            // call will occur later just before the parent viewport calls SetContact.
        }
        else
        {
            if (!pCrossSlideViewport->GetIsRejectionViewport())
            {
                // Do not add the contact Id when the current count is 0 because the cross-slide viewport was not registered with
                // the DManip service in that case. This situation occurs when the owning list control is not manipulatable by touch.
                IFC(pCrossSlideViewport->GetContactIdCount(&cContactIds));
                if (cContactIds > 0)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Adding pointerId to existing cross-slide viewport 0x%p.", pCrossSlideViewport));
                    }
#endif // DM_DEBUG

                    IFC(AddCrossSlideViewportContactId(pointerId, pCrossSlideViewport, pfContactFailure));
                }
            }
            else if (configuration == XcpDMConfigurationZoom)
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Adding pointerId to existing rejection viewport 0x%p.", pCrossSlideViewport));
                }
#endif // DM_DEBUG
                // Registering the new pointer Id with the existing rejection viewport meant to block DManip zoom manipulations.
                IFC(AddCrossSlideViewportContactId(pointerId, pCrossSlideViewport, pfContactFailure));
            }
#ifdef DM_DEBUG
            else if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Skipping pointerId for existing rejection cross-slide viewport 0x%p.", pCrossSlideViewport));
            }
#endif // DM_DEBUG
        }
    }

Cleanup:
    ReleaseInterface(pNewCrossSlideViewport);
    ReleaseInterface(pCrossSlideViewport);

    TraceDmSetCrossSlideContainerEnd();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::AddCrossSlideViewportContactId
//
//  Synopsis:
//    Calls SetContact for the provided pointerId and cross-slide viewport
//    and registers that pointerId.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::AddCrossSlideViewportContactId(
    _In_ XUINT32 pointerId,
    _In_ CDMCrossSlideViewport* pCrossSlideViewport,
    _Out_ bool* pfContactFailure)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  AddCrossSlideViewportContactId entry with pointerId=%d, pCrossSlideViewport=0x%p.", this, pointerId, pCrossSlideViewport));
    }
#endif // DM_DEBUG

    IFCPTR(pCrossSlideViewport);
    IFCPTR(pfContactFailure);
    *pfContactFailure = FALSE;

    auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pCrossSlideViewport->GetDMContainerNoRef());
    ASSERT(pDMCrossSlideService);

    IFC(pDMCrossSlideService->SetContact(pCrossSlideViewport, pointerId, pfContactFailure));
    ExitOnSetContactFailure(*pfContactFailure);

    IFC(pCrossSlideViewport->AddContactId(pointerId));
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        XUINT32 cContactIdsDbg = 0;
        IGNOREHR(pCrossSlideViewport->GetContactIdCount(&cContactIdsDbg));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Added pointerId=%d. New pointerId count on cross-slide viewport is %d.", pointerId, cContactIdsDbg));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::SetupDirectManipulationViewport
//
//  Synopsis:
//    Configures a DM viewport either on pointer down or when an input
//    message is forwarded to DM. Configures the viewport bounds,
//    chaining mode, primary content, configuration mode, snap points
//    and accesses the initial transformation values.
//    Only does a partial setup when fIsForFullSetup is FALSE because
//    the DM container is sending notifications while
//    pViewport->GetUITicksForNotifications() > 0.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetupDirectManipulationViewport(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ CDMViewport* pViewport,
    _In_ CUIDMContainer* pDirectManipulationContainer,
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ bool fIsForTouch,
    _In_ bool fIsForBringIntoViewport,
    _In_ bool fIsForAnimatedBringIntoViewport,
    _In_ bool fIsForFullSetup,
    _Out_opt_ bool* pCancelOperation)
{
    XDMConfigurations activatedConfiguration = XcpDMConfigurationNone;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  SetupDirectManipulationViewport entry. pViewport=0x%p, fIsForTouch=%d, fIsForBringIntoViewport=%d, fIsForFullSetup=%d.",
            this, pViewport, fIsForTouch, fIsForBringIntoViewport, fIsForFullSetup));
    }
#endif // DM_DEBUG

    ASSERT(!(fIsForTouch && fIsForBringIntoViewport));
    ASSERT(!(fIsForAnimatedBringIntoViewport && !fIsForBringIntoViewport));

    if (pCancelOperation != nullptr)
    {
        *pCancelOperation = false;
    }

    IFCPTR_RETURN(pDMContainer);
    IFCPTR_RETURN(pManipulatedElement);
    IFCPTR_RETURN(pViewport);
    IFCPTR_RETURN(pDirectManipulationContainer);
    IFCPTR_RETURN(pDirectManipulationService);

    IFC_RETURN(pDirectManipulationService->DisableViewport(pViewport));

    if (fIsForFullSetup)
    {
        IFC_RETURN(UpdateManipulationViewport(
            pDMContainer,
            pManipulatedElement,
            TRUE  /*fUpdateBounds*/,
            TRUE  /*fUpdateInputTransform*/,
            TRUE  /*fUpdateTouchConfiguration*/,
            TRUE  /*fUpdateNonTouchConfiguration*/,
            FALSE /*fUpdateConfigurations*/,
            !fIsForBringIntoViewport || fIsForAnimatedBringIntoViewport /*fUpdateChainedMotionTypes*/,
            fIsForTouch  /*fActivateTouchConfiguration*/,
            !fIsForTouch && !fIsForBringIntoViewport /*fActivateNonTouchConfiguration*/,
            fIsForBringIntoViewport /*fActivateBringIntoViewConfiguration*/,
            FALSE /*fUpdateHorizontalOverpanMode*/,
            FALSE /*fUpdateVerticalOverpanMode*/,
            NULL  /*pfConfigurationsUpdated*/));
    }

    IFC_RETURN(UpdateManipulationPrimaryContent(
        pDMContainer,
        pManipulatedElement,
        FALSE /*fUpdateLayoutRefreshed*/,
        TRUE  /*fUpdateBounds*/,
        fIsForFullSetup /*fUpdateHorizontalAlignment*/,
        fIsForFullSetup /*fUpdateVerticalAlignment*/,
        fIsForFullSetup /*fUpdateZoomFactorBoundary*/,
        pCancelOperation));

    if (pCancelOperation != nullptr && *pCancelOperation)
    {
        // *pCancelOperation was set to True within the UpdateManipulationPrimaryContent update. Skip the remaining updates below.
        return S_OK;
    }

    if (fIsForFullSetup)
    {
        // Skipping the snap points update when this setup is for a BringIntoViewport operation without animation (where snap points are ignored)
        if (!fIsForBringIntoViewport || fIsForAnimatedBringIntoViewport)
        {
            if (fIsForTouch)
            {
                activatedConfiguration = pViewport->GetTouchConfiguration();
            }
            else if (fIsForBringIntoViewport)
            {
                activatedConfiguration = pViewport->GetBringIntoViewportConfiguration();
            }
            else
            {
                activatedConfiguration = pViewport->GetNonTouchConfiguration();
            }

            if ((activatedConfiguration & XcpDMConfigurationPanX) != 0)
            {
                IFC_RETURN(UpdateManipulationSnapPoints(
                    pDMContainer,
                    pManipulatedElement,
                    XcpDMMotionTypePanX));
            }
            if ((activatedConfiguration & XcpDMConfigurationPanY) != 0)
            {
                IFC_RETURN(UpdateManipulationSnapPoints(
                    pDMContainer,
                    pManipulatedElement,
                    XcpDMMotionTypePanY));
            }
            if ((activatedConfiguration & XcpDMConfigurationZoom) != 0)
            {
                IFC_RETURN(UpdateManipulationSnapPoints(
                    pDMContainer,
                    pManipulatedElement,
                    XcpDMMotionTypeZoom));
            }
        }
    }

    // Set the initial and current transformation values using the provided DManip service. The 0, 0, 1 transform is ignored here.
    IFC_RETURN(InitializeDirectManipulationViewportValues(
        pViewport,
        pDirectManipulationService,
        0.0f /*initialTranslationX*/,
        0.0f /*initialTranslationY*/,
        1.0f /*initialZoomFactor*/));

    if (fIsForFullSetup)
    {
        // Set up the parametric curves for overpan behavior.
        // Note that this must be done after the content rect has been set by UpdateManipulationPrimaryContent(),
        // and after InitializeDirectManipulationViewportValues() has synchronized initial translation values.
        IFC_RETURN(UpdateManipulationOverpanModes(
            pDirectManipulationService,
            pViewport,
            TRUE /*fIsStartingNewManipulation*/));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DisableViewport
//
//  Synopsis:
//    Disables the viewport associated to the provided DM container
//    and manipulated element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DisableViewport(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement)
{
    HRESULT hr = S_OK;
    CDMViewport* pViewport = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
    if (pViewport)
    {
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  DisableViewport - pViewport=0x%p.", this, pViewport));
        }
#endif // DM_DEBUG

        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDirectManipulationService->DisableViewport(pViewport));
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::UnregisterCrossSlideViewportContactId
//
//  Synopsis:
//    Loops through all the cross-slide viewports and removes the provided pointerId
//    if recorded. Unregisters the viewports with no remaining contacts.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UnregisterCrossSlideViewportContactId(
    _In_ XUINT32 pointerId)
{
    HRESULT hr = S_OK;
    XUINT32 cContactIds = 0;
    XUINT32 cCrossSlideViewports = 0;
    bool fContainsContactId = false;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  UnregisterCrossSlideViewportContactId with pointerId=%d.", this, pointerId));
    }
#endif // DM_DEBUG

    if (m_pCrossSlideViewports)
    {
        cCrossSlideViewports = m_pCrossSlideViewports->size();
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   CrossSlide Viewport count=%d.", cCrossSlideViewports));
        }
#endif // DM_DEBUG

        for (XUINT32 iCrossSlideViewport = 0; iCrossSlideViewport < cCrossSlideViewports; iCrossSlideViewport++)
        {
            IFC(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewport));
            IFC(pCrossSlideViewport->ContainsContactId(pointerId, &fContainsContactId));
            if (fContainsContactId)
            {
                IFC(pCrossSlideViewport->RemoveContactId(pointerId));
                IFC(pCrossSlideViewport->GetContactIdCount(&cContactIds));
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   pCrossSlideViewport=0x%p, Removed pointerId=%d, New contactId count=%d.",
                        pCrossSlideViewport, pointerId, cContactIds));
                }
#endif // DM_DEBUG

                if (cContactIds == 0)
                {
                    IFC(UnregisterCrossSlideViewport(pCrossSlideViewport));
                    cCrossSlideViewports--;
                    iCrossSlideViewport--;
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UnregisterContactIds
//
//  Synopsis:
//    Called to unregister all contact IDs and complete the current
//    manipulation for the provided viewport.
//    When fReleaseAllContactsAndDisableViewport is set, the DManip
//    DisableViewport and ReleaseAllContacts methods are also called.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UnregisterContactIds(_In_ CDMViewport* pViewport, _In_ bool fReleaseAllContactsAndDisableViewport)
{
    HRESULT hr = S_OK;
    XUINT32 contactId = 0;
    XUINT32 cContactIds = 0;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  UnregisterContactIds pViewport=0x%p, fReleaseAllContactsAndDisableViewport=%d.",
            this, pViewport, fReleaseAllContactsAndDisableViewport));
    }
#endif // DM_DEBUG

    if (fReleaseAllContactsAndDisableViewport && pViewport->GetDMContainerNoRef())
    {
        IFC(GetDMService(pViewport->GetDMContainerNoRef(), &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDirectManipulationService->DisableViewport(pViewport));
            IFC(pDirectManipulationService->ReleaseAllContacts(pViewport));
        }
    }

    IFC(pViewport->GetContactIdCount(&cContactIds));

    if (cContactIds > 0)
    {
        if (m_pViewports)
        {
            do
            {
                IFC(pViewport->GetContactId(0, &contactId));
                IFC(UnregisterContactId(contactId, pViewport /*pExclusiveViewport*/, TRUE /*fCompleteManipulation*/));
                IFC(pViewport->GetContactIdCount(&cContactIds));
            }
            while (cContactIds > 0);
        }
    }
    else
    {
        // Complete the manipulation by updating the viewport state, the DM container and the compositor
        IFC(CompleteDirectManipulation(pViewport, FALSE /*fDisableViewport*/));
    }

Cleanup:
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::UnregisterContactId
//
//  Synopsis:
//    Removes the provided contact Id from the pExclusiveViewport viewport
//    or all viewports when pExclusiveViewport is NULL.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UnregisterContactId(
    _In_ XUINT32 pointerId,
    _In_opt_ CDMViewport* pExclusiveViewport,
    _In_ bool fCompleteManipulation)
{
    XUINT32 cContactIds = 0;
    XUINT32 cViewports = 0;
    bool fContainsContactId = false;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CDMViewport* pViewport = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  UnregisterContactId with pointerId=%d, pExclusiveViewport=0x%p, fCompleteManipulation=%d.",
            this, pointerId, pExclusiveViewport, fCompleteManipulation));
    }
#endif // DM_DEBUG

    if (m_pViewports)
    {
        cViewports = m_pViewports->size();
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Viewport count=%d.", cViewports));
        }
#endif // DM_DEBUG

        for (XUINT32 iViewport = 0; iViewport < cViewports; iViewport++)
        {
            IFC_RETURN(m_pViewports->get_item(iViewport, pViewport));
            if (pExclusiveViewport == NULL || pExclusiveViewport == pViewport)
            {
                IFC_RETURN(pViewport->ContainsContactId(pointerId, &fContainsContactId));
                if (fContainsContactId)
                {
                    IFC_RETURN(pViewport->RemoveContactId(pointerId));
                    IFC_RETURN(pViewport->GetContactIdCount(&cContactIds));
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   pViewport=0x%p, Removed pointerId=%d, New contactId count=%d.",
                            pViewport, pointerId, cContactIds));
                    }
#endif // DM_DEBUG

                    if (cContactIds == 0)
                    {
                        IFC_RETURN(pViewport->GetCurrentStatus(status));
                        if (status != XcpDMViewportRunning && status != XcpDMViewportInertia)
                        {
                            if (status == XcpDMViewportAutoRunning)
                            {
#ifdef DM_DEBUG
                                if (m_fIsDMInfoTracingEnabled)
                                {
                                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                        L"                   Delaying manipulation completion during constant velocity pan for pViewport=0x%p.", pViewport));
                                }
#endif // DM_DEBUG

                                pViewport->SetIsCompletedStateDelayedByConstantVelocityPan(TRUE);
                            }
                            else
                            {
                                if (pViewport != pExclusiveViewport || fCompleteManipulation)
                                {
                                    if (pViewport->GetIsCompletedStateSkipped())
                                    {
#ifdef DM_DEBUG
                                        if (m_fIsDMInfoTracingEnabled)
                                        {
                                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                                L"                   Skipping CompleteDirectManipulation because IsCompletedStateSkipped was set. Resetting flag."));
                                        }
#endif // DM_DEBUG

                                        pViewport->SetIsCompletedStateSkipped(FALSE);
                                    }
                                    else
                                    {
                                        IFC_RETURN(CompleteDirectManipulation(pViewport, FALSE /*fDisableViewport*/));
                                    }
                                }
                                else if (!pViewport->GetIsCompletedStateSkipped())
                                {
                                    pViewport->DeclareOldViewportForCompositor();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::UnregisterCrossSlideViewport
//
//  Synopsis:
//    Unregisters a cross-slide viewport with DM, discards the DM PAL service,
//    and removes it from the m_pCrossSlideViewports hashtable.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UnregisterCrossSlideViewport(
    _In_ CDMCrossSlideViewport* pCrossSlideViewport)
{
    CDMCrossSlideViewport* pCrossSlideViewportTmp = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  UnregisterCrossSlideViewport pCrossSlideViewport=0x%p - entry.", this, pCrossSlideViewport));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pCrossSlideViewport);

    if (m_pCrossSlideViewports)
    {
        auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pCrossSlideViewport->GetDMContainerNoRef());
        ASSERT(pDMCrossSlideService);

        for (XUINT32 iCrossSlideViewport = 0; iCrossSlideViewport < m_pCrossSlideViewports->size(); iCrossSlideViewport++)
        {
            IFC_RETURN(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewportTmp));
            ASSERT(pCrossSlideViewportTmp);

            if (pCrossSlideViewportTmp == pCrossSlideViewport)
            {
                IFC_RETURN(pDMCrossSlideService->UnregisterViewport(pCrossSlideViewport));

                IFC_RETURN(m_pCrossSlideViewports->erase(iCrossSlideViewport));
                ReleaseInterface(pCrossSlideViewport);

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Unregistering done."));
                }
#endif // DM_DEBUG

                break;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::UnregisterViewport
//
//  Synopsis:
//    Unregisters the viewport with the DM PAL service.
//    Removes the viewport from the internal m_pViewports xvector
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UnregisterViewport(
    _In_ CDMViewport* pViewport)
{
    HRESULT hr = S_OK;
    CDMViewport* pViewportTmp = NULL;
    CDMContent* pContent = NULL;
    CDMViewport* pNewOwningViewport = NULL;
    CUIElement* pDMContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  UnregisterViewport pViewport=0x%p - entry.", this, pViewport));
    }
#endif // DM_DEBUG

    IFCPTR(pViewport);

    if (m_pViewports)
    {
        for (XUINT32 iViewport = 0; iViewport < m_pViewports->size(); iViewport++)
        {
            IFC(m_pViewports->get_item(iViewport, pViewportTmp));
            ASSERT(pViewportTmp);

            if (pViewportTmp == pViewport)
            {
                pDMContainer = pViewport->GetDMContainerNoRef();
                ASSERT(pDMContainer);

                TraceDmCompositorViewportRemovedInfo(
                    (UINT64)pViewport,
                    (UINT64)pViewport->GetCompositorViewportNoRef(),
                    (UINT64)pDMContainer);

                IFC(GetDMService(pDMContainer, &pDirectManipulationService));
                ASSERT(pDirectManipulationService);

                xref_ptr<IDirectManipulationViewport> spDMViewport;
                IFC(pDirectManipulationService->GetDirectManipulationViewport(pViewport, spDMViewport.ReleaseAndGetAddressOf()));
                IFC(ProcessDeferredReleaseQueue(spDMViewport));
                spDMViewport = nullptr;

                // Take care to remove DManip's transform before we actually unregister the viewport.
                // DManip requires we do this before abandoning the DM Viewport.  Not doing so now
                // would cause an error downstream for the HWCompTreeNode when it tries to remove the transform.
                IFC(pDirectManipulationService->ReleaseSharedContentTransform(pViewport->GetCompositorPrimaryContentNoRef(), XcpDMContentTypePrimary));

                for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
                {
                    IFC(pViewport->GetContent(iContent, &pContent));
                    if (pContent)
                    {
                        IFC(RemoveSecondaryContent(pContent->GetContentElementNoRef(), pContent, pViewport, pDirectManipulationService));
                        ReleaseInterface(pContent);
                        iContent--;
                    }
                }

                for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
                {
                    IFC(pViewport->GetClipContent(iClipContent, &pContent));
                    if (pContent)
                    {
                        IFC(RemoveSecondaryClipContent(pContent->GetContentElementNoRef(), pContent, pViewport, pDirectManipulationService));
                        ReleaseInterface(pContent);
                        iClipContent--;
                    }
                }

                if (pViewport->HasViewportInteraction())
                {
                    IFC(pDirectManipulationService->RemoveViewportInteraction(pViewport));
                    pViewport->SetHasViewportInteraction(false);
                }
                IFC(pDirectManipulationService->UnregisterViewport(pViewport));

                pViewport->SetCompositorViewport(NULL);
                pViewport->SetCompositorPrimaryContent(NULL);

                xref_ptr<CUIElement> pManipulatedElement(pViewport->GetManipulatedElementNoRef());

                if (pManipulatedElement && pViewport->GetIsCompositorAware())
                {
                    ASSERT(pManipulatedElement->IsManipulatedIndependently());
                    pManipulatedElement->UnsetRequiresComposition(
                        CompositionRequirement::IndependentManipulation,
                        IndependentAnimationType::None);
                }

                IFC(m_pViewports->erase(iViewport));
                pViewport->SetUnregistered();
                ReleaseInterface(pViewport);

                if (pManipulatedElement && pManipulatedElement->IsManipulatable())
                {
                    IFC(GetViewport(
                        NULL /*pDMContainer*/,
                        pManipulatedElement,
                        &pNewOwningViewport));
                    if (!pNewOwningViewport)
                    {
                        // Mark the element as no longer manipulatable
                        pManipulatedElement->UnsetRequiresComposition(
                            CompositionRequirement::Manipulatable,
                            IndependentAnimationType::None);

                        // The RemoveSecondaryContent calls above already marked the potential secondary contents as no longer manipulatable.
                    }
                    // else the manipulated element already belongs to a new viewport, which is assumed to be manipulatable.
                }

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Unregistering done."));
                }
#endif // DM_DEBUG

                break;
            }
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    ReleaseInterface(pNewOwningViewport);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::EnableViewport
//
//  Synopsis:
//    Enables the provided viewport. Increments the viewport's m_cRemovedRunningStatuses
//    count if enabling caused a transitional Running status.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::EnableViewport(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport)
{
    bool fCausedRunningStatus = false;

    IFCPTR_RETURN(pDirectManipulationService);
    IFCPTR_RETURN(pViewport);

    if (!pViewport->HasValidBounds())
    {
        // If we enable the viewport before setting the viewport bounds, DManip will use the bounds of the
        // input hwnd.  This isn't valid when in XamlOneCoreTransforms mode.  To avoid this, we set the bounds first
        // to the empty bounds.
        IFC_RETURN(SetViewportBounds(pDirectManipulationService, pViewport, {} /* empty bounds */));
    }

    IFC_RETURN(pDirectManipulationService->EnableViewport(pViewport, fCausedRunningStatus));
    if (fCausedRunningStatus)
    {
        // Expect transitions to the Running, Ready, Disabled, Enabled statuses.
        // Those transitions will be pulled out in ProcessDirectManipulationViewportStatusUpdate.
        pViewport->IncrementRemovedRunningStatuses();
    }

    return S_OK;
}

// Set the bounds of the given viewport
_Check_return_ HRESULT
CInputServices::SetViewportBounds(
        _In_ IPALDirectManipulationService* directManipulationService,
        _In_ CDMViewport* viewport,
        _In_ const XRECTF& bounds) const
{
    IFC_RETURN(directManipulationService->SetViewportBounds(viewport, bounds));
    viewport->SetHasValidBounds(true);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DeleteDMViewports
//
//  Synopsis:
//    Removes the viewports from the internal m_pViewports xvector.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DeleteDMViewports()
{
    HRESULT hr = S_OK;
    CDMViewport* pViewport = NULL;

    if (m_pViewports)
    {
        while (m_pViewports->size() > 0)
        {
            IFC(m_pViewports->get_item(0, pViewport));
            ASSERT(pViewport);

            IFC(UnregisterViewport(pViewport));
        }
    }

Cleanup:
    delete m_pViewports;
    m_pViewports = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DeleteDMCrossSlideViewports
//
//  Synopsis:
//    Removes the viewports from the internal m_pCrossSlideViewports xvector.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DeleteDMCrossSlideViewports()
{
    HRESULT hr = S_OK;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

    if (m_pCrossSlideViewports)
    {
        while (m_pCrossSlideViewports->size() > 0)
        {
            IFC(m_pCrossSlideViewports->get_item(0, pCrossSlideViewport));
            ASSERT(pCrossSlideViewport);
            IFC(m_pCrossSlideViewports->erase(0));

            ReleaseInterface(pCrossSlideViewport);
        }
    }

Cleanup:
    delete m_pCrossSlideViewports;
    m_pCrossSlideViewports = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DeleteSecondaryContentRelationshipsToBeApplied
//
//  Synopsis:
//    Deletes the secondary content relationships to be applied and the m_pSecondaryContentRelationshipsToBeApplied xvector
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DeleteSecondaryContentRelationshipsToBeApplied()
{
    HRESULT hr = S_OK;
    CSecondaryContentRelationship* pSecondaryContentRelationship = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  DeleteSecondaryContentRelationshipsToBeApplied.", this));
    }
#endif // DM_DEBUG

    if (m_pSecondaryContentRelationshipsToBeApplied)
    {
        while (m_pSecondaryContentRelationshipsToBeApplied->size() > 0)
        {
            IFC(m_pSecondaryContentRelationshipsToBeApplied->get_item(0, pSecondaryContentRelationship));
            ASSERT(pSecondaryContentRelationship);
            IFC(m_pSecondaryContentRelationshipsToBeApplied->erase(0));

            ReleaseInterface(pSecondaryContentRelationship);
        }
    }

Cleanup:
    delete m_pSecondaryContentRelationshipsToBeApplied;
    m_pSecondaryContentRelationshipsToBeApplied = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DeleteDMServices
//
//  Synopsis:
//    Removes the IPALDirectManipulationService implementations from the
//    internal m_pDMServices xchainedmap. Release both CUIElement and
//    DM services.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DeleteDMServices()
{
    CUIElement* pDMContainer = NULL;
    IPALDirectManipulationService* pDMService = NULL;

    if (m_pDMServices)
    {
        for (xchainedmap<CUIElement*, IPALDirectManipulationService*>::const_iterator it = m_pDMServices->begin();
            it != m_pDMServices->end();
            ++it)
        {
            pDMContainer = (*it).first;
            pDMService = (*it).second;
            ReleaseInterface(pDMContainer);
            ReleaseInterface(pDMService);
        }
        m_pDMServices->Clear();
    }

    delete m_pDMServices;
    m_pDMServices = NULL;
    RRETURN(S_OK);
}

void CInputServices::ResetSharedDManipCompositor()
{
    m_DMServiceSharedState->ResetSharedDCompManipulationCompositor();
}

// Re-creates the compositor objects inside DManip, used by test hook only.
_Check_return_ HRESULT CInputServices::ResetDManipCompositors()
{
    m_DMServiceSharedState->ResetSharedDCompManipulationCompositor();
    if (m_pDMServices)
    {
        for (xchainedmap<CUIElement*, IPALDirectManipulationService*>::const_iterator it = m_pDMServices->begin();
            it != m_pDMServices->end();
            ++it)
        {
            IFC_RETURN(it->second->ResetCompositor());
        }
    }

    return S_OK;
}

void CInputServices::DeleteDMContainersNeedingInitialization()
{
#ifdef DBG
    if (m_pDMContainersNeedingInitialization)
    {
        for (auto& elem : *m_pDMContainersNeedingInitialization)
        {
            ASSERT(elem);
        }
    }
#endif
    m_pDMContainersNeedingInitialization.reset();
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::GetDirectManipulationViewportEventHandler
//
//  Synopsis:
//      Returns a IXcpDirectManipulationViewportEventHandler implementation
//      for the IPALDirectManipulationService implementation to
//      use for forwarding DM's notifications
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetDirectManipulationViewportEventHandler(
    _Outptr_ IXcpDirectManipulationViewportEventHandler** ppDirectManipulationViewportEventHandler)
{
    CInputManagerDMViewportEventHandler* pDirectManipulationViewportEventHandler = NULL;

    IFCPTR_RETURN(ppDirectManipulationViewportEventHandler);
    *ppDirectManipulationViewportEventHandler = NULL;

    IFC_RETURN(CInputManagerDMViewportEventHandler::Create(&pDirectManipulationViewportEventHandler, this));
    *ppDirectManipulationViewportEventHandler = static_cast<IXcpDirectManipulationViewportEventHandler*>(pDirectManipulationViewportEventHandler);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   InitializeDirectManipulationViewportValues
//
//  Synopsis:
//      Caches the initial primary content transformation values
//      for the provided viewport.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::InitializeDirectManipulationViewportValues(
    _In_ CDMViewport* pViewport,
    _In_opt_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ XFLOAT initialTranslationX,
    _In_ XFLOAT initialTranslationY,
    _In_ XFLOAT initialZoomFactor)
{
    HRESULT hr = S_OK;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;
    XFLOAT translationX = 0.0f;
    XFLOAT translationY = 0.0f;
    XFLOAT uncompressedZoomFactor = 1.0f;
    XFLOAT zoomFactorX = 1.0f;
    XFLOAT zoomFactorY = 1.0f;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  InitializeDirectManipulationViewportValues entry - pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    IFCPTR(pViewport);

    if (pDirectManipulationService)
    {
        IFC(pDirectManipulationService->GetPrimaryContentTransform(
            pViewport,
            translationX,
            translationY,
            uncompressedZoomFactor,
            zoomFactorX,
            zoomFactorY));
    }
    else
    {
        // Use provided values for instance for edge scrolling scenarios.
        translationX = initialTranslationX;
        translationY = initialTranslationY;
        uncompressedZoomFactor = initialZoomFactor;
        zoomFactorX = initialZoomFactor;
        zoomFactorY = initialZoomFactor;
    }

    // Store the current content offsets so GetDirectManipulationTransform
    // can take them into account when evaluating transforms for the UI thread.
    pViewport->GetContentOffsets(
        contentOffsetX, contentOffsetY);
    pViewport->SetInitialContentOffsets(
        contentOffsetX, contentOffsetY);

    pViewport->SetInitialTransformationValues(
        translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);

    pViewport->SetCurrentTransformationValues(
        translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"                   Initial DM translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf",
            translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
#endif // DM_DEBUG

    // Do the same initialization for each secondary content.
    for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
    {
        IFC(pViewport->GetContent(iContent, &pContent));
        if (pContent)
        {
            if (pDirectManipulationService)
            {
                IFC(pDirectManipulationService->GetSecondaryContentTransform(
                    pViewport,
                    pContent,
                    pContent->GetContentType(),
                    translationX,
                    translationY,
                    uncompressedZoomFactor,
                    zoomFactorX,
                    zoomFactorY));
            }
            else
            {
                // Use provided values for instance for edge scrolling scenarios.
                switch (pContent->GetContentType())
                {
                case XcpDMContentTypeTopLeftHeader:
                    translationX = 0.0f;
                    translationY = 0.0f;
                    break;
                case XcpDMContentTypeTopHeader:
                    translationX = initialTranslationX;
                    translationY = 0.0f;
                    break;
                case XcpDMContentTypeLeftHeader:
                    translationX = 0.0f;
                    translationY = initialTranslationY;
                    break;
                case XcpDMContentTypeCustom:
                case XcpDMContentTypeDescendant:
                    translationX = 0.0f;
                    translationY = 0.0f;
                    break;
                }
            }

            pContent->SetInitialTransformationValues(
                translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);

            pContent->SetCurrentTransformationValues(
                translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Initial transform for secondary pContent=0x%p, translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.",
                    pContent, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
            }
#endif // DM_DEBUG

            ReleaseInterface(pContent);
        }
    }

    for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
    {
        IFC(pViewport->GetClipContent(iClipContent, &pContent));
        if (pContent)
        {
            IFC(pDirectManipulationService->GetSecondaryClipContentTransform(
                pViewport,
                pContent,
                pContent->GetContentType(),
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));

            pContent->SetInitialTransformationValues(
                translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);

            pContent->SetCurrentTransformationValues(
                translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Initial transform for secondary pContent=0x%p, translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.",
                    pContent, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
            }
#endif // DM_DEBUG

            ReleaseInterface(pContent);
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessInputMessageWithDirectManipulation
//
//  Synopsis:
//    Called when the DM container wants the InputManager to process the current
//    input message, by forwarding it to DirectManipulation.
//    The fHandled flag must be set to True if the message was handled.
//
//    Forwards an input message to DirectManipulation for processing. Key
//    messages that DM is interested in are:
//       - mouse wheel messages
//       - arrow keys
//       - page up / page down keys
//       - home / end keys
//       - ctrl +/- for zooming
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessInputMessageWithDirectManipulation(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool ignoreFlowDirection,
    _In_ CContentRoot* contentRoot,
    _Out_ bool& fHandled)
{
    HRESULT hr = S_OK;
    bool fCanManipulateElementsByTouch = false;
    bool fCanManipulateElementsNonTouch = false;
    bool fCanManipulateElementsWithBringIntoViewport = false;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CDMViewport* pViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessInputMessageWithDirectManipulation entry - ignoreFlowDirection=%d.",
            this, ignoreFlowDirection));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    if (contentRoot->GetInputManager().GetCurrentMsgForDirectManipulationProcessing() == nullptr)
    {
        goto Cleanup;
    }

    // Access the DM service associated to this DM container.
    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    if (pDirectManipulationService)
    {
        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        ASSERT(pDirectManipulationContainer);

        IFC(pDirectManipulationContainer->GetCanManipulateElements(&fCanManipulateElementsByTouch, &fCanManipulateElementsNonTouch, &fCanManipulateElementsWithBringIntoViewport));
        if (!fCanManipulateElementsNonTouch)
        {
            goto Cleanup;
        }

        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
        ASSERT(pViewport);
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        switch (pViewport->GetState())
        {
        case ManipulationStarting:
        case ManipulationStarted:
        case ManipulationDelta:
        {
            if (pViewport->GetIsTouchConfigurationActivated())
            {
                // Do not forward message to DM when a touch-based manipulation is in progress
                goto Cleanup;
            }
        }
        }

        IFC(pDirectManipulationService->GetViewportStatus(pViewport, status));

#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   pViewport=0x%p, IM state=%d, DM status=%d.", pViewport, pViewport->GetState(), status));
        }
#endif // DM_DEBUG

        if (pViewport->GetIsProcessingMakeVisibleInertia())
        {
            bool inertiaStopped = false;
            IFC(StopInertialViewport(pViewport, true /*restrictToKnownInertiaEnd*/, &inertiaStopped));
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Called StopInertialViewport during bring-into-viewport inertia - inertiaStopped=%d.",
                    inertiaStopped));
            }
#endif // DM_DEBUG
            if (!inertiaStopped && pViewport->GetIsProcessingMakeVisibleInertia())
            {
                // Do not forward this message to DManip in this rare case. It is still performing the animation for a
                // MakeVisible call and inertia could not be stopped. This avoids the viewport landing on a random transform.
                goto Cleanup;
            }
        }

        if (!IsViewportActive(status) && pViewport->GetUITicksForNotifications() == 0)
        {
            IFC(SetupDirectManipulationViewport(
                pDMContainer,
                pManipulatedElement,
                pViewport,
                pDirectManipulationContainer,
                pDirectManipulationService,
                FALSE /*fIsForTouch*/,
                FALSE /*fIsForBringIntoViewport*/,
                FALSE /*fIsForAnimatedBringIntoViewport*/,
                TRUE  /*fIsForFullSetup*/));
        }
        else
        {
            if (!IsViewportActive(status) &&
                (pViewport->GetState() == ManipulationCompleted || pViewport->GetState() == ConstantVelocityScrollStopped))
            {
                // A partial setup is required while pViewport->GetUITicksForNotifications() > 0.
                IFC(SetupDirectManipulationViewport(
                    pDMContainer,
                    pManipulatedElement,
                    pViewport,
                    pDirectManipulationContainer,
                    pDirectManipulationService,
                    FALSE /*fIsForTouch*/,
                    FALSE /*fIsForBringIntoViewport*/,
                    FALSE /*fIsForAnimatedBringIntoViewport*/,
                    FALSE /*fIsForFullSetup*/));
            }
            if (!pViewport->GetIsNonTouchConfigurationActivated())
            {
                // Activate the non-touch configuration since another one was activated
                IFC(UpdateManipulationNonTouchConfiguration(
                    pDirectManipulationService,
                    pViewport,
                    pViewport->GetNonTouchConfiguration()));
            }
        }

        // Whether the message is handled by DM or not, the viewport was properly
        // setup. So let the DM container know about our interest in DM change
        // notifications and reset the countdown
        IFC(SetDirectManipulationHandlerWantsNotifications(
            pViewport,
            pManipulatedElement,
            pDirectManipulationContainer,
            TRUE /*fWantsNotifications*/));

        // Make sure to tick the UI thread so the RefreshDirectManipulationHandlerWantsNotifications method gets called
        IFC(RequestAdditionalFrame());

        InputMessage* currentMsgForDirectManipulationProcessing = contentRoot->GetInputManager().GetCurrentMsgForDirectManipulationProcessing();

        FAIL_FAST_ASSERT(currentMsgForDirectManipulationProcessing->m_hPlatformPacket);

        // Forward input message to DirectManipulation for processing
        IFC(EnableViewport(pDirectManipulationService, pViewport));

        IFC(pDirectManipulationService->ProcessInput(
            pViewport,
            currentMsgForDirectManipulationProcessing->m_hPlatformPacket,
            currentMsgForDirectManipulationProcessing->m_msgID,
            currentMsgForDirectManipulationProcessing->m_bIsSecondaryMessage,
            !ignoreFlowDirection && pDMContainer->IsRightToLeft() /*fInvertForRightToLeft*/,
            pViewport->GetNonTouchConfiguration(),
            fHandled));

        if (fHandled)
        {
            // Let the compositor know about the new manipulation, if it's unaware of this viewport.
            IFC(DeclareNewViewportForCompositor(pViewport, pDirectManipulationContainer));

            // Enter the ManipulationStarting state if no manipulation is already in process.
            switch (pViewport->GetState())
            {
            case ManipulationNone:
            case ManipulationCompleted:
            case ConstantVelocityScrollStopped:
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Calling NotifyManipulationProgress(ManipulationStarting) for pViewport=0x%p.", pViewport));
                }
#endif // DM_DEBUG

                pViewport->SetState(ManipulationStarting);
                IFC(pDirectManipulationContainer->NotifyManipulationProgress(
                    pManipulatedElement,
                    ManipulationStarting,
                    0.0f /*xCumulativeTranslation*/,
                    0.0f /*yCumulativeTranslation*/,
                    1.0f /*zCumulativeFactor*/,
                    0.0f /*xInertiaEndTranslation*/,
                    0.0f /*yInertiaEndTranslation*/,
                    1.0f /*zInertiaEndFactor*/,
                    0.0f /*xCenter*/,
                    0.0f /*yCenter*/,
                    FALSE /*fIsInertiaEndTransformValid*/,
                    FALSE /*fIsInertial*/,
                    FALSE /*fIsTouchConfigurationActivated*/,
                    FALSE /*fIsBringIntoViewportConfigurationActivated*/));
                break;
            }
            }

            // In the case of a mouse scroll, we want to replay the most recent pointer update after the DM inertia is done.
            // The pointer could be over a new element after the content is done scrolling.
            if (currentMsgForDirectManipulationProcessing->m_msgID == XCP_POINTERWHEELCHANGED)
            {
                // The viewport was set up to handle a mouse wheel scroll for a ScrollViewer. Mark it to do another hit test once
                // the scroll completes.
                pViewport->SetRequestReplayPointerUpdateWhenInertiaCompletes(TRUE);
            }
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   SetDirectManipulationHandlerWantsNotifications
//
//  Synopsis:
//    Called when the input manager wants to tell a DM container that it
//    is interested in knowing any DM-influencing change, or is no longer
//    interested.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetDirectManipulationHandlerWantsNotifications(
    _In_ CDMViewport* pViewport,
    _In_ CUIElement* pManipulatedElement,
    _In_ CUIDMContainer* pDirectManipulationContainer,
    _In_ bool fWantsNotifications)
{
    IFCPTR_RETURN(pViewport);
    IFCPTR_RETURN(pManipulatedElement);

    IFCPTR_RETURN(pDirectManipulationContainer);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        if (fWantsNotifications)
        {
            if (pViewport->GetUITicksForNotifications() == 0)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"DMIM[0x%p]:  SetDirectManipulationHandlerWantsNotifications starts countdown for pViewport=0x%p, IM state=%d.", this, pViewport, pViewport->GetState()));
            }
        }
        else
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"DMIM[0x%p]:  SetDirectManipulationHandlerWantsNotifications resets countdown for pViewport=0x%p, IM state=%d.", this, pViewport, pViewport->GetState()));
        }
    }
#endif // DM_DEBUG

    pViewport->SetUITicksForNotifications(fWantsNotifications ? 1 : 0);
    IFC_RETURN(pDirectManipulationContainer->SetManipulationHandlerWantsNotifications(
        pManipulatedElement,
        fWantsNotifications));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RefreshDirectManipulationHandlerWantsNotifications
//
//  Synopsis:
//    Called on each UI tick to update the flag on each DM container that
//    determines whether the input manager is interested in knowing any
//    DM-influencing change.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RefreshDirectManipulationHandlerWantsNotifications()
{
    HRESULT hr = S_OK;
    XUINT32 cUITicksForNotifications = 0;
    CDMViewport* pViewport = NULL;
    CUIElement* pDMContainer = NULL;
    xvector<CDMViewport*>::const_iterator itV;
    xvector<CDMViewport*>::const_iterator endV;
    CUIDMContainer* pDirectManipulationContainer = NULL;

    IFCPTR(m_pCoreService);

    if (m_pViewports)
    {
        endV = m_pViewports->end();
        for (itV = m_pViewports->begin(); itV != endV; ++itV)
        {
            pViewport = (*itV);
            ASSERT(pViewport);

            cUITicksForNotifications = pViewport->GetUITicksForNotifications();

            if (cUITicksForNotifications > 0)
            {
                if (cUITicksForNotifications == UITicksThresholdForNotifications)
                {
                    pDMContainer = pViewport->GetDMContainerNoRef();
                    ASSERT(pDMContainer);

                    ReleaseInterface(pDirectManipulationContainer);
                    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
                    ASSERT(pDirectManipulationContainer);
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"DMIM[0x%p]:  RefreshDirectManipulationHandlerWantsNotifications stops countdown for pViewport=0x%p, IM state=%d.", this, pViewport, pViewport->GetState()));
                    }
#endif // DM_DEBUG

                    IFC(SetDirectManipulationHandlerWantsNotifications(
                        pViewport,
                        pViewport->GetManipulatedElementNoRef(),
                        pDirectManipulationContainer,
                        FALSE /*fWantsNotifications*/));
                }
                else
                {
                    pViewport->SetUITicksForNotifications(cUITicksForNotifications + 1);

                    // Keep ticking while actively listening to characteristic notifications.
#ifdef DBG
                    ITickableFrameScheduler *pFrameScheduler = m_pCoreService->GetBrowserHost()->GetFrameScheduler();
                    ASSERT(pFrameScheduler != NULL && pFrameScheduler->IsInTick());
#endif // DBG
                    IFC(RequestAdditionalFrame());
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   RequestAdditionalFrame
//
//  Synopsis:
//    Makes sure the compositor renders an additional frame by calling
//    ITickableFrameScheduler::RequestAdditionalFrame.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
CInputServices::RequestAdditionalFrame()
{
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = m_pCoreService->GetBrowserHost();
    if (pBH)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::InputManager));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportChanges
//
//  Synopsis:
//    Called at each UI tick to handle any potential viewport status or
//    transform updates.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportChanges()
{
    XUINT32 iViewport = 0;
    CDMViewport* pViewport = NULL;

    if (m_pViewports)
    {
        while (iViewport < m_pViewports->size())
        {
            IFC_RETURN(m_pViewports->get_item(iViewport, pViewport));
            ASSERT(pViewport);

            IFC_RETURN(ProcessDirectManipulationViewportChanges(
                pViewport));

            iViewport++;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportChanges
//
//  Synopsis:
//    Handles any potential viewport status or transform updates.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportChanges(
    _In_ CDMViewport* pViewport)
{
    static const XUINT32 StatusChangesForIntermediaryStatus = 2;

    bool fHasActiveStatus = false;
    bool fIgnoreStatusChange = false;
    bool fManipulationCompleted = false;
    XDMViewportStatus oldStatus, currentStatus;

    IFCPTR_RETURN(m_pCoreService);
    IFCPTR_RETURN(pViewport);

    if (!pViewport->GetNeedsUnregistration())
    {
        if (pViewport->GetStatusesCount() > StatusChangesForIntermediaryStatus)
        {
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessDirectManipulationViewportChanges with intermediary status. pViewport=0x%p.", this, pViewport));
#ifdef DBG
                pViewport->DebugStatuses();
#endif // DBG
            }
#endif // DM_DEBUG

            // Workaround for bug 689141. Occasionally DM sends an extraneous Ready status in between two active statuses.
            IFC_RETURN(pViewport->HasActiveStatus(fHasActiveStatus));
            IFC_RETURN(pViewport->GetOldStatus(oldStatus));
            IFC_RETURN(pViewport->GetCurrentStatus(currentStatus));

            if (!pViewport->GetHasDelayedStatusChangeProcessing() && // if the processing was already delayed, do not delay it any further
                !IsViewportActive(oldStatus) &&                      // viewport started tick period in a non-active status
                fHasActiveStatus &&                                  // viewport momentarily went to an active status
                currentStatus == XcpDMViewportReady &&               // viewport is back to the Ready status
                pViewport->GetIgnoredRunningStatuses() == 0)         // No Running status was pre-queued because of a synchronous BringIntoViewport call
            {
                // Delaying processing to see if the viewport is going back to an active status shortly.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Delaying viewport changes because of Inactive Active Ready status sequence."));
                }
#endif // DM_DEBUG

                pViewport->SetHasDelayedStatusChangeProcessing(TRUE);

                IFC_RETURN(RequestAdditionalFrame());
                return S_OK;
            }
            // End workaround
        }

        if (pViewport->GetHasDelayedStatusChangeProcessing())
        {
            // Clear the delaying flag since this viewport is now processing its status changes
            pViewport->SetHasDelayedStatusChangeProcessing(FALSE);
        }

        for (XUINT32 statusIndex = 0;
            (statusIndex == 0) || (pViewport->GetStatusesCount() > 1 && statusIndex < pViewport->GetStatusesCount() - 1);
            statusIndex++)
        {
            IFC_RETURN(pViewport->GetStatus(statusIndex, oldStatus));
            if (statusIndex + 1 < pViewport->GetStatusesCount())
            {
                IFC_RETURN(pViewport->GetStatus(statusIndex + 1, currentStatus));
            }
            else
            {
                // Status has not changed during this UI tick
                ASSERT(pViewport->GetStatusesCount() <= 1);
                currentStatus = oldStatus;
            }

            ASSERT(!(oldStatus == currentStatus && pViewport->GetStatusesCount() > 1));

            fIgnoreStatusChange = FALSE;

            if (oldStatus != currentStatus)
            {
                if (currentStatus != XcpDMViewportAutoRunning && oldStatus != XcpDMViewportAutoRunning)
                {
                    TraceDmViewportStatusUpdateBegin((UINT64) pViewport, oldStatus, currentStatus);

                    IFC_RETURN(ProcessDirectManipulationViewportStatusUpdate(
                        pViewport,
                        oldStatus,
                        currentStatus,
                        TRUE /*fIsValuesChangePreProcessing*/,
                        &fIgnoreStatusChange,
                        &fManipulationCompleted));

                    TraceDmViewportStatusUpdateEnd((UINT64)pViewport);

                    // Raise any queued up DirectManipulationStarted event before raising any ViewChanging/ViewChanged event.
                    IFC_RETURN(RaiseQueuedDirectManipulationStateChanges(
                        pViewport,
                        true  /*isValuesChangePreProcessing*/,
                        false /*isValuesChangePostProcessing*/));
                }
                else
                {
                    // currentStatus == XcpDMViewportAutoRunning || oldStatus == XcpDMViewportAutoRunning
                    IFC_RETURN(ProcessConstantVelocityViewportStatusUpdate(
                        pViewport,
                        currentStatus));
                }
            }

            if (fIgnoreStatusChange)
            {
                // When ProcessDirectManipulationViewportStatusUpdate returns fIgnoreStatusChange=True, the new status is ignored and removed from the queue.

                // Processing for example a transitional Running status that must be ignored. Take it out of the statuses queue.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Ignoring status change from %d to %d and taking it out of the queue.", oldStatus, currentStatus));
#ifdef DBG
                    pViewport->DebugStatuses();
#endif // DBG
                }
#endif // DM_DEBUG

                ASSERT(statusIndex + 1 < pViewport->GetStatusesCount());
                IFC_RETURN(pViewport->RemoveStatus(statusIndex + 1));

                if (statusIndex + 1 < pViewport->GetStatusesCount())
                {
                    // Refresh the current status now that the ignored one was removed.
                    IFC_RETURN(pViewport->GetStatus(statusIndex + 1, currentStatus));
                    if (oldStatus == currentStatus)
                    {
                        // Also remove the duplicate new current status
                        IFC_RETURN(pViewport->RemoveStatus(statusIndex + 1));
                    }
                }
            }
            else
            {
                // CDMViewport's state can already be ManipulationCompleted in rare cases where
                // CInputServices::CompleteDirectManipulation was called in chaining viewports or
                // canceled manipulation scenarios. Skip ProcessDirectManipulationViewportValuesUpdate in those cases.
                if (pViewport->GetState() != ManipulationCompleted &&
                    ((oldStatus != XcpDMViewportSuspended && currentStatus == XcpDMViewportSuspended) ||
                    fManipulationCompleted ||
                    currentStatus == XcpDMViewportInertia ||
                    currentStatus == XcpDMViewportRunning ||
                    currentStatus == XcpDMViewportAutoRunning))
                {
                    IFC_RETURN(ProcessDirectManipulationViewportValuesUpdate(
                        pViewport,
                        oldStatus == XcpDMViewportInertia /*fWasInertial*/,
                        currentStatus == XcpDMViewportInertia || (fManipulationCompleted && oldStatus == XcpDMViewportInertia) /*fIsInertial*/,
                        fManipulationCompleted && !pViewport->GetIsCompletedStateSkipped() /*fIsLastDelta*/));
                }

                ASSERT((oldStatus == XcpDMViewportAutoRunning && currentStatus == XcpDMViewportAutoRunning) ||
                    !(oldStatus == currentStatus && pViewport->GetStatusesCount() > 1));

                if (oldStatus != currentStatus)
                {
                    if (currentStatus != XcpDMViewportAutoRunning && oldStatus != XcpDMViewportAutoRunning)
                    {
                        // Note this ProcessDMViewportStatusUpdate call will detect when the manipulation
                        // is completed and declare the viewport as old for such cases.
                        IFC_RETURN(ProcessDirectManipulationViewportStatusUpdate(
                            pViewport,
                            oldStatus,
                            currentStatus,
                            FALSE /*fIsValuesChangePreProcessing*/,
                            NULL  /*pfIgnoreStatusChange*/,
                            NULL  /*pfManipulationCompleted*/));
                        // Raise any queued up DirectManipulationCompleted event after raising any ViewChanging/ViewChanged event.
                        IFC_RETURN(RaiseQueuedDirectManipulationStateChanges(
                            pViewport,
                            false /*isValuesChangePreProcessing*/,
                            true  /*isValuesChangePostProcessing*/));
                    }
                }
            }
            if ((oldStatus != currentStatus && (pViewport->GetStatusesCount() == 1 || statusIndex == pViewport->GetStatusesCount() - 2)) ||
                (oldStatus == currentStatus && fIgnoreStatusChange && statusIndex == pViewport->GetStatusesCount() - 1)) // Check if the trailing status changes were ignored and taken out of the queue.
            {
                // The last status change just got processed - clear the statuses and declare
                // the latest current status as the old status for the next UI tick.
                IFC_RETURN(pViewport->SetOldStatus(currentStatus));
            }
        }

        // If the viewport is active, request another tick to continue updating the UI thread tree
        // in response to the ongoing DM changes.
        IFC_RETURN(pViewport->GetCurrentStatus(currentStatus));
        if (IsViewportActive(currentStatus))
        {
            IFC_RETURN(RequestAdditionalFrame());
        }

        // Check if a pointer Id was registered while the viewport was in inertia phase and did not prolong the manipulation.
        if (pViewport->HasQueuedInteractionType() &&
            pViewport->GetFrontInteractionType() == XcpDMViewportInteractionEnd &&
            pViewport->GetState() == ManipulationStarting &&
            currentStatus == XcpDMViewportEnabled)
        {
            // A WM_POINTERDOWN was received while a viewport was in inertia phase. No transition to an active status occurred for pViewport and no WM_POINTERUP is received.
            // Thus pViewport would remain in the ManipulationStarting state. Complete the current manipulation by unregistering all known contact Ids for this pViewport and
            // entering the ManipulationCompleted state.
            IFC_RETURN(UnregisterContactIds(pViewport, FALSE /*fReleaseAllContactsAndDisableViewport*/));
        }

        // Raise any queued up DirectManipulationStarted/DirectManipulationCompleted event that may be left over.
        // For example a DirectManipulationStarted after a switch to the touch-based configuration, or
        // DirectManipulationCompleted when tapping a viewport.
        IFC_RETURN(RaiseQueuedDirectManipulationStateChanges(
            pViewport,
            false /*isValuesChangePreProcessing*/,
            false /*isValuesChangePostProcessing*/));
    }

    return S_OK;
}

// Called by OnPostUIThreadTick at each UI tick to stop any viewport that may
// be in inertia phase while its manipulated element has no composition peer.
// This processing is performed after CCoreServices::NWDrawTree so that
// required composition peers are present.
_Check_return_ HRESULT
CInputServices::StopInertialViewportsWithoutCompositorPeer()
{
    if (m_pViewports != nullptr)
    {
        XUINT32 iViewport = 0;
        while (iViewport < m_pViewports->size())
        {
            CDMViewport* pViewportNoRef = nullptr;
            IFC_RETURN(m_pViewports->get_item(iViewport, pViewportNoRef));
            ASSERT(pViewportNoRef);

            IFC_RETURN(StopInertialViewportWithoutCompositorPeer(pViewportNoRef));

            iViewport++;
        }
    }

    return S_OK;
}

// Called during OnPostUIThreadTick at each UI tick to stop the provided viewport in
// case it is in inertia phase while its manipulated element has no composition peer.
// This processing is performed after CCoreServices::NWDrawTree so that the composition
// peer's presence is up-to-date.
_Check_return_ HRESULT
CInputServices::StopInertialViewportWithoutCompositorPeer(
    _In_ CDMViewport* pViewport)
{
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;

    IFC_RETURN(pViewport->GetCurrentStatus(currentStatus));

    if (currentStatus == XcpDMViewportInertia && pViewport->GetManipulatedElementNoRef()->GetCompositionPeer() == nullptr)
    {
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"DMIM[0x%p]:  StopInertialViewportWithoutCompositorPeer invokes StopInertialViewport for pViewport=0x%p.", this, pViewport));
        }
#endif // DM_DEBUG

        // The viewport is in the Inertia phase and it does not have a composition peer.
        // Immediately jump to the end-of-inertia transform and complete the manipulation since there
        // are no shared transforms for this viewport.
        IFC_RETURN(StopInertialViewport(pViewport, false /*restrictToKnownInertiaEnd*/, nullptr));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportStatusUpdate
//
//  Synopsis:
//    Called as soon as the status of the provided viewport changed.
//    This method is called outside of the tick, but the processing of that change
//    will happen at the next UI tick.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportStatusUpdate(
    _In_ CDMViewport* pViewport,
    _In_ XDMViewportStatus oldStatus,
    _In_ XDMViewportStatus newStatus)
{
    HRESULT hr = S_OK;
    bool fIsNewStatusActive = IsViewportActive(newStatus);
    bool fHasActiveStatus = false;
    XDMViewportStatus firstStatus = XcpDMViewportBuilding;
    XDMViewportStatus previousStatus = XcpDMViewportBuilding;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    IObject* pDragDropViewportHandle = nullptr;

    IFCPTR(m_pCoreService);
    IFCPTR(pViewport);

    TraceDmViewportStatusInfo((UINT64)pViewport, oldStatus, newStatus);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessDirectManipulationViewportStatusUpdate entry. pViewport=0x%p, oldStatus=%d, newStatus=%d.",
            this, pViewport, oldStatus, newStatus));
        if (m_fIsDMVerboseInfoTracingEnabled)
        {
#ifdef DBG
            pViewport->DebugStatuses();
#endif // DBG
        }
    }
#endif // DM_DEBUG

    auto* pDMCrossSlideService = GetDMCrossSlideServiceNoRefForUIElement(pViewport->GetDMContainerNoRef());
    if (pDMCrossSlideService)
    {
        IFC(pDMCrossSlideService->GetDragDropViewport(&pDragDropViewportHandle));
        if (pViewport == pDragDropViewportHandle)
        {
            // Drag drop viewport doesn't handle Viewport status update
            goto Cleanup;
        }
        else if ((pDragDropViewportHandle != nullptr) && (IsViewportActive(oldStatus)) && (newStatus == XcpDMViewportReady) && (pViewport->GetRemovedRunningStatuses() == 0))
        {
            CDMCrossSlideViewport* pDragDropViewport = static_cast<CDMCrossSlideViewport*>(pDragDropViewportHandle);
            // if the panning viewport takes over, discard drag and drop viewport.
            // Ignore the transition to the Ready status though when the previous status was not active, or the Running status was only transitional.
            IFC(DirectManipulationCrossSlideContainerCompleted(pDragDropViewport->GetDMContainerNoRef(), pDragDropViewport));
        }
    }

    // Check if the incoming Running status needs to be ignored. The m_cRemovedRunningStatuses count takes precedence though:
    // a Enabled->Running->Ready->Disabled sequence may need to be removed first before a subsequent Running status is ignored.
    if (newStatus == XcpDMViewportRunning &&
        pViewport->GetRemovedRunningStatuses() == 0 &&
        pViewport->GetIgnoredRunningStatuses() > 0)
    {
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Ignoring Running status because of positive IgnoredRunningStatuses count."));
#ifdef DBG
            pViewport->DebugStatuses();
#endif // DBG
        }
#endif // DM_DEBUG

        pViewport->DecrementIgnoredRunningStatuses();
        if (pViewport->GetIgnoredRunningStatuses() == 0)
        {
            // Now that all ignored transitions to the Running status were processed, refresh the DMData structures associated with this viewport
            IFC(DirtyDirectManipulationTransforms(pViewport));
        }
        goto Cleanup;
    }

    IFC(pViewport->GetCurrentStatus(currentStatus));

    // Workaround for DM bug 689141.
    // Ignore momentary transition into the Ready status, during a single UI tick
    // to avoid declaring the manipulation complete erroneously.
    IFC(pViewport->HasActiveStatus(fHasActiveStatus));
    if (currentStatus == XcpDMViewportReady &&  // latest status queued up is the Ready status
        fHasActiveStatus &&                     // an active status was already queued up during this UI tick
        fIsNewStatusActive)                     // incoming status is active
    {
        if (pViewport->GetIsCompletedStateSkipped())
        {
            IFC(pViewport->GetOldStatus(firstStatus));
            if (firstStatus == XcpDMViewportReady && pViewport->GetIsCompletedStateSkipped())
            {
                // The first and skipped Ready status is ignored
                // Ready Active1 Ready Active2 ==> Active1 Ready Active2
                // Ready Active1 Ready Active1 ==> Active1 Ready Active1
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Pulling out first & skipped Ready status."));
#ifdef DBG
                    pViewport->DebugStatuses();
#endif // DBG
                }
#endif // DM_DEBUG

                IFC(pViewport->RemoveOldStatus());
            }
        }

        // The temporary Ready status is ignored
        // Active1 Ready Active2 ==> Active1 Active2
        // Active1 Ready Active1 ==> Active1
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Pulling out latest Ready status."));
#ifdef DBG
            pViewport->DebugStatuses();
#endif // DBG
        }
#endif // DM_DEBUG

        IFC(pViewport->RemoveCurrentStatus());

        if (pViewport->GetIsCompletedStateSkipped())
        {
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Resetting IsCompletedStateSkipped flag."));
            }
#endif // DM_DEBUG

            pViewport->SetIsCompletedStateSkipped(FALSE);
        }
    }

    // Pulling out Running status if it's squeezed in between Inertia and Inertia/Ready in a single UI tick.
    // These situations occur occasionally during non-touch operations and we do not want to transition to
    // the touch configuration.
    if (currentStatus == XcpDMViewportRunning &&   // latest status queued up is the Running status
        pViewport->GetStatusesCount() > 1 &&       // there is another status queued up before that Running status
        (!fIsNewStatusActive || newStatus == XcpDMViewportInertia)) // incoming status is inactive (like Ready) or Inertia
    {
        IFC(pViewport->GetStatus(pViewport->GetStatusesCount() - 2, previousStatus));
        if (previousStatus == XcpDMViewportInertia) // the previous status is Inertia
        {
            if (pViewport->GetHasReceivedContactIdInInertia())
            {
                // Clear the flag stating a contact Id was received during an inertia phase. The Running status is legitimate and
                // must be handled. The Inertia Running transition will unregister all known contact Ids.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Resetting HasReceivedContactIdInInertia flag."));
                }
#endif // DM_DEBUG
                pViewport->SetHasReceivedContactIdInInertia(FALSE);
            }
            else
            {
                // The temporary Running status is ignored
                // Inertia Running Inertia ==> Inertia
                // Inertia Running Ready ==> Inertia Ready
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Pulling out latest Running status."));
#ifdef DBG
                    pViewport->DebugStatuses();
#endif // DBG
                }
#endif // DM_DEBUG
                IFC(pViewport->RemoveCurrentStatus());
            }
        }
    }

    // Handle the rare situation where a call to IDirectManipulationViewport::Enable() causes a temporary
    // transition to the Running status, and results in a final Ready status. The DM service then disables
    // and re-enables the viewport to end up in the expected Enabled status. Remove all the superfluous
    // status transitions, especially the Running one.
    if (pViewport->GetRemovedRunningStatuses() && // viewport expects a transitional Running status
        currentStatus == XcpDMViewportDisabled && // latest status queued up is the Disabled status
        pViewport->GetStatusesCount() > 3 &&      // there are another 3 statuses queued up before that Disabled status
        newStatus == XcpDMViewportEnabled)        // incoming status is Enabled
    {
        IFC(pViewport->GetStatus(pViewport->GetStatusesCount() - 2, previousStatus));
        if (previousStatus == XcpDMViewportReady)
        {
            IFC(pViewport->GetStatus(pViewport->GetStatusesCount() - 3, previousStatus));
            if (previousStatus == XcpDMViewportRunning)
            {
                IFC(pViewport->GetStatus(pViewport->GetStatusesCount() - 4, previousStatus));
                if (previousStatus == XcpDMViewportEnabled)
                {
                    // The status sequence is Building ==> Enabled ==> Running ==> Ready ==> Disabled ==> Enabled
                    // or Disabled ==> Enabled ==> Running ==> Ready ==> Disabled ==> Enabled.
                    // Pull out the four last queued statuses.
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Pulling out latest Enabled->Running->Ready->Disabled statuses."));
#ifdef DBG
                        pViewport->DebugStatuses();
#endif // DBG
                    }
#endif // DM_DEBUG

                    IFC(pViewport->RemoveCurrentStatus()); // Pull out Disabled status
                    IFC(pViewport->RemoveCurrentStatus()); // Pull out Ready status
                    IFC(pViewport->RemoveCurrentStatus()); // Pull out Running status
                    IFC(pViewport->RemoveCurrentStatus()); // Pull out Enabled status
                    pViewport->DecrementRemovedRunningStatuses(); // Decrease the expected count since the removal is fulfilled.
                }
            }
        }
    }

    if (oldStatus == XcpDMViewportInertia && pViewport->GetIsProcessingMakeVisibleInertia())
    {
        // The viewport exited the inertia status previously triggered by a call to MakeVisible. Clear its
        // m_fIsProcessingMakeVisibleInertia flag so CInputServices::ProcessInputMessageWithDirectManipulation
        // no longer needs to stop inertia.
        pViewport->SetIsProcessingMakeVisibleInertia(FALSE);
    }

    IFC(pViewport->GetCurrentStatus(currentStatus));
    if (newStatus != currentStatus)
    {
        if (newStatus == XcpDMViewportRunning && pViewport->GetIsTouchInteractionEndExpected() && !pViewport->GetIsTouchInteractionStartProcessed())
        {
            // A touch-based interaction has triggered a manipulation.
            // Make sure any subsequent transition to the Running status for this current interaction does not queue up another start notification.
            pViewport->SetIsTouchInteractionStartProcessed(TRUE);
            // Queue up the notification for handling at the next UI thread tick in RaiseQueuedDirectManipulationStateChanges.
            pViewport->PushInteractionType(XcpDMViewportInteractionManipulation);
        }

        IFC(pViewport->SetCurrentStatus(newStatus));
    }

    // Request a UI thread tick so we can start process the newly activated viewport.
    if (fIsNewStatusActive)
    {
        IFC(RequestAdditionalFrame());
    }

Cleanup:
    ReleaseInterface(pDragDropViewportHandle);

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"                   ProcessDirectManipulationViewportStatusUpdate exit."));
#ifdef DBG
        if (pViewport != nullptr)
        {
            pViewport->DebugStatuses();
        }
#endif // DBG
    }
#endif // DM_DEBUG

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportDraggingStatusChange
//
//  Synopsis:
//    Called by CInputManagerDMViewportEventHandler to forward DirectManipulation's
//    notification of dragging status change
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportDraggingStatusChange(
    _In_ CDMCrossSlideViewport* pCrossSlideViewport,
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous)
{
    bool fIsDraggable = false;
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessDirectManipulationViewportDraggingStatusChange entry. pViewport=0x%p, current=%d, previous=%d.", this, pCrossSlideViewport, current, previous));
    }
#endif // DM_DEBUG
    IFC_RETURN(pCrossSlideViewport->GetDMContainerNoRef()->CanDrag(&fIsDraggable));

    if (current == DIRECTMANIPULATION_DRAG_DROP_DRAGGING
        && fIsDraggable
        && pCrossSlideViewport->GetDMContainerNoRef()->IsEnabled())
    {
        ASSERT(previous == DIRECTMANIPULATION_DRAG_DROP_READY);
        return FxCallbacks::UIElement_OnDirectManipulationDraggingStarted(
            pCrossSlideViewport->GetDMContainerNoRef());
    }
    else
    {
        return DirectManipulationCrossSlideContainerCompleted(
            pCrossSlideViewport->GetDMContainerNoRef(),
            pCrossSlideViewport);
    }
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportInteractionTypeUpdate
//
//  Synopsis:
//    Called by CInputManagerDMViewportEventHandler to forward DirectManipulation's
//    notification of viewport interaction type change.
//    Called as soon as the status of the provided viewport changed.
//    This method is called outside of the UI thread tick.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportInteractionTypeUpdate(
    _In_ CDMViewport* pViewport,
    _In_ XDMViewportInteractionType newInteractionType)
{
    xref_ptr<CUIDMContainer> spDirectManipulationContainer;
    bool fIsDraggable = false;

    IFCPTR_RETURN(pViewport);
    IFC_RETURN(pViewport->GetDMContainerNoRef()->CanDrag(&fIsDraggable));
    if (fIsDraggable)
    {
        // Drag drop viewport doesn't handle Viewport interaction type update
        return S_OK;
    }

    IFCPTR_RETURN(m_pCoreService);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessDirectManipulationViewportInteractionTypeUpdate entry. pViewport=0x%p, newInteractionType=%d.", this, pViewport, newInteractionType));
    }
#endif // DM_DEBUG

    if (pViewport->GetHasDMHitTestContactId())
    {
        if (newInteractionType == XcpDMViewportInteractionManipulation)
        {
            // This notification marks the recognition of a DManip manipulation for a DM_POINTERHITTEST-initiated contact.
            // The transition to a inactive status can be used to mark the end of the manipulation rather than the XcpDMViewportInteractionEnd
            // interaction type notification.
            pViewport->SetHasDMHitTestContactId(FALSE /*fHasDMHitTestContactId*/);
        }
        else if (newInteractionType == XcpDMViewportInteractionEnd)
        {
            // This notification marks the end of an interaction where at least one contact Id was registered due to a DM_POINTERHITTEST message.
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                XUINT32 cContactIdsDbg = 0;
                IGNOREHR(pViewport->GetContactIdCount(&cContactIdsDbg));
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Completing manipulation because of interaction end - state=%d, cContactIds=%d.",
                    pViewport->GetState(), cContactIdsDbg));
            }
#endif // DM_DEBUG

            pViewport->SetHasDMHitTestContactId(FALSE /*fHasDMHitTestContactId*/);

            IFC_RETURN(UnregisterContactIds(pViewport, FALSE /*fReleaseAllContactsAndDisableViewport*/));
        }
    }

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"                   IsTouchInteractionStartProcessed=%d, IsTouchInteractionEndExpected=%d, IsTouchConfigurationActivated=%d, IsNonTouchConfigurationActivated=%d, IsBringIntoViewportConfigurationActivated=%d.",
            pViewport->GetIsTouchInteractionStartProcessed(), pViewport->GetIsTouchInteractionEndExpected(), pViewport->GetIsTouchConfigurationActivated(),
            pViewport->GetIsNonTouchConfigurationActivated(), pViewport->GetIsBringIntoViewportConfigurationActivated()));
    }
#endif // DM_DEBUG

    // The XcpDMViewportInteractionManipulation case is not being handled here. Instead the first transition to the Running status, if any, causes
    // pViewport->PushInteractionType(XcpDMViewportInteractionManipulation) in CInputServices::ProcessDirectManipulationViewportStatusUpdate.
    // This is to ensure that ScrollViewer.DirectManipulationStart gets raised before ViewChanging/ViewChanged at the beginning of a touch interaction,
    // because the Running status notification is occurring before the XcpDMViewportInteractionManipulation notification.
    switch (newInteractionType)
    {
        case XcpDMViewportInteractionBegin:
        {
            if (pViewport->GetIsTouchConfigurationActivated())
            {
                // Handle the XcpDMViewportInteractionEnd notification for this touch interaction.
                // Ignore the ones for non-touch interactions (mouse-wheel or keyboard).
                pViewport->SetIsTouchInteractionEndExpected(TRUE);

                // No interaction start notification has been processed for this new touch interaction.
                pViewport->SetIsTouchInteractionStartProcessed(FALSE);
            }
            break;
        }
        case XcpDMViewportInteractionEnd:
        {
            // Only handle manipulation completion for touch-based interactions.
            if (pViewport->GetIsTouchInteractionEndExpected())
            {
                pViewport->SetIsTouchInteractionEndExpected(FALSE);
                // Queue up the notification for handling at the next UI thread tick.
                pViewport->PushInteractionType(XcpDMViewportInteractionEnd);
                IFC_RETURN(RequestAdditionalFrame());
            }
            break;
        }
    }

    return S_OK;
}

// Called by ProcessDirectManipulationViewportChanges during a UI thread tick to raise potentially queued up DirectManipulationStarted
// and DirectManipulationCompleted events for the provided viewport.
// When isValuesChangePreProcessing is True, only the leading DirectManipulationStarted event is raised.
// When isValuesChangePostProcessing is True, only the leading DirectManipulationCompleted event is raised.
_Check_return_ HRESULT
CInputServices::RaiseQueuedDirectManipulationStateChanges(
    _In_ CDMViewport* pViewport,
    _In_ bool isValuesChangePreProcessing,
    _In_ bool isValuesChangePostProcessing)
{
    xref_ptr<CUIDMContainer> spDirectManipulationContainer;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled && pViewport->HasQueuedInteractionType())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  RaiseQueuedDirectManipulationStateChanges entry. pViewport=0x%p, state=%d, isValuesChangePreProcessing=%d, isValuesChangePostProcessing=%d.",
            this, pViewport, pViewport->GetState(), isValuesChangePreProcessing, isValuesChangePostProcessing));
    }
#endif // DM_DEBUG

    while (pViewport->HasQueuedInteractionType())
    {
        XDMViewportInteractionType interactionType = pViewport->GetFrontInteractionType();
        ASSERT(interactionType == XcpDMViewportInteractionManipulation || interactionType == XcpDMViewportInteractionEnd);

        if (isValuesChangePreProcessing && interactionType == XcpDMViewportInteractionEnd)
        {
            break;
        }
        if (isValuesChangePostProcessing && interactionType == XcpDMViewportInteractionManipulation)
        {
            break;
        }

        pViewport->PopInteractionType();

        if (spDirectManipulationContainer == nullptr)
        {
            IFC_RETURN(pViewport->GetDMContainerNoRef()->GetDirectManipulationContainer(spDirectManipulationContainer.ReleaseAndGetAddressOf()));
        }

        if (spDirectManipulationContainer != nullptr)
        {
            DirectManipulationState state = (interactionType == XcpDMViewportInteractionManipulation) ? ManipulationStarted : ManipulationCompleted;
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Calling NotifyManipulationStateChanged(%d).", state));
            }
#endif // DM_DEBUG
            IFC_RETURN(spDirectManipulationContainer->NotifyManipulationStateChanged(state));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportStatusUpdate
//
//  Synopsis:
//    Called on a UI tick or XCP_POINTERDOWN message when the status of
//    the provided viewport changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportStatusUpdate(
    _In_ CDMViewport* pViewport,
    _In_ XDMViewportStatus oldStatus,
    _In_ XDMViewportStatus newStatus,
    _In_ bool fIsValuesChangePreProcessing,
    _Out_opt_ bool* pfIgnoreStatusChange,
    _Out_opt_ bool* pfManipulationCompleted)
{
    HRESULT hr = S_OK;
    bool fHasChainingChildViewport = false;
    bool fManipulationStarting = false;
    bool fManipulationStarted = false;
    bool fManipulationCompleted = false;
    bool fEnsureTouchConfigurationActivated = false;
    bool  isInertiaEndTransformValid = false;
    XUINT32 contactId = 0;
    XUINT32 cContactIds = 0;
    XFLOAT initialTranslationX = 0.0f;
    XFLOAT initialTranslationY = 0.0f;
    XFLOAT initialUncompressedZoomFactor = 1.0f;
    XFLOAT initialZoomFactorX = 1.0f;
    XFLOAT initialZoomFactorY = 1.0f;
    XFLOAT currentTranslationX = 0.0f;
    XFLOAT currentTranslationY = 0.0f;
    XFLOAT currentUncompressedZoomFactor = 1.0f;
    XFLOAT currentZoomFactorX = 1.0f;
    XFLOAT currentZoomFactorY = 1.0f;
    XFLOAT xInertiaEndTranslation = 0.0f;
    XFLOAT yInertiaEndTranslation = 0.0f;
    XFLOAT zInertiaEndFactor = 1.0f;
    XFLOAT xCenter = 0.0f;
    XFLOAT yCenter = 0.0f;
    CUIElement* pManipulatedElement = NULL;
    CUIElement* pDMContainer = NULL;
    CDMContent* pContent = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessDirectManipulationViewportStatusUpdate entry. pViewport=0x%p, oldStatus=%d, newStatus=%d, fIsValuesChangePreProcessing=%d.",
            this, pViewport, oldStatus, newStatus, fIsValuesChangePreProcessing));
    }
#endif // DM_DEBUG

    if (pfIgnoreStatusChange)
    {
        *pfIgnoreStatusChange = FALSE;
    }

    if (pfManipulationCompleted)
    {
        *pfManipulationCompleted = FALSE;
    }

    IFCPTR(pViewport);

    pDMContainer = pViewport->GetDMContainerNoRef();
    ASSERT(pDMContainer);

    pManipulatedElement = pViewport->GetManipulatedElementNoRef();
    ASSERT(pManipulatedElement);

    fManipulationStarted = fIsValuesChangePreProcessing && !IsViewportActive(oldStatus) && IsViewportActive(newStatus);
    fManipulationCompleted = (!fIsValuesChangePreProcessing || pfManipulationCompleted) && IsViewportActive(oldStatus) && !IsViewportActive(newStatus);

    if (fManipulationCompleted && newStatus == XcpDMViewportReady)
    {
        // Do not consider the new XcpDMViewportReady status as signaling the end of a manipulation
        // when there is a chaining child viewport in XcpDMViewportRunning status.
        IFC(HasChainingChildViewport(pViewport, FALSE /*fCheckState*/, fHasChainingChildViewport));
        if (fHasChainingChildViewport)
        {
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Not treating XcpDMViewportReady status as ManipulationCompleted."));
            }
#endif // DM_DEBUG

            fManipulationCompleted = FALSE;
        }
    }

    if (pfManipulationCompleted)
    {
        *pfManipulationCompleted = fManipulationCompleted;
    }

    if (fIsValuesChangePreProcessing)
    {
        fManipulationCompleted = FALSE;
    }

    if (fManipulationStarted && pViewport->GetState() != ManipulationStarting)
    {
        // Do not consider the new active status as signaling the beginning of a manipulation
        // when there is a chaining child viewport in Starting/Started/Delta state.
        IFC(HasChainingChildViewport(pViewport, TRUE /*fCheckState*/, fHasChainingChildViewport));
        if (fHasChainingChildViewport)
        {
            if (pViewport->GetState() == ManipulationCompleted)
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Calling NotifyManipulationProgress(ManipulationStarting) for pViewport=0x%p because of active chaining child.", pViewport));
                }
#endif // DM_DEBUG

                fManipulationStarting = TRUE;

                // Refresh the CDMViewport's initial transformation values with the most current ones.
                pViewport->GetCurrentTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                pViewport->SetInitialTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);

                // Do the same initialization for each secondary content.
                for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
                {
                    IFC(pViewport->GetContent(iContent, &pContent));
                    if (pContent)
                    {
                        pContent->GetCurrentTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                        pContent->SetInitialTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                        ReleaseInterface(pContent);
                    }
                }

                for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
                {
                    IFC(pViewport->GetClipContent(iClipContent, &pContent));
                    if (pContent)
                    {
                        pContent->GetCurrentTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                        pContent->SetInitialTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                        ReleaseInterface(pContent);
                    }
                }

                // Reinitialize variables for possible reuse.
                initialTranslationX = initialTranslationY = 0.0f;
                initialUncompressedZoomFactor = initialZoomFactorX = initialZoomFactorY = 1.0f;
            }
            else
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Not treating active status as ManipulationStarted."));
                }

#endif // DM_DEBUG
                fManipulationStarted = FALSE;
            }
        }
        else if (pViewport->GetIsCompletedStateSkipped() || pViewport->GetState() == ManipulationCompleted || pViewport->GetState() == ConstantVelocityScrollStopped)
        {
            // pViewport->GetIsCompletedStateSkipped()==TRUE:
            // Ensure that the touch configuration is activated if the viewport completion is skipped and the new status is Running.
            // pViewport->GetState() == ManipulationCompleted==TRUE:
            // Ensure that the touch configuration is activated if there is a transitional Ready status in between a non-touch Inertia and a Running status.
            // pViewport->GetState() == ConstantVelocityScrollStopped==TRUE:
            // Ensure that fManipulationStarting is set to TRUE when the user starts a scroll with the mouse-wheel just as the auto-scroll completed.
            // The sequence of statuses is XcpDMViewportAutoRunning, XcpDMViewportReady and XcpDMViewportInertia.
            if (pViewport->GetStatusesCount() < 3 || oldStatus != XcpDMViewportReady || newStatus != XcpDMViewportRunning)
            {
                fEnsureTouchConfigurationActivated = TRUE;
            }
            else
            {
                XDMViewportStatus currentStatus;

                IFC(pViewport->GetCurrentStatus(currentStatus));
                if (currentStatus != XcpDMViewportReady)
                {
                    fEnsureTouchConfigurationActivated = TRUE;
                }
                // else there are at least 3 queued up statuses, with a sequence Ready->Running->Ready:
                // pViewport->GetStatusesCount() >= 3 && oldStatus == XcpDMViewportReady && newStatus == XcpDMViewportRunning && currentStatus == XcpDMViewportReady
                // Do not activate the touch configuration and queue up a DirectManipulationStarted notification for a transitional Running status.
            }

            if (pViewport->GetIsCompletedStateSkipped())
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Not treating active status as ManipulationStarted for pViewport=0x%p because of skipped ManipulationCompleted, state=%d.",
                        pViewport, pViewport->GetState()));
                }
#endif // DM_DEBUG

                pViewport->SetIsCompletedStateSkipped(FALSE);

                fManipulationStarted = FALSE;
            }
            else
            {
                // Bug 689141 for instance causes a transitional Ready status.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Calling NotifyManipulationProgress(ManipulationStarting) for pViewport=0x%p as workaround for transitional Ready status.", pViewport));
                }
#endif // DM_DEBUG

                fManipulationStarting = TRUE;
            }
        }
    }

    if (fEnsureTouchConfigurationActivated && newStatus == XcpDMViewportRunning && !pViewport->GetIsTouchConfigurationActivated())
    {
        if (pViewport->GetTouchConfiguration() == XcpDMConfigurationNone)
        {
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Skipping touch configuration activation for pViewport=0x%p because it is XcpDMConfigurationNone.", pViewport));
            }
#endif // DM_DEBUG

        }
        else
        {
            // User switched from a non-touch or bring-into-viewport manipulation to a touch-based manipulation.
            // Activate the touch configuration.
            ASSERT(!pDirectManipulationService);
            IFC(GetDMService(pDMContainer, &pDirectManipulationService));
            ASSERT(pDirectManipulationService);

            IFC(SwitchToTouchConfiguration(pDirectManipulationService, pViewport));
            ReleaseInterface(pDirectManipulationService);
        }
    }

    if (fManipulationStarting)
    {
        ASSERT(!pDirectManipulationContainer);
        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        ASSERT(pDirectManipulationContainer);

        // Let the compositor know about the active viewport
        IFC(DeclareNewViewportForCompositor(pViewport, pDirectManipulationContainer));

        pViewport->SetState(ManipulationStarting);
        IFC(pDirectManipulationContainer->NotifyManipulationProgress(
            pManipulatedElement,
            ManipulationStarting,
            0.0f /*xCumulativeTranslation*/,
            0.0f /*yCumulativeTranslation*/,
            1.0f /*zCumulativeFactor*/,
            0.0f /*xInertiaEndTranslation*/,
            0.0f /*yInertiaEndTranslation*/,
            1.0f /*zInertiaEndFactor*/,
            0.0f /*xCenter*/,
            0.0f /*yCenter*/,
            FALSE /*fIsInertiaEndTransformValid*/,
            newStatus == XcpDMViewportInertia /*fIsInertial*/,
            pViewport->GetIsTouchConfigurationActivated() /*fIsTouchConfigurationActivated*/,
            pViewport->GetIsBringIntoViewportConfigurationActivated() /*fIsBringIntoViewportConfigurationActivated*/));
    }

    if (fManipulationStarted && pViewport->GetState() != ManipulationStarting)
    {
        // Processing a transitional Running status. Just ignore it.
        // The viewport may transition to Running and then immediately to Ready.
        if (pfIgnoreStatusChange)
        {
            *pfIgnoreStatusChange = TRUE;
        }
        goto Cleanup;
    }

    ASSERT(!fManipulationStarted || pViewport->GetState() == ManipulationStarting);

    if (pViewport->GetNeedsUnregistration() || fManipulationStarted || fManipulationCompleted)
    {
        pViewport->GetInitialTransformationValues(
            initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);

        pViewport->GetCurrentTransformationValues(
            currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY);
    }

    if (fManipulationStarted)
    {
        if (!pDirectManipulationContainer)
        {
            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        }
        ASSERT(pDirectManipulationContainer);

#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"                   Calling NotifyManipulationProgress(ManipulationStarted) for pViewport=0x%p because of Running status, state=%d.",
                pViewport, pViewport->GetState()));
        }
#endif // DM_DEBUG

        ASSERT(pViewport->GetState() == ManipulationStarting);

        pViewport->SetState(ManipulationStarted);

        ASSERT(!pDirectManipulationService);
        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        ASSERT(pDirectManipulationService);

        IFC(pDirectManipulationService->GetViewportCenterPoint(
            pViewport, xCenter, yCenter));

        if (newStatus == XcpDMViewportInertia)
        {
            IFC(GetDirectManipulationContentInertiaEndTransform(
                pDirectManipulationService,
                pViewport,
                &xInertiaEndTranslation,
                &yInertiaEndTranslation,
                &zInertiaEndFactor,
                &isInertiaEndTransformValid));
        }

        IFC(pDirectManipulationContainer->NotifyManipulationProgress(
            pManipulatedElement,
            ManipulationStarted,
            0.0f /*xCumulativeTranslation*/,
            0.0f /*yCumulativeTranslation*/,
            1.0f /*zCumulativeFactor*/,
            xInertiaEndTranslation,
            yInertiaEndTranslation,
            zInertiaEndFactor,
            xCenter,
            yCenter,
            isInertiaEndTransformValid,
            newStatus == XcpDMViewportInertia /*fIsInertial*/,
            pViewport->GetIsTouchConfigurationActivated() /*fIsTouchConfigurationActivated*/,
            pViewport->GetIsBringIntoViewportConfigurationActivated() /*fIsBringIntoViewportConfigurationActivated*/));
    }
    else if (pViewport->GetState() != ManipulationCompleted &&
        (pViewport->GetNeedsUnregistration() || (fManipulationCompleted && !pViewport->GetIsCompletedStateSkipped())))
    {
        if (!pDirectManipulationContainer)
        {
            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        }
        ASSERT(pDirectManipulationContainer);

        pManipulatedElement = pViewport->GetManipulatedElementNoRef();
        ASSERT(pManipulatedElement);

#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"                   Calling NotifyManipulationProgress(ManipulationCompleted) for pViewport=0x%p because of %s, state=%d.",
                pViewport, pViewport->GetNeedsUnregistration() ? L"unregistration" : L"status change", pViewport->GetState()));
        }
#endif // DM_DEBUG

        ASSERT(pViewport->GetState() != ManipulationCompleted);

        pViewport->SetState(ManipulationCompleted);
        pViewport->SetIsPrimaryContentLayoutRefreshedAfterCompletion(FALSE);

        ASSERT(!pDirectManipulationService);
        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        ASSERT(pDirectManipulationService);

        IFC(pDirectManipulationService->GetViewportCenterPoint(
            pViewport, xCenter, yCenter));

        IFC(pDirectManipulationContainer->NotifyManipulationProgress(
            pManipulatedElement,
            ManipulationCompleted,
            currentTranslationX - initialTranslationX /*xCumulativeTranslation*/,
            currentTranslationY - initialTranslationY /*yCumulativeTranslation*/,
            currentUncompressedZoomFactor / initialUncompressedZoomFactor /*zCumulativeFactor*/,
            0.0f /*xInertiaEndTranslation*/,
            0.0f /*yInertiaEndTranslation*/,
            1.0f /*zInertiaEndFactor*/,
            xCenter,
            yCenter,
            FALSE /*fIsInertiaEndTransformValid*/,
            oldStatus == XcpDMViewportInertia /*fIsInertial*/,
            pViewport->GetIsTouchConfigurationActivated() /*fIsTouchConfigurationActivated*/,
            pViewport->GetIsBringIntoViewportConfigurationActivated() /*fIsBringIntoViewportConfigurationActivated*/));
    }

    if (!pViewport->GetNeedsUnregistration())
    {
        if (fIsValuesChangePreProcessing)
        {
            if (oldStatus == XcpDMViewportInertia && newStatus == XcpDMViewportRunning)
            {
                if (pViewport->GetHasReceivedContactIdInInertia())
                {
                    // User interrupted a low velocity inertia. Clear the HasReceivedContactIdInInertia flag which is no longer useful at this point.
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   Resetting HasReceivedContactIdInInertia flag."));
                    }
#endif // DM_DEBUG
                    pViewport->SetHasReceivedContactIdInInertia(FALSE);
                }
                if (!pViewport->GetIsTouchConfigurationActivated())
                {
                    // User switched from a non-touch or bring-into-viewport manipulation to a touch-based manipulation.
                    // Activate the touch configuration.
                    if (!pDirectManipulationService)
                    {
                        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
                    }
                    ASSERT(pDirectManipulationService);

                    IFC(SwitchToTouchConfiguration(pDirectManipulationService, pViewport));
                }
            }
        }
        else if (!fHasChainingChildViewport &&
            (IsViewportActive(oldStatus) && (newStatus == XcpDMViewportDisabled || newStatus == XcpDMViewportReady || newStatus == XcpDMViewportInertia)))
        {
            IFC(pViewport->GetContactIdCount(&cContactIds));
            if (cContactIds > 0)
            {
                do
                {
                    IFC(pViewport->GetContactId(0, &contactId));
                    IFC(UnregisterContactId(contactId, pViewport /*pExclusiveViewport*/, FALSE /*fCompleteManipulation*/));
                    IFC(pViewport->GetContactIdCount(&cContactIds));
                }
                while (cContactIds > 0);
            }
            else if (!pViewport->GetIsCompletedStateSkipped() &&
                pViewport->GetState() != ManipulationStarting &&
                (newStatus == XcpDMViewportReady || newStatus == XcpDMViewportDisabled))
            {
                // Inertia phase is over or DManip was cancelled. Let the compositor know that the manipulation is complete.
                pViewport->DeclareOldViewportForCompositor();
            }

            // Complete the manipulations of the parent viewports with Ready status
            IFC(CompleteParentViewports(pViewport));

            if (!pViewport->GetIsCompletedStateSkipped() &&
                (newStatus == XcpDMViewportReady || newStatus == XcpDMViewportDisabled))
            {
                // Check if the DM manager needs to be deactivated now that one of its viewports is no longer active.
                IFC(UpdateDirectManipulationManagerActivation(pDMContainer, FALSE /*fRefreshViewportStatus*/));
            }
        }
    }

    if (!fIsValuesChangePreProcessing
         && oldStatus == XcpDMViewportInertia
         && pViewport->GetRequestReplayPointerUpdateWhenInertiaCompletes())
    {
        pViewport->SetRequestReplayPointerUpdateWhenInertiaCompletes(FALSE);
        if (!IsViewportActive(newStatus))
        {
            m_pCoreService->RequestReplayPreviousPointerUpdate();
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessConstantVelocityViewportStatusUpdate
//
//  Synopsis:
//    Called on a UI tick to process a status change related to constant
//    velocity pan.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessConstantVelocityViewportStatusUpdate(
    _In_ CDMViewport* pViewport,
    _In_ XDMViewportStatus currentStatus)
{
    HRESULT hr = S_OK;
    XFLOAT initialTranslationX = 0.0f;
    XFLOAT initialTranslationY = 0.0f;
    XFLOAT initialUncompressedZoomFactor = 1.0f;
    XFLOAT initialZoomFactorX = 1.0f;
    XFLOAT initialZoomFactorY = 1.0f;
    XFLOAT currentTranslationX = 0.0f;
    XFLOAT currentTranslationY = 0.0f;
    XFLOAT currentUncompressedZoomFactor = 1.0f;
    XFLOAT currentZoomFactorX = 1.0f;
    XFLOAT currentZoomFactorY = 1.0f;
    XFLOAT xCenter = 0.0f;
    XFLOAT yCenter = 0.0f;
    bool fIsInAutoRunning = currentStatus == XcpDMViewportAutoRunning;
    bool fIsInRunning = currentStatus == XcpDMViewportRunning;
    bool wasPreConstantVelocityPanStateStarting = false;
    DirectManipulationState oldState = ManipulationNone;
    DirectManipulationState newState = (fIsInAutoRunning) ? ConstantVelocityScrollStarted : ConstantVelocityScrollStopped;
    CUIElement* pManipulatedElement = NULL;
    CUIElement* pDMContainer = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessConstantVelocityViewportStatusUpdate entry. pViewport=0x%p, currentStatus=%d.", this, pViewport, currentStatus));
    }
#endif // DM_DEBUG

    ASSERT(pViewport);

    wasPreConstantVelocityPanStateStarting = pViewport->GetIsPreConstantVelocityPanStateStarting();
    oldState = pViewport->GetState();

    pDMContainer = pViewport->GetDMContainerNoRef();
    ASSERT(pDMContainer);

    ASSERT(!fIsInAutoRunning || (oldState != ManipulationStarted && oldState != ManipulationDelta && oldState != ManipulationLastDelta));

    // The ManipulationStarting state only needs to be restored after an auto-scroll if touch caused that current ManipulationStarting state.
    // A ManipulationStarting caused by a mouse wheel for instance must not be restored.
    pViewport->SetIsPreConstantVelocityPanStateStarting(fIsInAutoRunning ? (oldState == ManipulationStarting && pViewport->GetIsTouchConfigurationActivated()) : FALSE);

    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
    ASSERT(pDirectManipulationContainer);

    if (!fIsInRunning && !fIsInAutoRunning && !wasPreConstantVelocityPanStateStarting)
    {
        pViewport->DeclareOldViewportForCompositor();
    }
    else if (fIsInAutoRunning && !pViewport->GetHasNewManipulation())
    {
        // Ensure the compositor is aware of the viewport when an auto-scroll begins, whether it's touch- or mouse-driven.
        IFC(DeclareNewViewportForCompositor(pViewport, pDirectManipulationContainer));
    }

    pManipulatedElement = pViewport->GetManipulatedElementNoRef();
    ASSERT(pManipulatedElement);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  calling NotifyManipulationProgress(%s) for pViewport=0x%p, state=%d.",
            this, fIsInAutoRunning ? L"ConstantVelocityScrollStarted" : L"ConstantVelocityScrollStopped", pViewport, pViewport->GetState()));
    }
#endif // DM_DEBUG

    if (wasPreConstantVelocityPanStateStarting || fIsInRunning)
    {
        // If the viewport was in ManipulationStarting state, we need to notify DM of the new offsets.
        // Get those offsets here.
        pViewport->GetInitialTransformationValues(
            initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
        pViewport->GetCurrentTransformationValues(
            currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY);

        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        ASSERT(pDirectManipulationService);

        IFC(pDirectManipulationService->GetViewportCenterPoint(
            pViewport, xCenter, yCenter));
    }

    pViewport->SetState(newState);
    IFC(pDirectManipulationContainer->NotifyManipulationProgress(pManipulatedElement,
        newState,
        currentTranslationX - initialTranslationX /*xCumulativeTranslation*/,
        currentTranslationY - initialTranslationY /*yCumulativeTranslation*/,
        currentUncompressedZoomFactor / initialUncompressedZoomFactor /*zCumulativeFactor*/,
        0.0f /*xInertiaEndTranslation*/,
        0.0f /*yInertiaEndTranslation*/,
        1.0f /*zInertiaEndFactor*/,
        xCenter,
        yCenter,
        FALSE /*fIsInertiaEndTransformValid*/,
        FALSE /*fIsInertial*/,
        FALSE /*fIsTouchConfigurationActivated*/,
        FALSE /*fIsBringIntoViewportConfigurationActivated*/));

    if (newState == ConstantVelocityScrollStopped)
    {
        // An edge scroll completed.
        // Make sure the auto-scroll configuration and status are reset in the DManip Service.
        IFC(SetConstantVelocities(pDMContainer, pManipulatedElement, 0.0f /*panXVelocity*/, 0.0f /*panYVelocity*/));

        if (wasPreConstantVelocityPanStateStarting || fIsInRunning)
        {
            // If the viewport was in ManipulationStarting state, we need to make sure to re-activate it after we process our
            // ConstantVelocityScrollStopped status...unless the viewport's ManipulationCompleted state was just delayed.
            if (pViewport->GetIsCompletedStateDelayedByConstantVelocityPan())
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Completing delayed manipulation after constant velocity pan for pViewport=0x%p.", pViewport));
                }
#endif // DM_DEBUG

                pViewport->SetIsCompletedStateDelayedByConstantVelocityPan(FALSE);
                IFC(CompleteDirectManipulation(pViewport, FALSE /*fDisableViewport*/));
            }
            else
            {
                if (!pViewport->GetIsTouchConfigurationActivated())
                {
                    // User switches to a touch-based manipulation. Activate the touch configuration.
                    ASSERT(pDirectManipulationService);
                    IFC(UpdateManipulationTouchConfiguration(
                        pDirectManipulationService,
                        pViewport,
                        pViewport->GetTouchConfiguration()));
                }

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Calling NotifyManipulationProgress(ManipulationStarting) for pViewport=0x%p after constant velocity pan suspension.", pViewport));
                }

#endif // DM_DEBUG
                pViewport->SetState(ManipulationStarting);
                IFC(pDirectManipulationContainer->NotifyManipulationProgress(pManipulatedElement,
                    ManipulationStarting,
                    currentTranslationX - initialTranslationX /*xCumulativeTranslation*/,
                    currentTranslationY - initialTranslationY /*yCumulativeTranslation*/,
                    currentUncompressedZoomFactor / initialUncompressedZoomFactor /*zCumulativeFactor*/,
                    0.0f /*xInertiaEndTranslation*/,
                    0.0f /*yInertiaEndTranslation*/,
                    1.0f /*zInertiaEndFactor*/,
                    xCenter,
                    yCenter,
                    FALSE /*fIsInertiaEndTransformValid*/,
                    FALSE /*fIsInertial*/,
                    TRUE  /*fIsTouchConfigurationActivated*/,
                    FALSE /*fIsBringIntoViewportConfigurationActivated*/));

                if (fIsInRunning)
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Calling NotifyManipulationProgress(ManipulationStarted) for pViewport=0x%p after constant velocity pan suspension.", pViewport));
                    }

#endif // DM_DEBUG
                    pViewport->SetState(ManipulationStarted);
                    IFC(pDirectManipulationContainer->NotifyManipulationProgress(pManipulatedElement,
                        ManipulationStarted,
                        currentTranslationX - initialTranslationX /*xCumulativeTranslation*/,
                        currentTranslationY - initialTranslationY /*yCumulativeTranslation*/,
                        currentUncompressedZoomFactor / initialUncompressedZoomFactor /*zCumulativeFactor*/,
                        0.0f /*xInertiaEndTranslation*/,
                        0.0f /*yInertiaEndTranslation*/,
                        1.0f /*zInertiaEndFactor*/,
                        xCenter,
                        yCenter,
                        FALSE /*fIsInertiaEndTransformValid*/,
                        FALSE /*fIsInertial*/,
                        TRUE  /*fIsTouchConfigurationActivated*/,
                        FALSE /*fIsBringIntoViewportConfigurationActivated*/));
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationViewportValuesUpdate
//
//  Synopsis:
//    Called when a new transform for a viewport's primary content is
//    available. Caches the new transform.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationViewportValuesUpdate(
    _In_ CDMViewport* pViewport,
    _In_ bool fWasInertial,
    _In_ bool fIsInertial,
    _In_ bool fIsLastDelta)
{
    HRESULT hr = S_OK;
    XFLOAT initialTranslationX = 0.0f;
    XFLOAT initialTranslationY = 0.0f;
    XFLOAT initialUncompressedZoomFactor = 1.0f;
    XFLOAT initialZoomFactorX = 1.0f;
    XFLOAT initialZoomFactorY = 1.0f;
    XFLOAT oldTranslationX = 0.0f;
    XFLOAT oldTranslationY = 0.0f;
    XFLOAT oldUncompressedZoomFactor = 1.0f;
    XFLOAT oldZoomFactorX = 1.0f;
    XFLOAT oldZoomFactorY = 1.0f;
    XFLOAT newTranslationX = 0.0f;
    XFLOAT newTranslationY = 0.0f;
    XFLOAT newUncompressedZoomFactor = 1.0f;
    XFLOAT newZoomFactorX = 1.0f;
    XFLOAT newZoomFactorY = 1.0f;
    XFLOAT inertiaEndTranslationX = 0.0f;
    XFLOAT inertiaEndTranslationY = 0.0f;
    XFLOAT inertiaEndZoomFactor = 1.0f;
    XFLOAT xCenter = 0.0f;
    XFLOAT yCenter = 0.0f;
    bool fNotifyManipulationDelta = false;
    bool isInertiaEndTransformValid = false;
    CUIElement* pManipulatedElement = NULL;
    CUIElement* pDMContainer = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    TraceDmViewportValuesUpdateBegin((UINT64)pViewport);

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: ProcessDirectManipulationViewportValuesUpdate entry - pViewport=0x%p, fIsInertial=%d, fIsLastDelta=%d.",
            this, pViewport, fIsInertial, fIsLastDelta));
    }

#endif // DM_DEBUG

    IFCPTR(pViewport);

    pDMContainer = pViewport->GetDMContainerNoRef();
    ASSERT(pDMContainer);

    if (!pDMContainer->IsActive())
    {
        goto Cleanup;
    }

    pManipulatedElement = pViewport->GetManipulatedElementNoRef();
    ASSERT(pManipulatedElement);

    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
    ASSERT(pDirectManipulationContainer);

    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    ASSERT(pDirectManipulationService);

    IFC(pDirectManipulationService->GetPrimaryContentTransform(
        pViewport,
        newTranslationX,
        newTranslationY,
        newUncompressedZoomFactor,
        newZoomFactorX,
        newZoomFactorY));

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  ProcessDirectManipulationViewportValuesUpdate content output transform - pViewport=0x%p, pManipulatedElement=0x%p, fIsInertial=%d.",
            this, pViewport, pManipulatedElement, fIsInertial));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"                   newTranslationX=%4.6lf, newTranslationY=%4.6lf, newUncompressedZoomFactor=%4.8lf, newZoomFactorX=%4.8lf, newZoomFactorY=%4.8lf.",
            newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY));
    }

#endif // DM_DEBUG

    pViewport->GetInitialTransformationValues(
        initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);

    pViewport->GetCurrentTransformationValues(
        oldTranslationX, oldTranslationY, oldUncompressedZoomFactor, oldZoomFactorX, oldZoomFactorY);

    if (newTranslationX != oldTranslationX)
    {
        fNotifyManipulationDelta = TRUE;
    }

    if (newTranslationY != oldTranslationY)
    {
        fNotifyManipulationDelta = TRUE;
    }

    if (newZoomFactorX != oldZoomFactorX)
    {
        fNotifyManipulationDelta = TRUE;
    }

    if (newZoomFactorY != oldZoomFactorY)
    {
        fNotifyManipulationDelta = TRUE;
    }

    pViewport->SetCurrentTransformationValues(
        newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY);

    // Call NotifyManipulationProgress even when the transform has not changed but fWasInertial != fIsInertial.
    // This is in order to clear the ScrollViewer::m_isInertiaEndTransformValid flag so that an old and invalid
    // inertia-end-transform is not used in the ScrollViewer::ViewChanging event args.
    if (fNotifyManipulationDelta || fWasInertial != fIsInertial || fIsLastDelta)
    {
        if (fNotifyManipulationDelta || fIsLastDelta)
        {
            // Note that this invalidation is marked as independent. This prevents an unnecessary render walk
            // from occurring if the only change this frame was a DM value update, and no dependent changes
            // occurred in response. If a dependent change occurs (e.g. new virtualized items added, scroll indicator moved)
            // then those will also mark an element dirty and that will cause a render walk to occur.
            CUIElement::NWSetTransformDirty(
                pManipulatedElement,
                DirtyFlags::Render | DirtyFlags::Bounds | DirtyFlags::Independent
                );

            IFC(ProcessDirectManipulationSecondaryContentsUpdate(
                pViewport,
                pDirectManipulationService));
        }

#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"                   Calling NotifyManipulationProgress(%s) for pViewport=0x%p, state=%d.",
                fIsLastDelta ? L"ManipulationLastDelta" : L"ManipulationDelta", pViewport, pViewport->GetState()));
        }

#endif // DM_DEBUG

        ASSERT(pViewport->GetState() == ManipulationStarted ||
            pViewport->GetState() == ManipulationDelta ||
            pViewport->GetState() == ManipulationLastDelta ||
            pViewport->GetState() == ConstantVelocityScrollStarted);

        pViewport->SetState(fIsLastDelta ? ManipulationLastDelta : ManipulationDelta);

        IFC(pDirectManipulationService->GetViewportCenterPoint(
            pViewport, xCenter, yCenter));

        if (fIsInertial)
        {
            IFC(GetDirectManipulationContentInertiaEndTransform(
                pDirectManipulationService,
                pViewport,
                &inertiaEndTranslationX,
                &inertiaEndTranslationY,
                &inertiaEndZoomFactor,
                &isInertiaEndTransformValid));
        }

        IFC(pDirectManipulationContainer->NotifyManipulationProgress(
            pManipulatedElement,
            fIsLastDelta ? ManipulationLastDelta : ManipulationDelta,
            newTranslationX - initialTranslationX,
            newTranslationY - initialTranslationY,
            newUncompressedZoomFactor / initialUncompressedZoomFactor,
            inertiaEndTranslationX,
            inertiaEndTranslationY,
            inertiaEndZoomFactor,
            xCenter,
            yCenter,
            isInertiaEndTransformValid,
            fIsInertial,
            pViewport->GetIsTouchConfigurationActivated() /*fIsTouchConfigurationActivated*/,
            pViewport->GetIsBringIntoViewportConfigurationActivated() /*fIsBringIntoViewportConfigurationActivated*/));
    }

Cleanup:
    TraceDmViewportValuesUpdateEnd(
        (UINT64)pViewport,
        newTranslationX,
        newTranslationY,
        newUncompressedZoomFactor,
        newZoomFactorX,
        newZoomFactorY);

    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ProcessDirectManipulationSecondaryContentsUpdate
//
//  Synopsis:
//    Updates the secondary contents' CDMContent transform values.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessDirectManipulationSecondaryContentsUpdate(
    _In_ CDMViewport* pViewport,
    _In_ IPALDirectManipulationService* pDirectManipulationService)
{
    HRESULT hr = S_OK;
    XFLOAT oldTranslationX = 0.0f;
    XFLOAT oldTranslationY = 0.0f;
    XFLOAT oldUncompressedZoomFactor = 1.0f;
    XFLOAT oldZoomFactorX = 1.0f;
    XFLOAT oldZoomFactorY = 1.0f;
    XFLOAT newTranslationX = 0.0f;
    XFLOAT newTranslationY = 0.0f;
    XFLOAT newUncompressedZoomFactor = 1.0f;
    XFLOAT newZoomFactorX = 1.0f;
    XFLOAT newZoomFactorY = 1.0f;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: ProcessDirectManipulationSecondaryContentsUpdate entry - pViewport=0x%p.", this, pViewport));
    }

#endif // DM_DEBUG

    IFCPTR(pViewport);
    IFCPTR(pDirectManipulationService);

    IFC(pViewport->GetCurrentStatus(status));

    for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
    {
        IFC(pViewport->GetContent(iContent, &pContent));
        if (pContent)
        {
            if (status == XcpDMViewportAutoRunning)
            {
                // When operating in constant velocity mode, filter out the primary content's transform
                // since DManip is unaware of the manipulation.
                pViewport->GetCurrentTransformationValues(
                    newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY);
                switch (pContent->GetContentType())
                {
                case XcpDMContentTypeTopLeftHeader:
                    newTranslationX = newTranslationY = 0.0f;
                    break;
                case XcpDMContentTypeTopHeader:
                    newTranslationY = 0.0f;
                    break;
                case XcpDMContentTypeLeftHeader:
                    newTranslationX = 0.0f;
                    break;
                case XcpDMContentTypeCustom:
                case XcpDMContentTypeDescendant:
                    newTranslationX = 0.0f;
                    newTranslationY = 0.0f;
                    break;
                }
            }
            else
            {
                TraceDmContentValuesUpdateBegin((UINT64)pViewport, (UINT64)pContent);

                IFC(pDirectManipulationService->GetSecondaryContentTransform(
                    pViewport,
                    pContent,
                    pContent->GetContentType(),
                    newTranslationX,
                    newTranslationY,
                    newUncompressedZoomFactor,
                    newZoomFactorX,
                    newZoomFactorY));

                TraceDmContentValuesUpdateEnd(
                    (UINT64)pViewport,
                    (UINT64)pContent,
                    pContent->GetContentType(),
                    newTranslationX,
                    newTranslationY,
                    newUncompressedZoomFactor,
                    newZoomFactorX,
                    newZoomFactorY);
            }

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"DMIM[0x%p]:  ProcessDirectManipulationSecondaryContentsUpdate content output transform - pViewport=0x%p, pContent=0x%p, newTranslationX=%4.6lf, newTranslationY=%4.6lf, newUncompressedZoomFactor=%4.8lf, newZoomFactorX=%4.8lf, newZoomFactorY=%4.8lf.",
                    this, pViewport, pContent, newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY));
            }

#endif // DM_DEBUG

            pContent->GetCurrentTransformationValues(
                oldTranslationX, oldTranslationY, oldUncompressedZoomFactor, oldZoomFactorX, oldZoomFactorY);

            if (newTranslationX != oldTranslationX ||
                newTranslationY != oldTranslationY ||
                newUncompressedZoomFactor != oldUncompressedZoomFactor ||
                newZoomFactorX != oldZoomFactorX ||
                newZoomFactorY != oldZoomFactorY)
            {
                pContent->SetCurrentTransformationValues(
                    newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY);

                // Using the DirtyFlags::Independent flag to prevent an unnecessary render walk.
                CUIElement::NWSetTransformDirty(pContent->GetContentElementNoRef(), DirtyFlags::Render | DirtyFlags::Bounds | DirtyFlags::Independent);
            }

            ReleaseInterface(pContent);
        }
    }

    for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
    {
        IFC(pViewport->GetClipContent(iClipContent, &pContent));
        if (pContent)
        {
            if (status == XcpDMViewportAutoRunning)
            {
                // When operating in constant velocity mode, filter out the primary content's transform
                // since DManip is unaware of the manipulation.
                pViewport->GetCurrentTransformationValues(
                    newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY);
                switch (pContent->GetContentType())
                {
                case XcpDMContentTypeTopLeftHeader:
                    newTranslationX = newTranslationY = 0.0f;
                    break;
                case XcpDMContentTypeTopHeader:
                    newTranslationY = 0.0f;
                    break;
                case XcpDMContentTypeLeftHeader:
                    newTranslationX = 0.0f;
                    break;
                case XcpDMContentTypeCustom:
                case XcpDMContentTypeDescendant:
                    newTranslationX = 0.0f;
                    newTranslationY = 0.0f;
                    break;
                }
            }
            else
            {
                IFC(pDirectManipulationService->GetSecondaryClipContentTransform(
                    pViewport,
                    pContent,
                    pContent->GetContentType(),
                    newTranslationX,
                    newTranslationY,
                    newUncompressedZoomFactor,
                    newZoomFactorX,
                    newZoomFactorY));
            }

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"DMIM[0x%p]:  ProcessDirectManipulationSecondaryContentsUpdate content output transform - pViewport=0x%p, pContent=0x%p, newTranslationX=%4.6lf, newTranslationY=%4.6lf, newUncompressedZoomFactor=%4.8lf, newZoomFactorX=%4.8lf, newZoomFactorY=%4.8lf.",
                    this, pViewport, pContent, newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY));
            }

#endif // DM_DEBUG

            pContent->GetCurrentTransformationValues(
                oldTranslationX, oldTranslationY, oldUncompressedZoomFactor, oldZoomFactorX, oldZoomFactorY);

            if (newTranslationX != oldTranslationX ||
                newTranslationY != oldTranslationY ||
                newUncompressedZoomFactor != oldUncompressedZoomFactor ||
                newZoomFactorX != oldZoomFactorX ||
                newZoomFactorY != oldZoomFactorY)
            {
                pContent->SetCurrentTransformationValues(
                    newTranslationX, newTranslationY, newUncompressedZoomFactor, newZoomFactorX, newZoomFactorY);

                // Using the DirtyFlags::Independent flag to prevent an unnecessary render walk.
                CUIElement::NWSetTransformDirty(pContent->GetContentElementNoRef(), DirtyFlags::Render | DirtyFlags::Bounds | DirtyFlags::Independent);
            }

            ReleaseInterface(pContent);
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   DeclareNewViewportForCompositor
//
//  Synopsis:
//      Called when a viewport may have to be submitted to the compositor.
//      In the affirmative, the translation adjustments are cached for
//      future consumption in GetDirectManipulationTransform.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DeclareNewViewportForCompositor(
    _In_ CDMViewport* pViewport,
    _In_ CUIDMContainer* pDirectManipulationContainer)
{
    XFLOAT translationAdjustmentX = 0.0f;
    XFLOAT translationAdjustmentY = 0.0f;
    XFLOAT marginX = 0.0f;
    XFLOAT marginY = 0.0f;

    IFCPTR_RETURN(pViewport);
    IFCPTR_RETURN(pDirectManipulationContainer);

    if (pViewport->DeclareNewViewportForCompositor())
    {
        IFC_RETURN(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
            pViewport->GetManipulatedElementNoRef(),
            FALSE /*fInManipulation*/,
            TRUE  /*fForInitialTransformationAdjustment*/,
            FALSE /*fForMargins*/,
            &translationAdjustmentX,
            &translationAdjustmentY,
            NULL  /*pZoomFactor*/));

        pViewport->SetTranslationAdjustments(translationAdjustmentX, translationAdjustmentY);

        IFC_RETURN(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
            pViewport->GetManipulatedElementNoRef(),
            FALSE /*fInManipulation*/,
            FALSE /*fForInitialTransformationAdjustment*/,
            TRUE  /*fForMargins*/,
            &marginX,
            &marginY,
            NULL  /*pZoomFactor*/));

        pViewport->SetMargins(marginX, marginY);

        IFC_RETURN(RequestAdditionalFrame());
    }

    return S_OK;
}

// Returns True when:
// - the provided UIElement is the manipulated element of a DManip viewport
// - that viewport belongs to a regular ScrollViewer or a touch-manipulatable root ScrollViewer
bool
CInputServices::RequiresViewportInteraction(_In_ CUIElement* manipulatedElement)
{
    xref_ptr<CDMViewport> viewport;

    ASSERT(manipulatedElement);

    IFCFAILFAST(GetViewport(nullptr /*pDMContainer*/, manipulatedElement, viewport.ReleaseAndGetAddressOf()));
    if (viewport)
    {
        CUIElement* dmContainerNoRef = viewport->GetDMContainerNoRef();

        if (dmContainerNoRef)
        {
            if (dmContainerNoRef->GetTypeIndex() == KnownTypeIndex::RootScrollViewer)
            {
                // The root ScrollViewer only requires a viewport interaction object when it is manipulatable with touch.
                return viewport->GetTouchConfiguration() != XcpDMConfigurationNone;
            }
            else
            {
                // Regular ScrollViewer controls always require a viewport interaction object.
                return true;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessUIThreadTick
//
//  Synopsis:
//      Called on each UI thread tick to process possible DirectManipulation updates.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ProcessUIThreadTick()
{
    bool isDirectManipulationSupported = false;

    IFCPTR_RETURN(m_pCoreService);

#ifdef DM_DEBUG
    EvaluateInfoTracingStatuses();
#endif // DM_DEBUG

    IFC_RETURN(gps->IsDirectManipulationSupported(isDirectManipulationSupported));
    if (isDirectManipulationSupported)
    {
        IFC_RETURN(InitializeDirectManipulationContainers());
        IFC_RETURN(ProcessDirectManipulationViewportChanges());
        IFC_RETURN(RefreshDirectManipulationHandlerWantsNotifications());
    }

    // Process the manipulation inertia with interaction manager if manipulation
    // is processing with inertia.
    RaiseManipulationInertiaProcessingEvent();

    IFC_RETURN(ProcessDeferredReleaseQueue(nullptr /*pUnregisteringViewport*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   OnPostUIThreadTick
//
//  Synopsis:
//      Called after the UI thread tick has processed a frame.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::OnPostUIThreadTick()
{
    IFC_RETURN(StopInertialViewportsWithoutCompositorPeer());

    // Clear the saved input qpc.
    m_qpcFirstPointerUpSinceLastFrame = 0;

    for (const xref_ptr<CContentRoot>& contentRoot: m_pCoreService->GetContentRootCoordinator()->GetContentRoots())
    {
        contentRoot->GetInputManager().SetNoCandidateDirectionPerTick(FocusNavigationDirection::None);
        contentRoot->GetFocusManagerNoRef()->ClearXYFocusCache();
    }

    return S_OK;
}

#ifdef DM_DEBUG
//------------------------------------------------------------------------
//
//  Method:   EvaluateInfoTracingStatuses
//
//  Synopsis:
//      Refreshes the values of m_fIsDMInfoTracingEnabled and m_fIsDMVerboseInfoTracingEnabled
//      which allow quick checks on whether ETW or output debug strings are turned on.
//
//------------------------------------------------------------------------
void
CInputServices::EvaluateInfoTracingStatuses()
{
    m_fIsDMInfoTracingEnabled = DMIM_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER);
    m_fIsDMVerboseInfoTracingEnabled = DMIMv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE));
}
#endif // DM_DEBUG

//------------------------------------------------------------------------
//
//  Method:   SetConstantVelocities
//
//  Synopsis:
//      Initiate a constant-velocity pan on the given
//      container.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::SetConstantVelocities(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ XFLOAT panXVelocity,
    _In_ XFLOAT panYVelocity)
{
    HRESULT hr = S_OK;
    CDMViewport* pViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;
    XDMViewportStatus viewportStatus = XcpDMViewportBuilding;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: SetConstantVelocities entry. pDMContainer=0x%p, pManipulatedElement=0x%p, panXVelocity=%4.6lf, panYVelocity=%4.6lf.",
            this, pDMContainer, pManipulatedElement, panXVelocity, panYVelocity));
    }

#endif // DM_DEBUG

    ASSERT(!(panXVelocity != 0.0f && panYVelocity != 0.0f));

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    IFC(GetDMService(pDMContainer, &pDirectManipulationService));

    if (pDirectManipulationService)
    {
        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));

        // We might not have a viewport. If we don't, don't do a pan.
        // This could happen if our content size is smaller than the viewport.
        if (pViewport != NULL)
        {
            IFC(pDirectManipulationService->GetViewportStatus(pViewport, viewportStatus));

#ifdef DM_DEBUG
            if (m_fIsDMVerboseInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                    L"                   pViewport=0x%p, DM status=%d.", pViewport, viewportStatus));
            }

#endif // DM_DEBUG

            // Don't start a constant-velocity pan if we're already in a real DM manipulation.
            if ((viewportStatus != XcpDMViewportRunning &&
                 viewportStatus != XcpDMViewportInertia &&
                 viewportStatus != XcpDMViewportSuspended) ||
                // DManip auto-scroll configuration and status must be reset when the viewport transitions to the Running status
                (panXVelocity == 0.0f && panYVelocity == 0.0f))
            {
                ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

                XFLOAT currentAutoScrollXVelocity = 0.0f;
                XFLOAT currentAutoScrollYVelocity = 0.0f;

                pViewport->GetCurrentAutoScrollVelocities(&currentAutoScrollXVelocity, &currentAutoScrollYVelocity);

                if (panXVelocity != 0.0f || panYVelocity != 0.0f)
                {
                    if ((panXVelocity > 0.0f && currentAutoScrollXVelocity > 0) ||
                        (panXVelocity < 0.0f && currentAutoScrollXVelocity < 0) ||
                        (panYVelocity > 0.0f && currentAutoScrollYVelocity > 0) ||
                        (panYVelocity < 0.0f && currentAutoScrollYVelocity < 0))
                    {
                        // Ongoing auto scroll keeps the same direction and orientation. No need to update anything.
                        goto Cleanup;
                    }

                    // Auto scroll is starting, or changing direction, or changing orientation, or both.
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"DMIM[0x%p]:  SetConstantVelocities. pViewport=0x%p, DM status=%d, panXVelocity=%4.6lf, panYVelocity=%4.6lf.",
                            this, pViewport, viewportStatus, panXVelocity, panYVelocity));
                    }
#endif // DM_DEBUG

                    if ((panXVelocity != 0.0f && currentAutoScrollXVelocity == 0) ||
                        (panYVelocity != 0.0f && currentAutoScrollYVelocity == 0))
                    {
                        // Auto scroll is starting
                        IFC(pViewport->GetDMContainerNoRef()->GetDirectManipulationContainer(&pDirectManipulationContainer));
                        ASSERT(pDirectManipulationContainer);

                        // No need to setup DManip for touch-based auto-scroll since it was already done while processing XCP_POINTERDOWN.
                        if (pViewport->GetState() != ManipulationStarting)
                        {
                            // Ensure DManip is properly set up to handle the interactions with IDirectManipulationAutoScrollBehavior
                            IFC(SetupDirectManipulationViewport(
                                pDMContainer,
                                pManipulatedElement,
                                pViewport,
                                pDirectManipulationContainer,
                                pDirectManipulationService,
                                FALSE /*fIsForTouch*/,
                                TRUE  /*fIsForBringIntoViewport*/,
                                TRUE  /*fIsForAnimatedBringIntoViewport*/,
                                TRUE  /*fIsForFullSetup*/));
                        }
                        else
                        {
                            // Just update and activate the bring-into-viewport configuration which is re-used for auto-scrolling purposes
                            IFC(UpdateManipulationViewport(
                                pDMContainer,
                                pManipulatedElement,
                                FALSE /*fUpdateBounds*/,
                                FALSE /*fUpdateInputTransform*/,
                                FALSE /*fUpdateTouchConfiguration*/,
                                FALSE /*fUpdateNonTouchConfiguration*/,
                                FALSE /*fUpdateConfigurations*/,
                                FALSE /*fUpdateChainedMotionTypes*/,
                                FALSE /*fActivateTouchConfiguration*/,
                                FALSE /*fActivateNonTouchConfiguration*/,
                                TRUE  /*fActivateBringIntoViewConfiguration*/,
                                FALSE /*fUpdateHorizontalOverpanMode*/,
                                FALSE /*fUpdateVerticalOverpanMode*/,
                                NULL  /*pfConfigurationsUpdated*/));
                        }

                        IFC(EnableViewport(pDirectManipulationService, pViewport));

                        IFC(DeclareNewViewportForCompositor(pViewport, pDirectManipulationContainer));

                        if (!pViewport->GetIsCompositorAware())
                        {
                            XFLOAT contentOffsetX = 0.0f;
                            XFLOAT contentOffsetY = 0.0f;

                            XFLOAT contentTranslationX = 0.0f;
                            XFLOAT contentTranslationY = 0.0f;
                            XFLOAT contentZoomFactor = 0.0f;

                            IFC(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
                                pManipulatedElement,
                                TRUE  /*fInManipulation*/,
                                FALSE /*fForInitialTransformationAdjustment*/,
                                FALSE /*fForMargins*/,
                                &contentTranslationX,
                                &contentTranslationY,
                                &contentZoomFactor));

                            // The content offsets are not cleared in between manipulations and they need to be
                            // taken into account in the InitializeDirectManipulationViewportValues call below.
                            pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);
                            contentTranslationX -= contentOffsetX * contentZoomFactor;
                            contentTranslationY -= contentOffsetY * contentZoomFactor;

                            // Set the CDMViewport's initial and current transformation values, as well as for its potential secondary contents,
                            // for mouse-driven auto scrolling. Values were already initialized for touch scenarios and must not be re-initialized
                            // when an auto scroll was interrupted.
                            IFC(InitializeDirectManipulationViewportValues(
                                pViewport,
                                NULL /*pDirectManipulationService*/,
                                contentTranslationX,
                                contentTranslationY,
                                contentZoomFactor));
                        }
                    }

                    if (panXVelocity != 0.0f)
                    {
                        IFC(pDirectManipulationService->ActivateAutoScroll(
                            pViewport,
                            XcpDMMotionTypePanX,
                            panXVelocity < 0.0f /*autoScrollForward*/));
                    }
                    else
                    {
                        IFC(pDirectManipulationService->ActivateAutoScroll(
                            pViewport,
                            XcpDMMotionTypePanY,
                            panYVelocity < 0.0f /*autoScrollForward*/));
                    }

                    pViewport->SetCurrentAutoScrollVelocities(panXVelocity, panYVelocity);
                }
                else if (currentAutoScrollXVelocity != 0.0f || currentAutoScrollYVelocity != 0.0f)
                {
                    // Auto scroll is ending.
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"DMIM[0x%p]:  SetConstantVelocities - ending auto-scroll. pViewport=0x%p, DM status=%d.", this, pViewport, viewportStatus));
                    }
#endif // DM_DEBUG

                    // Provide the motion types of the activated configuration so the DirectManipulation service
                    // can provide the correct motion to IDirectManipulationAutoScrollBehavior::SetConfiguration
                    // when stopping the current auto-scroll.
                    const XDMConfigurations activeConfiguration = pViewport->GetActivatedConfiguration();
                    XDMMotionTypes motionType = XcpDMMotionTypeNone;
                    if ((activeConfiguration & XcpDMConfigurationPanX) == XcpDMConfigurationPanX)
                    {
                        motionType = static_cast<XDMMotionTypes>(motionType | XcpDMMotionTypePanX);
                    }
                    if ((activeConfiguration & XcpDMConfigurationPanY) == XcpDMConfigurationPanY)
                    {
                        motionType = static_cast<XDMMotionTypes>(motionType | XcpDMMotionTypePanY);
                    }
                    IFC(pDirectManipulationService->StopAutoScroll(pViewport, motionType));
                    pViewport->SetCurrentAutoScrollVelocities(0.0f, 0.0f);
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   DiscardRejectionViewportsInSubTree
//
//  Synopsis:
//    Discards the rejection cross-slide viewport that might currently
//    be associated with the provided element and its subtree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DiscardRejectionViewportsInSubTree(
    _In_ CUIElement* pElement)
{
    bool fIsAncestor = false;
    CUIElement* pViewportOwner = NULL;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: DiscardRejectionViewportsInSubTree. pElement=0x%p.", this, pElement));
    }

#endif // DM_DEBUG

    IFCPTR_RETURN(pElement);

    for (XUINT32 iCrossSlideViewport = 0; m_pCrossSlideViewports && iCrossSlideViewport < m_pCrossSlideViewports->size();)
    {
        fIsAncestor = false;
        IFC_RETURN(m_pCrossSlideViewports->get_item(iCrossSlideViewport, pCrossSlideViewport));
        if (pCrossSlideViewport && pCrossSlideViewport->GetIsRejectionViewport())
        {
            pViewportOwner = pCrossSlideViewport->GetDMContainerNoRef();
            if (pViewportOwner)
            {
                fIsAncestor = pElement->IsAncestorOf(pViewportOwner);
                if (fIsAncestor)
                {
                    IFC_RETURN(DirectManipulationCrossSlideContainerCompleted(pViewportOwner, pCrossSlideViewport));
                }
            }
        }
        if (!fIsAncestor)
        {
            iCrossSlideViewport++;
        }
    }

    return S_OK;
}

// Stops the viewport associated with the provided DManip container and
// manipulated element if it's in inertia phase.
_Check_return_ HRESULT
CInputServices::StopInertialViewport(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _Out_ bool* pHandled)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  StopInertialViewport - entry. pDMContainer=0x%p, pManipulatedElement=0x%p.", this, pDMContainer, pManipulatedElement));
    }
#endif // DM_DEBUG

    *pHandled = false;

    xref_ptr<CDMViewport> spViewport;
    IFC_RETURN(GetViewport(pDMContainer, pManipulatedElement, spViewport.ReleaseAndGetAddressOf()));
    if (spViewport != nullptr)
    {
        ASSERT(spViewport->GetDMContainerNoRef() == pDMContainer);

        IFC_RETURN(StopInertialViewport(spViewport, false /*restrictToKnownInertiaEnd*/, pHandled));

        if (*pHandled)
        {
            // Immediately process the status change resulting from the inertia cancellation so that the viewport
            // enters the ManipulationCompleted state.
            IFC_RETURN(ProcessDirectManipulationViewportChanges(spViewport));
        }
    }

    return S_OK;
}

// When the viewport is in inertia phase, jumps to the intended inertia-end-point, unless the manipulated
// element of the viewport is collapsed. In that case, inertia is interrupted and the transform is left
// unchanged, because the content size is changing to 0x0.
// When restrictToKnownInertiaEnd is True, inertia is only stopped when the end-of-inertia transform can
// be retrieved from DManip.
_Check_return_ HRESULT
CInputServices::StopInertialViewport(
    _In_ CDMViewport* pViewport,
    _In_ bool restrictToKnownInertiaEnd,
    _Out_opt_ bool* pHandled)
{
    if (pHandled != nullptr)
    {
        *pHandled = false;
    }

    CUIElement* pDMContainerNoRef = pViewport->GetDMContainerNoRef();
    ASSERT(pDMContainerNoRef != nullptr);

    CUIElement* pManipulatedElement = pViewport->GetManipulatedElementNoRef();
    ASSERT(pManipulatedElement != nullptr);

    xref_ptr<IPALDirectManipulationService> spDirectManipulationService = nullptr;
    IFC_RETURN(GetDMService(pDMContainerNoRef, spDirectManipulationService.ReleaseAndGetAddressOf()));
    ASSERT(spDirectManipulationService != nullptr);

    // Only interrupt the manipulation if the viewport is active.
    XDMViewportStatus status = XcpDMViewportBuilding;
    IFC_RETURN(spDirectManipulationService->GetViewportStatus(pViewport, status));

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  StopInertialViewport. pViewport=0x%p, restrictToKnownInertiaEnd=%d, DM status=%d, IM state=%d.",
            this, pViewport, restrictToKnownInertiaEnd, status, pViewport->GetState()));
    }
#endif // DM_DEBUG

    if (status == XcpDMViewportInertia)
    {
        XFLOAT xInertiaEndTranslation = 0.0f;
        XFLOAT yInertiaEndTranslation = 0.0f;
        XFLOAT zInertiaEndFactor = 1.0f;

        // Only jump to the end-of-inertia transform when the manipulated element is visible.
        bool bringInertiaEndIntoViewport = pManipulatedElement->GetVisibility() == DirectUI::Visibility::Visible;

        if (bringInertiaEndIntoViewport)
        {
            // DManip has a race condition bug which causes GetContentInertiaEndTransform to fail in rare cases.
            // This is indicated by *pIsInertiaEndTransformValid == false.
            // Leave the transform as is then.
            IFC_RETURN(GetDirectManipulationContentInertiaEndTransform(
                spDirectManipulationService,
                pViewport,
                &xInertiaEndTranslation,
                &yInertiaEndTranslation,
                &zInertiaEndFactor,
                &bringInertiaEndIntoViewport /*pIsInertiaEndTransformValid*/));
        }

        // At this point bringInertiaEndIntoViewport is False when no end-of-inertia transform is known,
        // and True when it's known. Do not stop inertia when restrictToKnownInertiaEnd is True and
        // bringInertiaEndIntoViewport is False.
        if (bringInertiaEndIntoViewport || !restrictToKnownInertiaEnd)
        {
            IFC_RETURN(spDirectManipulationService->StopViewport(pViewport));
        }

        if (bringInertiaEndIntoViewport)
        {
            // Jump to the previously intended end-of-inertia transform.
            // Note that NotifyBringIntoViewportNeeded does not raise any public events so m_pViewports->size() has no chance of changing,
            // and pElement has no chance of being released.
            xref_ptr<CUIDMContainer> spDirectManipulationContainer;
            IFC_RETURN(pDMContainerNoRef->GetDirectManipulationContainer(spDirectManipulationContainer.ReleaseAndGetAddressOf()));
            ASSERT(spDirectManipulationContainer != nullptr);
            IFC_RETURN(spDirectManipulationContainer->NotifyBringIntoViewportNeeded(
                pManipulatedElement, xInertiaEndTranslation, yInertiaEndTranslation, zInertiaEndFactor, TRUE /*fTransformIsValid*/, TRUE /*fTransformIsInertiaEnd*/));
        }

        if (pHandled != nullptr)
        {
            *pHandled = bringInertiaEndIntoViewport || !restrictToKnownInertiaEnd;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInputServices::DiscardRejectionViewportsInParentChain
//
//  Synopsis:
//    Discards all rejection cross-slide viewports associated with the
//    provided object and all its ancestors.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DiscardRejectionViewportsInParentChain(
    _In_ CDependencyObject* pDO)
{
    HRESULT hr = S_OK;
    CUIElement* pElement = NULL;
    CDMCrossSlideViewport* pCrossSlideViewport = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: DiscardRejectionViewportsInParentChain. pDO=0x%p.", this, pDO));
    }

#endif // DM_DEBUG

    IFCPTR(pDO);

    if (m_pCrossSlideViewports && m_pCrossSlideViewports->size() > 0)
    {
        do
        {
            pElement = do_pointer_cast<CUIElement>(pDO);
            if (pElement)
            {
                IFC(GetCrossSlideViewport(pElement, &pCrossSlideViewport));
                if (pCrossSlideViewport)
                {
                    if (pCrossSlideViewport->GetIsRejectionViewport())
                    {
                        IFC(DirectManipulationCrossSlideContainerCompleted(pElement, pCrossSlideViewport));
                    }
                    ReleaseInterface(pCrossSlideViewport);
                }
            }
            pDO = pDO->GetParentInternal();
        }
        while (pDO && m_pCrossSlideViewports && m_pCrossSlideViewports->size() > 0);
    }

Cleanup:
    ReleaseInterface(pCrossSlideViewport);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CancelDirectManipulations
//
//  Synopsis:
//    Cancels all ongoing DManip-based manipulations involving the
//    provided element and its ancestors.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CancelDirectManipulations(
    _In_ CUIElement* pElement,
    _Out_ bool* pfHandled)
{
    HRESULT hr = S_OK;
    XUINT32 cContactIds = 0;
    XDMViewportStatus oldStatus = XcpDMViewportBuilding;

    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CDependencyObject* pDO = pElement;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  CancelDirectManipulations entry. pElement=0x%p.", this, pElement));
    }

#endif // DM_DEBUG

    IFCPTR(pElement);
    IFCPTR(pfHandled);
    *pfHandled = FALSE;

    do
    {
        if (pElement && pElement->GetIsDirectManipulationContainer())
        {
            IFC(GetDMService(pElement, &pDirectManipulationService));
            if (pDirectManipulationService)
            {
                xref_ptr<CDMViewport> viewport(GetViewport(pElement /*pDMContainer*/));
                if (viewport)
                {
                    ASSERT(pElement == viewport->GetDMContainerNoRef());
                    ASSERT(!viewport->GetNeedsUnregistration());
                    ASSERT(!viewport->GetUnregistered());

                    IFC(pDirectManipulationService->GetViewportStatus(viewport, status));
                    IFC(viewport->GetOldStatus(oldStatus));
                    IFC(viewport->GetCurrentStatus(currentStatus));
                    IFC(viewport->GetContactIdCount(&cContactIds));

#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"DMIM[0x%p]:  CancelDirectManipulations - pViewport=0x%p, cContactIds=%d, DM status=%d, IM state=%d, IM old status=%d, IM current status=%d.",
                            this, viewport.get(), cContactIds, status, viewport->GetState(), oldStatus, currentStatus));
                    }
#endif // DM_DEBUG

                    // Viewports that are in auto-running for a constant velocity pan are not eligible for cancellation.
                    if (status != XcpDMViewportAutoRunning)
                    {
                        if (IsViewportActive(status) || IsViewportActive(currentStatus))
                        {
                            // When status is active, viewport needs to be disabled, and all contacts are released.
                            // New inactive status will be handled asynchronously and complete the manipulation.
                            // When only currentStatus is active, do the same. The manipulation is about to be completed.
                            IFC(CompleteDirectManipulation(viewport, TRUE /*fDisableViewport*/));
                            *pfHandled = TRUE;
                        }
                        else
                        {
                            // When the old status is active, there is a pending transition from active to inactive which
                            // will complete the ongoing manipulation. So no need to call CompleteDirectManipulation then.
                            if (!IsViewportActive(oldStatus) &&
                                (cContactIds > 0 ||
                                (viewport->GetState() != ManipulationNone &&
                                viewport->GetState() != ManipulationCompleted &&
                                viewport->GetState() != ConstantVelocityScrollStopped)))
                            {
                                if (cContactIds > 0)
                                {
                                    // Disable viewport and release all contacts.
                                    IFC(CompleteDirectManipulation(viewport, TRUE /*fDisableViewport*/));
                                }
                                // Finally send the ManipulationCompleted notification and call DeclareOldViewportForCompositor.
                                IFC(CompleteDirectManipulation(viewport, FALSE /*fDisableViewport*/));
                                *pfHandled = TRUE;
                            }
                        }
                    }
                }
                ReleaseInterface(pDirectManipulationService);
            }
        }

        // Using fPublic==False to be able to reach the root ScrollViewer.
        pDO = pDO->GetParentInternal(false /*fPublic*/);
        pElement = do_pointer_cast<CUIElement>(pDO);
    }
    while (pDO);

Cleanup:
    ReleaseInterface(pDirectManipulationService);
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled && pfHandled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  CancelDirectManipulations exit. handled=%d.", this, *pfHandled));
    }

#endif // DM_DEBUG
    RRETURN(hr);
}

// Attempts to start a manipulation on the UIElement associated with the given Pointer.
_Check_return_ HRESULT
CInputServices::TryStartDirectManipulation(
    _In_ CPointer* pValue,
    _Out_ bool* pHandled)
{
    *pHandled = false;

    // This method has a public entry point so let's do some validation on the input.
    //
    // Validate the Pointer is touch, this is the only type we handle DManip manipulations on.
    // If the PenNavigation feature is on DManip will also be handled on pen manipulations.

    if (pValue->m_pointerDeviceType != DirectUI::PointerDeviceType::Pen &&
        pValue->m_pointerDeviceType != DirectUI::PointerDeviceType::Touch)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Try to find the mapping from pointerID -> UIElement.
    // We will have a valid entry for this pointerID if we've started tracking it in the
    // WM_POINTERDOWN handler.  If we don't have an entry, it means we're not tracking it
    // and the result will be a no-op and return false.  This helps reinforce that we only
    // start a manipulation that would have been started normally in our WM_POINTERDOWN handler.
    UINT32 pointerId = pValue->GetPointerId();
    {
        PointerDownTrackerMap::iterator itFind = m_mapPointerDownTracker.find(pointerId);
        if (itFind != m_mapPointerDownTracker.end())
        {
            CUIElement* pTrackedElement = itFind->second;
            IFC_RETURN(InitializeDirectManipulationForPointerId(pointerId, FALSE /*fIsForDMHitTest*/, pTrackedElement, pHandled));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   NotifyReleaseManipulationContainer
//
//  Synopsis:
//    Called when the container is being destroyed and the manipulation
//    handler needs to release all references related to it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyReleaseManipulationContainer(_In_ CUIElement* pDMContainer)
{
    bool fViewportUnregistered = false;
    CDMViewport* pViewport = NULL;
    IPALDirectManipulationService* pDMService = NULL;

    IFCPTR_RETURN(pDMContainer);

    if (m_pViewports)
    {
        do
        {
            fViewportUnregistered = FALSE;
            for (XUINT32 iViewport = 0; iViewport < m_pViewports->size(); iViewport++)
            {
                IFC_RETURN(m_pViewports->get_item(iViewport, pViewport));
                ASSERT(pViewport);

                if (pDMContainer == pViewport->GetDMContainerNoRef())
                {
                    IFC_RETURN(UnregisterViewport(pViewport));
                    fViewportUnregistered = TRUE;
                    break;
                }
            }
        }
        while (fViewportUnregistered);
    }

    if (m_pDMServices)
    {
        for (xchainedmap<CUIElement*, IPALDirectManipulationService*>::const_iterator it = m_pDMServices->begin();
            it != m_pDMServices->end();
            ++it)
        {
            if (pDMContainer == (*it).first)
            {
                pDMService = (*it).second;
                break;
            }
        }

        IFC_RETURN(m_pDMServices->Remove(pDMContainer, pDMService));
        ReleaseInterface(pDMContainer);
        ReleaseInterface(pDMService);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   NotifyCanManipulateElements
//
//  Synopsis:
//    Called when the container's ability to manipulate elements has changed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyCanManipulateElements(
    _In_ CUIElement* pDMContainer,
    _In_ bool fCanManipulateElementsByTouch,
    _In_ bool fCanManipulateElementsNonTouch,
    _In_ bool fCanManipulateElementsWithBringIntoViewport)
{
    ASSERT(pDMContainer);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifyCanManipulateElements entry - pDMContainer=0x%p, fCanManipulateElementsByTouch=%d, fCanManipulateElementsNonTouch=%d, fCanManipulateElementsWithBringIntoViewport=%d.",
            this, pDMContainer, fCanManipulateElementsByTouch, fCanManipulateElementsNonTouch, fCanManipulateElementsWithBringIntoViewport));
    }
#endif // DM_DEBUG

    // Ongoing manipulation must be cancelled when
    // - touch configuration is active and fCanManipulateElementsByTouch is false or
    // - non-touch configuration is active and fCanManipulateElementsNonTouch is false or
    // - bring-into-viewport configuration is active and fCanManipulateElementsWithBringIntoViewport is false
    // UpdateDirectManipulationManagerActivation does those checks.
    RRETURN(UpdateDirectManipulationManagerActivation(
        pDMContainer,
        TRUE /*fCancelManipulations*/,
        fCanManipulateElementsByTouch,
        fCanManipulateElementsNonTouch,
        fCanManipulateElementsWithBringIntoViewport,
        FALSE /*fRefreshViewportStatus*/));
}

//------------------------------------------------------------------------
//
//  Method:   NotifyManipulatableElementChanged
//
//  Synopsis:
//    Called when:
//     - originally, when CUIDMContainer.put_Handler is called in order to declare the existing manipulated elements.
//     - afterwards, whenever the list of manipulated elements has changed.
//    pOldManipulatableElement == NULL && pNewManipulatableElement != NULL ==> a new manipulated element is available
//    pOldManipulatableElement != NULL && pNewManipulatableElement == NULL ==> an old manipulated element is gone
//    pOldManipulatableElement != NULL && pNewManipulatableElement != NULL ==> an old manipulated element was replaced with another one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyManipulatableElementChanged(
    _In_ CUIElement* pDMContainer,
    _In_opt_ CUIElement* pOldManipulatableElement,
    _In_opt_ CUIElement* pNewManipulatableElement)
{
    HRESULT hr = S_OK;
    XUINT8 cConfigurations = 0;
    XUINT32 cContactIds = 0;
    XUINT32 contactId = 0;
    XDMViewportStatus oldStatus = XcpDMViewportBuilding;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMConfigurations* pConfigurations = NULL;
    XDMConfigurations activatedConfiguration = XcpDMConfigurationNone;
    XDMConfigurations touchConfiguration = XcpDMConfigurationNone;
    XDMConfigurations nonTouchConfiguration = XcpDMConfigurationNone;
    CDMViewport* pNewViewport = NULL;
    CDMViewport* pViewport = NULL;
    CDMContent* pContent = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;
    bool fContentOffsetChanged = false;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifyManipulatableElementChanged entry - pDMContainer=0x%p, pOldManipulatableElement=0x%p, pNewManipulatableElement=0x%p.",
            this, pDMContainer, pOldManipulatableElement, pNewManipulatableElement));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);

    ASSERT(pOldManipulatableElement || pNewManipulatableElement);

    if (!pOldManipulatableElement && pNewManipulatableElement)
    {
        // First check if that new manipulatable element is already associated with a viewport for the provided DM container.
        IFC(GetViewport(pDMContainer, pNewManipulatableElement, &pViewport));
        if (pViewport)
        {
            // Reuse the existing viewport that was about to be unregistered.

            // This situation occurs when a DM container, or its manipulatable element, momentarily leaves the tree
            // during a manipulation, or while there are contact points but no manipulation yet. When leaving the tree,
            // the manipulation is canceled and the viewport is marked for unregistration. When re-entering the tree,
            // the unregistration had no time to complete.
            ASSERT(pViewport->GetNeedsUnregistration());
            ASSERT(pNewManipulatableElement->IsManipulatable());

            // Clear the unregistration requirement flag
            pViewport->SetNeedsUnregistration(FALSE /*fNeedsUnregistration*/);

            // Clear old statuses
            do
            {
                IFC(pViewport->GetOldStatus(oldStatus));
                if (oldStatus != XcpDMViewportBuilding)
                {
                    IFC(pViewport->RemoveOldStatus());
                }
            }
            while (oldStatus != XcpDMViewportBuilding);

            // Clear the ManipulationCompleted state and use the state of a newly created viewport
            pViewport->SetState(ManipulationNone);

            // Clear the queued interaction types so no superfluous ScrollViewer.DirectManipulationStarted/Completed
            // event gets raised for this new usage of the CDMViewport instance.
            pViewport->ClearQueuedInteractionTypes();
            // Clear the flag stating an interaction completion notification is expected.
            pViewport->SetIsTouchInteractionEndExpected(FALSE);
            // Clear the flag stating an interaction start notification was already processed.
            pViewport->SetIsTouchInteractionStartProcessed(FALSE);
            // Clear the flag stating a contact Id was received during an inertia phase.
            pViewport->SetHasReceivedContactIdInInertia(FALSE);
            // Clear the flag requesting another hit test once inertia completes.
            pViewport->SetRequestReplayPointerUpdateWhenInertiaCompletes(FALSE);

            pViewport->SetHasValidBounds(false);

            // Clear the existing contact IDs, if any
            do
            {
                IFC(pViewport->GetContactIdCount(&cContactIds));
                if (cContactIds > 0)
                {
                    IFC(pViewport->GetContactId(0, &contactId));
                    IFC(pViewport->RemoveContactId(contactId));
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   pViewport=0x%p, Removed pointerId=%d, New contactId count=%d.",
                            pViewport, contactId, cContactIds));
                    }
#endif // DM_DEBUG
                }
            }
            while (cContactIds > 1);

            // Disable the viewport and release all existing contacts for the case where no manipulation had started.
            IFC(GetDMService(pDMContainer, &pDirectManipulationService));
            if (pDirectManipulationService)
            {
                IFC(pDirectManipulationService->DisableViewport(pViewport));
                IFC(pDirectManipulationService->ReleaseAllContacts(pViewport));
            }

            // Clear the potential secondary contents. If they are still present on the DM container side,
            // they will be re-added later.
            for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
            {
                IFC(pViewport->GetContent(iContent, &pContent));
                if (pContent)
                {
                    IFC(RemoveSecondaryContent(pContent->GetContentElementNoRef(), pContent, pViewport, pDirectManipulationService));
                    ReleaseInterface(pContent);
                    iContent--;
                }
            }

            for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
            {
                IFC(pViewport->GetClipContent(iClipContent, &pContent));
                if (pContent)
                {
                    IFC(RemoveSecondaryClipContent(pContent->GetContentElementNoRef(), pContent, pViewport, pDirectManipulationService));
                    ReleaseInterface(pContent);
                    iClipContent--;
                }
            }

            // Clear the potential m_cRemovedRunningStatuses count in the CDMViewport to start from scratch.
            pViewport->ResetRemovedRunningStatuses();

            // m_cIgnoredRunningStatuses is used for synchronous BringIntoViewport calls.
            pViewport->ResetIgnoredRunningStatuses();

            // Clear the m_fIsProcessingMakeVisibleInertia flag in case the viewport was processing a bring-into-viewport
            // animation for a MakeVisible call.
            pViewport->SetIsProcessingMakeVisibleInertia(FALSE /*fIsProcessingMakeVisibleInertia*/);

            // Also reset the content offsets since they are not cleared at the end of a manipulation.
            pViewport->SetContentOffsets(0.0f, 0.0f, &fContentOffsetChanged);

            if (fContentOffsetChanged)
            {
                IFC(UpdateSecondaryContentsOffsets(pDMContainer, pNewManipulatableElement, pViewport));
            }

            ReleaseInterface(pDirectManipulationService);
            ReleaseInterface(pViewport);
        }
        else
        {
            // A new manipulatable element is being declared. It needs a dedicated DM viewport.
            IFC(CDMViewport::Create(&pNewViewport, pDMContainer, pNewManipulatableElement));

            IFC(EnsureViewports());
            IFC(m_pViewports->push_back(pNewViewport));
            pViewport = pNewViewport;
            AddRefInterface(pViewport);
            pNewViewport = NULL;

            ASSERT(pViewport->GetContentsCount() == 0);
            ReleaseInterface(pViewport);

            if (!pNewManipulatableElement->IsManipulatable())
            {
                // Mark the element as manipulatable
                // This assumes that at least one of CanManipulateElementsByTouch, CanManipulateElementsNonTouch and CanManipulateElementsWithBringIntoViewport is True.
                IFC(pNewManipulatableElement->SetRequiresComposition(
                    CompositionRequirement::Manipulatable,
                    IndependentAnimationType::None));

                // Since pViewport->GetContentsCount() == 0, no secondary content needs to be declared as manipulatable.
            }
        }

        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(GetViewport(pDMContainer, pNewManipulatableElement, &pViewport));
            ASSERT(pViewport);

            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
            IFC(pDirectManipulationContainer->GetManipulationViewport(
                pNewManipulatableElement,
                NULL /*pBounds*/,
                NULL /*pInputTransform*/,
                &touchConfiguration,
                &nonTouchConfiguration,
                NULL /*pBringIntoViewportConfiguration*/,
                &cConfigurations,
                &pConfigurations,
                NULL /*pChainedMotionTypes*/,
                NULL /*pHorizontalOverpanMode*/,
                NULL /*pVerticalOverpanMode*/));
            IFC(UpdateManipulationConfigurations(
                pDirectManipulationService,
                pViewport,
                cConfigurations,
                pConfigurations,
                NULL /*pfConfigurationsUpdated*/));
            IFC(UpdateManipulationTouchConfiguration(
                touchConfiguration == XcpDMConfigurationNone ? NULL : pDirectManipulationService,
                pViewport,
                touchConfiguration));
            IFC(UpdateManipulationNonTouchConfiguration(
                (touchConfiguration == XcpDMConfigurationNone && nonTouchConfiguration != XcpDMConfigurationNone) ? pDirectManipulationService : NULL,
                pViewport,
                nonTouchConfiguration));
            IFC(UpdateManipulationOverpanModes(
                pDirectManipulationService,
                pViewport,
                FALSE /*fIsStartingNewManipulation*/));
            IFC(UpdateManipulationViewport(
                pDMContainer,
                pNewManipulatableElement,
                TRUE  /*fUpdateBounds*/,
                FALSE /*fUpdateInputTransform*/,
                FALSE /*fUpdateTouchConfiguration*/,
                FALSE /*fUpdateNonTouchConfiguration*/,
                FALSE /*fUpdateConfigurations*/,
                FALSE /*fUpdateChainedMotionTypes*/,
                FALSE /*fActivateTouchConfiguration*/,
                FALSE /*fActivateNonTouchConfiguration*/,
                FALSE /*fActivateBringIntoViewConfiguration*/,
                FALSE /*fUpdateHorizontalOverpanMode*/,
                FALSE /*fUpdateVerticalOverpanMode*/,
                NULL  /*pfConfigurationsUpdated*/));
            // Let the DM container make a BringIntoViewport call to synchronize DManip and XAML.
            // This synchronization is required when the manipulatable element is changed from A to null to A in a single tick.
            IFC(pDirectManipulationContainer->NotifyBringIntoViewportNeeded(pNewManipulatableElement));
        }
    }
    else if (pOldManipulatableElement)
    {
        IFC(GetViewport(pDMContainer, pOldManipulatableElement, &pViewport));
        ASSERT(pViewport);
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        if (pNewManipulatableElement)
        {
            // One manipulatable element is replaced with another one.
            IFC(pViewport->SetManipulatedElement(pNewManipulatableElement));

            if (pViewport->GetIsCompositorAware())
            {
                // The swap occurs during an active manipulation
                activatedConfiguration = pViewport->GetActivatedConfiguration();

                if ((activatedConfiguration & XcpDMConfigurationPanX) != 0)
                {
                    IFC(UpdateManipulationSnapPoints(
                        pDMContainer,
                        pNewManipulatableElement,
                        XcpDMMotionTypePanX));
                }
                if ((activatedConfiguration & XcpDMConfigurationPanY) != 0)
                {
                    IFC(UpdateManipulationSnapPoints(
                        pDMContainer,
                        pNewManipulatableElement,
                        XcpDMMotionTypePanY));
                }
                if ((activatedConfiguration & XcpDMConfigurationZoom) != 0)
                {
                    IFC(UpdateManipulationSnapPoints(
                        pDMContainer,
                        pNewManipulatableElement,
                        XcpDMMotionTypeZoom));
                }

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   NotifyManipulatableElementChanged. Calling UnsetRequiresCompositionNode for pOldManipulatableElement=0x%p.", pOldManipulatableElement));
                }
#endif // DM_DEBUG

                // The old manipulated element no longer requires a composition node
                pOldManipulatableElement->UnsetRequiresComposition(
                    CompositionRequirement::IndependentManipulation,
                    IndependentAnimationType::None
                    );

                // ...and it can no longer be manipulated
                pOldManipulatableElement->UnsetRequiresComposition(
                    CompositionRequirement::Manipulatable,
                    IndependentAnimationType::None);

                // Only the manipulatable primary content changed - no need to update the manipulability of the secondary contents.

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   NotifyManipulatableElementChanged. Calling SetRequiresCompositionNode for pNewManipulatableElement=0x%p.", pNewManipulatableElement));
                }
#endif // DM_DEBUG

                // The new manipulated element now requires a composition node
                // but first it needs to be marked as manipulatable.
                IFC(pNewManipulatableElement->SetRequiresComposition(
                    CompositionRequirement::Manipulatable,
                    IndependentAnimationType::None));
                IFC(pNewManipulatableElement->SetRequiresComposition(
                    CompositionRequirement::IndependentManipulation,
                    IndependentAnimationType::None
                    ));
            }
            else
            {
                // No manipulation in progress.

                // Mark the old element as no longer manipulatable
                pOldManipulatableElement->UnsetRequiresComposition(
                    CompositionRequirement::Manipulatable,
                    IndependentAnimationType::None);

                // Mark the new element as manipulatable
                IFC(pNewManipulatableElement->SetRequiresComposition(
                    CompositionRequirement::Manipulatable,
                    IndependentAnimationType::None));

                IFC(GetDMService(pDMContainer, &pDirectManipulationService));
                if (pDirectManipulationService)
                {
                    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
                    IFC(pDirectManipulationContainer->GetManipulationViewport(
                        pNewManipulatableElement,
                        NULL /*pBounds*/,
                        NULL /*pInputTransform*/,
                        &touchConfiguration,
                        &nonTouchConfiguration,
                        NULL /*pBringIntoViewportConfiguration*/,
                        &cConfigurations,
                        &pConfigurations,
                        NULL /*pChainedMotionTypes*/,
                        NULL /*pHorizontalOverpanMode*/,
                        NULL /*pVerticalOverpanMode*/));
                    // pConfigurations is NULL when the DM container becomes non-manipulatable without detection.
                    if (pConfigurations)
                    {
                        IFC(UpdateManipulationConfigurations(
                            pDirectManipulationService,
                            pViewport,
                            cConfigurations,
                            pConfigurations,
                            NULL /*pfConfigurationsUpdated*/));
                        IFC(UpdateManipulationTouchConfiguration(
                            touchConfiguration == XcpDMConfigurationNone ? NULL : pDirectManipulationService,
                            pViewport,
                            touchConfiguration));
                        IFC(UpdateManipulationNonTouchConfiguration(
                            (touchConfiguration == XcpDMConfigurationNone && nonTouchConfiguration != XcpDMConfigurationNone) ? pDirectManipulationService : NULL,
                            pViewport,
                            nonTouchConfiguration));
                        // Reset the notifications countdown so the new potential snap points for instance get pushed to
                        // DirectManipulation at the next manipulation start.
                        IFC(SetDirectManipulationHandlerWantsNotifications(
                            pViewport,
                            pNewManipulatableElement,
                            pDirectManipulationContainer,
                            FALSE /*fWantsNotifications*/));
                    }
                }
            }
        }
        else
        {
            // An old manipulatable element was removed. Its dedicated DM viewport needs to be unregistered.
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Old manipulatable element was removed for pViewport=0x%p.", pViewport));
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   HasOldManipulation=%d, HasNewManipulation=%d, IsCompositorAware=%d, IM state=%d.",
                    pViewport->GetHasOldManipulation(), pViewport->GetHasNewManipulation(), pViewport->GetIsCompositorAware(), pViewport->GetState()));
            }
#endif // DM_DEBUG

            // Reset the notifications countdown since the viewport is being unregistered.
            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
            IFC(SetDirectManipulationHandlerWantsNotifications(
                pViewport,
                pOldManipulatableElement,
                pDirectManipulationContainer,
                FALSE /*fWantsNotifications*/));

            const bool fIsCompositorAware = pViewport->GetIsCompositorAware();

            if (fIsCompositorAware)
            {
                pViewport->SetHasOldManipulation(TRUE);
            }

            // Not only complete the manipulation when the IsCompositorAware flag is set, but also when it's not set yet even though a manipulation is
            // in the starting state. This situation occurs when a ScrollViewer is removed from the visual tree just as a manipulation starts.
            // For example when a finger comes down onto the ScrollViewer, its corresponding CDMViewport HasNewManipulation flag goes from False to True
            // and State goes to ManipulationStarting. Later its IsCompositorAware flag goes from False to True. When the ScrollViewer happens to leave
            // the tree in between those two stages, the CDMViewport needs to transition from the ManipulationStarting to the ManipulationCompleted state.
            if (fIsCompositorAware || (pViewport->GetHasNewManipulation() && pViewport->GetState() == ManipulationStarting))
            {
                // Make sure the ManipulationCompleted event is raised for the ongoing manipulation
                pViewport->SetNeedsUnregistration(TRUE);

                IFC(pViewport->GetCurrentStatus(currentStatus));
                IFC(ProcessDirectManipulationViewportStatusUpdate(
                    pViewport,
                    currentStatus /*oldStatus*/,
                    currentStatus /*newStatus*/,
                    FALSE /*fIsValuesChangePreProcessing*/,
                    NULL  /*pfIgnoreStatusChange*/,
                    NULL  /*pfManipulationCompleted*/));
            }

            if (!fIsCompositorAware)
            {
                // Unregister the viewport immediately if it has not been declared to the compositor yet.
                // Else the unregistration occurs later in CInputServices::GetOldDirectManipulationViewport.
                IFC(UnregisterViewport(pViewport));
            }
        }
    }

Cleanup:
    delete[] pConfigurations;
    ReleaseInterface(pNewViewport);
    ReleaseInterface(pViewport);
    ReleaseInterface(pContent);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifySecondaryContentAdded
//
//  Synopsis:
//    Notification from a DM container that a secondary content was
//    added. A new CDMContent instance is created if there is an
//    existing CDMViewport, and the DManip service registers the secondary
//    content if available.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifySecondaryContentAdded(
    _In_ CUIElement* pDMContainer,
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement,
    _In_ XDMContentType contentType)
{
    HRESULT hr = S_OK;
    XUINT32 cDefinitions = 0;
    CParametricCurveDefinition *pDefinitions = NULL;
    XUINT32 nIndex = 0;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifySecondaryContentAdded entry - pDMContainer=0x%p, pManipulatableElement=0x%p, pContentElement=0x%p, contentType=%d.",
            this, pDMContainer, pManipulatableElement, pContentElement, contentType));
    }
#endif // DM_DEBUG

    switch (contentType)
    {
    case XcpDMContentTypeTopLeftHeader:
        cDefinitions = 2;
        break;

    case XcpDMContentTypeLeftHeader:
    case XcpDMContentTypeTopHeader:
        cDefinitions = 1;
        break;
    }

    if (cDefinitions > 0)
    {
        pDefinitions = new CParametricCurveDefinition[cDefinitions];
    }

    if (contentType == XcpDMContentTypeLeftHeader || contentType == XcpDMContentTypeTopLeftHeader)
    {
        pDefinitions[nIndex].m_segments = 1;
        pDefinitions[nIndex].m_pSegments = new CParametricCurveSegmentDefinition[1];

        pDefinitions[nIndex].m_pSegments[0].m_beginOffset = 0;
        pDefinitions[nIndex].m_pSegments[0].m_constantCoefficient = 0;
        pDefinitions[nIndex].m_pSegments[0].m_linearCoefficient = 0;
        pDefinitions[nIndex].m_pSegments[0].m_quadraticCoefficient = 0;
        pDefinitions[nIndex].m_pSegments[0].m_cubicCoefficient = 0;

        pDefinitions[nIndex].m_primaryDMProperty = XcpDMPropertyTranslationX;
        pDefinitions[nIndex].m_secondaryDMProperty = XcpDMPropertyTranslationX;

        nIndex++;
    }

    if (contentType == XcpDMContentTypeTopHeader || contentType == XcpDMContentTypeTopLeftHeader)
    {
        pDefinitions[nIndex].m_segments = 1;
        pDefinitions[nIndex].m_pSegments = new CParametricCurveSegmentDefinition[1];

        pDefinitions[nIndex].m_pSegments[0].m_beginOffset = 0;
        pDefinitions[nIndex].m_pSegments[0].m_constantCoefficient = 0;
        pDefinitions[nIndex].m_pSegments[0].m_linearCoefficient = 0;
        pDefinitions[nIndex].m_pSegments[0].m_quadraticCoefficient = 0;
        pDefinitions[nIndex].m_pSegments[0].m_cubicCoefficient = 0;

        pDefinitions[nIndex].m_primaryDMProperty = XcpDMPropertyTranslationY;
        pDefinitions[nIndex].m_secondaryDMProperty = XcpDMPropertyTranslationY;

        nIndex++;
    }

    IFC(NotifySecondaryContentAdded(
        pDMContainer,
        pManipulatableElement,
        pContentElement,
        contentType,
        cDefinitions,
        pDefinitions,
        NULL));

Cleanup:
    if (pDefinitions)
    {
        for (XUINT32 i = 0; i < cDefinitions; i++)
        {
            delete[] pDefinitions[i].m_pSegments;
        }

        delete[] pDefinitions;
        pDefinitions = NULL;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifySecondaryContentAdded
//
//  Synopsis:
//    Notification from a DM container that a secondary content was
//    added. A new CDMContent instance is created if there is an
//    existing CDMViewport, and the DManip service registers the secondary
//    content if available.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifySecondaryContentAdded(
    _In_ CUIElement* pDMContainer,
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement,
    _In_ XDMContentType contentType,
    _In_ XUINT32 cDefinitions,
    _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions,
    _In_opt_ CSecondaryContentRelationship *pSecondaryContentRelationship)
{
    HRESULT hr = S_OK;
    XFLOAT initialTranslationX = 0.0f;
    XFLOAT initialTranslationY = 0.0f;
    XFLOAT initialUncompressedZoomFactor = 1.0f;
    XFLOAT initialZoomFactorX = 1.0f;
    XFLOAT initialZoomFactorY = 1.0f;
    XFLOAT currentTranslationX = 0.0f;
    XFLOAT currentTranslationY = 0.0f;
    XFLOAT currentUncompressedZoomFactor = 1.0f;
    XFLOAT currentZoomFactorX = 1.0f;
    XFLOAT currentZoomFactorY = 1.0f;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;
    CDMViewport* pViewport = NULL;
    CDMContent* pContent = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;
    bool bTargetsClip = pSecondaryContentRelationship != NULL && pSecondaryContentRelationship->m_shouldTargetClip;
    bool bNewContentCreated = false;
    bool disableViewport = false;
    XDMViewportStatus status = XcpDMViewportBuilding;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifySecondaryContentAdded entry - pDMContainer=0x%p, pManipulatableElement=0x%p, pContentElement=0x%p, contentType=%d.",
            this, pDMContainer, pManipulatableElement, pContentElement, contentType));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);
    IFCPTR(pContentElement);

    if (pManipulatableElement)
    {
        IFC(GetViewport(pDMContainer, pManipulatableElement, &pViewport));
    }

    if (pViewport)
    {
        // DM container has an associated viewport
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        IFC(GetContentForContentElement(pContentElement, pSecondaryContentRelationship != NULL ? pSecondaryContentRelationship->m_shouldTargetClip : FALSE, &pContent));

        if (pContent == NULL)
        {
            if (bTargetsClip)
            {
                IFC(pViewport->AddClipContent(pContentElement, contentType, &pContent));
            }
            else
            {
                IFC(pViewport->AddContent(pContentElement, contentType, &pContent));
            }

            bNewContentCreated = TRUE;
        }

        ASSERT(pContent);

        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDirectManipulationService->GetViewportStatus(pViewport, status));

            // Threshold Bug #1239265
            // We must take special care to enable the viewport for DManip to fully invalidate the viewport
            // after setting up the secondary content.  Not doing so would result in the content's transform
            // not being updated immediately, and leaving it in this state could cause the wrong transform
            // to show up on screen.
            if (status == XcpDMViewportReady || status == XcpDMViewportBuilding || status == XcpDMViewportDisabled)
            {
                IFC(EnableViewport(pDirectManipulationService, pViewport));
            }

            // In addition to the above, we must also take care not to inadvertently keep the viewport enabled if it was
            // previously disabled.  In this case we keep track of this and disable the viewport after setting up the
            // secondary content.
            if (status == XcpDMViewportDisabled)
            {
                disableViewport = true;
            }

            pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);
            if (bTargetsClip)
            {
                IFC(pDirectManipulationService->AddSecondaryClipContent(pViewport, pContent, cDefinitions, pDefinitions, contentOffsetX, contentOffsetY));
            }
            else
            {
                IFC(pDirectManipulationService->AddSecondaryContent(pViewport, pContent, cDefinitions, pDefinitions, contentOffsetX, contentOffsetY));
            }
        }

        if (pSecondaryContentRelationship != NULL)
        {
            pContent->SetSecondaryContentRelationship(pSecondaryContentRelationship);
        }

        // If we temporarily enabled the viewport above, disable it again.
        if (disableViewport)
        {
            IFC(DisableViewport(pDMContainer, pManipulatableElement));
        }

        if (bNewContentCreated)
        {
            // If pManipulatableElement is marked as manipulatable, its new secondary content also needs to be marked as such.
            ASSERT(pManipulatableElement);
            if (pManipulatableElement->IsManipulatable())
            {
                if (!pContentElement->IsManipulatable())
                {
                    IFC(pContentElement->SetRequiresComposition(
                        CompositionRequirement::Manipulatable,
                        IndependentAnimationType::None));
                }
            }

            if (pViewport->GetIsCompositorAware())
            {
                // Secondary content is added during a manipulation, mark it as requiring a composition node.
                if (bTargetsClip)
                {
                    if (!pContentElement->IsClipManipulatedIndependently())
                    {
                        IFC(pContentElement->SetRequiresComposition(
                            CompositionRequirement::IndependentClipManipulation,
                            IndependentAnimationType::None));
                    }
                }
                else
                {
                    if (!pContentElement->IsManipulatedIndependently())
                    {
                        IFC(pContentElement->SetRequiresComposition(
                            CompositionRequirement::IndependentManipulation,
                            IndependentAnimationType::None));
                    }
                }

                // Set the initial and current transforms based on the primary content's transforms.
                pViewport->GetInitialTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                pViewport->GetCurrentTransformationValues(currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY);

                switch (pContent->GetContentType())
                {
                case XcpDMContentTypeTopLeftHeader:
                case XcpDMContentTypeCustom:
                case XcpDMContentTypeDescendant:
                    initialTranslationX = 0.0f;
                    currentTranslationX = 0.0f;
                    initialTranslationY = 0.0f;
                    currentTranslationY = 0.0f;
                    break;
                case XcpDMContentTypeTopHeader:
                    initialTranslationY = 0.0f;
                    currentTranslationY = 0.0f;
                    break;
                case XcpDMContentTypeLeftHeader:
                    initialTranslationX = 0.0f;
                    currentTranslationX = 0.0f;
                    break;
                }

                pContent->SetInitialTransformationValues(initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);
                pContent->SetCurrentTransformationValues(currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY);

#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Initial transform for new secondary pContent=0x%p, initialTranslationX=%4.6lf, initialTranslationY=%4.6lf, initialUncompressedZoomFactor=%4.8lf, initialZoomFactorX=%4.8lf, initialZoomFactorY=%4.8lf.",
                        pContent, initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY));
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Current transform for new secondary pContent=0x%p, currentTranslationX=%4.6lf, currentTranslationY=%4.6lf, currentUncompressedZoomFactor=%4.8lf, currentZoomFactorX=%4.8lf, currentZoomFactorY=%4.8lf.",
                        pContent, currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY));
                }
#endif // DM_DEBUG

            }
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pContent);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

// This function helps synchronize changing sticky header curves.
// Dynamic changes to sticky header curves exposes a tricky synchronization issue with DManip.
// The flow that leads to a problem goes like this:
//
// -The UI thread runs layout and detects a change to the position/size of a sticky header group.
// -In response to this change, we re-generate a new sticky header curve for the affected group.
// -On this same UI thread frame, we are submitting a new DComp batch drawing all the content at its new location.
//
// The problem with this flow is that as we create a new sticky header curve and hand it over to DManip,
// DManip applies this new curve immediately to the header's shared transform and sends this updated transform
// over to the DWM using its dedicated DComp device.
// Meanwhile we are still preparing the batch of content changes to submit on XAML's DComp device, which can include
// changing the position of the sticky header.
// These two changes (DManip's change to the shared transform, and XAML's change to the position) need to be picked up
// on the same DWM frame, otherwise the content will first jump to the wrong location, then jump back.  Unfortunately
// there's no guarantee these will be picked up together so we see the sticky header jump around before setting.
//
// This problem is fixed by forcing synchronization through XAML's device and is unfortunately very tricky.
// The technique works like this:
// Instead of overwriting the DM content with a new curve, we create a brand new piece of DM content and
// a brand new shared transform.  We put the updated curve on this new piece of content.  Since the operation
// of switching from the old transform to the new transform is done on the XAML DComp device, DManip is free to
// update its side of the shared transform and this change won't show up on screen until after the XAML batch has
// been picked up.  This XAML batch also carries the change to the position of the header.
// Another key thing is that if a manipulation is in progress, we must allow the old curve and the old transform to
// continue to stay live, otherwise the manipulation will stutter.  We accomplish this by transferring information
// about which content/transform to release into a deferral data structure, DMDeferredRelease, and we wait until
// DComp has processed the batch making the switch to the new transform before releasing the old content/transform.
// See ProcessDeferredReleaseQueue(), which does this final releasing.
//
// Note that the same problem applies to the independent clip manipulation that keeps the clip underneath the
// sticky header in sync with the header.  The solution is identical, although some extra plumbing is required to
// apply the solution to clip content, which is managed by a different set of data structures.
//
// This approach does have a performance cost - for some period of time we are letting DManip drive two (or more) pieces of
// content simultaneously.
//
// Long term we can solve this problem much more efficiently (and cleanly) by moving to Expressions and Universal DManip,
// which avoids this whole problem naturally, because in that world we will only have 1 device, a new expression can be
// created along with layout changes and be guaranteed to be picked up together.
_Check_return_ HRESULT
CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate(
    _In_ CSecondaryContentRelationship *pSecondaryContentRelationship)
{
    xref_ptr<CUIElement> spDMContainer = pSecondaryContentRelationship->GetPrimaryContent();
    xref_ptr<CUIElement> spSecondaryContent = pSecondaryContentRelationship->GetSecondaryContent();
    bool targetsClip = !!pSecondaryContentRelationship->m_shouldTargetClip;

    // Avoid doing the shuffle if we don't have a shared transform yet then the bits aren't even on screen yet so no problem.
    if (spDMContainer != nullptr &&
        spSecondaryContent != nullptr &&
        spSecondaryContent->HasSharedManipulationTransform(targetsClip))
    {
        xref_ptr<CDependencyObject> spDManipElement;
        CUIElement *pManipulatableElementNoRef = nullptr;
        xref_ptr<CDMViewport> spViewport;
        xref_ptr<CDMContent> spContent;
        xref_ptr<IPALDirectManipulationService> spDMService;
        xref_ptr<IDCompositionDesktopDevicePartner> spDCompDevice;
        xref_ptr<IDCompositionDesktopDevicePartner3> spDCompDevice3;
        DMDeferredRelease deferredRelease;

        IFC_RETURN(FxCallbacks::UIElement_GetDManipElement(spDMContainer, spDManipElement.ReleaseAndGetAddressOf()));
        pManipulatableElementNoRef = do_pointer_cast<CUIElement>(spDManipElement);
        ASSERT(pManipulatableElementNoRef != nullptr);

        IFC_RETURN(GetViewport(spDMContainer, pManipulatableElementNoRef, spViewport.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetContentForContentElement(spSecondaryContent, targetsClip, spContent.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetDMService(spDMContainer, spDMService.ReleaseAndGetAddressOf()));
        ASSERT(spViewport != nullptr);
        ASSERT(spContent != nullptr);
        ASSERT(spDMService != nullptr);

        // Do some surgery on the book-keeping data structures we have for the current content and transform...
        // First transfer the shared transform to our deferral object.  The next RenderWalk will create a new transform.
        // And we'll defer releasing this shared transform until after the new one is live.
        spSecondaryContent->PrepareForSecondaryCurveUpdate(targetsClip);

        // Remove the DM Service's mapping to this piece of content, and transfer the DM Content to our deferral object.
        if (targetsClip)
        {
            IFC_RETURN(spDMService->RemoveSecondaryClipContent(spViewport, spContent, &deferredRelease));
        }
        else
        {
            IFC_RETURN(spDMService->RemoveSecondaryContent(spViewport, spContent, &deferredRelease));
        }

        // Remove the DM Service's mapping from content -> "parts", we'll create a new one on the next RenderWalk.
        IFC_RETURN(spDMService->RemoveSharedContentTransformMapping(spContent->GetCompositorSecondaryContentNoRef()));

        // Remove the CDMContent from the CDMViewport.  We'll add another one just after this function returns.
        if (targetsClip)
        {
            IFC_RETURN(spViewport->RemoveClipContent(spSecondaryContent, nullptr));
        }
        else
        {
            IFC_RETURN(spViewport->RemoveContent(spSecondaryContent, nullptr));
        }

        // Record the current DComp batch ID in our deferral object.  We'll track this batch ID as we continue
        // to tick the UI thread, and release the content/transform only after the new one is live.
        m_pCoreService->GetDCompDevice(spDCompDevice.ReleaseAndGetAddressOf());
        IFC_RETURN(spDCompDevice->QueryInterface(IID_PPV_ARGS(spDCompDevice3.ReleaseAndGetAddressOf())));
        ULONG batchId = 0;
        IFC_RETURN(spDCompDevice3->GetCurrentBatchID(&batchId));
        deferredRelease.SetBatchId(batchId);

        // Finally, add our deferral object to a collection of deferral objects to process on the next tick.
        // See ProcessDeferredReleaseQueue().
        m_vecDeferredRelease.push_back(deferredRelease);
     }

    return S_OK;
}

// Process the deferral objects being tracked for release enqueued above in PrepareSecondaryContentRelationshipForCurveUpdate().
// If pViewport is non-null, then filter to just those entries with matching viewport
_Check_return_ HRESULT
CInputServices::ProcessDeferredReleaseQueue(_In_opt_ IDirectManipulationViewport* pViewport)
{
    if (m_vecDeferredRelease.size() > 0)
    {
        ULONG lastConfirmedBatchId = 0;

        if (pViewport == nullptr)
        {
            // Ask the DComp device for the batch ID of the last batch it's picked up and processed.
            xref_ptr<IDCompositionDesktopDevicePartner> spDCompDevice;
            xref_ptr<IDCompositionDesktopDevicePartner3> spDCompDevice3;

            m_pCoreService->GetDCompDevice(spDCompDevice.ReleaseAndGetAddressOf());
            IFC_RETURN(spDCompDevice->QueryInterface(IID_PPV_ARGS(spDCompDevice3.ReleaseAndGetAddressOf())));
            IFC_RETURN(spDCompDevice3->GetLastConfirmedBatchId(&lastConfirmedBatchId));
        }

        std::vector<DMDeferredRelease>::iterator it = m_vecDeferredRelease.begin();
        while (it  != m_vecDeferredRelease.end())
        {
            // If we've confirmed this deferral's batch has been processed,
            // or this entry is for a viewport that's just about to be unregistered,
            // it's now safe to release the DM content/transform.
            if ((pViewport == nullptr && it->GetBatchId() <= lastConfirmedBatchId) ||
                it->GetDMViewport() == pViewport)
            {
                IFC_RETURN(it->DoDeferredRelease());
                it = m_vecDeferredRelease.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ApplySecondaryContentRelationship
//
//  Synopsis:
//    Creates secondary content based on a given secondary content relationship.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ApplySecondaryContentRelationship(
    _In_ CSecondaryContentRelationship *pSecondaryContentRelationship)
{
    HRESULT hr = S_OK;
    XUINT32 cDefinitions = 0;
    CParametricCurveDefinition *pDefinitions = nullptr;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ApplySecondaryContentRelationship - pSecondaryContentRelationship=0x%p.", this, pSecondaryContentRelationship));
    }
#endif // DM_DEBUG

    xref_ptr<CUIElement> pPrimaryContent;
    xref_ptr<CUIElement> pSecondaryContent;

    IFCPTR(pSecondaryContentRelationship);

    pPrimaryContent = pSecondaryContentRelationship->GetPrimaryContent();
    pSecondaryContent = pSecondaryContentRelationship->GetSecondaryContent();

    // We'll first check the queue to see if a relationship pointing to the same content elements exists there.
    // If one does, then this is a reapplication of a relationship on the same elements, and should override
    // the one(s) present in the queue.
    if (m_pSecondaryContentRelationshipsToBeApplied != nullptr)
    {
        auto pDependencyPropertyHolder = pSecondaryContentRelationship->GetDependencyPropertyHolder();

        for (XINT32 i = static_cast<XINT32>(m_pSecondaryContentRelationshipsToBeApplied->size()) - 1; i >= 0; i--)
        {
            CSecondaryContentRelationship *pQueuedSecondaryContentRelationship = nullptr;

            IFC(m_pSecondaryContentRelationshipsToBeApplied->get_item(i, pQueuedSecondaryContentRelationship));

            if (pPrimaryContent == pQueuedSecondaryContentRelationship->GetPrimaryContent() &&
                pSecondaryContent == pQueuedSecondaryContentRelationship->GetSecondaryContent() &&
                pDependencyPropertyHolder == pQueuedSecondaryContentRelationship->GetDependencyPropertyHolder())
            {
                IFC(m_pSecondaryContentRelationshipsToBeApplied->erase(i));

                // The input manager had a reference on the relationship in the queue,
                // so we need to release it as we remove it from the queue.
                ReleaseInterface(pQueuedSecondaryContentRelationship);
                break;
            }
        }
    }

    if (pPrimaryContent != nullptr && pSecondaryContent != nullptr)
    {
        CUIElement *pManipulatableElement = nullptr;
        xref_ptr<CDMViewport> pViewport;
        xref_ptr<CDependencyObject> pDManipElement;
        IFC(FxCallbacks::UIElement_GetDManipElement(pPrimaryContent, pDManipElement.ReleaseAndGetAddressOf()));

        pManipulatableElement = do_pointer_cast<CUIElement>(pDManipElement);

        if (pManipulatableElement != nullptr)
        {
            IFC(GetViewport(pPrimaryContent, pManipulatableElement, pViewport.ReleaseAndGetAddressOf()));
        }

        // If we don't have a manipulatable element or a viewport yet, then we'll want to put this relationship
        // in the list of relationships to be applied later.
        if (pManipulatableElement == nullptr || pViewport == nullptr)
        {
            if (m_pSecondaryContentRelationshipsToBeApplied == nullptr)
            {
                m_pSecondaryContentRelationshipsToBeApplied = new xvector<CSecondaryContentRelationship*>();
            }

            IFC(m_pSecondaryContentRelationshipsToBeApplied->push_back(pSecondaryContentRelationship));

            // The input manager now owns a reference to this secondary content relationship.
            AddRefInterface(pSecondaryContentRelationship);

            goto Cleanup;
        }

        IFC(pSecondaryContentRelationship->GetParametricCurveDefinitions(&cDefinitions, &pDefinitions));

        IFC(NotifySecondaryContentAdded(
            pPrimaryContent,
            pManipulatableElement,
            pSecondaryContent,
            pSecondaryContentRelationship->m_isDescendant ? XcpDMContentTypeDescendant : XcpDMContentTypeCustom,
            cDefinitions,
            pDefinitions,
            pSecondaryContentRelationship));
    }

Cleanup:
    for (XUINT32 i = 0; i < cDefinitions; i++)
    {
        delete[] pDefinitions[i].m_pSegments;
    }

    delete[] pDefinitions;
    pDefinitions = NULL;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ApplySecondaryContentRelationships
//
//  Synopsis:
//    Creates secondary content based on the secondary content relationships
//     that couldn't be applied immediately.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::ApplySecondaryContentRelationships()
{
    HRESULT hr = S_OK;
    xvector<CSecondaryContentRelationship*>* pSecondaryContentRelationshipsToBeApplied = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ApplySecondaryContentRelationships.", this));
    }
#endif // DM_DEBUG

    // To avoid an infinite loop in case some of these relationships still can't be applied,
    // we'll NULL out the InputManager field so it can be recreated if need be.
    // Otherwise, we'd be eternally removing and then re-adding elements
    // to the same vector.
    pSecondaryContentRelationshipsToBeApplied = m_pSecondaryContentRelationshipsToBeApplied;
    m_pSecondaryContentRelationshipsToBeApplied = NULL;

    if (pSecondaryContentRelationshipsToBeApplied)
    {
        while (pSecondaryContentRelationshipsToBeApplied->size() > 0)
        {
            CSecondaryContentRelationship* pSecondaryContentRelationship = NULL;

            IFC(pSecondaryContentRelationshipsToBeApplied->get_item(0, pSecondaryContentRelationship));
            IFC(ApplySecondaryContentRelationship(pSecondaryContentRelationship));
            IFC(pSecondaryContentRelationshipsToBeApplied->erase(0));

            // The input manager is done with the secondary content relationship object now,
            // so we'll release our reference to it now.
            ReleaseInterface(pSecondaryContentRelationship);
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        if (pSecondaryContentRelationshipsToBeApplied)
        {
            while (pSecondaryContentRelationshipsToBeApplied->size() > 0)
            {
                CSecondaryContentRelationship* pSecondaryContentRelationship = NULL;

                IGNOREHR(pSecondaryContentRelationshipsToBeApplied->get_item(0, pSecondaryContentRelationship));
                ASSERT(pSecondaryContentRelationship);
                IGNOREHR(pSecondaryContentRelationshipsToBeApplied->erase(0));

                ReleaseInterface(pSecondaryContentRelationship);
            }
        }
    }

    SAFE_DELETE(pSecondaryContentRelationshipsToBeApplied);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   NotifySecondaryContentRemoved
//
//  Synopsis:
//    Notification from a DM container that a secondary content was
//    removed. The associated CDMContent instance is deleted and the
//    DManip service unregisters the secondary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifySecondaryContentRemoved(
    _In_ CUIElement* pDMContainer,
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement)
{
    HRESULT hr = S_OK;
    CDMViewport* pViewport = NULL;
    CDMContent* pContent = NULL;
    CDMContent* pClipContent = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifySecondaryContentRemoved entry - pDMContainer=0x%p, pManipulatableElement=0x%p, pContentElement=0x%p.",
            this, pDMContainer, pManipulatableElement, pContentElement));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);
    IFCPTR(pContentElement);

    if (pManipulatableElement)
    {
        IFC(GetViewport(pDMContainer, pManipulatableElement, &pViewport));
    }
    else
    {
        SetInterface(pViewport, GetViewport(pDMContainer));
    }

    if (pViewport)
    {
        // DM container has an associated viewport
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        IFC(GetDMService(pDMContainer, &pDirectManipulationService));

        IFC(pViewport->GetContentNoRef(pContentElement, &pContent, NULL /*pContentIndex*/));

        IFC(pViewport->GetClipContentNoRef(pContentElement, &pClipContent, NULL /*pContentIndex*/));

        ASSERT(
            (pContent != NULL && pContent->GetContentElementNoRef() == pContentElement) ||
            (pClipContent != NULL && pClipContent->GetContentElementNoRef() == pContentElement));
    }

    if (pContent != NULL)
    {
        IFC(RemoveSecondaryContent(pContentElement, pContent, pViewport, pDirectManipulationService));
    }
    else if (pClipContent != NULL)
    {
        IFC(RemoveSecondaryClipContent(pContentElement, pClipContent, pViewport, pDirectManipulationService));
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RemoveSecondaryContent
//
//  Synopsis:
//    Deletes the CDMContent instance associated with the provided CUIElement,
//    and unregisters the secondary content on the DManip service side.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RemoveSecondaryContent(
    _In_ CUIElement* pContentElement,
    _In_opt_ CDMContent* pContent,
    _In_opt_ CDMViewport* pViewport,
    _In_opt_ IPALDirectManipulationService* pDirectManipulationService)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  RemoveSecondaryContent entry - pContentElement=0x%p, pContent=0x%p, pViewport=0x%p, pDirectManipulationService=0x%p.",
            this, pContentElement, pContent, pViewport, pDirectManipulationService));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pContentElement);

    ASSERT(!pContent || pContent->GetContentElementNoRef() == pContentElement);

    if (pViewport)
    {
        if (pDirectManipulationService != nullptr)
        {
            IFC_RETURN(pDirectManipulationService->ReleaseSharedContentTransform(pContent->GetCompositorSecondaryContentNoRef(), pContent->GetContentType()));
        }

        // DM container has an associated viewport
        IFC_RETURN(pViewport->RemoveContent(pContentElement, NULL /*ppContent*/));

        // Unregister the secondary content on the DManip service side
        if (pDirectManipulationService)
        {
            IFC_RETURN(pDirectManipulationService->RemoveSecondaryContent(pViewport, pContent));
        }

        if (pViewport->GetIsCompositorAware())
        {
            // Secondary content is removed during a manipulation, mark it as no longer requiring a composition node.
            pContentElement->UnsetRequiresComposition(
                CompositionRequirement::IndependentManipulation,
                IndependentAnimationType::None);
        }

        if (pContentElement->IsManipulatable())
        {
            // The removed secondary content is no longer manipulatable.
            pContentElement->UnsetRequiresComposition(
                CompositionRequirement::Manipulatable,
                IndependentAnimationType::None);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveSecondaryClipContent
//
//  Synopsis:
//    Deletes the CDMContent instance associated with the provided CUIElement,
//    and unregisters the secondary clip content on the DManip service side.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RemoveSecondaryClipContent(
    _In_ CUIElement* pClipContentElement,
    _In_opt_ CDMContent* pClipContent,
    _In_opt_ CDMViewport* pViewport,
    _In_opt_ IPALDirectManipulationService* pDirectManipulationService)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  RemoveSecondaryClipContent entry - pClipContentElement=0x%p, pClipContent=0x%p, pViewport=0x%p, pDirectManipulationService=0x%p.",
            this, pClipContentElement, pClipContent, pViewport, pDirectManipulationService));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pClipContentElement);

    ASSERT(!pClipContent || pClipContent->GetContentElementNoRef() == pClipContentElement);

    if (pViewport)
    {
        if (pDirectManipulationService != nullptr && pClipContent != nullptr)
        {
            IFC_RETURN(pDirectManipulationService->ReleaseSharedContentTransform(pClipContent->GetCompositorSecondaryContentNoRef(), pClipContent->GetContentType()));
        }

        // DM container has an associated viewport
        IFC_RETURN(pViewport->RemoveClipContent(pClipContentElement, NULL /*ppContent*/));

        // Unregister the secondary clip content on the DManip service side
        if (pDirectManipulationService)
        {
            IFC_RETURN(pDirectManipulationService->RemoveSecondaryClipContent(pViewport, pClipContent));
        }

        if (pViewport->GetIsCompositorAware())
        {
            // Secondary content is removed during a manipulation, mark it as no longer requiring a composition node.
            pClipContentElement->UnsetRequiresComposition(
                CompositionRequirement::IndependentClipManipulation,
                IndependentAnimationType::None);
        }

        if (pClipContentElement->IsManipulatable())
        {
            // The removed secondary content is no longer manipulatable.
            pClipContentElement->UnsetRequiresComposition(
                CompositionRequirement::Manipulatable,
                IndependentAnimationType::None);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveSecondaryContentRelationship
//
//  Synopsis:
//    Removes secondary content added for a given secondary content relationship.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::RemoveSecondaryContentRelationship(
    _In_ CSecondaryContentRelationship *pSecondaryContentRelationship)
{
    CUIElement *pPrimaryContent = NULL;
    CUIElement *pSecondaryContent = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  RemoveSecondaryContentRelationship entry - pSecondaryContentRelationship=0x%p.",
            this, pSecondaryContentRelationship));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pSecondaryContentRelationship);

    // We'll first check for this relationship in the queue, and remove it from there if it exists there.
    if (m_pSecondaryContentRelationshipsToBeApplied != NULL)
    {
        for (XINT32 i = static_cast<XINT32>(m_pSecondaryContentRelationshipsToBeApplied->size()) - 1; i >= 0; i--)
        {
            CSecondaryContentRelationship *pQueuedSecondaryContentRelationship = NULL;

            IFC_RETURN(m_pSecondaryContentRelationshipsToBeApplied->get_item(i, pQueuedSecondaryContentRelationship));

            if (pSecondaryContentRelationship == pQueuedSecondaryContentRelationship)
            {
                IFC_RETURN(m_pSecondaryContentRelationshipsToBeApplied->erase(i));

                // The input manager had a reference on the relationship in the queue,
                // so we need to release it as we remove it from the queue.
                ReleaseInterface(pQueuedSecondaryContentRelationship);
                break;
            }
        }
    }

    pPrimaryContent = do_pointer_cast<CUIElement>(pSecondaryContentRelationship->GetPrimaryContent());
    pSecondaryContent = do_pointer_cast<CUIElement>(pSecondaryContentRelationship->GetSecondaryContent());

    // If we don't have both components, then there's nothing for us to do -
    // without both components, this relationship couldn't have been added anyway.
    if (pPrimaryContent == NULL ||
        pSecondaryContent == NULL)
    {
        return S_OK;
    }

    IFC_RETURN(NotifySecondaryContentRemoved(
        pPrimaryContent,
        NULL,
        pSecondaryContent));

    return S_OK;
}

#pragma region DManipOnDComp_Staging
//------------------------------------------------------------------------
//
//  Method:   UpdateSecondaryContentRelationshipDependencyProperties
//
//  Synopsis:
//    Updates the associated dependency properties for secondary content relationships
//    assigned to the given viewport.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateSecondaryContentRelationshipDependencyProperties(
    _In_ CDMViewport *pViewport)
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  UpdateSecondaryContentRelationshipDependencyProperties entry - pViewport=0x%p.",
            this, pViewport));
    }
#endif // DM_DEBUG

    for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
    {
        IFC(pViewport->GetContent(iContent, &pContent));
        if (pContent)
        {
            if (pContent->GetSecondaryContentRelationship() != NULL)
            {
                IFC(pContent->GetSecondaryContentRelationship()->UpdateDependencyProperties(TRUE /* bManipulationCompleting */));
            }

            ReleaseInterface(pContent);
        }
    }

    for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
    {
        IFC(pViewport->GetClipContent(iClipContent, &pContent));
        if (pContent)
        {
            if (pContent->GetSecondaryContentRelationship() != NULL)
            {
                IFC(pContent->GetSecondaryContentRelationship()->UpdateDependencyProperties(TRUE /* bManipulationCompleting */));
            }

            ReleaseInterface(pContent);
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}
#pragma endregion DManipOnDComp_Staging

//------------------------------------------------------------------------
//
//  Method:   NotifyViewportChanged
//
//  Synopsis:
//    Called when one or more viewport characteristics have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyViewportChanged(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fBoundsChanged,
    _In_ bool fTouchConfigurationChanged,
    _In_ bool fNonTouchConfigurationChanged,
    _In_ bool fConfigurationsChanged,
    _In_ bool fChainedMotionTypesChanged,
    _In_ bool fHorizontalOverpanModeChanged,
    _In_ bool fVerticalOverpanModeChanged,
    _Out_ bool* pfConfigurationsUpdated)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifyViewportChanged entry. fInManipulation=%d, fBoundsChanged=%d, fTouchConfigurationChanged=%d, fNonTouchConfigurationChanged=%d, fConfigurationsChanged=%d, fChainedMotionTypesChanged=%d.",
            this, fInManipulation, fBoundsChanged, fTouchConfigurationChanged, fNonTouchConfigurationChanged, fConfigurationsChanged, fChainedMotionTypesChanged));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pDMContainer);
    IFCPTR_RETURN(pManipulatedElement);
    IFCPTR_RETURN(pfConfigurationsUpdated);
    *pfConfigurationsUpdated = FALSE;

    ASSERT(fBoundsChanged || fTouchConfigurationChanged || fNonTouchConfigurationChanged || fConfigurationsChanged || fChainedMotionTypesChanged || fHorizontalOverpanModeChanged || fVerticalOverpanModeChanged);

    if (!fInManipulation && fBoundsChanged)
    {
        // Any updates to the viewport bounds outside a manipulation must be done while the viewport is disabled to avoid a transitional Running status.
        IFC_RETURN(DisableViewport(pDMContainer, pManipulatedElement));
    }

    IFC_RETURN(UpdateManipulationViewport(
        pDMContainer,
        pManipulatedElement,
        fBoundsChanged /*fUpdateBounds*/,
        FALSE /*fUpdateInputTransform*/,
        fTouchConfigurationChanged /*fUpdateTouchConfiguration*/,
        fNonTouchConfigurationChanged /*fUpdateNonTouchConfiguration*/,
        fConfigurationsChanged /*fUpdateConfigurations*/,
        fChainedMotionTypesChanged /*fUpdateChainedMotionTypes*/,
        fTouchConfigurationChanged /*fActivateTouchConfiguration*/,
        fNonTouchConfigurationChanged /*fActivateNonTouchConfiguration*/,
        FALSE /*fActivateBringIntoViewConfiguration*/,
        fHorizontalOverpanModeChanged /*fUpdateHorizontalOverpanMode*/,
        fVerticalOverpanModeChanged /*fUpdateVerticalOverpanMode*/,
        pfConfigurationsUpdated));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationViewport
//
//  Synopsis:
//    Called when one or more viewport characteristics need to be pushed
//    to DM through the DM service.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationViewport(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fUpdateBounds,
    _In_ bool fUpdateInputTransform,
    _In_ bool fUpdateTouchConfiguration,
    _In_ bool fUpdateNonTouchConfiguration,
    _In_ bool fUpdateConfigurations,
    _In_ bool fUpdateChainedMotionTypes,
    _In_ bool fActivateTouchConfiguration,
    _In_ bool fActivateNonTouchConfiguration,
    _In_ bool fActivateBringIntoViewConfiguration,
    _In_ bool fUpdateHorizontalOverpanMode,
    _In_ bool fUpdateVerticalOverpanMode,
    _Out_opt_ bool* pfConfigurationsUpdated)
{
    HRESULT hr = S_OK;
    XRECTF bounds = { 0, 0, 0, 0 };
    XUINT8 cConfigurations = 0;
    XDMConfigurations touchConfiguration = XcpDMConfigurationNone;
    XDMConfigurations nonTouchConfiguration = XcpDMConfigurationNone;
    XDMConfigurations bringIntoViewportConfiguration = XcpDMConfigurationNone;
    XDMConfigurations* pConfigurations = NULL;
    XDMMotionTypes chainedMotionTypes = XcpDMMotionTypeNone;
    CMILMatrix inputTransform(TRUE);
    CDMViewport* pViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  UpdateManipulationViewport - pDMContainer=0x%p, pManipulatedElement=0x%p, fUpdateBounds=%d, fUpdateInputTransform=%d, fUpdateTouchConfiguration=%d, fUpdateNonTouchConfiguration=%d.",
            this, pDMContainer, pManipulatedElement, fUpdateBounds, fUpdateInputTransform, fUpdateTouchConfiguration, fUpdateNonTouchConfiguration));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"                   fUpdateConfigurations=%d, fUpdateChainedMotionTypes=%d, fActivateTouchConfiguration=%d, fActivateNonTouchConfiguration=%d, fActivateBringIntoViewConfiguration=%d.",
            fUpdateConfigurations, fUpdateChainedMotionTypes, fActivateTouchConfiguration, fActivateNonTouchConfiguration, fActivateBringIntoViewConfiguration));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);
    if (pfConfigurationsUpdated)
    {
        *pfConfigurationsUpdated = FALSE;
    }

    ASSERT(fUpdateBounds || fUpdateInputTransform || fUpdateTouchConfiguration || fUpdateConfigurations || fUpdateChainedMotionTypes || fActivateBringIntoViewConfiguration || fUpdateHorizontalOverpanMode || fUpdateVerticalOverpanMode);
    ASSERT(!(!fUpdateTouchConfiguration && fActivateTouchConfiguration));
    ASSERT(!(!fUpdateNonTouchConfiguration && fActivateNonTouchConfiguration));
    ASSERT(!(fActivateBringIntoViewConfiguration && fActivateTouchConfiguration));
    ASSERT(!(fActivateBringIntoViewConfiguration && fActivateNonTouchConfiguration));

    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    if (pDirectManipulationService)
    {
        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        IFC(pDirectManipulationContainer->GetManipulationViewport(
            pManipulatedElement,
            fUpdateBounds ? &bounds : NULL,
            fUpdateInputTransform ? &inputTransform : NULL,
            fUpdateTouchConfiguration ? &touchConfiguration : NULL,
            fUpdateNonTouchConfiguration ? &nonTouchConfiguration : NULL,
            (fUpdateConfigurations || fActivateBringIntoViewConfiguration) ? &bringIntoViewportConfiguration : NULL,
            fUpdateConfigurations ? &cConfigurations : NULL,
            fUpdateConfigurations ? &pConfigurations : NULL,
            fUpdateChainedMotionTypes ? &chainedMotionTypes : NULL,
            NULL /*pHorizontalOverpanMode*/,
            NULL /*pVerticalOverpanMode*/));

        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
        ASSERT(pViewport);
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        if (fUpdateBounds)
        {
             IFC(SetViewportBounds(pDirectManipulationService, pViewport, bounds));
        }

        // Skip the DManip input transform update when the matrix determinant is nil,
        // since those are invalid for DManip. This situation can only occur for non-animated
        // BringIntoViewport operations for which the input transform is irrelevant anyway.
        // All other DManip-driven operations are disabled (see ScrollViewer::GetManipulationConfigurations)
        if (fUpdateInputTransform && inputTransform.GetDeterminant() != 0)
        {
            if (XamlOneCoreTransforms::IsEnabled())
            {
                // In XamlOneCoreTransforms mode we talk to DManip in visual-relative coordinates, no need
                // to apply the zoom scale.
            }
            else
            {
                const auto scale = RootScale::GetRasterizationScaleForElement(pDMContainer);
                // Note that this uniform scaling does not affect the determinant test above.
                inputTransform.Scale(scale, scale);
            }
            IFC(pDirectManipulationService->SetViewportInputTransform(pViewport, &inputTransform));
        }

        if (fUpdateConfigurations)
        {
            IFC(UpdateManipulationConfigurations(
                pDirectManipulationService,
                pViewport,
                cConfigurations,
                pConfigurations,
                pfConfigurationsUpdated));

            if (pfConfigurationsUpdated && !*pfConfigurationsUpdated)
            {
                // Configurations could not be updated properly. Do not attempt to activate any configuration.
                fActivateTouchConfiguration = fActivateNonTouchConfiguration = fActivateBringIntoViewConfiguration = FALSE;
            }

            if (fUpdateTouchConfiguration &&
                fUpdateNonTouchConfiguration &&
                touchConfiguration == XcpDMConfigurationNone &&
                nonTouchConfiguration == XcpDMConfigurationNone &&
                bringIntoViewportConfiguration != XcpDMConfigurationNone)
            {
                // Since both touchConfiguration and nonTouchConfiguration are XcpDMConfigurationNone,
                // activate the bring-into-viewport configuration.
                fActivateBringIntoViewConfiguration = TRUE;
            }
        }

        if (touchConfiguration == XcpDMConfigurationNone)
        {
            fActivateTouchConfiguration = FALSE;
        }
        if (nonTouchConfiguration == XcpDMConfigurationNone)
        {
            fActivateNonTouchConfiguration = FALSE;
        }

        if (fActivateTouchConfiguration && fActivateNonTouchConfiguration)
        {
            // Only one configuration can be activated. Pick the one based
            // on their new values and which type is currently activated.
            if (pViewport->GetIsTouchConfigurationActivated())
            {
                fActivateNonTouchConfiguration = FALSE;
            }
            else
            {
                fActivateTouchConfiguration = FALSE;
            }
        }

        if (fUpdateTouchConfiguration && fActivateTouchConfiguration &&
            !fUpdateNonTouchConfiguration && !pViewport->GetIsTouchConfigurationActivated())
        {
            // Do not activate the touch configuration when it's the only one changing while another configuration is activated.
            // This scenario occurs when the manipulated element's extent changes causing the touch configuration to change.
            fActivateTouchConfiguration = FALSE;
        }

        ASSERT(!(fActivateTouchConfiguration && fActivateNonTouchConfiguration));

        if (fUpdateTouchConfiguration)
        {
            IFC(UpdateManipulationTouchConfiguration(
                fActivateTouchConfiguration ? pDirectManipulationService : NULL,
                pViewport,
                touchConfiguration));
        }

        if (fUpdateNonTouchConfiguration)
        {
            IFC(UpdateManipulationNonTouchConfiguration(
                fActivateNonTouchConfiguration ? pDirectManipulationService : NULL,
                pViewport,
                nonTouchConfiguration));
        }

        if (fActivateBringIntoViewConfiguration)
        {
            IFC(UpdateBringIntoViewportConfiguration(
                pDirectManipulationService,
                pViewport,
                bringIntoViewportConfiguration));
        }

        if (fUpdateChainedMotionTypes)
        {
            IFC(pDirectManipulationService->SetViewportChaining(pViewport, chainedMotionTypes));
        }

        if (fUpdateHorizontalOverpanMode || fUpdateVerticalOverpanMode)
        {
            IFC(UpdateManipulationOverpanModes(
                pDirectManipulationService,
                pViewport,
                FALSE /*fIsStartingNewManipulation*/));
        }
    }

Cleanup:
    delete[] pConfigurations;
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifyPrimaryContentChanged
//
//  Synopsis:
//    Called when one or more primary content characteristics have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyPrimaryContentChanged(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fLayoutRefreshed,
    _In_ bool fBoundsChanged,
    _In_ bool fHorizontalAlignmentChanged,
    _In_ bool fVerticalAlignmentChanged,
    _In_ bool fZoomFactorBoundaryChanged)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifyPrimaryContentChanged entry. fInManipulation=%d, fLayoutRefreshed=%d, fBoundsChanged=%d, fHorizontalAlignmentChanged=%d, fVerticalAlignmentChanged=%d, fZoomFactorBoundaryChanged=%d.",
            this, fInManipulation, fLayoutRefreshed, fBoundsChanged, fHorizontalAlignmentChanged, fVerticalAlignmentChanged, fZoomFactorBoundaryChanged));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pDMContainer);
    IFCPTR_RETURN(pManipulatedElement);

    ASSERT(fLayoutRefreshed || fBoundsChanged || fHorizontalAlignmentChanged || fVerticalAlignmentChanged || fZoomFactorBoundaryChanged);

    if (!fInManipulation && (fBoundsChanged || fHorizontalAlignmentChanged || fVerticalAlignmentChanged))
    {
        // Any updates to the content bounds or alignments outside a manipulation must be done while the viewport is disabled to avoid a transitional Running status.
        IFC_RETURN(DisableViewport(pDMContainer, pManipulatedElement));
    }

    IFC_RETURN(UpdateManipulationPrimaryContent(
        pDMContainer,
        pManipulatedElement,
        fLayoutRefreshed /*fUpdateLayoutRefreshed*/,
        fBoundsChanged   /*fUpdateBounds*/,
        fHorizontalAlignmentChanged /*fUpdateHorizontalAlignment*/,
        fVerticalAlignmentChanged   /*fUpdateVerticalAlignment*/,
        fZoomFactorBoundaryChanged  /*fUpdateZoomFactorBoundary*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationPrimaryContent
//
//  Synopsis:
//    Called when one or more primary content characteristics need to be
//    pushed to DM through the DM service
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationPrimaryContent(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fUpdateLayoutRefreshed,
    _In_ bool fUpdateBounds,
    _In_ bool fUpdateHorizontalAlignment,
    _In_ bool fUpdateVerticalAlignment,
    _In_ bool fUpdateZoomFactorBoundary,
    _Out_opt_ bool* pCancelOperation)
{
    HRESULT hr = S_OK;
    XRECTF bounds = { 0, 0, 0, 0 };
    XDMAlignment horizontalAligment = XcpDMAlignmentNear;
    XDMAlignment verticalAligment = XcpDMAlignmentNear;
    XFLOAT minZoomFactor = 1.0f;
    XFLOAT maxZoomFactor = 1.0f;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;
    bool fIsLayoutRefreshed = false;
    CDMViewport* pViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  UpdateManipulationPrimaryContent - pDMContainer=0x%p, pManipulatedElement=0x%p, fUpdateLayoutRefreshed=%d, fUpdateBounds=%d, fUpdateHorizontalAlignment=%d, fUpdateVerticalAlignment=%d, fUpdateZoomFactorBoundary=%d.",
            this, pDMContainer, pManipulatedElement, fUpdateLayoutRefreshed, fUpdateBounds, fUpdateHorizontalAlignment, fUpdateVerticalAlignment, fUpdateZoomFactorBoundary));
    }
#endif // DM_DEBUG

    if (pCancelOperation != nullptr)
    {
        *pCancelOperation = false;
    }

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    ASSERT(fUpdateLayoutRefreshed || fUpdateBounds || fUpdateHorizontalAlignment || fUpdateVerticalAlignment || fUpdateZoomFactorBoundary);

    if (fUpdateLayoutRefreshed)
    {
        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
        ASSERT(pViewport);
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());
        pViewport->SetIsPrimaryContentLayoutRefreshedAfterStart(TRUE);
        pViewport->SetIsPrimaryContentLayoutRefreshedAfterCompletion(TRUE);

        if (pViewport->GetState() == ManipulationCompleted)
        {
            // The primary content completed the post-manipulation layout. The potential secondary content relationships
            // can be updated by consuming the final content transform.
            IFC(UpdateSecondaryContentRelationshipDependencyProperties(pViewport));
        }
    }

    if (fUpdateBounds || fUpdateHorizontalAlignment || fUpdateVerticalAlignment || fUpdateZoomFactorBoundary)
    {
        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));

            // Check if the zoom factor is static or not.
            IFC(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
                pManipulatedElement,
                FALSE /*fInManipulation*/,
                FALSE /*fForInitialTransformationAdjustment*/,
                FALSE /*fForMargins*/,
                NULL /*pTranslationX*/,
                NULL /*pTranslationY*/,
                NULL /*pZoomFactor*/));

            IFC(pDirectManipulationContainer->GetManipulationPrimaryContent(
                pManipulatedElement,
                NULL /*pOffsets*/,
                fUpdateBounds ? &bounds : NULL,
                fUpdateHorizontalAlignment ? &horizontalAligment : NULL,
                fUpdateVerticalAlignment ? &verticalAligment : NULL,
                fUpdateZoomFactorBoundary ? &minZoomFactor : NULL,
                fUpdateZoomFactorBoundary ? &maxZoomFactor : NULL,
                NULL /*pfIsHorizontalStretchAlignmentTreatedAsNear*/,
                NULL /*pfIsVerticalStretchAlignmentTreatedAsNear*/,
                fUpdateBounds ? &fIsLayoutRefreshed : NULL));

            IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
            if (pViewport)
            {
                ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

                if (fUpdateHorizontalAlignment)
                {
                    IFC(pDirectManipulationService->SetContentAlignment(pViewport, horizontalAligment, TRUE /*fIsHorizontal*/));
                }

                if (fUpdateVerticalAlignment)
                {
                    IFC(pDirectManipulationService->SetContentAlignment(pViewport, verticalAligment, FALSE /*fIsHorizontal*/));
                }

                if (fUpdateZoomFactorBoundary)
                {
                    // When the zoom factor is static, [1.0, 1.0] is provided for the zoom boundaries.
                    IFC(pDirectManipulationService->SetPrimaryContentZoomBoundaries(pViewport, minZoomFactor, maxZoomFactor));
                }

                if (fUpdateBounds)
                {
                    pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);
                    bounds.X += contentOffsetX;
                    bounds.Y += contentOffsetY;
                    IFC(pDirectManipulationService->SetContentBounds(pViewport, bounds));

                    pViewport->SetIsPrimaryContentLayoutRefreshed(fIsLayoutRefreshed);
                }
            }
            else if (pCancelOperation != nullptr)
            {
                // *pCancelOperation is set to True because the provided DManip container became
                // non-manipulatable during the data retrieval. The pDMContainer + pManipulatedElement
                // combination is no longer associated with a valid CDMViewport instance.
                *pCancelOperation = true;
            }
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifyPrimaryContentTransformChanged
//
//  Synopsis:
//    Called when one or more primary content transform characteristics
//    have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyPrimaryContentTransformChanged(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fTranslationXChanged,
    _In_ bool fTranslationYChanged,
    _In_ bool fZoomFactorChanged)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  NotifyPrimaryContentTransformChanged - pDMContainer=0x%p, pManipulatedElement=0x%p, fInManipulation=%d, fTranslationXChanged=%d, fTranslationYChanged=%d, fZoomFactorChanged=%d.",
            this, pDMContainer, pManipulatedElement, fInManipulation, fTranslationXChanged, fTranslationYChanged, fZoomFactorChanged));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pDMContainer);
    IFCPTR_RETURN(pManipulatedElement);

    ASSERT(fTranslationXChanged || fTranslationYChanged || fZoomFactorChanged);

    IFC_RETURN(UpdateManipulationPrimaryContentTransform(
        pDMContainer,
        pManipulatedElement,
        fInManipulation,
        fTranslationXChanged /*fUpdateTranslationX*/,
        fTranslationYChanged /*fUpdateTranslationY*/,
        fZoomFactorChanged   /*fUpdateZoomFactor*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationPrimaryContentTransform
//
//  Synopsis:
//    Called when one or more primary content transform characteristics
//    need to be pushed to DM through the DM service
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationPrimaryContentTransform(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fUpdateTranslationX,
    _In_ bool fUpdateTranslationY,
    _In_ bool fUpdateZoomFactor)
{
    HRESULT hr = S_OK;
    bool fContentOffsetChanged = false;
    XFLOAT translationX = 0.0f;
    XFLOAT translationY = 0.0f;
    XFLOAT uncompressedZoomFactor = 1.0f;
    XFLOAT zoomFactorX = 1.0f;
    XFLOAT zoomFactorY = 1.0f;
    XFLOAT currentTranslationX = 0.0f;
    XFLOAT currentTranslationY = 0.0f;
    XFLOAT currentUncompressedZoomFactor = 1.0f;
    XFLOAT currentZoomFactorX = 1.0f;
    XFLOAT currentZoomFactorY = 1.0f;
    XFLOAT translationAdjustmentX = 0.0f;
    XFLOAT translationAdjustmentY = 0.0f;
    XFLOAT newTranslationAdjustmentX = 0.0f;
    XFLOAT newTranslationAdjustmentY = 0.0f;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;
    XRECTF contentBounds = { 0, 0, 0, 0 };
    XRECTF viewportBounds = { 0, 0, 0, 0 };
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CDMViewport* pViewport = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  UpdateManipulationPrimaryContentTransform - pDMContainer=0x%p, pManipulatedElement=0x%p, fInManipulation=%d, fUpdateTranslationX=%d, fUpdateTranslationY=%d, fUpdateZoomFactor=%d.",
            this, pDMContainer, pManipulatedElement, fInManipulation, fUpdateTranslationX, fUpdateTranslationY, fUpdateZoomFactor));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    ASSERT(fUpdateTranslationX || fUpdateTranslationY || fUpdateZoomFactor);

    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    if (pDirectManipulationService)
    {
        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        IFC(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
            pManipulatedElement,
            fInManipulation,
            FALSE /*fForInitialTransformationAdjustment*/,
            FALSE /*fForMargins*/,
            (fUpdateTranslationX || fUpdateTranslationY) ? &translationX : NULL, // Access both translations
            (fUpdateTranslationX || fUpdateTranslationY) ? &translationY : NULL, // if any of them has changed
            fUpdateZoomFactor ? &uncompressedZoomFactor : NULL));

        ASSERT(uncompressedZoomFactor > 0.0f);
        zoomFactorX = zoomFactorY = uncompressedZoomFactor;

        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));

#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"                   UpdateManipulationPrimaryContentTransform - GetManipulationPrimaryContentTransform for pViewport=0x%p returned translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf.",
                pViewport, translationX, translationY, uncompressedZoomFactor));
        }
#endif // DM_DEBUG

        ASSERT(!pViewport || pDMContainer == pViewport->GetDMContainerNoRef());

        if (fInManipulation)
        {
            ASSERT(pViewport);

            IFC(pViewport->GetCurrentStatus(currentStatus));
            IFC(pDirectManipulationService->GetViewportStatus(pViewport, status));

#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   UpdateManipulationPrimaryContentTransform - IM state=%d, IM status=%d, DM status=%d.",
                    pViewport->GetState(), currentStatus, status));
            }
#endif // DM_DEBUG

            if (IsViewportActive(status) || IsViewportActive(currentStatus))
            {
                if (fUpdateZoomFactor)
                {
                    // Zoom factor was programmatically altered during a DM manipulation
                    // The zoom factor is driven by DM. Stop the current manipulation and then apply the new factor.
                    IFC(CompleteDirectManipulation(pViewport, TRUE /*fDisableViewport*/));
                }
                else if (fUpdateTranslationX || fUpdateTranslationY)
                {
                    // IDirectManipulationContentBehavior::SyncContentTransform can not be called on an active viewport.
                    // IDirectManipulationContent::SetContentRect must be called instead.

                    // Determine the content's size
                    // Asking the manipulation handler is more precise than asking DManip which rounds the values to the closest integer.
                    IFC(pDirectManipulationContainer->GetManipulationPrimaryContent(
                        pManipulatedElement,
                        NULL /*pOffsets*/,
                        &contentBounds,
                        NULL /*pHorizontalAlignment*/,
                        NULL /*pVerticalAlignment*/,
                        NULL /*pMinZoomFactor*/,
                        NULL /*pMaxZoomFactor*/,
                        NULL /*pfIsHorizontalStretchAlignmentTreatedAsNear*/,
                        NULL /*pfIsVerticalStretchAlignmentTreatedAsNear*/,
                        NULL /*pfIsLayoutRefreshed*/));

                    // Determine the viewport's size
                    IFC(pDirectManipulationService->GetViewportBounds(pViewport, viewportBounds));

                    // Adjust the top/left location of the content
                    pViewport->GetCurrentTransformationValues(
                        currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY);

                    // Retrieve current offset adjustments...
                    pViewport->GetTranslationAdjustments(
                        translationAdjustmentX, translationAdjustmentY);

                    // ...and potentially new ones.
                    IFC(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
                        pManipulatedElement,
                        FALSE /*fInManipulation*/,
                        TRUE  /*fForInitialTransformationAdjustment*/,
                        FALSE /*fForMargins*/,
                        fUpdateTranslationX ? &newTranslationAdjustmentX : NULL,
                        fUpdateTranslationY ? &newTranslationAdjustmentY : NULL,
                        NULL  /*pZoomFactor*/));

#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Viewport is active with currentTranslationX=%4.6lf, currentTranslationY=%4.6lf, currentUncompressedZoomFactor=%4.8lf, currentZoomFactorX=%4.8lf, currentZoomFactorY=%4.8lf.",
                            currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY));
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   translationAdjustmentX=%4.6lf, translationAdjustmentY=%4.6lf.",
                            translationAdjustmentX, translationAdjustmentY));
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   contentBounds.X=%4.6lf, contentBounds.Y=%4.6lf, contentBounds.Width=%4.6lf, contentBounds.Height=%4.6lf.",
                            contentBounds.X, contentBounds.Y, contentBounds.Width, contentBounds.Height));
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   viewportBounds.X=%4.6lf, viewportBounds.Y=%4.6lf, viewportBounds.Width=%4.6lf, viewportBounds.Height=%4.6lf.",
                            viewportBounds.X, viewportBounds.Y, viewportBounds.Width, viewportBounds.Height));
                    }
#endif // DM_DEBUG

                    if (fUpdateTranslationX)
                    {
                        if (contentBounds.Width * currentUncompressedZoomFactor > viewportBounds.Width)
                        {
                            if (translationAdjustmentX < 0.0f)
                            {
                                // Content width became larger than viewport width
                                // Whether the width without the zoom factor is larger than the viewport (newTranslationAdjustmentX == 0)
                                // or not (newTranslationAdjustmentX < 0), the newTranslationAdjustmentX can be used.
                                translationAdjustmentX = newTranslationAdjustmentX;
                            }
                            else
                            {
                                // Content width already was larger than viewport width - update content's horizontal offset
                                contentBounds.X = (translationX - currentTranslationX) / currentUncompressedZoomFactor;
                            }
                        }
                        else
                        {
                            if (translationAdjustmentX >= 0.0f)
                            {
                                // Content width became smaller than viewport width - update horizontal adjustment offset
                                if (contentBounds.Width < viewportBounds.Width)
                                {
                                    translationAdjustmentX += newTranslationAdjustmentX;
                                }
                            }
                            else
                            {
                                // Content width already was smaller than viewport width - update horizontal adjustment offset
                                translationAdjustmentX = newTranslationAdjustmentX;
                            }
                            contentBounds.X = 0.0f;
                        }
                    }

                    if (fUpdateTranslationY)
                    {
                        if (contentBounds.Height * currentUncompressedZoomFactor > viewportBounds.Height)
                        {
                            if (translationAdjustmentY < 0.0f)
                            {
                                // Content height became larger than viewport height
                                // Whether the height without the zoom factor is larger than the viewport (newTranslationAdjustmentY == 0)
                                // or not (newTranslationAdjustmentY < 0), the newTranslationAdjustmentY can be used.
                                translationAdjustmentY = newTranslationAdjustmentY;
                            }
                            else
                            {
                                // Content height already was larger than viewport height - update content's vertical offset
                                contentBounds.Y = (translationY - currentTranslationY) / currentUncompressedZoomFactor;
                            }
                        }
                        else
                        {
                            if (translationAdjustmentY >= 0.0f)
                            {
                                // Content height became smaller than viewport height - update vertical adjustment offset
                                if (contentBounds.Height < viewportBounds.Height)
                                {
                                    translationAdjustmentY += newTranslationAdjustmentY;
                                }
                            }
                            else
                            {
                                // Content height already was smaller than viewport height - update vertical adjustment offset
                                translationAdjustmentY = newTranslationAdjustmentY;
                            }
                            contentBounds.Y = 0.0f;
                        }
                    }

                    if (!fUpdateTranslationX || !fUpdateTranslationY)
                    {
                        // Do not overwrite an offset adjustment previously set on a direction with no new update.
                        pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);
                        if (!fUpdateTranslationX)
                        {
                            // Reuse the previously set horizontal offset adjustment
                            contentBounds.X = contentOffsetX;
                        }
                        if (!fUpdateTranslationY)
                        {
                            // Reuse the previously set vertical offset adjustment
                            contentBounds.Y = contentOffsetY;
                        }
                    }

                    pViewport->SetContentOffsets(contentBounds.X, contentBounds.Y, &fContentOffsetChanged);
                    pViewport->SetTranslationAdjustments(translationAdjustmentX, translationAdjustmentY);

                    // Update DirectManipulation with the new location
                    IFC(pDirectManipulationService->SetContentBounds(pViewport, contentBounds));

                    if (fContentOffsetChanged)
                    {
                        IFC(UpdateSecondaryContentsOffsets(pDMContainer, pManipulatedElement, pViewport));
                    }
                }

                // Mark the manipulated element and the potential secondary elements dirty so that HWCompTreeNode::SetElementData gets called for them and new DMData structures get used on the compositor.
                IFC(DirtyDirectManipulationTransforms(pViewport));
            }
            else
            {
                // Let the DM container make a BringIntoViewport call instead to synchronize DManip and XAML.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Requesting BringIntoViewport call."));
                }
#endif // DM_DEBUG

                ASSERT(pDirectManipulationContainer);
                IFC(pDirectManipulationContainer->NotifyBringIntoViewportNeeded(pManipulatedElement));
            }
        }
        else
        {
            if (pViewport)
            {
                // Let the DM container make a BringIntoViewport call to synchronize DManip and XAML.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Requesting BringIntoViewport call."));
                }
#endif // DM_DEBUG

                ASSERT(pDirectManipulationContainer);
                IFC(pDirectManipulationContainer->NotifyBringIntoViewportNeeded(pManipulatedElement));

                if (fUpdateTranslationX && fUpdateTranslationY && fUpdateZoomFactor)
                {
                    pViewport->SetCompositorTransformationValues(translationX, translationY, uncompressedZoomFactor);
                    CUIElement::NWSetTransformDirty(pManipulatedElement, DirtyFlags::Render | DirtyFlags::Bounds);
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifySnapPointsChanged
//
//  Synopsis:
//    Called when the snap points for the provided motion type have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifySnapPointsChanged(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ XDMMotionTypes motionType)
{
    IFCPTR_RETURN(pDMContainer);
    IFCPTR_RETURN(pManipulatedElement);

    IFC_RETURN(UpdateManipulationSnapPoints(
        pDMContainer,
        pManipulatedElement,
        motionType));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationSnapPoints
//
//  Synopsis:
//    Called when the snap points for the provided motion type need to be
//    pushed to DM through the DM service.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationSnapPoints(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ XDMMotionTypes motionType)
{
    HRESULT hr = S_OK;
    CDMViewport* pViewport = NULL;
    bool fAreSnapPointsOptional = false;
    bool fAreSnapPointsSingle = false;
    bool fAreSnapPointsRegular = false;
    XFLOAT regularOffset = 0.0f;
    XFLOAT regularInterval = 0.0f;
    XUINT32 cIrregularSnapPoints = 0;
    XUINT32 iIrregularSnapPointKept = 0;
    XFLOAT* pIrregularSnapPoints = NULL;
    XDMSnapCoordinate snapCoordinate = (motionType == XcpDMMotionTypeZoom) ? XcpDMSnapCoordinateOrigin : XcpDMSnapCoordinateBoundary;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  UpdateManipulationSnapPoints entry. pDMContainer=0x%p, pManipulatedElement=0x%p, motionType=%d.",
            this, pDMContainer, pManipulatedElement, motionType));
    }
#endif // DM_DEBUG

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    if (pDirectManipulationService)
    {
        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        IFC(pDirectManipulationContainer->GetManipulationSnapPoints(
            pManipulatedElement,
            motionType,
            &fAreSnapPointsOptional,
            &fAreSnapPointsSingle,
            &fAreSnapPointsRegular,
            &regularOffset,
            &regularInterval,
            &cIrregularSnapPoints,
            &pIrregularSnapPoints,
            &snapCoordinate));

        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
        ASSERT(pViewport);
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        if (fAreSnapPointsRegular)
        {
            // First reset the irregular snap points in case they were previously set.
            IFC(pDirectManipulationService->SetPrimaryContentSnapPoints(
                pViewport,
                motionType,
                0 /*cIrregularSnapPoints*/,
                static_cast<XFLOAT*>(NULL) /*pIrregularSnapPoints*/));

            IFC(pDirectManipulationService->SetPrimaryContentSnapPoints(
                pViewport,
                motionType,
                regularOffset,
                regularInterval));
        }
        else
        {
            // First reset the regular snap points' offset and interval
            // in case they were previously set.
            IFC(pDirectManipulationService->SetPrimaryContentSnapPoints(
                pViewport,
                motionType,
                0.0f /*regularOffset*/,
                0.0f /*regularInterval*/));

            if (cIrregularSnapPoints > 1)
            {
                ASSERT(pIrregularSnapPoints);
                // First sort the irregular snap points in increasing order
                ArrayInsertionSort(pIrregularSnapPoints, cIrregularSnapPoints);

                // Finally get rid of potential duplicates
                for (XUINT32 iIrregularSnapPoint = 1; iIrregularSnapPoint < cIrregularSnapPoints; iIrregularSnapPoint++)
                {
                    if (pIrregularSnapPoints[iIrregularSnapPointKept] != pIrregularSnapPoints[iIrregularSnapPoint])
                    {
                        iIrregularSnapPointKept++;
                        pIrregularSnapPoints[iIrregularSnapPointKept] = pIrregularSnapPoints[iIrregularSnapPoint];
                    }
                }
                cIrregularSnapPoints = iIrregularSnapPointKept + 1;
            }

            IFC(pDirectManipulationService->SetPrimaryContentSnapPoints(
                pViewport,
                motionType,
                cIrregularSnapPoints,
                pIrregularSnapPoints));
        }

        IFC(pDirectManipulationService->SetPrimaryContentSnapPointsType(
            pViewport,
            motionType,
            fAreSnapPointsOptional,
            fAreSnapPointsSingle));

        // We only ever use XcpDMSnapCoordinateOrigin with origin == 0.0f for the zoom motion,
        // or the origin value is ignored when we use XcpDMSnapCoordinateBoundary.
        // Therefore origin can just be hard-coded to 0.0f and we save a parameter to have to pass around.
        IFC(pDirectManipulationService->SetPrimaryContentSnapPointsCoordinate(
            pViewport,
            motionType,
            snapCoordinate,
            0.0f));
    }

Cleanup:
    delete[] pIrregularSnapPoints;
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   NotifyManipulatedElementAlignmentChanged
//
//  Synopsis:
//    Called when an alignment for the primary content has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::NotifyManipulatedElementAlignmentChanged(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fIsForHorizontalAlignment,
    _In_ bool fIsForStretchAlignment,
    _In_ bool fIsStretchAlignmentTreatedAsNear)
{
    HRESULT hr = S_OK;
    CDMViewport* pViewport = NULL;
    CUIElement* pDMContainer = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;

    IFCPTR(pManipulatedElement);

    IFC(GetViewport(NULL /*pDMContainer*/, pManipulatedElement, &pViewport));
    if (pViewport)
    {
        // The presence of an associated viewport implies that this element can be manipulated by DM.
        pDMContainer = pViewport->GetDMContainerNoRef();
        ASSERT(pDMContainer);

        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        ASSERT(pDirectManipulationContainer);

        IFC(pDirectManipulationContainer->NotifyContentAlignmentAffectingPropertyChanged(pManipulatedElement, fIsForHorizontalAlignment, fIsForStretchAlignment, fIsStretchAlignmentTreatedAsNear));
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    RRETURN(hr);
}

// Retrieves the current Stretch-to-Top/Left alignment overriding status from
// the ScrollViewer control owning the provided manipulated element.
_Check_return_ HRESULT
CInputServices::IsStretchAlignmentTreatedAsNear(
    _In_ CUIElement* pManipulatedElement,
    _In_ bool isForHorizontalAlignment,
    _Out_ bool* pIsStretchAlignmentTreatedAsNear)
{
    bool fIsStretchAlignmentTreatedAsNear = false;
    xref_ptr<CDMViewport> spViewport;
    xref_ptr<CUIDMContainer> spDirectManipulationContainer;

    ASSERT(pManipulatedElement);
    *pIsStretchAlignmentTreatedAsNear = false;

    IFC_RETURN(GetViewport(nullptr /*pDMContainer*/, pManipulatedElement, spViewport.ReleaseAndGetAddressOf()));
    if (spViewport != nullptr)
    {
        ASSERT(spViewport->GetDMContainerNoRef());
        IFC_RETURN(spViewport->GetDMContainerNoRef()->GetDirectManipulationContainer(spDirectManipulationContainer.ReleaseAndGetAddressOf()));
        ASSERT(spDirectManipulationContainer != nullptr);
        IFC_RETURN(spDirectManipulationContainer->GetManipulationPrimaryContent(
            pManipulatedElement,
            nullptr /*pOffsets*/,
            nullptr /*pContentBounds*/,
            nullptr /*pHorizontalAligment*/,
            nullptr /*pVerticalAligment*/,
            nullptr /*pMinZoomFactor*/,
            nullptr /*pMaxZoomFactor*/,
            isForHorizontalAlignment ? &fIsStretchAlignmentTreatedAsNear : nullptr,
            isForHorizontalAlignment ? nullptr : &fIsStretchAlignmentTreatedAsNear,
            nullptr /*pfIsLayoutRefreshed*/));
        if (fIsStretchAlignmentTreatedAsNear)
        {
            *pIsStretchAlignmentTreatedAsNear = true;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetPrimaryContentTransform
//
//  Synopsis:
//    Called when the DM container needs access to the latest primary content
//    output transform during a manipulation.
//    When fForBringIntoViewport is True, the DManip current transform is used,
//    otherwise the CDMViewport's current transformation values are used.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetPrimaryContentTransform(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fForBringIntoViewport,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    HRESULT hr = S_OK;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;
    CDMViewport* pViewport = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    translationX = 0.0f;
    translationY = 0.0f;
    uncompressedZoomFactor = 1.0f;
    zoomFactorX = 1.0f;
    zoomFactorY = 1.0f;

    IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
    ASSERT(pViewport);
    ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

    if (fForBringIntoViewport)
    {
        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDirectManipulationService->GetPrimaryContentTransform(
                pViewport,
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));
        }
    }
    else
    {
        // Access latest content output transform reported by DM, which is influenced by previous manipulations.
        pViewport->GetCurrentTransformationValues(
            translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY);
    }

    // Access the content's offset provided to DM's SetContentRect API.
    pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);
    translationX += contentOffsetX * zoomFactorX;
    translationY += contentOffsetY * zoomFactorY;

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   BringIntoViewport
//
//  Synopsis:
//    Called when the DM container wants to:
//    - bring the specified bounds of the manipulated element into the viewport,
//    when fAnimate is True. DManip's ZoomToRect API is used.
//    - move to the specified transform when fAnimate is False. DManip's
//    SetContentTransformValues API is used when fTransformIsValid is True.
//
//    The trio translateX, translateY & zoomFactor is valid when fTransformIsValid is True.
//    Only the bounds parameter can be used otherwise. When fAnimate is False,
//    the trio must be provided.
//
//    When fSkipAnimationWhileRunning and fAnimate are True, the
//    SetContentTransformValues API is used as long as zoomFactor > 0.
//
//    When fIsForMakeVisible is True, the bring-into-viewport request is
//    triggered by a MakeVisible call.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::BringIntoViewport(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pManipulatedElement,
    _In_ XRECTF& bounds,
    _In_ XFLOAT translateX,
    _In_ XFLOAT translateY,
    _In_ XFLOAT zoomFactor,
    _In_ bool fTransformIsValid,
    _In_ bool fSkipDuringTouchContact,
    _In_ bool fSkipAnimationWhileRunning,
    _In_ bool fAnimate,
    _In_ bool fApplyAsManip,
    _In_ bool fIsForMakeVisible,
    _Out_ bool* pfHandled)
{
    HRESULT hr = S_OK;
    DirectManipulationState state = ManipulationNone;
    CDMViewport* pViewport = NULL;
    XRECTF contentBounds = { 0, 0, 0, 0 };
    XRECTF viewportActualBounds = { 0, 0, 0, 0 };
    XRECTF viewportAdjustedBounds = { 0, 0, 0, 0 };
    XUINT32 cContactIds = 0;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;
    XFLOAT translationX = 0.0f;
    XFLOAT translationY = 0.0f;
    XFLOAT uncompressedZoomFactor = 1.0f;
    XFLOAT zoomFactorX = 1.0f;
    XFLOAT zoomFactorY = 1.0f;
    XFLOAT targetZoomFactor = 1.0f;
    XFLOAT newTranslationX = 0.0f;
    XFLOAT newTranslationY = 0.0f;
    XFLOAT newUncompressedZoomFactor = 1.0f;
    XFLOAT newZoomFactorX = 1.0f;
    XFLOAT newZoomFactorY = 1.0f;
    bool fHandled = false;
    bool fIsActiveStatus = false;
    bool fIsActiveState = false;
    bool fIsActiveStateOrStatus = false;
    bool fUpdateBounds = false;
    bool fCanManipulateElementsByTouch = false;
    bool fCanManipulateElementsNonTouch = false;
    bool fCanManipulateElementsWithBringIntoViewport = false;
    bool fWasTouchConfigurationActivated = false;
    bool fWasNonTouchConfigurationActivated = false;
    bool fWasPrimaryContentLayoutRefreshedAfterCompletion = false;
    bool fContentOffsetChanged = false;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMViewportStatus newStatus = XcpDMViewportBuilding;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  BringIntoViewport entry. pDMContainer=0x%p, pManipulatedElement=0x%p, bounds.X=%4.6lf, bounds.Y=%4.6lf, bounds.Width=%4.6lf, bounds.Height=%4.6lf.",
            this, pDMContainer, pManipulatedElement, bounds.X, bounds.Y, bounds.Width, bounds.Height));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"                   translateX=%4.6lf, translateY=%4.6lf, zoomFactor=%4.8lf, fTransformIsValid=%d.",
            translateX, translateY, zoomFactor, fTransformIsValid));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"                   fSkipDuringTouchContact=%d, fSkipAnimationWhileRunning=%d, fAnimate=%d, fApplyAsManip=%d.",
            fSkipDuringTouchContact, fSkipAnimationWhileRunning, fAnimate, fApplyAsManip));
    }
#endif // DM_DEBUG

    ASSERT(!(fTransformIsValid && zoomFactor == 0.0f));
    ASSERT(!(fAnimate && !fApplyAsManip));
    ASSERT(pfHandled);
    *pfHandled = FALSE;

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    if (pDirectManipulationService)
    {
        IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
        ASSERT(pDirectManipulationContainer);

        IFC(GetViewport(pDMContainer, pManipulatedElement, &pViewport));
        ASSERT(pViewport);
        ASSERT(pDMContainer == pViewport->GetDMContainerNoRef());

        state = pViewport->GetState();

        fIsActiveState = state == ManipulationStarting || state == ManipulationStarted || state == ManipulationDelta || state == ManipulationLastDelta;

        if (!fIsActiveState)
        {
            IFC(pDirectManipulationContainer->GetCanManipulateElements(&fCanManipulateElementsByTouch, &fCanManipulateElementsNonTouch, &fCanManipulateElementsWithBringIntoViewport));
            if (!fCanManipulateElementsWithBringIntoViewport)
            {
                goto Cleanup;
            }
        }

        IFC(pViewport->GetCurrentStatus(currentStatus));
        IFC(pDirectManipulationService->GetViewportStatus(pViewport, status));

#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"                   pViewport=0x%p, IM state=%d, DM status=%d.",
                pViewport, state, status));
        }
#endif // DM_DEBUG

        if (status == XcpDMViewportRunning)
        {
            if (fAnimate && fSkipAnimationWhileRunning)
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Turning off animation because of Running status."));
                }
#endif // DM_DEBUG

                fAnimate = FALSE;
            }
        }
        else if (fSkipDuringTouchContact)
        {
            IFC(pViewport->GetContactIdCount(&cContactIds));
            if (cContactIds > 0)
            {
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"                   Skipping operation because of positive contact count."));
                }
#endif // DM_DEBUG

                goto Cleanup;
            }
        }

        // When pManipulatedElement->GetCompositionPeer() is null, we do not set fAnimate to FALSE because the composition peer
        // may only be temporarily null. An imminent call to CCoreServices::NWDrawTree may restore a composition peer and let the
        // animated view change proceed normally. If that is not the case, then StopInertialViewport called in OnPostUIThreadTick
        // will cancel inertia and jump to the end-of-inertia transform.

        fIsActiveStatus = IsViewportActive(status) || IsViewportActive(currentStatus);
        fIsActiveStateOrStatus = fIsActiveState || IsViewportActive(status);

        if (fIsActiveStateOrStatus)
        {
            fWasTouchConfigurationActivated = pViewport->GetIsTouchConfigurationActivated();
            fWasNonTouchConfigurationActivated = pViewport->GetIsNonTouchConfigurationActivated();
        }

        // IsPrimaryContentLayoutRefreshedAfterCompletion needs to be evaluated here, before a potential call to
        // CUIDMContainer.GetManipulationPrimaryContent which may cause a re-layout of the manipulated element.
        // That would require an update of the translation adjustments for the compositor's consumption.
        fWasPrimaryContentLayoutRefreshedAfterCompletion = false;

        bool cancelOperation = false;

        if (!fIsActiveStateOrStatus &&
            pViewport->GetUITicksForNotifications() == 0)
        {
            IFC(SetupDirectManipulationViewport(
                pDMContainer,
                pManipulatedElement,
                pViewport,
                pDirectManipulationContainer,
                pDirectManipulationService,
                FALSE    /*fIsForTouch*/,
                TRUE     /*fIsForBringIntoViewport*/,
                fAnimate /*fIsForAnimatedBringIntoViewport*/,
                TRUE     /*fIsForFullSetup*/,
                &cancelOperation));
        }
        else
        {
            if (!fIsActiveStateOrStatus)
            {
                // A partial setup is required while pViewport->GetUITicksForNotifications() > 0.
                IFC(SetupDirectManipulationViewport(
                    pDMContainer,
                    pManipulatedElement,
                    pViewport,
                    pDirectManipulationContainer,
                    pDirectManipulationService,
                    FALSE    /*fIsForTouch*/,
                    TRUE     /*fIsForBringIntoViewport*/,
                    fAnimate /*fIsForAnimatedBringIntoViewport*/,
                    FALSE    /*fIsForFullSetup*/,
                    &cancelOperation));
            }
            if (!cancelOperation && (pViewport->GetIsTouchConfigurationActivated() || pViewport->GetIsNonTouchConfigurationActivated()))
            {
                IFC(UpdateManipulationViewport(
                    pDMContainer,
                    pManipulatedElement,
                    FALSE /*fUpdateBounds*/,
                    FALSE /*fUpdateInputTransform*/,
                    FALSE /*fUpdateTouchConfiguration*/,
                    FALSE /*fUpdateNonTouchConfiguration*/,
                    FALSE /*fUpdateConfigurations*/,
                    FALSE /*fUpdateChainedMotionTypes*/,
                    FALSE /*fActivateTouchConfiguration*/,
                    FALSE /*fActivateNonTouchConfiguration*/,
                    TRUE  /*fActivateBringIntoViewConfiguration*/,
                    FALSE /*fUpdateHorizontalOverpanMode*/,
                    FALSE /*fUpdateVerticalOverpanMode*/,
                    NULL  /*pfConfigurationsUpdated*/));
            }
        }

        if (cancelOperation)
        {
            // Aborting the operation. The occurs when the pDMContainer + pManipulatedElement combination
            // is no longer associated with a valid CDMViewport instance after the SetupDirectManipulationViewport
            // call above. Return immediately with *pfHandled set to False.
            goto Cleanup;
        }

        // Retrieve actual viewport size.
        IFC(pDirectManipulationContainer->GetManipulationViewport(
            pManipulatedElement,
            &viewportActualBounds,
            NULL /*pInputTransform*/,
            NULL /*pTouchConfiguration*/,
            NULL /*pNonTouchConfiguration*/,
            NULL /*pBringIntoViewportConfiguration*/,
            NULL /*pcConfigurations*/,
            NULL /*ppConfigurations*/,
            NULL /*pChainedMotionTypes*/,
            NULL /*pHorizontalOverpanMode*/,
            NULL /*pVerticalOverpanMode*/));

        if (!fAnimate && fTransformIsValid)
        {
            targetZoomFactor = zoomFactor;
        }
        else
        {
            // Retrieve adjusted DManip viewport size.
            IFC(pDirectManipulationService->GetViewportBounds(pViewport, viewportAdjustedBounds));

            // Because DManip consumes viewport sizes as integers, the target window size needs to be adjusted
            // based on that DManip viewport size. Otherwise the final zoom factor is not the requested one.
            ASSERT(viewportAdjustedBounds.Width != 0.0f);
            ASSERT(viewportAdjustedBounds.Height != 0.0f);
            ASSERT(viewportActualBounds.Width != 0.0f);
            ASSERT(viewportActualBounds.Height != 0.0f);
            ASSERT(XcpAbsF(viewportAdjustedBounds.Width - viewportActualBounds.Width) < 1.0f);
            ASSERT(XcpAbsF(viewportAdjustedBounds.Height - viewportActualBounds.Height) < 1.0f);
            bounds.Width *= viewportAdjustedBounds.Width / viewportActualBounds.Width;
            bounds.Height *= viewportAdjustedBounds.Height / viewportActualBounds.Height;

            ASSERT(bounds.Width != 0.0f);
            targetZoomFactor = viewportAdjustedBounds.Width / bounds.Width;
        }

        IFC(pDirectManipulationService->GetPrimaryContentTransform(
            pViewport,
            translationX,
            translationY,
            uncompressedZoomFactor,
            zoomFactorX,
            zoomFactorY));

        fUpdateBounds = fIsActiveStatus && status == XcpDMViewportAutoRunning && targetZoomFactor == uncompressedZoomFactor;

        if (fUpdateBounds)
        {
            // Only synchronous offset changes are requested, or offset changes during an edge scroll.
            // The zoom factor remains the same.
            // Simply invoke DManip's SetContentRect method instead of ZoomToRect to avoid
            // terminating the current manipulation.
            // Evaluate the correction needed in the content's offsets.
            IFC(pDirectManipulationContainer->GetManipulationPrimaryContent(
                pManipulatedElement,
                NULL /*pOffsets*/,
                &contentBounds,
                NULL /*pHorizontalAligment*/,
                NULL /*pVerticalAligment*/,
                NULL /*pMinZoomFactor*/,
                NULL /*pMaxZoomFactor*/,
                NULL /*pfIsHorizontalStretchAlignmentTreatedAsNear*/,
                NULL /*pfIsVerticalStretchAlignmentTreatedAsNear*/,
                NULL /*pfIsLayoutRefreshed*/));

            contentBounds.X = -translationX / uncompressedZoomFactor - bounds.X;
            contentBounds.Y = -translationY / uncompressedZoomFactor - bounds.Y;
            pViewport->SetContentOffsets(contentBounds.X, contentBounds.Y, &fContentOffsetChanged);
            IFC(pDirectManipulationService->SetContentBounds(pViewport, contentBounds));

            if (fContentOffsetChanged)
            {
                IFC(UpdateSecondaryContentsOffsets(pDMContainer, pManipulatedElement, pViewport));
            }

            fHandled = TRUE;
        }
        else
        {
            if (!fAnimate)
            {
                // With DManip-on-DComp we must guard against a synchronization problem when performing
                // the ZoomToRect().  DManip would normally send the updated transform to DComp immediately
                // which may be out of sync with operations XAML is trying to perform on the UI thread, resulting
                // in a noticeable glitch.
                // There are several scenarios where this can happen:
                // 1) With virtualizing panels, they may change their estimate of how much content there is, most notably
                //    as the user is grabbing the thumb and scrolling it with the mouse, but could also happen via ChangeView.
                //    This results in the content being laid out to a different position, and an adjustment transform is
                //    made on the UI thread to counteract the jump.  This change must be essentially done last otherwise
                //    the content will appear to jump to the wrong position, then jump back when the UI thread finally
                //    ticks and adds the adjustment transform/redraws as the new location.
                // 2) If a layout change is accompanied by changes to the overall DManip configuration (eg Viewport rect,
                //    content rect, alignment, etc), this can cause DManip to change the output transform before we've even
                //    called ZoomToRect (will happen below as we call EnableViewport), which will make the content jump
                //    as we haven't completed the UI thread frame that renders the content at its new location yet.
                // 3) V/SIS customers may complete the drawing to their surface and also call ChangeView together.
                //    In this case the new content should appear on the same frame as the view change otherwise the old
                //    content will move before the new content is presented.
                // The problem is fixed by forcing a change in the transform such that the XAML UI thread commits the final change.
                // Unfortunately this is a bit tricky.  Here's the order of operations:
                // 1) First we detach the shared DManip transform from the DManip content, but leave this transform set on the visual.
                //    We also clear out the data we have about this transform on the corresponding CompNode.
                // 2) Next we dirty the UIElement so that on the next UI thread frame, it will force the creation of a new DMAnip transform.
                // 3) Next we call ZoomToRect.  This causes DMAnip to update its notion of the transform but this transform is not propagated
                //    to DComp yet since we've detached the DComp transform from DManip
                // 4) On the next UI thread frame, we'll create a new shared transform, update it with the latest output transform value,
                //    attach it to the visual, and commit this update from the XAML UI thread.
                //    Since it's a brand new transform it won't take effect until the UI thread batch is processed.
                //
                // Note that we don't do this synchronization when we're animating the ChangeView as in the animated case, we expect the transform
                // to smoothly transition to its new value and thus the potential for glitching is not possible in the first place.
                IFC(PrepareCompositionNodesForBringIntoView(pViewport));
            }

            // Note that the above call to PrepareCompositionNodesForBringIntoView MUST happen before
            // EnableViewport is called as EnableViewport may cause DManip to update the output transform immediately,
            // which will also cause the content to jump.
            IFC(EnableViewport(pDirectManipulationService, pViewport));

            if (!fAnimate)
            {
                // Since enabling the viewport might change DManip's transform,
                // it needs to be re-fetched in order to accurately determine
                // if ZoomToRect has an effect.
                IFC(pDirectManipulationService->GetPrimaryContentTransform(
                    pViewport,
                    translationX,
                    translationY,
                    uncompressedZoomFactor,
                    zoomFactorX,
                    zoomFactorY));
            }

            // Access the content's offset provided to DM's SetContentRect API.
            pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);

            if (!fAnimate && fTransformIsValid)
            {
                // When the targeted transform is provided with the translateX, translateY & zoomFactor parameters and animation is turned off, use the DManip SetContentTransformValues
                // API instead of ZoomToRect to avoid rounding errors when the translation offsets are large.
                IFC(pDirectManipulationService->SetPrimaryContentTransform(pViewport, translateX - contentOffsetX * zoomFactor, translateY - contentOffsetY * zoomFactor, zoomFactor));
            }
            else
            {
                bounds.X += contentOffsetX;
                bounds.Y += contentOffsetY;
                IFC(pDirectManipulationService->BringIntoViewport(pViewport, bounds, fAnimate));
            }

            IFC(pDirectManipulationService->GetViewportStatus(pViewport, newStatus));
            if (fAnimate)
            {
                if (newStatus == XcpDMViewportInertia)
                {
                    fHandled = TRUE;
                    // Set our target offsets into the viewort so that we can pull them out to
                    // know where the element is going to end up.
                    pViewport->SetTargetTranslation(-bounds.X, -bounds.Y);

                    if (fIsForMakeVisible)
                    {
                        // Set the m_fIsProcessingMakeVisibleInertia flag since a animation triggered by a MakeVisible started.
                        pViewport->SetIsProcessingMakeVisibleInertia(TRUE);
                    }
                }
                // Else DManip did not take any action because the target transform is too close to the current one.
            }
            else
            {
                IFC(pDirectManipulationService->GetPrimaryContentTransform(
                    pViewport,
                    newTranslationX,
                    newTranslationY,
                    newUncompressedZoomFactor,
                    newZoomFactorX,
                    newZoomFactorY));
                if (newTranslationX != translationX ||
                    newTranslationY != translationY ||
                    newUncompressedZoomFactor != uncompressedZoomFactor)
                {
                    fHandled = TRUE;
                }
                // Else DManip did not take any action because the target transform is too close to the current one.
            }

            if (fHandled && fApplyAsManip)
            {
                // Do not request temporary notifications from the DM container when the snap points were not updated because no animation was requested.
                if (fAnimate)
                {
                    IFC(SetDirectManipulationHandlerWantsNotifications(
                        pViewport,
                        pManipulatedElement,
                        pDirectManipulationContainer,
                        TRUE /*fWantsNotifications*/));
                }

                // Let the compositor know about the new manipulation, if it's unaware of this viewport.
                // A call to CUIDMContainer.GetManipulationPrimaryContent causes a re-layout of the DM container which sets IsPrimaryContentLayoutRefreshedAfterCompletion to True.
                IFC(DeclareNewViewportForCompositor(
                    pViewport,
                    pDirectManipulationContainer
                    ));

                // Enter the ManipulationStarting state if no manipulation is already in process.
                switch (pViewport->GetState())
                {
                case ManipulationNone:
                case ManipulationCompleted:
                case ConstantVelocityScrollStopped:
                {
#ifdef DM_DEBUG
                    if (m_fIsDMInfoTracingEnabled)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                            L"                   Calling NotifyManipulationProgress(ManipulationStarting) for pViewport=0x%p.", pViewport));
                    }
#endif // DM_DEBUG

                    pViewport->SetState(ManipulationStarting);
                    IFC(pDirectManipulationContainer->NotifyManipulationProgress(
                        pManipulatedElement,
                        ManipulationStarting,
                        0.0f /*xCumulativeTranslation*/,
                        0.0f /*yCumulativeTranslation*/,
                        1.0f /*zCumulativeFactor*/,
                        0.0f /*xInertiaEndTranslation*/,
                        0.0f /*yInertiaEndTranslation*/,
                        1.0f /*zInertiaEndFactor*/,
                        0.0f /*xCenter*/,
                        0.0f /*yCenter*/,
                        FALSE /*fIsInertiaEndTransformValid*/,
                        FALSE /*fIsInertial*/,
                        FALSE /*fIsTouchConfigurationActivated*/,
                        TRUE  /*fIsBringIntoViewportConfigurationActivated*/));
                    break;
                }
                case ManipulationStarting:
                {
                    if (status == XcpDMViewportEnabled)
                    {
                        // The DM container calls BringIntoViewport while processing a PointerReleased notification.
                        // Skip the call to CompleteDirectManipulation in the imminent UnregisterContactId call
                        // and do not enter the ManipulationCompleted state.
                        pViewport->SetIsCompletedStateSkipped(TRUE);
                    }
                    break;
                }
                case ManipulationDelta:
                {
                    if (status == XcpDMViewportReady)
                    {
                        // The DM container calls BringIntoViewport while handling a ManipulationDelta notification.
                        // Ignore the queued status change to Ready and do not enter the ManipulationCompleted state.
                        pViewport->SetIsCompletedStateSkipped(TRUE);
                    }
                    break;
                }
                case ManipulationLastDelta:
                {
                    // The DM container calls BringIntoViewport while handling the state change to ManipulationLastDelta.
                    // Ignore the status change to Ready and do not enter the ManipulationCompleted state.
                    pViewport->SetIsCompletedStateSkipped(TRUE);
                    break;
                }
                }
            }

            if (!fAnimate && fIsActiveStateOrStatus)
            {
                // Activate the configuration that was used before the non-animated ZoomToRect call.
                if (fWasTouchConfigurationActivated)
                {
                    IFC(UpdateManipulationTouchConfiguration(
                        pDirectManipulationService,
                        pViewport,
                        pViewport->GetTouchConfiguration()));
                }
                else if (fWasNonTouchConfigurationActivated)
                {
                    IFC(UpdateManipulationNonTouchConfiguration(
                        pDirectManipulationService,
                        pViewport,
                        pViewport->GetNonTouchConfiguration()));
                }
            }
        }

        if (fHandled)
        {
            if (!fIsForMakeVisible && pViewport->GetIsProcessingMakeVisibleInertia())
            {
                // Clear the m_fIsProcessingMakeVisibleInertia flag when a bring-into-viewport
                // request was made for another reason than a MakeVisible call.
                pViewport->SetIsProcessingMakeVisibleInertia(FALSE);
            }

            if (!fApplyAsManip)
            {
                // Ignore the transitional Running status that was put into the queue.
                pViewport->IncrementIgnoredRunningStatuses();
            }

            if ((!fApplyAsManip && newUncompressedZoomFactor != uncompressedZoomFactor) || fUpdateBounds)
            {
                if ((!fApplyAsManip && newUncompressedZoomFactor != uncompressedZoomFactor))
                {
                    // Apply the resulting transform for the compositor.
                    IFC(pDirectManipulationContainer->GetManipulationPrimaryContentTransform(
                        pManipulatedElement,
                        FALSE /*fInManipulation*/,
                        FALSE /*fForInitialTransformationAdjustment*/,
                        FALSE /*fForMargins*/,
                        &translationX,
                        &translationY,
                        &uncompressedZoomFactor));
                    pViewport->SetCompositorTransformationValues(translationX, translationY, uncompressedZoomFactor);
                }

                // Mark the manipulated element, and potential secondary contents, dirty so that HWCompTreeNode::SetElementData gets called for them and new DMData structures get used on the compositor.
                IFC(DirtyDirectManipulationTransforms(pViewport));
            }

            *pfHandled = TRUE;
        }
    }

Cleanup:
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationContainer);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   DirtyDirectManipulationTransforms
//
//  Synopsis:
//    Mark the manipulated element of the provided viewport dirty so that
//    HWCompTreeNode::SetElementData gets called for it and a new DMData
//    structure gets used on the compositor.
//    Do the same for its potential secondary contents.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::DirtyDirectManipulationTransforms(
    _In_ CDMViewport* pViewport)
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

    IFCPTR(pViewport);

    if (pViewport->GetManipulatedElementNoRef())
    {
        CUIElement::NWSetTransformDirty(pViewport->GetManipulatedElementNoRef(), DirtyFlags::Render | DirtyFlags::Bounds);
    }

    for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
    {
        IFC(pViewport->GetContent(iContent, &pContent));
        if (pContent)
        {
            if (pContent->GetContentElementNoRef())
            {
                CUIElement::NWSetTransformDirty(pContent->GetContentElementNoRef(), DirtyFlags::Render | DirtyFlags::Bounds);
            }
            ReleaseInterface(pContent);
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}

// Prepares manipulated elements for an upcoming BringIntoView
_Check_return_ HRESULT
CInputServices::PrepareCompositionNodesForBringIntoView(_In_ CDMViewport* pViewport)
{
    xref_ptr<CDMContent> spContent;
    xref_ptr<IPALDirectManipulationService> spDirectManipulationService;
    xref_ptr<IDirectManipulationViewport> spDMViewport;

    // First clear out the manipulation transform for primary content.
    pViewport->GetManipulatedElementNoRef()->ResetCompositionNodeManipulationData();

    // We don't want secondary/clip content to be updated immediately either as this
    // operation would likely happen on a different frame and appear to glitch
    UINT32 cContents = pViewport->GetContentsCount();
    for (UINT32 iContent = 0; iContent < cContents; iContent++)
    {
        IFC_RETURN(pViewport->GetContent(iContent, spContent.ReleaseAndGetAddressOf()));
        spContent->GetContentElementNoRef()->ResetCompositionNodeManipulationData();
    }

    UINT32 cClipContents = pViewport->GetClipContentsCount();
    for (UINT32 iClipContent = 0; iClipContent < cClipContents; iClipContent++)
    {
        IFC_RETURN(pViewport->GetClipContent(iClipContent, spContent.ReleaseAndGetAddressOf()));
        spContent->GetContentElementNoRef()->ResetCompositionNodeManipulationData();
    }

    // If we have items for this viewport in the deferred release queue, we need to process them immediately.
    // Not doing so would leave live DManip transforms in the tree and cause a visible flash.
    if (m_vecDeferredRelease.size() > 0)
    {
        IFC_RETURN(GetDMService(pViewport->GetDMContainerNoRef(), spDirectManipulationService.ReleaseAndGetAddressOf()));
        IFC_RETURN(spDirectManipulationService->GetDirectManipulationViewport(pViewport, spDMViewport.ReleaseAndGetAddressOf()));
        IFC_RETURN(ProcessDeferredReleaseQueue(spDMViewport));
    }

    return S_OK;
}

// Retrieve the DManip transform directly from DManip Compositor
_Check_return_ HRESULT
CInputServices::GetDirectManipulationCompositorTransform(
    _In_ CUIElement* pManipulatedElement,
    _In_ TransformRetrievalOptions transformRetrievalOptions,
    _Out_ BOOL& fTransformSet,
    _Out_ FLOAT& translationX,
    _Out_ FLOAT& translationY,
    _Out_ FLOAT& uncompressedZoomFactor,
    _Out_ FLOAT& zoomFactorX,
    _Out_ FLOAT& zoomFactorY)
{
    fTransformSet = FALSE;
    translationX = 0.0f;
    translationY = 0.0f;
    uncompressedZoomFactor = 1.0f;
    zoomFactorX = 1.0f;
    zoomFactorY = 1.0f;

    xref_ptr<IPALDirectManipulationService> spService;
    xref_ptr<IPALDirectManipulationCompositorService> spCompositorService;
    xref_ptr<IObject> spCompositorContent;
    xref_ptr<IObject> spCompositorClipContent;
    XDMContentType contentType;
    bool isInertial;
    float contentOffsetX;
    float contentOffsetY;

    IFC_RETURN(GetDirectManipulationServiceAndContent(
        pManipulatedElement,
        spService.ReleaseAndGetAddressOf(),
        spCompositorContent.ReleaseAndGetAddressOf(),
        spCompositorClipContent.ReleaseAndGetAddressOf(),
        &contentType,
        &contentOffsetX,
        &contentOffsetY
        ));

    // Only extract the DManip transform for compositor content.
    // Clip content only affects the clip transform which we're not interested in here.
    if (spCompositorContent != nullptr)
    {
        IFC_RETURN(spService->GetCompositorService(spCompositorService.ReleaseAndGetAddressOf()));
        IFC_RETURN(spCompositorService->GetCompositorContentTransform(
            spCompositorContent,
            contentType,
            isInertial,
            translationX,
            translationY,
            uncompressedZoomFactor,
            zoomFactorX,
            zoomFactorY));

        if ((transformRetrievalOptions & TransformRetrievalOptions::UseManipulationTargets) == TransformRetrievalOptions::UseManipulationTargets)
        {
            xref_ptr<CDMViewport> spViewport;
            IFC_RETURN(GetViewport(nullptr /*pDMContainer*/, pManipulatedElement, spViewport.ReleaseAndGetAddressOf()));
            if (spViewport)
            {
                float targetX;
                float targetY;
                if (spViewport->GetTargetTranslation(targetX, targetY))
                {
                    translationX = targetX;
                    translationY = targetY;
                }
            }
        }

        // Add in a special adjustment for content offsets.
        // Content offsets are typically supplied when virtualizing panels change their estimate on how big content extents are.
        // This adjustment needs to be multiplied by the zoom factor as it's in local content coordinates.
        translationX += zoomFactorX * contentOffsetX;
        translationY += zoomFactorY * contentOffsetY;

        fTransformSet = TRUE;
    }

    return S_OK;
}

// Retrieve the clip content's DManip transform directly from DManip Compositor
_Check_return_ HRESULT
CInputServices::GetDirectManipulationClipContentTransform(
    _In_ CUIElement* pClipContentElement,
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ IObject* pCompositorClipContent,
    _Out_ FLOAT& translationX,
    _Out_ FLOAT& translationY,
    _Out_ FLOAT& zoomFactorX,
    _Out_ FLOAT& zoomFactorY)
{
    FLOAT uncompressedZoomFactor = 1.0f;
    CDMViewport* pViewportNoRef = nullptr;
    xref_ptr<CDMContent> spClipContent;

    translationX = 0.0f;
    translationY = 0.0f;
    zoomFactorX = 1.0f;
    zoomFactorY = 1.0f;

    UINT cViewports = m_pViewports ? m_pViewports->size() : 0;
    for (UINT32 iViewport = 0; iViewport < cViewports; iViewport++)
    {
        IFC_RETURN(m_pViewports->get_item(iViewport, pViewportNoRef));
        ASSERT(pViewportNoRef != nullptr);

        // Search through all secondary clip contents for this viewport
        UINT32 cClipContents = pViewportNoRef->GetClipContentsCount();

        for (UINT32 iClipContent = 0; iClipContent < cClipContents; iClipContent++)
        {
            IFC_RETURN(pViewportNoRef->GetClipContent(iClipContent, spClipContent.ReleaseAndGetAddressOf()));

            if (spClipContent->GetContentElementNoRef() == pClipContentElement)
            {
                ASSERT(pCompositorClipContent == spClipContent->GetCompositorSecondaryContentNoRef());
                IFC_RETURN(pDirectManipulationService->GetSecondaryClipContentTransform(
                    pViewportNoRef,
                    spClipContent,
                    spClipContent->GetContentType(),
                    translationX,
                    translationY,
                    uncompressedZoomFactor,
                    zoomFactorX,
                    zoomFactorY));
                return S_OK;
            }
        }
    }

    return S_OK;
}

// Retrieve the end-of-inertia transform from the DManip service.
// pIsInertiaEndTransformValid is set to false when no transform could be retrieved.
// The returned transform accounts for the possible non-zero content offsets.
_Check_return_ HRESULT
CInputServices::GetDirectManipulationContentInertiaEndTransform(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _Out_ FLOAT* pInertiaEndTranslationX,
    _Out_ FLOAT* pInertiaEndTranslationY,
    _Out_ FLOAT* pInertiaEndZoomFactor,
    _Out_ bool* pIsInertiaEndTransformValid)
{
    HRESULT hr = S_OK;
    FLOAT inertiaEndTranslationX = 0.0f;
    FLOAT inertiaEndTranslationY = 0.0f;
    FLOAT inertiaEndZoomFactor = 1.0f;

    *pInertiaEndTranslationX = 0.0f;
    *pInertiaEndTranslationY = 0.0f;
    *pInertiaEndZoomFactor = 1.0f;
    *pIsInertiaEndTransformValid = false;

    // DManip has a race condition bug which causes GetContentInertiaEndTransform to fail in rare cases.
    // This is indicated by hr == S_FALSE.
    hr = pDirectManipulationService->GetContentInertiaEndTransform(
        pViewport, inertiaEndTranslationX, inertiaEndTranslationY, inertiaEndZoomFactor);
    *pIsInertiaEndTransformValid = (hr != S_FALSE);
    IFC_RETURN(hr);

    if (*pIsInertiaEndTransformValid)
    {
        FLOAT contentOffsetX = 0.0f;
        FLOAT contentOffsetY = 0.0f;
        pViewport->GetContentOffsets(contentOffsetX, contentOffsetY);

        *pInertiaEndTranslationX = inertiaEndTranslationX + contentOffsetX * inertiaEndZoomFactor;
        *pInertiaEndTranslationY = inertiaEndTranslationY + contentOffsetY * inertiaEndZoomFactor;
        *pInertiaEndZoomFactor = inertiaEndZoomFactor;
    }

    return S_OK;
}

// If pCandidateElement is primary or secondary content, returns the DManip service and content for the element, otherwise nullptrs.
_Check_return_ HRESULT
CInputServices::GetDirectManipulationServiceAndContent(
    _In_ CUIElement *pCandidateElement,
    _Outptr_result_maybenull_ IPALDirectManipulationService** ppDMService,
    _Outptr_result_maybenull_ IObject** ppCompositorContent,
    _Outptr_result_maybenull_ IObject** ppCompositorClipContent,
    _Out_ XDMContentType* pDMContentType,
    _Out_ float* pContentOffsetX,
    _Out_ float* pContentOffsetY)
{
    HRESULT hr = S_OK;
    CDMViewport *pViewport = nullptr;
    xref_ptr<IPALDirectManipulationService> spDMService;
    xref_ptr<IObject> spCompositorContent;
    xref_ptr<IObject> spCompositorClipContent;
    xref_ptr<CDMContent> spContent;

    *ppDMService = nullptr;
    *ppCompositorContent = nullptr;
    *ppCompositorClipContent = nullptr;
    *pContentOffsetX = 0;
    *pContentOffsetY = 0;

    UINT cViewports = m_pViewports ? m_pViewports->size() : 0;
    for (UINT32 iViewport = 0; iViewport < cViewports; iViewport++)
    {
        IFC(m_pViewports->get_item(iViewport, pViewport));

        // First check to see if this candidate is the primary content for this viewport.
        if (pViewport->GetNeedsUnregistration())
        {
            // Skip over viewports that have unregistration pending.
            continue;
        }

        if (pViewport->GetManipulatedElementNoRef() == pCandidateElement)
        {
            // Aha!  The candidate is the primary content.
            IFC(GetDMService(pViewport->GetDMContainerNoRef(), spDMService.ReleaseAndGetAddressOf()));
            ASSERT(spDMService != nullptr);

            spCompositorContent = pViewport->GetCompositorPrimaryContentNoRef();

            // Ensure that we create the primary content wrapper object and push it into the viewport.
            // This doesn't happen until a manipulation begins when we're not in DManip-on-DComp mode.
            if (spCompositorContent == nullptr)
            {
                IFC(spDMService->GetCompositorPrimaryContent(pViewport, spCompositorContent.ReleaseAndGetAddressOf()));
                pViewport->SetCompositorPrimaryContent(spCompositorContent);
            }
            ASSERT(spCompositorContent != nullptr);
            *pDMContentType = XcpDMContentTypePrimary;
            pViewport->GetContentOffsets(*pContentOffsetX, *pContentOffsetY);
            goto Cleanup;
        }

        // Now search through all secondary content for this viewport
        UINT32 cContents = pViewport->GetContentsCount();

        for (UINT32 iContent = 0; iContent < cContents; iContent++)
        {
            IFC(pViewport->GetContent(iContent, spContent.ReleaseAndGetAddressOf()));

            if (spContent->GetContentElementNoRef() == pCandidateElement)
            {
                // Aha!  The candidate is secondary content
                IFC(GetDMService(pViewport->GetDMContainerNoRef(), spDMService.ReleaseAndGetAddressOf()));
                ASSERT(spDMService != nullptr);

                spCompositorContent = spContent->GetCompositorSecondaryContentNoRef();
                if (spCompositorContent == nullptr)
                {
                    IFC(spDMService->GetCompositorSecondaryContent(pViewport, spContent, spCompositorContent.ReleaseAndGetAddressOf()));
                    spContent->SetCompositorSecondaryContent(spCompositorContent);
                }
                *pDMContentType = spContent->GetContentType();
                XFLOAT unusedContentOffset = 0.0f;
                if (*pDMContentType == XcpDMContentTypeLeftHeader)
                {
                    pViewport->GetContentOffsets(unusedContentOffset, *pContentOffsetY);
                }
                else if (*pDMContentType == XcpDMContentTypeTopHeader)
                {
                    pViewport->GetContentOffsets(*pContentOffsetX, unusedContentOffset);
                }
                goto Cleanup;
            }
        }

        // Now search through all secondary clip content for this viewport
        UINT32 cClipContents = pViewport->GetClipContentsCount();

        for (UINT32 iClipContent = 0; iClipContent < cClipContents; iClipContent++)
        {
            IFC(pViewport->GetClipContent(iClipContent, spContent.ReleaseAndGetAddressOf()));

            if (spContent->GetContentElementNoRef() == pCandidateElement)
            {
                // Aha!  The candidate is secondary clip content
                IFC(GetDMService(pViewport->GetDMContainerNoRef(), spDMService.ReleaseAndGetAddressOf()));
                ASSERT(spDMService != nullptr);

                spCompositorClipContent = spContent->GetCompositorSecondaryContentNoRef();
                if (spCompositorClipContent == nullptr)
                {
                    IFC(spDMService->GetCompositorSecondaryClipContent(pViewport, spContent, spCompositorClipContent.ReleaseAndGetAddressOf()));
                    spContent->SetCompositorSecondaryContent(spCompositorClipContent);
                }
                *pDMContentType = spContent->GetContentType();
                goto Cleanup;
            }
        }
    }

Cleanup:
    *ppDMService = spDMService.detach();
    *ppCompositorContent = spCompositorContent.detach();
    *ppCompositorClipContent = spCompositorClipContent.detach();

    RRETURN(hr);
}

_Check_return_ _Maybenull_ CUIElement*
CInputServices::GetPrimaryContentDMContainer(_In_ CUIElement* manipulatedElement) const
{
    if (m_pViewports)
    {
        XUINT32 viewportCount = m_pViewports->size();
        for (UINT32 viewportIndex = 0; viewportIndex != viewportCount; ++viewportIndex)
        {
            CDMViewport *viewport;
            IFCFAILFAST(m_pViewports->get_item(viewportIndex, viewport));
            if (!viewport->GetNeedsUnregistration() && viewport->GetManipulatedElementNoRef() == manipulatedElement)
            {
                return viewport->GetDMContainerNoRef();
            }
        }
    }
    return nullptr;
}

_Check_return_ _Maybenull_ CUIElement*
CInputServices::GetPrimaryContentManipulatedElement(_In_ CUIElement* dmContainerElement) const
{
    CDMViewport *viewport = GetViewport(dmContainerElement);
    return viewport ? viewport->GetManipulatedElementNoRef() : nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   GetDirectManipulationContents
//
//  Synopsis:
//    Returns a vector of secondary contents associated with the provided manipulated primary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetDirectManipulationContents(
    _In_ CUIElement* pManipulatedElement,
    _Out_ xvector<CDMContent*>** ppContents)
{
    HRESULT hr = S_OK;
    XUINT32 cContents = 0;
    CDMContent* pContent = NULL;
    CDMViewport* pViewport = NULL;
    xvector<CDMContent*>* pContents = NULL;

    IFCPTR(pManipulatedElement);
    IFCPTR(ppContents);
    *ppContents = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: GetDirectManipulationContents - entry.", this));
    }
#endif // DM_DEBUG

    IFC(GetViewport(NULL /*pDMContainer*/, pManipulatedElement, &pViewport));
    if (pViewport && (cContents = pViewport->GetContentsCount()) > 0)
    {
        pContents = new xvector<CDMContent*>();

        for (XUINT32 iContent = 0; iContent < cContents; iContent++)
        {
            ASSERT(!pContent);
            IFC(pViewport->GetContent(iContent, &pContent));
            ASSERT(pContent);
            IFC(pContents->push_back(pContent));
            pContent = NULL;
        }
        *ppContents = pContents;
        pContents = NULL;
    }

Cleanup:
#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        if (ppContents && *ppContents)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                L"                   Returns array with %d entrie(s).", (*ppContents)->size()));
        }
        else
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                L"                   Returns NULL array."));
        }
    }
#endif // DM_DEBUG

    ReleaseInterface(pContent);
    ReleaseInterface(pViewport);

    if (pContents)
    {
        cContents = pContents->size();
        for (XUINT32 iContent = 0; iContent < cContents; iContent++)
        {
            ASSERT(!pContent);
            IGNOREHR(pContents->get_item(iContent, pContent));
            ASSERT(pContent);
            ReleaseInterface(pContent);
        }
        delete pContents;
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetDirectManipulationClipContents
//
//  Synopsis:
//    Returns a vector of secondary clip contents associated with the provided manipulated primary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::GetDirectManipulationClipContents(
    _In_ CUIElement* pManipulatedElement,
    _Out_ xvector<CDMContent*>** ppClipContents)
{
    HRESULT hr = S_OK;
    XUINT32 cClipContents = 0;
    CDMContent* pClipContent = NULL;
    CDMViewport* pViewport = NULL;
    xvector<CDMContent*>* pClipContents = NULL;

    IFCPTR(pManipulatedElement);
    IFCPTR(ppClipContents);
    *ppClipContents = NULL;

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: GetDirectManipulationClipContents - entry.", this));
    }
#endif // DM_DEBUG

    IFC(GetViewport(NULL /*pDMContainer*/, pManipulatedElement, &pViewport));
    if (pViewport && (cClipContents = pViewport->GetClipContentsCount()) > 0)
    {
        pClipContents = new xvector<CDMContent*>();

        for (XUINT32 iClipContent = 0; iClipContent < cClipContents; iClipContent++)
        {
            ASSERT(!pClipContent);
            IFC(pViewport->GetClipContent(iClipContent, &pClipContent));
            ASSERT(pClipContent);
            IFC(pClipContents->push_back(pClipContent));
            pClipContent = NULL;
        }
        *ppClipContents = pClipContents;
        pClipContents = NULL;
    }

Cleanup:
#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        if (ppClipContents && *ppClipContents)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                L"                   Returns array with %d entrie(s).", (*ppClipContents)->size()));
        }
        else
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
                L"                   Returns NULL array."));
        }
    }
#endif // DM_DEBUG

    ReleaseInterface(pClipContent);
    ReleaseInterface(pViewport);

    if (pClipContents)
    {
        cClipContents = pClipContents->size();
        for (XUINT32 iClipContent = 0; iClipContent < cClipContents; iClipContent++)
        {
            ASSERT(!pClipContent);
            IGNOREHR(pClipContents->get_item(iClipContent, pClipContent));
            ASSERT(pClipContent);
            ReleaseInterface(pClipContent);
        }
        delete pClipContents;
    }
    RRETURN(hr);
}

// Returns manipulated element's viewport status
_Check_return_ HRESULT
CInputServices::GetDirectManipulationViewportStatus(
    _In_ CUIElement* pManipulatedElement,
    _Out_ XDMViewportStatus* pStatus)
{
    xref_ptr<CDMViewport> spViewport;

    *pStatus = XcpDMViewportBuilding;

    IFC_RETURN(GetViewport(nullptr /*pDMContainer*/, pManipulatedElement, spViewport.ReleaseAndGetAddressOf()));
    if (spViewport != nullptr)
    {
        IFC_RETURN(spViewport->GetCurrentStatus(*pStatus));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CompleteDirectManipulation
//
//  Synopsis:
//    Called when the ongoing DirectManipulation for the provided viewport
//    needs to be completed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::CompleteDirectManipulation(
    _In_ CDMViewport* pViewport,
    _In_ bool fDisableViewport)
{
    HRESULT hr = S_OK;
    XUINT32 contactId = 0;
    XUINT32 cContactIds = 0;
    XFLOAT initialTranslationX = 0.0f;
    XFLOAT initialTranslationY = 0.0f;
    XFLOAT initialUncompressedZoomFactor = 1.0f;
    XFLOAT initialZoomFactorX = 1.0f;
    XFLOAT initialZoomFactorY = 1.0f;
    XFLOAT currentTranslationX = 0.0f;
    XFLOAT currentTranslationY = 0.0f;
    XFLOAT currentUncompressedZoomFactor = 1.0f;
    XFLOAT currentZoomFactorX = 1.0f;
    XFLOAT currentZoomFactorY = 1.0f;
    CUIElement* pDMContainer = NULL;
    CUIElement* pManipulatedElement = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;

    IFCPTR(pViewport);

#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  CompleteDirectManipulation for pViewport=0x%p, state=%d, fDisableViewport=%d.",
            this, pViewport, pViewport->GetState(), fDisableViewport));
    }
#endif // DM_DEBUG

    if (pViewport->GetIsCompletedStateSkipped())
    {
#ifdef DM_DEBUG
        if (m_fIsDMInfoTracingEnabled)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                L"                   Manipulation completed even though IsCompletedStateSkipped was set. Resetting flag."));
        }
#endif // DM_DEBUG

        pViewport->SetIsCompletedStateSkipped(FALSE);
    }

    if (!fDisableViewport ||
        (!pViewport->GetNeedsUnregistration() && !pViewport->GetUnregistered()))
    {
        pDMContainer = pViewport->GetDMContainerNoRef();
        ASSERT(pDMContainer);

        if (fDisableViewport)
        {
            IFC(GetDMService(pDMContainer, &pDirectManipulationService));
            ASSERT(pDirectManipulationService);

            IFC(pDirectManipulationService->DisableViewport(pViewport));
            IFC(pDirectManipulationService->ReleaseAllContacts(pViewport));

            // Since all contacts were released with DManip, clear them from the CDMViewportBase
            IFC(pViewport->GetContactIdCount(&cContactIds));
            for (XUINT32 iContactId = 0; iContactId < cContactIds; iContactId++)
            {
                IFC(pViewport->GetContactId(0, &contactId));
                IFC(pViewport->RemoveContactId(contactId));
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   pViewport=0x%p, Removed pointerId=%d, New contactId count=%d.",
                        pViewport, contactId, cContactIds));
                }
#endif // DM_DEBUG
            }
        }
        else if (pViewport->GetState() != ManipulationCompleted)
        {
            // Typically pViewport->GetState() is ManipulationStarting when lifting a finger while no DManip was recognized.
            // Occasionally pViewport->GetState() is ManipulationStarted in nested ScrollViewers cases.
            // Occasionally pViewport->GetState() is ManipulationDelta in nested ScrollViewers cases, when a parent viewport's transition to Ready does not complete a manip because of a chaining child viewport.
            // Occasionally pViewport->GetState() is ManipulationDelta when a DManip is cancelled. All contacts are then released and a subsequent XCP_POINTERUP message causes a UnregisterContactId call.
            if (pViewport->GetState() == ManipulationDelta)
            {
#ifdef DM_DEBUG
                // Make sure the NotifyManipulationProgress(ManipulationLastDelta) notification is raised in the rare cases where the CDMViewport IM state is ManipulationDelta.
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"                   CompleteDirectManipulation called during IM state ManipulationDelta."));
                }
#endif // DM_DEBUG
                IFC(ProcessDirectManipulationViewportValuesUpdate(pViewport, FALSE /*fWasInertial*/, FALSE /*fIsInertial*/, TRUE /*fIsLastDelta*/));
            }

            // Update the viewport state, the DM container and the compositor
            IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
            ASSERT(pDirectManipulationContainer);

            pManipulatedElement = pViewport->GetManipulatedElementNoRef();
            ASSERT(pManipulatedElement);
#ifdef DM_DEBUG
            if (m_fIsDMInfoTracingEnabled)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                    L"                   Calling NotifyManipulationProgress(ManipulationCompleted) for pViewport=0x%p, state=%d.",
                    pViewport, pViewport->GetState()));
            }
#endif // DM_DEBUG

            pViewport->GetInitialTransformationValues(
                initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY);

            pViewport->GetCurrentTransformationValues(
                currentTranslationX, currentTranslationY, currentUncompressedZoomFactor, currentZoomFactorX, currentZoomFactorY);

            pViewport->SetState(ManipulationCompleted);
            pViewport->SetIsPrimaryContentLayoutRefreshedAfterCompletion(FALSE);

            IFC(pDirectManipulationContainer->NotifyManipulationProgress(
                pManipulatedElement,
                ManipulationCompleted,
                currentTranslationX - initialTranslationX /*xCumulativeTranslation*/,
                currentTranslationY - initialTranslationY /*yCumulativeTranslation*/,
                currentUncompressedZoomFactor / initialUncompressedZoomFactor /*zCumulativeFactor*/,
                0.0f /*xInertiaEndTranslation*/,
                0.0f /*yInertiaEndTranslation*/,
                1.0f /*zInertiaEndFactor*/,
                0.0f /*xCenter*/,
                0.0f /*yCenter*/,
                FALSE /*fIsInertiaEndTransformValid*/,
                FALSE /*fIsInertial*/,
                pViewport->GetIsTouchConfigurationActivated() /*fIsTouchConfigurationActivated*/,
                pViewport->GetIsBringIntoViewportConfigurationActivated() /*fIsBringIntoViewportConfigurationActivated*/));

            pViewport->DeclareOldViewportForCompositor();
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationService);
    ReleaseInterface(pDirectManipulationContainer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UpdateDirectManipulationManagerActivation
//
//  Synopsis:
//    Stops the ongoing DM manipulations for the provided DM container
//    when fCancelManipulations is True, for the viewports for which
//    the activate configuration corresponds to a disabled manipulability.
//    Attempts the activate or deactivate the DM manager associated with
//    the provided DM container according to the three fCanManipulateElements***
//    parameters.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateDirectManipulationManagerActivation(
    _In_ CUIElement* pDMContainer,
    _In_ bool fCancelManipulations,
    _In_ bool fCanManipulateElementsByTouch,
    _In_ bool fCanManipulateElementsNonTouch,
    _In_ bool fCanManipulateElementsWithBringIntoViewport,
    _In_ bool fRefreshViewportStatus)
{
    HRESULT hr = S_OK;
    XUINT32 cViewports = 0;
    bool fHasActiveViewport = false;
    XDMViewportStatus currentStatus = XcpDMViewportBuilding;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CDMViewport* pViewport = NULL;
    CUIElement* pManipulatedElement = NULL;
    CUIElement* pContentElement = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    IFCPTR(pDMContainer);

    // Access the DM service associated to this DM container.
    IFC(GetDMService(pDMContainer, &pDirectManipulationService));
    if (pDirectManipulationService)
    {
        if (m_pViewports)
        {
            cViewports = m_pViewports->size();
            for (XUINT32 iViewport = 0; iViewport < cViewports; iViewport++)
            {
                IFC(m_pViewports->get_item(iViewport, pViewport));
                ASSERT(pViewport);
                if (pViewport->GetDMContainerNoRef() == pDMContainer)
                {
                    IFC(pDirectManipulationService->GetViewportStatus(pViewport, status));
                    if (fCanManipulateElementsByTouch || fCanManipulateElementsNonTouch || fCanManipulateElementsWithBringIntoViewport)
                    {
                        if (fRefreshViewportStatus)
                        {
                            // When the application is reactivated, make sure the CDMViewport's status
                            // reflects the actual viewport status, which is XcpDMViewportBuilding or XcpDMViewportDisabled
                            IFC(pViewport->GetCurrentStatus(currentStatus));
                            if (status != currentStatus)
                            {
#ifdef DM_DEBUG
                                if (m_fIsDMInfoTracingEnabled)
                                {
                                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                        L"DMIM[0x%p]:  UpdateDirectManipulationManagerActivation updates current DM status %d for pViewport=0x%p, state=%d.",
                                        this, status, pViewport, pViewport->GetState()));
                                }
#endif // DM_DEBUG

                                IFC(pViewport->SetCurrentStatus(status));
                            }
                        }

                        pManipulatedElement = pViewport->GetManipulatedElementNoRef();
                        if (pManipulatedElement && !pManipulatedElement->IsManipulatable())
                        {
                            // Mark the element as manipulatable
                            IFC(pManipulatedElement->SetRequiresComposition(
                                CompositionRequirement::Manipulatable,
                                IndependentAnimationType::None));
                            // ...as well as its potential secondary contents.
                            for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
                            {
                                IFC(pViewport->GetContentElementNoRef(iContent, &pContentElement));
                                ASSERT(pContentElement);
                                IFC(pContentElement->SetRequiresComposition(
                                    CompositionRequirement::Manipulatable,
                                    IndependentAnimationType::None));
                            }
                            for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
                            {
                                IFC(pViewport->GetClipContentElementNoRef(iClipContent, &pContentElement));
                                ASSERT(pContentElement);
                                IFC(pContentElement->SetRequiresComposition(
                                    CompositionRequirement::Manipulatable,
                                    IndependentAnimationType::None));
                            }
                        }
                    }

                    IFC(pViewport->GetCurrentStatus(currentStatus));
                    if (IsViewportActive(currentStatus) || IsViewportActive(status))
                    {
#ifdef DM_DEBUG
                        if (m_fIsDMInfoTracingEnabled)
                        {
                            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                L"DMIM[0x%p]:  UpdateDirectManipulationManagerActivation - pViewport=0x%p is active: DM status=%d, IM state=%d, IM status=%d.",
                                this, pViewport, status, pViewport->GetState(), currentStatus));
                        }
#endif // DM_DEBUG

                        // Disable any possible viewport for this container.
                        if (fCancelManipulations)
                        {
                            if ((pViewport->GetIsTouchConfigurationActivated() && !fCanManipulateElementsByTouch) ||
                                (pViewport->GetIsNonTouchConfigurationActivated() && !fCanManipulateElementsNonTouch) ||
                                (pViewport->GetIsBringIntoViewportConfigurationActivated() && !fCanManipulateElementsWithBringIntoViewport))
                            {
#ifdef DM_DEBUG
                                if (m_fIsDMInfoTracingEnabled)
                                {
                                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                                        L"                   Disabling viewport and releasing all contacts.",
                                        this, pViewport, status, pViewport->GetState(), currentStatus));
                                }
#endif // DM_DEBUG

                                IFC(pDirectManipulationService->DisableViewport(pViewport));
                                IFC(pDirectManipulationService->ReleaseAllContacts(pViewport));
                            }
                        }
                        fHasActiveViewport = TRUE;
                    }
                    else if (!fCanManipulateElementsByTouch && !fCanManipulateElementsNonTouch && !fCanManipulateElementsWithBringIntoViewport)
                    {
                        if (fCancelManipulations && pViewport->GetState() == ManipulationStarting)
                        {
                            // The manipulated element became non-manipulatable in an inactive status, while the state is ManipulationStarting.
                            // Release all contacts, disable the viewport and complete the manipulation by entering the ManipulationCompleted state.
                            IFC(UnregisterContactIds(pViewport, TRUE /*fReleaseAllContactsAndDisableViewport*/));
                        }

                        pManipulatedElement = pViewport->GetManipulatedElementNoRef();
                        if (pManipulatedElement && pManipulatedElement->IsManipulatable() && !pManipulatedElement->IsManipulatedIndependently())
                        {
                            // Mark the element as no longer manipulatable
                            pManipulatedElement->UnsetRequiresComposition(
                                CompositionRequirement::Manipulatable,
                                IndependentAnimationType::None);
                            // ...as well as its potential secondary contents.
                            for (XUINT32 iContent = 0; iContent < pViewport->GetContentsCount(); iContent++)
                            {
                                IFC(pViewport->GetContentElementNoRef(iContent, &pContentElement));
                                ASSERT(pContentElement);
                                pContentElement->UnsetRequiresComposition(
                                    CompositionRequirement::Manipulatable,
                                    IndependentAnimationType::None);
                            }
                            for (XUINT32 iClipContent = 0; iClipContent < pViewport->GetClipContentsCount(); iClipContent++)
                            {
                                IFC(pViewport->GetClipContentElementNoRef(iClipContent, &pContentElement));
                                ASSERT(pContentElement);
                                pContentElement->UnsetRequiresComposition(
                                    CompositionRequirement::Manipulatable,
                                    IndependentAnimationType::None);
                            }
                        }
                    }

                    // Make sure that when the viewport belongs to the root ScrollViewer, the manipulated element is dirtied so that
                    // HWCompTreeNodeWinRT::IsViewportInteractionRequired gets invoked in order to create or discard the viewport interaction object.
                    if (pDMContainer->GetTypeIndex() == KnownTypeIndex::RootScrollViewer)
                    {
                        if (!pManipulatedElement)
                        {
                            pManipulatedElement = pViewport->GetManipulatedElementNoRef();
                        }
                        CUIElement::NWSetContentDirty(pManipulatedElement, DirtyFlags::Render);
                    }
                }
            }
        }

        // Activate or deactivate the DM manager for this container.
        if (fCanManipulateElementsByTouch || fCanManipulateElementsNonTouch || fCanManipulateElementsWithBringIntoViewport)
        {
            IFC(pDirectManipulationService->ActivateDirectManipulationManager());
        }
        else if (!fHasActiveViewport)
        {
            // Avoid calling IDirectManipulationManager::Deactivate while there is an active viewport otherwise the expected
            // status change event never gets raised.
            // DM manager gets deactivated once active viewports have completed for this DM container.
            IFC(pDirectManipulationService->DeactivateDirectManipulationManager());
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UpdateDirectManipulationManagerActivation
//
//  Synopsis:
//    Attempts the activate or deactivate the DM manager associated with
//    the provided DM container according to the manipulability of its elements.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateDirectManipulationManagerActivation(
    _In_ CUIElement* pDMContainer,
    _In_ bool fRefreshViewportStatus)
{
    HRESULT hr = S_OK;
    bool fCanManipulateElementsByTouch = false;
    bool fCanManipulateElementsNonTouch = false;
    bool fCanManipulateElementsWithBringIntoViewport = false;
    CUIDMContainer* pDirectManipulationContainer = NULL;

    IFCPTR(pDMContainer);

    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
    ASSERT(pDirectManipulationContainer);

    IFC(pDirectManipulationContainer->GetCanManipulateElements(&fCanManipulateElementsByTouch, &fCanManipulateElementsNonTouch, &fCanManipulateElementsWithBringIntoViewport));

    // Update DM manager active status for this container.
    IFC(UpdateDirectManipulationManagerActivation(
        pDMContainer,
        FALSE /*fCancelManipulations*/,
        fCanManipulateElementsByTouch,
        fCanManipulateElementsNonTouch,
        fCanManipulateElementsWithBringIntoViewport,
        fRefreshViewportStatus));

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationConfigurations
//
//  Synopsis:
//    Pushes configuration changes to DM through the DM Service.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationConfigurations(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _In_ XUINT32 cConfigurations,
    _In_reads_(cConfigurations) XDMConfigurations* pConfigurations,
    _Out_opt_ bool* pfConfigurationsUpdated)
{
    HRESULT hr = S_OK;
    XUINT32 iOldConfiguration = 0;
    XUINT32 iNewConfiguration = 0;
    XDMConfigurations configuration = XcpDMConfigurationNone;

    IFCPTR(pDirectManipulationService);
    IFCPTR(pViewport);
    IFCPTR(pConfigurations);
    if (pfConfigurationsUpdated)
    {
        *pfConfigurationsUpdated = FALSE;
    }

    // Remove all old configurations that are no longer valid.
    do
    {
        IFC(pViewport->GetConfiguration(iOldConfiguration, configuration));
        if (configuration != XcpDMConfigurationNone)
        {
            for (iNewConfiguration = 0; iNewConfiguration < cConfigurations; iNewConfiguration++)
            {
                if (pConfigurations[iNewConfiguration] == configuration)
                {
                    // Old configuration is still valid
                    break;
                }
            }
            if (iNewConfiguration == cConfigurations)
            {
                // Old configuration is no longer valid
                IFC(pDirectManipulationService->RemoveViewportConfiguration(pViewport, configuration));
                if (hr == S_FALSE)
                {
                    // Configuration could not be removed.
                    goto Cleanup;
                }
                IFC(pViewport->RemoveConfiguration(configuration));
            }
            else
            {
                iOldConfiguration++;
            }
        }
    }
    while (configuration != XcpDMConfigurationNone);

    // Add all new configurations
    for (iNewConfiguration = 0; iNewConfiguration < cConfigurations; iNewConfiguration++)
    {
        ASSERT(pConfigurations[iNewConfiguration] != XcpDMConfigurationNone);
        iOldConfiguration = 0;
        do
        {
            IFC(pViewport->GetConfiguration(iOldConfiguration, configuration));
            if (configuration != pConfigurations[iNewConfiguration])
            {
                iOldConfiguration++;
            }
        }
        while (configuration != XcpDMConfigurationNone &&
               configuration != pConfigurations[iNewConfiguration]);

        if (configuration == XcpDMConfigurationNone)
        {
            // New configuration is not registered yet
            hr = pDirectManipulationService->AddViewportConfiguration(pViewport, FALSE /*fIsCrossSlideViewport*/, FALSE /*fIsDragDrop*/, pConfigurations[iNewConfiguration]);
            if (hr == S_FALSE)
            {
                // Configuration could not be added.
                goto Cleanup;
            }
            if (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
            {
                // Workaround for RS1 bug 6848725. Somehow that configuration was already registered with DManip.
                // The root cause of that bug is unknown. A configuration is probably activated without being first
                // registered. As a harmless workaround, the DManip return code is ignored and the configuration is
                // added to the CDMViewport configurations array since it was not found in it above.
#ifdef DM_DEBUG
                if (m_fIsDMInfoTracingEnabled)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
                        L"DMIM[0x%p]:  UpdateManipulationConfigurations - unexpected registered configuration. pDirectManipulationService=0x%p, pViewport=0x%p, config=%d.",
                        this, pDirectManipulationService, pViewport, pConfigurations[iNewConfiguration]));
                }
#endif // DM_DEBUG
                hr = S_OK;
            }
            else
            {
                IFC(hr);
            }
            IFC(pViewport->AddConfiguration(pConfigurations[iNewConfiguration]));

            // Now that we have a viewport and a viewport configuration, let's try applying the secondary content relationships again.
            IFC(ApplySecondaryContentRelationships());
        }
    }

    if (pfConfigurationsUpdated)
    {
        *pfConfigurationsUpdated = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationTouchConfiguration
//
//  Synopsis:
//    Stores touchConfiguration in the provided CDMViewport.
//    Pushes the new active configuration to DM through the DM Service if
//    one is provided.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationTouchConfiguration(
    _In_opt_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _In_ XDMConfigurations touchConfiguration)
{
    IFCPTR_RETURN(pViewport);

    // Set active configuration
    pViewport->SetTouchConfiguration(touchConfiguration);
    if (pDirectManipulationService)
    {
        // Activate active configuration
        ASSERT(touchConfiguration != XcpDMConfigurationNone);

#ifdef DBG
        bool fHasConfiguration = false;
        IGNOREHR(pViewport->HasConfiguration(touchConfiguration, fHasConfiguration));
        ASSERT(fHasConfiguration);
#endif // DBG

        bool activationFailed = false;

        IFC_RETURN(pDirectManipulationService->ActivateViewportConfiguration(pViewport, touchConfiguration, &activationFailed));
        if (activationFailed)
        {
            // c.f. RS1 bug 5697333.
            // Configuration could not be activated because it was not pre-registered before the viewport
            // became active and sealed. Do not mark the configuration as activated in the CDMViewport instance.
            return S_FALSE;
        }
        pViewport->SetIsTouchConfigurationActivated(TRUE);
        pViewport->SetIsNonTouchConfigurationActivated(FALSE);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationNonTouchConfiguration
//
//  Synopsis:
//    Stores nonTouchConfiguration in the provided CDMViewport.
//    Pushes the new non-touch configuration to DM through the DM Service
//    if one is provided.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationNonTouchConfiguration(
    _In_opt_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _In_ XDMConfigurations nonTouchConfiguration)
{
    IFCPTR_RETURN(pViewport);

    // Set non-touch configuration
    pViewport->SetNonTouchConfiguration(nonTouchConfiguration);
    if (pDirectManipulationService)
    {
        // Activate non-touch configuration
        ASSERT(nonTouchConfiguration != XcpDMConfigurationNone);

#ifdef DBG
        bool fHasConfiguration = false;
        IGNOREHR(pViewport->HasConfiguration(nonTouchConfiguration, fHasConfiguration));
        ASSERT(fHasConfiguration);
#endif // DBG

        bool activationFailed = false;

        IFC_RETURN(pDirectManipulationService->ActivateViewportConfiguration(pViewport, nonTouchConfiguration, &activationFailed));
        if (activationFailed)
        {
            // c.f. RS1 bug 5697333.
            // Configuration could not be activated because it was not pre-registered before the viewport
            // became active and sealed. Do not mark the configuration as activated in the CDMViewport instance.
            return S_FALSE;
        }
        pViewport->SetIsNonTouchConfigurationActivated(TRUE);
        pViewport->SetIsTouchConfigurationActivated(FALSE);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateManipulationOverpanModes
//
//  Synopsis:
//    Fetches and stores the horizontal and vertical XDMOverpanModes for the viewport.
//    Recreates the DM curves to define the overpan with compression effect,
//    or overpan suppression effect.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateManipulationOverpanModes(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _In_ bool fIsStartingNewManipulation)
{
    HRESULT hr = S_OK;
    bool fAreOverpanModesChanged = false;
    XDMOverpanMode horizontalOverpanMode = XcpDMOverpanModeDefault;
    XDMOverpanMode verticalOverpanMode = XcpDMOverpanModeDefault;
    CUIElement* pManipulatedElement = NULL;
    CUIDMContainer* pDirectManipulationContainer = NULL;
    CUIElement* pDMContainer = NULL;

    ASSERT(pViewport);

    IFCPTR(pDirectManipulationService);

    pManipulatedElement = pViewport->GetManipulatedElementNoRef();
    ASSERT(pManipulatedElement);
    pDMContainer = pViewport->GetDMContainerNoRef();
    IFC(pDMContainer->GetDirectManipulationContainer(&pDirectManipulationContainer));
    ASSERT(pDirectManipulationContainer);

    IFC(pDirectManipulationContainer->GetManipulationViewport(
        pManipulatedElement,
        NULL /*pBounds*/,
        NULL /*pInputTransform*/,
        NULL /*pTouchConfiguration*/,
        NULL /*pNonTouchConfiguration*/,
        NULL /*pBringIntoViewportConfiguration*/,
        NULL /*pcConfigurations*/,
        NULL /*ppConfigurations*/,
        NULL /*pChainedMotionTypes*/,
        &horizontalOverpanMode /*pHorizontalOverpanMode*/,
        &verticalOverpanMode /*pVerticalOverpanMode*/));

    const auto scale = RootScale::GetRasterizationScaleForElement(pManipulatedElement);
    IFC(pDirectManipulationService->ApplyOverpanModes(
        pViewport,
        horizontalOverpanMode,
        verticalOverpanMode,
        scale,
        fIsStartingNewManipulation,
        &fAreOverpanModesChanged));

    if (fAreOverpanModesChanged)
    {
        // Make sure new DManip transforms get set so that CDirectManipulationService::ReleaseSharedContentTransform and CDirectManipulationService::EnsureSharedContentTransform
        // get called for all content types affected by overpan reflexes.

        // First force a refresh of the primary content's manipulation transform.
        pManipulatedElement->ResetCompositionNodeManipulationData();

        // Then do the same for the 2 secondary content types affected by overpan reflexes.
        xref_ptr<CDMContent> spContent;
        UINT32 cContents = pViewport->GetContentsCount();
        for (UINT32 iContent = 0; iContent < cContents; iContent++)
        {
            IFC(pViewport->GetContent(iContent, spContent.ReleaseAndGetAddressOf()));
            XDMContentType contentType = spContent->GetContentType();
            if (contentType == XcpDMContentTypeLeftHeader || contentType == XcpDMContentTypeTopHeader)
            {
                spContent->GetContentElementNoRef()->ResetCompositionNodeManipulationData();
            }
        }
    }

Cleanup:
    ReleaseInterface(pDirectManipulationContainer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UpdateBringIntoViewportConfiguration
//
//  Synopsis:
//    Stores bringIntoViewportConfiguration in the provided CDMViewport.
//    Pushes the bring-into-viewport configuration to DM through the DM Service.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInputServices::UpdateBringIntoViewportConfiguration(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _In_ XDMConfigurations bringIntoViewportConfiguration)
{
    IFCPTR_RETURN(pDirectManipulationService);
    IFCPTR_RETURN(pViewport);

    ASSERT(bringIntoViewportConfiguration != XcpDMConfigurationNone);

#ifdef DBG
    bool fHasConfiguration = false;
    IGNOREHR(pViewport->HasConfiguration(bringIntoViewportConfiguration, fHasConfiguration));
    ASSERT(fHasConfiguration);
#endif // DBG

    // Set bring-into-viewport configuration
    pViewport->SetBringIntoViewportConfiguration(bringIntoViewportConfiguration);

    bool activationFailed = false;

    IFC_RETURN(pDirectManipulationService->ActivateViewportConfiguration(pViewport, bringIntoViewportConfiguration, &activationFailed));
    if (activationFailed)
    {
        // c.f. RS1 bug 5697333.
        // Configuration could not be activated because it was not pre-registered before the viewport
        // became active and sealed. Do not mark the configuration as activated in the CDMViewport instance.
        return S_FALSE;
    }
    pViewport->SetIsTouchConfigurationActivated(FALSE);
    pViewport->SetIsNonTouchConfigurationActivated(FALSE);

    return S_OK;
}

// Pushes the viewport's touch configuration to DM through the DM Service.
// Marks the viewport as expecting an interaction completion notification
// and queues up a manipulation recognition notification.
_Check_return_ HRESULT
CInputServices::SwitchToTouchConfiguration(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport)
{
#ifdef DM_DEBUG
    if (m_fIsDMInfoTracingEnabled && pViewport->HasQueuedInteractionType())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/,
            L"DMIM[0x%p]:  SwitchToTouchConfiguration entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    IFC_RETURN(UpdateManipulationTouchConfiguration(
        pDirectManipulationService,
        pViewport,
        pViewport->GetTouchConfiguration()));

    // Handle the upcoming XcpDMViewportInteractionEnd notification for this touch-based interaction.
    pViewport->SetIsTouchInteractionEndExpected(TRUE);
    // Make sure any subsequent transition to the Running status for this current interaction does not queue up another start notification.
    pViewport->SetIsTouchInteractionStartProcessed(TRUE);
    // Queue up a start notification to trigger the ScrollViewer.DirectManipulationStarted event.
    pViewport->PushInteractionType(XcpDMViewportInteractionManipulation);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsElementInViewport
//
//  Synopsis:
//    Debugging method that checks if a UIElement is a child of
//    the manipulated element for the provided viewport.
//
//------------------------------------------------------------------------
bool
CInputServices::IsElementInViewport(
    _In_ CUIElement* pUIElement,
    _In_ CDMViewport* pViewport)
{
    CDependencyObject* pParentDO = pUIElement;

    ASSERT(pViewport);

    while (pParentDO)
    {
        pParentDO = pParentDO->GetParentInternal();
        if (pParentDO == pViewport->GetManipulatedElementNoRef())
        {
            return true;
        }
    }
    return false;
}

// Reconfigure a Secondary Content Relationship taking into account primary content's location change
_Check_return_ HRESULT CInputServices::UpdateSecondaryContentRelationshipOffsets(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ CDMViewport* pViewport,
    _In_ CDMContent* pContent,
    _In_ bool fIsForClip,
    _In_ XFLOAT contentOffsetX,
    _In_ XFLOAT contentOffsetY)
{
    HRESULT hr = S_OK;
    XUINT32 cDefinitions = 0;
    CParametricCurveDefinition *pDefinitions = NULL;

    CSecondaryContentRelationship *pSecondaryContentRelationshipNoRef = pContent->GetSecondaryContentRelationship();
    if (pSecondaryContentRelationshipNoRef)
    {
        IFC(pSecondaryContentRelationshipNoRef->GetParametricCurveDefinitions(&cDefinitions, &pDefinitions));
        xref_ptr<CUIElement> spSecondaryContent = pSecondaryContentRelationshipNoRef->GetSecondaryContent();
        if (spSecondaryContent->HasSharedManipulationTransform(!!fIsForClip))
        {
            // Synchronize the change of this sticky header/clip curve
            // See extensive banner comments on PrepareSecondaryContentRelationshipForCurveUpdate().
            IFC(PrepareSecondaryContentRelationshipForCurveUpdate(pSecondaryContentRelationshipNoRef));

            xref_ptr<CUIElement> spDMContainer = pSecondaryContentRelationshipNoRef->GetPrimaryContent();
            xref_ptr<CDependencyObject> spDManipDO;
            xref_ptr<CUIElement> spManipulatableElement;
            IFC(FxCallbacks::UIElement_GetDManipElement(spDMContainer, spDManipDO.ReleaseAndGetAddressOf()));
            spManipulatableElement = do_pointer_cast<CUIElement>(spDManipDO);

            // Now that we've surgically transferred the old content/transform to our deferral object,
            // call back into NotifySecondaryContentAdded, it will create new content in its place.
            IFC(NotifySecondaryContentAdded(
                spDMContainer,
                spManipulatableElement,
                spSecondaryContent,
                XcpDMContentTypeDescendant,
                cDefinitions,
                pDefinitions,
                pSecondaryContentRelationshipNoRef));
        }
        else
        {
            if (fIsForClip)
            {
                IFC(pDirectManipulationService->AddSecondaryClipContent(pViewport, pContent, cDefinitions, pDefinitions, contentOffsetX, contentOffsetY));
            }
            else
            {
                IFC(pDirectManipulationService->AddSecondaryContent(pViewport, pContent, cDefinitions, pDefinitions, contentOffsetX, contentOffsetY));
            }
        }
    }

Cleanup:
    for (XUINT32 i = 0; i < cDefinitions; i++)
    {
        delete[] pDefinitions[i].m_pSegments;
    }
    delete[] pDefinitions;

    RRETURN(hr);
}

// Reconfigure all Secondary Content Relationships of type Custom and Descendant
// if the primary content's location has changed
_Check_return_ HRESULT
CInputServices::UpdateSecondaryContentsOffsets(
    _In_ CUIElement* dmContainer,
    _In_ CUIElement* manipulatedElement,
    _In_ CDMViewport* viewport)
{
    xref_ptr<IPALDirectManipulationService> directManipulationService;
    xref_ptr<CDMContent> content;
    bool enableViewport = false;
    XDMViewportStatus status = XcpDMViewportBuilding;
    XFLOAT contentOffsetX = 0.0f;
    XFLOAT contentOffsetY = 0.0f;

    viewport->GetContentOffsets(contentOffsetX, contentOffsetY);

#ifdef DM_DEBUG
    if (m_fIsDMVerboseInfoTracingEnabled)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | XCP_TRACE_VERBOSE | DMIMv_DBG) /*traceType*/,
            L"DMIMv[0x%p]: Need to update SecondaryContentRelationships: contentOffsetX=%f contentOffsetY=%f.",
            this, contentOffsetX, contentOffsetY));
    }
#endif // DM_DEBUG

    IFC_RETURN(GetDMService(dmContainer, directManipulationService.ReleaseAndGetAddressOf()));
    if (directManipulationService)
    {
        // In order for the parametric curves to be fully taken into account by DManip,
        // the secondary contents need to be added while the viewport is active or disabled.
        IFC_RETURN(directManipulationService->GetViewportStatus(viewport, status));
        if (status == XcpDMViewportEnabled || status == XcpDMViewportReady || status == XcpDMViewportBuilding)
        {
            if (status == XcpDMViewportEnabled)
            {
                // Temporarily disable the viewport so the EnableViewport call below has an effect.
                IFC_RETURN(DisableViewport(dmContainer, manipulatedElement));
            }
            enableViewport = true;
        }

        for (XUINT32 iContent = 0; iContent < viewport->GetContentsCount(); iContent++)
        {
            IFC_RETURN(viewport->GetContent(iContent, content.ReleaseAndGetAddressOf()));
            if (content && ((content->GetContentType() == XcpDMContentTypeCustom || content->GetContentType() == XcpDMContentTypeDescendant)))
            {
                IFC_RETURN(UpdateSecondaryContentRelationshipOffsets(directManipulationService, viewport, content, FALSE /* fIsForClip */, contentOffsetX, contentOffsetY));
            }
        }

        for (XUINT32 iClipContent = 0; iClipContent < viewport->GetClipContentsCount(); iClipContent++)
        {
            IFC_RETURN(viewport->GetClipContent(iClipContent, content.ReleaseAndGetAddressOf()));
            if (content && ((content->GetContentType() == XcpDMContentTypeCustom || content->GetContentType() == XcpDMContentTypeDescendant)))
            {
                IFC_RETURN(UpdateSecondaryContentRelationshipOffsets(directManipulationService, viewport, content, TRUE /* fIsForClip */, contentOffsetX, contentOffsetY));
            }
        }

        // Enable the viewport, in case it is inactive, after the secondary content was declared and set up.
        if (enableViewport)
        {
            ASSERT(directManipulationService);
            IFC_RETURN(EnableViewport(directManipulationService, viewport));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CInputServices::GetSecondaryContentTransform(
    _In_ CUIElement* pDMContainer,
    _In_ CUIElement* pSecondaryContent,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;
    CDMViewport* pViewport = NULL;
    IPALDirectManipulationService* pDirectManipulationService = NULL;

    IFC(GetViewportForContentElement(pSecondaryContent, &pViewport, &pContent));

    if (pViewport && pContent)
    {
        IFC(GetDMService(pDMContainer, &pDirectManipulationService));
        if (pDirectManipulationService)
        {
            IFC(pDirectManipulationService->GetSecondaryContentTransform(
                pViewport,
                pContent,
                pContent->GetContentType(),
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));
        }
    }

Cleanup:
    ReleaseInterface(pContent);
    ReleaseInterface(pViewport);
    ReleaseInterface(pDirectManipulationService);
    RRETURN(hr);
}

void CInputServices::GetViewportInteraction(
    _In_ CUIElement* element,
    _In_ IUnknown* compositor,
    _Outptr_ IUnknown** interaction)
{
    CreateViewportInteractionForManipulatableElement(element, compositor, interaction);
}

void CInputServices::CreateViewportInteractionForManipulatableElement(
    _In_ CUIElement* manipulatedElement,
    _In_ IUnknown* compositor,
    _Outptr_ IUnknown** interaction)
{
    xref_ptr<CDMViewport> viewport;
    xref_ptr<IPALDirectManipulationService> dmService;

    TraceCreateViewportInteraction(manipulatedElement, false);
    IFCFAILFAST(GetViewport(nullptr, manipulatedElement, viewport.ReleaseAndGetAddressOf()));
    IFCFAILFAST(GetDMService(viewport->GetDMContainerNoRef(), dmService.ReleaseAndGetAddressOf()));
    IFCFAILFAST(dmService->CreateViewportInteraction(compositor, viewport, interaction));
    viewport->SetHasViewportInteraction(true);
}

XPointerInputType ToXPointerInputType(mui::PointerDeviceType pointerDeviceType)
{
    switch (pointerDeviceType)
    {
        case mui::PointerDeviceType_Touch: return XcpPointerInputTypeTouch;
        case mui::PointerDeviceType_Pen: return XcpPointerInputTypePen;
        case mui::PointerDeviceType_Mouse: return XcpPointerInputTypeMouse;
        // Two-finger scrolling on the Precision Touch Pad gives a device type that is "Pen | Mouse", treat as mouse.
        #pragma warning(suppress : 4063)
        case mui::PointerDeviceType_Pen | mui::PointerDeviceType_Mouse: return XcpPointerInputTypeMouse;
    }

    // We don't expect any other types, but if we get one, treat as mouse.
    // (When XAML was part of Windows, there were other private values that could be
    // returned)
    ASSERT(FALSE);
    return XcpPointerInputTypeMouse;
}

// Fill in the given PointerInfo struct with information from the given IPointerPoint.
// Intended to be called in XamlOneCoreTransforms mode or when using XamlIslandRoots.
_Check_return_ HRESULT
CInputServices::GetPointerInfoFromPointerPoint(
    _In_ ixp::IPointerPoint* pointerPoint,
    _Out_ PointerInfo* pointerInfoResult)
{
    // In strict mode, use PointerPoint APIs instead of win32 APIs
    // Since XamlIslandRoots use CoreComponentInput, we keep them on the OneCore input path
    // as well.
    mui::PointerDeviceType pointerDeviceType;
    wrl::ComPtr<ixp::IPointerPointProperties> pointerPointProperties;

    IFC_RETURN(pointerPoint->get_Properties(&pointerPointProperties));
    IFC_RETURN(pointerPoint->get_PointerDeviceType(&pointerDeviceType));

    pointerInfoResult->m_pointerInputType = ToXPointerInputType(pointerDeviceType);

    IFC_RETURN(pointerPoint->get_PointerId(&pointerInfoResult->m_pointerId));
    IFC_RETURN(pointerPoint->get_FrameId(&pointerInfoResult->m_frameId));

    UINT64 uint64Value = 0;
    IFC_RETURN(pointerPoint->get_Timestamp(&uint64Value));
    pointerInfoResult->m_timeStamp = static_cast<XUINT32>(uint64Value);

    boolean booleanValue = false;

    IFC_RETURN(pointerPoint->get_IsInContact(&booleanValue));
    pointerInfoResult->m_bInContact = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsInRange(&booleanValue));
    pointerInfoResult->m_bInRange = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsPrimary(&booleanValue));
    pointerInfoResult->m_bInPrimary = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsCanceled(&booleanValue));
    pointerInfoResult->m_bCanceled = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsLeftButtonPressed(&booleanValue));
    pointerInfoResult->m_bLeftButtonPressed = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsRightButtonPressed(&booleanValue));
    pointerInfoResult->m_bRightButtonPressed = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsMiddleButtonPressed(&booleanValue));
    pointerInfoResult->m_bMiddleButtonPressed = !!booleanValue;

    IFC_RETURN(pointerPointProperties->get_IsBarrelButtonPressed(&booleanValue));
    pointerInfoResult->m_bBarrelButtonPressed = !!booleanValue;

    wf::Point position = {};
    IFC_RETURN(pointerPoint->get_Position(&position));

    pointerInfoResult->m_pointerLocation.x = position.X;
    pointerInfoResult->m_pointerLocation.y = position.Y;

    return S_OK;
}

/*static*/ UINT CInputServices::GetMessageFromPointerCaptureLostArgs(_In_ ixp::IPointerEventArgs* args)
{
    // PointerCaptureLost fires in response to both a WM_CAPTURECHANGED and a WM_POINTERCAPTURECHANGED.  XAML
    // does different things for each message, so we tease apart which message we're getting before we inject it over
    // to JupiterWindow.
    // Note the WM_CAPTURECHANGED is fired synchronously during a call to ::ReleaseCapture.  This is a message
    // XAML historically has not handled -- we inject it to make the island scenario match the XamlApp scenario
    // as closely as possible, but note that CJupiterControl::HandlePointerMessage drops the message.
    // WM_POINTERCAPTURECHANGED is fired when DManip takes over the input stream for a touch gesture.
    // We can know which message originated this event by checking the input device type.

    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(args->get_CurrentPoint(&pointerPoint));

    mui::PointerDeviceType pointerDeviceType = {};
    IFCFAILFAST(pointerPoint->get_PointerDeviceType(&pointerDeviceType));

    return pointerDeviceType == mui::PointerDeviceType_Mouse ? WM_CAPTURECHANGED : WM_POINTERCAPTURECHANGED;
}



_Check_return_ HRESULT
CDragDropState::CacheInitialState(
    _In_ DragMsg* dragMsg,
    _In_ XPOINTF pointDrag,
    _In_opt_ CDependencyObject* oldDragDO,
    _In_opt_ CDependencyObject* newDragDO,
    _In_ IInspectable* winRtDragInfo,
    _In_opt_ IInspectable* dragDropAsyncOperation,
    _In_ DirectUI::DataPackageOperation acceptedOperation)
{
    // this method should never be called when some drag state is already cached.
    ASSERT(!m_dragDO);

    switch (dragMsg->m_msgID)
    {
    case XCP_DRAGENTER:
        m_dragType = DirectUI::DragDropMessageType::DragEnter;
        break;
    case XCP_DRAGOVER:
        m_dragType = DirectUI::DragDropMessageType::DragOver;
        break;
    case XCP_DRAGLEAVE:
        m_dragType = DirectUI::DragDropMessageType::DragLeave;
        break;
    case XCP_DROP:
        m_dragType = DirectUI::DragDropMessageType::Drop;
        break;
    default:
        return E_UNEXPECTED;
    }

    // Init the state
    m_handled = FALSE;
    m_acceptedOperation = acceptedOperation;
    m_dragPoint = pointDrag;
    m_winRtDragInfo = winRtDragInfo;
    m_dragDropAsyncOperation = dragDropAsyncOperation;
    m_dragDO = newDragDO;

    if (newDragDO != oldDragDO)
    {
        m_nextDragDO = oldDragDO;
        m_nextDragType = DirectUI::DragDropMessageType::DragLeave;

        // Pass 1: Set the leave and dirty bits on all elements in the parent chain
        CDependencyObject *pVisual = nullptr;
        for (auto it = std::begin(m_enterStack); it != std::end(m_enterStack); ++it)
        {
            pVisual = (*it).lock();
            if (pVisual != nullptr)
            {
                pVisual->SetDragInputNodeDirty(TRUE);
                pVisual->SetDragEnter(FALSE);
            }
        }
        ResetEnterStack();

        // Pass 2 set the enter on the elements until we encounter the first dirty bit
        pVisual = newDragDO;
        while (pVisual)
        {
            pVisual->SetDragEnter(TRUE);
            pVisual = pVisual->GetParentInternal();
        }

        m_nextDragDO = oldDragDO;

        // All DragEnter events share a CDragEventArgs instance, create one here
        IFC_RETURN(CDragEventArgs::Create(m_coreServicesNoRef, m_dragEnterEventArgs.ReleaseAndGetAddressOf(), m_winRtDragInfo.get(), m_dragDropAsyncOperation.get()));
    }
    else
    {   // if newDragDO == oldDragDO and neither is null,
        // no need to raise DragEnter or DragLeave
        ASSERT(newDragDO);
        m_nextDragDO = newDragDO;
        ASSERT((m_dragType == DirectUI::DragDropMessageType::DragOver) ||
            (m_dragType == DirectUI::DragDropMessageType::Drop));
        m_nextDragType = m_dragType;
    }

    IFC_RETURN(CDragEventArgs::Create(m_coreServicesNoRef, m_dragEventArgs.ReleaseAndGetAddressOf(), m_winRtDragInfo.get(), m_dragDropAsyncOperation.get()));
    m_dragEventArgs->SetGlobalPoint(m_dragPoint);
    // Peg the args object to avoid re-creating the managed peer
    // for each object we raise an event on.
    IFC_RETURN(m_dragEventArgs->PegManagedPeerForRoutedEventArgs());

    return S_OK;
}

void CInputServices::SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow)
{
    m_hCoreWindow = static_cast<XHANDLE>(pCoreWindow);

    if (m_pCoreService->IsTSF3Enabled())
    {
        m_textInputProducerHelper.Init(pCoreWindow);
    }

    CContentRoot* contentRoot = m_pCoreService->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    FocusObserver* focusObserver = contentRoot->GetFocusManagerNoRef()->GetFocusObserverNoRef();

    IFCFAILFAST(focusObserver->Init(pCoreWindow));
}

wuc::ICoreWindow* CInputServices::GetCoreWindow() const
{
    return static_cast<wuc::ICoreWindow*>(m_hCoreWindow);
}

_Check_return_ HRESULT
CDragDropState::RaiseEvents(_Out_opt_ DirectUI::DataPackageOperation* acceptedOperation)
{
    switch (m_nextDragType)
    {
    case DirectUI::DragDropMessageType::DragLeave:
        IFC_RETURN(RaiseDragLeaveEvents());
        break;
    case DirectUI::DragDropMessageType::DragEnter:
        IFC_RETURN(RaiseDragEnterEvents());
        break;
    case DirectUI::DragDropMessageType::DragOver:
    case DirectUI::DragDropMessageType::Drop:
        IFC_RETURN(RaiseDragOverOrDropEvents());
        break;
    default:
        return E_FAIL;
    }

    if (acceptedOperation != nullptr)
    {
        *acceptedOperation = m_acceptedOperation;
    }

    return S_OK;
}

_Check_return_ HRESULT
CDragDropState::RaiseDragLeaveEvents()
{
    bool isDeferred = false;

    while (m_nextDragDO && !m_nextDragDO->HasDragEnter())
    {
        m_nextDragDO->SetDragInputNodeDirty(FALSE);

        // m_dragDropAsyncOperation might be null when we leave the XAML Window, in which case we don't
        // really care as the DragVisual won't be ours anymore
        if (m_dragDropAsyncOperation.get())
        {
            // If we are Leaving the UIElement which has set the custom visual, we'll have to clear the visual
            if (FxCallbacks::HasDragDrop_CheckIfCustomVisualShouldBeCleared())
            {
                IFC_RETURN(FxCallbacks::DragDrop_CheckIfCustomVisualShouldBeCleared(m_nextDragDO.get()));
            }

            // If we are Leaving the UIElement which has set AcceptedOperation, we'll have to reset it before returning to core
            if (FxCallbacks::HasRaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset())
            {
                IFC_RETURN(FxCallbacks::RaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset(m_dragDropAsyncOperation.get(), m_nextDragDO.get()));
            }
        }

        if (static_sp_cast<CUIElement>(m_nextDragDO)->IsHitTestVisible() && !m_handled)
        {
            xref_ptr<CDragEventArgs> dragLeaveArgs;
            IFC_RETURN(CDragEventArgs::Create(m_coreServicesNoRef, dragLeaveArgs.ReleaseAndGetAddressOf(), m_winRtDragInfo.get(), m_dragDropAsyncOperation.get()));
            IFC_RETURN(dragLeaveArgs->put_Source(m_nextDragDO));
            dragLeaveArgs->UpdateAcceptedOperation(m_acceptedOperation);
            dragLeaveArgs->SetDragDropEventType(DirectUI::DragDropMessageType::DragLeave);
            dragLeaveArgs->SetGlobalPoint(m_dragPoint);
            dragLeaveArgs->m_bAllowDataAccess = FALSE;

            m_eventManager->Raise(
                EventHandle(KnownEventIndex::UIElement_DragLeave),
                TRUE,
                m_nextDragDO.get(),
                dragLeaveArgs.get(),
                TRUE /* raiseAsync */,
                TRUE /* fInputEvent */);

            IFC_RETURN(dragLeaveArgs->get_Handled(&m_handled));
            IFC_RETURN(dragLeaveArgs->get_AcceptedOperation(&m_acceptedOperation));

            IFC_RETURN(dragLeaveArgs->GetIsDeferred(m_coreServicesNoRef, &isDeferred));
        }
        m_nextDragDO = m_nextDragDO->GetParentInternal();

        if (isDeferred)
        {
            return S_OK;
        }
    }

    m_nextDragType = DirectUI::DragDropMessageType::DragEnter;
    m_nextDragDO = m_dragDO;
    m_handled = FALSE;

    ResetEnterStack();
    IFC_RETURN(RaiseDragEnterEvents());

    return S_OK;
}

_Check_return_ HRESULT
CDragDropState::RaiseDragEnterEvents()
{
    m_dragEnterEventArgs->UpdateAcceptedOperation(m_acceptedOperation);
    // Pass 4: Now walk up the tree
    //          A) firing enter events until we hit the first guy that has dirty bit set
    //             and from there reset the dirty bit
    //          B) Finding the lowest element that has the dragenter set.
    while (m_nextDragDO)
    {
        bool isDeferred = false;

        m_enterStack.push_back(xref::get_weakref(m_nextDragDO));

        if (m_nextDragDO->HasDragEnter()
            && !m_nextDragDO->IsDragInputNodeDirty()
            && !m_handled)
        {
            if (static_sp_cast<CUIElement>(m_nextDragDO)->IsHitTestVisible())
            {
                m_dragEnterEventArgs->SetGlobalPoint(m_dragPoint);
                m_dragEnterEventArgs->m_bAllowDataAccess = FALSE;
                m_dragEnterEventArgs->SetDragDropEventType(DirectUI::DragDropMessageType::DragEnter);

                // We need this to keep track of the UIElement that sets
                // a custom visual. Only applies when Core DragDrop API
                // is used.
                IFC_RETURN(m_dragEnterEventArgs->put_Source(m_nextDragDO));

                m_eventManager->Raise(
                    EventHandle(KnownEventIndex::UIElement_DragEnter),
                    TRUE,
                    m_nextDragDO.get(),
                    m_dragEnterEventArgs.get(),
                    TRUE /* raiseAsync */,
                    TRUE /* fInputEvent */);

                IFC_RETURN(m_dragEnterEventArgs->get_Handled(&m_handled));
                IFC_RETURN(m_dragEnterEventArgs->get_AcceptedOperation(&m_acceptedOperation));
            }
        }
        else if (m_nextDragDO->IsDragInputNodeDirty())
        {
            m_nextDragDO->SetDragInputNodeDirty(FALSE);
        }

        m_nextDragDO = m_nextDragDO->GetParentInternal();

        IFC_RETURN(m_dragEnterEventArgs->GetIsDeferred(m_coreServicesNoRef, &isDeferred));
        if (isDeferred)
        {
            return S_OK;
        }
    }

    if ((m_dragType == DirectUI::DragDropMessageType::DragOver) ||
        (m_dragType == DirectUI::DragDropMessageType::Drop))
    {
        m_handled = FALSE;
        m_nextDragType = m_dragType;
        m_nextDragDO = m_dragDO;

        m_dragEventArgs->SetDragDropEventType(m_nextDragType);
        m_dragEventArgs->m_bAllowDataAccess = (m_nextDragType == DirectUI::DragDropMessageType::Drop);

        IFC_RETURN(RaiseDragOverOrDropEvents());
    }
    else
    {
        IFC_RETURN(ClearCache(m_dragType == DirectUI::DragDropMessageType::DragLeave /*clearDragEnterEventArgsToo*/));
    }

    return S_OK;
}

_Check_return_ HRESULT
CDragDropState::RaiseDragOverOrDropEvents()
{
    m_dragEventArgs->UpdateAcceptedOperation(m_acceptedOperation);

    ASSERT((m_nextDragType == DirectUI::DragDropMessageType::DragOver) ||
        (m_nextDragType == DirectUI::DragDropMessageType::Drop));

    while (m_nextDragDO && !m_handled)
    {
        CUIElement *pUIE = do_pointer_cast<CUIElement>(m_nextDragDO);
        bool isDeferred = false;

        if (pUIE && pUIE->IsHitTestVisible())
        {
            // We need this to keep track of the UIElement (source) that sets
            // a custom visual. Only applies when Core DragDrop API
            // is used.
            IFC_RETURN(m_dragEventArgs->put_Source(m_nextDragDO.get()));

            m_eventManager->Raise(
                EventHandle((m_nextDragType == DirectUI::DragDropMessageType::DragOver) ? KnownEventIndex::UIElement_DragOver : KnownEventIndex::UIElement_Drop),
                TRUE, m_nextDragDO.get(),
                m_dragEventArgs.get(),
                TRUE /* raiseAsync */,
                TRUE);

            IFC_RETURN(m_dragEventArgs->get_Handled(&m_handled));
            IFC_RETURN(m_dragEventArgs->get_AcceptedOperation(&m_acceptedOperation));
        }

        m_nextDragDO = m_nextDragDO->GetParentInternal();

        IFC_RETURN(m_dragEventArgs->GetIsDeferred(m_coreServicesNoRef, &isDeferred));
        if (isDeferred)
        {
            return S_OK;
        }
    }

    IFC_RETURN(ClearCache(m_nextDragType == DirectUI::DragDropMessageType::Drop /*clearDragEnterEventArgsToo*/));

    return S_OK;
}

_Check_return_ HRESULT
CDragDropState::ClearCache(bool clearDragEnterEventArgsToo)
{
    m_dragDO = nullptr;
    m_nextDragDO = nullptr;

    if (m_dragEventArgs != nullptr)
    {
        m_dragEventArgs->UnpegManagedPeerForRoutedEventArgs();
        m_dragEventArgs = nullptr;
    }

    if (clearDragEnterEventArgsToo)
    {
        m_dragEnterEventArgs = nullptr;
    }

    return S_OK;
}

void CDragDropState::ResetEnterStack()
{
    m_enterStack.clear();
}
