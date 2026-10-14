// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewColumn.h"
#include "TableViewCellsPanel.h"
#include "SharedHelpers.h"

bool TableView::IsContextMenuColumn(winrt::TableViewColumn const& column) const
{
    if (!column || column.Visibility() != winrt::Visibility::Visible)
    {
        return false;
    }

    auto const owner = winrt::get_self<TableViewColumn>(column)->GetOwningTableView();
    if (!owner || winrt::get_self<TableView>(owner) != this)
    {
        return false;
    }

    uint32_t index{};
    auto const columns = owner.Columns();
    return columns && columns.IndexOf(column, index);
}

bool TableView::IsContextMenuTargetCurrent(
    TableViewDetails::ContextMenuTarget const& target) const
{
    using TableViewDetails::ContextMenuTargetKind;

    if (!target.Anchor || !target.ScopeRoot || !target.Anchor.IsLoaded())
    {
        return false;
    }

    auto node = target.ScopeRoot.as<winrt::DependencyObject>();
    winrt::TableView owner{ nullptr };
    while (node && !owner)
    {
        owner = node.try_as<winrt::TableView>();
        if (!owner)
        {
            node = winrt::VisualTreeHelper::GetParent(node);
        }
    }

    if (!owner || winrt::get_self<TableView>(owner) != this)
    {
        return false;
    }

    if (target.Column && !IsContextMenuColumn(target.Column))
    {
        return false;
    }

    if (target.Kind == ContextMenuTargetKind::Body)
    {
        if (!target.Row || winrt::get_self<TableViewRow>(target.Row)->GetOwningTableView() != owner)
        {
            return false;
        }

        return winrt::get_self<TableViewRow>(target.Row)->IsContextMenuTargetCurrent(target);
    }

    auto const showsColumnHeaders =
        (static_cast<uint32_t>(owner.HeadersVisibility()) & static_cast<uint32_t>(winrt::TableViewHeadersVisibility::Column)) != 0;
    if (target.Row || target.Item || !target.Column ||
        target.ScopeRoot != m_headerHost.get() ||
        !showsColumnHeaders)
    {
        return false;
    }

    return TableViewDetails::IsLiveCellAnchor(target.Anchor, target.ScopeRoot, target.Column);
}

bool TableViewDetails::IsLiveCellAnchor(
    winrt::FrameworkElement const& anchor,
    winrt::Panel const& scopeRoot,
    winrt::TableViewColumn const& column)
{
    return anchor && scopeRoot && column &&
        anchor.Visibility() == winrt::Visibility::Visible &&
        winrt::VisualTreeHelper::GetParent(anchor) == scopeRoot &&
        TableViewCellsPanel::CellForColumn(scopeRoot, column) == anchor;
}

bool TableViewDetails::IsOwnRequestSource(
    winrt::DependencyObject const& source,
    winrt::DependencyObject const& scope)
{
    for (auto node = source; node; node = winrt::VisualTreeHelper::GetParent(node))
    {
        if (node == scope)
        {
            return true;
        }

        if (node.try_as<winrt::TableView>() || node.try_as<winrt::TableViewRow>())
        {
            return false;
        }
    }

    return false;
}

std::optional<TableViewDetails::ContextMenuTarget> TableView::ResolveHeaderContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args)
{
    auto const host = m_headerHost.get();
    auto const source = args.OriginalSource().try_as<winrt::DependencyObject>();
    if (!host || !TableViewDetails::IsOwnRequestSource(source, host))
    {
        return std::nullopt;
    }

    auto node = source;
    while (node && node != host)
    {
        auto const parent = winrt::VisualTreeHelper::GetParent(node);
        if (parent == host)
        {
            auto const cell = node.try_as<winrt::Grid>();
            if (!cell)
            {
                return std::nullopt;
            }

            auto const column = cell.Tag().try_as<winrt::TableViewColumn>();
            if (!column)
            {
                return std::nullopt;
            }

            TableViewDetails::ContextMenuTarget target;
            target.Kind = TableViewDetails::ContextMenuTargetKind::Header;
            target.Column = column;
            target.Anchor = cell;
            target.ScopeRoot = host;
            if (!IsContextMenuTargetCurrent(target))
            {
                return std::nullopt;
            }

            return target;
        }

        node = parent;
    }

    return std::nullopt;
}

void TableView::OnHeaderContextRequested(winrt::ContextRequestedEventArgs const& args)
{
    if (args.Handled())
    {
        return;
    }

    auto target = ResolveHeaderContextMenuTarget(args);
    if (!target)
    {
        return;
    }

    if (ProcessContextMenuRequest(*target, args))
    {
        args.Handled(true);
    }
}

namespace
{
    bool IsTextControl(winrt::DependencyObject const& element)
    {
        return element.try_as<winrt::TextBlock>() || element.try_as<winrt::RichTextBlock>() ||
            element.try_as<winrt::RichTextBlockOverflow>() || element.try_as<winrt::TextBox>() ||
            element.try_as<winrt::RichEditBox>() || element.try_as<winrt::PasswordBox>();
    }

    bool HasAppContextFlyout(winrt::UIElement const& element)
    {
        if (!element.ContextFlyout())
        {
            return false;
        }

        // Text controls carry a built-in copy/select flyout as their default value; only a flyout the
        // app set on the element itself counts. A Style-set flyout on a text control is therefore
        // skipped here (documented on TableViewColumn.CellContextFlyout).
        return !IsTextControl(element) ||
            element.ReadLocalValue(winrt::UIElement::ContextFlyoutProperty()) != winrt::DependencyProperty::UnsetValue();
    }

    // The row's own XAML menu: the first app ContextFlyout from just above the cell (or the row
    // itself, for a row target) up to and including the row. Covers a row style setter and row
    // template elements, anchored on the element that owns it, as the framework would show it.
    TableViewDetails::ContextMenuCandidate FindRowContextFlyout(TableViewDetails::ContextMenuTarget const& target)
    {
        if (!target.Row)
        {
            return {};
        }

        winrt::DependencyObject const row = target.Row;
        auto node = target.Column && target.Anchor
            ? winrt::VisualTreeHelper::GetParent(target.Anchor)
            : row;
        while (node)
        {
            if (auto const element = node.try_as<winrt::FrameworkElement>(); element && HasAppContextFlyout(element))
            {
                return { element.ContextFlyout(), element };
            }

            if (node == row)
            {
                break;
            }

            node = winrt::VisualTreeHelper::GetParent(node);
        }

        return {};
    }

    // First app-supplied ContextFlyout inside a cell or header wrapper (CellTemplate, HeaderTemplate,
    // generated element). Text controls' built-in flyouts are ignored.
    TableViewDetails::ContextMenuCandidate FindContentContextFlyout(winrt::FrameworkElement const& wrapper)
    {
        if (!wrapper)
        {
            return {};
        }

        // Depth-first in visual order, so the outermost app element (the template root) wins over
        // anything nested inside it. Item containers of a nested items host (ListView, ItemsRepeater,
        // and ItemsView through its repeater) belong to that host, as a nested TableView's cells do:
        // the host's own ContextFlyout counts, its items' do not.
        std::vector<winrt::DependencyObject> stack;
        auto const pushChildren = [&stack](winrt::DependencyObject const& parent)
        {
            auto const count = winrt::VisualTreeHelper::GetChildrenCount(parent);
            for (int32_t i = count - 1; i >= 0; --i)
            {
                stack.push_back(winrt::VisualTreeHelper::GetChild(parent, i));
            }
        };

        pushChildren(wrapper);
        while (!stack.empty())
        {
            auto const node = std::move(stack.back());
            stack.pop_back();

            auto const element = node.try_as<winrt::UIElement>();
            if (!element || element.Visibility() != winrt::Visibility::Visible ||
                node.try_as<winrt::TableView>())
            {
                continue;
            }

            if (HasAppContextFlyout(element))
            {
                if (auto const anchor = element.try_as<winrt::FrameworkElement>())
                {
                    return { element.ContextFlyout(), anchor };
                }
            }

            if (node.try_as<winrt::ItemsControl>() || node.try_as<winrt::ItemsRepeater>())
            {
                continue;
            }

            pushChildren(node);
        }

        return {};
    }
}

TableViewDetails::ContextMenuCandidate TableView::ResolveContextFlyout(
    TableViewDetails::ContextMenuTarget const& target)
{
    // A ContextFlyout the app placed inside the cell or header (template or style) wins over the
    // column/row properties, so keyboard and wrapper-padding requests match what a pointer request
    // on the content already shows natively.
    if (target.Column)
    {
        if (auto content = FindContentContextFlyout(target.Anchor))
        {
            return content;
        }
    }

    if (target.Kind == TableViewDetails::ContextMenuTargetKind::Header)
    {
        return { target.Column.HeaderContextFlyout(), target.Anchor };
    }

    if (target.Column)
    {
        if (auto const flyout = target.Column.CellContextFlyout())
        {
            return { flyout, target.Anchor };
        }
    }

    // Handling the request here stops the framework from showing the row's own ContextFlyout, so
    // resolve it at the same place it would have won natively: after the cell's own menus. With
    // nothing found the request stays Unhandled and bubbles to the table's ContextFlyout.
    return FindRowContextFlyout(target);
}

void TableView::ShowContextFlyout(
    TableViewDetails::ContextMenuCandidate const& candidate,
    winrt::ContextRequestedEventArgs const& args)
{
    winrt::FlyoutShowOptions options;
    options.Placement(winrt::FlyoutPlacementMode::Auto);

    winrt::Point point{};
    if (args && args.TryGetPosition(candidate.Anchor, point))
    {
        options.Position(point);
    }

    candidate.Flyout.ShowAt(candidate.Anchor, options);
}

bool TableView::IsContextMenuRequestCurrent(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::IInspectable const& sourceSnapshot)
{
    return IsContextMenuTargetCurrent(target) &&
        (target.Kind != TableViewDetails::ContextMenuTargetKind::Body ||
            (ItemsSource() == sourceSnapshot && !IsEditing()));
}

bool TableView::FocusContextMenuTarget(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::IInspectable const& sourceSnapshot)
{
    auto const focusedElement = [this]() -> winrt::DependencyObject
    {
        auto const root = XamlRoot();
        return root
            ? winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>()
            : nullptr;
    };
    auto const isFocusWithin = [](winrt::DependencyObject const& focused, winrt::DependencyObject const& element)
    {
        return focused &&
            (focused == element || SharedHelpers::IsAncestor(focused, element, false /* checkVisibility */));
    };

    // Focus the exact target, so the flyout's focus restore returns to it and Narrator reads the
    // element the menu is for: the cell for a cell request (unless focus is already inside it, e.g.
    // on its content), the row for a row request, the header cell for a header request. Body
    // targets go through the two-level focus helpers so the cursor level follows.
    if (target.Kind != TableViewDetails::ContextMenuTargetKind::Body)
    {
        if (!isFocusWithin(focusedElement(), target.Anchor))
        {
            // Header cells are tab stops; a request on the header's content keeps focus where it is.
            target.Anchor.Focus(winrt::FocusState::Programmatic);
        }
        return true;
    }

    if (target.Column)
    {
        SetCurrentCell(target.Item, target.Column);
    }

    if (!IsContextMenuRequestCurrent(target, sourceSnapshot))
    {
        return false;
    }

    winrt::DependencyObject const rowObject = target.Row;
    auto focused = focusedElement();

    if (!target.Column)
    {
        if (focused != rowObject)
        {
            FocusRowContainerInternal(target.Row, winrt::FocusState::Programmatic);
        }
        return true;
    }

    // Entering the row from outside lands on the row first. App code reacting to that focus
    // change (removing the item, say) recycles the row back to row level, which would leave a
    // direct cell focus aimed at a cell that is no longer focusable.
    if (!isFocusWithin(focused, rowObject))
    {
        FocusRowContainerInternal(target.Row, winrt::FocusState::Programmatic);
        if (!IsContextMenuRequestCurrent(target, sourceSnapshot))
        {
            return false;
        }
        focused = focusedElement();
    }

    if (!isFocusWithin(focused, target.Anchor))
    {
        auto const rowImpl = winrt::get_self<TableViewRow>(target.Row);
        if (int32_t const column = rowImpl->GetVisibleCellIndexInternal(target.Anchor); column >= 0)
        {
            FocusRealizedRowCell(target.Row, column, winrt::FocusState::Programmatic);
        }
    }

    return true;
}

bool TableView::ProcessContextMenuRequest(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::ContextRequestedEventArgs const& args)
{
    // args is null for a row automation peer request, which has no routed event.
    if (args && args.Handled())
    {
        return false;
    }

    if (m_isProcessingContextMenu)
    {
        return true;
    }

    if (target.Kind == TableViewDetails::ContextMenuTargetKind::Body && IsEditing())
    {
        return false;
    }

    if (!IsContextMenuTargetCurrent(target))
    {
        return true;
    }

    // Moving focus runs app code. If that makes the target stale, the request stays handled only
    // when the TableView would have shown a menu, so it cannot fall through to the native chain
    // at a different element.
    bool const staleResult = static_cast<bool>(ResolveContextFlyout(target));

    auto const lifetime = get_strong();
    m_isProcessingContextMenu = true;
    auto restore = wil::scope_exit([this]() noexcept
    {
        m_isProcessingContextMenu = false;
    });
    auto const sourceSnapshot = ItemsSource();

    if (!FocusContextMenuTarget(target, sourceSnapshot) || !IsContextMenuRequestCurrent(target, sourceSnapshot))
    {
        return staleResult;
    }

    // Resolved again once focus has settled: app code run by the focus move may have replaced a
    // column's menu or the cell's content, and the menu shown is the one in place now.
    auto const candidate = ResolveContextFlyout(target);
    if (!candidate)
    {
        return false;
    }

    ShowContextFlyout(candidate, args);
    return true;
}
