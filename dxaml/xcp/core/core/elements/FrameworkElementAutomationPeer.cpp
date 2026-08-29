// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
CFrameworkElementAutomationPeer::CFrameworkElementAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value) : CAutomationPeer(pCore, value)
{
    CUIElement* pUIElement = nullptr;
    CDependencyObject* pDependencyObject = value.AsObject();

    ASSERT(pDependencyObject != nullptr && pDependencyObject->OfTypeByIndex<KnownTypeIndex::FrameworkElement>());

    // This shall always work here as pDependencyObject must be an FrameworkElement or its override.
    VERIFYHR(DoPointerCast(pUIElement, pDependencyObject));
    VERIFYHR(pUIElement->SetAutomationPeer(this));
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
CFrameworkElementAutomationPeer::~CFrameworkElementAutomationPeer()
{
}

//------------------------------------------------------------------------
//
//  Method:   GetAPParent
//
//  Synopsis:
//      Returns the automation peer parent
//
//------------------------------------------------------------------------

CAutomationPeer* CFrameworkElementAutomationPeer::GetAPParent()
{
    return __super::GetLogicalAPParent();
}

//======================================================================================================
//
//  The "Core" methods below are the standard implementation for UIElements.
//  They contain the default action, which is to read the AutomationProperty, or a generic value.
//  Elements which wish to have custom returns should override the below functions
//
//======================================================================================================

//------------------------------------------------------------------------
//
//  Method:   GetRootChildrenCore
//
//  Synopsis:
//      Walks the root DO tree to return the root children APs
//
//------------------------------------------------------------------------
XINT32 CFrameworkElementAutomationPeer::GetRootChildrenCore(CAutomationPeer*** pppReturnAP)
{
    CUIElement *pRoot = nullptr;

    if (m_pDO && (pRoot = do_pointer_cast<CUIElement>(m_pDO->GetPublicRootVisual())))
    {
        XUINT32 uAPCount = (XINT32)CUIAWindow::GetAutomationPeersForRoot(pRoot, pppReturnAP);
        return (XINT32)uAPCount;
    }
    else
    {
        *pppReturnAP = nullptr;
        return 0;
    }
}

//------------------------------------------------------------------------
//
//  Method:   HasKeyboardFocusCore
//
//  Synopsis:
//      Checks whether has keyboard focus
//
//------------------------------------------------------------------------

HRESULT CFrameworkElementAutomationPeer::HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal)
{
    BOOLEAN hasKeyboardFocus = FALSE;
    BOOLEAN isEnabledForFocus = FALSE;

    IFC_RETURN(ThrowElementNotAvailableError());
    IFC_RETURN(IsEnabledForFocus(&isEnabledForFocus));

    if (isEnabledForFocus && m_pDO && m_pDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement *uielement = static_cast<CUIElement*>(m_pDO);
        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(uielement);
        if (focusManager->IsPluginFocused() && uielement->IsFocused())
        {
            hasKeyboardFocus = TRUE;
        }
    }
    *pRetVal = hasKeyboardFocus;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsEnabledCore
//
//  Synopsis:
//      Checks whether the control is enabled
//
//------------------------------------------------------------------------

HRESULT CFrameworkElementAutomationPeer::IsEnabledHelper(_Out_ BOOLEAN* pRetVal)
{
    BOOLEAN isEnabled = TRUE;
    XINT32 isControlElement = FALSE;

    IFC_RETURN(ThrowElementNotAvailableError());
    IFC_RETURN(IsControlElement(&isControlElement));

    if (isControlElement && m_pDO && m_pDO->OfTypeByIndex<KnownTypeIndex::Control>())
    {
        CControl *pControl = static_cast<CControl*>(m_pDO);
        isEnabled = static_cast<BOOLEAN>(pControl->IsEnabled());
    }

    // Return true for non control based AutomationPeers. eg. TextBlock
    *pRetVal = isEnabled;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsKeyboardFocusableCore
//
//  Synopsis:
//      Checks whether the control can be focusable
//
//------------------------------------------------------------------------

HRESULT CFrameworkElementAutomationPeer::IsKeyboardFocusableHelper(_Out_ BOOLEAN* pRetVal)
{
    BOOLEAN isKeyboardFocusable = FALSE;
    BOOLEAN isEnabledForFocus = FALSE;

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(ThrowElementNotAvailableError());
    IFC_RETURN(IsEnabledForFocus(&isEnabledForFocus));

    if (isEnabledForFocus && m_pDO && m_pDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement *uielement = static_cast<CUIElement*>(m_pDO);
        if (uielement->IsFocusable())
        {
            isKeyboardFocusable = TRUE;
        }
    }
    *pRetVal = isKeyboardFocusable;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsOffscreenCore
//
//  Synopsis:
//      Checks whether the control is out of screen
//
//------------------------------------------------------------------------

HRESULT CFrameworkElementAutomationPeer::IsOffscreenHelper(bool ignoreClippingOnScrollContentPresenters, _Out_ BOOLEAN* pRetVal)
{
    BOOLEAN bIsOffscreen = TRUE;

    XRECTF_RB bounds = { };

    IFCPTR_RETURN(pRetVal);
    IFC_RETURN(ThrowElementNotAvailableError());

    CUIElement *pElement = do_pointer_cast<CUIElement>(m_pDO);
    ASSERT(pElement != nullptr);

    // In OneCoreTransforms mode, we return full bounds for elements that are off-screen. This is needed because
    // magnifier will clip out parts of the shell, and those parts still need to report bounds to be accessed by
    // UIA. We do this by ignoring the window-sized clip on the CScrollContentPresenter inside the CRootScrollViewer.
    IFC_RETURN(pElement->GetGlobalBoundsWithOptions(
        &bounds,
        false /* ignoreClipping */,
        ignoreClippingOnScrollContentPresenters,
        false /* useTargetInformation */));

    // Note: the old code used && instead of ||, so we're going with that for safety.
    const bool hasEmptyBounds = (bounds.left == bounds.right && bounds.top == bounds.bottom);
    bIsOffscreen = hasEmptyBounds;

    if (!hasEmptyBounds)
    {
        // If the element has nonzero bounds, check its ancestor chain.
        while (pElement)
        {
            const bool isVisible = pElement->IsVisible();

            if (auto popup = do_pointer_cast<CPopup>(pElement))
            {
                // Note: Popups don't have to be active because they could be parentless.
                const bool isOpen = popup->m_fIsOpen;
                bIsOffscreen = !isVisible || !isOpen;

                // We can stop at a Popup, because Popup visibility doesn't depend on its parents.
                break;
            }
            else if (auto lte = do_pointer_cast<CLayoutTransitionElement>(pElement))
            {
                bIsOffscreen = !isVisible;

                // We can similarly stop at an LTE, because they aren't parented to the rest of the visual tree.
                break;
            }
            else
            {
                const bool isActive = pElement->IsActive();
                bIsOffscreen = !isVisible || !isActive;

                if (bIsOffscreen)
                {
                    break;
                }
            }

            CUIElement* pParent = nullptr;

            if (pElement->IsHiddenForLayoutTransition())
            {
                pParent = pElement->GetFirstLTETargetingThis();
            }
            else
            {
                pParent = do_pointer_cast<CUIElement>(pElement->GetParent());
            }

            if (!pParent)
            {
                // We hit the end of the visual chain. Check for a logical parent.
                pParent = do_pointer_cast<CUIElement>(pElement->GetLogicalParentNoRef());
            }

            pElement = pParent;
        }
    }

    *pRetVal = bIsOffscreen;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetFocusCore
//
//  Synopsis:
//      Sets focus to current Element through FocusManager
//
//------------------------------------------------------------------------
HRESULT CFrameworkElementAutomationPeer::SetFocusHelper()
{
    if(!m_pDO)
    {
        return S_OK;
    }
    CFrameworkElement* pElement = do_pointer_cast<CFrameworkElement>(m_pDO);
    if (pElement)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pElement);
        contentRoot->GetInputManager().OnSetFocusFromUIA();
        bool focusUpdated = false;
        IFC_RETURN(pElement->Focus(DirectUI::FocusState::Programmatic, false/*animateIfBringIntoView*/, &focusUpdated));
    }
    return S_OK;
}

HRESULT CFrameworkElementAutomationPeer::ShowContextMenuHelper()
{
    if (!m_pDO)
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    auto element = do_pointer_cast<CFrameworkElement>(m_pDO);
    if (element)
    {
        IFC_RETURN(GetContext()->GetInputServices()->RaiseRightTappedEvent(element, DirectUI::PointerDeviceType::Mouse));

        IFC_RETURN(VisualTree::GetContentRootForElement(element)->GetInputManager().RaiseContextRequestedEvent(
            element,
            { -1, -1 },
            false /* isTouchInput */));
    }

    return S_OK;
}

HRESULT CFrameworkElementAutomationPeer::GetCultureHelper(_Out_ int* returnValue)
{
    IFC_RETURN(ThrowElementNotAvailableError());

    // Return AutomationProperties.Culture if set. Else return FrameworkElement.Language
    if (!m_pDO->IsPropertyDefaultByIndex(KnownPropertyIndex::AutomationProperties_Culture))
    {
        CValue culture;
        IFC_RETURN(m_pDO->GetValueByIndex(KnownPropertyIndex::AutomationProperties_Culture, &culture));
        *returnValue = culture.As<valueSigned>();
    }
    else
    {
        CValue language;
        IFC_RETURN(m_pDO->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Language, &language));
        *returnValue = XStringPtrToLCID(language.AsString());
    }

    return S_OK;
}

// Helper function to check if the element is either enabled, or if AllowFocusWhenDisabled
// is set.  The element is not necessarily focusable when this returns true.
_Check_return_ HRESULT CFrameworkElementAutomationPeer::IsEnabledForFocus(_Out_ BOOLEAN* result)
{
    *result = FALSE;

    if (CFrameworkElement* frameworkElement = do_pointer_cast<CFrameworkElement>(m_pDO))
    {
        if (frameworkElement->AllowFocusWhenDisabled())
        {
            *result = TRUE;
            return S_OK;
        }
    }

    XINT32 isEnabled = FALSE;
    IFC_RETURN(IsEnabled(&isEnabled));

    *result = !!isEnabled;
    return S_OK;
}
