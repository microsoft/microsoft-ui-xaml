// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "FocusProperties.h"
#include "FocusedElementRemovedEventArgs.h"
#include "GettingFocusEventArgs.h"
#include "LosingFocusEventArgs.h"
#include <FocusSelection.h>
#include <FocusRectManager.h>
#include "ErrorHelper.h"
#include "NoFocusCandidateFoundEventArgs.h"
#include "FocusableHelper.h"
#include <CValueBoxer.h>
#include <FocusManagerLostFocusEventArgs.h>
#include <FocusManagerGotFocusEventArgs.h>

#include "InitialFocusSIPSuspender.h"
#include "FocusLockOverrideGuard.h"

#define E_FOCUS_ASYNCOP_INPROGRESS 64L

using namespace DirectUI;

using namespace Focus;

using namespace RuntimeFeatureBehavior;

CFocusManager::CFocusManager(_In_ CCoreServices *pCoreService, _In_ CContentRoot& contentRoot) :
    m_pFocusedElement(nullptr),
    m_bPluginFocused(false),
    m_bCanTabOutOfPlugin(FALSE),
    m_isPrevFocusTextControl(FALSE),
    m_realFocusStateForFocusedElement(DirectUI::FocusState::Unfocused),
    m_contentRoot(contentRoot)
{
    XCP_WEAK(&m_pCoreService);
    m_pCoreService = static_cast<CCoreServices*>(pCoreService);

    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableAutoFocusOverride))
    {
        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::AutoFocus_Distance))
        {
            const DWORD primaryAxisDistanceWeight = runtimeEnabledFeatureDetector->GetFeatureValue(RuntimeEnabledFeature::AutoFocus_Distance);
            m_xyFocus.SetPrimaryAxisDistanceWeight(primaryAxisDistanceWeight);
        }

        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::AutoFocus_SecondaryDistance))
        {
            const DWORD secondaryAxisDistanceWeight = runtimeEnabledFeatureDetector->GetFeatureValue(RuntimeEnabledFeature::AutoFocus_SecondaryDistance);
            m_xyFocus.SetSecondaryAxisDistanceWeight(secondaryAxisDistanceWeight);
        }

        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::AutoFocus_ManifoldShadow))
        {
            const DWORD percentInManifoldShadowWeight = runtimeEnabledFeatureDetector->GetFeatureValue(RuntimeEnabledFeature::AutoFocus_ManifoldShadow);
            m_xyFocus.SetPercentInManifoldShadowWeight(percentInManifoldShadowWeight);
        }

        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::AutoFocus_Shadow))
        {
            const DWORD percentInShadowWeight = runtimeEnabledFeatureDetector->GetFeatureValue(RuntimeEnabledFeature::AutoFocus_Shadow);
            m_xyFocus.SetPercentInShadowWeight(percentInShadowWeight);
        }
    }
}

CFocusManager::~CFocusManager()
{
    ReleaseInterface(m_pFocusedElement);
    m_pCoreService = nullptr;
}

XUINT32 CFocusManager::AddRef()
{
    return m_contentRoot.AddRef();
}

XUINT32 CFocusManager::Release()
{
    return m_contentRoot.Release();
}

void CFocusManager::SetFocusObserver(_In_ std::unique_ptr<FocusObserver> focusObserver)
{
    m_focusObserver = std::move(focusObserver);
}

xref_ptr<CAutomationPeer> CFocusManager::GetFocusedAutomationPeer()
{
    if (!m_pFocusedAutomationPeer && m_pFocusedElement != nullptr)
    {
        m_pFocusedAutomationPeer = xref::get_weakref(m_pFocusedElement->OnCreateAutomationPeer());
    }

    return m_pFocusedAutomationPeer.lock();
}

// Returns the current focused element without adding a reference to it.
CDependencyObject*
CFocusManager::GetFocusedElementNoRef() const
{
    return m_pFocusedElement;
}

const FocusMovementResult
CFocusManager::SetFocusedElement(_In_ const FocusMovement& movement)
{
    CDependencyObject* pFocusedElement = movement.GetTarget();

    if (pFocusedElement == nullptr) { return FocusMovementResult(); }

    // This is the only occurrence where ignoreOffScreenPosition==True is used.
    // For compatibility reasons, an element that is still placed in a ModernCollectionBasePanel's garbage section (because it has not been arranged yet)
    // can be focused through the UpdadateFocus call below. This situation can occur when app code invokes UIElement.Focus(FocusState) inside a 
    // ListViewBase.ContainerContentChanging event handler.
    if (!IsFocusable(pFocusedElement, true /*ignoreOffScreenPosition*/))
    {
        pFocusedElement = GetFirstFocusableElement(pFocusedElement);

        if (pFocusedElement == nullptr || !IsFocusable(pFocusedElement)) { return FocusMovementResult(); }
    }

    const FocusNavigationDirection navigationDirection = movement.GetDirection();
    ASSERT(!movement.isProcessingTab || (navigationDirection == DirectUI::FocusNavigationDirection::Next || navigationDirection == DirectUI::FocusNavigationDirection::Previous));

    return UpdateFocus(FocusMovement(pFocusedElement, movement));
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::ClearFocus
//
//  Synopsis:
//      Clear the focus.
//
//------------------------------------------------------------------------

void
CFocusManager::ClearFocus()
{
    if (m_contentRoot.IsShuttingDown())
    {
        // UpdateFocus doesn't do anything when resetting visual tree in the
        // interest of shut-down time.  Here's the super-abbreviated subset
        // of UpdateFocus that we still need to run.

        // Clear the focused control
        ReleaseInterface(m_pFocusedElement);
        m_realFocusStateForFocusedElement = DirectUI::FocusState::Unfocused;
    }
    else
    {
        const FocusMovementResult result = UpdateFocus(FocusMovement(nullptr, FocusNavigationDirection::None, FocusState::Unfocused));
        VERIFYHR(result.GetHResult());
    }
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::ReleaseFocusRectManagerResources
//
//  Synopsis:
//      Releases resources held by FocusManager's FocusRectManager. These
//      elements are automatically created on CFrameworkManager::UpdateFocus()
//      and must be released before core releases its main render target on
//      shutdown. Exposed by fixing core leak RS1 bug #7300521.
//
//------------------------------------------------------------------------

void
CFocusManager::ReleaseFocusRectManagerResources()
{
    // Releases references on CUIElements held by CFocusRectManager
    const bool isDeviceLost = false, cleanupDComp = false, clearPCData = true;
    m_focusRectManager.ReleaseResources(isDeviceLost, cleanupDComp, clearPCData);
}

void
CFocusManager::CleanupDeviceRelatedResources(bool cleanupDComp)
{
    const bool isDeviceLost = true, clearPCData = false;
    m_focusRectManager.ReleaseResources(isDeviceLost, cleanupDComp, clearPCData);
}

bool CFocusManager::FocusedElementIsBehindFullWindowMediaRoot() const
{
    const auto visualTree = m_contentRoot.GetVisualTreeNoRef();
    return visualTree != nullptr && visualTree->IsBehindFullWindowMediaRoot(m_pFocusedElement);
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetFirstFocusableElementFromRoot
//
//  Synopsis:
//      Return the first focusable element on the root
//
//------------------------------------------------------------------------
CDependencyObject*
CFocusManager::GetFirstFocusableElementFromRoot(_In_ bool bReverse)
{
    CDependencyObject *pFocusableElement = nullptr;

    CDependencyObject *pRoot = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
    if (pRoot)
    {
        if (!bReverse)
        {
            pFocusableElement = GetFirstFocusableElement(pRoot, pFocusableElement);
        }
        else
        {
            pFocusableElement = GetLastFocusableElement(pRoot, pFocusableElement);
        }
    }

    return pFocusableElement;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetFirstFocusableElement(Private)
//
//  Synopsis:
//      Return the first focusable element from the specified visual tree
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetFirstFocusableElement(_In_ CDependencyObject* pSearchStart, _In_ CDependencyObject *pFirstFocus)
{

    pFirstFocus = GetFirstFocusableElementInternal(pSearchStart, pFirstFocus);

    // GetFirstFocusableElementInternal can return the TabStop element that might not be a focusable
    // container like as UserControl(IsTabStop=FALSE) even though it have a focusable child.
    // If the pFirstFocus is not focusable and has a focusable child, the below code will find
    // a first focusable child
    // Keep finding the first focusable child

    if (pFirstFocus && !IsFocusable(pFirstFocus) && CanHaveFocusableChildren(pFirstFocus))
    {
        pFirstFocus = GetFirstFocusableElement(pFirstFocus, NULL);
    }

    return pFirstFocus;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetFirstFocusableElementInternal(Private)
//
//  Synopsis:
//      Return the first focusable element from the specified visual tree
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetFirstFocusableElementInternal(
    _In_ CDependencyObject* pSearchStart,
    _In_ CDependencyObject *pFirstFocus)
{
    CDependencyObject *pFirstFocusableFromCallback = NULL;
    bool useFirstFocusableFromCallback = false;

    // Ask UIElement for a first focusable suggestion(this is to solve ListViewBase focus issues).
    IGNOREHR(FxCallbacks::UIElement_GetFirstFocusableElement(static_cast<CDependencyObject *>(pSearchStart), &pFirstFocusableFromCallback));
    if (pFirstFocusableFromCallback != NULL)
    {
        useFirstFocusableFromCallback = IsFocusable(pFirstFocusableFromCallback) || CanHaveFocusableChildren(pFirstFocusableFromCallback);
    }

    if (useFirstFocusableFromCallback)
    {
        if (pFirstFocus == NULL ||
            (GetTabIndex(pFirstFocusableFromCallback) < GetTabIndex(pFirstFocus)))
        {
            pFirstFocus = static_cast<CDependencyObject *>(pFirstFocusableFromCallback);
        }
    }
    else
    {
        CDependencyObject* childNoRef = nullptr;

        FocusProperties::FocusChildrenIteratorWrapper iterator = FocusProperties::GetFocusChildrenInTabOrderIterator(pSearchStart);
        while (iterator.TryMoveNext(&childNoRef))
        {
            if (childNoRef && IsVisible(childNoRef))
            {
                bool bHaveFocusableChild = CanHaveFocusableChildren(childNoRef);

                if (IsPotentialTabStop(childNoRef))
                {
                    if (pFirstFocus == nullptr && (IsFocusable(childNoRef) || bHaveFocusableChild))
                    {
                        pFirstFocus = childNoRef;
                    }

                    if (IsFocusable(childNoRef) || bHaveFocusableChild)
                    {
                        if (GetTabIndex(childNoRef) < GetTabIndex(pFirstFocus))
                        {
                            pFirstFocus = childNoRef;
                        }
                    }
                }
                else if (bHaveFocusableChild)
                {
                    pFirstFocus = GetFirstFocusableElementInternal(childNoRef, pFirstFocus);
                }
            }
        }
    }

    ReleaseInterface(pFirstFocusableFromCallback);
    return pFirstFocus;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetLastFocusableElement(Private)
//
//  Synopsis:
//      Return the last focusable element from the specified visual tree
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetLastFocusableElement(_In_ CDependencyObject* pSearchStart, _In_ CDependencyObject *pLastFocus)
{
    pLastFocus = GetLastFocusableElementInternal(pSearchStart, pLastFocus);

    // GetLastFocusableElementInternal can return the TabStop element that might not be a focusable
    // container like as UserControl(IsTabStop=FALSE) even though it have a focusable child.
    // If the pLastFocus is not focusable and has a focusable child, the below code will find
    // a last focusable child
    if (pLastFocus && CanHaveFocusableChildren(pLastFocus))
    {
        pLastFocus = GetLastFocusableElement(pLastFocus, NULL);
    }

    return pLastFocus;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetLastFocusableElementInternal(Private)
//
//  Synopsis:
//      Return the last focusable element from the specified visual tree
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetLastFocusableElementInternal(
    _In_ CDependencyObject* pSearchStart,
    _In_ CDependencyObject *pLastFocus)
{
    CDependencyObject *pLastFocusableFromCallback = NULL;
    bool useLastFocusableFromCallback = false;

    // Ask UIElement for a first focusable suggestion(this is to solve ListViewBase focus issues).
    IGNOREHR(FxCallbacks::UIElement_GetLastFocusableElement(static_cast<CDependencyObject *>(pSearchStart), &pLastFocusableFromCallback));
    if (pLastFocusableFromCallback != NULL)
    {
        useLastFocusableFromCallback = IsFocusable(pLastFocusableFromCallback) || CanHaveFocusableChildren(pLastFocusableFromCallback);
    }

    if (useLastFocusableFromCallback)
    {
        if (pLastFocus == NULL ||
            (GetTabIndex(pLastFocusableFromCallback) >= GetTabIndex(pLastFocus)))
        {
            pLastFocus = static_cast<CDependencyObject *>(pLastFocusableFromCallback);
        }
    }
    else
    {
        CDependencyObject* childNoRef = nullptr;

        FocusProperties::FocusChildrenIteratorWrapper iterator = FocusProperties::GetFocusChildrenInTabOrderIterator(pSearchStart);
        while (iterator.TryMoveNext(&childNoRef))
        {
            if (childNoRef && IsVisible(childNoRef))
            {
                bool bHaveFocusableChild = CanHaveFocusableChildren(childNoRef);

                if (IsPotentialTabStop(childNoRef))
                {
                    if (pLastFocus == nullptr && (IsFocusable(childNoRef) || bHaveFocusableChild))
                    {
                        pLastFocus = childNoRef;
                    }

                    if (IsFocusable(childNoRef) || bHaveFocusableChild)
                    {
                        if (GetTabIndex(childNoRef) >= GetTabIndex(pLastFocus))
                        {
                            pLastFocus = childNoRef;
                        }
                    }
                }
                else if (bHaveFocusableChild)
                {
                    pLastFocus = GetLastFocusableElementInternal(childNoRef, pLastFocus);
                }
            }
        }
    }

    ReleaseInterface(pLastFocusableFromCallback);
    return pLastFocus;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::CanProcessTabStop
//
//  Synopsis:
//      Return true if TabStop can process by us.
//
//------------------------------------------------------------------------

bool
CFocusManager::CanProcessTabStop(_In_ bool reverse)
{
    bool canProcessTab = true;
    bool isFocusOnFirst = false;
    bool isFocusOnLast = false;
    CDependencyObject *pFocused = m_pFocusedElement;

    if (IsFocusedElementInPopup())
    {
        // focused element is inside a poup. Since we treat tabnavigation
        // in popup as Cycle, we dont need to check if this element is
        // the first or the last tabstop.
        return true;
    }

    if (reverse)
    {
        // Backward tab processing
        isFocusOnFirst = IsFocusOnFirstTabStop();
    }
    else
    {
        // Forward tab processing
        isFocusOnLast = IsFocusOnLastTabStop();
    }

    if (isFocusOnFirst || isFocusOnLast)
    {
        // Can't process tab from the focus on first or last
        canProcessTab = false;
    }

    if (canProcessTab)
    {
        // Get the first/last focusable control. This is the opposite direction to check up
        // the scope boundary. (e.g. Forward direction need to get the last focusable control)
        CDependencyObject *pEdge = GetFirstFocusableElementFromRoot(!reverse);

        // Need to check the Once navigation mode to be out the plugin control
        // if the current focus is on the edge boundary with Once mode.
        if (pEdge)
        {
            CUIElement *pEdgeParent = GetParentElement(pEdge);
            if (pEdgeParent && GetTabNavigation(pEdgeParent) == DirectUI::KeyboardNavigationMode::Once &&
                pEdgeParent == GetParentElement(pFocused))
            {
                // Can't process, so tab will be out of plugin
                canProcessTab = false;
            }
        }
        else
        {
            canProcessTab = false;
        }
    }
    else
    {
        // We can process tab in case of Cycle navigation mode
        // even though the focus is on the edge of plug-in control.
        if (isFocusOnLast || isFocusOnFirst)
        {
            if (GetTabNavigation(pFocused) == DirectUI::KeyboardNavigationMode::Cycle)
            {
                canProcessTab = true;
            }
            else
            {
                CUIElement *pFocusedParent = GetParentElement(pFocused);
                while (pFocusedParent)
                {
                    if (GetTabNavigation(pFocusedParent) == DirectUI::KeyboardNavigationMode::Cycle)
                    {
                        canProcessTab = true;
                        break;
                    }
                    pFocusedParent = GetParentElement(pFocusedParent);
                }
            }
        }
    }

    return canProcessTab;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetTabStopCandidateElement
//
//  Synopsis:
//      Return the candidate next or previous tab stop element.
//
//------------------------------------------------------------------------
CDependencyObject*
CFocusManager::GetTabStopCandidateElement(_In_ bool isShiftPressed, _In_ bool queryOnly, _Out_ bool& didCycleFocusAtRootVisualScope)
{
    CDependencyObject *pNewTabStop = nullptr;

    didCycleFocusAtRootVisualScope = false;
    CDependencyObject *pRoot = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();

    if (!pRoot)
    {
        return nullptr;
    }

    //------------------------------------------------------------------------
    // Bug #29388
    //
    // The internalCycleWorkaround flag is a workaround for an issue in
    // GetNextTabStop()/GetPreviousTabStop() where it fails to properly
    // detect a TabNavigation="Cycle" in a root-level UserControl.
    //
    bool internalCycleWorkaround = false;

    if (m_pFocusedElement && m_bCanTabOutOfPlugin)
    {
        // CanProcessTabStop() used to be an early-out test, but the heuristic
        // is flawed and caused bugs like #25058.
        internalCycleWorkaround = CanProcessTabStop(isShiftPressed);
    }
    //------------------------------------------------------------------------

    if (m_pFocusedElement == nullptr || FocusedElementIsBehindFullWindowMediaRoot())
    {
        if (!isShiftPressed)
        {
            pNewTabStop = GetFirstFocusableElement(pRoot, nullptr);
        }
        else
        {
            pNewTabStop = GetLastFocusableElement(pRoot, nullptr);
        }

        didCycleFocusAtRootVisualScope = true;
    }
    else if (!isShiftPressed)
    {
        pNewTabStop = GetNextTabStop();

        // If we could not find a tab stop, see if we need to tab cycle.
        if (pNewTabStop == nullptr && (!m_bCanTabOutOfPlugin || internalCycleWorkaround || queryOnly))
        {
            pNewTabStop = GetFirstFocusableElement(pRoot, nullptr);
            didCycleFocusAtRootVisualScope = true;
        }
    }
    else
    {
        pNewTabStop = GetPreviousTabStop();

        // If we could not find a tab stop, see if we need to tab cycle.
        if (pNewTabStop == nullptr && (!m_bCanTabOutOfPlugin || internalCycleWorkaround || queryOnly))
        {
            pNewTabStop = GetLastFocusableElement(pRoot, nullptr);
            didCycleFocusAtRootVisualScope = true;
        }
    }

    return pNewTabStop;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::ProcessTabStopInternal
//
//  Synopsis:
//      Process TabStop to retrieve the next or previous tab stop element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFocusManager::ProcessTabStopInternal(
    _In_ bool bPressedShift,
    _In_ bool queryOnly,
    _Outptr_ CDependencyObject** ppNewTabStopElement)
{
    HRESULT hr = S_OK;
    bool isTabStopOverridden = false;
    bool didCycleFocusAtRootVisualScope = false;
    CDependencyObject *pNewTabStop = nullptr;
    CDependencyObject *pDefaultCandidateTabStop = nullptr;
    CDependencyObject *pNewTabStopFromCallback = nullptr;

    IFCPTR(ppNewTabStopElement);
    *ppNewTabStopElement = NULL;

    // Get the default tab stoppable element.
    pDefaultCandidateTabStop = GetTabStopCandidateElement(!!bPressedShift, queryOnly, didCycleFocusAtRootVisualScope);

    // Ask UIElement for a new tab stop suggestion (this is to solve appbar focus issues)
    IFC(FxCallbacks::UIElement_ProcessTabStop(
        &m_contentRoot,
        static_cast<CDependencyObject *>(m_pFocusedElement),
        static_cast<CDependencyObject *>(pDefaultCandidateTabStop),
        !!bPressedShift,
        didCycleFocusAtRootVisualScope,
        &pNewTabStopFromCallback,
        &isTabStopOverridden));

    if (isTabStopOverridden)
    {
        pNewTabStop = static_cast<CDependencyObject *>(pNewTabStopFromCallback);
    }

    // If no suggestions from framework apply the regular logic
    if (!isTabStopOverridden && pNewTabStop == NULL && pDefaultCandidateTabStop)
    {
        pNewTabStop = pDefaultCandidateTabStop;
    }

    if (pNewTabStop)
    {
        *ppNewTabStopElement = pNewTabStop;

        pNewTabStop->AddRef();
        pNewTabStop = nullptr;
    }

Cleanup:
    ReleaseInterface(pNewTabStopFromCallback);
    ReleaseInterface(pNewTabStop);

    RRETURN(hr);
}

FocusObserver* CFocusManager::GetFocusObserverNoRef()
{
    return m_focusObserver.get();
}

// Process the TabStop navigation.
//   In bHandled returns false if we can't navigate to the next or previous TabStop.
_Check_return_ HRESULT
CFocusManager::ProcessTabStop(_In_ bool bPressedShift, _Out_ bool* bHandled)
{
    (*bHandled) = FALSE;

    xref_ptr<CDependencyObject> spNewTabStop;

    auto scopeGuard = wil::scope_exit([&]
    {
        if (bPressedShift)
        {
            m_isMovingFocusToPreviousTabStop = false;
        }
        else
        {
            m_isMovingFocusToNextTabStop = false;
        }
    });

    if (bPressedShift)
    {
        m_isMovingFocusToPreviousTabStop = true;
    }
    else
    {
        m_isMovingFocusToNextTabStop = true;
    }

    // Get the new tab stoppable element.
    const bool queryOnly = false;
    IFC_RETURN(ProcessTabStopInternal(bPressedShift, queryOnly, spNewTabStop.ReleaseAndGetAddressOf()));

    const DirectUI::FocusNavigationDirection navigationDirection =
        (bPressedShift == TRUE) ? DirectUI::FocusNavigationDirection::Previous : DirectUI::FocusNavigationDirection::Next;

    if (spNewTabStop)
    {
        // Set the focus to the new TabStop control
        const FocusMovementResult result = SetFocusedElement(FocusMovement(spNewTabStop, navigationDirection, FocusState::Keyboard));
        IFC_RETURN(result.GetHResult());
        *bHandled = result.WasMoved();
    }
    else
    {
        GUID correlationId = {};
        UuidCreate(&correlationId);
        IFC_RETURN(m_focusObserver->DepartFocus(navigationDirection, correlationId, bHandled));
    }

    return S_OK;
}

// Returns the first focusable uielement from the root.
CUIElement*
CFocusManager::GetFirstFocusableElement()
{
    CDependencyObject *pFirstFocus = NULL;
    CUIElement *pFirstFocusElement = NULL;

    if (m_contentRoot.GetVisualTreeNoRef())
    {
        // First give focus to the topmost light-dismiss-enabled popup or a Flyout if any is open.
        CPopupRoot* pPopupRoot = m_contentRoot.GetVisualTreeNoRef()->GetPopupRoot();
        if (pPopupRoot)
        {
            CPopup* pTopmostLightDismissPopup = pPopupRoot->GetTopmostPopup(CPopupRoot::PopupFilter::LightDismissOrFlyout);
            if (pTopmostLightDismissPopup)
            {
                pFirstFocusElement = do_pointer_cast<CUIElement>(pTopmostLightDismissPopup);
            }
        }

        if (!pFirstFocusElement)
        {
            pFirstFocus = GetFirstFocusableElementFromRoot(false /* bReverse */);

            if (pFirstFocus)
            {
                if (CUIElement* pFirstFocusAsUIE = do_pointer_cast<CUIElement>(pFirstFocus))
                {
                    pFirstFocusElement = pFirstFocusAsUIE;
                }
                else
                {
                    // When the first focusable element is not a Control, look for an
                    // appropriate reference to return to the caller who wants a Control.
                    // Example: Hyperlink --> Text control hosting the hyperlink
                    pFirstFocusElement = GetParentElement(pFirstFocus);
                }
            }
        }
    }

    return pFirstFocusElement;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetNextFocusableElement
//
//  Synopsis:
//      Return the next focusable element if it is available.
//
//------------------------------------------------------------------------

CUIElement*
CFocusManager::GetNextFocusableElement()
{
    CDependencyObject *pNextFocus = GetNextTabStop();
    CUIElement *pNextFocusElement = NULL;

    if (pNextFocus)
    {
        if (auto elementCast = do_pointer_cast<CUIElement>(pNextFocus))
        {
            pNextFocusElement = elementCast;
        }
        else
        {
            // When the first focusable element is not a Control, look for an
            // appropriate reference to return to the caller who wants a Control.
            // Example: Hyperlink --> Text control hosting the hyperlink
            pNextFocusElement = GetParentElement(pNextFocus);
        }
    }

    return pNextFocusElement;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetNextTabStop
//
//  Synopsis:
//      Return the next TabStop control if it is available.
//      Otherwise, return NULL.
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetNextTabStop(
    _In_opt_ CDependencyObject* pCurrentTabStop,
    _In_ bool bIgnoreCurrentTabStopScope)
{
    CDependencyObject *pNewTabStop = NULL;
    CDependencyObject *pFocused = pCurrentTabStop ? pCurrentTabStop : m_pFocusedElement;
    CDependencyObject *pCurrentCompare = NULL;
    CDependencyObject *pNewTabStopFromCallback = NULL;

    if (pFocused == NULL || !m_contentRoot.GetVisualTreeNoRef())
    {
        return NULL; // No next tab stop
    }

    // Assign the compare(TabIndex value) control with the focused control
    pCurrentCompare = pFocused;

    // #0. Ask UIElement for the next tab stop suggestion.
    // For example, LightDismiss enabled Popup will process GetNextTabStop callbacks.
    IGNOREHR(FxCallbacks::UIElement_GetNextTabStop(static_cast<CDependencyObject *>(pFocused), &pNewTabStopFromCallback));
    pNewTabStop = static_cast<CDependencyObject *>(pNewTabStopFromCallback);

    // #1. Search TabStop from the children
    if (pNewTabStop == NULL &&
        !bIgnoreCurrentTabStopScope &&
        (IsVisible(pFocused) && (CanHaveChildren(pFocused) || CanHaveFocusableChildren(pFocused))))
    {
        pNewTabStop = GetFirstFocusableElement(pFocused, pNewTabStop);
    }

    // #2. Search TabStop from the sibling of parent
    if (pNewTabStop == NULL)
    {
        bool bCurrentPassed = false;
        CDependencyObject *pCurrent = pFocused;
        CDependencyObject *pParent = GetFocusParent(pFocused);
        bool parentIsRootVisual = pParent == m_contentRoot.GetVisualTreeNoRef()->GetRootVisual();

        while (pParent != NULL && !parentIsRootVisual && pNewTabStop == NULL)
        {
            if (IsValidTabStopSearchCandidate(pCurrent) && GetTabNavigation(pCurrent) == DirectUI::KeyboardNavigationMode::Cycle)
            {
                if (pCurrent == GetParentElement(pFocused))
                {
                    // The focus will be cycled under the focusable children if the current focused
                    // control is the cycle navigation mode
                    pNewTabStop = GetFirstFocusableElement(pCurrent, NULL);
                }
                else
                {
                    // The current can be focusable
                    pNewTabStop = GetFirstFocusableElement(pCurrent, pCurrent);
                }
                break;
            }

            if (IsValidTabStopSearchCandidate(pParent) && GetTabNavigation(pParent) == DirectUI::KeyboardNavigationMode::Once)
            {
                pCurrent = pParent;
                pParent = GetFocusParent(pParent);
                if (pParent == NULL)
                {
                    break;
                }
            }
            else if (!IsValidTabStopSearchCandidate(pParent))
            {
                // Get the parent control whether it is a focusable or not
                CUIElement *pParentElement = GetParentElement(pParent);
                if (pParentElement == NULL)
                {
                    // if the focused element is under a popup and there is no control in its ancestry up until
                    // the popup, then consider tabnavigation as cycle within the popup subtree.
                    pParent = GetRootOfPopupSubTree(pCurrent);
                    if (pParent)
                    {
                        // try to find the next tabstop.
                        pNewTabStop = GetNextTabStopInternal(pParent, pCurrent, pNewTabStop, &bCurrentPassed, &pCurrentCompare);
                        // Retrieve the first tab stop from the current tab stop when the current tab stop is not focusable.
                        if (pNewTabStop && !IsFocusable(pNewTabStop))
                        {
                            pNewTabStop = GetFirstFocusableElement(pNewTabStop, NULL);
                        }
                        if (!pNewTabStop)
                        {
                            // the focused element is the last tabstop. move the focus to the first
                            // focusable element within the popup.
                            pNewTabStop = GetFirstFocusableElement(pParent, NULL);
                        }
                        break;
                    }
                    pParent = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
                }
                else if (pParentElement != NULL && GetTabNavigation(pParentElement) == DirectUI::KeyboardNavigationMode::Once)
                {
                    // We need to get out of the current scope in case of Once navigation mode, so
                    // reset the current and parent to search the next available focus control
                    pCurrent = pParentElement;
                    pParent = GetFocusParent(pParentElement);
                    if (pParent == NULL)
                    {
                        break;
                    }
                }
                else
                {
                    // Assign the parent that can have a focusable child.
                    // If there is no parent that can be a TapStop, assign the root
                    // to figure out the next focusable element from the root
                    if (pParentElement)
                    {
                        pParent = pParentElement;
                    }
                    else
                    {
                        pParent = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
                    }
                }
            }

            pNewTabStop = GetNextTabStopInternal(pParent, pCurrent, pNewTabStop, &bCurrentPassed, &pCurrentCompare);

            // GetNextTabStopInternal can return the not focusable element which has a focusable child
            if (pNewTabStop && !IsFocusable(pNewTabStop) && CanHaveFocusableChildren(pNewTabStop))
            {
                pNewTabStop = GetFirstFocusableElement(pNewTabStop, NULL);
            }

            if (pNewTabStop)
            {
                break;
            }

            // Only assign the current when the parent is a element that can TabStop
            if (IsValidTabStopSearchCandidate(pParent))
            {
                pCurrent = pParent;
            }

            pParent = GetFocusParent(pParent);

            bCurrentPassed = FALSE;

            parentIsRootVisual = pParent == m_contentRoot.GetVisualTreeNoRef()->GetRootVisual();
        }
    }

    ReleaseInterface(pNewTabStopFromCallback);

    return pNewTabStop;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetNextTabStopInternal
//
//  Synopsis:
//      Return the next TabStop control if it is available.
//      Otherwise, return NULL.
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetNextTabStopInternal(
    _In_ CDependencyObject* pParent,
    _In_ CDependencyObject* pCurrent,
    _In_ CDependencyObject* pCandidate,
    _Inout_ bool *bCurrentPassed,
    _Inout_ CDependencyObject **pCurrentCompare)
{
    CDependencyObject *pNewTabStop = pCandidate;
    CDependencyObject *pChildStop = NULL;

    // Update the compare(TabIndex value) control by searching the siblings of ancestor
    if (IsValidTabStopSearchCandidate(pCurrent))
    {
        *pCurrentCompare = pCurrent;
    }

    if (pParent)
    {
        CDependencyObject* childNoRef = nullptr;
        XINT32 compareIndexResult;
        bool bFoundCurrent = false;

        // Find next TabStop from the children
        FocusProperties::FocusChildrenIteratorWrapper iterator = FocusProperties::GetFocusChildrenInTabOrderIterator(pParent);
        while (iterator.TryMoveNext(&childNoRef))
        {
            pChildStop = nullptr;

            if (childNoRef && childNoRef == pCurrent)
            {
                bFoundCurrent = TRUE;
                *bCurrentPassed = TRUE;
                continue;
            }

            if (childNoRef && IsVisible(childNoRef))
            {
                // This will only hit in Pre-RS5 scenarios
                if (childNoRef == pCurrent)
                {
                    bFoundCurrent = TRUE;
                    *bCurrentPassed = TRUE;
                    continue;
                }

                if (IsValidTabStopSearchCandidate(childNoRef))
                {
                    // If we have a UIElement, such as a StackPanel, we want to check its children for the next tab stop
                    if (!IsPotentialTabStop(childNoRef))
                    {
                        pChildStop = GetNextTabStopInternal(childNoRef, pCurrent, pNewTabStop, bCurrentPassed, pCurrentCompare);
                    }
                    else
                    {
                        pChildStop = childNoRef;
                    }
                }
                else if (CanHaveFocusableChildren(childNoRef))
                {
                    pChildStop = GetNextTabStopInternal(childNoRef, pCurrent, pNewTabStop, bCurrentPassed, pCurrentCompare);
                }

                if (!*bCurrentPassed)
                {
                    // Handling of 21H2 bug 32260809: When the focus change occurs as the result of pCurrent's removal from the tree,
                    // childNoRef will never be set to pCurrent and thus *bCurrentPassed will not be set to TRUE above.
                    // This would lead to the wrong element getting focus. While GetNextTabStopInternal is processed, pCurrent still has
                    // its m_pParent set, but pCurrent no longer appears in that parent's children collection. We detect the parent-child
                    // relationship and set *bCurrentPassed to TRUE so that pNewTabStop can later be set even though compareIndexResult
                    // may be 0. Unfortunately this only works when the leaving pCurrent element is the last focusable child for
                    // currentParentNoRef. pCurrent's ex-position within currentParentNoRef is unknown at this point and focusing a previous
                    // sibling would require deep changes.
                    CDependencyObject* currentParentNoRef = pCurrent->GetParentInternal(false /*publicParentOnly*/);

                    if (currentParentNoRef && (currentParentNoRef == childNoRef || childNoRef->IsAncestorOf(currentParentNoRef)))
                    {
                        *bCurrentPassed = TRUE;
                    }
                }
            }

            if (pChildStop && (IsFocusable(pChildStop) || CanHaveFocusableChildren(pChildStop)))
            {
                compareIndexResult = CompareTabIndex(pChildStop, *pCurrentCompare);

                if (compareIndexResult > 0 || ((bFoundCurrent || *bCurrentPassed) && compareIndexResult == 0))
                {
                    if (pNewTabStop)
                    {
                        if (CompareTabIndex(pChildStop, pNewTabStop) < 0)
                        {
                            pNewTabStop = pChildStop;
                        }
                    }
                    else
                    {
                        pNewTabStop = pChildStop;
                    }
                }
            }
        }
    }

    return pNewTabStop;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetPreviousTabStop
//
//  Synopsis:
//      Return the previous TabStop control if it is available.
//      Otherwise, return NULL.
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetPreviousTabStop(_In_opt_ CDependencyObject* pCurrentTabStop)
{
    CDependencyObject *pFocused = pCurrentTabStop ? pCurrentTabStop : m_pFocusedElement;
    CDependencyObject *pNewTabStop = NULL;
    CDependencyObject *pCurrentCompare = NULL;
    CDependencyObject *pNewTabStopFromCallback = NULL;

    if (pFocused == NULL && !m_contentRoot.GetVisualTreeNoRef())
    {
        return NULL; // No previous tab stop
    }

    // Assign the compare(TabIndex value) control with the focused control
    pCurrentCompare = pFocused;

    // #0. Ask UIElement for the previous tab stop suggestion.
    // For example, LightDismiss enabled Popup will process GetPreviousTabStop callbacks.
    IGNOREHR(FxCallbacks::UIElement_GetPreviousTabStop(static_cast<CDependencyObject *>(pFocused), &pNewTabStopFromCallback));
    pNewTabStop = static_cast<CDependencyObject *>(pNewTabStopFromCallback);

    // Search the previous TabStop from the sibling of parent
    if (pNewTabStop == NULL)
    {
        bool bCurrentPassed = false;

        CDependencyObject *pCurrent = pFocused;
        CDependencyObject *pParent = GetFocusParent(pFocused);

        while (pParent != NULL && !pParent->OfTypeByIndex<KnownTypeIndex::RootVisual>() && pNewTabStop == NULL)
        {
            if (IsValidTabStopSearchCandidate(pCurrent) && GetTabNavigation(pCurrent) == DirectUI::KeyboardNavigationMode::Cycle)
            {
                pNewTabStop = GetLastFocusableElement(pCurrent, pCurrent);
                break;
            }

            if (IsValidTabStopSearchCandidate(pParent) && GetTabNavigation(pParent) == DirectUI::KeyboardNavigationMode::Once)
            {
                // Set focus on the parent if it is a focusable control. Otherwise, keep search it up.
                if ((IsFocusable(pParent)))
                {
                    pNewTabStop = pParent;
                    break;
                }
                else
                {
                    pCurrent = pParent;
                    pParent = GetFocusParent(pParent);
                    if (pParent == NULL)
                    {
                        break;
                    }
                }
            }
            else if (!IsValidTabStopSearchCandidate(pParent))
            {
                // Get the parent control whether it is a focusable or not
                CUIElement *pParentElement = GetParentElement(pParent);
                if (pParentElement == NULL)
                {
                    // if the focused element is under a popup and there is no control in its ancestry up until
                    // the popup, then consider tabnavigation as cycle within the popup subtree.
                    pParent = GetRootOfPopupSubTree(pCurrent);
                    if (pParent)
                    {
                        // find the previous tabstop
                        pNewTabStop = GetPreviousTabStopInternal(pParent, pCurrent, pNewTabStop, &bCurrentPassed, &pCurrentCompare);
                        // Retrieve the last tab stop from the current tab stop when the current tab stop is not focusable.
                        if (pNewTabStop && !IsFocusable(pNewTabStop))
                        {
                            pNewTabStop = GetLastFocusableElement(pNewTabStop, NULL);
                        }
                        if (!pNewTabStop)
                        {
                            // focused element is the first tabstop within the popup. move focus
                            // to the last tabstop within the popup
                            pNewTabStop = GetLastFocusableElement(pParent, NULL);
                        }
                        break;
                    }
                    pParent = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
                }
                else if (pParentElement != NULL && GetTabNavigation(pParentElement) == DirectUI::KeyboardNavigationMode::Once)
                {
                    // Set focus on the parent control if it is a focusable control. Otherwise, keep search it up.
                    if (IsFocusable(pParentElement))
                    {
                        pNewTabStop = pParentElement;
                        break;
                    }
                    else
                    {
                        // We need to get out of the current scope in case of Once navigation mode, so
                        // reset the current and parent to search the next available focus control
                        pCurrent = pParentElement;
                        pParent = GetFocusParent(pParentElement);
                        if (pParent == NULL)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // Assign the parent that can have a focusable child.
                    // If there is no parent that can be a TabStop, assign the root
                    // to figure out the next focusable element from the root
                    if (pParentElement)
                    {
                        pParent = pParentElement;
                    }
                    else
                    {
                        pParent = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
                    }
                }
            }

            pNewTabStop = GetPreviousTabStopInternal(pParent, pCurrent, pNewTabStop, &bCurrentPassed, &pCurrentCompare);

            if (pNewTabStop == NULL && IsPotentialTabStop(pParent) && IsFocusable(pParent))
            {
                if (IsPotentialTabStop(pParent) && GetTabNavigation(pParent) == DirectUI::KeyboardNavigationMode::Cycle)
                {
                    pNewTabStop = GetLastFocusableElement(pParent, NULL /*LastFocusable*/);
                }
                else
                {
                    pNewTabStop = pParent;
                }
            }
            else
            {
                // Find the last focusable element from the current focusable container
                if (pNewTabStop && CanHaveFocusableChildren(pNewTabStop))
                {
                    pNewTabStop = GetLastFocusableElement(pNewTabStop, NULL);
                }
            }

            if (pNewTabStop)
            {
                break;
            }

            // Only assign the current when the parent is a element that can TapStop
            if (IsValidTabStopSearchCandidate(pParent))
            {
                pCurrent = pParent;
            }

            pParent = GetFocusParent(pParent);
            bCurrentPassed = false;
        }
    }

    ReleaseInterface(pNewTabStopFromCallback);

    return pNewTabStop;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetPreviousTabStopInternal
//
//  Synopsis:
//      Return the previous TabStop control if it is available.
//      Otherwise, return NULL.
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetPreviousTabStopInternal(
    _In_ CDependencyObject* pParent,
    _In_ CDependencyObject* pCurrent,
    _In_ CDependencyObject* pCandidate,
    _Inout_ bool *bCurrentPassed,
    _Inout_ CDependencyObject **pCurrentCompare)
{
    CDependencyObject *pNewTabStop = pCandidate;
    CDependencyObject *pChildStop = NULL;

    // Update the compare(TabIndex value) control by searching the siblings of ancestor
    if (IsValidTabStopSearchCandidate(pCurrent))
    {
        *pCurrentCompare = pCurrent;
    }

    if (pParent)
    {
        CDependencyObject* childNoRef = nullptr;
        XINT32 compareIndexResult;
        bool bFoundCurrent = false;
        bool bCurrentCompare;

        // Find previous TabStop from the children
        FocusProperties::FocusChildrenIteratorWrapper iterator = FocusProperties::GetFocusChildrenInTabOrderIterator(pParent);
        while (iterator.TryMoveNext(&childNoRef))
        {
            bCurrentCompare = FALSE;
            pChildStop = nullptr;

            if (childNoRef && childNoRef == pCurrent)
            {
                bFoundCurrent = TRUE;
                *bCurrentPassed = TRUE;
                continue;
            }

            if (childNoRef && IsVisible(childNoRef))
            {
                // This will only hit in Pre-RS5 scenarios
                if (childNoRef == pCurrent)
                {
                    bFoundCurrent = TRUE;
                    *bCurrentPassed = TRUE;
                    continue;
                }

                if (IsValidTabStopSearchCandidate(childNoRef))
                {
                    // If we have a UIElement, such as a StackPanel, we want to check it's children for the next tab stop
                    if (!IsPotentialTabStop(childNoRef))
                    {
                        pChildStop = GetPreviousTabStopInternal(childNoRef, pCurrent, pNewTabStop, bCurrentPassed, pCurrentCompare);
                        bCurrentCompare = TRUE;
                    }
                    else
                    {
                        pChildStop = childNoRef;
                    }
                }
                else if (CanHaveFocusableChildren(childNoRef))
                {
                    pChildStop = GetPreviousTabStopInternal(childNoRef, pCurrent, pNewTabStop, bCurrentPassed, pCurrentCompare);
                    bCurrentCompare = TRUE;
                }
            }

            if (pChildStop && (IsFocusable(pChildStop) || CanHaveFocusableChildren(pChildStop)))
            {
                compareIndexResult = CompareTabIndex(pChildStop, *pCurrentCompare);

                if (compareIndexResult < 0 ||
                    (((!bFoundCurrent && !*bCurrentPassed) || bCurrentCompare) && compareIndexResult == 0))
                {
                    if (pNewTabStop)
                    {
                        if (CompareTabIndex(pChildStop, pNewTabStop) >= 0)
                        {
                            pNewTabStop = pChildStop;
                        }
                    }
                    else
                    {
                        pNewTabStop = pChildStop;
                    }
                }
            }
        }
    }

    return pNewTabStop;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::CompareTabIndex
//
//  Synopsis:
//      Compare TabIndex value between focusable(TabStop) controls.
//      Return +1 if control1 > control2
//      Return = if control1 = control2
//      Return -1 if control1 < control2
//
//------------------------------------------------------------------------

XINT32
CFocusManager::CompareTabIndex(_In_ CDependencyObject *pControl1, _In_ CDependencyObject *pControl2)
{
    if (GetTabIndex(pControl1) > GetTabIndex(pControl2))
    {
        return 1;
    }
    else if (GetTabIndex(pControl1) < GetTabIndex(pControl2))
    {
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::IsFocusOnFirstTabStop
//
//  Synopsis:
//      Return true if the focus in the first TabStop.
//
//------------------------------------------------------------------------

bool
CFocusManager::IsFocusOnFirstTabStop()
{
    CDependencyObject *pFocused = m_pFocusedElement;
    CDependencyObject *pRoot = NULL;
    CDependencyObject *pFirstFocus = NULL;

    if (pFocused == NULL || !m_contentRoot.GetVisualTreeNoRef())
    {
        return false;
    }

    pRoot = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
    ASSERT(pRoot);

    pFirstFocus = GetFirstFocusableElement(pRoot, NULL);

    if (pFocused == pFirstFocus)
    {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::IsFocusOnLastTabStop
//
//  Synopsis:
//      Return true if the focus in the last TabStop.
//
//------------------------------------------------------------------------

bool
CFocusManager::IsFocusOnLastTabStop()
{
    CDependencyObject *pFocused = m_pFocusedElement;
    CDependencyObject *pRoot = NULL;
    CDependencyObject *pLastFocus = NULL;

    if (pFocused == NULL || !m_contentRoot.GetVisualTreeNoRef())
    {
        return false;
    }

    pRoot = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
    ASSERT(pRoot);

    pLastFocus = GetLastFocusableElement(pRoot, NULL);

    if (pFocused == pLastFocus)
    {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::CanHaveFocusableChildren
//
//  Synopsis:
//      Return true if there is a focusable child
//
//------------------------------------------------------------------------

bool
CFocusManager::CanHaveFocusableChildren(_In_ CDependencyObject *pParent)
{
    return FocusProperties::CanHaveFocusableChildren(pParent);
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetParentControl
//
//  Synopsis:
//      Return the control of parent(ancestor) from the current control
//
//------------------------------------------------------------------------

CUIElement*
CFocusManager::GetParentElement(_In_ CDependencyObject *pCurrent)
{
    CDependencyObject *pParent = nullptr;

    if (pCurrent)
    {
        pParent = GetFocusParent(pCurrent);

        while (pParent != nullptr)
        {
            if (IsValidTabStopSearchCandidate(pParent) && pParent->OfTypeByIndex(KnownTypeIndex::UIElement))
            {
                return static_cast<CUIElement*>(pParent);
            }

            pParent = GetFocusParent(pParent);
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::NotifyFocusChanged
//
//  Synopsis:
//      Notify the focus changing that ensure the focused element visible with
//      Input Host Manager
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFocusManager::NotifyFocusChanged(_In_ bool bringIntoView, _In_ bool animateIfBringIntoView)
{
    if (m_pFocusedElement && static_cast<CDependencyObject*>(m_pFocusedElement)->GetContext())
    {
        IFC_RETURN(m_contentRoot.GetInputManager().NotifyFocusChanged(m_pFocusedElement, bringIntoView, animateIfBringIntoView));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CFocusManager::IsFocusable
//
//  Synopsis:
//      Determine if a particular DependencyObject cares to take focus.
//
//------------------------------------------------------------------------

bool
CFocusManager::IsFocusable(_In_ CDependencyObject *pObject, bool ignoreOffScreenPosition)
{
    return FocusProperties::IsFocusable(pObject, ignoreOffScreenPosition);
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetFocusParent
//
//  Synopsis:
//      Return the object that should be considered the parent for the purposes
//  of tab focus.  This is not necessarily the same as the standard
//  CDependencyObject::GetParent() object.
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetFocusParent(_In_ CDependencyObject *pObject)
{
    if (pObject)
    {
        if (CFocusableHelper::IsFocusableDO(pObject))
        {
            return CFocusableHelper::GetContainingFrameworkElementIfFocusable(pObject);

        }
        else
        {
            CDependencyObject *pVisualRoot = m_contentRoot.GetVisualTreeNoRef()->GetActiveRootVisual();
            ASSERT(pVisualRoot);

            // Root SV is located between the visual root and hidden root.
            // GetFocusParent shouldn't return the ancestor of the visual root.
            if (pObject != pVisualRoot)
            {
                return pObject->GetParent();
            }
        }
    }

    // NULL input --> NULL output
    return NULL;
}


//------------------------------------------------------------------------
//
//  Method:   CFocusManager::IsVisible
//
//  Synopsis:
//      Determine if the object is visible.
//
//------------------------------------------------------------------------

bool
CFocusManager::IsVisible(_In_ CDependencyObject *pObject)
{
    return FocusProperties::IsVisible(pObject);
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetTabIndex
//
//  Synopsis:
//      Returns the value set by application developer to determine the tab
//  order.  Default value is XINT32_MAX which means to evaluate them in
//  their ordering of the focus children collection.
//
//------------------------------------------------------------------------

XINT32
CFocusManager::GetTabIndex(_In_ CDependencyObject *pObject)
{
    if (pObject)
    {
        if (pObject->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            return static_cast<CUIElement*>(pObject)->TabIndex();
        }
        else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(pObject))
        {
            return ifocusable->GetTabIndex();
        }
    }

    return XINT32_MAX;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::CanHaveChildren
//
//  Synopsis:
//      Determines if this object type can have children that should be considered
//  for tab ordering.  Note that this doesn't necessarily mean if the object
//  instance has focusable children right now.  CanHaveChildren() may be TRUE
//  even though the focus children collection is empty.
//
//------------------------------------------------------------------------

bool
CFocusManager::CanHaveChildren(_In_ CDependencyObject *pObject)
{
    if (CUIElement* pUIElement = do_pointer_cast<CUIElement>(pObject)) // Really INDEX_VISUAL but Visual is not in the publicly exposed hierarchy.
    {
        return pUIElement->CanHaveChildren();
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetRootOfPopupSubTree
//
//  Synopsis:
//      Returns non-null if the given object is within a popup.
//
//------------------------------------------------------------------------

CDependencyObject*
CFocusManager::GetRootOfPopupSubTree(_In_ CDependencyObject *pObject)
{
    CDependencyObject *pRoot = NULL;

    if (pObject)
    {
        if (CUIElement *pUIElement = do_pointer_cast<CUIElement>(pObject))
        {
            pRoot = pUIElement->GetRootOfPopupSubTree();
        }
        else
        {
            CUIElement* pParentElement = GetParentElement(pObject);

            if (pParentElement)
            {
                pRoot = pParentElement->GetRootOfPopupSubTree();
            }
        }
    }

    return pRoot;
}

//------------------------------------------------------------------------
//
//  Method:   CFocusManager::GetTabNavigation
//
//  Synopsis:
//      Returns the value of the TabNavigation property.
//
//------------------------------------------------------------------------

DirectUI::KeyboardNavigationMode
CFocusManager::GetTabNavigation(_In_ CDependencyObject *pObject) const
{
    CUIElement* element = do_pointer_cast<CUIElement>(pObject);

    if (element == nullptr) { return DirectUI::KeyboardNavigationMode::Local; }

    return element->TabNavigation();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Tests to see if the parameter object's type supports being a tab stop.
//  Though the specific instance might not be a valid tab stop at the moment.
//  (Example: Disabled state)
//
//------------------------------------------------------------------------
bool
CFocusManager::IsPotentialTabStop(_In_ CDependencyObject *pObject) const
{
    return FocusProperties::IsPotentialTabStop(pObject);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Allow the focus change event with the below condition
//      1. Plugin has a focus
//      2. FullScreen window mode
//      3. Windowless mode on FireFox, Safari browser which is not IE
//
//------------------------------------------------------------------------
bool
CFocusManager::CanRaiseFocusEventChange()
{
    bool bCanRaiseFocusEvent = false;

    if (m_pCoreService)
    {
        if (IsPluginFocused())
        {
            bCanRaiseFocusEvent = TRUE;
        }
    }

    return bCanRaiseFocusEvent;
}

bool CFocusManager::ShouldUpdateFocus(
    _In_opt_ CDependencyObject * const pNewFocus,
    _In_ DirectUI::FocusState focusState) const
{
    bool shouldUpdateFocus = true;

    //TFS 5777889. There are some scenarios (mainly with SIP), where we want to interact with another element, but
    //have the focus stay with the currently focused element
    if (pNewFocus)
    {
        if (auto newFocusCastFE = do_pointer_cast<CFrameworkElement>(pNewFocus))
        {
            shouldUpdateFocus = Focus::FocusSelection::ShouldUpdateFocus(
                newFocusCastFE,
                focusState);
        }
        else if (auto newFocusCastFlyout = do_pointer_cast<CFlyoutBase>(pNewFocus))
        {
            shouldUpdateFocus = Focus::FocusSelection::ShouldUpdateFocus(
                newFocusCastFlyout,
                focusState);
        }
        else if (auto newFocusCastTextElement = do_pointer_cast<CTextElement>(pNewFocus))
        {
            shouldUpdateFocus = Focus::FocusSelection::ShouldUpdateFocus(
                newFocusCastTextElement,
                focusState);
        }
    }

    return shouldUpdateFocus;
}

// Returns true if we should set Win32 focus to the containing HWND based on the FocusMovement.
// This can cause our top-level window to be activated, so there are some cases where we want to skip it.
bool CFocusManager::ShouldSetWindowFocus(const FocusMovement& movement) const
{
    // If we're updating the state to Unfocused, we don't want to set focus to our HWND.
    if (movement.GetFocusState() == FocusState::Unfocused)
    {
        return false;
    }

    // Don't set window focus if the movement hasn't requested input activation (such as because
    // this was an "emergency/correction" of which element should have focus when the focused
    // element is hidden or removed from the tree).
    if (movement.requestInputActivation == false)
    {
        return false;
    }

    // When an element gets light-dismissed, XAML sets focus back to the previously-focused element (CPopup::Close).
    // If the light-dismiss is due to the focus moving away from our window, we don't want this focus change to
    // activate our window again.
    const bool focusIsForLightDismissOnInactiveWindow = movement.isForLightDismiss && !m_contentRoot.GetIsInputActive();
    if (focusIsForLightDismissOnInactiveWindow)
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Changes our currently focused element.  FocusMovementResult.WasMoved() indicates
//  whether the update is successful.  We will also be responsible for raising focus related
//  events (GettingFocus/LosingFocus and GotFocus/LostFocus).
//
//  FocusMovementResult.WasMoved() indicates that the process has executed successfully.
//
// We have two ways (FocusMovementResult.GetHResult() and FocusMovementResult.WasMoved()) to report success and failure.
// This is because it is possible that the given pNewFocus could not actually take focus for
// innocent reasons.  (Currently in disabled state, etc.)  This is not an
// important failure and GetHResult() return S_OK even though WasMoved() is false.
//
// A failing HRESULT is reserved for important failures like OOM conditions or
// reentrancy prevention. This distinction is made in the interest of reducing noise
// in tools like XcpMon.
//
//  Boundary cases:
// - If pNewFocus is the object that already has current focus,
//   it is counted as success even though nothing was technically "updated".
//
// - If a focus change is cancelled in a Getting/Losing Focus handler, it
//   is counted as success even though focus was technically not "updated".
//
//------------------------------------------------------------------------
_Check_return_ FocusMovementResult
CFocusManager::UpdateFocus(_In_ const FocusMovement& movement)
{
    CDependencyObject* pNewFocus = movement.GetTarget();
    const DirectUI::FocusNavigationDirection focusNavigationDirection = movement.GetDirection();
    const bool forceBringIntoView = movement.forceBringIntoView;
    const bool animateIfBringIntoView = movement.animateIfBringIntoView;

    const DirectUI::FocusState nonCoercedFocusState = movement.GetFocusState();
    const DirectUI::FocusState coercedFocusState = CoerceFocusState(nonCoercedFocusState);

    bool success = false;

    bool focusCancelled = false;
    bool shouldCompleteAsyncOperation = movement.shouldCompleteAsyncOperation;

    HRESULT hr = S_OK;
    CDependencyObject *pOldFocusedElement = nullptr;

    bool shouldBringIntoView = false;
    GUID correlationId = m_asyncOperation != nullptr ? m_asyncOperation->GetCorrelationId() : movement.GetCorrelationId();

    TraceUpdateFocusBegin();

    DirectUI::InputDeviceType lastInputDeviceType = DirectUI::InputDeviceType::None;

    // This function makes some synchronous callouts to user code, let's make sure the object stays alive during those calls
    xref_ptr<CFocusManager> keepAlive{this};

    if (m_asyncOperation != nullptr && shouldCompleteAsyncOperation == false)
    {
        VERIFYHR(ErrorHelper::OriginateErrorUsingResourceID(
            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_FOCUS_ASYNCOP_INPROGRESS),
            ERROR_FOCUSMANAGER_MOVING_FOCUS));

        goto Cleanup;
    }

    if (m_focusLocked && m_ignoreFocusLock == false)
    {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
        IFC(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_FOCUSMANAGER_MOVING_FOCUS));
    }

    if (!ShouldUpdateFocus(pNewFocus, nonCoercedFocusState))
    {
        goto Cleanup;
    }

    if (m_contentRoot.IsShuttingDown())
    {
        // Don't change focus setting in case of the processing of ResetVisualTree
        goto Cleanup;
    }

    // When navigating between pages, we need to clear the (prior focus) state on XYFocus
    if (pNewFocus == nullptr
        && coercedFocusState == DirectUI::FocusState::Unfocused)
    {
        m_xyFocus.ResetManifolds();
    }

    lastInputDeviceType = m_contentRoot.GetInputManager().GetLastInputDeviceType();

    if (lastInputDeviceType == DirectUI::InputDeviceType::GamepadOrRemote)
    {
        CTextBoxBase* const pElementAsTextBox = do_pointer_cast<CTextBoxBase>(pNewFocus);
        if (pElementAsTextBox)
        {
            pElementAsTextBox->IncomingFocusFromGamepad();
        }
    }

    if (pNewFocus == m_pFocusedElement)
    {
        CUIElement* newFocusAsElement = do_pointer_cast<CUIElement>(pNewFocus);

        if (newFocusAsElement && newFocusAsElement->GetFocusState() != coercedFocusState)
        {
            // We do not raise GettingFocus here since the OldFocusedElement and NewFocusedElement
            // would be the same element.
            IFC(RaiseGotFocusEvent(m_pFocusedElement, correlationId));

            // Make sure the FocusState is up-to-date.
            IFC(newFocusAsElement->UpdateFocusState(coercedFocusState));
            m_realFocusStateForFocusedElement = nonCoercedFocusState;
        }
        else if (IFocusable* newFocusAsIFocusable = CFocusableHelper::GetIFocusableForDO(pNewFocus))
        {
            CValue value;
            IFC(pNewFocus->GetValueByIndex(newFocusAsIFocusable->GetFocusStatePropertyIndex(), &value));

            if (coercedFocusState != static_cast<DirectUI::FocusState>(value.AsEnum()))
            {
                IFC(RaiseGotFocusEvent(m_pFocusedElement, correlationId));

                value.ReleaseAndReset();
                value.Set(coercedFocusState, KnownTypeIndex::FocusState);

                IFC(pNewFocus->SetValueByIndex(newFocusAsIFocusable->GetFocusStatePropertyIndex(), value));
            }

            m_realFocusStateForFocusedElement = nonCoercedFocusState;
        }
        
        // Ensure the island has Win32 focus.
        // Even though we're not changing the focused Xaml element, the app expects setting focus to a Xaml element will
        // also set Win32 focus to the island that contains it. (Note that "Win32 focus" is distinct from "Xaml focus".
        // Xaml elements are not backed by HWNDs, so from Win32's point of view the whole island either has focus or it
        // doesn't.  Xaml tracks which Xaml element has focus for each island.  In most cases, when a Xaml element gets
        // focus, Xaml also sets Win32 focus to the element's island's HWND.)
        //
        // One scenario we hit in FileExplorer:
        //  1. A TextBox A has focus.
        //  2. The island loses focus.
        //  3. The user clicks on element B, which calls Focus() on TextBox A.
        //
        // In this case, both m_pFocusedElement and pNewFocus will be the same TextBox, so we'll wind up here. We don't
        // need to go through all the focus change logic, but we *do* want to ensure focus is on the island, because
        // that's what generally happens when Focus() is called on a control.  If the control is a TextBox, we must
        // set focus to the island so it will accept keystrokes.  Acquiring island focus is also important because
        // if the user later moves focus outside the island, we'll get a lost-focus event on the island which will
        // allow us to notify the control that it has lost focus.
        if (ShouldSetWindowFocus(movement))
        {
            // Note: This ultimately calls IInputKeyboardSource2::TrySetFocus, which may re-entrantly call back into
            // this function.
            m_contentRoot.GetFocusAdapter().SetFocus();
        }
        success = true;

        // No change in focus element - can skip the rest of this method.
        goto Cleanup;
    }

    if (movement.raiseGettingLosingEvents)
    {
        if (CanRaiseFocusEventChange() &&
            RaiseAndProcessGettingAndLosingFocusEvents(
                m_pFocusedElement,
                &pNewFocus,
                nonCoercedFocusState,
                focusNavigationDirection,
                movement.canCancel,
                correlationId))
        {
            success = true;
            focusCancelled = true;

            goto Cleanup;
        }
    }

    ASSERT((pNewFocus == nullptr) || IsFocusable(pNewFocus, true /*ignoreOffScreenPosition*/));

    // Update the previous focused control
    pOldFocusedElement = m_pFocusedElement; // Still has reference that will be freed in Cleanup.

    if (pOldFocusedElement && CFocusableHelper::IsFocusableDO(pOldFocusedElement))
    {
        CValue value;
        value.Set(DirectUI::FocusState::Unfocused, KnownTypeIndex::FocusState);
        IFC(pOldFocusedElement->SetValueByIndex(CFocusableHelper::GetIFocusableForDO(pOldFocusedElement)->GetFocusStatePropertyIndex(), value));
    }
    else if (pOldFocusedElement && pOldFocusedElement->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        static_cast<CUIElement*>(pOldFocusedElement)->UpdateFocusState(DirectUI::FocusState::Unfocused);
    }

    // Update the focused control
    m_pFocusedElement = pNewFocus;
    AddRefInterface(m_pFocusedElement);
    m_realFocusStateForFocusedElement = nonCoercedFocusState;

    if (ShouldSetWindowFocus(movement))
    {
        // Note: This ultimately calls IInputKeyboardSource2::TrySetFocus. We need to verify that we don't already have
        // focus to avoid an infinite loop from reentrancy.
        m_contentRoot.GetFocusAdapter().SetFocus();
    }

    // Bring element into view when it is focused using the keyboard, so user
    // can see the element that was tabbed to
    if (m_pFocusedElement
        && (nonCoercedFocusState == DirectUI::FocusState::Keyboard || forceBringIntoView)
        )
    {
        const CUIElement* const pFocusedElement = do_pointer_cast<CUIElement>(m_pFocusedElement);
        if (pFocusedElement)
        {
            // Note that this is done before calling UpdateFocusState, so the
            // control can use the state before it gains focus to decide whether it should
            // be brought into view when focus is gained.
            shouldBringIntoView = true;
        }
        else
        {
            // Other TabStop elements should be brought into view.
            shouldBringIntoView = IsPotentialTabStop(m_pFocusedElement);
        }
    }

    // UpdateFocusState.
    // Use pNewFocus for UpdateFocusState instead of m_pFocusedElement because the ::SetFocus call above can cause
    // reentrancy and change m_pFocusedElement.
    if (pNewFocus && CFocusableHelper::IsFocusableDO(pNewFocus))
    {
        CValue value;
        value.Set(coercedFocusState, KnownTypeIndex::FocusState);
        IFC(pNewFocus->SetValueByIndex(CFocusableHelper::GetIFocusableForDO(pNewFocus)->GetFocusStatePropertyIndex(), value));
    }
    else if (pNewFocus && pNewFocus->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // Some controls query this flag immediately - setting in OnGotFocus is too late.
        static_cast<CUIElement*>(pNewFocus)->UpdateFocusState(coercedFocusState);
    }

    // We don't need to do a complete UpdateFocusRect now (redrawing the rect), as NWDrawTree will call UpdateFocusRect
    // when it needs it. But we do want to see if we can clean up FocusRectManager's secret children, particularly in
    // the case when the previously focused item has been removed from the tree
    UpdateFocusRect(focusNavigationDirection, true /* cleanOnly */);

    // At this point the focused pointer has been switched.  So success is TRUE
    // even in the case we run into trouble raising the event(s) to notify as such.
    success = true;


    // If the new focused element is not a Text Control then we need to
    // call ClearLastSelectedTextElement since we are sure that
    // there will be no selection rect drawn on the screen.
    // This is to achieve the light text selection dismiss model.
    if (m_pFocusedElement == nullptr || !CTextCore::IsTextControl(m_pFocusedElement))
    {
        if (m_isPrevFocusTextControl)
        {
            CTextCore *pTextCore = nullptr;
            IFC(m_pCoreService->GetTextCore(&pTextCore));
            pTextCore->ClearLastSelectedTextElement();
            m_isPrevFocusTextControl = FALSE;
        }
    }
    else
    {
        m_isPrevFocusTextControl = TRUE;
    }

    // Fire focus changed event for UIAutomation
    if (m_pFocusedElement)
    {
        FireAutomationFocusChanged();
    }

    // Raise the focus Lost/GotFocus events

    // Raise the focus event while plugin has a focus, on full screen mode or on windowless mode
    if (CanRaiseFocusEventChange())
    {
        CEventManager* const pEventManager = m_pCoreService->GetEventManager();

        // Raise the LostFocus event to the old focused element
        if (pOldFocusedElement)
        {
            const bool isNavigatedToByEngagingControl = pNewFocus && NavigatedToByEngagingControl(pNewFocus);

            if (m_spEngagedControl && !isNavigatedToByEngagingControl)
            {
                IFC(m_spEngagedControl->RemoveFocusEngagement());
            }

            IFC(RaiseLostFocusEvent(pOldFocusedElement, correlationId));
        }
        else
        {
            xref_ptr<CFocusManagerLostFocusEventArgs>  spFocusManagerLostFocusEventArgs;
            spFocusManagerLostFocusEventArgs.attach(new CFocusManagerLostFocusEventArgs(nullptr, correlationId));
            // Raise the FocusManagerLostFocus event asynchronously
            pEventManager->Raise(
                EventHandle(KnownEventIndex::FocusManager_LostFocus),
                true, /*bRefire*/
                nullptr, /*sender, passing null because this is a static event*/
                spFocusManagerLostFocusEventArgs.get());
        }

        if (m_pFocusedElement)
        {
            IFC(RaiseGotFocusEvent(m_pFocusedElement, correlationId));
        }
        else
        {
            xref_ptr<CFocusManagerGotFocusEventArgs>  spFocusManagerGotFocusEventArgs;
            spFocusManagerGotFocusEventArgs.attach(new CFocusManagerGotFocusEventArgs(m_pFocusedElement, correlationId));
            // Raise the FocusManagerGotFocus event asynchronously
            pEventManager->Raise(
                EventHandle(KnownEventIndex::FocusManager_GotFocus),
                true, /*bRefire*/
                nullptr, /*sender, passing null because this is a static event*/
                spFocusManagerGotFocusEventArgs.get());
        }
    }
    else if (pOldFocusedElement)
    {
        // Update the visual state of the old focused element even when IsPluginFocused() returns false so it no longer displays focus cues.
        IFC(FxCallbacks::Control_UpdateVisualState(static_cast<CDependencyObject*>(pOldFocusedElement), TRUE /*fUseTransitions*/));
    }

    // Notify the focus changing on InputManager to ensure the focused element visible with
    // Input Host Manager
    IFC(NotifyFocusChanged(shouldBringIntoView, animateIfBringIntoView));

    IFC(m_contentRoot.GetAKExport().UpdateScope());

    // Request the playing sound for changing focus with the keyboard, gamepad or remote input
    if ((coercedFocusState == DirectUI::FocusState::Keyboard && m_contentRoot.GetInputManager().ShouldRequestFocusSound()) &&
        (lastInputDeviceType == DirectUI::InputDeviceType::Keyboard || lastInputDeviceType == DirectUI::InputDeviceType::GamepadOrRemote))
    {
        IFC(FxCallbacks::ElementSoundPlayerService_RequestInteractionSoundForElement(DirectUI::ElementSoundKind::Focus, pNewFocus));
    }

Cleanup:
    TraceUpdateFocusEnd((UINT64)pNewFocus);
    ReleaseInterface(pOldFocusedElement);

    // Before RS2, UpdateFocus did not propagate errors. As a result, we want to limit the number of failure
    // cases that callers should deal with to only those related to the new RS2 GettingFocus and LosingFocus events
    if (hr != MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING))
    {
        hr = S_OK;
    }

    const FocusMovementResult result = FocusMovementResult(success, focusCancelled, hr);

    if (m_asyncOperation != nullptr)
    {
        CancelCurrentAsyncOperation(result);
    }

    m_currentFocusOperationCancellable = true;

    return result;
}

void CFocusManager::FireAutomationFocusChanged()
{
    CAutomationPeer *pAP = nullptr;
    if (m_pFocusedElement && S_OK == m_pCoreService->UIAClientsAreListening(UIAXcp::AEAutomationFocusChanged))
    {
        pAP = m_pFocusedElement->OnCreateAutomationPeer();

        // There's one specific circumstance that we want to handle: attempting to focus a ContentControl inside a popup.
        // The ContentControl is able to be keyboard focused, but because ContentControlAutomationPeer doesn't exist,
        // we won't raise any UIA focus changed event since we have no automation peer to raise it on.
        // Since UIA clients like Narrator may be relying on this to transfer focus into an opened popup,
        // we should raise the focus changed event on the popup containing the ContentControl,
        // so that way we can ensure that UIA clients like Narrator have properly had focus trapped in the popup.
        // TODO 10588657: Undo this change when we implement a ContentControlAutomationPeer.
        if (pAP == nullptr &&
            m_pFocusedElement->OfTypeByIndex<KnownTypeIndex::ContentControl>())
        {
            xref_ptr<CUIElement> focusedElementAsUIE(static_cast<CUIElement*>(m_pFocusedElement));
            xref_ptr<CPopup> popupToFocus;

            VERIFYHR(CPopupRoot::GetOpenPopupForElement(focusedElementAsUIE.get(), popupToFocus.ReleaseAndGetAddressOf()));

            if (popupToFocus)
            {
                pAP = popupToFocus->OnCreateAutomationPeer();
            }
        }

        if (pAP)
        {
            m_pFocusedAutomationPeer = xref::get_weakref(pAP);
            if (pAP->GetAPEventsSource())
            {
                pAP = pAP->GetAPEventsSource();
            }
            pAP->RaiseAutomationEvent(UIAXcp::AEAutomationFocusChanged);
        }
    }

    if (!pAP)
    {
        m_pFocusedAutomationPeer.reset();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Raise the lost focus event asynchronously
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFocusManager::RaiseLostFocusEvent(
    CDependencyObject* pLostFocusElement,
    GUID correlationId)
{
    xref_ptr<CRoutedEventArgs> spLostFocusEventArgs;
    xref_ptr<CFocusManagerLostFocusEventArgs>  spFocusManagerLostFocusEventArgs;

    IFCPTR_RETURN(pLostFocusElement);

    CEventManager* const pEventManager = m_pCoreService->GetEventManager();
    IFCPTR_RETURN(pEventManager);

    // Create DO that represents the LostFocus event args
    spLostFocusEventArgs.attach(new CRoutedEventArgs());
    spFocusManagerLostFocusEventArgs.attach(new CFocusManagerLostFocusEventArgs(pLostFocusElement, correlationId));

    // Set the LostFocus Source value on the routed event args
    IFCFAILFAST(spLostFocusEventArgs->put_Source(pLostFocusElement));

    // Raise the LostFocus event to the old focused element asynchronously
    if (IFocusable* lostFocusElementFocusable = CFocusableHelper::GetIFocusableForDO(pLostFocusElement))
    {
        // In the case of an IFocusable raise both <IFocusable>_LostFocus and UIElement_LostFocus.
        // UIElement_LostFocus is used internally for to decide where focus rects should be rendered.
        pEventManager->Raise(
            EventHandle(lostFocusElementFocusable->GetLostFocusEventIndex()),
            true, /*bRefire*/
            pLostFocusElement,
            spLostFocusEventArgs.get(),
            false /*fRaiseSync*/
        );
    }

    pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_LostFocus),
        static_cast<CDependencyObject *>(pLostFocusElement),
        spLostFocusEventArgs.get(),
        TRUE /* bIgnoreVisibility */);

    // Raise the FocusManagerLostFocus event to the focus manager asynchronously
    pEventManager->Raise(
        EventHandle(KnownEventIndex::FocusManager_LostFocus),
        true, /*bRefire*/
        nullptr, /*sender, passing null because this is a static event*/
        spFocusManagerLostFocusEventArgs.get());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if pFocused was navigated to by engaging a control
//------------------------------------------------------------------------
bool CFocusManager::NavigatedToByEngagingControl(
    _In_ CDependencyObject* pFocused)
{
    bool isNavigatedTo = false;

    if (m_spEngagedControl)
    {
        bool isChild = false;
        bool isElementChildOfPopupOpenedDuringEngagement = false;
        bool isElementPopupOpenedDuringEngagement = false;
        bool isSelf = (m_spEngagedControl == (do_pointer_cast<CControl>(pFocused)));

        if (CFocusableHelper::IsFocusableDO(pFocused))
        {
            pFocused = GetFocusParent(pFocused);
        }

        CUIElement *pNewFocusedElement = do_pointer_cast<CUIElement>(pFocused);

        if (pNewFocusedElement)
        {
            isChild = m_spEngagedControl->IsAncestorOf(pNewFocusedElement);
        }

        CPopup * const pNewFocusedElementAsPopup = do_pointer_cast<CPopup>(pFocused);
        if (GetRootOfPopupSubTree(pNewFocusedElement) != nullptr || pNewFocusedElementAsPopup)
        {
            const auto& popupList = CPopupRoot::GetPopupChildrenOpenedDuringEngagement(pFocused);

            for (const auto& popup : popupList)
            {
                isElementPopupOpenedDuringEngagement = (popup == pNewFocusedElement);
                if (isElementPopupOpenedDuringEngagement) { break; }

                /*
                  In the case of a (Menu)Flyout, the new Focused Element could
                  be the actual popup, which is an ancestor of the (Menu)FlyoutPresenter
                  popup opened during engagement. This is a result of the fact that Flyouts (eg Button.Flyout)
                  are excluded from the Visual Tree.

                  In this situation, the new Focused element would be a Popup with no parent.
                  It's child would parent the 'popup' opened during engagement.
                */
                const bool isNewFocusedElementChildOfFlyoutOpenedDuringEngagement =
                            pNewFocusedElementAsPopup &&
                            (pNewFocusedElementAsPopup->m_pChild != nullptr) &&
                            (popup == pNewFocusedElementAsPopup->m_pChild ||
                             pNewFocusedElementAsPopup->m_pChild->IsAncestorOf(popup));

                isElementChildOfPopupOpenedDuringEngagement =
                    isNewFocusedElementChildOfFlyoutOpenedDuringEngagement
                    || popup->IsAncestorOf(pNewFocusedElement);

                if (isElementChildOfPopupOpenedDuringEngagement) { break; }
            }
        }

        /*
        The new focused element is  navigated to during Engagement if:
        1. It is the engaged control OR
        2. It is a child of the engaged control OR
        3. It is a popup opened during engagement OR
        4. It is the descendant of a popup opened during engagement
        */
        isNavigatedTo = isChild || isSelf || isElementPopupOpenedDuringEngagement || isElementChildOfPopupOpenedDuringEngagement;
    }

    return isNavigatedTo;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Raise the got focus event asynchronously
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFocusManager::RaiseGotFocusEvent(
    CDependencyObject* pGotFocusElement,
    GUID correlationId)
{
    CEventManager *pEventManager = NULL;
    xref_ptr<CRoutedEventArgs> spGotFocusEventArgs;
    xref_ptr<CFocusManagerGotFocusEventArgs> spFocusManagerGotFocusEventArgs;
    CREATEPARAMETERS cp(m_pCoreService);

    IFCPTR_RETURN(pGotFocusElement);

    pEventManager = m_pCoreService->GetEventManager();
    IFCPTR_RETURN(pEventManager);

    // Create DO that represents the GotFocus event args
    spGotFocusEventArgs.attach(new CRoutedEventArgs());
    spFocusManagerGotFocusEventArgs.attach(new CFocusManagerGotFocusEventArgs(m_pFocusedElement, correlationId));

    // Set the GotFocus Source value on the routed event args
    IFCFAILFAST(spGotFocusEventArgs->put_Source(pGotFocusElement));

    if (IFocusable* gotFocusElementFocusable = CFocusableHelper::GetIFocusableForDO(pGotFocusElement))
    {
        // In the case of an IFocusable raise both <IFocusable>_GotFocus and UIElement_GotFocus.
        // UIElement_GotFocus is used internally for to decide where focus rects should be rendered.
        pEventManager->Raise(
            EventHandle(gotFocusElementFocusable->GetGotFocusEventIndex()),
            true, /*bRefire*/
            pGotFocusElement,
            spGotFocusEventArgs.get(),
            false /*fRaiseSync*/);
    }

    // Raise the GotFocus event to the new focused element asynchronously
    pEventManager->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_GotFocus),
        static_cast<CDependencyObject *>(pGotFocusElement),
        spGotFocusEventArgs.get(),
        TRUE /* bIgnoreVisibility */);

    // Raise the FocusManagerGotFocus event to the focus manager asynchronously
    pEventManager->Raise(
        EventHandle(KnownEventIndex::FocusManager_GotFocus),
        true, /*bRefire*/
        nullptr, /*sender, passing null because this is a static event*/
        spFocusManagerGotFocusEventArgs.get());

    return S_OK;
}

CDependencyObject*
CFocusManager::RaiseFocusElementRemovedEvent(
    _In_ CDependencyObject* currentNextFocusableElement)
{
    xref_ptr<CFocusedElementRemovedEventArgs> eventArgs;
    eventArgs.attach(new CFocusedElementRemovedEventArgs(m_pFocusedElement, currentNextFocusableElement));

    VERIFYHR(FxCallbacks::JoltHelper_RaiseEvent(
        nullptr /* target */,
        ManagedEvent::ManagedEventFocusedElementRemoved /* Managed Event */,
        eventArgs /* Event Args */));

    return eventArgs->GetNewFocusedElementNoRef();
}

//------------------------------------------------------------------------
//
//  Method:   SetFocusOnNextFocusableElement
//
//  Synopsis:
//      Set the focus on the next focusable control. This method will look
//      up both the children and ancestor to find the next focusable control
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFocusManager::SetFocusOnNextFocusableElement(DirectUI::FocusState focusState, bool shouldFireFocusedRemoved, InputActivationBehavior inputActivationBehavior)
{
    CDependencyObject *pFocusable = nullptr;
    bool focusSet = true;

    // Find the next focusable element
    pFocusable = GetNextFocusableElement();
    if (pFocusable == nullptr)
    {
        // Find the first focusable element from the root
        pFocusable = GetFirstFocusableElement();
    }

    //On Xbox, we want to give them the power dictate where the focus should go when the currently focused element is
    //leaving the tree
    if (shouldFireFocusedRemoved)
    {
        pFocusable = RaiseFocusElementRemovedEvent(pFocusable);
    }

    if (pFocusable && CFocusableHelper::IsFocusableDO(pFocusable))
    {
        auto focusMovement = FocusMovement(pFocusable, DirectUI::FocusNavigationDirection::Next, focusState);
        focusMovement.requestInputActivation = (inputActivationBehavior == InputActivationBehavior::RequestActivation);
        const FocusMovementResult result = SetFocusedElement(focusMovement);
        focusSet = result.WasMoved();
        IFC_RETURN(result.GetHResult());
    }

    // When the candidate element has AllowFocusOnInteraction set to false, we should still set focus on this element
    if (auto focusableCast = do_pointer_cast<CFrameworkElement>(pFocusable))
    {
        if (!Focus::FocusSelection::ShouldUpdateFocus(focusableCast, focusState))
        {
            focusState = FocusState::Programmatic;
        }
    }


    CUIElement* pUIElement = do_pointer_cast<CUIElement>(pFocusable);
    if (!pFocusable || !focusSet)
    {
        ClearFocus();
    }
    else if (pUIElement)
    {
        bool focusUpdated = false;
        // FUTURE: Why are we setting Focus again here when we already called SetFocusedElement() above?
        IFC_RETURN(pUIElement->Focus(focusState, false /*animateIfBringIntoView*/, &focusUpdated, DirectUI::FocusNavigationDirection::None, inputActivationBehavior));
        if (!focusUpdated)
        {
            // Failed to set focus. We need to clean the focus state
            ClearFocus();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TryMoveFocus
//
//  Synopsis:
//
//      Move the focus to the specified navigation direction(Next/Previous).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFocusManager::TryMoveFocus(
    _In_ FocusNavigationDirection focusNavigationDirection,
    _Out_ bool* pbMoved)
{
    IFCPTR_RETURN(pbMoved);
    *pbMoved = FALSE;
    Focus::XYFocusOptions xyFocusOptions;
    const FocusMovementResult result = FindAndSetNextFocus(FocusMovement(xyFocusOptions, focusNavigationDirection, nullptr));
    IFC_RETURN(result.GetHResult());
    *pbMoved = result.WasMoved();
    return S_OK;
}

bool
CFocusManager::FindAndSetNextFocus(
    _In_ FocusNavigationDirection direction)
{
    Focus::XYFocusOptions xyFocusOptions;
    const FocusMovementResult result = FindAndSetNextFocus(FocusMovement(xyFocusOptions, direction, nullptr));
    IFCFAILFAST(result.GetHResult());
    return result.WasMoved();
}

FocusMovementResult
CFocusManager::FindAndSetNextFocus(_In_ const FocusMovement& movement)
{
    HRESULT hr = S_OK;
    FocusMovementResult result;

    const FocusNavigationDirection direction = movement.GetDirection();
    bool queryOnly = true;

    ASSERT(direction == DirectUI::FocusNavigationDirection::Down || direction == DirectUI::FocusNavigationDirection::Left ||
        direction == DirectUI::FocusNavigationDirection::Right || direction == DirectUI::FocusNavigationDirection::Up ||
        direction == DirectUI::FocusNavigationDirection::Next || direction == DirectUI::FocusNavigationDirection::Previous);

    ASSERT(movement.GetXYFocusOptions());
    Focus::XYFocusOptions &xyFocusOptions = *movement.GetXYFocusOptions();

    if (m_focusLocked)
    {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
        IFC(ErrorHelper::OriginateErrorUsingResourceID(hr, ERROR_FOCUSMANAGER_MOVING_FOCUS));
    }

    if (xyFocusOptions.updateManifoldsFromFocusHintRect && xyFocusOptions.focusHintRectangle!=nullptr)
    {
        XYFocus::Manifolds manifolds;
        manifolds.hManifold = std::make_pair(xyFocusOptions.focusHintRectangle->top, xyFocusOptions.focusHintRectangle->bottom);
        manifolds.vManifold = std::make_pair(xyFocusOptions.focusHintRectangle->left, xyFocusOptions.focusHintRectangle->right);
        m_xyFocus.SetManifolds(manifolds);
    }

    queryOnly = !m_contentRoot.GetFocusAdapter().ShouldDepartFocus(direction);

    if (CDependencyObject* const nextFocusedElement = FindNextFocus(FindFocusOptions(direction, queryOnly), xyFocusOptions, movement.GetTarget(), false))
    {
        result = SetFocusedElement(FocusMovement(nextFocusedElement, movement));
        IFC(result.GetHResult());

        if (result.WasMoved() && !result.WasCanceled() && xyFocusOptions.updateManifold)
        {
            const XRECTF_RB bounds = xyFocusOptions.focusHintRectangle != nullptr ? *xyFocusOptions.focusHintRectangle : xyFocusOptions.focusedElementBounds;
            m_xyFocus.UpdateManifolds(direction, bounds, nextFocusedElement, xyFocusOptions.ignoreClipping);
        }
    }
    else if (movement.canDepartFocus)
    {
        bool bFocusDeparted = false;
        IFC(m_focusObserver->DepartFocus(direction, movement.GetCorrelationId(), &bFocusDeparted));
    }

Cleanup:
    return FocusMovementResult(hr, result);
}

CDependencyObject* CFocusManager::FindNextFocus(_In_ FocusNavigationDirection direction)
{
    Focus::XYFocusOptions options;
    return FindNextFocus(FindFocusOptions(direction), options);
}

CDependencyObject* CFocusManager::FindNextFocus(
    _In_ const FindFocusOptions& findFocusOptions,
    _In_ Focus::XYFocusOptions& xyFocusOptions,
    _In_opt_ CDependencyObject* component,
    _In_opt_ bool updateManifolds)
{
    DirectUI::FocusNavigationDirection direction = findFocusOptions.GetDirection();
    ASSERT(direction == DirectUI::FocusNavigationDirection::Down || direction == DirectUI::FocusNavigationDirection::Left ||
        direction == DirectUI::FocusNavigationDirection::Right || direction == DirectUI::FocusNavigationDirection::Up ||
        direction == DirectUI::FocusNavigationDirection::Next || direction == DirectUI::FocusNavigationDirection::Previous);

     switch (direction)
     {
         case DirectUI::FocusNavigationDirection::Next:
           TraceXYFocusEnteredBegin(L"Next");
           break;
         case DirectUI::FocusNavigationDirection::Previous:
           TraceXYFocusEnteredBegin(L"Previous");
           break;
         case DirectUI::FocusNavigationDirection::Up:
           TraceXYFocusEnteredBegin(L"Up");
           break;
         case DirectUI::FocusNavigationDirection::Down:
           TraceXYFocusEnteredBegin(L"Down");
           break;
         case DirectUI::FocusNavigationDirection::Left:
           TraceXYFocusEnteredBegin(L"Left");
           break;
         case DirectUI::FocusNavigationDirection::Right:
           TraceXYFocusEnteredBegin(L"Right");
           break;
         default:
           TraceXYFocusEnteredBegin(L"Invalid");
     }

    xref_ptr<CDependencyObject> nextFocusedElement;
    CControl* const engagedControl = xyFocusOptions.considerEngagement ? m_spEngagedControl : nullptr;

    //If we're hosting a component (for e.g. WebView) and focus is moving from within one of our hosted component's children,
    //we interpret the component (WebView) as previously focused element
    auto currentFocusedElementOrComponent = (component == nullptr) ? m_pFocusedElement : component;

    if (direction == DirectUI::FocusNavigationDirection::Previous ||
        direction == DirectUI::FocusNavigationDirection::Next ||
        currentFocusedElementOrComponent == nullptr)
    {
        const bool bPressedShift = direction == DirectUI::FocusNavigationDirection::Previous;

        // Get the move candidate element according to next/previous navigation direction.
        if (findFocusOptions.IsQueryOnly())
        {
            if (FAILED(ProcessTabStopInternal(bPressedShift, true /*queryOnly*/, nextFocusedElement.ReleaseAndGetAddressOf())))
            {
                return nullptr;
            }
        }
        else
        {
            auto scopeGuard = wil::scope_exit([&]
            {
                if (bPressedShift)
                {
                    m_isMovingFocusToPreviousTabStop = false;
                }
                else if (direction == DirectUI::FocusNavigationDirection::Next)
                {
                    m_isMovingFocusToNextTabStop = false;
                }
            });

            if (bPressedShift)
            {
                m_isMovingFocusToPreviousTabStop = true;
            }
            else if (direction == DirectUI::FocusNavigationDirection::Next)
            {
                m_isMovingFocusToNextTabStop = true;
            }

            if (FAILED(ProcessTabStopInternal(bPressedShift, false /*queryOnly*/, nextFocusedElement.ReleaseAndGetAddressOf())))
            {
                return nullptr;
            }
        }
    }
    else
    {
        {
            CUIElement* elementAsUI = do_pointer_cast<CUIElement>(currentFocusedElementOrComponent);

            if (elementAsUI == nullptr && CFocusableHelper::IsFocusableDO(currentFocusedElementOrComponent))
            {
                elementAsUI = do_pointer_cast<CUIElement>(GetFocusParent(currentFocusedElementOrComponent));
            }

            VERIFYHR(elementAsUI->GetGlobalBoundsLogical(&xyFocusOptions.focusedElementBounds, xyFocusOptions.ignoreClipping));
        }

        nextFocusedElement = m_xyFocus.GetNextFocusableElement(direction, currentFocusedElementOrComponent, engagedControl, m_contentRoot.GetVisualTreeNoRef(), updateManifolds, xyFocusOptions);
    }

    TraceXYFocusEnteredEnd();

    return nextFocusedElement;
}

// Get the focus target for this element if one exists (may return null, or a child element of given element)
/*static*/ CUIElement* CFocusManager::GetFocusTargetDescendant(_In_ CUIElement* element)
{
    if (element->OfTypeByIndex<KnownTypeIndex::Control>())
    {
        CValue focusTargetDescendantValue;

        const CDependencyProperty *pdp = element->GetPropertyByIndexInline(KnownPropertyIndex::Control_FocusTargetDescendant);
        IFCFAILFAST(element->GetValue(pdp, &focusTargetDescendantValue));

        CDependencyObject* pDO = nullptr;
        IFCFAILFAST(CValueBoxer::UnwrapWeakRef(&focusTargetDescendantValue, pdp, &pDO));

        return do_pointer_cast<CUIElement>(pDO);
    }
    return nullptr;
}

// Get the element we need to draw the focus rect on.  Returning nullptr will cause the focus rectangle
// to not be drawn
CDependencyObject* CFocusManager::GetFocusTarget()
{
    CDependencyObject* candidate = GetFocusedElementNoRef();

    if (!candidate)
    {
        xref_ptr<CUIElement> focusRectangleUIElement = GetFocusRectangleUIElement().lock();
        if (focusRectangleUIElement && focusRectangleUIElement->IsActive())
        {
            candidate = focusRectangleUIElement.get();
        }
    }

    if (!candidate)
    {
        return nullptr;
    }

    if (!IsPluginFocused())
    {
        return nullptr;
    }

    // don't draw focus rect for disabled FE(Got focus because of AllowFocusWhenDisabled is set)
    CFrameworkElement* candidateAsFE = do_pointer_cast<CFrameworkElement>(candidate);
    if (candidateAsFE
        && candidateAsFE->AllowFocusWhenDisabled()
        && !candidateAsFE->IsEnabled())
    {
        return nullptr;
    }

    // Test for if the element doesn't have a child to draw to.
    CUIElement* candidateAsElement = do_pointer_cast<CUIElement>(candidate);
    if (candidateAsElement
        && candidateAsElement->IsFocused()
        && candidateAsElement->IsKeyboardFocused())
    {
        const CDependencyProperty *pFocusProperty = candidateAsElement->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_UseSystemFocusVisuals);
        CValue useSystemFocusVisualsValue;
        IFCFAILFAST(candidateAsElement->GetValue(pFocusProperty, &useSystemFocusVisualsValue));

        if (useSystemFocusVisualsValue.AsBool())
        {
            // Remember the focusTarget, it's different from the focused element in this case
            CUIElement* focusTargetDescendant = GetFocusTargetDescendant(candidateAsElement);
            if (focusTargetDescendant)
            {
                return focusTargetDescendant;
            }
            else
            {
                return candidateAsElement;
            }
        }
    }

    // For Hyperlinks:
    //  * If we're not in high-visibility mode, CRichTextBlock::HWRenderFocusRects will do the drawing
    if (m_focusRectManager.AreHighVisibilityFocusRectsEnabled())
    {
        CTextElement* textElement = GetTextElementForFocusRectCandidate();
        if (textElement != nullptr)
        {
            return textElement;
        }
    }

    return nullptr;
}

CTextElement* CFocusManager::GetTextElementForFocusRectCandidate() const
{
    // Draw focus rect for HyperLink with conditions:
    // - Window focused or we are delegating input (GameBar does this)
    // - not in special case when focus is gained by SIP keyboard input
    // - last input device is keyboard or gamepad
    if (IsPluginFocused())
    {
        CDependencyObject* focusedElement = GetFocusedElementNoRef();
        if (focusedElement && CFocusableHelper::IsFocusableDO(focusedElement))
        {
            CFocusManager *focusManager = VisualTree::GetFocusManagerForElement(focusedElement);
            CInputManager& inputManager = m_contentRoot.GetInputManager();
            if (inputManager.GetLastInputDeviceType() == DirectUI::InputDeviceType::Keyboard &&
                inputManager.LastInputWasNonFocusNavigationKeyFromSIP())
            {
                return nullptr;
            }
            const bool lastInputDeviceWasKeyboardWithProgrammaticFocusState = focusManager &&
                (focusManager->GetRealFocusStateForFocusedElement() == DirectUI::FocusState::Programmatic) &&
                (inputManager.GetLastInputDeviceType() == DirectUI::InputDeviceType::Keyboard
                    || inputManager.GetLastInputDeviceType() == DirectUI::InputDeviceType::GamepadOrRemote);

            const bool currentFocusStateIsKeyboard = focusManager &&
                (focusManager->GetRealFocusStateForFocusedElement() == DirectUI::FocusState::Keyboard);

            if (currentFocusStateIsKeyboard || lastInputDeviceWasKeyboardWithProgrammaticFocusState)
            {
                // TODO: don't assume this will alays be a CTextElement going forward
                return static_cast<CTextElement*>(focusedElement);
            }
        }
    }
    return nullptr;
}

void CFocusManager::UpdateFocusRect(_In_ const DirectUI::FocusNavigationDirection focusNavigationDirection, _In_ bool cleanOnly)
{
    //
    // The focus rect LayoutTransitionElement and Canvas are secretly parented to the focus target, and the UpdateFocusRect
    // call can clean them up which propagates dirty flags up the tree. There are scenarios where this is the last reference
    // to the old focus target, so make sure we keep it alive for this process to avoid calling into a released element.
    //
    // We're seeing this setup in explorer's context menus which use islands. When the popup closes and the controls inside
    // leave the tree Xaml is supposed to find the next focusable element and shift focus away (CControl::LeaveImpl ->
    // CFocusManager::SetFocusOnNextFocusableElement), but CFocusManager::GetNextFocusableElement is returning the currently
    // focused element as the next eligible one so we don't actually change focus. However, the entire island has lost focus,
    // so the CFocusManager::GetFocusTarget call below reports nullptr for the focused element, and we do a cleanup in
    // UpdateFocusRect.
    //
    xref_ptr<CDependencyObject> keepAlive = m_focusTarget;
    m_focusTarget = GetFocusTarget();
    m_focusRectManager.UpdateFocusRect(m_pCoreService, GetFocusedElementNoRef(), m_focusTarget, focusNavigationDirection,  cleanOnly);
}

void CFocusManager::OnFocusedElementKeyPressed() const
{
    m_focusRectManager.OnFocusedElementKeyPressed();
}

void CFocusManager::OnFocusedElementKeyReleased() const
{
    m_focusRectManager.OnFocusedElementKeyReleased();
}

void CFocusManager::RenderFocusRectForElementIfNeeded(_In_ CUIElement* element, _In_ IContentRenderer* renderer)
{
    if (element == m_focusTarget)
    {
        m_focusRectManager.RenderFocusRectForElement(element, renderer);
    }
}

// Call when properties of focus visual change.  Specifically called when Application::FocusVisualKind
// changes.
void CFocusManager::SetFocusVisualDirty()
{
    CDependencyObject* focusedObject = GetFocusedElementNoRef();
    CUIElement* focusedElement = do_pointer_cast<CUIElement>(focusedObject);
    if (focusedElement)
    {
        CUIElement::NWSetContentDirty(focusedElement, DirtyFlags::Render);
    }
    else
    {
        if (focusedObject && CFocusableHelper::IsFocusableDO(focusedObject))
        {
            CFrameworkElement* hyperlinkHost = CFocusableHelper::GetContainingFrameworkElementIfFocusable(focusedObject);
            if (hyperlinkHost)
            {
                CUIElement::NWSetContentDirty(hyperlinkHost, DirtyFlags::Render);
            }
        }
    }
}

_Check_return_ HRESULT CFocusManager::OnAccessKeyDisplayModeChanged() const
{
    // We should update the caret to visible/collapsed depending on if AK mode is active
    CTextBoxBase* pTextBox = do_pointer_cast<CTextBoxBase>(m_pFocusedElement);

    if (pTextBox)
    {
        IFC_RETURN(pTextBox->GetView()->ShowOrHideCaret());
    }

    return S_OK;
}

//Returns true if the event was cancelled.
template <class ChangingFocusEventArgs>
bool CFocusManager::RaiseChangingFocusEvent(
    _In_ CDependencyObject* const pLosingFocusElement,
    _In_ CDependencyObject* pGettingFocusElement,
    _In_ DirectUI::FocusState newFocusState,
    _In_ DirectUI::FocusNavigationDirection navigationDirection,
    _In_ KnownEventIndex index,
    _In_ GUID correlationId,
    _Outptr_ CDependencyObject** pFinalGettingFocusElement)
{
    auto scopeGuard = wil::scope_exit([&]
    {
        m_focusLocked = false;
    });

    //Locking focus to prevent Focus changes in Getting/Losing focus handlers
    m_focusLocked = true;

    xref_ptr <ChangingFocusEventArgs> pChangingFocusEventArgs;
    CEventManager* const pEventManager = m_pCoreService->GetEventManager();

    DirectUI::FocusInputDeviceKind deviceKind =  m_contentRoot.GetInputManager().GetLastFocusInputDeviceKind();

    pChangingFocusEventArgs.attach(new ChangingFocusEventArgs(
        pLosingFocusElement,
        pGettingFocusElement,
        newFocusState,
        navigationDirection,
        deviceKind,
        m_currentFocusOperationCancellable,
        correlationId
    ));

    CUIElement *pChangingFocusTarget = nullptr;
    if (index == KnownEventIndex::UIElement_LosingFocus)
    {
        pChangingFocusTarget = do_pointer_cast<CUIElement>(pLosingFocusElement);
    }
    else if (index == KnownEventIndex::UIElement_GettingFocus)
    {
        pChangingFocusTarget = do_pointer_cast<CUIElement>(pGettingFocusElement);
    }


    if (pChangingFocusTarget)
    {
        IFCFAILFAST(pChangingFocusEventArgs->put_Source(pChangingFocusTarget));

        pEventManager->RaiseRoutedEvent(
            EventHandle(index),
            pChangingFocusTarget,
            pChangingFocusEventArgs,
            TRUE /*bIgnoreVisibility*/,
            TRUE /*fRaiseSync*/,
            FALSE /*fInputEvent*/
        );

    }

    //Always raises FocusManagerGettingFocus/LosingFocus synchronous event.
    VERIFYHR(FxCallbacks::JoltHelper_RaiseEvent(
        nullptr, /*sender, passing null for focus manager*/
        index == KnownEventIndex::UIElement_GettingFocus ? DirectUI::ManagedEvent::ManagedEventGettingFocus : DirectUI::ManagedEvent::ManagedEventLosingFocus,
        pChangingFocusEventArgs));

    *pFinalGettingFocusElement = pChangingFocusEventArgs->m_newFocusedElement;

    // Check if :
    // 1. Focus was redirected
    // 2. The element to which we are redirecting focus is focusable.
    // If this element is not focusable, look for the focusable child.
    // If there is no focusable child, cancel the redirection.
    if ((*pFinalGettingFocusElement != pGettingFocusElement) &&
        *pFinalGettingFocusElement &&
        !IsFocusable(*pFinalGettingFocusElement))
    {
        CDependencyObject *pChildFocus = nullptr;

        if (navigationDirection == FocusNavigationDirection::Previous)
        {
            pChildFocus = GetLastFocusableElement(*pFinalGettingFocusElement);
        }
        else
        {
            pChildFocus = GetFirstFocusableElement(*pFinalGettingFocusElement);
        }

        *pFinalGettingFocusElement = pChildFocus;
    }

    //We cancel the focus change if:
    //1. The Cancel flag on the args is set
    //2. The focus target is the same as the old focused element
    //3. The focus was redirected to null
    //4. The focus target after a redirection is not focusable and has no focusable children
    if (pChangingFocusEventArgs->m_bCancel
        || (*pFinalGettingFocusElement == pLosingFocusElement)
        || (pGettingFocusElement && (*pFinalGettingFocusElement == nullptr)))
    {
        return true;
    }
    return false;
}

//Returns true if the focus change is cancelled
bool CFocusManager::RaiseAndProcessGettingAndLosingFocusEvents(
    _In_ CDependencyObject* const pOldFocus,
    _Inout_opt_ CDependencyObject** pFocusTarget,
    _In_ DirectUI::FocusState focusState,
    _In_ DirectUI::FocusNavigationDirection focusNavigationDirection,
    _In_ const bool focusChangeCancellable,
    _In_ GUID correlationId)
{
    CDependencyObject* pFinalNewFocus = nullptr;
    CDependencyObject *pNewFocus = pFocusTarget ? (*pFocusTarget) : nullptr;

    m_currentFocusOperationCancellable = m_currentFocusOperationCancellable && focusChangeCancellable;

    bool focusRedirected = false;

    if (RaiseChangingFocusEvent<CLosingFocusEventArgs>(
        pOldFocus,
        pNewFocus,
        focusState,
        focusNavigationDirection,
        KnownEventIndex::UIElement_LosingFocus,
        correlationId,
        &pFinalNewFocus))
    {
        return true;
    }

    if (pNewFocus != pFinalNewFocus)
    {
        focusRedirected = true;
    }

    if (!focusRedirected)
    {
        pFinalNewFocus = nullptr;

        if (RaiseChangingFocusEvent<CGettingFocusEventArgs>(
            pOldFocus,
            pNewFocus,
            focusState,
            focusNavigationDirection,
            KnownEventIndex::UIElement_GettingFocus,
            correlationId,
            &pFinalNewFocus))
        {
            return true;
        }

        if (pNewFocus != pFinalNewFocus)
        {
            focusRedirected = true;
        }
    }

    if (focusRedirected)
    {
        if (!ShouldUpdateFocus(pFinalNewFocus, focusState))
        {
            return true;
        }

        if (RaiseAndProcessGettingAndLosingFocusEvents(
            pOldFocus,
            &pFinalNewFocus,
            focusState,
            focusNavigationDirection,
            focusChangeCancellable,
            correlationId))
        {
            return true;
        }

        if (pFocusTarget)
        {
            *pFocusTarget = pFinalNewFocus;
        }

    }
    return false;
}

bool CFocusManager::IsValidTabStopSearchCandidate(
    _In_ CDependencyObject* const element) const
{
    bool isValid = IsPotentialTabStop(element);

    // If IsPotentialTabStop is false, we could have a UIElement that has TabFocusNavigation Set. If it does, then
    // it is still a valid search candidate
    if (isValid == false)
    {
        // We only care if we have a UIElement has TabFocusNavigation set
        isValid = element->OfTypeByIndex<KnownTypeIndex::UIElement>() &&
            static_cast<CUIElement*>(element)->IsTabNavigationSet();
    }

    return isValid;
}

void CFocusManager::RaiseNoFocusCandidateFoundEvent(
    _In_ DirectUI::FocusNavigationDirection navigationDirection)
{
    CUIElement *noFocusCandidateFoundTarget = do_pointer_cast<CUIElement>(m_pFocusedElement);
    CTextElement* focusedElementAsTextElement = do_pointer_cast<CTextElement>(m_pFocusedElement);

    switch (navigationDirection)
    {
        case DirectUI::FocusNavigationDirection::Next:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Next");
            break;
        case DirectUI::FocusNavigationDirection::Previous:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Previous");
            break;
        case DirectUI::FocusNavigationDirection::Up:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Up");
            break;
        case DirectUI::FocusNavigationDirection::Down:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Down");
            break;
        case DirectUI::FocusNavigationDirection::Left:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Left");
            break;
        case DirectUI::FocusNavigationDirection::Right:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Right");
            break;
        default:
            TraceXYFocusNotFoundInfo((UINT64)m_pFocusedElement, L"Invalid");
    }

    if (focusedElementAsTextElement)
    {
        //We get the containing framework element for all text elements till Bug 10065690 is resolved.
        noFocusCandidateFoundTarget = focusedElementAsTextElement->GetContainingFrameworkElement();
    }

    GUID correlationId = {};
    UuidCreate(&correlationId);
    bool bHandled = false;
    IFCFAILFAST(m_focusObserver->DepartFocus(navigationDirection, correlationId, &bHandled));

    //We should never raise on a NULL source
    if (noFocusCandidateFoundTarget)
    {
        xref_ptr <CNoFocusCandidateFoundEventArgs> pNoFocusCandidateFoundEventArgs = nullptr;
        CEventManager* const pEventManager = m_pCoreService->GetEventManager();
        DirectUI::FocusInputDeviceKind deviceKind = m_contentRoot.GetInputManager().GetLastFocusInputDeviceKind();

        pNoFocusCandidateFoundEventArgs.attach(new CNoFocusCandidateFoundEventArgs(
            navigationDirection,
            deviceKind
        ));

        //The source should be the focused element
        IFCFAILFAST(pNoFocusCandidateFoundEventArgs->put_Source(m_pFocusedElement));

        pEventManager->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_NoFocusCandidateFound),
            noFocusCandidateFoundTarget,
            pNoFocusCandidateFoundEventArgs,
            TRUE /*bIgnoreVisibility*/,
            TRUE /*fRaiseSync*/,
            FALSE /*fInputEvent*/
        );
    }
}

void CFocusManager::SetPluginFocusStatus(_In_ bool pluginFocused)
{
    m_bPluginFocused = pluginFocused;
}

// When setting focus to a new element and we don't have a FocusState to use, this function helps us
// decide a good focus state based on a recently-used input device type.  This function assumes focus is
// actually being set, so it won't return "Unfocued" as a FocusState.
/*static*/
DirectUI::FocusState CFocusManager::GetFocusStateFromInputDeviceType(DirectUI::InputDeviceType inputDeviceType)
{
    switch (inputDeviceType)
    {
        case DirectUI::InputDeviceType::None:
            return DirectUI::FocusState::Programmatic;
        case DirectUI::InputDeviceType::Mouse:
        case DirectUI::InputDeviceType::Touch:
        case DirectUI::InputDeviceType::Pen:
            return DirectUI::FocusState::Pointer;
        case DirectUI::InputDeviceType::Keyboard:
        case DirectUI::InputDeviceType::GamepadOrRemote:
            return DirectUI::FocusState::Keyboard;
    }

    // Unexpected inputDeviceType
    ASSERT(FALSE);
    return DirectUI::FocusState::Programmatic;
}

bool CFocusManager::TrySetAsyncOperation(_In_ ICoreAsyncFocusOperation* asyncOperation)
{
    if (m_asyncOperation != nullptr) { return false; }

    m_asyncOperation = asyncOperation;
    return true;
}

void CFocusManager::CancelCurrentAsyncOperation(_In_ const Focus::FocusMovementResult& result)
{
    if (m_asyncOperation != nullptr)
    {
        m_asyncOperation->CoreSetResults(result);
        m_asyncOperation->CoreFireCompletion();
        m_asyncOperation->CoreReleaseRef();

        m_asyncOperation = nullptr;
    }
}

DirectUI::FocusState CFocusManager::CoerceFocusState(_In_ DirectUI::FocusState focusState) const
{
    const DirectUI::InputDeviceType lastInputDeviceType = m_contentRoot.GetInputManager().GetLastInputDeviceType();

    // Set the new focus state with the last input device type
    // if the focus state set as programmatic.
    if (focusState == DirectUI::FocusState::Programmatic)
    {
        /*
        * On Programmatic focus, we look at the last input device to decide what could be the focus state.
        * Depending on focus state we decide whether to show a focus rectangle or not.
        * If focus state is keyboard we always show a rectangle.
        * Focus state will be set to Keyboard iff last input device was
        * a. GamepadOrRemote
        * With GamepadOrRemote, we always show focus rectangle on Xbox unless developer explicitly wants us not to do so,
        * in that case it will be set to FocusState::Pointer. Even, if SIP is open and input is any key
        * we will display focus rectangle if user is using GamepadOrRemote to type on SIP
        * b. Hardware Keyboard
        * c. Software keyboard (SIP) and focus movement keys (Tab or Arrow keys)
        * Otherwise we set focus state to default FocusState::Pointer state. Setting focus state to Pointer
        * will make sure that focus rectangle will not be drawn
        */
        switch (lastInputDeviceType)
        {
        case DirectUI::InputDeviceType::Keyboard:
            // In case of non focus navigation keys from SIP, we do not want to display
            // focus rectangle, hence setting new focus state as a Pointer.
            if (m_contentRoot.GetInputManager().LastInputWasNonFocusNavigationKeyFromSIP())
            {
                return DirectUI::FocusState::Pointer;
            }
            /*
            * Intentional fall through in the case where input is
            * a. any one of focus navigation key(Tab or an arrow key) from SIP or
            * b. any key from actual hardware keyboard
            */
            __fallthrough;
        case DirectUI::InputDeviceType::GamepadOrRemote:
            return DirectUI::FocusState::Keyboard;
        default:
            // For all other rest of the cases, setting focus state to Pointer
            // will make sure that focus rectangle will not be drawn
            return DirectUI::FocusState::Pointer;
        }
    }

    return focusState;
}

_Check_return_ HRESULT CFocusManager::SetWindowFocus(
    _In_ const bool isFocused,
    _In_ const bool isShiftDown)
{
    xref_ptr<CRoutedEventArgs> args;

    GUID correlationId = {};
    UuidCreate(&correlationId);

    SetPluginFocusStatus(isFocused);

    CDependencyObject* focusedElement = m_pFocusedElement;

    // We cache the value of UISettings.AnimationsEnabled to avoid the expensive call
    // to get it all the time. Unfortunately there is no notification mechanism yet when it
    // changes. We work around that by re-evaluating it only when the app window focus changes.
    // This is usually the case because in order to change the setting the user needs to switch
    // to the settings app and back.
    m_pCoreService->SetShouldReevaluateIsAnimationEnabled(true);

    if(isFocused && focusedElement && ((m_contentRoot.GetInputManager().GetLastInputDeviceType() == InputDeviceType::Mouse)
                                   || (m_contentRoot.GetInputManager().GetLastInputDeviceType() == InputDeviceType::Touch)
                                   || (m_contentRoot.GetInputManager().GetLastInputDeviceType() == InputDeviceType::Pen)))
    {
        // When user is coming back to the window, we restore the focus while may be a click event going to some other control, 
        // so we need to skip a few rendering frames to let the click event be processed to avoid rendering focus state for a control which may be able to lose focus to the clicked control
        m_pCoreService->SkipFrames(5);
    }

    if (focusedElement == nullptr && isFocused)
    {
        // Find the first focusable element from the root
        // and set it as the new focused element.
        focusedElement = static_cast<CDependencyObject*>(GetFirstFocusableElementFromRoot(isShiftDown /*bReverse*/));
        if (focusedElement)
        {
            DirectUI::FocusState initialFocusState = DirectUI::FocusState::Programmatic;
            if (m_contentRoot.GetInputManager().GetLastInputDeviceType() == InputDeviceType::GamepadOrRemote)
            {
                initialFocusState = DirectUI::FocusState::Keyboard;
            }

            InitialFocusSIPSuspender setInitalFocusTrue(this);
            //If an error is propagated to the Input Manager here, we are in an invalid state.

            const FocusMovementResult result = SetFocusedElement(FocusMovement(focusedElement, DirectUI::FocusNavigationDirection::None, initialFocusState));
            IFCFAILFAST(result.GetHResult());

            // Note that because of focus redirection, m_pFocusedElement can be different than focusedElement
            // For RS5, we are NOT going to fix this, so we will leave the next line commented.
            // focusedElement = m_pFocusedElement;

            return S_OK;
         }
    }

    if (focusedElement == nullptr)
    {
        focusedElement = static_cast<CDependencyObject*>(m_contentRoot.GetVisualTreeNoRef()->GetPublicRootVisual());
    }

    if (focusedElement)
    {
        // Create the DO that represents the event args
        IFC_RETURN(args.init(new CRoutedEventArgs()));

        // Call raise event AND reset handled to false if this is not ours
        if (isFocused)
        {
            CDependencyObject* focusTarget = focusedElement;
            bool wasFocusChangedDuringSyncEvents = false;

            UnsafeFocusLockOverrideGuard focusLockGuard(this);
            CDependencyObject* currentlyFocusedElement = m_pFocusedElement;

            // Raise changing focus events. We cannot cancel or redirect focus here.
            // RaiseAndProcessGettingAndLosingFocusEvents returns true if a focus change has been cancelled.
            const bool focusChangeCancelled = RaiseAndProcessGettingAndLosingFocusEvents(
                nullptr /*pOldFocus*/,
                &focusTarget,
                GetRealFocusStateForFocusedElement(),
                DirectUI::FocusNavigationDirection::None,
                false /*focusChangeCancellable*/,
                correlationId);

            ASSERT(!focusChangeCancelled);
            SetCurrentFocusOperationCancellable(true);

            // The focused element could have changed after raising the getting/losing events
            if (currentlyFocusedElement != m_pFocusedElement)
            {
                wasFocusChangedDuringSyncEvents = true;
                focusedElement = static_cast<CDependencyObject*>(m_pFocusedElement);
            }

            // Set the Source value on the routed event args
            IFC_RETURN(args->put_Source(focusedElement));

            // We only want to set the focused element as dirty if it is a uielement
            if (focusedElement->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                CUIElement::NWSetContentDirty(focusedElement, DirtyFlags::Render);
            }

            // If focus changed, then the GotFocus event has already been raised for the focused element
            if (wasFocusChangedDuringSyncEvents == false)
            {
                // In the case of hyperlink raise both Hyperlink_GotFocus and UIElement_GotFocus. UIElement_GotFocus
                // is used internally for to decide where focus rects should be rendered.
                if (focusedElement->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
                {
                    m_pCoreService->GetEventManager()->Raise(
                        EventHandle(KnownEventIndex::Hyperlink_GotFocus),
                        true, /*bRefire*/
                        focusedElement,
                        args.get(),
                        false /* fRaiseSync */
                    );
                }

                // Raise the current focused element
                m_pCoreService->GetEventManager()->RaiseRoutedEvent(
                    EventHandle(KnownEventIndex::UIElement_GotFocus),
                    focusedElement,
                    args.get(),
                    TRUE /* bIgnoreVisibility */);

                xref_ptr<CFocusManagerLostFocusEventArgs> spFocusManagerLostFocusEventArgs;
                spFocusManagerLostFocusEventArgs.attach(new CFocusManagerLostFocusEventArgs(nullptr, correlationId));
                // Raise the FocusManagerLostFocus event to the focus manager asynchronously
                m_pCoreService->GetEventManager()->Raise(
                    EventHandle(KnownEventIndex::FocusManager_LostFocus),
                    true, /*bRefire*/
                    nullptr, /*sender, passing null because this is a static event*/
                    spFocusManagerLostFocusEventArgs.get());

                xref_ptr<CFocusManagerGotFocusEventArgs> spFocusManagerGotFocusEventArgs;
                spFocusManagerGotFocusEventArgs.attach(new CFocusManagerGotFocusEventArgs(focusedElement, correlationId));
                // Raise the FocusManagerGotFocus event to the focus manager asynchronously
                m_pCoreService->GetEventManager()->Raise(
                    EventHandle(KnownEventIndex::FocusManager_GotFocus),
                    true, /*bRefire*/
                    nullptr, /*sender, passing null because this is a static event*/
                    spFocusManagerGotFocusEventArgs.get());
            }

            // Fire focus changed event for UIAutomation
            FireAutomationFocusChanged();
        }
        else
        {
            UnsafeFocusLockOverrideGuard focusLockGuard(this);
            CDependencyObject* currentlyFocusedElement = m_pFocusedElement;

            // Raise changing focus events. We cannot cancel or redirect focus here.
            // RaiseAndProcessGettingAndLosingFocusEvents returns true if a focus change has been cancelled.
            const bool focusChangeCancelled = RaiseAndProcessGettingAndLosingFocusEvents(
                focusedElement,
                nullptr /*pFocusTarget*/,
                DirectUI::FocusState::Unfocused,
                DirectUI::FocusNavigationDirection::None,
                false /*focusChangeCancellable*/,
                correlationId);

            ASSERT(!focusChangeCancelled);
            SetCurrentFocusOperationCancellable(true);

            // The focused element could have changed after raising the getting/losing events
            if (currentlyFocusedElement != m_pFocusedElement)
            {
                focusedElement = static_cast<CDependencyObject*>(m_pFocusedElement);
            }

            // Set the Source value on the routed event args
            IFC_RETURN(args->put_Source(focusedElement));

            //Be wary of the fact that a got focus event also updates the visual. In this scenario, we preceed a lost focus event, so
            //we don't have to worry about visuals being updated.

            //Manually go and update the visuals. Raising a got focus here instead of calling to UpdateVisualState is risky since it is async
            //and we cannot guarantee when it will complete.
            //TODO: How will this affect third party apps?
            if (CControl* focusedElementAsControl = do_pointer_cast<CControl>(focusedElement))
            {
                //We want to store the current focus state. We will simulate an unfocused state so that the visuals will change,
                //but we will not actually change the focusedState.
                DirectUI::FocusState focusedState = focusedElementAsControl->GetFocusState();
                focusedElementAsControl->UpdateFocusState(DirectUI::FocusState::Unfocused);

                FxCallbacks::Control_UpdateVisualState(focusedElement, true /*fUseTransitions*/);

                //Restore the original focus state. We lie and say that the focused state is unfocused, although that isn't
                //the case. This is done so that if this element is being inspected before a window activated has been received,
                //we can ensure that the behavior doesn't change.
                focusedElementAsControl->UpdateFocusState(focusedState);
            }

            // In the case of hyperlink raise both Hyperlink_LostFocus and UIElement_LostFocus. UIElement_LostFocus
            // is used internally for to decide where focus rects should be rendered.
            if (focusedElement->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
            {
                m_pCoreService->GetEventManager()->Raise(
                    EventHandle(KnownEventIndex::Hyperlink_LostFocus),
                    true, /*bRefire*/
                    focusedElement,
                    args.get(),
                    false /*fRaiseSync*/
                );
            }

            m_pCoreService->GetEventManager()->RaiseRoutedEvent(
                EventHandle(KnownEventIndex::UIElement_LostFocus),
                focusedElement,
                args.get(),
                TRUE /* bIgnoreVisibility */);

            xref_ptr<CFocusManagerLostFocusEventArgs> spFocusManagerLostFocusEventArgs;
            spFocusManagerLostFocusEventArgs.attach(new CFocusManagerLostFocusEventArgs(focusedElement, correlationId));
            // Raise the FocusManagerLostFocus event to the focus manager asynchronously
            m_pCoreService->GetEventManager()->Raise(
                EventHandle(KnownEventIndex::FocusManager_LostFocus),
                true, /*bRefire*/
                nullptr, /*sender, passing null because this is a static event*/
                spFocusManagerLostFocusEventArgs.get());

            xref_ptr<CFocusManagerGotFocusEventArgs> spFocusManagerGotFocusEventArgs;
            spFocusManagerGotFocusEventArgs.attach(new CFocusManagerGotFocusEventArgs(nullptr, correlationId));
            // Raise the FocusManagerGotFocus event to the focus manager asynchronously
            m_pCoreService->GetEventManager()->Raise(
                EventHandle(KnownEventIndex::FocusManager_GotFocus),
                true, /*bRefire*/
                nullptr, /*sender, passing null because this is a static event*/
                spFocusManagerGotFocusEventArgs.get());

        }
    }

    return S_OK;
}
