// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
class CDependencyObject;
class CUIElement;
class CAutomationPeer;


struct IUIAInvokeProvider;
struct IUIAToggleProvider;
struct IUIASelectionItemProvider;
struct IUIAExpandCollapseProvider;

namespace KeyboardAutomationInvoker {
    // This function is used as the default way to "invoke" a piece of UI through AccessKeys or Keyboard Accelerators.
    // It is not meant to be exhaustive, simply the most valuable defaults for those features.  The order of attempts
    // here is publicly documented.
    template<class CDObject = CDependencyObject,
             class _CAutomationPeer = CAutomationPeer,
             class UIAInvokeProvider = IUIAInvokeProvider,
             class UIAToggleProvider = IUIAToggleProvider,
             class UIASelectionItemProvider = IUIASelectionItemProvider,
             class UIAExpandCollapseProvider = IUIAExpandCollapseProvider>
    bool InvokeAutomationAction(_In_ CDObject* const pDO)
    {
        // This implementation is adapted from the VuiButtonBehavior.cpp from the xbox shell for voice UI automation
        // Implementation differs from the XBOX in that we do not move focus

        // GetAutomationPeer does not automatically create a peer if it does not exist.
        _CAutomationPeer* ownerAutomationPeer = pDO->GetAutomationPeer();
        if (ownerAutomationPeer == nullptr)
        {
            // let's try creating an automation peer
            ownerAutomationPeer = pDO->OnCreateAutomationPeer();
            // Want to print a message when there is no automation peer for the owner and the item is not a scope (and can be navigated into).
            if (ownerAutomationPeer == nullptr && !AccessKeys::IsAccessKeyScope(pDO))
            {
                OutputDebugString(L"An automation peer for this component could not be found or created, meaning we are unable to attempt an invoke on this element. Consider implementing desired behavior in the event handler for AccessKeyInvoked or KeyboardAccelerator.Invoked.\n");
                return false;
            }
        }
        // For the next methods, the logic is to first get the IUIAProvider for a specific automation pattern.
        // Then, if the IUIAProvider is not null, get the automation pattern interface for the provider. 
        // The automation pattern interface iteself exposes the available automation methods.
        auto uIAInvokeProvider = ownerAutomationPeer->GetPattern(UIAXcp::APPatternInterface::PIInvoke);
        if (uIAInvokeProvider != nullptr) //For e.g. Button
        {
            auto invokeProvider = static_cast<UIAInvokeProvider*>(uIAInvokeProvider->GetPatternInterface());
            if (invokeProvider != nullptr)
            {
                return SUCCEEDED(invokeProvider->Invoke());
            }
        }

        auto uIAToggleProvider = ownerAutomationPeer->GetPattern(UIAXcp::APPatternInterface::PIToggle);
        if (uIAToggleProvider != nullptr)  //E.g. CheckBox, AppBar
        {
            auto toggleProvider = static_cast<UIAToggleProvider*>(uIAToggleProvider->GetPatternInterface());
            if (toggleProvider != nullptr)
            {
                return SUCCEEDED(toggleProvider->Toggle());
            }
        }

        auto uIASelectionItemProvider = ownerAutomationPeer->GetPattern(UIAXcp::APPatternInterface::PISelectionItem);
        if (uIASelectionItemProvider != nullptr) //For e.g. RadioButton
        {
            auto selectionItemProvider = static_cast<UIASelectionItemProvider*>(uIASelectionItemProvider->GetPatternInterface());
            if (selectionItemProvider != nullptr)
            {
                return SUCCEEDED(selectionItemProvider->Select());
            }
        }

        auto uIAExpandCollapseProvider = ownerAutomationPeer->GetPattern(UIAXcp::APPatternInterface::PIExpandCollapse);
        if (uIAExpandCollapseProvider != nullptr) //For e.g. Combobox
        {
            auto expandCollapseProvider = static_cast<UIAExpandCollapseProvider*>(uIAExpandCollapseProvider->GetPatternInterface());
            if (expandCollapseProvider != nullptr)
            {
                UIAXcp::ExpandCollapseState state = UIAXcp::ExpandCollapseState::ExpandCollapseState_Collapsed;
                if (SUCCEEDED(expandCollapseProvider->get_ExpandCollapseState(&state)))
                {

                    if (state == UIAXcp::ExpandCollapseState::ExpandCollapseState_Expanded)
                    {
                        return SUCCEEDED(expandCollapseProvider->Collapse());
                    }
                    else
                    {
                        return SUCCEEDED(expandCollapseProvider->Expand());
                    }
                }
            }
        }

        // At this point we diverge from the XBox implementation. XBOX shell includes support for list view multi-selection 
        // as well as combo box dismiss when a combobox item was selected
        // For now this will wait for PM guidance as far as how this looks from an access keys standpoint.
        return false;
    }
}