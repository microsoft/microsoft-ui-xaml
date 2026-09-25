// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewColumn.h"
#include "TableViewCellsPanel.h"
#include "TableViewContextFlyoutRequestedEventArgs.h"

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

    if (target.Column)
    {
        uint32_t index{};
        auto const columns = owner.Columns();
        if (!columns || !columns.IndexOf(target.Column, index) ||
            target.Column.Visibility() != winrt::Visibility::Visible ||
            winrt::get_self<TableViewColumn>(target.Column)->GetOwningTableView() != owner)
        {
            return false;
        }
    }

    if (target.Kind == ContextMenuTargetKind::Body)
    {
        if (!target.Row || winrt::get_self<TableViewRow>(target.Row)->GetOwningTableView() != owner)
        {
            return false;
        }

        return winrt::get_self<TableViewRow>(target.Row)->IsContextMenuTargetCurrent(target);
    }

    auto const ownerHeadersVisibility = owner.HeadersVisibility();
    if (target.Row || target.Item || !target.Column ||
        target.ScopeRoot != m_headerHost.get() ||
        ownerHeadersVisibility != winrt::TableViewHeadersVisibility::Column)
    {
        return false;
    }

    return target.Anchor.Visibility() == winrt::Visibility::Visible &&
        winrt::VisualTreeHelper::GetParent(target.Anchor) == target.ScopeRoot &&
        TableViewCellsPanel::CellForColumn(target.ScopeRoot, target.Column) == target.Anchor;
}

std::optional<TableViewDetails::ContextMenuTarget> TableView::ResolveHeaderContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args)
{
    auto const host = m_headerHost.get();
    if (!host)
    {
        return std::nullopt;
    }

    auto node = args.OriginalSource().try_as<winrt::DependencyObject>();
    while (node && node != host)
    {
        if (node.try_as<winrt::TableView>() || node.try_as<winrt::TableViewRow>())
        {
            return std::nullopt;
        }

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

    auto const result = ProcessContextMenuRequest(*target, args);
    if (result != TableViewDetails::ContextMenuResult::Unhandled)
    {
        args.Handled(true);
    }
}

winrt::FlyoutBase TableView::ResolveContextFlyout(
    TableViewDetails::ContextMenuTarget const& target)
{
    if (target.Kind == TableViewDetails::ContextMenuTargetKind::Header)
    {
        return target.Column.HeaderContextFlyout();
    }

    if (target.Column)
    {
        if (auto const flyout = target.Column.CellContextFlyout())
        {
            return flyout;
        }
    }

    return RowContextFlyout();
}

winrt::TableViewContextFlyoutRequestedEventArgs TableView::RaiseContextFlyoutRequested(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& resolvedFlyout)
{
    auto result = winrt::make_self<TableViewContextFlyoutRequestedEventArgs>(
        target.Item,
        target.Column,
        target.Kind == TableViewDetails::ContextMenuTargetKind::Header,
        resolvedFlyout);
    if (m_contextFlyoutRequestedEventSource)
    {
        m_contextFlyoutRequestedEventSource(*this, *result);
    }
    return *result;
}

void TableView::ShowContextFlyout(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& flyout,
    winrt::ContextRequestedEventArgs const& args)
{
    winrt::FlyoutShowOptions options;
    options.Placement(winrt::FlyoutPlacementMode::Auto);

    winrt::Point point{};
    if (args.TryGetPosition(target.Anchor, point))
    {
        options.Position(point);
    }

    flyout.ShowAt(target.Anchor, options);
}

TableViewDetails::ContextMenuResult TableView::ProcessContextMenuRequest(
    TableViewDetails::ContextMenuTarget target,
    winrt::ContextRequestedEventArgs const& args)
{
    using TableViewDetails::ContextMenuResult;
    using TableViewDetails::ContextMenuTargetKind;

    if (args.Handled())
    {
        return ContextMenuResult::Unhandled;
    }

    if (m_isProcessingContextMenu)
    {
        return ContextMenuResult::Suppressed;
    }

    bool const body = target.Kind == ContextMenuTargetKind::Body;
    if (body && IsEditing())
    {
        return ContextMenuResult::Unhandled;
    }

    if (!IsContextMenuTargetCurrent(target))
    {
        return ContextMenuResult::Suppressed;
    }

    auto const lifetime = get_strong();
    m_isProcessingContextMenu = true;
    auto restore = wil::scope_exit([this]() noexcept
    {
        m_isProcessingContextMenu = false;
    });
    auto const sourceSnapshot = ItemsSource();
    auto const requestIsCurrent = [&]()
    {
        return IsContextMenuTargetCurrent(target) &&
            (!body || (ItemsSource() == sourceSnapshot && !IsEditing()));
    };

    if (body)
    {
        if (target.Column)
        {
            SetCurrentCell(target.Item, target.Column);
        }

        if (!requestIsCurrent())
        {
            return ContextMenuResult::Suppressed;
        }

        target.Row.Focus(winrt::FocusState::Programmatic);
        if (!requestIsCurrent())
        {
            return ContextMenuResult::Suppressed;
        }
    }

    auto flyout = ResolveContextFlyout(target);
    if (!flyout)
    {
        return ContextMenuResult::Unhandled;
    }

    auto decision = RaiseContextFlyoutRequested(target, flyout);
    if (decision.Handled())
    {
        return ContextMenuResult::Suppressed;
    }

    flyout = decision.ContextFlyout();
    if (!flyout)
    {
        return ContextMenuResult::Unhandled;
    }

    if (!requestIsCurrent())
    {
        return ContextMenuResult::Suppressed;
    }

    ShowContextFlyout(target, flyout, args);
    return ContextMenuResult::Shown;
}
