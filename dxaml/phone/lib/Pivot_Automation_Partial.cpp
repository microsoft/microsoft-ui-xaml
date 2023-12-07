// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Pivot_Partial.h"
#include "PivotAutomationPeer_Partial.h"
#include "PivotItemDataAutomationPeer_Partial.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

// Work around disruptive max/min macros
#undef max
#undef min

_Check_return_ HRESULT
Pivot::AutomationGetIsScrollable(
    _Out_ BOOLEAN* pIsScrollable)
{
    HRESULT hr = S_OK;
    UINT itemsCount = 0;
    BOOLEAN isLocked = FALSE;

    IFC(GetItems(&itemsCount, NULL));
    IFC(get_IsLocked(&isLocked));

    *pIsScrollable = AutomationCalculateIsScrollable(itemsCount, isLocked);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationGetScrollPercent(
    _Out_ DOUBLE* pScrollPercent)
{
    HRESULT hr = S_OK;
    UINT itemsCount = 0;
    INT selectedIndex = 0;
    BOOLEAN isLocked = FALSE;

    IFC(GetItems(&itemsCount, NULL));
    IFC(get_SelectedIndex(&selectedIndex));
    IFC(get_IsLocked(&isLocked));

    *pScrollPercent = AutomationCalculateScrollPercent(itemsCount, isLocked, selectedIndex);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationGetViewSize(
    _Out_ DOUBLE* pViewSize)
{
    HRESULT hr = S_OK;
    UINT itemsCount = 0;
    BOOLEAN isLocked = FALSE;

    IFC(GetItems(&itemsCount, NULL));
    IFC(get_IsLocked(&isLocked));

    *pViewSize = AutomationCalculateViewSize(itemsCount, isLocked);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationScroll(
    _In_ xaml_automation::ScrollAmount scrollAmount)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollable = FALSE;

    ASSERT(scrollAmount != xaml_automation::ScrollAmount_NoAmount);

    IFC(AutomationGetIsScrollable(&isScrollable));
    if(isScrollable)
    {
        INT selectedIndex = 0;
        BOOLEAN isEnabled = FALSE;
        wrl::ComPtr<Pivot> spPivot(this);
        wrl::ComPtr<xaml_controls::IControl> spPivotAsControl;

        IFC(spPivot.As(&spPivotAsControl));
        IFC(spPivotAsControl->get_IsEnabled(&isEnabled));

        if(!isEnabled)
        {
            IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
        }

        IFC(get_SelectedIndex(&selectedIndex));

        UINT itemsCount = 0;
        IFC(GetItems(&itemsCount, NULL));

        if(scrollAmount == xaml_automation::ScrollAmount_LargeIncrement ||
           scrollAmount == xaml_automation::ScrollAmount_SmallIncrement)
        {
            selectedIndex = PositiveMod(selectedIndex + 1, itemsCount);
        }
        else
        {
            ASSERT(scrollAmount == xaml_automation::ScrollAmount_LargeDecrement ||
                      scrollAmount == xaml_automation::ScrollAmount_SmallDecrement);

            selectedIndex = PositiveMod(selectedIndex - 1, itemsCount);
        }

        IFC(put_SelectedIndex(selectedIndex));
    }
    else
    {
        IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationSetScrollPercent(
    _In_ DOUBLE scrollPercent)
{
    HRESULT hr = S_OK;

    if(scrollPercent < 0.0 || scrollPercent > 100.0)
    {
        IFC(E_INVALIDARG);
    }
    else
    {
        BOOLEAN isEnabled = FALSE;
        BOOLEAN isScrollable = FALSE;
        wrl::ComPtr<Pivot> spPivot(this);
        wrl::ComPtr<xaml_controls::IControl> spPivotAsControl;

        IFC(spPivot.As(&spPivotAsControl));
        IFC(spPivotAsControl->get_IsEnabled(&isEnabled));

        if(!isEnabled)
        {
            IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
        }

        IFC(AutomationGetIsScrollable(&isScrollable));
        if(isScrollable)
        {
            UINT itemsCount = 0;
            IFC(GetItems(&itemsCount, NULL));

            UINT newIndex = std::min(static_cast<UINT>(itemsCount * scrollPercent / 100.0), itemsCount - 1);
            IFC(put_SelectedIndex(newIndex));
        }
        else
        {
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationOnSelectionChanged(
_In_ IInspectable* pOldItem,
_In_ IInspectable* pNewItem)
{
    HRESULT hr = S_OK;
    BOOLEAN listenerExists = FALSE;

    IFC(Private::AutomationHelper::ListenerExistsHelper(
        xaml::Automation::Peers::AutomationEvents_SelectionItemPatternOnElementSelected,
        &listenerExists));

    if(listenerExists)
    {
        IFC(AutomationRaiseSelectionChangedEvent());
    }

    IFC(Private::AutomationHelper::ListenerExistsHelper(
        xaml::Automation::Peers::AutomationEvents_PropertyChanged,
        &listenerExists));

    if(listenerExists)
    {
        wrl::ComPtr<xaml_controls::IItemsControl> spItemsControl;
        wrl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsAsVector;
        BOOLEAN oldIndexFound = FALSE;
        BOOLEAN newIndexFound = FALSE;
        UINT oldIndex = 0;
        UINT newIndex = 0;
        UINT itemsCount = 0;

        IFC(QueryInterface(
            __uuidof(xaml_controls::IItemsControl),
            &spItemsControl));
        IFC(spItemsControl->get_Items(&spItems));
        IFC(spItems.As(&spItemsAsVector));
        IFC(spItemsAsVector->get_Size(&itemsCount));

        IFC(spItemsAsVector->IndexOf(pOldItem, &oldIndex, &oldIndexFound));
        IFC(spItemsAsVector->IndexOf(pNewItem, &newIndex, &newIndexFound));

        IFC(AutomationRaisePropertyChangedEvents());
    }

Cleanup:
    RRETURN(hr);
}

#pragma region Property values calculation methods
BOOLEAN
Pivot::AutomationCalculateIsScrollable(
    _In_ UINT itemsCount,
    _In_ BOOLEAN isLocked)
{
    return itemsCount > 1 && !isLocked;
}

DOUBLE
Pivot::AutomationCalculateViewSize(
    _In_ UINT itemsCount,
    _In_ BOOLEAN isLocked)
{
    if (AutomationCalculateIsScrollable(itemsCount, isLocked))
    {
        return 100.0 / itemsCount;
    }
    else
    {
        return 100.0;
    }
}

DOUBLE
Pivot::AutomationCalculateScrollPercent(
    _In_ UINT itemsCount,
    _In_ BOOLEAN isLocked,
    _In_ INT selectedIndex)
{
    if (AutomationCalculateIsScrollable(itemsCount, isLocked) && selectedIndex >= 0)
    {
        ASSERT(itemsCount > 1);
        ASSERT(selectedIndex < static_cast<INT>(itemsCount));
        return selectedIndex * 100.0 / itemsCount;
    }
    else
    {
        return UIA_ScrollPatternNoScroll;
    }
}

_Check_return_ HRESULT
Pivot::AutomationRaiseSelectionChangedEvent()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<IInspectable> spSelectedItem;

    IFC(get_SelectedItem(&spSelectedItem));

    if(spSelectedItem)
    {
        wrl::ComPtr<xaml_controls::IItemsControl> spThisAsIC;
        wrl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;

        IFC(QueryInterface(__uuidof(xaml_controls::IItemsControl), &spThisAsIC));
        IFC(spThisAsIC->get_ItemContainerGenerator(&spGenerator));

        if (spGenerator)
        {
            wrl::ComPtr<xaml::IDependencyObject> spContainer;
            wrl::ComPtr<xaml::IUIElement> spContainerAsUE;
            IFC(spGenerator->ContainerFromItem(spSelectedItem.Get(), &spContainer));

            if (spContainer)
            {
                IFC(spContainer.As(&spContainerAsUE));
                IFC(Private::AutomationHelper::RaiseEventIfListener(
                    spContainerAsUE.Get(),
                    xaml::Automation::Peers::AutomationEvents_SelectionItemPatternOnElementSelected));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationRaisePropertyChangedEvents()
{
    HRESULT hr = S_OK;

    BOOLEAN listenerExists = FALSE;
    UINT newItemsCount = 0;
    INT newSelectedIndex = 0;
    BOOLEAN newIsLocked = FALSE;

    IFC(GetItems(&newItemsCount, NULL));
    IFC(get_SelectedIndex(&newSelectedIndex));
    IFC(get_IsLocked(&newIsLocked));
    IFC(Private::AutomationHelper::ListenerExistsHelper(
        xaml::Automation::Peers::AutomationEvents_PropertyChanged,
        &listenerExists));

    if (listenerExists)
    {
        wrl::ComPtr<xaml::IUIElement> spThisAsUE;
        wrl::ComPtr<xaml::Automation::IScrollPatternIdentifiersStatics> spScrollPatternStatics;
        wrl::ComPtr<xaml::Automation::IAutomationProperty> spAutomationIsScrollableProperty;
        wrl::ComPtr<xaml::Automation::IAutomationProperty> spAutomationViewSizeProperty;
        wrl::ComPtr<xaml::Automation::IAutomationProperty> spAutomationScrollPercentProperty;


        BOOLEAN oldIsScrollable = AutomationCalculateIsScrollable(m_automationItemCount, m_automationIsLocked);
        DOUBLE oldViewSize = AutomationCalculateViewSize(m_automationItemCount, m_automationIsLocked);
        DOUBLE oldScrollPercent = AutomationCalculateScrollPercent(m_automationItemCount, m_automationIsLocked, m_automationSelectedIndex);

        BOOLEAN newIsScrollable = FALSE;
        DOUBLE newViewSize = 0.0;
        // We calculate newScrollPercent because SelectedIndex is not
        // updated yet at this point.
        DOUBLE newScrollPercent = AutomationCalculateScrollPercent(newItemsCount, newIsLocked, newSelectedIndex);

        IFC(AutomationGetIsScrollable(&newIsScrollable));
        IFC(AutomationGetViewSize(&newViewSize));

        IFC(QueryInterface(
            __uuidof(xaml::IUIElement),
            &spThisAsUE));

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_ScrollPatternIdentifiers).Get(),
            &spScrollPatternStatics));
        IFC(spScrollPatternStatics->get_HorizontallyScrollableProperty(&spAutomationIsScrollableProperty));
        IFC(spScrollPatternStatics->get_HorizontalViewSizeProperty(&spAutomationViewSizeProperty));
        IFC(spScrollPatternStatics->get_HorizontalScrollPercentProperty(&spAutomationScrollPercentProperty));

        if(oldIsScrollable != newIsScrollable)
        {
            IFC(Private::AutomationHelper::RaisePropertyChanged<BOOLEAN>(
                spThisAsUE.Get(),
                spAutomationIsScrollableProperty.Get(),
                oldIsScrollable,
                newIsScrollable));
        }

        if(oldViewSize != newViewSize)
        {
            IFC(Private::AutomationHelper::RaisePropertyChanged<DOUBLE>(
                spThisAsUE.Get(),
                spAutomationViewSizeProperty.Get(),
                oldViewSize,
                newViewSize));
        }

        if(oldScrollPercent != newScrollPercent)
        {
            IFC(Private::AutomationHelper::RaisePropertyChanged<DOUBLE>(
                spThisAsUE.Get(),
                spAutomationScrollPercentProperty.Get(),
                oldScrollPercent,
                newScrollPercent));
        }

        if(newSelectedIndex != -1)
        {
            wrl::ComPtr<xaml::IDependencyObject> spSelectedItem;
            wrl::ComPtr<xaml::IUIElement> spSelectedItemAsUE;
            wrl::ComPtr<xaml_controls::IItemsControl> spItemsControl;
            wrl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
            wrl::ComPtr<xaml::Automation::ISelectionItemPatternIdentifiersStatics> spSelectionItemPatternStatics;
            wrl::ComPtr<xaml::Automation::IAutomationProperty> spAutomationIsSelectedProperty;

            IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_SelectionItemPatternIdentifiers).Get(),
                &spSelectionItemPatternStatics));
            IFC(spSelectionItemPatternStatics->get_IsSelectedProperty(&spAutomationIsSelectedProperty));

            IFC(QueryInterface(
                __uuidof(xaml_controls::IItemsControl),
                &spItemsControl));

            IFC(spItemsControl->get_ItemContainerGenerator(&spGenerator));
            IFC(spGenerator->ContainerFromIndex(newSelectedIndex, &spSelectedItem));

            if(spSelectedItem)
            {
                IFC(spSelectedItem.As(&spSelectedItemAsUE));

                IFC(Private::AutomationHelper::RaisePropertyChanged<BOOLEAN>(
                    spSelectedItemAsUE.Get(),
                    spAutomationIsSelectedProperty.Get(),
                    FALSE /* oldValue */,
                    TRUE /* newValue */));
            }
        }
    }

    m_automationItemCount = newItemsCount;
    m_automationIsLocked = newIsLocked;
    m_automationSelectedIndex = newSelectedIndex;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::AutomationFocusSelectedItem()
{
    INT selectedIndex = 0;

    IFC_RETURN(get_SelectedIndex(&selectedIndex));

    if (selectedIndex >= 0)
    {
        AutomationFocusItem(selectedIndex);
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::AutomationFocusItem(_In_ INT index)
{
    ASSERT(index >= 0);

    // We should only notify UI automation of an item having focus
    // if it actually does.
    bool headerPanelHasKeyboardFocus = false;

    IFC_RETURN(HeaderPanelHasKeyboardFocus(&headerPanelHasKeyboardFocus))

    if (headerPanelHasKeyboardFocus)
    {
        wrl::ComPtr<xaml_controls::IItemsControl> thisAsIC;
        wrl::ComPtr<xaml_controls::IItemContainerGenerator> generator;
        wrl::ComPtr<xaml::IDependencyObject> container;

        IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IItemsControl), &thisAsIC));
        IFC_RETURN(thisAsIC->get_ItemContainerGenerator(&generator));

        if (generator)
        {
            IFC_RETURN(generator->ContainerFromIndex(index, &container));

            if (container)
            {
                wrl::ComPtr<xaml::IUIElement> containerAsUIE;

                // The container could theoretically not be a UIElement, in which case
                // we want to just ignore the error code this will return and do nothing,
                // since a non-UIElement DO can't get focus.
                IGNOREHR(container.As(&containerAsUIE));

                if (containerAsUIE)
                {
                    IFC_RETURN(Private::AutomationHelper::SetAutomationFocusIfListener(containerAsUIE.Get()));
                }
            }
        }
    }

    return S_OK;
}

#pragma endregion

} } } } XAML_ABI_NAMESPACE_END