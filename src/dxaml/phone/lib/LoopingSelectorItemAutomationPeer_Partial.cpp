// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::InitializeImpl(
    _In_ xaml_primitives::ILoopingSelectorItem* pOwner)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerFactory> spInnerFactory;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeer> spInnerInstance;
    wrl::ComPtr<xaml::IFrameworkElement> spLoopingSelectorItemAsFE;
    wrl::ComPtr<xaml_primitives::ILoopingSelectorItem> spOwner(pOwner);
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(LoopingSelectorItemAutomationPeerGenerated::InitializeImpl(pOwner));

    IFC(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
          &spInnerFactory));

    IFC((static_cast<IInspectable*>(pOwner))->QueryInterface<xaml::IFrameworkElement>(
        &spLoopingSelectorItemAsFE));

    IFC(spInnerFactory->CreateInstanceWithOwner(
            spLoopingSelectorItemAsFE.Get(),
            static_cast<xaml_automation_peers::ILoopingSelectorItemAutomationPeer*>(this),
            &spInnerInspectable,
            &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(UpdateEventSource());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemAutomationPeer::UpdateEventSource()
{
    wrl::ComPtr<xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer> spLSIDAP;

    IFC_RETURN(GetDataAutomationPeer(&spLSIDAP));

    if (spLSIDAP)
    {
        IFC_RETURN(SetEventSource(spLSIDAP.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItemAutomationPeer::UpdateItemIndex(_In_ int itemIndex)
{
    wrl::ComPtr<xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer> spLSIDAP;

    IFC_RETURN(GetDataAutomationPeer(&spLSIDAP));

    if (spLSIDAP)
    {
        IFC_RETURN(static_cast<xaml_automation_peers::LoopingSelectorItemDataAutomationPeer*>(spLSIDAP.Get())->SetItemIndex(itemIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItemAutomationPeer::SetEventSource(_In_ xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer* pLSIDAP)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spLSIDAPAsAP;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spThisAsAP;

    IFC(pLSIDAP->QueryInterface<xaml::Automation::Peers::IAutomationPeer>(&spLSIDAPAsAP));

    IFC(QueryInterface(
        __uuidof(xaml::Automation::Peers::IAutomationPeer),
        &spThisAsAP));
    IFC(spThisAsAP->put_EventsSource(spLSIDAPAsAP.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemAutomationPeer::GetDataAutomationPeer(_Outptr_result_maybenull_ xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer** ppLSIDAP)
{
    xaml_primitives::LoopingSelectorItem* pOwnerNoRef = nullptr;
    xaml_primitives::LoopingSelector* pLoopingSelectorNoRef = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerStatics> spFEAPStatics;
    wrl::ComPtr<xaml::IUIElement> spLSAsUIE;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spLSAPAsAP;
    wrl::ComPtr<xaml_automation_peers::ILoopingSelectorAutomationPeer> spLSAP;
    LoopingSelectorAutomationPeer* pLoopingSelectorAPNoRef = nullptr;
    wrl::ComPtr<IInspectable> spItem;
    wrl::ComPtr<xaml_controls::IContentControl> spLSIAsCC;

    *ppLSIDAP = nullptr;

    IFC_RETURN(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC_RETURN(pOwnerNoRef->QueryInterface(
            __uuidof(xaml_controls::IContentControl),
            &spLSIAsCC));
        IFC_RETURN(spLSIAsCC->get_Content(&spItem));

        // If we don't have an item yet, then we don't want to generate a data automation peer yet.
        // Otherwise, we'll insert an entry into our LoopingSelector's automation peer map
        // corresponding to a null item, which gets us into a bad state.
        // See LoopingSelectorAutomationPeer::GetDataAutomationPeerForItem().
        if (spItem)
        {
            IFC_RETURN(pOwnerNoRef->GetParentNoRef(&pLoopingSelectorNoRef));

            ASSERT(pLoopingSelectorNoRef);

            IFC_RETURN(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
                &spFEAPStatics));
            IFC_RETURN(pLoopingSelectorNoRef->QueryInterface(
                __uuidof(xaml::IUIElement),
                &spLSAsUIE));
            IFC_RETURN(spFEAPStatics->CreatePeerForElement(spLSAsUIE.Get(), &spLSAPAsAP));

            IFC_RETURN(spLSAPAsAP.As(&spLSAP));
            pLoopingSelectorAPNoRef = static_cast<LoopingSelectorAutomationPeer*>(spLSAP.Get());

            IFC_RETURN(pLoopingSelectorAPNoRef->GetDataAutomationPeerForItem(spItem.Get(), ppLSIDAP));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItemAutomationPeer::GetOwnerAsInternalPtrNoRef(_Outptr_result_maybenull_ xaml_primitives::LoopingSelectorItem** ppOwnerNoRef)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IUIElement> spOwnerAsUIElement;
    wrl::ComPtr<xaml_primitives::ILoopingSelectorItem> spOwner;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeer> spThisAsFEAP;

    *ppOwnerNoRef = nullptr;

    IFC(QueryInterface(
        __uuidof(xaml::Automation::Peers::IFrameworkElementAutomationPeer),
        &spThisAsFEAP));
    IFC(spThisAsFEAP->get_Owner(&spOwnerAsUIElement));

    if (spOwnerAsUIElement.Get())
    {
        IFC(spOwnerAsUIElement.As(&spOwner));

        // No ref is passed back to the caller.
        *ppOwnerNoRef = static_cast<xaml_primitives::LoopingSelectorItem*>(spOwner.Get());
    }

Cleanup:
    RRETURN(hr);
}

#pragma region IAutomationPeerOverrides

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::GetPatternCoreImpl(
    _In_ xaml::Automation::Peers::PatternInterface patternInterface,
    _Outptr_ IInspectable **returnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml::Automation::Peers::PatternInterface_ScrollItem ||
        patternInterface == xaml::Automation::Peers::PatternInterface_SelectionItem )
    {
        *returnValue = static_cast<ILoopingSelectorItemAutomationPeer*>(this);
        AddRef();
    }
    else
    {
        IFC(LoopingSelectorItemAutomationPeerGenerated::GetPatternCoreImpl(patternInterface, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::GetAutomationControlTypeCoreImpl(
    _Out_ xaml::Automation::Peers::AutomationControlType *returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = xaml::Automation::Peers::AutomationControlType_ListItem;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::GetClassNameCoreImpl(
    _Out_ HSTRING *returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(L"LoopingSelectorItem").CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::IsKeyboardFocusableCoreImpl(
    _Out_ BOOLEAN* returnValue)
{
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> loopingSelectorAP;

    *returnValue = FALSE;

    // LoopingSelectorItems aren't actually keyboard focusable,
    // but we need to give them automation focus in order to have
    // UIA clients like Narrator read their contents when they're
    // selected, so we'll act as though they're keyboard focusable
    // to enable that to be possible.  In order to do this,
    // for the keyboard focus status of a LoopingSelectorItem,
    // we'll just report the keyboard focus status of its parent LoopingSelector.
    IFC_RETURN(GetParentAutomationPeer(&loopingSelectorAP));

    if (loopingSelectorAP)
    {
        IFC_RETURN(loopingSelectorAP->IsKeyboardFocusable(returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::HasKeyboardFocusCoreImpl(
    _Out_ BOOLEAN* returnValue)
{
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> loopingSelectorAP;

    *returnValue = FALSE;

    // In order to support giving automation focus to selected LoopingSelectorItem
    // automation peers, we'll report that a LoopingSelectorItem has keyboard focus
    // if its parent LoopingSelector has keyboard focus, and if this LoopingSelectorItem
    // is selected.
    IFC_RETURN(GetParentAutomationPeer(&loopingSelectorAP));

    if (loopingSelectorAP)
    {
        BOOLEAN hasKeyboardFocus = FALSE;

        IFC_RETURN(loopingSelectorAP->HasKeyboardFocus(&hasKeyboardFocus));

        if (hasKeyboardFocus)
        {
            IFC_RETURN(get_IsSelectedImpl(returnValue));
        }
    }

    return S_OK;
}


#pragma endregion

#pragma region IScrollItemProvider

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::ScrollIntoViewImpl()
{
    xaml_primitives::LoopingSelectorItem* pOwnerNoRef = nullptr;

    IFC_RETURN(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        xaml_primitives::LoopingSelector* pOwnerParentNoRef = nullptr;

        IFC_RETURN(pOwnerNoRef->GetParentNoRef(&pOwnerParentNoRef));

        if (pOwnerParentNoRef)
        {
            wrl::ComPtr<xaml_controls::IContentControl> spOwnerAsContentControl;
            wrl::ComPtr<IInspectable> spContent;

            IFC_RETURN(pOwnerNoRef->QueryInterface(__uuidof(xaml_controls::IContentControl), &spOwnerAsContentControl));
            IFC_RETURN(spOwnerAsContentControl->get_Content(&spContent));
            IFC_RETURN(pOwnerParentNoRef->AutomationTryScrollItemIntoView(spContent.Get()));
        }
    }

    return S_OK;
}

#pragma endregion

#pragma region ISelectionItemProvider

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::get_IsSelectedImpl(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;
    xaml_primitives::LoopingSelectorItem* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));
    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationGetIsSelected(value));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::get_SelectionContainerImpl(_Outptr_ xaml_automation::Provider::IIRawElementProviderSimple **ppValue)
{
    HRESULT hr = S_OK;
    xaml_primitives::LoopingSelectorItem* pOwnerNoRef = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;

    *ppValue = nullptr;

    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationGetSelectionContainerUIAPeer(&spAutomationPeer));

        if (spAutomationPeer.Get())
        {
            wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerProtected> spAutomationPeerAsProtected;
            wrl::ComPtr<xaml::Automation::Provider::IIRawElementProviderSimple> spProvider;

            IFC(spAutomationPeer.As(&spAutomationPeerAsProtected));
            IFC(spAutomationPeerAsProtected->ProviderFromPeer(spAutomationPeer.Get(), &spProvider));

            *ppValue = spProvider.Detach();
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::AddToSelectionImpl()
{
    RRETURN(UIA_E_INVALIDOPERATION);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::RemoveFromSelectionImpl()
{
    RRETURN(UIA_E_INVALIDOPERATION);
}

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::SelectImpl()
{
    HRESULT hr = S_OK;
    xaml_primitives::LoopingSelectorItem* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));
    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationSelect());
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion

_Check_return_ HRESULT
LoopingSelectorItemAutomationPeer::GetParentAutomationPeer(
    _Outptr_ xaml_automation_peers::IAutomationPeer **parentAutomationPeer)
{
    xaml_primitives::LoopingSelectorItem* pOwnerNoRef = nullptr;

    *parentAutomationPeer = nullptr;

    IFC_RETURN(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        xaml_primitives::LoopingSelector* pLoopingSelectorNoRef = nullptr;

        IFC_RETURN(pOwnerNoRef->GetParentNoRef(&pLoopingSelectorNoRef));

        if (pLoopingSelectorNoRef)
        {
            wrl::ComPtr<xaml_primitives::LoopingSelector> loopingSelector(pLoopingSelectorNoRef);
            wrl::ComPtr<xaml::IUIElement> loopingSelectorAsUIE;

            IGNOREHR(loopingSelector.As(&loopingSelectorAsUIE));

            if (loopingSelectorAsUIE)
            {
                wrl::ComPtr<xaml_automation_peers::IAutomationPeer> loopingSelectorAP;

                IFC_RETURN(Private::AutomationHelper::CreatePeerForElement(
                    loopingSelectorAsUIE.Get(),
                    &loopingSelectorAP));

                *parentAutomationPeer = loopingSelectorAP.Detach();
            }
        }
    }

    return S_OK;
}

} } } } } XAML_ABI_NAMESPACE_END
