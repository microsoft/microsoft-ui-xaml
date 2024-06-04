// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::InitializeImpl(
    _In_ xaml_primitives::ILoopingSelector* pOwner)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerFactory> spInnerFactory;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeer> spInnerInstance;
    wrl::ComPtr<xaml::IFrameworkElement> spLoopingSelectorAsFE;
    wrl::ComPtr<xaml_primitives::ILoopingSelector> spOwner(pOwner);
    wrl::ComPtr<IInspectable> spInnerInspectable;

    ARG_NOTNULL(pOwner, "pOwner");

    IFC(LoopingSelectorAutomationPeerGenerated::InitializeImpl(pOwner));

    IFC(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
          &spInnerFactory));

    IFC((static_cast<IInspectable*>(pOwner))->QueryInterface<xaml::IFrameworkElement>(
        &spLoopingSelectorAsFE));

    IFC(spInnerFactory->CreateInstanceWithOwner(
            spLoopingSelectorAsFE.Get(),
            static_cast<xaml_automation_peers::ILoopingSelectorAutomationPeer*>(this),
            &spInnerInspectable,
            &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::GetOwnerAsInternalPtrNoRef(_Outptr_result_maybenull_ xaml_primitives::LoopingSelector** ppOwnerNoRef)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IUIElement> spOwnerAsUIElement;
    wrl::ComPtr<xaml_primitives::ILoopingSelector> spOwner;
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
        *ppOwnerNoRef = static_cast<xaml_primitives::LoopingSelector*>(spOwner.Get());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::ClearPeerMap()
{
    for(PeerMap::iterator iter = _peerMap.begin(); iter != _peerMap.end(); iter++)
    {
        iter->second->Release();
        iter->second = nullptr;
    }

    _peerMap.clear();

    RRETURN(S_OK);
}

HRESULT LoopingSelectorAutomationPeer::RealizeItemAtIndex(INT index)
{
    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    IFC_RETURN(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC_RETURN(pOwnerNoRef->AutomationRealizeItemForAP(index));
    }

    RRETURN(S_OK);
}

#pragma region IAutomationPeerOverrides

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::GetPatternCoreImpl(
    _In_ xaml::Automation::Peers::PatternInterface patternInterface,
    _Outptr_ IInspectable **returnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml::Automation::Peers::PatternInterface_Scroll ||
        patternInterface == xaml::Automation::Peers::PatternInterface_Selection ||
        patternInterface == xaml::Automation::Peers::PatternInterface_ItemContainer)
    {
        *returnValue = static_cast<ILoopingSelectorAutomationPeer*>(this);
        AddRef();
    }
    else
    {
        IFC(LoopingSelectorAutomationPeerGenerated::GetPatternCoreImpl(patternInterface, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::GetAutomationControlTypeCoreImpl(
    _Out_ xaml::Automation::Peers::AutomationControlType *returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = xaml::Automation::Peers::AutomationControlType_List;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::GetChildrenCoreImpl(
    _Outptr_ wfc::IVector<xaml::Automation::Peers::AutomationPeer*> **returnValue)
{
    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    wrl::ComPtr<wfci_::Vector<xaml::Automation::Peers::AutomationPeer*>> spReturnValue;

    IFC_RETURN(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));
    IFC_RETURN(wfci_::Vector<xaml::Automation::Peers::AutomationPeer*>::Make(&spReturnValue));

    if (pOwnerNoRef)
    {
        wrl::ComPtr<wfc::IVector<IInspectable*>> spItems;
        UINT count = 0;

        IFC_RETURN(pOwnerNoRef->get_Items(&spItems));
        IFC_RETURN(spItems->get_Size(&count));

        for (UINT itemIdx = 0; itemIdx < count; itemIdx++)
        {
            wrl::ComPtr<IInspectable> spItem;
            wrl::ComPtr<ILoopingSelectorItemDataAutomationPeer> spLSIDAP;
            wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spLSIDAPAsAP;

            IFC_RETURN(spItems->GetAt(itemIdx, &spItem));
            IFC_RETURN(GetDataAutomationPeerForItem(spItem.Get(), &spLSIDAP));
            if (spLSIDAP)
            {
                IFC_RETURN((static_cast<LoopingSelectorItemDataAutomationPeer*>(spLSIDAP.Get()))->SetItemIndex(itemIdx));

                // Update\set the EventsSource here for corresponding container peer for the Item, this ensures
                // its always the data peer that the lower layer is working with. This is especially required
                // anytime bottom up approach comes into play like hit-testing (after finding the right UI target)
                // that code moves bottom up to find relevant AutomationPeer, another case is UIA events.
                wrl::ComPtr<ILoopingSelectorItemAutomationPeer> spContainerPeer;
                IFC_RETURN(GetContainerAutomationPeerForItem(spItem.Get(), &spContainerPeer));
                if (spContainerPeer && spLSIDAP)
                {
                    IFC_RETURN((static_cast<LoopingSelectorItemAutomationPeer*>(spContainerPeer.Get()))->SetEventSource(spLSIDAP.Get()));
                }
            }

            IFC_RETURN(spLSIDAP.As(&spLSIDAPAsAP));
            IFC_RETURN(spReturnValue->Append(spLSIDAPAsAP.Get()));
        }
    }

    IFC_RETURN(spReturnValue.CopyTo(returnValue));

    RRETURN(S_OK);
}

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::GetClassNameCoreImpl(
    _Out_ HSTRING *returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(L"LoopingSelector").CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region ISelectionProvider
_Check_return_ HRESULT
LoopingSelectorAutomationPeer::get_CanSelectMultipleImpl(
    _Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    *value = FALSE;

    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::get_IsSelectionRequiredImpl(
    _Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    *value = TRUE;

    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::GetSelectionImpl(
    _Out_ UINT32* pReturnValueSize,
    _Outptr_result_buffer_maybenull_(*pReturnValueSize) xaml::Automation::Provider::IIRawElementProviderSimple ***pppReturnValue)
{
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;

    *pReturnValueSize = 0;
    *pppReturnValue = nullptr;

    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationTryGetSelectionUIAPeer(&spAutomationPeer));

        if (spAutomationPeer.Get())
        {
            wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerProtected> spAutomationPeerAsProtected;
            wrl::ComPtr<xaml::Automation::Provider::IIRawElementProviderSimple> spProvider;

            IFC(spAutomationPeer.As(&spAutomationPeerAsProtected));
            IFC(spAutomationPeerAsProtected->ProviderFromPeer(spAutomationPeer.Get(), &spProvider));

            *pppReturnValue = static_cast<xaml::Automation::Provider::IIRawElementProviderSimple**>(CoTaskMemAlloc(sizeof(xaml::Automation::Provider::IIRawElementProviderSimple*)));
            if (*pppReturnValue)
            {
                (**pppReturnValue) = spProvider.Detach();
                *pReturnValueSize = 1;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region IItemsContainerProvider

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::FindItemByPropertyImpl(
    _In_opt_ xaml::Automation::Provider::IIRawElementProviderSimple* startAfter,
    _In_opt_ xaml_automation::IAutomationProperty* automationProperty,
    _In_opt_ IInspectable* value,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** returnValue)
{
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsCollection;

    *returnValue = nullptr;

    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->get_Items(&spItemsCollection));
    }

    if (spItemsCollection)
    {
        INT startIdx = 0;
        UINT totalItems = 0;
        wrl_wrappers::HString nameProperty;
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerProtected> spThisAsProtected;

        // For the Name and IsSelected property search cases we cache these
        // values outside of the loop. Otherwise these variables remain unused.
        BOOLEAN isSelected = FALSE;
        wrl_wrappers::HString strNameToFind;
        wrl::ComPtr<IInspectable> spSelectedItem;

        Private::AutomationHelper::AutomationPropertyEnum propertyAsEnum =
            Private::AutomationHelper::AutomationPropertyEnum::EmptyProperty;

        IFC(spItemsCollection->get_Size(&totalItems));
        IFC(FindStartIndex(
            startAfter,
            spItemsCollection.Get(),
            &startIdx));
        IFC(Private::AutomationHelper::ConvertPropertyToEnum(
            automationProperty,
            &propertyAsEnum));

        IFC(QueryInterface(
            __uuidof(xaml::Automation::Peers::IAutomationPeerProtected),
            &spThisAsProtected));

        if (propertyAsEnum == Private::AutomationHelper::AutomationPropertyEnum::NameProperty && value)
        {
            IFC(Private::ValueBoxer::UnboxString(value, strNameToFind.ReleaseAndGetAddressOf()));
        }
        else if (propertyAsEnum == Private::AutomationHelper::AutomationPropertyEnum::IsSelectedProperty && value)
        {
            wrl::ComPtr<wf::IPropertyValue> spValueAsPropertyValue;
            IFC(value->QueryInterface<wf::IPropertyValue>(&spValueAsPropertyValue));
            IFC(spValueAsPropertyValue->GetBoolean(&isSelected));
            IFC(pOwnerNoRef->get_SelectedItem(&spSelectedItem));
        }

        for (INT itemIdx = startIdx + 1; itemIdx < static_cast<INT>(totalItems); itemIdx++)
        {
            BOOLEAN breakOnPeer = FALSE;

            wrl::ComPtr<ILoopingSelectorItemDataAutomationPeer> spItemDataAP;
            {
                wrl::ComPtr<IInspectable> spItem;
                IFC(spItemsCollection->GetAt(itemIdx, &spItem));
                IFC(GetDataAutomationPeerForItem(spItem.Get(), &spItemDataAP));
            }

            switch(propertyAsEnum)
            {
            case Private::AutomationHelper::AutomationPropertyEnum::EmptyProperty:
                breakOnPeer = TRUE;
                break;
            case Private::AutomationHelper::AutomationPropertyEnum::NameProperty:
                {
                    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
                    IFC(spItemDataAP.As(&spAutomationPeer));
                    wrl_wrappers::HString strNameToCompare;
                    IFC(spAutomationPeer->GetName(strNameToCompare.ReleaseAndGetAddressOf()));
                    if (strNameToCompare == strNameToFind)
                    {
                        breakOnPeer = TRUE;
                    }
                }
                break;
            case Private::AutomationHelper::AutomationPropertyEnum::IsSelectedProperty:
                {
                    wrl::ComPtr<IInspectable> spItem;
                    IFC((static_cast<LoopingSelectorItemDataAutomationPeer*>(spItemDataAP.Get()))->GetItem(&spItem));

                    if (isSelected && spSelectedItem.Get() == spItem.Get() ||
                        !isSelected && spSelectedItem.Get() != spItem.Get())
                    {
                        breakOnPeer = true;
                    }
                }
                break;
            default:
                break;
            }

            if (breakOnPeer)
            {
                wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spItemPeerAsAP;

                IFC(spItemDataAP.As(&spItemPeerAsAP));
                IFC(spThisAsProtected->ProviderFromPeer(spItemPeerAsAP.Get(), returnValue));
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region IScrollProvider

_Check_return_ HRESULT LoopingSelectorAutomationPeer::get_HorizontallyScrollableImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    *pValue = FALSE;

    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::get_VerticallyScrollableImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    *pValue = FALSE;

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationGetIsScrollable(pValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::get_HorizontalScrollPercentImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = UIA_ScrollPatternNoScroll;

    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::get_VerticalScrollPercentImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    *pValue = 0.0;

    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationGetScrollPercent(pValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::get_VerticalViewSizeImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    *pValue = 0.0;

    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationGetScrollViewSize(pValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::get_HorizontalViewSizeImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = 100.0;

    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::ScrollImpl(_In_ xaml_automation::ScrollAmount horizontalAmount, _In_ xaml_automation::ScrollAmount verticalAmount)
{
    UNREFERENCED_PARAMETER(horizontalAmount);
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationScroll(verticalAmount));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::SetScrollPercentImpl(_In_ DOUBLE horizontalPercent, _In_ DOUBLE verticalPercent)
{
    UNREFERENCED_PARAMETER(horizontalPercent);
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC_NOTRACE(pOwnerNoRef->AutomationSetScrollPercent(verticalPercent));
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region DataItem Support

_Check_return_ HRESULT LoopingSelectorAutomationPeer::GetDataAutomationPeerForItem(
    _In_ IInspectable* pItem,
    _Outptr_ xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer** ppPeer)
{
    HRESULT hr = S_OK;

    PeerMap::iterator peerIter;

    peerIter = _peerMap.find(pItem);

    if (peerIter == _peerMap.end())
    {
        wrl::ComPtr<ILoopingSelectorItemDataAutomationPeer> spDataPeer;
        IFC(wrl::MakeAndInitialize<LoopingSelectorItemDataAutomationPeer>(
            &spDataPeer,
            pItem,
            static_cast<ILoopingSelectorAutomationPeer*>(this)));

        // PeerMap keeps a ref pointer to this automation peer.
        // The peers lifetime is owned by this map and is released
        // when LoopingSelectorAP dies or when the items collection
        // changes.
        _peerMap[pItem] = spDataPeer.Get();
        spDataPeer.Get()->AddRef();
        IFC(spDataPeer.CopyTo(ppPeer));
    }
    else
    {
        *ppPeer = peerIter->second;
        (*ppPeer)->AddRef();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::GetContainerAutomationPeerForItem(
    _In_ IInspectable* pItem,
    _Outptr_result_maybenull_ xaml_automation_peers::ILoopingSelectorItemAutomationPeer** ppPeer)
{
    HRESULT hr = S_OK;

    *ppPeer = nullptr;
    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationGetContainerUIAPeerForItem(pItem, ppPeer));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorAutomationPeer::FindStartIndex(
    _In_opt_ xaml::Automation::Provider::IIRawElementProviderSimple* pStartAfter,
    _In_ wfc::IVector<IInspectable*>* pItems,
    _Out_ INT* pIndex)
{
    HRESULT hr = S_OK;


    LoopingSelectorItemDataAutomationPeer* pDataPeerNoRef = nullptr;

    *pIndex = -1;

    if (pStartAfter)
    {
        wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spProviderAsPeer;
        wrl::ComPtr<xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer> spDataPeer;
        wrl::ComPtr<xaml_automation_peers::IAutomationPeerProtected> spThisAsAPProtected;

        IFC(QueryInterface(
            __uuidof(xaml::Automation::Peers::IAutomationPeerProtected),
            &spThisAsAPProtected));

        IFC(spThisAsAPProtected->PeerFromProvider(pStartAfter, &spProviderAsPeer));
        IFC(spProviderAsPeer.As(&spDataPeer));

        pDataPeerNoRef = static_cast<LoopingSelectorItemDataAutomationPeer*>(spDataPeer.Get());
    }

    if (pDataPeerNoRef)
    {
        wrl::ComPtr<IInspectable> spItem = nullptr;
        BOOLEAN found = FALSE;
        UINT index = 0;

        IFC(pDataPeerNoRef->GetItem(&spItem));

        IFC(pItems->IndexOf(spItem.Get(), &index, &found));

        if (found)
        {
            *pIndex = static_cast<INT>(index);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorAutomationPeer::TryScrollItemIntoView(_In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;

    xaml_primitives::LoopingSelector* pOwnerNoRef = nullptr;
    IFC(GetOwnerAsInternalPtrNoRef(&pOwnerNoRef));

    if (pOwnerNoRef)
    {
        IFC(pOwnerNoRef->AutomationTryScrollItemIntoView(pItem));
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion

LoopingSelectorAutomationPeer::~LoopingSelectorAutomationPeer()
{
    VERIFYHR(ClearPeerMap());
}

} } } } } XAML_ABI_NAMESPACE_END

