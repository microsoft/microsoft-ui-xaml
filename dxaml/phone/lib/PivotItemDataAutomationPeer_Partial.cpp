// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotItemDataAutomationPeer_Partial.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

    _Check_return_ HRESULT
    PivotItemDataAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
        _In_ IInspectable* pItem,
        _In_ IPivotAutomationPeer* pParent,
        _Outptr_ IPivotItemDataAutomationPeer **ppInstance)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<PivotItemDataAutomationPeer> spAutomationPeer;

        IFC(wrl::MakeAndInitialize<PivotItemDataAutomationPeer>(&spAutomationPeer, pItem, pParent));
        *ppInstance = spAutomationPeer.Detach();

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::InitializeImpl(
        _In_ IInspectable* pItem,
        _In_ IPivotAutomationPeer* pParent)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::Automation::Peers::IItemsControlAutomationPeer> spParentAsItemsControl;
        wrl::ComPtr<xaml::Automation::Peers::IItemAutomationPeerFactory> spInnerFactory;
        wrl::ComPtr<xaml::Automation::Peers::IItemAutomationPeer> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(PivotItemDataAutomationPeerGenerated::InitializeImpl(pItem, pParent));

        IFC(pParent->QueryInterface(
            __uuidof(xaml::Automation::Peers::IItemsControlAutomationPeer),
            &spParentAsItemsControl));

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_ItemAutomationPeer).Get(),
            &spInnerFactory));

        IFC(spInnerFactory->CreateInstanceWithParentAndItem(
            pItem,
            spParentAsItemsControl.Get(),
            static_cast<IPivotItemDataAutomationPeer*>(this),
            &spInnerInspectable,
            &spInnerInstance));

        IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    Cleanup:
        RRETURN(hr);
    }

#pragma region IAutomationPeerOverrides

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::GetPatternCoreImpl(
        _In_ xaml::Automation::Peers::PatternInterface patternInterface,
        _Outptr_ IInspectable **returnValue)
    {
        HRESULT hr = S_OK;

        if (patternInterface == xaml::Automation::Peers::PatternInterface_SelectionItem ||
            patternInterface == xaml::Automation::Peers::PatternInterface_ScrollItem)
        {
            *returnValue = wrl::ComPtr<IPivotItemDataAutomationPeer>(this).Detach();
        }
        else
        {
            IFC(PivotItemDataAutomationPeerGenerated::GetPatternCoreImpl(patternInterface, returnValue));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::GetBoundingRectangleCoreImpl(
        _Out_ wf::Rect* returnValue)
    {
        HRESULT hr = S_OK;
        BOOLEAN isOffscreen = FALSE;

        IFC(IsOffscreenCore(&isOffscreen));
        if(!isOffscreen)
        {
            IFC(PivotItemDataAutomationPeerGenerated::GetBoundingRectangleCoreImpl(returnValue));
        }
        else
        {
            wf::Rect emptyRect = {};
            *returnValue = emptyRect;
        }

    Cleanup:
        RRETURN(hr);
    }

#pragma endregion

#pragma region ISelectionItemProvider

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::get_IsSelectedImpl(
        _Out_ boolean *value)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spThisAsIAP;
        wrl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spParentItemsControlAP;

        *value = FALSE;

        IFC(QueryInterface(
            __uuidof(xaml_automation_peers::IItemAutomationPeer),
            &spThisAsIAP));

        IFC(spThisAsIAP->get_ItemsControlAutomationPeer(&spParentItemsControlAP));

        if(spParentItemsControlAP)
        {
            xaml_controls::IPivot* pParentPivotNoRef;

            IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef<xaml_controls::IPivot>(
                spParentItemsControlAP.Get(),
                &pParentPivotNoRef));

            if(pParentPivotNoRef)
            {
                wrl::ComPtr<IInspectable> spItem;
                wrl::ComPtr<IInspectable> spSelectedItem;

                IFC(spThisAsIAP->get_Item(&spItem));
                IFC(pParentPivotNoRef->get_SelectedItem(&spSelectedItem));

                *value = xaml_controls::PlatformHelpers::AreSameObject(spItem.Get(), spSelectedItem.Get());
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::get_SelectionContainerImpl(
        _Outptr_ xaml::Automation::Provider::IIRawElementProviderSimple **value)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spThisAsIAP;
        wrl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spParentItemsControlAP;

        *value = nullptr;

        IFC(QueryInterface(
            __uuidof(xaml_automation_peers::IItemAutomationPeer),
            &spThisAsIAP));

        IFC(spThisAsIAP->get_ItemsControlAutomationPeer(&spParentItemsControlAP));

        if(spParentItemsControlAP)
        {
            wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spItemsControlAPAsAP;
            wrl::ComPtr<xaml_automation_peers::IAutomationPeerProtected> spThisAsAPProtected;

            IFC(spParentItemsControlAP.As(&spItemsControlAPAsAP));
            IFC(QueryInterface(
                __uuidof(xaml_automation_peers::IAutomationPeerProtected),
                &spThisAsAPProtected));

            IFC(spThisAsAPProtected->ProviderFromPeer(spItemsControlAPAsAP.Get(), value));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::AddToSelectionImpl()
    {
        // Pivot only allows single selection and it does not have a collection
        // of selected items.
        RRETURN(UIA_E_INVALIDOPERATION);
    }

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::RemoveFromSelectionImpl()
    {
        // Throw exception here as Pivot does not have a collection of
        // selected items.
        RRETURN(UIA_E_INVALIDOPERATION);
    }

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::SelectImpl()
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spThisAsIAP;
        wrl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spParentItemsControlAP;

        IFC(QueryInterface(
            __uuidof(xaml_automation_peers::IItemAutomationPeer),
            &spThisAsIAP));

        IFC(spThisAsIAP->get_ItemsControlAutomationPeer(&spParentItemsControlAP));

        if(spParentItemsControlAP)
        {
            xaml_controls::IPivot* pParentPivotNoRef;

            IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef<xaml_controls::IPivot>(
                spParentItemsControlAP.Get(),
                &pParentPivotNoRef));

            if(pParentPivotNoRef)
            {
                BOOLEAN isLocked = FALSE;
                wrl::ComPtr<IInspectable> spItem;

                IFC(spThisAsIAP->get_Item(&spItem));
                IFC(pParentPivotNoRef->get_IsLocked(&isLocked));

                if(isLocked)
                {
                    wrl::ComPtr<IInspectable> spSelectedItem;
                    IFC(pParentPivotNoRef->get_SelectedItem(&spSelectedItem));

                    if(spItem != spSelectedItem)
                    {
                        IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
                    }
                }

                IFC(pParentPivotNoRef->put_SelectedItem(spItem.Get()));
            }
        }

    Cleanup:
        RRETURN(hr);
    }

#pragma endregion

#pragma region IScrollItemProvider

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::ScrollIntoViewImpl()
    {
        RRETURN(Select());
    }

#pragma endregion

#pragma region IVirtualizedItemProvider

    _Check_return_ HRESULT
    PivotItemDataAutomationPeer::RealizeImpl()
    {
        RRETURN(Select());
    }

#pragma endregion

} } } } } XAML_ABI_NAMESPACE_END
