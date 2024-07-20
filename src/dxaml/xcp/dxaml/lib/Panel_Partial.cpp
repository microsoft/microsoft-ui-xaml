// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Panel.g.h"
#include "ListViewBase.g.h"

using namespace DirectUI;

_Check_return_ HRESULT Panel::GetFirstFocusableElementOverride(
    _Outptr_ DependencyObject** ppFirstFocusable)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemsHost = FALSE;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;

    IFC(get_IsItemsHost(&isItemsHost));

    if (isItemsHost)
    {
        // Get the items owner for this panel - use ignoreGrouping=TRUE, we want the nearest ItemsControl.
        IFC(ItemsControl::GetItemsOwner(
            this,
            TRUE, // ignoreGrouping.
            &spItemsControl));

        if (spItemsControl != NULL &&
            ctl::is<xaml_controls::IListViewBase>(spItemsControl))
        {
            // Calling GetFocusableElement instead of Get{First|Last}FocusableElementOverride to get focusable element once
            // header/footer had a chance to be focused.  This change restores the original focus traversal order, e.g.:
            // header
            // panel -> ListViewBase::Get{First|Last}FocusableElementOverride -- returns either item or group header
            // footer
            // Calling overridden Get{First|Last}FocusableElementOverride which returns item/group header on ListViewBase skips
            // header/footer in navigation logic.
            IFC(spItemsControl.Cast<ListViewBase>()->GetFocusableElement(
                FALSE,  // isBackward
                ppFirstFocusable));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Panel::GetLastFocusableElementOverride(
    _Outptr_ DependencyObject** ppLastFocusFocusable)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemsHost = FALSE;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;

    IFC(get_IsItemsHost(&isItemsHost));

    if (isItemsHost)
    {
        // Get the items owner for this panel - use ignoreGrouping=TRUE, we want the nearest ItemsControl.
        IFC(ItemsControl::GetItemsOwner(
            this,
            TRUE, // ignoreGrouping.
            &spItemsControl));

        if (spItemsControl != NULL &&
            ctl::is<xaml_controls::IListViewBase>(spItemsControl))
        {
            // Calling GetFocusableElement instead of Get{First|Last}FocusableElementOverride to get focusable element once
            // header/footer had a chance to be focused.  This change restores the original focus traversal order, e.g.:
            // header
            // panel -> ListViewBase::Get{First|Last}FocusableElementOverride -- returns either item or group header
            // footer
            // Calling overridden Get{First|Last}FocusableElementOverride which returns item/group header on ListViewBase skips
            // header/footer in navigation logic.
            IFC(spItemsControl.Cast<ListViewBase>()->GetFocusableElement(
                TRUE,  // isBackward
                ppLastFocusFocusable));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Panel::OnTreeParentUpdated(
    _In_opt_ CDependencyObject *pNewParent,
    _In_opt_ BOOLEAN isParentAlive)
{
    HRESULT hr = S_OK;

    // Forget who our items host is.
    m_wrItemsOwner = nullptr;

    IFC(PanelGenerated::OnTreeParentUpdated(pNewParent, isParentAlive));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Panel::SetItemsOwner(
    _In_opt_ xaml_controls::IItemsControl* pItemsOwner)
{
    HRESULT hr = S_OK;

    IFC(ctl::AsWeak(pItemsOwner, &m_wrItemsOwner))

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Panel::GetItemsOwner(_Outptr_ xaml_controls::IItemsControl** ppItemsOwner)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsOwner;

    IFC(m_wrItemsOwner.As(&spItemsOwner));
    *ppItemsOwner = spItemsOwner.Detach();

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Panel::get_Children(
    _Outptr_ wfc::IVector<xaml::UIElement*>** pValue)
{
    HRESULT hr = S_OK;

    if (!m_tpChildren)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
        IFC(PanelGenerated::get_Children(&spChildren));
        SetPtrValue(m_tpChildren, spChildren);
    }

    IFC(m_tpChildren.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}
