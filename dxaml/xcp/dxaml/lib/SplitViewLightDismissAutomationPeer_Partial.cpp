// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitViewLightDismissAutomationPeer.g.h"
#include "SplitView.g.h"
#include "SplitView.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP SplitViewLightDismissAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    bool fLightDismissEnabled = false;
    IFC_RETURN(IsLightDismissEnabled(&fLightDismissEnabled));

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke && fLightDismissEnabled)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC_RETURN(SplitViewLightDismissAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP SplitViewLightDismissAutomationPeer::GetClassNameCore(_Out_ HSTRING* pValue)
{
    IFCPTR_RETURN(pValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SplitViewLightDismiss")).CopyTo(pValue));
    return S_OK;
}

IFACEMETHODIMP SplitViewLightDismissAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml_automation_peers::AutomationControlType_Button;
    return S_OK;
}

IFACEMETHODIMP SplitViewLightDismissAutomationPeer::GetNameCore(_Out_ HSTRING* pValue)
{
    IFCPTR_RETURN(pValue);
    IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_LIGHTDISMISS_NAME, pValue));
    return S_OK;
}

IFACEMETHODIMP SplitViewLightDismissAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* pValue)
{
    IFCPTR_RETURN(pValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"LightDismiss")).CopyTo(pValue));
    return S_OK;
}

// Support the IInvokeProvider interface.
_Check_return_ HRESULT SplitViewLightDismissAutomationPeer::InvokeImpl()
{
    ctl::ComPtr<DependencyObject> spTemplatedParent;

    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;
    ctl::ComPtr<xaml::IFrameworkElement> spOwnerAsFE;
    IFC_RETURN(get_Owner(&spOwnerAsUIE));
    IFC_RETURN(spOwnerAsUIE.As(&spOwnerAsFE));
    IFC_RETURN(spOwnerAsFE.Cast<FrameworkElement>()->get_TemplatedParent(&spTemplatedParent));

    ctl::ComPtr<xaml_controls::ISplitView> spSplitView = spTemplatedParent.AsOrNull<xaml_controls::ISplitView>();
    if (spSplitView)
    {
        auto splitViewCore = static_cast<CSplitView*>(spSplitView.Cast<SplitView>()->GetHandle());
        if (splitViewCore->CanLightDismiss())
        {
            IFC_RETURN(splitViewCore->TryCloseLightDismissiblePane());
        }
    }
    return S_OK;
}

HRESULT SplitViewLightDismissAutomationPeer::IsLightDismissEnabled(bool *pIsLightDimissEnabled)
{
    *pIsLightDimissEnabled = false;
    ctl::ComPtr<DependencyObject> spTemplatedParent;

    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;
    ctl::ComPtr<xaml::IFrameworkElement> spOwnerAsFE;
    IFC_RETURN(get_Owner(&spOwnerAsUIE));
    IFC_RETURN(spOwnerAsUIE.As(&spOwnerAsFE));
    IFC_RETURN(spOwnerAsFE.Cast<FrameworkElement>()->get_TemplatedParent(&spTemplatedParent));

    ctl::ComPtr<xaml_controls::ISplitView> spSplitView = spTemplatedParent.AsOrNull<xaml_controls::ISplitView>();
    if (spSplitView)
    {
        auto splitViewCore = static_cast<CSplitView*>(spSplitView.Cast<SplitView>()->GetHandle());
        *pIsLightDimissEnabled = splitViewCore->CanLightDismiss();
    }
    return S_OK;
}