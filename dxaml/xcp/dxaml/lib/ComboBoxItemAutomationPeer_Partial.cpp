// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComboBoxItemAutomationPeer_partial.h"
#include "ComboBoxItem.g.h"
#include "ComboBox.g.h"
#include "ContentPResenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ComboBoxItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IComboBoxItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IComboBoxItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IComboBoxItemAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<ComboBoxItem*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ComboBoxItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the ComboBoxItemAutomationPeer class.
ComboBoxItemAutomationPeer::ComboBoxItemAutomationPeer()
{
}

// Deconstructor
ComboBoxItemAutomationPeer::~ComboBoxItemAutomationPeer()
{
}

IFACEMETHODIMP ComboBoxItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ComboBoxItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBoxItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}

// This override makes sure to retreive UIA children from the faceplate in case drop down is close for ComboBox
// and there is a default selection.
IFACEMETHODIMP ComboBoxItemAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<IComboBoxItem> spOwnerAsCBI;
    ctl::ComPtr<Selector> spParentComboBoxAsSelector;
    ctl::ComPtr<IComboBox> spParentComboBox;
    BOOLEAN bIsDropDownOpen = FALSE;

    IFCPTR(ppReturnValue);
    *ppReturnValue = nullptr;

    IFC(get_Owner(&spOwner));
    IFC(spOwner.As(&spOwnerAsCBI));
    IFC(spOwnerAsCBI.Cast<ComboBoxItem>()->GetParentSelector(&spParentComboBoxAsSelector));

    if (spParentComboBoxAsSelector)
    {
        IFC(spParentComboBoxAsSelector.As(&spParentComboBox));
        IFC(spParentComboBox.Cast<ComboBox>()->get_IsDropDownOpen(&bIsDropDownOpen));

        if (!bIsDropDownOpen)
        {
            BOOLEAN bIsSelected = FALSE;
            IFC(spOwnerAsCBI.Cast<ComboBoxItem>()->get_IsSelected(&bIsSelected));
            if (bIsSelected)
            {
                ctl::ComPtr<IContentPresenter> spContentPresenter;
                IFC(spParentComboBox.Cast<ComboBox>()->GetContentPresenterPart(&spContentPresenter));

                // in case of a selected Item when ComboBox's Drop down is closed, faceplate contentpresenter has the real content instead of ComboBoxItem.
                if (spContentPresenter)
                {
                    spContentPresenter.As<IUIElement>(&spOwner);
                }
            }
        }
    }

    IFC(ctl::make(&spAPChildren));
    // Gets AP Children, in case of Selection and Drop Down being closed spOwner will be ContentPresenter. By doing this we make sure even when Container exist for
    // Selected Content we utilize ContentPresenter to retrieve the Content as Container has already transferred the Content to ContentPresenter.
    IFC(GetAutomationPeerChildren(spOwner.Get(), spAPChildren.Get()));

    IFC(spAPChildren.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

// In this override we are making sure when an Item is selected and if it this item we always return IsOffScreen as False.
// In case of Faceplate when container Item exist but not part of the visual tree, it may not have bounding rects and hence IsOffScreen will be True
// but as faceplate exists we must override that value with False.
IFACEMETHODIMP ComboBoxItemAutomationPeer::IsOffscreenCore(_Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<IComboBoxItem> spOwnerAsCBI;
    ctl::ComPtr<Selector> spParentComboBoxAsSelector;
    ctl::ComPtr<IComboBox> spParentComboBox;
    BOOLEAN bIsDropDownOpen = FALSE;

    IFCPTR(pReturnValue);
    *pReturnValue = TRUE;

    IFC(get_Owner(&spOwner));
    IFC(spOwner.As(&spOwnerAsCBI));
    IFC(spOwnerAsCBI.Cast<ComboBoxItem>()->GetParentSelector(&spParentComboBoxAsSelector));

    if (spParentComboBoxAsSelector)
    {
        IFC(spParentComboBoxAsSelector.As(&spParentComboBox));
        IFC(spParentComboBox.Cast<ComboBox>()->get_IsDropDownOpen(&bIsDropDownOpen));

        if (!bIsDropDownOpen)
        {
            BOOLEAN bIsSelected = FALSE;
            IFC(spOwnerAsCBI.Cast<ComboBoxItem>()->get_IsSelected(&bIsSelected));

            // When Drop down is closed and this is the selected Item, it's always Visible and hence IsOffScreen is always False
            // and if it's not Selected then it's always OffScreen hence return value is TRUE;
            *pReturnValue = !bIsSelected;
        }
    }

    // if DropDown is Open then call the base or when parent is not ComboBox and ComboBoxItem is used as standalone control. As call to base is expensive as it determines clipping by
    // walking up the visual tree and in case of DropDown being closed determination of this value is trivial.
    if (bIsDropDownOpen || !spParentComboBoxAsSelector)
    {
        IFC(ComboBoxItemAutomationPeerGenerated::IsOffscreenCore(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

// In this override when we have this item as selected and drop down is close we must make sure to retrieve Bounding rect from faceplate.
IFACEMETHODIMP ComboBoxItemAutomationPeer::GetBoundingRectangleCore(_Out_ wf::Rect* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<IComboBoxItem> spOwnerAsCBI;
    ctl::ComPtr<Selector> spParentComboBoxAsSelector;
    ctl::ComPtr<IComboBox> spParentComboBox;
    BOOLEAN bIsDropDownOpen = FALSE;

    IFCPTR(pReturnValue);
    pReturnValue->X = pReturnValue->Y = pReturnValue->Width = pReturnValue->Height = 0;

    IFC(get_Owner(&spOwner));
    IFC(spOwner.As(&spOwnerAsCBI));
    IFC(spOwnerAsCBI.Cast<ComboBoxItem>()->GetParentSelector(&spParentComboBoxAsSelector));

    if (spParentComboBoxAsSelector)
    {
        IFC(spParentComboBoxAsSelector.As(&spParentComboBox));
        IFC(spParentComboBox.Cast<ComboBox>()->get_IsDropDownOpen(&bIsDropDownOpen));

        if (!bIsDropDownOpen)
        {
            BOOLEAN bIsSelected = FALSE;
            IFC(spOwnerAsCBI.Cast<ComboBoxItem>()->get_IsSelected(&bIsSelected));

            // In case of this being Selected Item The Bounding Rect is provided by the Faceplate ContentPresenter. In case where the Item is not the SelectedItem and DropDown is closed
            // bounds are 0.
            if (bIsSelected)
            {
                ctl::ComPtr<IContentPresenter> spContentPresenter;
                ctl::ComPtr<IAutomationPeer> spContentPresenterAP;
                IFC(spParentComboBox.Cast<ComboBox>()->GetContentPresenterPart(&spContentPresenter));

                if (spContentPresenter)
                {
                    // We look for AP for faceplate ContentPresenter as that provides the real bounds for the selected Content.
                    IFC(spContentPresenter.Cast<ContentPresenter>()->GetOrCreateAutomationPeer(&spContentPresenterAP));
                    if (spContentPresenterAP)
                    {
                        IFC(spContentPresenterAP->GetBoundingRectangle(pReturnValue));
                    }
                }
            }
        }
    }

    // We want to call base only when DropDown is Open or when parent is not ComboBox and ComboBoxItem is used as standalone control. The call to base is expensive as it looks for
    // cliping by walking up the Visual tree and in case Drop down is closed the determination of Bounding Rect is trivial.
    if (bIsDropDownOpen || !spParentComboBoxAsSelector)
    {
        IFC(ComboBoxItemAutomationPeerGenerated::GetBoundingRectangleCore(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}
