// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewGroupHeaderAutomationPeer.h"
#include "TableViewGroupHeader.h"
#include "TableViewGroupInfo.h"
#include "TableView.h"

#include "ResourceAccessor.h"

// Pulls in the generated activation factory (CppWinRTActivatableClassWithBasicFactory), the
// same way TableViewRowAutomationPeer does.
#include "TableViewGroupHeaderAutomationPeer.properties.cpp"

TableViewGroupHeaderAutomationPeer::TableViewGroupHeaderAutomationPeer(winrt::TableViewGroupHeader const& owner)
    : ReferenceTracker(owner)
{
}

winrt::TableViewGroupHeader TableViewGroupHeaderAutomationPeer::GetHeader() const
{
    return Owner().try_as<winrt::TableViewGroupHeader>();
}

winrt::IInspectable TableViewGroupHeaderAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // Unconditional, unlike the old adaptive row peer: this container is only ever a group
    // header, so the pattern never has to be withdrawn. Non-expandable groups are reported as
    // LeafNode by ExpandCollapseState rather than by hiding the pattern, which is what the
    // in-box Expander / NavigationViewItem peers do.
    if (patternInterface == winrt::PatternInterface::ExpandCollapse ||
        patternInterface == winrt::PatternInterface::GridItem)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::AutomationControlType TableViewGroupHeaderAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Group;
}

winrt::hstring TableViewGroupHeaderAutomationPeer::GetClassNameCore()
{
    return winrt::hstring{ L"TableViewGroupHeader" };
}

winrt::hstring TableViewGroupHeaderAutomationPeer::GetNameCore()
{
    // Read from the live projection rather than the visual tree, so the name reflects current
    // state instead of whatever was last rendered.
    //
    // ItemCountText already carries its own localized framing (SR_TableViewGroupHeaderCountFormat,
    // "({0})"), so the two parts are joined with the same single space the default template puts
    // between them. Anything richer belongs in a dedicated localized format string rather than
    // being assembled here.
    if (auto const header = GetHeader())
    {
        if (auto const info = header.Content().try_as<winrt::TableViewGroupInfo>())
        {
            auto const keyText = info.KeyText();
            auto const countText = info.ItemCountText();
            if (!keyText.empty() && !countText.empty())
            {
                return winrt::hstring{ std::wstring{ keyText } + L" " + std::wstring{ countText } };
            }
            if (!keyText.empty())
            {
                return keyText;
            }
        }
    }

    return __super::GetNameCore();
}

void TableViewGroupHeaderAutomationPeer::RaiseExpandCollapseAutomationEvent(
    winrt::ExpandCollapseState oldState,
    winrt::ExpandCollapseState newState)
{
    if (oldState == newState)
    {
        return;
    }

    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        return;
    }

    try
    {
        RaisePropertyChangedEvent(
            winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty(),
            box_value(oldState),
            box_value(newState));
    }
    catch (...)
    {
        // best-effort: the tree may be tearing down.
    }
}

void TableViewGroupHeaderAutomationPeer::Expand()
{
    SetExpansion(true);
}

void TableViewGroupHeaderAutomationPeer::Collapse()
{
    SetExpansion(false);
}

void TableViewGroupHeaderAutomationPeer::SetExpansion(bool expand)
{
    auto const header = GetHeader();
    if (!header || !header.IsExpandable())
    {
        return;
    }

    // Direction is passed through rather than resolved here into a toggle. The mutation is
    // applied on a later turn, so a guard reading IsExpanded() (the last state pushed to this
    // container) cannot make a toggle directional -- two Expand() calls in one client turn
    // would both pass such a guard and queue two toggles, leaving the group collapsed.
    // ExpandCollapsePattern requires Expand/Collapse to be idempotent.
    //
    // Resolved through the header's stored owner, not an ancestor walk: AT clients routinely
    // hold a provider across a scroll, and a walk from an unparented container finds nothing
    // and silently no-ops.
    if (auto const owner = winrt::get_self<TableViewGroupHeader>(header)->GetOwningTableView())
    {
        // CARVE-OUT: TableView::SetGroupExpansion lives in TableView_Grouping.cpp, which this
        // review branch does not carry. Restore this line when the grouping stack lands.
        // winrt::get_self<TableView>(owner)->SetGroupExpansion(header, expand);
        owner;
    }
}

winrt::ExpandCollapseState TableViewGroupHeaderAutomationPeer::ExpandCollapseState()
{
    if (auto const header = GetHeader())
    {
        if (!header.IsExpandable())
        {
            return winrt::ExpandCollapseState::LeafNode;
        }
        return header.IsExpanded()
            ? winrt::ExpandCollapseState::Expanded
            : winrt::ExpandCollapseState::Collapsed;
    }
    return winrt::ExpandCollapseState::LeafNode;
}

winrt::TableView TableViewGroupHeaderAutomationPeer::GetOwningTableView() const
{
    if (auto const header = GetHeader())
    {
        return winrt::get_self<TableViewGroupHeader>(header)->GetOwningTableView();
    }
    return nullptr;
}

// The band occupies one grid row and spans every visible column -- the merged-cell shape that
// TableViewAutomationPeer::GetItem reports for a header row.
int32_t TableViewGroupHeaderAutomationPeer::Row()
{
    if (auto const header = GetHeader())
    {
        if (auto const owner = GetOwningTableView())
        {
            // CARVE-OUT: TableView::GetRowsRepeaterForPeer lives on the feature branch. Restore
            // this block when the grouping stack lands; until then the row index is unknown.
            // if (auto const repeater = winrt::get_self<TableView>(owner)->GetRowsRepeaterForPeer())
            // {
            //     return repeater.GetElementIndex(header);
            // }
            owner;
        }
    }
    return -1;
}

int32_t TableViewGroupHeaderAutomationPeer::Column()
{
    return 0;
}

int32_t TableViewGroupHeaderAutomationPeer::RowSpan()
{
    return 1;
}

int32_t TableViewGroupHeaderAutomationPeer::ColumnSpan()
{
    if (auto const owner = GetOwningTableView())
    {
        if (auto const columns = owner.Columns())
        {
            int32_t count = 0;
            for (auto const& column : columns)
            {
                if (column && column.Visibility() == winrt::Visibility::Visible)
                {
                    ++count;
                }
            }
            return count > 0 ? count : 1;
        }
    }
    return 1;
}

winrt::IRawElementProviderSimple TableViewGroupHeaderAutomationPeer::ContainingGrid()
{
    if (auto const owner = GetOwningTableView())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(owner))
        {
            return ProviderFromPeer(peer);
        }
    }
    return nullptr;
}
