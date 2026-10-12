// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewColumn.h"
#include "TableViewCellsPanel.h"
#include "SharedHelpers.h"

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
        // app set on the element itself counts.
        return !IsTextControl(element) ||
            element.ReadLocalValue(winrt::UIElement::ContextFlyoutProperty()) != winrt::DependencyProperty::UnsetValue();
    }
}

TableViewDetails::ContextMenuCandidate TableViewDetails::FindContentContextFlyout(
    winrt::FrameworkElement const& wrapper)
{
    if (!wrapper)
    {
        return {};
    }

    // Depth-first in visual order, so the outermost app element (the template root) wins over
    // anything nested inside it.
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

        pushChildren(node);
    }

    return {};
}

TableViewDetails::ContextMenuCandidate TableView::ResolveContextFlyout(
    TableViewDetails::ContextMenuTarget const& target)
{
    // A ContextFlyout the app placed inside the cell or header (template or style) wins over the
    // column/row properties, so keyboard and wrapper-padding requests match what a pointer request
    // on the content already shows natively.
    if (target.Column)
    {
        if (auto content = TableViewDetails::FindContentContextFlyout(target.Anchor))
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

    return { RowContextFlyout(), target.Anchor };
}

void TableView::ShowContextFlyout(
    TableViewDetails::ContextMenuCandidate const& candidate,
    winrt::ContextRequestedEventArgs const& args)
{
    winrt::FlyoutShowOptions options;
    options.Placement(winrt::FlyoutPlacementMode::Auto);

    winrt::Point point{};
    if (args.TryGetPosition(candidate.Anchor, point))
    {
        options.Position(point);
    }

    candidate.Flyout.ShowAt(candidate.Anchor, options);
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
        // Capture participation only; valid requests resolve their candidate after focus.
        auto const invalidatedResult = ResolveContextFlyout(target)
            ? ContextMenuResult::Suppressed
            : ContextMenuResult::Unhandled;

        if (target.Column)
        {
            SetCurrentCell(target.Item, target.Column);
        }

        if (!requestIsCurrent())
        {
            return invalidatedResult;
        }

        // Focus the row only when focus is outside it. Pulling focus off a focused cell would make
        // the flyout's focus restore land on the row's first cell and move the current cell there.
        winrt::DependencyObject const rowObject = target.Row;
        auto const root = XamlRoot();
        auto const focused = root
            ? winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>()
            : nullptr;
        if (!focused ||
            (focused != rowObject && !SharedHelpers::IsAncestor(focused, rowObject, false /* checkVisibility */)))
        {
            target.Row.Focus(winrt::FocusState::Programmatic);
        }

        if (!requestIsCurrent())
        {
            return invalidatedResult;
        }
    }

    auto const candidate = ResolveContextFlyout(target);
    if (!candidate)
    {
        return ContextMenuResult::Unhandled;
    }

    ShowContextFlyout(candidate, args);
    return ContextMenuResult::Shown;
}
