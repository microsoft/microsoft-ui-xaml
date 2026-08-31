// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GlobalDependencyProperty.h"

#include <optional>

namespace TableViewDetails
{
    // The published HelpText is recorded rather than re-derived: it is not always the content.
    struct CellToolTipRecord : winrt::implements<CellToolTipRecord, winrt::IInspectable>
    {
        winrt::ToolTip ToolTip{ nullptr };
        winrt::hstring PublishedHelpText{};
    };

    inline GlobalDependencyProperty s_cellToolTipRecordProperty{ nullptr };

    inline winrt::DependencyProperty EnsureCellToolTipRecordProperty()
    {
        if (!s_cellToolTipRecordProperty)
        {
            s_cellToolTipRecordProperty = InitializeDependencyProperty(
                L"TableViewCellToolTipRecord",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::TableView>(),
                true /* isAttached */,
                nullptr /* defaultValue */,
                nullptr /* propertyChangedCallback */);
        }

        return s_cellToolTipRecordProperty;
    }

    // Called from the generated ClearTypeProperties so a XAML re-init re-registers against the new core.
    inline void ClearCellToolTipProperties()
    {
        s_cellToolTipRecordProperty = nullptr;
    }

    inline winrt::com_ptr<CellToolTipRecord> GetRecord(const winrt::FrameworkElement& element)
    {
        if (!element || !s_cellToolTipRecordProperty)
        {
            return nullptr;
        }

        return element.GetValue(s_cellToolTipRecordProperty).try_as<CellToolTipRecord>();
    }

    inline void ForgetRecord(const winrt::FrameworkElement& element)
    {
        if (s_cellToolTipRecordProperty)
        {
            element.ClearValue(s_cellToolTipRecordProperty);
        }
    }

    inline std::optional<winrt::hstring> TryGetString(const winrt::IInspectable& value)
    {
        if (auto const propertyValue = value ? value.try_as<winrt::IPropertyValue>() : nullptr;
            propertyValue && propertyValue.Type() == winrt::PropertyType::String)
        {
            return propertyValue.GetString();
        }

        return std::nullopt;
    }

    inline void RetractPublishedHelpText(const winrt::FrameworkElement& element, CellToolTipRecord& record)
    {
        if (!record.PublishedHelpText.empty() &&
            winrt::AutomationProperties::GetHelpText(element) == record.PublishedHelpText)
        {
            winrt::AutomationProperties::SetHelpText(element, winrt::hstring{});
        }

        record.PublishedHelpText = {};
    }

    // Neutralized in place rather than detached, so a recycled cell reuses the ToolTip. Matches
    // TabViewItem.
    inline void ClearOwnedToolTip(const winrt::FrameworkElement& element)
    {
        auto const record = GetRecord(element);
        if (!record)
        {
            return;
        }

        RetractPublishedHelpText(element, *record);

        if (record->ToolTip && record->ToolTip == winrt::ToolTipService::GetToolTip(element).try_as<winrt::ToolTip>())
        {
            // Close before dropping content: clearing an open tooltip's content removes a live
            // popup's child, the shape behind the reentrant CPopup::RemoveChild crash.
            record->ToolTip.IsEnabled(false);
            if (record->ToolTip.IsOpen())
            {
                record->ToolTip.IsOpen(false);
            }
            record->ToolTip.Content(nullptr);
        }
        else
        {
            ForgetRecord(element);
        }
    }

    // Returns whether the element is left carrying a control-owned tooltip. Never touches one the
    // app set itself.
    inline bool SetOwnedToolTip(
        const winrt::FrameworkElement& element,
        const winrt::IInspectable& content,
        const winrt::hstring& helpTextOverride,
        winrt::PlacementMode placement)
    {
        if (!element)
        {
            return false;
        }

        auto record = GetRecord(element);
        // The raw value, not a ToolTip-narrowed one: ToolTipService stores whatever the app set, and
        // a bare string would otherwise read as an empty slot.
        auto const existingValue = winrt::ToolTipService::GetToolTip(element);
        auto const owned = (record && record->ToolTip && record->ToolTip == existingValue.try_as<winrt::ToolTip>())
            ? record->ToolTip : nullptr;

        if (existingValue && !owned)
        {
            if (record)
            {
                RetractPublishedHelpText(element, *record);
                ForgetRecord(element);
            }
            return false;
        }

        // A ToolTip as content would render nested inside ours, and the control owns placement.
        auto const text = TryGetString(content);
        if (!content || (text && text->empty()) || content.try_as<winrt::ToolTip>())
        {
            ClearOwnedToolTip(element);
            return false;
        }

        EnsureCellToolTipRecordProperty();
        if (!record)
        {
            record = winrt::make_self<CellToolTipRecord>();
            element.SetValue(s_cellToolTipRecordProperty, *record);
        }

        RetractPublishedHelpText(element, *record);

        if (owned)
        {
            // Neutralize first: a throwing assignment must not leave the previous item's content live.
            owned.IsEnabled(false);
            if (owned.IsOpen())
            {
                owned.IsOpen(false);
            }
            owned.Content(nullptr);
            owned.Content(content);
            owned.Placement(placement);
            owned.IsEnabled(true);
        }
        else
        {
            winrt::ToolTip toolTip;
            toolTip.Content(content);
            toolTip.Placement(placement);

            // Recorded before attaching: a throw then reads as "not ours" rather than orphaning a
            // tooltip nothing can clear.
            record->ToolTip = toolTip;
            winrt::ToolTipService::SetToolTip(element, toolTip);
        }

        // Published unconditionally; TableViewCellAutomationPeer suppresses it at UIA query time if
        // it merely repeats the cell's own text. Comparing here would race the cell's binding.
        auto const helpText = !helpTextOverride.empty() ? helpTextOverride : (text ? *text : winrt::hstring{});
        if (!helpText.empty() &&
            winrt::AutomationProperties::GetHelpText(element).empty())
        {
            winrt::AutomationProperties::SetHelpText(element, helpText);
            record->PublishedHelpText = helpText;
        }

        return true;
    }
}
