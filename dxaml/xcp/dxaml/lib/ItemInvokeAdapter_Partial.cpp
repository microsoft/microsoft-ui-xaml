// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the ItemInvokeAdapter class
//      This is an internal class which implements IInvokeProvider UIA pattern
//      and serve as Adapter to our MoCo Items as they support ItemClick.

#include "precomp.h"
#include "ItemInvokeAdapter.g.h"
#include "ListViewBaseItem.g.h"
#include "ListViewBase.g.h"
#include "ListViewBaseHeaderItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Set the owner of Adapter
_Check_return_ HRESULT
ItemInvokeAdapter::put_Owner(
    _In_ xaml_automation_peers::IFrameworkElementAutomationPeer* automationPeer)
{
    IFC_RETURN(ctl::AsWeak(automationPeer, &m_automationPeerWeakRef));

    return S_OK;
}

// InvokeImpl override, defines behavior of when Invoke is called on an Item through UIA Client
_Check_return_ HRESULT ItemInvokeAdapter::InvokeImpl()
{
    ctl::ComPtr<xaml_automation_peers::IFrameworkElementAutomationPeer> automationPeer =
        m_automationPeerWeakRef.AsOrNull<xaml_automation_peers::IFrameworkElementAutomationPeer>();

    if (automationPeer)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(automationPeer->get_Owner(&owner));

        // Is owner a ListViewBaseItem?
        ctl::ComPtr<ListViewBaseItem> listViewBaseItem = owner.AsOrNull<ListViewBaseItem>();
        if (listViewBaseItem)
        {
            ctl::ComPtr<ListViewBase> parentListViewBase;
            parentListViewBase = listViewBaseItem->GetParentListView();

            // Notify that UIAutomation has invoked action on ListViewBaseItem
            if (parentListViewBase)
            {
                IFC_RETURN(parentListViewBase->AutomationItemClick(listViewBaseItem.Get()));
            }
        }
        else
        {
            // Is owner a ListViewBaseHeaderItem?
            ctl::ComPtr<ListViewBaseHeaderItem> listViewBaseHeaderItem = owner.AsOrNull<ListViewBaseHeaderItem>();
            if (listViewBaseHeaderItem)
            {
                ctl::ComPtr<IListViewBase> parentListViewBase;
                IFC_RETURN(listViewBaseHeaderItem->GetParent(&parentListViewBase));

                // Notify that UIAutomation has invoked action on ListViewBaseHeaderItem
                if (parentListViewBase)
                {
                    IFC_RETURN(parentListViewBase.Cast<ListViewBase>()->AutomationHeaderItemClick(listViewBaseHeaderItem.Get()));
                }
            }
        }
    }

    return S_OK;
}
