// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitViewPaneAutomationPeer.g.h"
#include "SplitView.g.h"
#include "SplitView.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the SplitViewPaneAutomationPeer class.
SplitViewPaneAutomationPeer::SplitViewPaneAutomationPeer()
{
}

// Deconstructor
SplitViewPaneAutomationPeer::~SplitViewPaneAutomationPeer()
{
}

IFACEMETHODIMP SplitViewPaneAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    bool fWindowContextEnabled = false;
    IFC_RETURN(IsWindowContextEnabled(&fWindowContextEnabled));

    if (patternInterface == xaml_automation_peers::PatternInterface_Window && fWindowContextEnabled)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
         ctl::addref_interface(this);
    } 
    else
    {
        IFC_RETURN(SplitViewPaneAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP SplitViewPaneAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SplitViewPane")).CopyTo(pReturnValue));
    return S_OK;
}

IFACEMETHODIMP SplitViewPaneAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Window;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitViewPaneAutomationPeer::get_IsModalImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitViewPaneAutomationPeer::get_IsTopmostImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitViewPaneAutomationPeer::get_MaximizableImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitViewPaneAutomationPeer::get_MinimizableImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitViewPaneAutomationPeer::get_InteractionStateImpl(_Out_ xaml_automation::WindowInteractionState* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml_automation::WindowInteractionState_Running;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitViewPaneAutomationPeer::get_VisualStateImpl(_Out_ xaml_automation::WindowVisualState* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml_automation::WindowVisualState_Normal;
    return S_OK;
}

_Check_return_ HRESULT SplitViewPaneAutomationPeer::CloseImpl()
{
    return S_OK;
}

_Check_return_ HRESULT SplitViewPaneAutomationPeer::SetVisualStateImpl(_In_ xaml_automation::WindowVisualState state)
{
    return S_OK;
}

_Check_return_ HRESULT SplitViewPaneAutomationPeer::WaitForInputIdleImpl(_In_ INT milliseconds, _Out_ BOOLEAN* returnValue)
{
    return S_OK;
}


HRESULT SplitViewPaneAutomationPeer::IsWindowContextEnabled(bool *pIsWindowContextEnabled)
{
    *pIsWindowContextEnabled = false;
    ctl::ComPtr<DependencyObject> spTemplatedParent;

    ctl::ComPtr<xaml::IUIElement> spOwnerasUIE;
    ctl::ComPtr<xaml::IFrameworkElement> spOwnerAsFE;
    IFC_RETURN(get_Owner(&spOwnerasUIE));
    IFC_RETURN(spOwnerasUIE.As(&spOwnerAsFE));
    IFC_RETURN(spOwnerAsFE.Cast<FrameworkElement>()->get_TemplatedParent(&spTemplatedParent));

    ctl::ComPtr<xaml_controls::ISplitView> spSplitView = spTemplatedParent.AsOrNull<xaml_controls::ISplitView>();
    if (spSplitView)
    {
        auto splitViewCore = static_cast<CSplitView*>(spSplitView.Cast<SplitView>()->GetHandle());
        *pIsWindowContextEnabled = splitViewCore->CanLightDismiss();
    }
    return S_OK;
}