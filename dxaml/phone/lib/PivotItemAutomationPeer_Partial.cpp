// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotItemAutomationPeer_Partial.h"
#include "PivotItem_Partial.h"
#include "Pivot_Partial.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    //---------------------------------------------------------------------
    // PivotItemAutomationPeer
    //---------------------------------------------------------------------

    _Check_return_ HRESULT
    PivotItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
        _In_ xaml_controls::IPivotItem* pOwner,
        _Outptr_ xaml_automation_peers::IPivotItemAutomationPeer **ppInstance)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<PivotItemAutomationPeer> spAutomationPeer;

        IFC(wrl::MakeAndInitialize<PivotItemAutomationPeer>(&spAutomationPeer, pOwner));
        *ppInstance = spAutomationPeer.Detach();

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::InitializeImpl(
        _In_ xaml_controls::IPivotItem* pOwner)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerFactory> spInnerFactory;
        wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeer> spInnerInstance;
        wrl::ComPtr<xaml::IFrameworkElement> spItemAsFE;
        wrl::ComPtr<xaml_controls::IPivotItem> spOwner(pOwner);
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(PivotItemAutomationPeerGenerated::InitializeImpl(pOwner));

        IFCPTR(pOwner);

        IFC(wf::GetActivationFactory(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
              &spInnerFactory));

        IFC(pOwner->QueryInterface(
            __uuidof(xaml::IFrameworkElement),
            &spItemAsFE));

        IFC(spInnerFactory->CreateInstanceWithOwner(
                spItemAsFE.Get(),
                static_cast<xaml_automation_peers::IPivotItemAutomationPeer*>(this),
                &spInnerInspectable,
                &spInnerInstance));

        IFC(SetComposableBasePointers(
                spInnerInspectable.Get(),
                spInnerFactory.Get()));

    Cleanup:
        RRETURN(hr);
    }

#pragma region IAutomationPeerOverrides methods

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetAutomationControlTypeCoreImpl(
        _Out_ xaml::Automation::Peers::AutomationControlType *returnValue)
    {
        *returnValue = xaml::Automation::Peers::AutomationControlType_TabItem;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetClassNameCoreImpl(
        _Out_ HSTRING *returnValue)
    {
        HRESULT hr = S_OK;

        IFC(wrl_wrappers::HStringReference(L"PivotItem").CopyTo(returnValue));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetNameCoreImpl(
        _Out_ HSTRING *returnValue)
    {
        HRESULT hr = S_OK;

        IFC(PivotItemAutomationPeerGenerated::GetNameCoreImpl(returnValue));

        if(!(*returnValue))
        {
            xaml_controls::IPivotItem* pOwnerNoRef = nullptr;
            wrl::ComPtr<IInspectable> spHeader;

            IFC(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
                static_cast<IPivotItemAutomationPeer*>(this),
                &pOwnerNoRef));

            IFCEXPECT(pOwnerNoRef);

            IFC(pOwnerNoRef->get_Header(&spHeader));
            if(spHeader)
            {
                IFC(Private::AutomationHelper::GetPlainText(spHeader.Get(), returnValue));
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetBoundingRectangleCoreImpl(
        _Out_ wf::Rect* returnValue)
    {
        xaml_controls::Pivot* pivotNoRef = nullptr;
        INT ownerIndex = 0;

        *returnValue = {0, 0, 0, 0};

        IFC_RETURN(GetPivot(&pivotNoRef));

        if (pivotNoRef)
        {
            IFC_RETURN(GetOwnerIndex(pivotNoRef, &ownerIndex));

            // We'll return the bounding rectangle of the selected item header in the parent Pivot
            // since that's what UI automation tools such as Narrator are going to want
            // to be focused on when the PivotItem has focus.
            if (ownerIndex >= 0)
            {
                IFC_RETURN(pivotNoRef->GetItemHeaderBoundingRectangle(ownerIndex, returnValue));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetClickablePointCoreImpl(
        _Out_ wf::Point* returnValue)
    {
        xaml_controls::Pivot* pivotNoRef = nullptr;
        INT ownerIndex = 0;

        *returnValue = {0, 0};

        IFC_RETURN(GetPivot(&pivotNoRef));

        if (pivotNoRef)
        {
            IFC_RETURN(GetOwnerIndex(pivotNoRef, &ownerIndex));

            // We'll return the clickable point of the item header in the parent Pivot,
            // since that's what can be clicked on to interact with a PivotItem.
            if (ownerIndex >= 0)
            {
                IFC_RETURN(pivotNoRef->GetItemHeaderClickablePoint(ownerIndex, returnValue));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::IsOffscreenCoreImpl(_Out_ BOOLEAN* returnValue)
    {
        xaml_controls::Pivot* pivotNoRef = nullptr;
        INT ownerIndex = 0;

        *returnValue = TRUE;

        IFC_RETURN(GetPivot(&pivotNoRef));

        if (pivotNoRef)
        {
            IFC_RETURN(GetOwnerIndex(pivotNoRef, &ownerIndex));

            // We'll return a value indicating whether or not the item header in the
            // parent Pivot is offscreen, since that's what we're using to represent
            // the PivotItem in the visual tree.
            if (ownerIndex >= 0)
            {
                bool isItemHeaderOffscreen = true;
                IFC_RETURN(pivotNoRef->IsItemHeaderOffscreen(ownerIndex, &isItemHeaderOffscreen));
                *returnValue = !!isItemHeaderOffscreen;
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::IsKeyboardFocusableCoreImpl(
        _Out_ BOOLEAN* returnValue)
    {
        xaml_controls::Pivot* pivotNoRef = nullptr;

        *returnValue = FALSE;

        // PivotItems are considered to be keyboard focusable if the header panel in our
        // parent Pivot control is keyboard focusable, since we're reporting HasKeyboardFocus = true
        // when the header panel has keyboard focus.
        IFC_RETURN(GetPivot(&pivotNoRef));

        if (pivotNoRef)
        {
            bool headerPanelIsKeyboardFocusable = false;
            IFC_RETURN(pivotNoRef->HeaderPanelIsKeyboardFocusable(&headerPanelIsKeyboardFocusable));

            *returnValue = headerPanelIsKeyboardFocusable;
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::HasKeyboardFocusCoreImpl(
        _Out_ BOOLEAN* returnValue)
    {
        xaml_controls::IPivotItem* ownerNoRef = nullptr;

        *returnValue = FALSE;

        // Pivot items never actually get keyboard focus; however, to allow UI automation
        // to work properly with Pivot items, we will nonetheless report that we have
        // keyboard focus if the header panel in our parent Pivot control has focus
        // and if the selected item in our parent Pivot control is this item.
        // This will enable AutomationFocusChanged events to be handled properly,
        // which will allow, for example, Narrator to read our header properly
        // when the Pivot first gets focus or when the user has moved to a new Pivot item.
        IFC_RETURN(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotItemAutomationPeer*>(this),
            &ownerNoRef));

        if (ownerNoRef)
        {
            xaml_controls::Pivot* pivotNoRef = nullptr;
            IFC_RETURN(GetPivot(&pivotNoRef));

            if (pivotNoRef)
            {
                bool headerPanelHasKeyboardFocus = false;
                IFC_RETURN(pivotNoRef->HeaderPanelHasKeyboardFocus(&headerPanelHasKeyboardFocus));

                if (headerPanelHasKeyboardFocus)
                {
                    wrl::ComPtr<xaml_controls::IPivot> pivot(pivotNoRef);
                    wrl::ComPtr<xaml_controls::IItemContainerMapping> pivotAsItemContainerMapping;
                    wrl::ComPtr<xaml_controls::IPivotItem> owner(ownerNoRef);
                    wrl::ComPtr<IInspectable> selectedItem;
                    wrl::ComPtr<xaml::IDependencyObject> containerForSelectedItem;
                    wrl::ComPtr<xaml::IDependencyObject> ownerAsDO;

                    IFC_RETURN(pivotNoRef->get_SelectedItem(&selectedItem));

                    IFC_RETURN(pivot.As(&pivotAsItemContainerMapping));
                    IFC_RETURN(pivotAsItemContainerMapping->ContainerFromItem(selectedItem.Get(), &containerForSelectedItem));
                    IFC_RETURN(owner.As(&ownerAsDO));

                    *returnValue = containerForSelectedItem == ownerAsDO;
                }
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::SetFocusCoreImpl()
    {
        xaml_controls::IPivotItem* ownerNoRef = nullptr;

        // Pivot items never actually get keyboard focus; however, to allow UI automation
        // to work properly with Pivot items, we will nonetheless report that we have
        // keyboard focus if the header panel in our parent Pivot control has focus
        // and if the selected item in our parent Pivot control is this item.
        // This will enable AutomationFocusChanged events to be handled properly,
        // which will allow, for example, Narrator to read our header properly
        // when the Pivot first gets focus or when the user has moved to a new Pivot item.
        IFC_RETURN(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotItemAutomationPeer*>(this),
            &ownerNoRef));

        if (ownerNoRef)
        {
            xaml_controls::Pivot* pivotNoRef = nullptr;
            IFC_RETURN(GetPivot(&pivotNoRef));

            if (pivotNoRef)
            {
                INT ownerIndex = 0;
                IFC_RETURN(GetOwnerIndex(pivotNoRef, &ownerIndex));

                // We'll return a value indicating whether or not the item header in the
                // parent Pivot is offscreen, since that's what we're using to represent
                // the PivotItem in the visual tree.
                if (ownerIndex >= 0)
                {
                    IFC_RETURN(pivotNoRef->SetFocusToItem(ownerIndex));
                }
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetPivot(
        _Out_ xaml_controls::Pivot** pivotNoRef)
    {
        xaml_controls::IPivotItem* ownerNoRef = nullptr;

        *pivotNoRef = nullptr;

        IFC_RETURN(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotItemAutomationPeer*>(this),
            &ownerNoRef));

        if (ownerNoRef)
        {
            wrl::WeakRef parentPivotAsWeak = static_cast<xaml_controls::PivotItem*>(ownerNoRef)->GetParent();

            if (parentPivotAsWeak)
            {
                wrl::ComPtr<xaml_controls::IPivot> parentPivot;

                IFC_RETURN(parentPivotAsWeak.As(&parentPivot));

                if (parentPivot)
                {
                    *pivotNoRef = static_cast<xaml_controls::Pivot*>(parentPivot.Get());
                }
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItemAutomationPeer::GetOwnerIndex(
        _In_ xaml_controls::Pivot* pivotNoRef,
        _Out_ INT* index)
    {
        xaml_controls::IPivotItem* ownerNoRef = nullptr;

        *index = -1;

        IFC_RETURN(Private::AutomationHelper::GetOwnerAsInternalPtrNoRef(
            static_cast<IPivotItemAutomationPeer*>(this),
            &ownerNoRef));

        if (ownerNoRef)
        {
            wrl::ComPtr<xaml_controls::IPivot> pivot(pivotNoRef);
            wrl::ComPtr<xaml_controls::IItemContainerMapping> pivotAsItemContainerMapping;
            wrl::ComPtr<xaml_controls::IPivotItem> owner(ownerNoRef);
            wrl::ComPtr<xaml::IDependencyObject> ownerAsDO;
            INT ownerIndex = 0;

            IFC_RETURN(pivot.As(&pivotAsItemContainerMapping));
            IFC_RETURN(owner.As(&ownerAsDO));
            IFC_RETURN(pivotAsItemContainerMapping->IndexFromContainer(ownerAsDO.Get(), &ownerIndex));

            *index = ownerIndex;
        }

        return S_OK;
    }

#pragma endregion

} } } } } XAML_ABI_NAMESPACE_END
