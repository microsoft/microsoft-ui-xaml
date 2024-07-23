// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Implements method call stubs for managed code
#include "precomp.h"

#include "xcpsafe.h"
#include "MetadataAPI.h"
#include "NodeStreamCache.h"
#include "Timemgr.h"
#include "Storyboard.h"
#include "EasingFunctions.h"
#include <CColor.h>
#include "FocusProperties.h"
#include <FocusSelection.h>
#include <XamlSchemaContext.h>
#include <XamlBinaryMetadata.h>
#include <XbfWriter.h>
#include <XbfVersioning.h>
#include "RootScale.h"

using namespace DirectUI;

namespace CoreImports
{
_Check_return_ HRESULT Host_GetActualHeight(
    _In_ CCoreServices* pCore,
    _Out_ XUINT32* pnActualHeight)
{
    IXcpBrowserHost* pBrowserHost = NULL;

    IFCEXPECT_RETURN(pCore);
    IFCEXPECT_RETURN(pnActualHeight);

    *pnActualHeight = 0;
    pBrowserHost = pCore->GetBrowserHost();
    IFCEXPECT_RETURN(pBrowserHost);
    IFC_RETURN(pBrowserHost->GetActualHeight(pnActualHeight));

    return S_OK;
}

_Check_return_ HRESULT Host_GetActualWidth(
    _In_ CCoreServices* pCore,
    _Out_ XUINT32* pnActualWidth)
{
    IXcpBrowserHost* pBrowserHost = NULL;

    IFCEXPECT_RETURN(pCore);
    IFCEXPECT_RETURN(pnActualWidth);

    *pnActualWidth = 0;
    pBrowserHost = pCore->GetBrowserHost();
    IFCEXPECT_RETURN(pBrowserHost);
    IFC_RETURN(pBrowserHost->GetActualWidth(pnActualWidth));

    return S_OK;
}

_Check_return_ HRESULT Host_GetEnableFrameRateCounter(
    _In_ CCoreServices* pCore,
    _Out_ bool* pbEnableFrameRateCounter
    )
{
    IXcpBrowserHost* pBrowserHost = NULL;
    bool enableFrameRateCounter = false;

    IFCEXPECT_RETURN(pCore);
    IFCEXPECT_RETURN(pbEnableFrameRateCounter);

    pBrowserHost = pCore->GetBrowserHost();
    IFCEXPECT_RETURN(pBrowserHost);

    IFC_RETURN(pBrowserHost->get_EnableFrameRateCounter(&enableFrameRateCounter));
    *pbEnableFrameRateCounter = enableFrameRateCounter;

    return S_OK;
}

_Check_return_ HRESULT Host_SetEnableFrameRateCounter(
    _In_ CCoreServices* pCore,
    _In_ bool bEnableFrameRateCounter
    )
{
    IXcpBrowserHost* pBrowserHost = NULL;

    IFCEXPECT_RETURN(pCore);

    pBrowserHost = pCore->GetBrowserHost();
    IFCEXPECT_RETURN(pBrowserHost);

    IFC_RETURN(pBrowserHost->set_EnableFrameRateCounter(bEnableFrameRateCounter));

    return S_OK;
}

_Check_return_ HRESULT Input_GetKeyboardModifiers(_Out_ wsy::VirtualKeyModifiers* pnKeyboardModifiers)
{
    XUINT32 modifierKeys = 0;

    IFCEXPECT_RETURN(pnKeyboardModifiers);
    *pnKeyboardModifiers = wsy::VirtualKeyModifiers::VirtualKeyModifiers_None;

    // Get the Keyboard modifiers state from platform service.

    if (gps.IsValid( ))
    {
        IFC_RETURN(gps->GetKeyboardModifiersState(&modifierKeys));
    }

    // Convert the modifierKeys with WinRT VirtualKeyModifiers definition
    if (modifierKeys & KEY_MODIFIER_ALT)
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Menu;
    }
    if (modifierKeys & KEY_MODIFIER_CTRL)
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Control;
    }
    if (modifierKeys & KEY_MODIFIER_SHIFT)
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Shift;
    }
    if (modifierKeys & KEY_MODIFIER_WINDOWS)
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Windows;
    }

    return S_OK;
}

_Check_return_ HRESULT Application_JupiterComplete(_In_ CDependencyObject* pDo)
{
    CApplication* pApp = NULL;
    CCoreServices* pCore = NULL;

    IFC_RETURN(DoPointerCast(pApp, pDo));
    pCore = pApp->GetContext();
    IFC_RETURN(pCore->SetCurrentApplication(NULL));
    IFC_RETURN(pCore->GetDeployment()->JupiterComplete(pApp));

    return S_OK;
}

_Check_return_ HRESULT Application_SetVisualRoot(
    _In_ CCoreServices* pCore,
    _In_ CUIElement* pPublicRootVisual)
{
    IXcpBrowserHost* pBrowserHost = NULL;

    IFCEXPECT_RETURN(pCore);

    pBrowserHost = pCore->GetBrowserHost();
    IFCEXPECT_RETURN(pBrowserHost);

    if (pCore->GetDeployment())
    {
        if (pCore->GetDeployment()->m_pApplication)
        {
            // Dump the PublicRootVisual set on the application (if there).
            // There can be an existing PublicRootVisual if this method is called more than once
            // before the application startup completed event has been processed, or if a
            // PublicRootVisual is set through XAML.
            ReleaseInterface(pCore->GetDeployment()->m_pApplication->m_pRootVisual);
            pCore->GetDeployment()->m_pApplication->m_pRootVisual = pPublicRootVisual;
            AddRefInterface(pCore->GetDeployment()->m_pApplication->m_pRootVisual);
            pCore->GetDeployment()->m_pApplication->m_fRootVisualSet = true;

            if (pPublicRootVisual)
            {
                // We associate the app's content element with the CoreWindow's ContentRoot right away.
                // This allows the app to query the ContentRoot from the element without having to wait for
                // the (async) ApplicationStartup event to fire.
                pPublicRootVisual->SetVisualTree(pCore->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot()->GetVisualTreeNoRef());
            }

            // Post a message so that we know when the startup event has been processed
            pCore->GetBrowserHost()->FireApplicationStartupEventComplete();
        }
        else
        {
            // The CDeployment object doesn't have its CApplication field set yet. This can happen
            // when Application_SetVisualRoot is called by a (managed) application's constructor, which
            // runs before we've set the CDeployment::m_pApplication in native code. Temporarily store
            // the root visual with the CDeployment object. CDeployment will update the CApplication with
            // the root visual later.
            ReleaseInterface(pCore->GetDeployment()->m_pTempRootVisual);
            pCore->GetDeployment()->m_pTempRootVisual = pPublicRootVisual;
            AddRefInterface(pCore->GetDeployment()->m_pTempRootVisual);
        }
    }

    //Event Trace for tracing AppModel First Render
    TraceApplicationStartupInfo();

    return S_OK;
}

_Check_return_ HRESULT Application_SetRootScrollViewer(
    _In_ CCoreServices* pCore,
    _In_opt_ CScrollContentControl* pRootScrollViewer,
    _In_opt_ CContentPresenter* pContentPresenter)
{
    CDependencyObject *pVisualRoot = NULL;

    pVisualRoot = static_cast<CDependencyObject*>(pCore->getVisualRoot());
    if (pVisualRoot)
    {
        CContentRoot* contentRoot = pCore->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        contentRoot->GetInputManager().DestroyInputPaneHandler();
    }

    if (pCore->GetDeployment())
    {
        if (pCore->GetDeployment()->m_pApplication)
        {
            ReplaceInterface(pCore->GetDeployment()->m_pApplication->m_pRootScrollViewer, pRootScrollViewer);
            ReplaceInterface(pCore->GetDeployment()->m_pApplication->m_pRootContentPresenter, pContentPresenter);
        }
        else
        {
            ReplaceInterface(pCore->GetDeployment()->m_pTempRootScrollViewer, pRootScrollViewer);
            ReplaceInterface(pCore->GetDeployment()->m_pTempRootContentPresenter, pContentPresenter);
        }
    }

    if (pRootScrollViewer)
    {
        // Set the root ScrollViewer explicitly on the ScrollContentControl.
        pRootScrollViewer->SetRootScrollViewer(true);
    }

    return S_OK; //RRETURN_REMOVAL
}

_Check_return_ HRESULT Application_GetInputPaneState(
    _In_ CUIElement* pElement,
    _Out_ DirectUI::InputPaneState* pInputPaneState,
    _Out_ XRECTF* pInputPaneBounds)
{
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pElement);

    *pInputPaneState = contentRoot->GetInputManager().GetInputPaneState();
    IFC_RETURN(contentRoot->GetInputManager().GetInputPaneBounds(pInputPaneBounds));

    return S_OK;
}

_Check_return_ HRESULT Application_LoadComponent(
    _In_ CCoreServices* pCore,
    _In_ CDependencyObject* pComponent,
    _In_ const xstring_ptr& strUri,
    _In_ ::ComponentResourceLocation resourceLocation
    )
{
    HRESULT hr = S_OK;
    IPALUri *pUri = NULL;

    IFCEXPECT(pCore);
    IFCEXPECT(pComponent);

    IFC(gps->UriCreate(strUri.GetCount(), strUri.GetBuffer(), &pUri));
    pUri->SetComponentResourceLocation(resourceLocation);
    IFC(CApplication::LoadComponent(pCore, pComponent, pUri));

Cleanup:
    ReleaseInterface(pUri);
    RRETURN(hr);
}

_Check_return_ HRESULT PopupRoot_CloseTopmostPopup(
    _In_ CDependencyObject *pPopupRootCDO, _In_ bool bLightDismissOnly)
{
    CPopupRoot* pPopupRoot = NULL;
    IFCPTR_RETURN(pPopupRootCDO);

    pPopupRoot = static_cast<CPopupRoot*>(pPopupRootCDO);
    IFC_RETURN(pPopupRoot->CloseTopmostPopup(DirectUI::FocusState::Programmatic, bLightDismissOnly ? CPopupRoot::PopupFilter::LightDismissOnly : CPopupRoot::PopupFilter::All));

    return S_OK;
}

_Check_return_ HRESULT Popup_Close(
    _In_ CDependencyObject* pPopupCDO)
{
    CPopup* pPopup = NULL;

    IFCPTR_RETURN(pPopupCDO);

    pPopup = static_cast<CPopup*>(pPopupCDO);
    IFC_RETURN(pPopup->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, false));

    return S_OK;
}

_Check_return_ HRESULT Popup_GetIsLightDismiss(
    _In_ CDependencyObject* pPopupCDO,
    _Out_ bool* pIsLightDismiss)
{
    HRESULT hr = S_OK;
    CPopup* pPopup = NULL;

    *pIsLightDismiss = false;
    pPopup = static_cast<CPopup*>(pPopupCDO);
    *pIsLightDismiss = pPopup->m_fIsLightDismiss;
    RRETURN(hr);
}

_Check_return_ HRESULT Popup_GetSavedFocusState(
    _In_ CDependencyObject* pPopupCDO,
    _Out_ DirectUI::FocusState *pFocusState)
{
    HRESULT hr = S_OK;
    CPopup* pPopup = NULL;

    *pFocusState = DirectUI::FocusState::Unfocused;
    pPopup = static_cast<CPopup*>(pPopupCDO);
    *pFocusState = pPopup->GetSavedFocusState();
    RRETURN(hr);
}

_Check_return_ HRESULT Popup_SetFocusStateAfterClosing(
    _In_ CDependencyObject* pPopupCDO,
    _In_ DirectUI::FocusState focusState)
{
    static_cast<CPopup*>(pPopupCDO)->SetFocusStateAfterClosing(focusState);
    RRETURN(S_OK);
}

_Check_return_ HRESULT Popup_SetShouldTakeFocus(
    _In_ CDependencyObject* pPopupCDO,
    _In_ bool shouldTakeFocus)
{
    static_cast<CPopup*>(pPopupCDO)->SetShouldTakeFocus(shouldTakeFocus);
    RRETURN(S_OK);
}

_Check_return_ HRESULT FocusManager_GetFirstFocusableElement(
    _In_ CDependencyObject* pSearchStart,
    _Outptr_ CDependencyObject** ppFirstFocusableElement)
{
    CFocusManager* pFocusManager = NULL;
    CDependencyObject* pFirstFocusableElementIDO = NULL;

    IFCCATASTROPHIC_RETURN(ppFirstFocusableElement);

    *ppFirstFocusableElement = NULL;
    pFocusManager = VisualTree::GetFocusManagerForElement(pSearchStart);
    IFCPTR_RETURN(pFocusManager);
    pFirstFocusableElementIDO = pFocusManager->GetFirstFocusableElement(pSearchStart);
    *ppFirstFocusableElement = static_cast<CDependencyObject *>(pFirstFocusableElementIDO);
    AddRefInterface(*ppFirstFocusableElement);

    return S_OK;
}

_Check_return_ HRESULT FocusManager_GetLastFocusableElement(
    _In_ CDependencyObject* pSearchStart,
    _Outptr_ CDependencyObject** ppLastFocusableElement)
{
    CFocusManager* pFocusManager = NULL;
    CDependencyObject* pLastFocusableElementIDO = NULL;

    IFCCATASTROPHIC_RETURN(ppLastFocusableElement);

    *ppLastFocusableElement = NULL;
    pFocusManager = VisualTree::GetFocusManagerForElement(pSearchStart);
    IFCPTR_RETURN(pFocusManager);
    pLastFocusableElementIDO = pFocusManager->GetLastFocusableElement(pSearchStart);
    if (pLastFocusableElementIDO == NULL && pFocusManager->IsFocusable(pSearchStart))
    {
        // Focus Manager will return null for GetLastFocusableElement if the element itself is focusable but there
        // are no focusable elements under it. So in this case we return the element itself.
        *ppLastFocusableElement = pSearchStart;
    }
    else
    {
        *ppLastFocusableElement = static_cast<CDependencyObject *>(pLastFocusableElementIDO);
    }
    AddRefInterface(*ppLastFocusableElement);

    return S_OK;
}

_Check_return_ HRESULT FocusManager_CanHaveFocusableChildren(
    _In_ CDependencyObject* pElement,
    _Out_ bool *pCanHaveFocusableChildren)
{
    bool canHaveFocusableChildren = false;
    *pCanHaveFocusableChildren = false;

    IFCPTR_RETURN(pElement);

    canHaveFocusableChildren = FocusProperties::CanHaveFocusableChildren(pElement);

    *pCanHaveFocusableChildren = canHaveFocusableChildren;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   LayoutInformation_GetLayoutExceptionElement
//
//  Synopsis: Returns the element that was being laid out when an exception
//  occurred.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutInformation_GetLayoutExceptionElement(
    _In_ CCoreServices* pCore,
    _Out_ CValue* pObject)
{
    CDependencyObject *pDO_out = NULL;

    IFCEXPECT_RETURN(pCore);

    pDO_out = pCore->GetLastLayoutExceptionElement();
    pObject->SetObjectAddRef(static_cast<CDependencyObject*>(pDO_out));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Function:   LayoutInformation_SetLayoutExceptionElement
//
//  Synopsis: Sets the element that was being laid out when an exception
//  occurred.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutInformation_SetLayoutExceptionElement(
    _In_ CCoreServices* pCore,
    _In_ CUIElement* pElement)
{
    IFCEXPECT_RETURN(pCore);
    IFCEXPECT_RETURN(pElement);

    pCore->GetMainLayoutManager()->SetLastExceptionElement(pElement);

    return S_OK;
}

_Check_return_ HRESULT DependencyObject_Freeze(
    _In_ CDependencyObject* pObject)
{
    pObject->SimulateFreeze();
    RRETURN(S_OK);
}

_Check_return_ HRESULT DependencyObject_Unfreeze(
    _In_ CDependencyObject* pObject,
    _Out_ bool* pbWasFrozen)
{
    IFCEXPECT_RETURN(pbWasFrozen);
    *pbWasFrozen = false;

    if (pObject->IsFrozen())
    {
        pObject->SimulateUnfreeze();
        *pbWasFrozen = true;
    }

    return S_OK;
}

// Export to call DependencyObject.Leave
_Check_return_ HRESULT DependencyObject_Leave(
    _In_ CDependencyObject* pObject,
    _In_ CDependencyObject* pNamescopeOwner,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    LeaveParams leaveParams(false, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset);
    IFC_RETURN( pObject->Leave(pNamescopeOwner, leaveParams));

    return S_OK;
}

_Check_return_ HRESULT Host_CreateFromXaml(
    _In_ CCoreServices* pCore,
    _In_ uint32_t cXaml,
    _In_reads_ (cXaml) const WCHAR* pXaml,
    _In_ bool bCreateNameScope,
    _In_ bool bRequireDefaultNamespace,
    _In_ bool bExpandTemplatesDuringParse,
    _Out_ CValue* pObject)
{
    CDependencyObject *pDO_out = nullptr;

    IFCEXPECT_RETURN(pCore);

    IFC_RETURN(pCore->GetBrowserHost()->CreateFromXaml(cXaml, pXaml, bCreateNameScope, bRequireDefaultNamespace, bExpandTemplatesDuringParse, &pDO_out));
    pObject->SetObjectNoRef(static_cast<CDependencyObject*>(pDO_out));

    return S_OK;
}

_Check_return_ HRESULT CreateFromXamlBytes(
    _In_ CCoreServices* pCore,
    _In_ const Parser::XamlBuffer& buffer,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ bool bCreateNameScope,
    _In_ bool bRequireDefaultNamespace,
    _In_ const xstring_ptr_view& strXamlResourceUri,
    _Out_ CValue *pObject)
{
    CDependencyObject *pDO_out = nullptr;

    IFCEXPECT_RETURN(pCore);

    IFC_RETURN(pCore->ParseXaml(
        buffer,
        false /*bForceUtf16*/,
        bCreateNameScope,
        bRequireDefaultNamespace,
        &pDO_out,
        strSourceAssemblyName,
        false /*bExpandTemplatesDuringParse*/,
        strXamlResourceUri));

    pObject->SetObjectNoRef(pDO_out);

    return S_OK;
}


_Check_return_ HRESULT FrameworkElement_MeasureOverride(
    _In_ CFrameworkElement* pElement,
    _In_ XFLOAT fAvailableWidth,
    _In_ XFLOAT fAvailableHeight,
    _Inout_ XFLOAT* pfDesiredWidth,
    _Inout_ XFLOAT* pfDesiredHeight)
{
    XSIZEF availableSize, desiredSize;

    IFCEXPECT_RETURN(pElement);
    IFCPTR_RETURN(pfDesiredWidth);
    IFCPTR_RETURN(pfDesiredHeight);

    availableSize.height = fAvailableHeight;
    availableSize.width = fAvailableWidth;
    desiredSize.height =  desiredSize.width = 0.0f;

    IFC_RETURN(pElement->MeasureOverrideForPInvoke(availableSize, &desiredSize));

    *pfDesiredWidth = desiredSize.width;
    *pfDesiredHeight = desiredSize.height;

    return S_OK;
}


_Check_return_ HRESULT FrameworkElement_ArrangeOverride(
    _In_ CFrameworkElement* pElement,
    _In_ XFLOAT fFinalWidth,
    _In_ XFLOAT fFinalHeight,
    _Inout_ XFLOAT* pfActualWidth,
    _Inout_ XFLOAT* pfActualHeight)
{
    XSIZEF finalSize, newFinalSize;

    IFCEXPECT_RETURN(pElement);
    IFCPTR_RETURN(pfActualWidth);
    IFCPTR_RETURN(pfActualHeight);

    finalSize.height = fFinalHeight;
    finalSize.width = fFinalWidth;
    newFinalSize.width = newFinalSize.height = 0.0f;

    IFC_RETURN(pElement->ArrangeOverrideForPInvoke(finalSize, &newFinalSize));

    *pfActualWidth = newFinalSize.width;
    *pfActualHeight = newFinalSize.height;

    return S_OK;
}

_Check_return_ HRESULT UIElement_SetCurrentTransitionLocation(
    _In_ CUIElement* pTarget,
    _In_ XFLOAT left,
    _In_ XFLOAT top,
    _In_ XFLOAT width,
    _In_ XFLOAT height)
{
    LayoutTransitionStorage* pStorage = NULL;
    CLayoutManager* pLayoutManager = NULL;
    IFCPTR_RETURN(pTarget);
    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);

    if (pTarget->HasLayoutTransitionStorage())
    {
        pStorage = pTarget->GetLayoutTransitionStorage();
        pStorage->m_currentOffset.x = pStorage->m_nextGenerationOffset.x = left;
        pStorage->m_currentOffset.y = pStorage->m_nextGenerationOffset.y = top;
        pStorage->m_currentSize.width = pStorage->m_nextGenerationSize.width = width;
        pStorage->m_currentSize.height = pStorage->m_nextGenerationSize.height= height;
        pStorage->m_nextGenerationCounter = pLayoutManager->GetNextLayoutCounter();
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement_SetIsEntering(
    _In_ CUIElement* pTarget,
    _In_ bool fValue)
{
    CLayoutManager* pLayoutManager = NULL;
    IFCPTR_RETURN(pTarget);

    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);

    if (pLayoutManager)
    {
        pTarget->m_enteredTreeCounter = fValue ? EnteredInThisTick : pLayoutManager->GetLayoutCounter() - 1;
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement_SetIsLeaving(
    _In_ CUIElement* pTarget,
    _In_ bool fValue)
{
    CLayoutManager* pLayoutManager = NULL;
    IFCPTR_RETURN(pTarget);

    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);

    if (pLayoutManager)
    {
        pTarget->m_leftTreeCounter = fValue ? LeftInThisTick : pLayoutManager->GetLayoutCounter() - 1;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//    Sets a flag to indicate this item was deferred in its unlinking.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT UIElement_SetDeferUnlinkContainer(
    _In_ CUIElement* pTarget
    )
{
    CUIElementCollection* pCollection = NULL;
    CUIElement* pParent = NULL;

    IFCPTR_RETURN(pTarget);
    ASSERT(pTarget->GetParentInternal()->OfTypeByIndex<KnownTypeIndex::UIElement>());
    pParent = static_cast<CUIElement*>(pTarget->GetParentInternal());

    pCollection = static_cast<CUIElementCollection*>(pParent->GetChildren());

    if (pCollection && pCollection->HasUnloadingStorage() && pCollection->m_pUnloadingStorage->ContainsKey(pTarget))
    {
        UnloadCleanup* pRemoveLogicToExecute = pCollection->m_pUnloadingStorage->Get(pTarget);
        *pRemoveLogicToExecute = (UnloadCleanup) (*pRemoveLogicToExecute | UC_UnlinkContainer);
    }
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement_GetHasTransition(
    _In_ CUIElement* pTarget,
    _Out_ bool* pHasTransition,
    _Out_opt_ DirectUI::TransitionTrigger* pTransitionTrigger)
{
    LayoutTransitionStorage* pStorage = NULL;

    IFCPTR_RETURN(pHasTransition);
    IFCPTR_RETURN(pTarget);

    *pHasTransition = false;

    if (pTarget->HasLayoutTransitionStorage())
    {
        pStorage = pTarget->GetLayoutTransitionStorage();
        TransitionTrigger trigger = pStorage->GetTrigger();

        *pHasTransition = trigger != DirectUI::TransitionTrigger::NoTrigger;
        if (*pHasTransition && pTransitionTrigger)
        {
            IFC_RETURN(pStorage->GetTriggerForPublicConsumption(pTransitionTrigger));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement_CancelTransition(
    _In_ CUIElement* pTarget)
{
    RRETURN(CTransition::CancelTransitions(pTarget));
}

// returns the tick that is going to be used during setup
_Check_return_ HRESULT LayoutManager_GetLayoutTickForTransition(_In_ CUIElement* pTarget, _Out_ XINT16* pTick)
{
    CLayoutManager* pLayoutManager = NULL;
    IFCPTR_RETURN(pTarget);

    pLayoutManager = VisualTree::GetLayoutManagerForElement(pTarget);
    if (pLayoutManager)
    {
        *pTick = pLayoutManager->GetLayoutCounter();
    }

    return S_OK;
}

_Check_return_ HRESULT CPanel_PanelGetClosestIndexSlow(
    _In_ CPanel* pNativeInstance,
    _In_ XPOINTF location,
    _Out_ XINT32* pReturnValue)
{
    RRETURN(pNativeInstance->PanelGetClosestIndexSlow(location, pReturnValue));
}

_Check_return_ HRESULT UIElement_Measure(
    _In_ CUIElement* pElement,
    _In_ XFLOAT fAvailableWidth,
    _In_ XFLOAT fAvailableHeight)
{
    XSIZEF availableSize;

    IFCEXPECT_RETURN(pElement);

    availableSize.width = fAvailableWidth;
    availableSize.height = fAvailableHeight;

    IFC_RETURN(pElement->Measure(availableSize));

    return S_OK;
}

_Check_return_ HRESULT UIElement_Arrange(
    _In_ CUIElement* pElement,
    _In_ XFLOAT fX,
    _In_ XFLOAT fY,
    _In_ XFLOAT fWidth,
    _In_ XFLOAT fHeight)
{
    XRECTF finalRect;

    IFCEXPECT_RETURN(pElement);

    finalRect.X = fX;
    finalRect.Y = fY;
    finalRect.Width= fWidth;
    finalRect.Height = fHeight;

    IFC_RETURN(pElement->Arrange(finalRect));

    return S_OK;
}


_Check_return_ HRESULT UIElement_GetDesiredSize(
    _In_ CUIElement* pElement,
    _Inout_ XFLOAT* pfDesiredWidth,
    _Inout_ XFLOAT* pfDesiredHeight)
{
    IFCEXPECT_RETURN(pElement);
    IFCPTR_RETURN(pfDesiredWidth);
    IFCPTR_RETURN(pfDesiredHeight);

    if(pElement->IsVisible() && pElement->HasLayoutStorage())
    {
        *pfDesiredWidth = pElement->DesiredSize.width;
        *pfDesiredHeight = pElement->DesiredSize.height;
    }
    else
    {
        *pfDesiredWidth = 0.0f;
        *pfDesiredHeight = 0.0f;
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement_GetVisualOffset(
    _In_ CUIElement* pElement,
    _Inout_ XFLOAT* pfOffsetX,
    _Inout_ XFLOAT* pfOffsetY)
{
    IFCEXPECT_RETURN(pElement && pfOffsetX && pfOffsetY);
    *pfOffsetX = pElement->VisualOffset.x;
    *pfOffsetY = pElement->VisualOffset.y;

    return S_OK;
}

_Check_return_ HRESULT UIElement_IsFocusable(
    _In_ CUIElement* pElement,
    _Out_ bool* pIsFocusable)
{
    IFCEXPECT_RETURN(pElement);
    *pIsFocusable = pElement->IsFocusable();

    return S_OK;
}

_Check_return_ HRESULT DispatcherTimer_Start(
    _In_ CDispatcherTimer *pDispatcherTimer)
{
    IFC_RETURN(pDispatcherTimer->Start());

    return S_OK;
}

_Check_return_ HRESULT DispatcherTimer_Stop(
    _In_ CDispatcherTimer *pDispatcherTimer)
{
    IFC_RETURN(pDispatcherTimer->Stop());

    return S_OK;
}

_Check_return_ HRESULT Control_Raise(
    _In_ CControl* pControl,
    _In_ CEventArgs* pEventArgs,
    _In_ KnownEventIndex nDelegateIndex)
{
    ASSERT(CControl::IsValidDelegate(nDelegateIndex));
    RRETURN(CControl::Delegates[static_cast<XUINT32>(nDelegateIndex) - 1 /* UnknownEvent */](pControl, pEventArgs));
}

_Check_return_ HRESULT SetAutomationPeerParent(
    _In_ CAutomationPeer* pAutomationPeer,              // comes in as a CDependencyObject interface
    _In_ CAutomationPeer* pParentAutomationPeer)
{
    HRESULT hr = S_OK;
    pAutomationPeer->SetAPParent(pParentAutomationPeer);

    RRETURN(hr);
}

_Check_return_ HRESULT GetTextProviderValue(
    _In_ CTextAdapter* pTextProvider,              // comes in as a CDependencyObject interface
    _In_ XUINT32 nAPIIndex,
    _In_ CValue arg,
    _Out_ CValue* pData)
{
    UIAXcp::SupportedTextSelection selectionType;
    CDependencyObject *pArgObject = NULL;
    XPOINTF* pPoint = NULL;

    pData->Unset();

    xref_ptr<CTextRangeAdapter> pTextRangeProviderOut;
    switch (nAPIIndex)
    {
    case 0:
        IFC_RETURN(pTextProvider->GetDocumentRange(pTextRangeProviderOut.ReleaseAndGetAddressOf()));
        if (pTextRangeProviderOut)
        {
            pData->SetObjectNoRef(pTextRangeProviderOut.detach());
        }
        else
        {
            pData->SetNull();
        }
        break;
    case 1:
        IFC_RETURN(pTextProvider->GetSupportedTextSelection(&selectionType));
        pData->SetEnum(selectionType);
        break;
    case 4:
        pData->SetNull();
        if (arg.GetType() == valueObject)
        {
            IFC_RETURN(arg.GetObject(pArgObject));
            IFC_RETURN(pTextProvider->RangeFromChild(static_cast<CAutomationPeer*>(pArgObject), pTextRangeProviderOut.ReleaseAndGetAddressOf()));
            if (pTextRangeProviderOut)
            {
                pData->SetObjectNoRef(pTextRangeProviderOut.detach());
            }
        }
        break;
    case 5:
        pData->SetNull();
        if(arg.GetType() ==  valuePoint)
        {
            IFC_RETURN(arg.GetPoint(pPoint));
            IFC_RETURN(pTextProvider->RangeFromPoint(*pPoint, pTextRangeProviderOut.ReleaseAndGetAddressOf()));
            if (pTextRangeProviderOut)
            {
                pData->SetObjectNoRef(pTextRangeProviderOut.detach());
            }
        }
        break;
    default:
        ASSERT(false, "Invalid call for an API in GetTextProviderValue");
        break;
    }

    return S_OK;
}

// here for performance. In tight loops (such as datacontext propagation) we were using
// VisualTreeHelper::GetChildren and only using one child.
_Check_return_ HRESULT GetTextRangeArray(
    _In_ CTextAdapter* pTextProvider,
    _In_ XUINT32 nAPIIndex,
    _Outptr_result_buffer_all_(*pnCount) CTextRangeAdapter*** pppChildren,
    _Out_ XINT32* pnCount)
{
    *pppChildren = NULL;
    *pnCount = 0;

    switch (nAPIIndex)
    {
    case 2:
        IFC_RETURN(pTextProvider->GetSelection(pppChildren, pnCount));
        break;
    case 3:
        IFC_RETURN(pTextProvider->GetVisibleRanges(pppChildren, pnCount));
        break;
    default:
        ASSERT(false, "Invalid call for an API in GetTextRangeArray");
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT GetTextRangeProviderValue(
    _In_ CTextRangeAdapter* pTextRangeProvider,              // comes in as a CDependencyObject interface
    _In_ XUINT32 nAPIIndex,
    _In_ CValue arg,
    _Out_ CValue* pData)
{
    CDependencyObject *pArgObject = NULL;
    CTextRangeAdapter *pTextRangeProviderIn = NULL;
    UIAXcp::TextUnit unit;
    XUINT32 argIntVal;
    bool bReturnVal = false;
    CAutomationPeer *pAPOut = NULL;

    pData->Unset();

    xref_ptr<CTextRangeAdapter> pTextRangeProviderOut;
    switch (nAPIIndex)
    {
    case 0: //Clone
        IFC_RETURN(pTextRangeProvider->Clone(pTextRangeProviderOut.ReleaseAndGetAddressOf()));
        if (pTextRangeProviderOut)
        {
            pData->SetObjectNoRef(pTextRangeProviderOut.detach());
        }
        else
        {
            pData->SetNull();
        }
        break;
    case 1: //Compare
        if (arg.GetType() == valueObject)
        {
            IFC_RETURN(arg.GetObject(pArgObject));
            pTextRangeProviderIn = do_pointer_cast<CTextRangeAdapter>(pArgObject);
            IFC_RETURN(pTextRangeProvider->Compare(pTextRangeProviderIn, &bReturnVal));
            pData->SetBool(!!bReturnVal);
            break;
        }
    case 3: //ExpandEnclosingUnit
        if(arg.IsEnum())
        {
            IFC_RETURN(arg.GetEnum(argIntVal));
            unit = (UIAXcp::TextUnit)argIntVal;
            IFC_RETURN(pTextRangeProvider->ExpandToEnclosingUnit(unit));
        }
        break;
    case 6: //GetAttributeValue
        if(arg.GetType() ==  valueSigned)
        {
            IFC_RETURN(pTextRangeProvider->GetAttributeValue(static_cast<DirectUI::AutomationTextAttributesEnum>(arg.AsSigned()), pData));
        }
        break;
    case 8:
        IFC_RETURN(pTextRangeProvider->GetEnclosingElement(&pAPOut));
        if (pAPOut)
        {
            pData->SetObjectAddRef(pAPOut);
        }
        else
        {
            pData->SetNull();
        }
        break;
    case 9:
        if (arg.GetType() == valueSigned)
        {
            xstring_ptr strString;
            IFC_RETURN(pTextRangeProvider->GetText(arg.AsSigned(), &strString));
            pData->SetString(std::move(strString));
        }
        break;
    case 13:
        IFC_RETURN(pTextRangeProvider->Select());
        break;
    case 14:
        IFC_RETURN(pTextRangeProvider->AddToSelection());
        break;
    case 15:
        IFC_RETURN(pTextRangeProvider->RemoveFromSelection());
        break;
    case 16:
        IFC_RETURN(pTextRangeProvider->ScrollIntoView());
        break;
    default:
        ASSERT(false, "Invalid call for an API in GetTextRangeProviderValue");
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter_CompareEndpoints(
    _In_ CTextRangeAdapter* pTextRangeProvider,
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ CTextRangeAdapter* pTargetTextRangeProvider,
    _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint,
    _Out_ XINT32* pReturnValue)
{
    IFC_RETURN(pTextRangeProvider->CompareEndpoints(endPoint, pTargetTextRangeProvider, targetEndPoint,  pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter_GetBoundingRectangles(
    _In_ CTextRangeAdapter* pTextRangeProvider,
    _Outptr_result_buffer_all_(*pnCount) double** ppRectangles,
    _Out_ XINT32* pnCount)
{
    IFC_RETURN(pTextRangeProvider->GetBoundingRectangles(ppRectangles, pnCount));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter_Move(
    _In_ CTextRangeAdapter* pTextRangeProvider,
    _In_ UIAXcp::TextUnit unit,
    _In_ XINT32 count,
    _Out_ XINT32* pReturnValue)
{
    IFC_RETURN(pTextRangeProvider->Move(unit, count, pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter_MoveEndpointByUnit(
    _In_ CTextRangeAdapter* pTextRangeProvider,
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ UIAXcp::TextUnit unit,
    _In_ XINT32 count,
    _Out_ XINT32* pReturnValue)
{
    IFC_RETURN(pTextRangeProvider->MoveEndpointByUnit(endPoint, unit, count, pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter_MoveEndpointByRange(
    _In_ CTextRangeAdapter* pTextRangeProvider,
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ CTextRangeAdapter* pTargetTextRangeProvider,
    _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint)
{
    IFC_RETURN(pTextRangeProvider->MoveEndpointByRange(endPoint, pTargetTextRangeProvider, targetEndPoint));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter_GetChildren(
    _In_ CTextRangeAdapter* pTextRangeProvider,
    _Outptr_result_buffer_all_(*pnCount) CAutomationPeer*** pppChildren,
    _Out_ XINT32* pnCount)
{
    *pppChildren = NULL;
    *pnCount = 0;
    IFC_RETURN(pTextRangeProvider->GetChildren(pppChildren, pnCount));
    return S_OK;
}


_Check_return_ HRESULT AutomationListenerExists(
    _In_ CCoreServices* pCore,
    _In_ UIAXcp::APAutomationEvents nAutomationEventIndex,
    _Out_ bool* bExists)
{
    if (!pCore || !bExists)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *bExists = pCore->UIAClientsAreListening(nAutomationEventIndex) == S_OK;

    return S_OK;
}

_Check_return_ HRESULT AutomationRaiseAutomationPropertyChanged(
    _In_ CAutomationPeer* pAutomationPeer,
    _In_ UIAXcp::APAutomationProperties nAutomationPropertyIndex,
    _In_ const CValue& oldValue,
    _In_ const CValue& newValue)
{
    HRESULT hr = S_OK;
    pAutomationPeer->RaisePropertyChangedEvent(nAutomationPropertyIndex, oldValue, newValue);

    RRETURN(hr);
}

_Check_return_ HRESULT AutomationRaiseFocusChangedOnUIAWindow(
    _In_ CCoreServices* pCore,
    _In_ CDependencyObject* sender)
{
    if (!pCore)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(pCore->UIARaiseFocusChangedEventOnUIAWindow(sender));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   DependencyObject_GetIsAssociated
//
//  Synopsis:
//    Determines whether the provided dependency object as associated or not.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_GetIsAssociated(
    _In_ CDependencyObject* pObject,
    _Out_ bool* pfIsAssociated,
    _Out_ bool* pfAllowsMultipleAssociations)
{
    IFCEXPECT_RETURN(pObject);
    IFCEXPECT_RETURN(pfIsAssociated);
    IFCEXPECT_RETURN(pfAllowsMultipleAssociations);

    *pfIsAssociated = (pObject->IsAssociated() != 0);
    *pfAllowsMultipleAssociations = (pObject->DoesAllowMultipleAssociation() != 0);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   DependencyObject_ClearValue
//
//  Synopsis:
//    clears the local value set on the dp
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_ClearValue(
    _In_ CDependencyObject* pObject,
    _In_ const CDependencyProperty* pDP)
{
    // First see if we can get the DP from the index given
    IFCEXPECT_RETURN(pObject);

    if (pDP->IsReadOnly())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(pObject->ClearValue(pDP));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   DependencyObject_HasValue
//
//  Synopsis:
//          Returns the true if value has ever been set
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_IsPropertyDefault(
    _In_ CDependencyObject* pObject,
    _In_ const CDependencyProperty* pDP,
    _Out_ bool* pbIsPropertyDefault)
{
    IFCEXPECT_RETURN(pbIsPropertyDefault);
    *pbIsPropertyDefault = false;

    IFCEXPECT_RETURN(pObject);
    *pbIsPropertyDefault = pObject->IsPropertyDefault(pDP);
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   DependencyObject_ShouldCreatePeerWithStrongRef
//
//  Synopsis:
//      Tells the managed peer if it should create a strong reference
//      to itself in the ManagedPeerTable.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_ShouldCreatePeerWithStrongRef(
    _In_ CDependencyObject* pObject,
    _Out_ bool* pbShouldCreate)
{
    HRESULT hr = S_OK;

    // NOTE: This function should always return S_OK.  We've made the return
    // type HRESULT for future versioning opportunity.  The managed pinvoke call
    // does not currently need to check the HRESULT, so that needs to be updated
    // if any new errors are returned.

    *pbShouldCreate = false;

    if(
        // Types that have managed state but don't participate in the
        // peer tree require a strong ref
        pObject->ControlsManagedPeerLifetime()

        ||
        // If an object is being parsed (and it needs protection in the managed tree),
        // it requires a strong ref
        (pObject->IsParsing() || pObject->ParserOwnsParent())
        && pObject->ParticipatesInManagedTree() )
    {
        *pbShouldCreate = true;
    }

//Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function: DependencyObject_GetTypeIndex
//
//  Return the GetTypeIndex() of a DO.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_GetTypeIndex(
    _In_ CDependencyObject* pObject,
    _Out_ KnownTypeIndex* piTypeIndex)
{
    HRESULT hr = S_OK;

    // NOTE: This function should always return S_OK.  We've made the return
    // type HRESULT for future versioning opportunity.  The managed pinvoke call
    // does not currently need to check the HRESULT, so that needs to be updated
    // if any new errors are returned.

    *piTypeIndex = pObject->GetTypeIndex();

//Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Retrieves the base URI of the given dependency object.
//
//  Notes:
//      If the object has no base URI, it returns a NULL CValue (CValue::SetNull)
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_GetBaseUri(
    _In_ CDependencyObject *pObject,
    _Out_ CValue *pValue
    )
{
    xstring_ptr strUri;

    wrl::ComPtr<IPALUri> pUri;
    pUri.Attach(pObject->GetBaseUri());
    if (!pUri)
    {
        pValue->SetNull();
        return S_OK;
    }

    IFC_RETURN(pUri->GetCanonical(&strUri));
    pValue->SetString(std::move(strUri));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Collection_Add
//
//  Synopsis:
//      Adds a new element to collection
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_Add(
    _In_ CCollection* pCollection,
    _In_ CValue *pItem)
{
    IFCEXPECT_RETURN(pCollection);
    IFC_RETURN(CCollection::Add(pCollection, 1, pItem, NULL));
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   Collection_GetItem
//
//  Synopsis:
//      Gets an element by index
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_GetItem(
    _In_ CCollection* pCollection,
    _In_ XUINT32 nIndex,
    _Out_ CValue* pItem)
{
    RRETURN(CCollection::GetItem(pCollection, nIndex, pItem));
}


//------------------------------------------------------------------------
//
//  Method:   Collection_Insert
//
//  Synopsis:
//      Inserts an element
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_Insert(
    _In_ CCollection* pCollection,
    _In_ XUINT32 nIndex,
    _In_ CValue* pItem)
{
    IFCEXPECT_RETURN(pCollection);
    IFCPTR_RETURN(pItem);
    IFC_RETURN(pCollection->FailIfLocked());
    if (pItem->GetType() != valueObject)
    {
        IFC_RETURN(pCollection->Insert(nIndex, *pItem));
    }
    else
    {
        IFC_RETURN(pCollection->InsertDO(pCollection, nIndex, *pItem));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Collection_SetAt
//
//  Synopsis:
//      Set the item to the specified index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObjectCollection_SetAt(
    _In_ CDependencyObjectCollection* pDependencyObjectCollection,
    _In_ XUINT32 nIndex,
    _In_ CValue* pItem)
{
    IFCEXPECT_RETURN(pDependencyObjectCollection);
    IFCPTR_RETURN(pItem);

    IFC_RETURN(pDependencyObjectCollection->FailIfLocked());

    IFC_RETURN(pDependencyObjectCollection->SetAt(nIndex, *pItem));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Collection_Remove
//
//  Synopsis:
//      Removes an element
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_RemoveAt(_In_ CCollection* collection, XUINT32 index)
{
    UINT size = 0;
    size = collection->GetCount();
    IFCEXPECTRC_RETURN(index < size, E_BOUNDS);

    if (collection->IsDOCollection())
    {
        void* removed = collection->RemoveAt(index);
        if (removed)
        {
            static_cast<CDependencyObject*>(removed)->Release();
        }
    }
    else if (collection->ContainsNoRefItems())
    {
        // No cleanup necessary
        collection->RemoveAt(index);
    }
    else
    {
        // This is the absurd case where Double/PointCollection allocates a new float just
        // to return to you to delete.
        void* removed = collection->RemoveAt(index);
        if (removed)
        {
            // BUG 41892250: CoreImports::Collection_RemoveAt deletes removed collection object through void*.
            delete removed;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Collection_IndexOf
//
//  Synopsis:
//      Gets the index of an element
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_IndexOf(
    _In_ CCollection* pCollection,
    _In_ CValue *pItem,
    _Out_ XINT32 *piIndex)
{
    return pCollection->IndexOf(*pItem, piIndex);
}


//------------------------------------------------------------------------
//
//  Method:   Collection_Clear
//
//  Synopsis:
//      Clears the collection
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_Clear(
    _In_ CCollection* pCollection)
{
    IFCEXPECT_RETURN(pCollection);

    IFC_RETURN(pCollection->FailIfLocked());

    IFC_RETURN(pCollection->Clear());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Collection_Move
//
//  Synopsis:
//      Moves the element to new position within the collection
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_Move(
    _In_ CCollection *pCollection,
    _In_ XINT32 nIndex,
    _In_ XINT32 nPosition)
{
    IFCEXPECT_RETURN(pCollection);

    IFC_RETURN(pCollection->MoveInternal(nIndex, nPosition));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Collection_SetOwner
//
//  Synopsis:
//      Sets the owner of the collection
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Collection_SetOwner(
    _In_ CCollection* pCollection,
    _In_ CDependencyObject* pOwner)
{
    RRETURN(pCollection->SetOwner(pOwner));
}

//------------------------------------------------------------------------
//
//  Function:   ConvertStringToTypedCValue
//
//  Synopsis:   Convert the passed string value to typed Jolt CValue
//              for Managed consumption.
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ConvertStringToTypedCValue(
    _In_ CCoreServices* pCore,
    _In_ const xstring_ptr& strClrTypeName,
    _In_ const xstring_ptr& strText,
    _Out_ CValue* pValue)
{
    XStringBuilder coreTypeNameBuilder;
    CREATEPARAMETERS cp(pCore);

    IFC_RETURN(coreTypeNameBuilder.Initialize());

    // Tags of the form '?Name' in the known types table are type converters. Try that first.
    IFC_RETURN(coreTypeNameBuilder.AppendChar(L'?'));
    IFC_RETURN(coreTypeNameBuilder.Append(strClrTypeName));

    const CClassInfo* pClassInfo = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(
        XSTRING_PTR_EPHEMERAL_FROM_BUILDER(coreTypeNameBuilder));

    // Tags of the form '.Name' in the known types table are abstract base classes.
    // Some of the abstract classes support creation from string. Try that next.
    if (pClassInfo == nullptr)
    {
        coreTypeNameBuilder.GetMutableBuffer()[0] = L'.';

        pClassInfo = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(
            XSTRING_PTR_EPHEMERAL_FROM_BUILDER(coreTypeNameBuilder));
    }

    // Finally, look up all other object types.
    if (pClassInfo == nullptr)
    {
        pClassInfo = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(strClrTypeName);
    }

    if (pClassInfo == nullptr)
    {
        IFC_RETURN(static_cast<HRESULT>(CLR_E_PARSER_BAD_TYPE));
    }

    // There is a subtle historical difference between a 0-length string
    // and "nothing/default" when calling Create methods, so we only
    // set the string value in non-null cases.
    if (strText.GetCount() > 0)
    {
        cp.m_value.SetString(strText);
    }

    const CREATEPFN pfnCreate = pClassInfo->GetCoreConstructor();

    // If there is no core constructor, then return E_NOTIMPL, caller will handle this error.
    if (pfnCreate == nullptr)
    {
        IFC_NOTRACE_RETURN(E_NOTIMPL);
    }

    xref_ptr<CDependencyObject> pDO;
    IFC_RETURN(pfnCreate(pDO.ReleaseAndGetAddressOf(), &cp));

    // Convert DO to CValue
    CValue* val = pValue;

    switch (pDO->GetTypeIndex())
    {
    case KnownTypeIndex::Int32:
        {
            CInt32 *cInt32 = static_cast<CInt32 *>(pDO.get());
            val->SetSigned(cInt32->m_iValue);
        }
        break;
    case KnownTypeIndex::Double:
        {
            CDouble *cDouble = static_cast<CDouble *>(pDO.get());
            val->SetFloat(cDouble->m_eValue);
        }
        break;
    case KnownTypeIndex::String:
        {
            CString *cString = static_cast<CString *>(pDO.get());
            if (cString->m_strString.IsNull())
            {
                IFC_RETURN(E_FAIL);
            }

            val->SetString(cString->m_strString);
        }
        break;
    case KnownTypeIndex::Point:
        {
            CPoint *cPoint = static_cast<CPoint *>(pDO.get());

            XPOINTF *pXf = new XPOINTF;

            *pXf = cPoint->m_pt;
            val->SetPoint(pXf);
        }
        break;
    case KnownTypeIndex::Rect:
        {
            CRect *cRect = static_cast<CRect *>(pDO.get());

            XRECTF* pXf = new XRECTF;

            *pXf = cRect->m_rc;
            val->SetRect(pXf);
        }
        break;
    case KnownTypeIndex::Color:
        {
            CColor *cColor = static_cast<CColor *>(pDO.get());
            val->SetColor(cColor->m_rgb);
        }
        break;
    case KnownTypeIndex::Thickness:
        {
            CThickness *cThickness = static_cast<CThickness *>(pDO.get());

            XTHICKNESS *pXf = new XTHICKNESS;

            *pXf = cThickness->m_thickness;
            val->SetThickness(pXf);
        }
        break;

    case KnownTypeIndex::GridLength:
        {
            CGridLength *pGridLength = static_cast<CGridLength *>(pDO.get());

            XGRIDLENGTH *pXf = new XGRIDLENGTH;

            *pXf = pGridLength->m_gridLength;
            val->SetGridLength(pXf);
        }
        break;

    default:
        // Note that Boolean types are created as Enumerated internally, so we don't need a separate case for INDEX_BOOLEAN here.
        if (pDO->GetClassInformation()->IsEnum())
        {
            CEnumerated *cEnum = static_cast<CEnumerated *>(pDO.get());
            if (pDO->GetClassInformation()->IsCompactEnum())
            {
                ASSERT(cEnum->m_nValue <= UINT8_MAX);
                val->SetEnum8(static_cast<uint8_t>(cEnum->m_nValue));
            }
            else
            {
                val->SetEnum(cEnum->m_nValue);
            }
        }
        else
        {
            // We want to disable UIElement derived types as custom properties.
            if (pDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                IFC_RETURN(E_FAIL);
            }
            else
            {
                val->SetObjectNoRef(pDO.detach());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT PrintDocument_BeginPreview(
    _In_ CPrintDocument* pPrintDocument,
    _In_ void* pPreviewPackageTarget)
{
    RRETURN(pPrintDocument->BeginPreview(pPreviewPackageTarget));
}

_Check_return_ HRESULT PrintDocument_EndPreview(
    _In_ CPrintDocument* pPrintDocument)
{
    pPrintDocument->EndPreview();
    RRETURN(S_OK);
}

_Check_return_ HRESULT PrintDocument_Paginate(
    _In_ CPrintDocument* pPrintDocument,
    _In_opt_ void* pDocumentSettings)
{
    RRETURN(pPrintDocument->Paginate(pDocumentSettings));
}

_Check_return_ HRESULT PrintDocument_GetPreviewPage(
    _In_ CPrintDocument* pPrintDocument,
    XUINT32 desiredJobPage,
    XFLOAT width,
    XFLOAT height
    )
{
    RRETURN(pPrintDocument->GetPreviewPage(desiredJobPage, width, height));
}

_Check_return_ HRESULT PrintDocument_MakeDocument(
    _In_ CPrintDocument* pPrintDocument,
    _In_ void* pDocPackageTarget,
    _In_opt_ void* pDocSettings)
{
    RRETURN(pPrintDocument->MakeDocument(pDocPackageTarget, pDocSettings));
}

//------------------------------------------------------------------------
//
//  Function:   DependencyObject_GetVisualRelative
//
//  Synopsis:   This is used to enable interaction of a control and data
//              templates.  Given a visual, it returns the visual's
//              parent or children (depending on linkKind argument).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_GetVisualRelative(
    _In_ CUIElement* pElement,
    _In_ XINT32 iRelativeLinkKind,
    _Out_ CValue* pRelative)
{
    CDependencyObject* pObjectNoRef = nullptr;

    IFCEXPECT_RETURN(pElement);

    // Get the parent or children

    if (iRelativeLinkKind == 0 /* VisualRelativeKind_Child */)
    {
        pObjectNoRef = (CDependencyObject*)pElement->GetChildren();
    }
    else if (iRelativeLinkKind == 1 /* VisualRelativeKind_Parent */)
    {
        pObjectNoRef = (CDependencyObject*)pElement->GetParent();
    }
    else // VisualRelativeKind_Root
    {
        ASSERT(iRelativeLinkKind == 2 /* VisualRelativeKind_Root */);
        pObjectNoRef = (CDependencyObject*)pElement->GetTreeRoot(true);
    }

    // If it's not NULL, return the object to the framework
    // with a ref count and with a type ID.
    if (pObjectNoRef != NULL)
    {
        pRelative->SetObjectAddRef(pObjectNoRef);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  ClearUIElementChildren
//
//  Clear the given element's visual children collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ClearUIElementChildren(
        _In_ CUIElement* pElement )
{
    CCollection* pChildren = pElement->GetChildren();

    if( pChildren != NULL )
    {
        IFC_RETURN( pChildren->Clear() );
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   GetManagedPropertyValueFromStyle
//
//  Synopsis: Get value of managed property from object's style or
//   built-in style. Returns null in CValue is property is not
//   found in style.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT GetManagedPropertyValueFromStyle(
    _In_ bool bUseBuiltInStyle,
    _In_ CDependencyObject* pElement,
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue,
    _Out_ bool* pbFoundValue)
{
    CFrameworkElement* pFrameworkElement = nullptr;
    CControl* pControl = nullptr;
    bool foundValue = false;

    IFCEXPECT_RETURN(pElement);
    IFCEXPECT_RETURN(pDP);

    if (bUseBuiltInStyle)
    {
        IFC_RETURN(DoPointerCast(pControl, pElement));

        IFC_RETURN(pControl->GetValueFromBuiltInStyle(
            pDP,
            pValue,
            &foundValue));
    }
    else
    {
        IFC_RETURN(DoPointerCast(pFrameworkElement, pElement));

        IFC_RETURN(pFrameworkElement->GetValueFromStyle(
            pDP,
            pValue,
            &foundValue));
    }

    *pbFoundValue = foundValue;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   DependencyObject_GetValue
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_GetValue(
    _In_ CDependencyObject* pObject,
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    IFCEXPECT_RETURN(pObject);

    ASSERT(valueAny == pValue->GetType());
    IFC_RETURN(pObject->GetValue(pDP, pValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   DependencyObject_SetValue
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_SetValue(
    _In_ CDependencyObject* pObject,
    _In_ const SetValueParams& args)
{
    // First see if we can get the DP from the index given
    IFCEXPECT_RETURN(pObject);

    {
    HRESULT xr = S_OK;

    CDependencyObject* pPreviousCaller = nullptr;
    pObject->GetContextInterface()->SetValueFromManaged(pObject, &pPreviousCaller);
    xr = pObject->SetValue(args);

    pObject->GetContextInterface()->SetValueFromManaged(pPreviousCaller);

    IFC_RETURN(xr);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:  DependencyObject_AddRef
//
//  Do an AddRef on a DO
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject_AddRef(
    _In_ CDependencyObject* pObject)
{
    HRESULT hr = S_OK;

    // NOTE: This function should always return S_OK.  We've made the return
    // type HRESULT for future versioning opportunity.  The managed pinvoke call
    // does not currently need to check the HRESULT, so that needs to be updated
    // if any new errors are returned.

    if (pObject != NULL)
    {
        AddRefInterface(pObject);
    }

//Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function:   WantsEvent
//
//  Synopsis:   Called for individual elements. Used for indicating whether
//              the managed object has listeners for a managed event that
//              has no native event counterpart. All event housekeeping is
//              done on the managed side, but we'll set a bit somewhere to
//              indicate that yes, the native side should call over to the
//              managed side for this event for this element.
//
//              The values for nEventIndex are arbitrary but must be kept
//              in sync with the managed framework.
//------------------------------------------------------------------------
_Check_return_ HRESULT WantsEvent(
    _In_ CDependencyObject* hElement,
    _In_ DirectUI::ManagedEvent nManagedEventIndex,
    _In_ bool bWantsToHandleEvent)
{
    CUIElement* pUIElement = do_pointer_cast<CUIElement>(hElement);

    switch (nManagedEventIndex)
    {
        case DirectUI::ManagedEvent::ManagedEventSizeChanged:
            IFCPTR_RETURN(pUIElement);
            pUIElement->SetWantsSizeChanged(bWantsToHandleEvent);
            break;

        case DirectUI::ManagedEvent::ManagedEventLayoutUpdated:
            {
                IFCPTR_RETURN(pUIElement);
                CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pUIElement);

                if (pLayoutManager)
                {
                    (bWantsToHandleEvent) ?
                        pLayoutManager->IncreaseLayoutUpdatedSubscriberCounter() :
                        pLayoutManager->DecreaseLayoutUpdatedSubscriberCounter();
                }
            }
            break;

        case DirectUI::ManagedEvent::ManagedEventInheritanceContextChanged:
            hElement->SetWantsInheritanceContextChanged(bWantsToHandleEvent);
            break;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Similar to WantsEvent, but does not require a DO passed in.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT WantsEventStatic(
    _In_ CCoreServices* pCore,
    _In_ DirectUI::ManagedEvent nManagedEventIndex,
    _In_ bool bWantsToHandleEvent)
{
    IFCEXPECT_RETURN(pCore);

    switch (nManagedEventIndex)
    {
    case DirectUI::ManagedEvent::ManagedEventRendering:
        IFC_RETURN(pCore->SetWantsRendering(bWantsToHandleEvent));
        break;
    case DirectUI::ManagedEvent::ManagedEventRendered:
        pCore->SetWantsCompositionTargetRenderedEvent(bWantsToHandleEvent);
        break;
    default:
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreateObjectByTypeIndex
//
//  Synopsis:
//      Given a known type index (example: INDEX_DEPENDENCYOBJECT), create
//  a new instance of the type.
//
//------------------------------------------------------------------------
// Closely follows the precedence of the FindName() exposed for managed layer.
// Except instead of finding an instance of an existing object identified by
// a string (name), we are creating a new instance identified by the type index.
_Check_return_ HRESULT CreateObjectByTypeIndex(
    _In_ CCoreServices* pCore,
    _In_ KnownTypeIndex iTypeIndex,
    _Out_ CDependencyObject** ppObject)
{
    *ppObject = NULL;
    CDependencyObject *pDO = NULL;
    const CClassInfo* pClassInfo = NULL;

    IFCEXPECT_RETURN(pCore);

    // Unknown: Any validation possible/necessary for iTypeIndex?
    pClassInfo = MetadataAPI::GetClassInfoByIndex(iTypeIndex);

    IFC_RETURN(pCore->CreateObject(pClassInfo, xstring_ptr::NullString(), &pDO));

    *ppObject = static_cast<CDependencyObject*>(pDO);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   NotifyHasManagedPeer
//
//  Synopsis:
//      Called by the framework to indicate that a managed peer
//      has been created for a CDependencyObject.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT NotifyHasManagedPeer(
    _In_ XHANDLE hObject,
    _In_ ValueType nType,
    _In_ bool bIsCustomType,
    _In_ bool bIsGCRoot)
{
    switch (nType)
    {
    case valueObject :
        {
            CDependencyObject *pIO = NULL;

            pIO = (CDependencyObject *) hObject;
            IFCEXPECT_RETURN(pIO);

            #if DBG
            // Set a flag that OnManagedPeerCreated is being called
            pIO->PreOnManagedPeerCreated();
            #endif

            IFC_RETURN(pIO->OnManagedPeerCreated(bIsCustomType, bIsGCRoot));

            #if DBG
            // Assert that OnManagedPeerCreated was called on the base (not completely overridden)
            pIO->PostOnManagedPeerCreated();
            #endif
        }
        break;
    default :
        ASSERT(0);
        break;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   GetClassFullName
//
//  Synopsis:
//      Called by the framework to get the full name of a core class,
//      including the namespace
//
//------------------------------------------------------------------------
_Check_return_ HRESULT GetClassFullName(
    _In_ CCoreServices* pCore,
    _In_ const xstring_ptr_view& strClassName,
    _Inout_ CValue* pFullName)
{
    xstring_ptr strFullName;

    IFCEXPECT_RETURN(pCore);

    IFC_RETURN(pCore->GetFullClassName(strClassName, &strFullName));

    pFullName->SetString(strFullName);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function: TESTSignalManagedMemLeak
//
//  Synopsis: Signals the unit tests that there is a managed memory leak and returns
//                 whether or not to show an Assert dialog on the managed side.
//
//------------------------------------------------------------------------
extern "C"
void
CDECL
TESTSignalManagedMemLeak(_Out_ bool *showAssert)
{
#if DEBUG
    GetPALDebuggingServices()->TESTSignalManagedMemLeak(showAssert);
#else
    *showAssert = false;
#endif
}

//------------------------------------------------------------------------
//
//  Method:   ContentControl_SetContentIsNotLogical
//
//  Synopsis:
//    sets a flag in contentcontrol indicating that it should not treat its content as
//    its logical child. this is called when contentcontrol is generated as a wrapper for
//    an item in itemscontrol.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ContentControl_SetContentIsNotLogical(
    _In_ CContentControl* pContentControl)
{
    pContentControl->SetContentIsLogical(false);

    return S_OK;
}

// Try to get the content template root.  This is a "try" because the CUIElement
// that is returned may not have a peer.
_Check_return_ HRESULT ContentControl_TryGetContentTemplateRoot(
    _In_ CContentControl* pContentControl,
    _Outptr_ CUIElement** ppContentTemplateRoot)
{
    IFCPTR_RETURN(pContentControl);
    IFCCATASTROPHIC_RETURN(ppContentTemplateRoot);

    // this returns addreffed
    *ppContentTemplateRoot = pContentControl->GetContentTemplateRoot().detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ScrollContentControl_SetRootScrollViewerOriginalHeight
//
//  Synopsis:
//    Set the root ScrollViewer original height.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ScrollContentControl_SetRootScrollViewerOriginalHeight(
    _In_ CScrollContentControl* pScrollContentControl,
    _In_ XFLOAT fOriginalHeight)
{
    IFCEXPECT_RETURN(pScrollContentControl);

    // Set the original root ScrollViewer height
    pScrollContentControl->SetRootScrollViewerOriginalHeight(fOriginalHeight);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ScrollContentControl_SetRootScrollViewerSetting
//
//  Synopsis:
//    Sets a flag for RootScrollViewerSetting
//
//    RootScrollViewerSetting_Null                        = 0x00,
//    RootScrollViewerSetting_ApplyTemplate               = 0x01,
//    RootScrollViewerSetting_ProcessWindowSizeChanged    = 0x02,
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ScrollContentControl_SetRootScrollViewerSetting(
    _In_ CScrollContentControl* pScrollContentControl,
    _In_ XUINT8 setting,
    _In_ bool bValue)
{
    IFCEXPECT_RETURN(pScrollContentControl);

    if (setting == RootScrollViewerSetting_ApplyTemplate)
    {
        pScrollContentControl->SetRootScrollViewerSettingApplyTemplate(bValue);
    }
    else if (setting == RootScrollViewerSetting_ProcessWindowSizeChanged)
    {
        pScrollContentControl->SetRootScrollViewerSettingWindowSizeChanged(bValue);
    }
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   ScrollContentControl_GetRootScrollViewerSetting
//
//  Synopsis:
//    Gets a flag for RootScrollViewerSetting
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ScrollContentControl_GetRootScrollViewerSetting(
    _In_ CScrollContentControl* pScrollContentControl,
    _In_ XUINT8 setting,
    _Out_ bool& bValue)
{
    IFCEXPECT_RETURN(pScrollContentControl);

    if (setting == RootScrollViewerSetting_ApplyTemplate)
    {
        bValue = pScrollContentControl->HasRootScrollViewerApplyTemplate();
    }
    else if (setting == RootScrollViewerSetting_ProcessWindowSizeChanged)
    {
        bValue = pScrollContentControl->HasRootScrollViewerProcessWindowSizeChanged();
    }
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ScrollContentControl_SetFocusOnFlyoutLightDismissPopupByPointer
//
//  Synopsis:
//    Set focus on the lightdismiss Popup of Flyout control.
//------------------------------------------------------------------------

_Check_return_ HRESULT ScrollContentControl_SetFocusOnFlyoutLightDismissPopupByPointer(
    _In_ CScrollContentControl* pScrollContentControl,
    _Out_ bool* pfIsFocusedOnLightDismissPopupOfFlyout)
{
    CPopupRoot* pPopupRoot = NULL;
    CPopup* pPopup = NULL;

    *pfIsFocusedOnLightDismissPopupOfFlyout = false;

    if (pScrollContentControl->GetRootOfPopupSubTree())
    {
        IFC_RETURN(VisualTree::GetPopupRootForElementNoRef(pScrollContentControl, &pPopupRoot));
        if (pPopupRoot)
        {
            pPopup = pPopupRoot->GetTopmostPopup(CPopupRoot::PopupFilter::LightDismissOnly);
            if (pPopup && Focus::FocusSelection::ShouldUpdateFocus(pPopup->m_pChild, pPopup->GetSavedFocusState()) && pPopup->IsFlyout())
            {
                bool wasFocusUpdated = false;
                IFC_RETURN(pPopup->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &wasFocusUpdated));
                *pfIsFocusedOnLightDismissPopupOfFlyout = wasFocusUpdated;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ContentControl_SetContentIsTemplateBoundManaged
//
//  Synopsis:
//    Allows the managed side to indicate that the content is templatebound.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ContentControl_SetContentIsTemplateBoundManaged(
    _In_ CContentControl* pContentControl,
    _In_ bool bIsTemplateBoundByManaged)
{
    pContentControl->SetContentIsTemplateboundManaged(bIsTemplateBoundByManaged);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   SetFrameworkContext
//
//  Synopsis:
//      Hold an opaque context value from the framework in the core context.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT SetFrameworkContext(
    _In_ CCoreServices* pCore,
    _In_ XHANDLE context )
{
    IFCEXPECT_RETURN(pCore);

    pCore->SetFrameworkContext(context);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   GetDefaultValue
//
//  Synopsis:
//      Get default value of property. Owner of property must be
//  be passed either using pPropertyOwner or nPropertyId. Otherwise
//  the correct owner of overridden properties (like Stretch) cannot
//  be determined.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT Property_GetDefaultValue(
    _In_ CCoreServices* pCore,
    _In_ CDependencyObject* pObject,
    _In_ KnownTypeIndex nObjectCoreTypeIndex,
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pDefaultValue)
{
    IFCEXPECT_RETURN(pCore);

    if (pObject != NULL)
    {
        // Get default value of property
        IFC_RETURN(pObject->GetDefaultValue(pDP, pDefaultValue));
    }
    else if (nObjectCoreTypeIndex != KnownTypeIndex::UnknownType)
    {
        if (pDP->IsAttached())
        {
            nObjectCoreTypeIndex = pDP->GetDeclaringType()->m_nIndex;
        }

        // Get class that owns property
        const CClassInfo* pClass = MetadataAPI::GetClassInfoByIndex(nObjectCoreTypeIndex);

        // Get default value of property
        IFC_RETURN(pDP->GetDefaultValue(pCore, /* pReferenceObject */ nullptr, pClass, pDefaultValue));
    }
    else
    {
        // Either owner DO or owner's type ID must be provided
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT RefreshXamlSchemaContext(
    _In_ CCoreServices* pCore)
{
    IFCEXPECT_RETURN(pCore);

    IFC_RETURN(pCore->RefreshXamlSchemaContext());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetCurrentState
//
//  Synopsis:
//      Returns the state a VisualStateGroup is currently in, or
//      nothing if there is no current state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT VisualStateGroup_GetCurrentState(
    _In_ CVisualStateGroup* pVisualStateGroup,
    _Out_ CValue *pCurrentState)
{
    CVisualState* pCurrentVisualState = nullptr;

    IFCEXPECT_RETURN(pVisualStateGroup);
    IFCEXPECT_RETURN(pCurrentState);

    // will addref
    IFC_RETURN(pVisualStateGroup->GetCurrentVisualState(&pCurrentVisualState));

    // A vsg does not necessarily have a current visual state
    if (pCurrentVisualState)
    {
        pCurrentState->SetObjectNoRef(pCurrentVisualState);
        pCurrentVisualState = NULL;
    }
    else
    {
        pCurrentState->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   XamlSchemaContext_AddAssemblyXmlnsDefinition
//
//  Synopsis:
//      Adds a TypeNamespace reference that can be associated with
//      the supplied xmlns uri.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT XamlSchemaContext_AddAssemblyXmlnsDefinition(
    _In_ CCoreServices* pCore,
    _In_ XamlAssemblyToken tAssembly,
    _In_ const xstring_ptr& strXmlNamespace,
    _In_ XamlTypeNamespaceToken tTypeNamespace,
    _In_ const xstring_ptr& strTypeNamespace)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;

    IFCEXPECT_RETURN(pCore);
    spXamlSchemaContext = pCore->GetSchemaContext();
    IFCEXPECT_RETURN(spXamlSchemaContext);

    IFC_RETURN(spXamlSchemaContext->AddAssemblyXmlnsDefinition(tAssembly, strXmlNamespace, tTypeNamespace, strTypeNamespace));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   EasingFunctionBase_Ease
//
//  Synopsis:
//      Calls the core Ease method for the provided IEasingFunctionBase.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT EasingFunctionBase_Ease(
    _In_ CEasingFunctionBase* pEasingFunction,
    _In_ XFLOAT fNormalizedTime,
    _Out_ XFLOAT* pfAlpha)
{
    IFCEXPECT_RETURN(pfAlpha);
    *pfAlpha = pEasingFunction->Ease(fNormalizedTime);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   WriteableBitmap_Invalidate
//
//  Synopsis:
//      Marks the WriteableBitmap object as dirty.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT WriteableBitmap_Invalidate(
    _In_ CWriteableBitmap* pWriteableBitmap)
{
    IFCPTR_RETURN(pWriteableBitmap);

    IFC_RETURN(pWriteableBitmap->Invalidate());

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   WriteableBitmap_CopyPixels
//
//  Synopsis:
//      Called from WriteableBitmap.SetSource
//      WriteableBitmap.SetSource uses the base implementation found in
//      BitmapSource to decode a source stream into WriteableBitmap's
//      surface. This is done on the native side.
//      WriteableBitmap allows access to the pixels, hence, the surface
//      contents must be copied to a managed array and that array will be
//      used by WriteableBitmap's surface.
//      This method takes in a pointer to the managed array, initializes
//      it with the current contents of WriteableBitmap's surface and then
//      sets that pointer as the bits of the surface.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT WriteableBitmap_CopyPixels(
    _In_ CWriteableBitmap* pWriteableBitmap,
    _In_ XHANDLE hPixels)
{
    IFCPTR_RETURN(pWriteableBitmap);

    IFC_RETURN(pWriteableBitmap->CopyPixels(hPixels));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   WriteableBitmap_Create
//
//  Synopsis:
//      Creates a new WriteableBitmap
//
//------------------------------------------------------------------------
_Check_return_ HRESULT WriteableBitmap_Create(
    _In_ CWriteableBitmap* pWriteableBitmap,
    _In_ XHANDLE hPixels,
    _In_ XINT32 iWidth,
    _In_ XINT32 iHeight)
{
    IFCPTR_RETURN(pWriteableBitmap);

    IFC_RETURN(pWriteableBitmap->Create(hPixels, iWidth, iHeight));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the invisible hit test mode flag.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HostProperties_SetInvisibleHitTestMode(
    _In_ CCoreServices* pCore,
    _In_ bool bInvisibleHitTestMode)
{
    IFCEXPECT_RETURN(pCore);
    pCore->SetInvisibleHitTestMode(bInvisibleHitTestMode);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ClearErrorOccurredDuringLayout
//
//  Synopsis:
//      Sets a flag indicating whether an error ocurred during layout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutManager_ClearErrorOccurredDuringLayout(
    _In_ CCoreServices* pCore)
{
    CLayoutManager *layoutManager;

    // Verify parameters
    IFCEXPECT_RETURN(pCore);

    layoutManager = pCore->GetMainLayoutManager();
    if (layoutManager)
    {
        layoutManager->ClearErrorOccurredDuringLayout();
    }
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   DidErrorOccurDuringLayout
//
//  Synopsis:
//      Gets a flag indicating whether an error ocurred during layout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT LayoutManager_DidErrorOccurDuringLayout(
    _In_ CCoreServices* pCore,
    _Out_ bool* bErrorOccurred)
{
    CLayoutManager *layoutManager = NULL;

    // Verify parameters
    IFCEXPECT_RETURN(pCore);

    layoutManager = pCore->GetMainLayoutManager();

    if (layoutManager)
    {
        *bErrorOccurred = layoutManager->DidErrorOccurDuringLayout();
    }
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the baseline offset property of passed text control.
//------------------------------------------------------------------------
_Check_return_ HRESULT Text_GetBaselineOffset(
    _In_ CDependencyObject* pObject,
    _Out_ XFLOAT *pfBaselineOffset)
{
    if (!pObject)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    if (auto pTextBoxBase = do_pointer_cast<CTextBoxBase>(pObject))
    {
        IFC_RETURN(pTextBoxBase->GetBaselineOffset(pfBaselineOffset));
    }
    else if (auto pTextBlock = do_pointer_cast<CTextBlock>(pObject))
    {
        IFC_RETURN(pTextBlock->GetBaselineOffset(pfBaselineOffset));
    }
    else if (auto pRichTextBlock = do_pointer_cast<CRichTextBlock>(pObject))
    {
        IFC_RETURN(pRichTextBlock->GetBaselineOffset(pfBaselineOffset));
    }
    else if (auto pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pObject))
    {
        IFC_RETURN(pRichTextBlockOverflow->GetBaselineOffset(pfBaselineOffset));
    }
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the Element Edge TextPointer of a TextElement
//------------------------------------------------------------------------
_Check_return_ HRESULT TextElement_GetEdge(
    _In_ CTextElement* pTextElement,
    _In_ XINT32 iEdgeIndex,
    _Out_ CValue* pTextPointer)
{

    IFCEXPECT_RETURN(pTextElement);

    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;
    switch(iEdgeIndex)
    {
        case CTextPointerWrapper::ElementEdge::ElementStart:
            IFC_RETURN(pTextElement->GetElementStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;

        case CTextPointerWrapper::ElementEdge::ContentStart:
            IFC_RETURN(pTextElement->GetContentStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;

        case CTextPointerWrapper::ElementEdge::ContentEnd:
            IFC_RETURN(pTextElement->GetContentEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;

        case CTextPointerWrapper::ElementEdge::ElementEnd:
            IFC_RETURN(pTextElement->GetElementEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;
    }

    if (pTextPointerWrapper != NULL)
    {
        pTextPointer->SetObjectNoRef(pTextPointerWrapper.detach());
    }
    else
    {
        pTextPointer->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the logical direction of the TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT TextPointer_GetLogicalDirection(
    _In_ CTextPointerWrapper* pTextPointer,
    _Out_ CValue* pLogicalDirection)
{
    XUINT32 logicalDirectionValue = 0;

    IFCEXPECT_RETURN(pTextPointer);

    IFC_RETURN(pTextPointer->GetLogicalDirection(&logicalDirectionValue));
    pLogicalDirection->SetEnum(logicalDirectionValue);

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieve the offset of a TextPointer
//------------------------------------------------------------------------
_Check_return_ HRESULT TextPointer_GetOffset(
    _In_ CTextPointerWrapper* pTextPointer,
    _Out_ XINT32* piOffset)
{
    IFCEXPECT_ASSERT_RETURN(pTextPointer);
    IFC_RETURN(pTextPointer->GetOffset(piOffset));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the TextElement enclosing the TextPointer.
//------------------------------------------------------------------------
_Check_return_ HRESULT TextPointer_GetParent(
    _In_ CTextPointerWrapper* pTextPointer,
    _Out_ CValue* pParent)
{
    CDependencyObject   *pParentElement      = NULL;

    IFCEXPECT_ASSERT_RETURN(pTextPointer);

    // pParentElement wasn't AddRef'ed.
    IFC_RETURN(pTextPointer->GetParent(&pParentElement));
    pParent->SetObjectAddRef(pParentElement);

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the FrameWorkElement containing the TextPointer.
//------------------------------------------------------------------------
_Check_return_ HRESULT TextPointer_GetVisualParent(
    _In_ CTextPointerWrapper* pTextPointer,
    _Out_ CValue* pParent)
{
    CFrameworkElement *pParentElement = NULL;

    IFCEXPECT_ASSERT_RETURN(pTextPointer);

    // pParentElement wasn't AddRef'ed.
    IFC_RETURN(pTextPointer->GetVisualParent(&pParentElement));
    pParent->SetObjectAddRef(pParentElement);

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets an insertion position at a given offset from another position.
//------------------------------------------------------------------------
_Check_return_ HRESULT TextPointer_GetPositionAtOffset(
    _In_ CTextPointerWrapper* pTextPointer,
    _In_ XINT32 iOffset,
    _In_ XINT32 iLogicalDirection,
    _Out_ CValue* pOffsetTextPointer)
{

    IFCEXPECT_ASSERT_RETURN(pTextPointer != NULL);

    xref_ptr<CTextPointerWrapper> pTextPointerAtOffset;
    IFC_RETURN(pTextPointer->GetPositionAtOffset(
        iOffset,
        static_cast<RichTextServices::LogicalDirection::Enum>(iLogicalDirection),
        pTextPointerAtOffset.ReleaseAndGetAddressOf()));

    if (pTextPointerAtOffset != NULL)
    {
        pOffsetTextPointer->SetObjectAddRef(pTextPointerAtOffset);
    }
    else
    {
        pOffsetTextPointer->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Get a character rect representing this text pointer
//------------------------------------------------------------------------
_Check_return_ HRESULT TextPointer_GetCharacterRect(
    _In_ CTextPointerWrapper* pTextPointer,
    _In_ XINT32 iLogicalDirection,
    _Out_ wf::Rect* pRect)
{
    XRECTF rect;

    IFCEXPECT_ASSERT_RETURN(pTextPointer != NULL);

    IFC_RETURN(pTextPointer->GetCharacterRect(
        static_cast<RichTextServices::LogicalDirection::Enum>(iLogicalDirection),
        &rect));

    pRect->X = rect.X;
    pRect->Y = rect.Y;
    pRect->Width = rect.Width;
    pRect->Height = rect.Height;

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the TextBlock's Edges
//------------------------------------------------------------------------
_Check_return_ HRESULT TextBlock_GetEdge(
    _In_ CTextBlock* pTextBlock,
    _In_ XINT32 iEdgeIndex,
    _Out_ CValue* pTextPointer)
{

    IFCEXPECT_ASSERT_RETURN(pTextBlock);

    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;
    switch(iEdgeIndex)
    {
        case CTextPointerWrapper::ElementEdge::ContentStart:
            IFC_RETURN(pTextBlock->GetContentStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;

        case CTextPointerWrapper::ElementEdge::ContentEnd:
            IFC_RETURN(pTextBlock->GetContentEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;
    }

    if (pTextPointerWrapper != NULL)
    {
        pTextPointer->SetObjectAddRef(pTextPointerWrapper);
    }
    else
    {
        pTextPointer->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the TextBlock's Selection Edges
//------------------------------------------------------------------------
_Check_return_ HRESULT TextBlock_GetSelectionEdge(
    _In_ CTextBlock* pTextBlock,
    _In_ XINT32 iEdgeIndex,
    _Out_ CValue* pTextPointer)
{

    IFCEXPECT_ASSERT_RETURN(pTextBlock);

    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;
    switch(iEdgeIndex)
    {
        case CTextPointerWrapper::ElementEdge::ContentStart:
            IFC_RETURN(pTextBlock->GetSelectionStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;

        case CTextPointerWrapper::ElementEdge::ContentEnd:
            IFC_RETURN(pTextBlock->GetSelectionEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;
    }

    if (pTextPointerWrapper != NULL)
    {
        pTextPointer->SetObjectAddRef(pTextPointerWrapper);
    }
    else
    {
        pTextPointer->SetNull();
    }

    return S_OK;
}


//------------------------------------------------------------------------
//  Summary:
//      Make the given TextPointer pair as the new Selection on TextBlock.
//------------------------------------------------------------------------
_Check_return_ HRESULT TextBlock_Select(
    _In_ CTextBlock* pTextBlock,
    _In_ CValue* pAnchorPositionTextPointer,
    _In_ CValue* pMovingPositionTextPointer)
{
    CTextPointerWrapper *pAnchorPositionWrapper = NULL;
    CTextPointerWrapper *pMovingPositionWrapper = NULL;

    if (!pTextBlock ||
        !pAnchorPositionTextPointer ||
        !pAnchorPositionTextPointer->AsObject() ||
        !pAnchorPositionTextPointer->AsObject()->OfTypeByIndex<KnownTypeIndex::TextPointerWrapper>() ||
        !pMovingPositionTextPointer ||
        !pMovingPositionTextPointer->AsObject() ||
        !pMovingPositionTextPointer->AsObject()->OfTypeByIndex<KnownTypeIndex::TextPointerWrapper>())
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    pAnchorPositionWrapper = do_pointer_cast<CTextPointerWrapper>(pAnchorPositionTextPointer->AsObject());
    pMovingPositionWrapper = do_pointer_cast<CTextPointerWrapper>(pMovingPositionTextPointer->AsObject());

    IFC_RETURN(pTextBlock->Select(pAnchorPositionWrapper, pMovingPositionWrapper));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the text for a CRichTextBlock
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock_GetPlainText(
    _In_ CFrameworkElement* pElement,
    _Out_ xstring_ptr* pstrPlainText)
{
    IFC_RETURN(CRichTextBlock::GetPlainText(
            pElement,
            pstrPlainText));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Make the given TextPointer pair as the new Selection on CRichTextBlock.
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock_Select(
    _In_ CRichTextBlock* pRichTextBlock,
    _In_ CValue* pAnchorPositionTextPointer,
    _In_ CValue* pMovingPositionTextPointer)
{
    CTextPointerWrapper *pAnchorPositionWrapper = NULL;
    CTextPointerWrapper *pMovingPositionWrapper = NULL;

    if (!pRichTextBlock ||
        !pRichTextBlock->OfTypeByIndex<KnownTypeIndex::RichTextBlock>() ||
        !pAnchorPositionTextPointer ||
        !pAnchorPositionTextPointer->AsObject() ||
        !pAnchorPositionTextPointer->AsObject()->OfTypeByIndex<KnownTypeIndex::TextPointerWrapper>() ||
        !pMovingPositionTextPointer ||
        !pMovingPositionTextPointer->AsObject() ||
        !pMovingPositionTextPointer->AsObject()->OfTypeByIndex<KnownTypeIndex::TextPointerWrapper>())
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    pAnchorPositionWrapper = do_pointer_cast<CTextPointerWrapper>(pAnchorPositionTextPointer->AsObject());
    pMovingPositionWrapper = do_pointer_cast<CTextPointerWrapper>(pMovingPositionTextPointer->AsObject());

    IFC_RETURN(pRichTextBlock->Select(pAnchorPositionWrapper, pMovingPositionWrapper));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the CRichTextBlock's Edges
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock_GetEdge(
    _In_ CFrameworkElement* pElement,
    _In_ XINT32 iEdgeIndex,
    _Out_ CValue* pTextPointer)
{
    CRichTextBlock *pRichTextBlock = NULL;
    CRichTextBlockOverflow *pRichTextBlockOverflow = NULL;

    pRichTextBlock = do_pointer_cast<CRichTextBlock>(pElement);
    pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pElement);

    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;
    switch(iEdgeIndex)
    {
        case CTextPointerWrapper::ElementEdge::ContentStart:
            if (pRichTextBlock != NULL)
            {
                IFC_RETURN(pRichTextBlock->GetContentStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else if (pRichTextBlockOverflow != NULL)
            {
                IFC_RETURN(pRichTextBlockOverflow->GetContentStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else
            {
                IFC_RETURN(E_UNEXPECTED);
            }
            break;

        case CTextPointerWrapper::ElementEdge::ContentEnd:
            if (pRichTextBlock != NULL)
            {
                IFC_RETURN(pRichTextBlock->GetContentEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else if (pRichTextBlockOverflow != NULL)
            {
                IFC_RETURN(pRichTextBlockOverflow->GetContentEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else
            {
                IFC_RETURN(E_UNEXPECTED);
            }
            break;
    }

    if (pTextPointerWrapper != NULL)
    {
        pTextPointer->SetObjectAddRef(pTextPointerWrapper);
    }
    else
    {
        pTextPointer->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the CRichTextBlock's Selection Edges
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock_GetSelectionEdge(
    _In_ CRichTextBlock* pRichTextBlock,
    _In_ XINT32 iEdgeIndex,
    _Out_ CValue* pTextPointer)
{

    IFCEXPECT_ASSERT_RETURN(pRichTextBlock);

    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;
    switch(iEdgeIndex)
    {
        case CTextPointerWrapper::ElementEdge::ContentStart:
            IFC_RETURN(pRichTextBlock->GetSelectionStart(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;

        case CTextPointerWrapper::ElementEdge::ContentEnd:
            IFC_RETURN(pRichTextBlock->GetSelectionEnd(pTextPointerWrapper.ReleaseAndGetAddressOf()));
            break;
    }

    if (pTextPointerWrapper != NULL)
    {
        pTextPointer->SetObjectAddRef(pTextPointerWrapper);
    }
    else
    {
        pTextPointer->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Public hit-test API to get a TextPointer from a point in
//      CRichTextBlock/Overflow's coordinate space.
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlock_GetTextPositionFromPoint(
    _In_ CFrameworkElement* pElement,
    _In_ XPOINTF point,
    _Out_ CValue *pTextPointer)
{
    CRichTextBlock *pRichTextBlock = NULL;
    CRichTextBlockOverflow *pRichTextBlockOverflow = NULL;

    IFCEXPECT_ASSERT_RETURN(pElement);

    pRichTextBlock = do_pointer_cast<CRichTextBlock>(pElement);
    pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pElement);

    xref_ptr<CTextPointerWrapper> pTextPointerWrapper;
    if (pRichTextBlock != NULL)
    {
        IFC_RETURN(pRichTextBlock->GetTextPositionFromPoint(point, pTextPointerWrapper.ReleaseAndGetAddressOf()));
    }
    else if (pRichTextBlockOverflow != NULL)
    {
        IFC_RETURN(pRichTextBlockOverflow->GetTextPositionFromPoint(point, pTextPointerWrapper.ReleaseAndGetAddressOf()));
    }
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    if (pTextPointerWrapper != NULL)
    {
        pTextPointer->SetObjectAddRef(pTextPointerWrapper);
    }
    else
    {
        pTextPointer->SetNull();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetMaster
//
//  Synopsis:
//      Gets the master CRichTextBlock for CRichTextBlockOverflow element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichTextBlockOverflow_GetMaster(
    _In_ CRichTextBlockOverflow* pObject,
    _Inout_ CRichTextBlock** ppMaster
    )
{
    IFCPTR_RETURN(pObject);
    *ppMaster = pObject->m_pMaster;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   GetLinkHostFrameworkElement
//
//  Synopsis:   Given a Link, find the FrameworkElement that hosts it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT GetLinkHostFrameworkElement(
    _In_ CInline* pLink,
    _Out_ CValue* pHostElement)
{
    CFrameworkElement *pHostFrameworkElement = nullptr;

    IFCEXPECT_RETURN(pLink);

    pHostFrameworkElement = pLink->GetContainingFrameworkElement();

    // If it's not NULL, return the object to the caller
    // with a ref count and with a type ID.
    if (pHostFrameworkElement != nullptr)
    {
        pHostElement->SetObjectAddRef(pHostFrameworkElement);
    }
    else
    {
        pHostElement->SetNull();
    }

    return S_OK;
}

_Check_return_ HRESULT CoreServices_SetCustomResourceLoader(
    _In_ CCoreServices *pCore,
    _In_opt_ ICustomResourceLoader *pLoader
    )
{
    pCore->SetCustomResourceLoader(pLoader);
    RRETURN(S_OK);
}

_Check_return_ HRESULT CoreServices_GetCustomResourceLoader(
    _In_ CCoreServices *pCore,
    _Outptr_ ICustomResourceLoader **ppLoader
    )
{
    IFCPTR_RETURN(ppLoader);
    *ppLoader = pCore->GetCustomResourceLoader();
    AddRefInterface(*ppLoader);

    return S_OK;
}

_Check_return_ HRESULT CoreServices_TryGetApplicationResource(
    _In_ CCoreServices *pCore,
    _In_ const xstring_ptr& strUri,
    _Outptr_result_maybenull_ IPALMemory **ppMemory,
    _Out_ bool *pfIsBinaryXaml
    )
{
    xref_ptr<IPALUri> spUri;

    *ppMemory = nullptr;
    *pfIsBinaryXaml = false;

    IFC_RETURN(gps->UriCreate(strUri.GetCount(), strUri.GetBuffer(), spUri.ReleaseAndGetAddressOf()));
    IFC_NOTRACE_RETURN(pCore->TryLoadXamlResource(spUri, pfIsBinaryXaml, ppMemory, nullptr));

    return S_OK;
}

_Check_return_ HRESULT CoreServices_CombineResourceUri(
    _In_ CCoreServices *pCore,
    _In_ const xstring_ptr_view& strBaseUri,
    _In_ const xstring_ptr_view& strUri,
    _Out_ xruntime_string_ptr* pstrCombinedUri
    )
{
    xref_ptr<IPALResourceManager> pResourceManager;
    xstring_ptr strLocalCombinedUri;

    wrl::ComPtr<IPALUri> pBaseUri;
    IFC_RETURN(gps->UriCreate(strBaseUri.GetCount(), strBaseUri.GetBuffer(), &pBaseUri));

    IFC_RETURN(pCore->GetResourceManager(pResourceManager.ReleaseAndGetAddressOf()));
    wrl::ComPtr<IPALUri> pCombinedUri;
    IFC_RETURN(pResourceManager->CombineResourceUri(pBaseUri.Get(), strUri, &pCombinedUri));
    IFC_RETURN(pCombinedUri->GetCanonical(&strLocalCombinedUri));
    IFC_RETURN(strLocalCombinedUri.Promote(pstrCombinedUri));

    return S_OK;
}

_Check_return_ HRESULT CoreServices_GetBaseUri(
    _In_ CCoreServices *pCore,
    _Out_ xstring_ptr* pstrBaseUri
    )
{
    IPALUri *pBaseUri = pCore->GetBaseUriNoRef();

    IFC_RETURN(pBaseUri->GetCanonical(pstrBaseUri));

    return S_OK;
}

_Check_return_ HRESULT Parser_GenerateBinaryXaml(
    _In_                                        CCoreServices  *pCore,
    _In_reads_bytes_(cXamlTextBufferSize)            XBYTE          *pXamlTextBuffer,
    _In_                                        XUINT32         cXamlTextBufferSize,
    _Outptr_result_bytebuffer_(*pcXamlBinaryBufferSize) XBYTE         **ppXamlBinaryBuffer,
    _Out_                                       XUINT32        *pcXamlBinaryBufferSize
    )
{
    HRESULT hr = S_OK;
    std::shared_ptr<XbfWriter> spXbfWriter;

    pCore->SetIsGeneratingBinaryXaml(true);

    spXbfWriter = std::make_shared<XbfWriter>(pCore, Parser::Versioning::OSVersions::WINBLUE);
    IFC(spXbfWriter->ProcessXamlTextBuffer(pXamlTextBuffer, cXamlTextBufferSize, ppXamlBinaryBuffer, pcXamlBinaryBufferSize));

Cleanup:
    pCore->SetIsGeneratingBinaryXaml(false);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_NotifyCanManipulateElements
//
//  Synopsis:
//    Called when the container's ability to manipulate
//    elements has changed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_NotifyCanManipulateElements(
    _In_ XHANDLE hManipulationHandler,
    _In_ bool fCanManipulateElementsByTouch,
    _In_ bool fCanManipulateElementsNonTouch,
    _In_ bool fCanManipulateElementsWithBringIntoViewport)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifyCanManipulateElements(fCanManipulateElementsByTouch, fCanManipulateElementsNonTouch, fCanManipulateElementsWithBringIntoViewport));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_NotifyManipulatableElementChanged
//
//  Synopsis:
//    Called when:
//     - originally, when IDirectManipulationContainer.put_Handler is called in order to declare the existing manipulated elements.
//     - afterwards, whenever the list of manipulated elements has changed.
//    pOldManipulatableElement == NULL && pNewManipulatableElement != NULL ==> a new manipulated element is available
//    pOldManipulatableElement != NULL && pNewManipulatableElement == NULL ==> an old manipulated element is gone
//    pOldManipulatableElement != NULL && pNewManipulatableElement != NULL ==> an old manipulated element was replaced with another one
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_NotifyManipulatableElementChanged(
    _In_ XHANDLE hManipulationHandler,
    _In_opt_ CUIElement* pOldManipulatableElement,
    _In_opt_ CUIElement* pNewManipulatableElement)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifyManipulatableElementChanged(pOldManipulatableElement, pNewManipulatableElement));

    return S_OK;
}

_Check_return_ HRESULT ManipulationHandler_NotifySecondaryContentAdded(
    _In_ XHANDLE hManipulationHandler,
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement,
    _In_ XDMContentType contentType)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pContentElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifySecondaryContentAdded(pManipulatableElement, pContentElement, contentType));

    return S_OK;
}

_Check_return_ HRESULT ManipulationHandler_NotifySecondaryContentRemoved(
    _In_ XHANDLE hManipulationHandler,
    _In_opt_ CUIElement* pManipulatableElement,
    _In_ CUIElement* pContentElement)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pContentElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifySecondaryContentRemoved(pManipulatableElement, pContentElement));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_NotifyViewportChanged
//
//  Synopsis:
//    Called when one or more viewport characteristic has changed.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT ManipulationHandler_NotifyViewportChanged(
    _In_ XHANDLE hManipulationHandler,
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
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);
    IFCEXPECT_RETURN(pfConfigurationsUpdated);
    *pfConfigurationsUpdated = false;

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifyViewportChanged(
        pManipulatedElement,
        fInManipulation,
        fBoundsChanged,
        fTouchConfigurationChanged,
        fNonTouchConfigurationChanged,
        fConfigurationsChanged,
        fChainedMotionTypesChanged,
        fHorizontalOverpanModeChanged,
        fVerticalOverpanModeChanged,
        pfConfigurationsUpdated));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_NotifyPrimaryContentChanged
//
//  Synopsis:
//    Called when one or more primary content characteristic has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_NotifyPrimaryContentChanged(
    _In_ XHANDLE hManipulationHandler,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fLayoutRefreshed,
    _In_ bool fBoundsChanged,
    _In_ bool fHorizontalChanged,
    _In_ bool fVerticalChanged,
    _In_ bool fZoomFactorBoundaryChanged)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifyPrimaryContentChanged(
        pManipulatedElement,
        fInManipulation,
        fLayoutRefreshed,
        fBoundsChanged,
        fHorizontalChanged,
        fVerticalChanged,
        fZoomFactorBoundaryChanged));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_NotifyPrimaryContentTransformChanged
//
//  Synopsis:
//    Called when one or more primary content transform characteristic
//    has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_NotifyPrimaryContentTransformChanged(
    _In_ XHANDLE hManipulationHandler,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fTranslationXChanged,
    _In_ bool fTranslationYChanged,
    _In_ bool fZoomFactorChanged)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifyPrimaryContentTransformChanged(pManipulatedElement, fInManipulation, fTranslationXChanged, fTranslationYChanged, fZoomFactorChanged));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_NotifySnapPointsChanged
//
//  Synopsis:
//    Called when the snap points for the provided motion type have changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_NotifySnapPointsChanged(
    _In_ XHANDLE hManipulationHandler,
    _In_ CUIElement* pManipulatedElement,
    _In_ XUINT8 motionType)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->NotifySnapPointsChanged(pManipulatedElement, (XDMMotionTypes) motionType));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_GetPrimaryContentTransform
//
//  Synopsis:
//    Called when the DM container needs access to the latest primary content
//    transform.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_GetPrimaryContentTransform(
    _In_ XHANDLE hManipulationHandler,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool fForBringIntoViewport,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->GetPrimaryContentTransform(pManipulatedElement, fForBringIntoViewport, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_BringIntoViewport
//
//  Synopsis:
//    Called when the DM container wants to bring the specified bounds of
//    the manipulated element into the viewport. If animate is True, a DM
//    animation is used.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_BringIntoViewport(
    _In_ XHANDLE hManipulationHandler,
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
    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);
    IFCEXPECT_RETURN(pfHandled);

    CUIDMContainerHandler* pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->BringIntoViewport(
        pManipulatedElement, bounds, translateX, translateY, zoomFactor, fTransformIsValid, fSkipDuringTouchContact, fSkipAnimationWhileRunning, fAnimate, fApplyAsManip, fIsForMakeVisible, pfHandled));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_SetConstantVelocities
//
//  Synopsis:
//    Called when the DM container wants to initiate a constant-velocity pan.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_SetConstantVelocities(
    _In_ XHANDLE hManipulationHandler,
    _In_ CUIElement* pManipulatedElement,
    _In_ XFLOAT panXVelocity,
    _In_ XFLOAT panYVelocity)
{

    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);

    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->SetConstantVelocities(pManipulatedElement, panXVelocity, panYVelocity));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ManipulationHandler_ProcessInputMessage
//
//  Synopsis:
//    Called when the DM container wants the handler to process the current
//    input message, by forwarding it to DirectManipulation.
//    The handler must set the bHandled flag to True if the message was
//    handled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ManipulationHandler_ProcessInputMessage(
    _In_ XHANDLE hManipulationHandler,
    _In_ CUIElement* pManipulatedElement,
    _In_ bool ignoreFlowDirection,
    _Out_ bool& bHandled)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    IFCEXPECT_RETURN(hManipulationHandler);
    IFCEXPECT_RETURN(pManipulatedElement);

    bHandled = false;
    pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(hManipulationHandler);
    IFC_RETURN(pUIDMContainerHandler->ProcessInputMessage(pManipulatedElement, ignoreFlowDirection, bHandled));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   StackPanel_GetIrregularSnapPoints
//
//  Synopsis:
//      Returns the irregular snap points for the provided CStackPanel,
//      orientation and alignment.
//      If no snap points are present, NULL is returned.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel_GetIrregularSnapPoints(
    _In_ CStackPanel* pStackPanel,
    _In_ bool bHorizontalOrientation,
    _In_ bool bNearAligned,
    _In_ bool bFarAligned,
    _Outptr_opt_result_buffer_(*pcSnapPoints) XFLOAT** ppSnapPoints,
    _Out_ uint32_t* pcSnapPoints)
{
    IFCEXPECT_RETURN(pStackPanel);
    IFCEXPECT_RETURN(ppSnapPoints);
    IFCEXPECT_RETURN(pcSnapPoints);

    IFC_RETURN(pStackPanel->GetIrregularSnapPoints(bHorizontalOrientation, bNearAligned, bFarAligned, ppSnapPoints, pcSnapPoints));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   StackPanel_GetRegularSnapPoints
//
//  Synopsis:
//      Returns the regular snap points' offset and interval for the
//      provided CStackPanel, orientation and alignment.
//      If no snap points are present, 0 is returned.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel_GetRegularSnapPoints(
    _In_ CStackPanel* pStackPanel,
    _In_ bool bHorizontalOrientation,
    _In_ bool bNearAligned,
    _In_ bool bFarAligned,
    _Out_ XFLOAT* pOffset,
    _Out_ XFLOAT* pInterval)
{
    IFCEXPECT_RETURN(pStackPanel);
    IFCEXPECT_RETURN(pOffset);
    IFCEXPECT_RETURN(pInterval);

    IFC_RETURN(pStackPanel->GetRegularSnapPoints(bHorizontalOrientation, bNearAligned, bFarAligned, pOffset, pInterval));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   StackPanel_SetSnapPointsChangeNotificationsRequirement
//
//  Synopsis:
//      Determines whether the StackPanel must call NotifySnapPointsChanged
//      when snap points change or not.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel_SetSnapPointsChangeNotificationsRequirement(
    _In_ CStackPanel* pStackPanel,
    _In_ bool bIsForHorizontalSnapPoints,
    _In_ bool bNotifyChanges)
{
    IFCEXPECT_RETURN(pStackPanel);

    IFC_RETURN(pStackPanel->SetSnapPointsChangeNotificationsRequirement(bIsForHorizontalSnapPoints, bNotifyChanges));

    return S_OK;
}

_Check_return_ HRESULT DragDrop_RaiseEvent(
    _In_ CContentRoot* contentRoot,
    _In_ CCoreServices* pCore,
    _In_ DirectUI::DragDropMessageType msgType,
    _In_ XPOINTF dropPoint,
    _In_ bool raiseDragEventsSync /*= false*/,
    _In_opt_ IInspectable* pWinRtDragInfo /*= nullptr*/,
    _In_opt_ IInspectable* pDragDropAsyncOperation /*= nullptr*/,
    _Inout_opt_ DirectUI::DataPackageOperation* pAcceptedOperation /*= nullptr*/,
    _In_opt_ CDependencyObject* hitTestRoot /*= nullptr*/)
{
    DragMsg dragMessage;
    bool handled = false;
    MessageMap message = XCP_NULL;
    // If the drag events are not raised by DirectUI::DragDrop, the drop point comes in as physical pixels, we need to transform
    // to logical pixels for the input message.
    const XFLOAT scale = pWinRtDragInfo ? 1.0f / RootScale::GetRasterizationScaleForContentRoot(contentRoot) : 1.0f;

    IFCPTR_RETURN(pCore);

    switch(msgType)
    {
        case DirectUI::DragDropMessageType::DragEnter:
        {
            message = XCP_DRAGENTER;
            break;
        }
        case DirectUI::DragDropMessageType::DragLeave:
        {
            message = XCP_DRAGLEAVE;
            break;
        }
        case DirectUI::DragDropMessageType::DragOver:
        {
            message = XCP_DRAGOVER;
            break;
        }
        case DirectUI::DragDropMessageType::Drop:
        {
            message = XCP_DROP;
            break;
        }
        default:
        {
            IFC_RETURN(E_UNEXPECTED);
        }
    }

    dragMessage.m_xPos = static_cast<XINT32>(dropPoint.x * scale);
    dragMessage.m_yPos = static_cast<XINT32>(dropPoint.y * scale);

    dragMessage.m_fileCount = 0;
    dragMessage.m_filePathSize = 0;
    dragMessage.m_filePaths = NULL;
    dragMessage.m_msgID = message;

    if (pWinRtDragInfo == nullptr)
    {
        // Legacy Drag and Drop

        // hitTestRoot only supported for WinRT Drag Drop path
        ASSERT(hitTestRoot == nullptr);
        IFC_RETURN(contentRoot->GetInputManager().GetDragDropProcessor().ProcessDragDrop(&dragMessage, false /*bRequireMouseMoveForDragOver*/, !!raiseDragEventsSync /*bRaiseDragEventsSync*/, &handled));
    }
    else
    {
        IFC_RETURN(contentRoot->GetInputManager().GetDragDropProcessor().ProcessWinRtDragDrop(&dragMessage, pWinRtDragInfo, pDragDropAsyncOperation, pAcceptedOperation, hitTestRoot));
    }

    return S_OK;
}

// Checks whether or not the given visual has an active associated drag visual.
// pVisual: A pointer to the visual (previously passed to DragDrop_CreateDragVisual as pVisual).
// Returns: true if the visual has an active associated drag visual.
bool DragDrop_IsVisualActive(
    _In_ CUIElement* pVisual)
{
    return pVisual->HasAbsolutelyPositionedLayoutTransitionRenderers();
}

_Check_return_ HRESULT Timeline_SetAllowDependentAnimations(_In_ bool allowDependentAnimations)
{
    CTimeline::s_allowDependentAnimations = allowDependentAnimations;

    RRETURN(S_OK);
}

_Check_return_ HRESULT Timeline_GetAllowDependentAnimations(_Out_ bool *pAllowDependentAnimations)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCEXPECT(pAllowDependentAnimations)

    *pAllowDependentAnimations = CTimeline::s_allowDependentAnimations;

Cleanup:
    RRETURN(S_OK);
}

_Check_return_ HRESULT CStoryboard_SeekPublic(
    _In_ CStoryboard* pNativeInstance,
    _In_ const CValue& seekTime)
{
    RRETURN(pNativeInstance->Seek(seekTime));
}

_Check_return_ HRESULT CStoryboard_SeekAlignedToLastTickPublic(
    _In_ CStoryboard* pNativeInstance,
    _In_ const CValue& seekTime)
{
    RRETURN(pNativeInstance->SeekAlignedToLastTickPublic(seekTime));
}

_Check_return_ HRESULT CStoryboard_SetStoryboardStartedCallback(
    _In_ std::function<HRESULT(CDependencyObject* /* storyboard */, CDependencyObject* /* target */)> callback)
{
    CStoryboard::SetStoryboardStartedCallback(callback);
    RRETURN(S_OK);
}

//
// List of properties that are disallowed on Page element when it has a SwapChainBackgroundPanel child.
//
static const KnownPropertyIndex INVALID_PAGE_PROPERTIES_IF_SCBP_CHILD[13] =
{
    KnownPropertyIndex::UIElement_Clip,
    KnownPropertyIndex::UIElement_Opacity,
    KnownPropertyIndex::UIElement_RenderTransform,
    KnownPropertyIndex::UIElement_Projection,
    KnownPropertyIndex::UIElement_RenderTransformOrigin,
    KnownPropertyIndex::FrameworkElement_Width,
    KnownPropertyIndex::FrameworkElement_Height,
    KnownPropertyIndex::FrameworkElement_MinWidth,
    KnownPropertyIndex::FrameworkElement_MaxWidth,
    KnownPropertyIndex::FrameworkElement_MinHeight,
    KnownPropertyIndex::FrameworkElement_MaxHeight,
    KnownPropertyIndex::FrameworkElement_Margin,
    KnownPropertyIndex::Control_Background
};

// Private helper method, checks if child of passed Page element is SwapChainBackgroundPanel.
void
Page_GetSwapChainBackgroundPanelChild(
    _In_ CUIElement *pUIElement,
    _Outptr_ CSwapChainBackgroundPanel **ppSCBP)
{
    CSwapChainBackgroundPanel *pSCBP = NULL;
    CUIElement **ppChildren = NULL;
    XUINT32 childCount = 0;

    pUIElement->GetChildrenInRenderOrder(&ppChildren, &childCount);

    if (childCount > 0)
    {
        CUIElement *pChild = ppChildren[0];
        pSCBP = do_pointer_cast<CSwapChainBackgroundPanel>(pChild);

        if (pSCBP != NULL)
        {
            // SCBP can be the only child of Page element.
            ASSERT(childCount == 1);
        }
    }

    *ppSCBP = pSCBP;
}

// Private helper method, checks that passed property is not set on the Page element.
_Check_return_ HRESULT Page_ValidatePropertyIsDefault(
    _In_ CCoreServices* pCore,
    _In_ CDependencyObject *pDO,
    KnownPropertyIndex propertyIndex)
{
    const CDependencyProperty *pdp = MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);

    if (!pDO->IsPropertyDefault(pdp))
    {
        //
        // We want to disallow setting of unsupported rendering property on the Page if its content is SwapChainBackgroundPanel.
        // We don?t apply any of the Page/SCBP properties to the background DX swap chain.
        // So content would look incorrect if Page rendered any of these properties and DX swap chain still looked rectangular
        // and occupied the render target size. Ideally, we want to do this validation and throw exception before SetValue.
        // Since that requires adding SetValue validation in DXaml side, it seems OK to just clear the property if it wasn?t supported.
        //
        IFC_RETURN(pDO->ClearValue(pdp));

        IFC_RETURN(CErrorService::OriginateInvalidOperationError(pCore, AG_E_PAGE_INVALID_PROPERTY_SET_HAS_SCBP_CHILD, pdp->GetName()));
    }

    return S_OK;
}

//
// When Page child is SwapChainBackgroundPanel, this helper validates that disallowed layout properties are not set on Page.
// The reason for enforcing this restriction is - some properties cannot be correctly implemented in both
// modes of SwapChainBackgroundPanel rendering, when multiple swap chain optimization is on and off. e.g. Margin, RenderTransform.
// Also some other properties like Background do not make sense when SCBP has a background swap chain set that occupies the whole render target size.
//
_Check_return_ HRESULT Page_ValidatePropertiesIfSwapChainBackgroundPanelChild(
    _In_ CCoreServices* pCore,
    _In_ CUIElement *pUIElement
    )
{
    CSwapChainBackgroundPanel *pSCBPNoRef = NULL;
    Page_GetSwapChainBackgroundPanelChild(pUIElement, &pSCBPNoRef);

    if (pSCBPNoRef != NULL)
    {
        for (XUINT32 i = 0; i < ARRAYSIZE(INVALID_PAGE_PROPERTIES_IF_SCBP_CHILD); i++)
        {
            IFC_RETURN(Page_ValidatePropertyIsDefault(
                pCore,
                pUIElement,
                INVALID_PAGE_PROPERTIES_IF_SCBP_CHILD[i]));
        }
    }

    return S_OK;
}

//
// Enforces same check as Page_ValidatePropertiesIfSwapChainBackgroundPanelChild(), except for just passed property index.
//
_Check_return_ HRESULT Page_ValidatePropertyIfSwapChainBackgroundPanelChild(
    _In_ CCoreServices* pCore,
    _In_ CUIElement *pUIElement,
    KnownPropertyIndex propertyIndex)
{
    CSwapChainBackgroundPanel *pSCBPNoRef = NULL;
    Page_GetSwapChainBackgroundPanelChild(pUIElement, &pSCBPNoRef);

    if (pSCBPNoRef != NULL)
    {
        // Check if this is one of the blocked properties.

        bool isBlockedProperty = false;

        for (XUINT32 i = 0; i < ARRAYSIZE(INVALID_PAGE_PROPERTIES_IF_SCBP_CHILD); i++)
        {
            if (propertyIndex == INVALID_PAGE_PROPERTIES_IF_SCBP_CHILD[i])
            {
                isBlockedProperty = true;
                break;
            }
        }

        if (isBlockedProperty)
        {
            IFC_RETURN(Page_ValidatePropertyIsDefault(pCore, pUIElement, propertyIndex));
        }
    }

    return S_OK;
}

//
// Ensures Page element is a root visual if its content is a SwapChainBackgroundPanel element.
//
_Check_return_ HRESULT Page_EnsureIsRootVisualIfSwapChainBackgroundPanelChild(
    _In_ CCoreServices* pCore,
    _In_ CUIElement *pUIElement
    )
{
    CSwapChainBackgroundPanel *pSCBPNoRef = NULL;
    Page_GetSwapChainBackgroundPanelChild(pUIElement, &pSCBPNoRef);

    if (pSCBPNoRef != NULL)
    {
        CUIElement *pRoot = static_cast<CUIElement*>(pCore->getVisualRoot());
        if (pRoot != pUIElement)
        {
            IFC_RETURN(CErrorService::OriginateInvalidOperationError(pCore, AG_E_PAGE_MUST_BE_ROOT_WHEN_SCBP_CHILD));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElement_CapturePointer(
    _In_ CUIElement* pNativeInstance,
    _In_ CPointer* pValue,
    _Out_ bool* pReturnValue)
{
    RRETURN(pNativeInstance->CapturePointer(pValue, pReturnValue));
}

_Check_return_ HRESULT CDataTemplate_LoadContent(
    _In_ CDataTemplate* pNativeInstance,
    _Outptr_ CDependencyObject** returnValue)
{
    RRETURN(pNativeInstance->LoadContent(returnValue));
}

_Check_return_ HRESULT LayoutTransitionElement_Destroy(
    _In_ CCoreServices* pCore,
    _In_opt_ CUIElement* pTargetUIElement,
    _In_opt_ CUIElement* pParentElement,
    _In_opt_ CUIElement* pTransitionElement)
{
    CTransitionRoot* layerNoRef = nullptr;

    IFCPTR_RETURN(pCore);

    if (pParentElement)
    {
        layerNoRef = pParentElement->GetLocalTransitionRoot(true);
    }
    else
    {
        // It's possible pTargetUIElement was in a XamlIslandRoot, but is no longer parented.  We'll check
        // both the pTargetUIElement and pTransitionElement to see if they were in an island.
        CXamlIslandRoot* xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(pTargetUIElement);
        if (!xamlIslandRoot)
        {
            xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(pTransitionElement);
        }

        if (xamlIslandRoot)
        {
            layerNoRef = xamlIslandRoot->GetTransitionRootNoRef();
        }
        else
        {
            layerNoRef = pCore->GetTransitionRootForElement(pTargetUIElement);
        }
    }
    IFCPTR_RETURN(layerNoRef);

    if (pTransitionElement)
    {
        ASSERT(pTransitionElement->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>());

        IFC_RETURN(static_cast<CLayoutTransitionElement*>(pTransitionElement)->DetachTransition(pTargetUIElement, layerNoRef));
    }

    return S_OK;
}

_Check_return_ HRESULT LayoutTransitionElement_Create(
    _In_ CCoreServices* pCore,
    _In_ CUIElement* pTargetUIElement,
    _In_opt_ CUIElement* pParentElement,
    _In_ bool isAbsolutelyPositioned,
    _Outptr_ CUIElement** ppTransitionElement)
{
    HRESULT hr = S_OK;

    CTransitionRoot* layerNoRef = nullptr;
    CLayoutTransitionElement* pDestinationElement = nullptr;
    XPOINTF origin = {0., 0.};

    IFCPTR(pCore);
    IFCPTR(pTargetUIElement);
    IFCPTR(ppTransitionElement);

    IFC(CLayoutTransitionElement::Create(
        pTargetUIElement,
        isAbsolutelyPositioned,
        &pDestinationElement));

    pDestinationElement->SetDestinationOffset(origin);

    if (pParentElement)
    {
        layerNoRef = pParentElement->GetLocalTransitionRoot(true);
    }
    else
    {
        layerNoRef = pCore->GetTransitionRootForElement(pTargetUIElement);
    }

    IFCPTR(layerNoRef);

    IFC(pDestinationElement->AttachTransition(pTargetUIElement, layerNoRef));
    *ppTransitionElement = pDestinationElement;
    pDestinationElement = nullptr;

Cleanup:
    if (FAILED(hr))
    {
        VERIFYHR(LayoutTransitionElement_Destroy(pCore, pTargetUIElement, pParentElement, pDestinationElement));
    }
    ReleaseInterface(pDestinationElement);
    RRETURN(hr);
}

_Check_return_ HRESULT LayoutTransitionElement_SetDestinationOffset(
    _In_ CUIElement* pTransitionElement,
    _In_ XFLOAT x,
    _In_ XFLOAT y)
{
    XPOINTF offset = { x, y };

    static_cast<CLayoutTransitionElement*>(pTransitionElement)->SetDestinationOffset(offset);

    RRETURN(S_OK);
}

// Should in fact never be called because the BackButton will be handled in Popup class
_Check_return_ HRESULT CPopup_OnBackButtonPressed(
    _In_ CPopup* pNativeInstance,
    _Out_ bool* returnValue)
{
    IFCEXPECT_RETURN(returnValue);
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT RootVisual_TransformToVisual(
    _In_ CCoreServices* pCore,
    _In_opt_ CUIElement* pRelativeElement,
    _Outptr_ CGeneralTransform** returnValue)
{
    CUIElement *pRootVisual = NULL;

    if (pRelativeElement && pCore->HasXamlIslandRoots())
    {
        pRootVisual = do_pointer_cast<CUIElement>(pCore->GetRootForElement(pRelativeElement));
    }

    if (!pRootVisual)
    {
        IFCEXPECT_RETURN(pCore);
        pRootVisual = pCore->GetMainRootVisual();
    }

    if (pRootVisual)
    {
        xref_ptr<CGeneralTransform> transform;
        IFC_RETURN(pRootVisual->TransformToVisual(pRelativeElement, &transform));
        *returnValue = transform.detach();
    }

    return S_OK;
}
}