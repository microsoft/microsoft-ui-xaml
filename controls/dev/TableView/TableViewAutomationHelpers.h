// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "TableView.h"
#include "TableViewAutomationPeer.h"
#include "TableViewCellsPanel.h"
#include "TableViewColumn.h"
#include "TableViewGroupingHelpers.h"
#include "ResourceAccessor.h"
#include "Utils.h"

// Shared helpers for the TableView automation peers, so the visible-column and column-header-string
// logic lives in one place instead of being copy-pasted across the peer translation units. Assumes
// pch.h (winrt type aliases) is included first, per the TableView header convention.

class TableViewTrackedItemIdentity
{
public:
    void Track(winrt::IInspectable const& item)
    {
        m_hasItem = static_cast<bool>(item);
        m_weakItem = winrt::weak_ref<winrt::IInspectable>{ nullptr };
        m_strongItem = nullptr;

        if (!item)
        {
            return;
        }

        if (item.try_as<::IWeakReferenceSource>())
        {
            m_weakItem = winrt::make_weak(item);
        }
        else
        {
            // Some app data items do not implement IWeakReferenceSource. Hold those narrowly as
            // strong identity tokens: they are ItemsSource objects, not peers/containers, so this
            // does not add a reference path back from app data to TableView.
            m_strongItem = item;
        }
    }

    bool IsTracking() const noexcept
    {
        return m_hasItem;
    }

    winrt::IInspectable Resolve() const
    {
        return m_strongItem ? m_strongItem : m_weakItem.get();
    }

    bool SameIdentityAs(winrt::IInspectable const& candidate) const
    {
        auto const item = Resolve();
        return candidate && item && TableView::SameInspectableIdentity(candidate, item);
    }

private:
    bool m_hasItem{ false };
    winrt::weak_ref<winrt::IInspectable> m_weakItem{ nullptr };
    winrt::IInspectable m_strongItem{ nullptr };
};

// Resource lookups feeding UIA are supplementary: degrade instead of letting a missing PRI (a host
// app that does not merge the control's resources) escape into a UIA call.
inline winrt::hstring TryGetLocalizedString(const std::wstring_view& resourceName)
{
    try
    {
        return ResourceAccessor::GetLocalizedStringResource(resourceName);
    }
    catch (...)
    {
        return {};
    }
}

inline winrt::hstring LocalizedOrFallbackForTableViewAutomation(std::wstring_view resourceName, std::wstring_view fallback) noexcept
{
    try
    {
        if (auto const resolved = ResourceAccessor::GetLocalizedStringResource(resourceName); !resolved.empty())
        {
            return resolved;
        }
    }
    catch (...)
    {
    }

    return winrt::hstring{ fallback };
}

inline winrt::hstring FormatLocalizedOrFallback(
    std::wstring_view resourceName,
    std::wstring_view fallback,
    wchar_t const* first,
    wchar_t const* second,
    std::wstring_view finalSeparator) noexcept
{
    auto const format = LocalizedOrFallbackForTableViewAutomation(resourceName, fallback);
    if (auto const formatted = StringUtil::FormatString(format, first, second, L"", L""); !formatted.empty())
    {
        return formatted;
    }

    if (auto const formatted = StringUtil::FormatString(fallback, first, second, L"", L""); !formatted.empty())
    {
        return formatted;
    }

    return winrt::hstring{ std::wstring{ first } + std::wstring{ finalSeparator } + second };
}

inline winrt::hstring FormatUIntForItemName(uint64_t value) noexcept
{
    try
    {
        if (auto const formatter = TableViewDetails::CreateCurrentCultureDecimalFormatter())
        {
            formatter.FractionDigits(0);
            return formatter.FormatUInt(value);
        }
    }
    catch (...)
    {
    }

    return winrt::to_hstring(value);
}

inline winrt::hstring FormatDoubleForItemName(double value) noexcept
{
    try
    {
        if (auto const formatter = TableViewDetails::CreateCurrentCultureDecimalFormatter())
        {
            formatter.FractionDigits(0);
            return formatter.FormatDouble(value);
        }
    }
    catch (...)
    {
    }

    return winrt::to_hstring(value);
}

inline bool IsVisibleColumn(winrt::TableViewColumn const& column)
{
    return column && column.Visibility() == winrt::Visibility::Visible;
}

inline winrt::AutomationPeer GetRealizedColumnHeaderPeer(
    winrt::TableView const& table,
    winrt::TableViewColumn const& column)
{
    if (!table || !IsVisibleColumn(column) ||
        winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != table)
    {
        return nullptr;
    }

    if (auto const tablePeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(table)
            .try_as<winrt::TableViewAutomationPeer>())
    {
        if (auto const headerPeer = winrt::get_self<TableViewAutomationPeer>(tablePeer)
                ->GetOrCreateColumnHeaderPeer(table, column))
        {
            return headerPeer;
        }
    }

    if (auto const host = winrt::get_self<TableView>(table)->GetHeaderHostInternal())
    {
        if (auto const header = TableViewCellsPanel::CellForColumn(host, column))
        {
            return winrt::FrameworkElementAutomationPeer::CreatePeerForElement(header);
        }
    }
    return nullptr;
}

// Stringifies a data item for UIA. Shared by the TableView peer's item search and the row peer's
// name fallback, so both describe the same item the same way.
inline winrt::hstring ItemToName(winrt::IInspectable const& item)
{
    // Boxed WinRT primitives surface as IPropertyValue, not IStringable.
    if (auto const propValue = item.try_as<winrt::IPropertyValue>())
    {
        switch (propValue.Type())
        {
        case winrt::PropertyType::String:  return propValue.GetString();
        case winrt::PropertyType::Boolean: return propValue.GetBoolean() ? LocalizedOrFallbackForTableViewAutomation(SR_TableViewBooleanTrue, L"True") : LocalizedOrFallbackForTableViewAutomation(SR_TableViewBooleanFalse, L"False");
        case winrt::PropertyType::Int16:   return TableViewDetails::FormatIntegerForCurrentCulture(propValue.GetInt16());
        case winrt::PropertyType::Int32:   return TableViewDetails::FormatIntegerForCurrentCulture(propValue.GetInt32());
        case winrt::PropertyType::Int64:   return TableViewDetails::FormatIntegerForCurrentCulture(propValue.GetInt64());
        case winrt::PropertyType::UInt8:   return FormatUIntForItemName(propValue.GetUInt8());
        case winrt::PropertyType::UInt16:  return FormatUIntForItemName(propValue.GetUInt16());
        case winrt::PropertyType::UInt32:  return FormatUIntForItemName(propValue.GetUInt32());
        case winrt::PropertyType::UInt64:  return FormatUIntForItemName(propValue.GetUInt64());
        case winrt::PropertyType::Single:  return FormatDoubleForItemName(propValue.GetSingle());
        case winrt::PropertyType::Double:  return FormatDoubleForItemName(propValue.GetDouble());
        default: break;
        }
    }

    if (auto const stringable = item.try_as<winrt::IStringable>())
    {
        return stringable.ToString();
    }

    return {};
}

inline winrt::hstring GroupInfoToName(winrt::TableViewGroupInfo const& info)
{
    const auto keyText = info.KeyText();
    const auto countText = info.ItemCountText();
    if (!keyText.empty() && !countText.empty())
    {
        return FormatLocalizedOrFallback(SR_TableViewGroupHeaderNameFormat, L"%1!s! %2!s!", keyText.c_str(), countText.c_str(), L" ");
    }
    return keyText;
}

inline int32_t CountVisibleColumns(winrt::IVector<winrt::TableViewColumn> const& columns)
{
    int32_t count = 0;
    for (auto const& column : columns)
    {
        if (IsVisibleColumn(column))
        {
            ++count;
        }
    }
    return count;
}

constexpr int32_t c_maxCellContentChildrenPerLevel = 32;

inline bool ShouldWalkCellContentSubtree(winrt::FrameworkElement const& content)
{
    return content &&
        (content.try_as<winrt::ContentPresenter>() ||
            content.try_as<winrt::Panel>() ||
            content.try_as<winrt::Border>());
}

inline winrt::hstring GetExplicitCellContentName(
    winrt::FrameworkElement const& content, bool allowPeerCreation)
{
    if (auto const name = winrt::AutomationProperties::GetName(content); !name.empty())
    {
        return name;
    }
    if (auto const label = winrt::AutomationProperties::GetLabeledBy(content))
    {
        auto const peer = allowPeerCreation
            ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(label)
            : winrt::FrameworkElementAutomationPeer::FromElement(label);
        if (peer)
        {
            if (auto const name = peer.GetName(); !name.empty())
            {
                return name;
            }
        }
    }
    return {};
}

// Returns the text a cell displays: the column-generated TextBlock's text, else the content's own
// computed UIA name. Shared so a cell's name and the row name composed from its cells agree.
//
// allowPeerCreation gates the fallback: CreatePeerForElement does not just read a name, it creates
// and permanently attaches a peer. Worth it for a cell naming itself; not for the row name, which
// walks every cell on every name query, so it passes false and only reads already-realized content.
inline winrt::hstring GetCellContentName(
    winrt::FrameworkElement const& content, bool allowPeerCreation, uint32_t depth, uint32_t& remaining)
{
    if (!content || content.Visibility() != winrt::Visibility::Visible || depth == 0 || remaining == 0)
    {
        return {};
    }
    --remaining;

    if (auto const name = GetExplicitCellContentName(content, allowPeerCreation); !name.empty())
    {
        return name;
    }
    if (auto const textBlock = content.try_as<winrt::TextBlock>())
    {
        return textBlock.Text();
    }

    // Templated presenter peers can stringify the data object instead of the visible template.
    auto const presenter = content.try_as<winrt::ContentPresenter>();
    if (!presenter || !presenter.ContentTemplate())
    {
        auto const peer = allowPeerCreation
            ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(content)
            : winrt::FrameworkElementAutomationPeer::FromElement(content);
        if (peer)
        {
            if (auto const name = peer.GetName(); !name.empty())
            {
                return name;
            }
        }
    }

    // A named control describes its own content; do not repeat its inner interactive labels.
    // Only traverse layout wrappers, and bound the work of each cell-name query.
    //
    // The per-level child cap is deliberate and is NOT merely a copy of the view-adjustment walks.
    // Removing it here (leaving only the shared `remaining` budget) was measured to destabilise the
    // column-header peer tests, which walk realized header content and create peers as they go. The
    // cap also costs nothing on the cell path, whose budget is 32 to begin with
    // (TableViewCellAutomationPeer::ReadDisplayName), so only the 64-budget header path could ever
    // see past it. Raising or removing this bound is a separate, measurable change - not a cleanup.
    if (ShouldWalkCellContentSubtree(content))
    {
        const auto count = winrt::VisualTreeHelper::GetChildrenCount(content);
        for (int32_t i = 0; i < count && i < c_maxCellContentChildrenPerLevel && remaining > 0; ++i)
        {
            if (auto const child = winrt::VisualTreeHelper::GetChild(content, i).try_as<winrt::FrameworkElement>())
            {
                if (auto const name = GetCellContentName(child, allowPeerCreation, depth - 1, remaining);
                    !name.empty())
                {
                    return name;
                }
            }
        }
    }
    return {};
}

inline winrt::FrameworkElement GetCellContentElement(winrt::FrameworkElement const& cell)
{
    if (!cell)
    {
        return nullptr;
    }
    if (auto const border = cell.try_as<winrt::Border>())
    {
        return border.Child().try_as<winrt::FrameworkElement>();
    }
    if (auto const grid = cell.try_as<winrt::Grid>(); grid && grid.Tag().try_as<winrt::TableViewColumn>())
    {
        auto const children = grid.Children();
        return children.Size() ? children.GetAt(0).try_as<winrt::FrameworkElement>() : nullptr;
    }
    return cell;
}

inline winrt::FrameworkElement GetCellAutomationContent(winrt::FrameworkElement const& cell)
{
    return GetCellContentElement(cell);
}

inline bool IsFocusableCellContent(winrt::UIElement const& element)
{
    if (!element || element.Visibility() != winrt::Visibility::Visible)
    {
        return false;
    }

    auto const control = element.try_as<winrt::Control>();
    return control && control.IsEnabled() && control.IsTabStop();
}

inline bool ContainsFocusableElement(
    winrt::UIElement const& element,
    uint32_t depthBudget = 8,
    uint32_t* remainingBudget = nullptr)
{
    uint32_t localBudget = 64;
    auto budget = remainingBudget ? remainingBudget : &localBudget;
    if (!element || element.Visibility() != winrt::Visibility::Visible || depthBudget == 0 || *budget == 0)
    {
        return false;
    }

    --(*budget);
    if (IsFocusableCellContent(element))
    {
        return true;
    }

    auto const childCount = winrt::VisualTreeHelper::GetChildrenCount(element);
    for (int32_t i = 0; i < childCount && i < c_maxCellContentChildrenPerLevel && *budget > 0; ++i)
    {
        if (auto const child = winrt::VisualTreeHelper::GetChild(element, i).try_as<winrt::UIElement>())
        {
            if (ContainsFocusableElement(child, depthBudget - 1, budget))
            {
                return true;
            }
        }
    }

    return false;
}

inline bool HasInteractiveCellContent(winrt::FrameworkElement const& cell)
{
    return ContainsFocusableElement(GetCellAutomationContent(cell));
}

inline void SetAccessibilityViewIfNeeded(
    winrt::FrameworkElement const& element,
    winrt::AccessibilityView const& view)
{
    if (element && winrt::AutomationProperties::GetAccessibilityView(element) != view)
    {
        winrt::AutomationProperties::SetAccessibilityView(element, view);
    }
}

inline bool ShouldPreserveCellContentElement(winrt::FrameworkElement const& element)
{
    if (!element)
    {
        return false;
    }

    if (element.try_as<winrt::ProgressBar>())
    {
        return true;
    }

    if (element.try_as<winrt::Image>())
    {
        return !winrt::AutomationProperties::GetName(element).empty() ||
            static_cast<bool>(winrt::AutomationProperties::GetLabeledBy(element));
    }

    return false;
}

inline void SetCellContentAccessibilityViewRaw(winrt::FrameworkElement const& root, uint32_t depthBudget = 8)
{
    if (!root || root.Visibility() != winrt::Visibility::Visible || depthBudget == 0 ||
        ShouldPreserveCellContentElement(root))
    {
        return;
    }

    SetAccessibilityViewIfNeeded(root, winrt::AccessibilityView::Raw);

    auto const childCount = winrt::VisualTreeHelper::GetChildrenCount(root);
    for (int32_t i = 0; i < childCount && i < c_maxCellContentChildrenPerLevel; ++i)
    {
        if (auto const child = winrt::VisualTreeHelper::GetChild(root, i).try_as<winrt::FrameworkElement>())
        {
            SetCellContentAccessibilityViewRaw(child, depthBudget - 1);
        }
    }
}

inline void SetInteractiveCellContentAccessibilityViewContent(winrt::FrameworkElement const& root, uint32_t depthBudget = 8)
{
    if (!root || root.Visibility() != winrt::Visibility::Visible || depthBudget == 0)
    {
        return;
    }

    if (IsFocusableCellContent(root))
    {
        SetAccessibilityViewIfNeeded(root, winrt::AccessibilityView::Content);
        return;
    }

    auto const childCount = winrt::VisualTreeHelper::GetChildrenCount(root);
    for (int32_t i = 0; i < childCount && i < c_maxCellContentChildrenPerLevel; ++i)
    {
        if (auto const child = winrt::VisualTreeHelper::GetChild(root, i).try_as<winrt::FrameworkElement>())
        {
            SetInteractiveCellContentAccessibilityViewContent(child, depthBudget - 1);
        }
    }
}

inline winrt::hstring GetCellDisplayText(winrt::FrameworkElement const& cell, bool allowPeerCreation = true)
{
    auto const content = GetCellContentElement(cell);
    uint32_t remaining = 32;
    return GetCellContentName(content, allowPeerCreation, 8, remaining);
}

// Returns the column Header's string form (an IStringable, or a String-typed IPropertyValue), or
// nullopt when the header is not a string (or the column is null). Returning nullopt rather than an
// empty string lets callers distinguish "no string header" from "an explicitly empty string header".
inline std::optional<winrt::hstring> TryGetColumnHeaderString(winrt::TableViewColumn const& column)
{
    if (column)
    {
        auto const header = column.Header();
        if (auto const stringable = header.try_as<winrt::IStringable>())
        {
            return stringable.ToString();
        }
        if (auto const propValue = header.try_as<winrt::IPropertyValue>())
        {
            if (propValue.Type() == winrt::PropertyType::String)
            {
                return propValue.GetString();
            }
        }
    }
    return std::nullopt;
}
