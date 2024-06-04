// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HyperLink.g.h"
#include "HyperLinkAutomationPeer.g.h"
#include "focusmgr.h"
#include "Launcher.h"
#include "ElementSoundPlayerService_Partial.h"


using namespace DirectUI;
using namespace Focus;

//------------------------------------------------------------------------
//
//  Click handler for the Hyperlink - performs URI navigation.
//
//------------------------------------------------------------------------
HRESULT Hyperlink::OnClick()
{
    ctl::ComPtr<wf::IUriRuntimeClass> spNavigateUri;

    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    BOOLEAN bAutomationListener = FALSE;

    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked, &bAutomationListener));
    if (bAutomationListener)
    {
        IFC_RETURN(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC_RETURN(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked));
        }
    }


    IFC_RETURN(get_NavigateUri(spNavigateUri.GetAddressOf()));
    if (spNavigateUri.Get() != NULL)
    {
        // Invoke default protocol handler app using Windows.System.Launcher
        IFC_RETURN(Launcher::TryInvokeLauncher(spNavigateUri.Get()));
    }

    // Request a play invoke sound for click Hyperlink
    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Invoke, this));

    return S_OK;
}

// Create HyperlinkAutomationPeer to represent the Hyperlink.
HRESULT Hyperlink::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer **ppAutomationPeer)
{
    ctl::ComPtr<HyperlinkAutomationPeer> spAutomationPeer;
    IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::HyperlinkAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC_RETURN(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();

    return S_OK;
}

_Check_return_ HRESULT Hyperlink::AutomationHyperlinkClick()
{
    // Call Navigate on core object that handles both Click event as well as NavigateUri.
    return static_cast<CHyperlink*>(GetHandle())->Navigate();
}


_Check_return_ HRESULT Hyperlink::FocusImpl(
    _In_ xaml::FocusState value,
    _Out_ BOOLEAN* returnValue)
{
    //If the App tries to call Focus with an Unfocused state, throw:
    if (xaml::FocusState_Unfocused == value)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    FocusState valueNative = static_cast<FocusState>(value);

    CHyperlink* coreHyperlink = static_cast<CHyperlink*>(GetHandle());
    if (!coreHyperlink)
    {
        // Focus may be called on a disconnected element (when the framework
        // peer has been disassociated from its core peer).  If the core peer
        // has already been disassociated, return 'unfocusable'.
        *returnValue = FALSE;

        return S_OK;
    }

    xref_ptr<CDependencyObject> spFocusTarget = nullptr;
    CCoreServices *pCoreService = nullptr;
    pCoreService = DXamlCore::GetCurrent()->GetHandle();
    IFCPTR_RETURN(pCoreService);

    CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(GetHandle());
    IFCPTR_RETURN(pFocusManager);

    if (coreHyperlink->IsFocusable())
    {
        spFocusTarget = coreHyperlink;
    }

    const FocusMovementResult result = pFocusManager->SetFocusedElement(FocusMovement(spFocusTarget.get(), DirectUI::FocusNavigationDirection::None, valueNative));
    IFC_RETURN(result.GetHResult());
    *returnValue = result.WasMoved();

    return S_OK;
}

