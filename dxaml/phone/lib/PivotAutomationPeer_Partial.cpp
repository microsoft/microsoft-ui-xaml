// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotAutomationPeer_Partial.h"
#include "Pivot_Partial.h"
#include "PivotItemDataAutomationPeer_Partial.h"
#include "AutomationHelper.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    //---------------------------------------------------------------------
    // PivotAutomationPeer
    //---------------------------------------------------------------------

    _Check_return_ HRESULT
    PivotAutomationPeerFactory::CreateInstanceWithOwnerImpl(
        _In_ xaml_controls::IPivot* pOwner,
        _Outptr_ xaml_automation_peers::IPivotAutomationPeer **ppInstance)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<PivotAutomationPeer> spAutomationPeer;

        IFC(wrl::MakeAndInitialize<PivotAutomationPeer>(&spAutomationPeer, pOwner));
        *ppInstance = spAutomationPeer.Detach();

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::InitializeImpl(
        _In_ xaml_controls::IPivot* pOwner)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::Automation::Peers::IItemsControlAutomationPeerFactory> spInnerFactory;
        wrl::ComPtr<xaml::Automation::Peers::IItemsControlAutomationPeer> spInnerInstance;
        wrl::ComPtr<xaml_controls::IItemsControl> spPivotAsItemsControl;
        wrl::ComPtr<xaml_controls::IPivot> spOwner(pOwner);
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(PivotAutomationPeerGenerated::InitializeImpl(pOwner));

        IFCPTR(pOwner);

        IFC(wf::GetActivationFactory(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_ItemsControlAutomationPeer).Get(),
              &spInnerFactory));

        IFC(pOwner->QueryInterface(
            __uuidof(xaml_controls::IItemsControl),
            &spPivotAsItemsControl));

        IFC(spInnerFactory->CreateInstanceWithOwner(
                spPivotAsItemsControl.Get(),
                static_cast<xaml_automation_peers::IPivotAutomationPeer*>(this),
                &spInnerInspectable,
                &spInnerInstance));

        IFC(SetComposableBasePointers(
                spInnerInspectable.Get(),
                spInnerFactory.Get()));

    Cleanup:
        return hr;
    }

#pragma region IAutomationPeerOverrides methods

    _Check_return_ HRESULT
    PivotAutomationPeer::GetPatternCoreImpl(
        _In_ xaml::Automation::Peers::PatternInterface patternInterface,
        _Outptr_ IInspectable **returnValue)
    {
        HRESULT hr = S_OK;

        if (patternInterface == xaml::Automation::Peers::PatternInterface_Selection ||
            patternInterface == xaml_automation_peers::PatternInterface_Scroll)
        {
            *returnValue = wrl::ComPtr<IPivotAutomationPeer>(this).Detach();
        }
        else
        {
            IFC(PivotAutomationPeerGenerated::GetPatternCoreImpl(patternInterface, returnValue));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::GetAutomationControlTypeCoreImpl(
        _Out_ xaml::Automation::Peers::AutomationControlType *returnValue)
    {
        *returnValue = xaml::Automation::Peers::AutomationControlType_Tab;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::OnCreateItemAutomationPeerImpl(
        _In_ IInspectable* item,
        _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<IPivotItemDataAutomationPeer> spPivotItemDataAutomationPeer;

        IFC(wrl::MakeAndInitialize<PivotItemDataAutomationPeer>(&spPivotItemDataAutomationPeer, item, this));
        IFC(spPivotItemDataAutomationPeer.CopyTo(returnValue));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::GetClassNameCoreImpl(
        _Out_ HSTRING *returnValue)
    {
        HRESULT hr = S_OK;

        IFC(wrl_wrappers::HStringReference(L"Pivot").CopyTo(returnValue));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::GetNameCoreImpl(
        _Out_ HSTRING *returnValue)
    {
        HRESULT hr = S_OK;

        // If the Name property wasn't explicitly set on this AP, then we will
        // return the pivot title.
        IFC(PivotAutomationPeerGenerated::GetNameCoreImpl(returnValue));

        if(!(*returnValue))
        {
            xaml_controls::IPivot* pOwnerNoRef = nullptr;
            wrl::ComPtr<IInspectable> spTitle;

            IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
                static_cast<IPivotAutomationPeer*>(this),
                &pOwnerNoRef));

            IFCEXPECT(pOwnerNoRef);

            IFC(pOwnerNoRef->get_Title(&spTitle));

            if(spTitle)
            {
                IFC(Private::AutomationHelper::GetPlainText(spTitle.Get(), returnValue));
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::GetChildrenCoreImpl(
        _Outptr_ wfc::IVector<xaml::Automation::Peers::AutomationPeer*> **returnValue)
    {
        xaml_controls::IPivot* pOwnerNoRef = nullptr;
        IFC_RETURN(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotAutomationPeer*>(this),
            &pOwnerNoRef));
        if (!pOwnerNoRef)
        {
            IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
        }

        IFC_RETURN(PivotAutomationPeerGenerated::GetChildrenCoreImpl(returnValue));
        IFCPTR_RETURN(*returnValue);

        BOOLEAN isLocked = FALSE;
        IFC_RETURN(pOwnerNoRef->get_IsLocked(&isLocked));

        if (isLocked)
        {
            INT selectedIndex = 0;
            UINT itemsCount = 0;

            IFC_RETURN(pOwnerNoRef->get_SelectedIndex(&selectedIndex));
            IFC_RETURN((*returnValue)->get_Size(&itemsCount));

            ASSERT(static_cast<UINT>(selectedIndex) < itemsCount);

            for(INT i = static_cast<INT>(itemsCount) - 1; i >= 0; --i)
            {
                if(i != selectedIndex)
                {
                    (*returnValue)->RemoveAt(i);
                }
            }
        }

        // In addition to APs for the PivotItems (which is returned by the base implementation of GetChildrenCoreImpl via ItemsControlAutomationCore::GetChildrenCore)
        // we also want to include APs for TitleControl, LeftHeader, and RightHeader if they exist.
        wrl::ComPtr<xaml::IFrameworkElement> leftHeader;
        IFC_RETURN(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->GetLeftHeaderPresenter(&leftHeader));

        if (leftHeader)
        {
            wrl::ComPtr<xaml::IUIElement> leftHeaderAsUIE;
            IFC_RETURN(leftHeader.As(&leftHeaderAsUIE));

            IFC_RETURN(AddAutomationPeerChildrenToCollection(leftHeaderAsUIE.Get(), *returnValue, false /* addAtEnd */));
        }

        wrl::ComPtr<xaml_controls::IContentControl> titleControl;
        IFC_RETURN(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->GetTitleControl(&titleControl));

        if (titleControl)
        {
            wrl::ComPtr<xaml::IUIElement> titleControlAsUIE;
            IFC_RETURN(titleControl.As(&titleControlAsUIE));

            IFC_RETURN(AddAutomationPeerChildrenToCollection(titleControlAsUIE.Get(), *returnValue, false /* addAtEnd */));
        }

        wrl::ComPtr<xaml::IFrameworkElement> rightHeader;
        IFC_RETURN(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->GetRightHeaderPresenter(&rightHeader));

        if (rightHeader)
        {
            wrl::ComPtr<xaml::IUIElement> rightHeaderAsUIE;
            IFC_RETURN(rightHeader.As(&rightHeaderAsUIE));

            IFC_RETURN(AddAutomationPeerChildrenToCollection(rightHeaderAsUIE.Get(), *returnValue, true /* addAtEnd */));
        }

        return S_OK;
    }

    _Check_return_ HRESULT
        PivotAutomationPeer::GetElementFromPointCoreImpl(
            _In_ wf::Point pointInWindowCoordinates,
            _Outptr_ IInspectable** returnValue)
    {
        BOOLEAN isLocked = FALSE;
        wrl::ComPtr<wfc::IVector<xaml::Automation::Peers::AutomationPeer*>> spPivotItemPeers;
        wf::Rect boundingRectangle;

        *returnValue = nullptr;

        // Get the pivot control associated with this PivotAutomationPeer.
        xaml_controls::IPivot* pOwnerNoRef = nullptr;
        IFC_RETURN(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(static_cast<IPivotAutomationPeer*>(this), &pOwnerNoRef));
        IFCEXPECT_RETURN(pOwnerNoRef);

        // If the pivot is locked, then there's no interactable pivot item available. In that case, return the pivot itself.
        IFC_RETURN(pOwnerNoRef->get_IsLocked(&isLocked));
        if (!isLocked)
        {
            // The pivot is not locked, so iterate through all the pivot items belonging to the pivot,
            // to check whether one of the items contains the point.
            IFC_RETURN(PivotAutomationPeerGenerated::GetChildrenCoreImpl(&spPivotItemPeers));

            UINT itemsCount;
            IFC_RETURN(spPivotItemPeers->get_Size(&itemsCount));
            if (itemsCount > 0)
            {
                BOOLEAN containsPoint = FALSE;
                wrl::ComPtr<xaml::IRectHelperStatics> spRectHelperStatics;

                IFC_RETURN(wf::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_RectHelper).Get(),
                    &spRectHelperStatics));

                for (INT i = 0; i < static_cast<INT>(itemsCount); ++i)
                {
                    IFC_RETURN(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->GetItemHeaderBoundingRectangle(i, &boundingRectangle));

                    // Both pointInWindowCoordinates and boundingRectangle are relative to the XAML window.
                    IFC_RETURN(spRectHelperStatics->Contains(boundingRectangle, pointInWindowCoordinates, &containsPoint));
                    if (containsPoint)
                    {
                        wrl::ComPtr<IAutomationPeer> spHitPivotItemPeer;
                        IFC_RETURN(spPivotItemPeers->GetAt(i, &spHitPivotItemPeer));

                        wrl::ComPtr<IInspectable> spHitPivotItemPeerAsIInspectable;
                        IFC_RETURN(spHitPivotItemPeer.As(&spHitPivotItemPeerAsIInspectable));
                        *returnValue = spHitPivotItemPeerAsIInspectable.Detach();

                        break;
                    }
                }
            }
        }

        // If no pivot item has been hit, return the AutomationPeer for the pivot.
        if (*returnValue == nullptr)
        {
            wrl::ComPtr<IInspectable> spThisAsIInspectable;
            IFC_RETURN(QueryInterface(__uuidof(IInspectable), &spThisAsIInspectable));
            *returnValue = spThisAsIInspectable.Detach();
        }

        return S_OK;
    }

#pragma endregion


#pragma region ISelectionProvider

    _Check_return_ HRESULT
    PivotAutomationPeer::get_CanSelectMultipleImpl(
        _Out_ BOOLEAN *value)
    {
        *value = FALSE;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::get_IsSelectionRequiredImpl(
        _Out_ BOOLEAN *value)
    {
        *value = TRUE;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::GetSelectionImpl(
        _Out_ UINT32* pReturnValueSize,
        _Outptr_result_buffer_maybenull_(*pReturnValueSize) xaml::Automation::Provider::IIRawElementProviderSimple ***pppReturnValue)
    {
        HRESULT hr = S_OK;
        xaml_controls::IPivot* pOwnerNoRef = nullptr;

        *pReturnValueSize = 0;
        *pppReturnValue = nullptr;

        IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotAutomationPeer*>(this),
            &pOwnerNoRef));

        if (pOwnerNoRef)
        {
            INT32 selectedIndex = 0;

            IFC(pOwnerNoRef->get_SelectedIndex(&selectedIndex));

            if (selectedIndex >= 0)
            {
                wrl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spThisAsICAP;

                if(SUCCEEDED(QueryInterface(
                    __uuidof(xaml_automation_peers::IItemsControlAutomationPeer),
                    &spThisAsICAP)))
                {
                    wrl::ComPtr<IInspectable> spSlectedItem;
                    wrl::ComPtr<xaml::Automation::Peers::IItemAutomationPeer> spItemAutomationPeer;

                    IFC(pOwnerNoRef->get_SelectedItem(&spSlectedItem));
                    IFC(spThisAsICAP->CreateItemAutomationPeer(spSlectedItem.Get(), &spItemAutomationPeer));

                    if (spItemAutomationPeer)
                    {
                        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
                        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerProtected> spAutomationPeerAsProtected;
                        wrl::ComPtr<xaml::Automation::Provider::IIRawElementProviderSimple> spProvider;

                        IFC(spItemAutomationPeer.As(&spAutomationPeer));
                        IFC(spItemAutomationPeer.As(&spAutomationPeerAsProtected));
                        IFC(spAutomationPeerAsProtected->ProviderFromPeer(spAutomationPeer.Get(), &spProvider));

                        *pppReturnValue = static_cast<xaml::Automation::Provider::IIRawElementProviderSimple**>(CoTaskMemAlloc(sizeof(xaml::Automation::Provider::IIRawElementProviderSimple*)));
                        if (*pppReturnValue)
                        {
                            *pppReturnValue[0] = spProvider.Detach();
                            *pReturnValueSize = 1;
                        }
                    }
                }
            }
        }

    Cleanup:
        RRETURN(hr);
    }

#pragma endregion

#pragma region IScrollProvider

    _Check_return_ HRESULT
    PivotAutomationPeer::get_HorizontallyScrollableImpl(
            _Out_ BOOLEAN *pValue)
    {
        HRESULT hr = S_OK;
        xaml_controls::IPivot* pOwnerNoRef = nullptr;

        IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotAutomationPeer*>(this),
            &pOwnerNoRef));
        IFCPTR(pOwnerNoRef);

        IFC(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->AutomationGetIsScrollable(pValue));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::get_HorizontalScrollPercentImpl(
        _Out_ DOUBLE *pValue)
    {
        HRESULT hr = S_OK;
        xaml_controls::IPivot* pOwnerNoRef = nullptr;

        IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotAutomationPeer*>(this),
            &pOwnerNoRef));
        IFCPTR(pOwnerNoRef);

        IFC(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->AutomationGetScrollPercent(pValue));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::get_HorizontalViewSizeImpl(
        _Out_ DOUBLE *pValue)
    {
        HRESULT hr = S_OK;
        xaml_controls::IPivot* pOwnerNoRef = nullptr;

        IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotAutomationPeer*>(this),
            &pOwnerNoRef));
        IFCPTR(pOwnerNoRef);

        IFC(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->AutomationGetViewSize(pValue));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::get_VerticallyScrollableImpl(
        _Out_ BOOLEAN *pValue)
    {
        *pValue = FALSE;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::get_VerticalScrollPercentImpl(
        _Out_ DOUBLE *pValue)
    {
        *pValue = UIA_ScrollPatternNoScroll;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::get_VerticalViewSizeImpl(
        _Out_ DOUBLE *pValue)
    {
        *pValue = 100.0;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::ScrollImpl(
        _In_ xaml_automation::ScrollAmount horizontalAmount,
        _In_ xaml_automation::ScrollAmount verticalAmount)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(verticalAmount);

        if(horizontalAmount != xaml_automation::ScrollAmount_NoAmount)
        {
            xaml_controls::IPivot* pOwnerNoRef = nullptr;

            IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
                static_cast<IPivotAutomationPeer*>(this),
                &pOwnerNoRef));
            IFCPTR(pOwnerNoRef);

            IFC(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->AutomationScroll(horizontalAmount));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotAutomationPeer::SetScrollPercentImpl(
        _In_ DOUBLE horizontalPercent,
        _In_ DOUBLE verticalPercent)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(verticalPercent);

        if(horizontalPercent != UIA_ScrollPatternNoScroll)
        {
            xaml_controls::IPivot* pOwnerNoRef = nullptr;

            IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
                static_cast<IPivotAutomationPeer*>(this),
                &pOwnerNoRef));
            IFCPTR(pOwnerNoRef);

            IFC(static_cast<xaml_controls::Pivot*>(pOwnerNoRef)->AutomationSetScrollPercent(horizontalPercent));
        }

    Cleanup:
        RRETURN(hr);
    }

#pragma endregion

    _Check_return_ HRESULT
    PivotAutomationPeer::AddAutomationPeerChildrenToCollection(
        _In_ xaml::IUIElement *element,
        _In_ wfc::IVector<xaml::Automation::Peers::AutomationPeer*> *collection,
        _In_ bool addAtEnd)
    {
        wrl::ComPtr<xaml_automation_peers::FrameworkElementAutomationPeerPrivate> spFrameworkElementAutomationPeerPrivate;
        IFC_RETURN(GetComposableBase().As(&spFrameworkElementAutomationPeerPrivate));

        wrl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> elementChildren;
        IFC_RETURN(spFrameworkElementAutomationPeerPrivate->GetAutomationPeersForChildrenOfElement(element, &elementChildren));

        UINT itemsCount = 0;
        IFC_RETURN(elementChildren->get_Size(&itemsCount));

        for (UINT i = 0; i < itemsCount; i++)
        {
            wrl::ComPtr<xaml_automation_peers::IAutomationPeer> child;
            IFC_RETURN(elementChildren->GetAt(i, &child));

            if (addAtEnd)
            {
                IFC_RETURN(collection->Append(child.Get()));
            }
            else
            {
                IFC_RETURN(collection->InsertAt(0, child.Get()));
            }
        }

        return S_OK;
    }

} } } } } XAML_ABI_NAMESPACE_END
