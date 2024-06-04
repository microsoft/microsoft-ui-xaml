// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComboBoxAutomationPeer.g.h"
#include "ComboBox.g.h"
#include "ComboBoxItemDataAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ComboBoxAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IComboBox* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IComboBoxAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IComboBoxAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<ComboBox*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ComboBoxAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ComboBoxAutomationPeer class.
ComboBoxAutomationPeer::ComboBoxAutomationPeer()
{
}

// Deconstructor
ComboBoxAutomationPeer::~ComboBoxAutomationPeer()
{
}

IFACEMETHODIMP ComboBoxAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));
    IFCPTR_RETURN(spOwner);

    *returnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Value)
    {
        BOOLEAN bIsEditable = false;
        IFC_RETURN(spOwner.Cast<ComboBox>()->get_IsEditable(&bIsEditable));

        if(bIsEditable)
        {
            *returnValue = ctl::as_iinspectable(this);
            ctl::addref_interface(this);
        }
    }
    else if (patternInterface == xaml_automation_peers::PatternInterface_ExpandCollapse)
    {
        *returnValue = ctl::as_iinspectable(this);
         ctl::addref_interface(this);
    }
    else if (patternInterface == xaml_automation_peers::PatternInterface_Window)
    {
        BOOLEAN isDropDownOpen = false;
        IFC_RETURN(spOwner.Cast<ComboBox>()->get_IsDropDownOpen(&isDropDownOpen));

        if (isDropDownOpen)
        {
            *returnValue = ctl::as_iinspectable(this);
            ctl::addref_interface(this);
        }
    }
    else
    {
        IFC_RETURN(ComboBoxAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP ComboBoxAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    BOOLEAN isOpened = FALSE;
    ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> apChildren;
    ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> peer;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> peerAsAP;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<xaml_controls::IComboBox> spComboBox;
    ctl::ComPtr<xaml::IUIElement> spLightDismissElement;
    ctl::ComPtr<xaml::IUIElement> spEditableTextElement;
    UINT nCount = 0;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = NULL;

    IFC_RETURN(get_Owner(&spOwner));
    IFCPTR_RETURN(spOwner.Get());

    IFC_RETURN(spOwner.As(&spComboBox));
    IFC_RETURN(spComboBox.Cast<ComboBox>()->get_IsDropDownOpen(&isOpened));
    if (isOpened)
    {
        IFC_RETURN(ItemsControlAutomationPeer::GetChildrenCore(ppReturnValue));
    }

    if (*ppReturnValue)
    {
        apChildren.Attach(*ppReturnValue);
        IFC_RETURN(apChildren->get_Size(&nCount))
    }

    if(!apChildren)
    {
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> apChildrenCollection;
        IFC_RETURN(ctl::make(&apChildrenCollection));

        apChildren = apChildrenCollection;
    }
    IFCPTR_RETURN(apChildren.Get());

    if (isOpened)
    {
        // Add automation peer for light dismiss layer
        IFC_RETURN(spComboBox.Cast<ComboBox>()->GetLightDismissElement(&spLightDismissElement));
        if (spLightDismissElement)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> ap;
            IFC_RETURN(spLightDismissElement.Cast<UIElement>()->GetOrCreateAutomationPeer(&ap));
            IFC_RETURN(apChildren->Append(ap.Get()));
        }
    }

    BOOLEAN isEditable = false;
    IFC_RETURN(spComboBox.Cast<ComboBox>()->get_IsEditable(&isEditable));

    if (isEditable)
    {
        IFC_RETURN(spComboBox.Cast<ComboBox>()->GetEditableTextPart(&spEditableTextElement));

        if (spEditableTextElement)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> ap;
            IFC_RETURN(spEditableTextElement.Cast<UIElement>()->GetOrCreateAutomationPeer(&ap));
            IFC_RETURN(apChildren->InsertAt(0, ap.Get()));
        }
    }

    *ppReturnValue = apChildren.Detach();
    return S_OK;
}

IFACEMETHODIMP ComboBoxAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ComboBox")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBoxAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(ComboBoxAutomationPeerGenerated::GetNameCore(returnValue));

    // If the Name property wasn't explicitly set on this AP, then we will
    // return the combo header.
    if(!*returnValue)
    {
        ctl::ComPtr<xaml::IUIElement> spOwner;
        ctl::ComPtr<xaml_controls::IComboBox> spComboBox;
        ctl::ComPtr<IInspectable> spHeader;

        IFC(get_Owner(&spOwner));
        IFCPTR(spOwner.Get());

        IFC(spOwner.As(&spComboBox));

        IFC(spComboBox.Cast<ComboBox>()->get_Header(&spHeader));

        IFC(IValueBoxer::UnboxValue(spHeader.Get(), returnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBoxAutomationPeer::GetAutomationControlTypeCore(
    _Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_ComboBox;
    RRETURN(S_OK);
}

IFACEMETHODIMP ComboBoxAutomationPeer::GetHelpTextCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(ComboBoxAutomationPeerGenerated::GetHelpTextCore(returnValue));

    // If the HelpText property wasn't explicitly set on this AP and no item is selected,
    // then we'll return the placeholder text.
    if (!*returnValue)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        ctl::ComPtr<xaml_controls::IComboBox> comboBox;
        ctl::ComPtr<IInspectable> placeholderText;

        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(owner.As(&comboBox));

        INT32 selectedIndex = -1;
        IFC_RETURN(comboBox.Cast<ComboBox>()->get_SelectedIndex(&selectedIndex));

        if (selectedIndex < 0)
        {
            IFC_RETURN(comboBox.Cast<ComboBox>()->get_PlaceholderText(returnValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBoxAutomationPeer::get_IsReadOnlyImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;
    BOOLEAN bIsEditable;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC(static_cast<ComboBox*>(pOwner)->get_IsEnabled(&bIsEnabled));
    IFC(static_cast<ComboBox*>(pOwner)->get_IsEditable(&bIsEditable));

    *pValue = !bIsEnabled || !bIsEditable;

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxAutomationPeer::get_ValueImpl(_Out_ HSTRING* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    IInspectable* pSelectionBoxItem = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ComboBox*>(pOwner))->get_SelectionBoxItem(&pSelectionBoxItem));
    IFC(FrameworkElement::GetStringFromObject(pSelectionBoxItem, pValue));

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pSelectionBoxItem);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxAutomationPeer::SetValueImpl(_In_ HSTRING value)
{
    HRESULT hr = S_OK;

    IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
    IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));

Cleanup:
    RRETURN(hr);
}

// IExpandCollapseProvider
_Check_return_ HRESULT ComboBoxAutomationPeer::ExpandImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ComboBox*>(pOwner))->put_IsDropDownOpen(TRUE));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxAutomationPeer::CollapseImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ComboBox*>(pOwner))->put_IsDropDownOpen(FALSE));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxAutomationPeer::get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* returnValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsDropDownOpen;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ComboBox*>(pOwner))->get_IsDropDownOpen(&bIsDropDownOpen));

    *returnValue = bIsDropDownOpen ? xaml_automation::ExpandCollapseState_Expanded : xaml_automation::ExpandCollapseState_Collapsed;

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxAutomationPeer::OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IComboBoxItemDataAutomationPeer* pComboBoxItemDataAutomationPeer = NULL;
    xaml_automation_peers::IComboBoxItemDataAutomationPeerFactory* pComboBoxItemDataAPFactory = NULL;
    IActivationFactory* pActivationFactory = NULL;
    IInspectable* inner = NULL;

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::ComboBoxItemDataAutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pComboBoxItemDataAPFactory, pActivationFactory));

    IFC(static_cast<ComboBoxItemDataAutomationPeerFactory*>(pComboBoxItemDataAPFactory)->CreateInstanceWithParentAndItem(item,
        this,
        NULL,
        &inner,
        &pComboBoxItemDataAutomationPeer));
    IFC(ctl::do_query_interface(*returnValue, pComboBoxItemDataAutomationPeer));

Cleanup:
    ReleaseInterface(pComboBoxItemDataAutomationPeer);
    ReleaseInterface(pComboBoxItemDataAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}

// Raise events for ExpandCollapseState changes to UIAutomation Clients.
_Check_return_ HRESULT ComboBoxAutomationPeer::RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isDropDownOpen)
{
    HRESULT hr = S_OK;

    xaml_automation::ExpandCollapseState oldValue;
    xaml_automation::ExpandCollapseState newValue;
    CValue valueOld;
    CValue valueNew;

    // Converting IsDropDown Open to appropriate enumerations
    if(isDropDownOpen)
    {
        oldValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Collapsed;
        newValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Expanded;
    }
    else
    {
        oldValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Expanded;
        newValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Collapsed;
    }

    IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
    IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));
    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APExpandCollapseStateProperty, valueOld, valueNew));

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT ComboBoxAutomationPeer::get_IsSelectionRequiredImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = TRUE;
    RRETURN(S_OK);
}

// IWindowProvider
_Check_return_ HRESULT
ComboBoxAutomationPeer::get_IsModalImpl(
_Out_ BOOLEAN* pValue)
{
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::get_IsTopmostImpl(
_Out_ BOOLEAN* pValue)
{
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::get_MaximizableImpl(
_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::get_MinimizableImpl(
_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::get_InteractionStateImpl(
_Out_ xaml_automation::WindowInteractionState* pValue)
{
    *pValue = xaml_automation::WindowInteractionState_Running;
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::get_VisualStateImpl(
_Out_ xaml_automation::WindowVisualState* pValue)
{
    *pValue = xaml_automation::WindowVisualState_Normal;
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::CloseImpl()
{
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::SetVisualStateImpl(
_In_ xaml_automation::WindowVisualState /* state */)
{
    return S_OK;
}

_Check_return_ HRESULT
ComboBoxAutomationPeer::WaitForInputIdleImpl(
_In_ INT /* milliseconds */,
_Out_ BOOLEAN* /* pValue */)
{
    return S_OK;
}

