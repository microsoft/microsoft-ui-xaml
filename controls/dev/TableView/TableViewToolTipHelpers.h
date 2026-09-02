// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GlobalDependencyProperty.h"
#include "TVDiag.h"

#include <optional>

namespace TableViewDetails
{
    // The published HelpText is recorded, not re-derived: it is not always the tooltip's content.
    struct CellToolTipRecord : winrt::implements<CellToolTipRecord, winrt::IInspectable>
    {
        winrt::ToolTip ToolTip{ nullptr };
        winrt::hstring PublishedHelpText{};
    };

    inline GlobalDependencyProperty s_cellToolTipRecordProperty{ nullptr };

    // Holds the evaluated CellToolTipBinding value. The binding lives on the cell wrapper, so the
    // row's DataContext drives it and a recycled row re-resolves through ordinary inheritance.
    inline GlobalDependencyProperty s_cellToolTipValueProperty{ nullptr };

    inline void EnsureCellToolTipRecordProperty()
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
    }

    // Called from the generated ClearTypeProperties so a XAML re-init re-registers against the new core.
    inline void ClearCellToolTipProperties()
    {
        s_cellToolTipRecordProperty = nullptr;
        s_cellToolTipValueProperty = nullptr;
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

    // Neutralized in place rather than detached, so a recycled cell reuses the ToolTip. Matches TabViewItem.
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
    // app set itself. publishHelpText is false when a peer publishes the text instead.
    inline bool SetOwnedToolTip(
        const winrt::FrameworkElement& element,
        const winrt::IInspectable& content,
        winrt::PlacementMode placement,
        bool publishHelpText = true,
        const wchar_t* propertyName = L"CellToolTipBinding")
    {
        if (!element)
        {
            return false;
        }

        auto record = GetRecord(element);
        // The raw value, not a ToolTip-narrowed one: a bare string would otherwise read as empty.
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
        if (content && content.try_as<winrt::ToolTip>())
        {
            TVDiag::LogRetailF(L"[TableView] A %ls value must be tooltip content, "
                L"not a ToolTip; the element has no tooltip.", propertyName);
            ClearOwnedToolTip(element);
            return false;
        }

        if (!content || (text && text->empty()))
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

            // Record before attaching: a throw then reads as "not ours" rather than orphaning a
            // tooltip nothing can clear.
            record->ToolTip = toolTip;
            winrt::ToolTipService::SetToolTip(element, toolTip);
        }

        // Published unconditionally and recorded; TableViewCellAutomationPeer decides at query time
        // whether it merely repeats the cell's own text. Comparing here would race the cell's binding.
        auto const helpText = text ? *text : winrt::hstring{};
        if (publishHelpText &&
            !helpText.empty() &&
            winrt::AutomationProperties::GetHelpText(element).empty())
        {
            winrt::AutomationProperties::SetHelpText(element, helpText);
            record->PublishedHelpText = helpText;
        }

        return true;
    }

    // The bound value did not change, so only an explicit re-apply restores a tooltip an edit retracted.
    inline void RefreshOwnedToolTip(const winrt::FrameworkElement& element)
    {
        if (!element || !s_cellToolTipValueProperty)
        {
            return;
        }

        if (auto const value = element.GetValue(s_cellToolTipValueProperty))
        {
            SetOwnedToolTip(element, value, winrt::PlacementMode::Mouse);
        }
        else
        {
            ClearOwnedToolTip(element);
        }
    }

    // The whole per-cell update path: no event, no invalidation, no realization-time work.
    inline void OnCellToolTipValueChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args)
    {
        auto const element = sender.try_as<winrt::FrameworkElement>();
        if (!element)
        {
            return;
        }

        // Contained: this runs from the property system, where an escaping exception is a fail-fast.
        try
        {
            if (auto const value = args.NewValue())
            {
                SetOwnedToolTip(element, value, winrt::PlacementMode::Mouse);
            }
            else
            {
                ClearOwnedToolTip(element);
            }
        }
        catch (...)
        {
            TVDiag::LogRetailF(L"[TableView] Applying a cell tooltip value failed (HRESULT 0x%08X).",
                static_cast<unsigned int>(winrt::to_hresult()));
        }
    }

    inline winrt::DependencyProperty EnsureCellToolTipValueProperty()
    {
        if (!s_cellToolTipValueProperty)
        {
            s_cellToolTipValueProperty = InitializeDependencyProperty(
                L"TableViewCellToolTipValue",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::TableView>(),
                true /* isAttached */,
                nullptr /* defaultValue */,
                winrt::PropertyChangedCallback(&OnCellToolTipValueChanged));
        }

        return s_cellToolTipValueProperty;
    }

    // Set once when the cell is created; the binding then tracks the row's DataContext.
    inline void ApplyCellToolTipBinding(
        const winrt::FrameworkElement& element,
        const winrt::Microsoft::UI::Xaml::Data::Binding& binding)
    {
        if (element && binding)
        {
            winrt::BindingOperations::SetBinding(element, EnsureCellToolTipValueProperty(), binding);
        }
    }

    // Headers are rebuilt wholesale, never recycled, so there is no binding or refresh path.
    // Contained: both call sites run from the property system or a header rebuild, where an
    // escaping exception is a fail-fast, and app-supplied content can throw (a UIElement already
    // parented by another cell's tooltip).
    inline void ApplyHeaderToolTip(
        const winrt::FrameworkElement& element,
        const winrt::IInspectable& content)
    {
        try
        {
            const bool attached = SetOwnedToolTip(
                element,
                content,
                winrt::PlacementMode::Mouse,
                false /* publishHelpText */,
                L"HeaderToolTip");

            // Rich content is a legitimate choice, but it is mouse-only: the header peer reports
            // string content and nothing else, so an author gets no other signal.
            if (attached && !TryGetString(content))
            {
                TVDiag::DbgLogF(L"[TableView] HeaderToolTip content is not a string; it shows on "
                    L"hover but is not reported to assistive technology.");
            }
        }
        catch (...)
        {
            TVDiag::LogRetailF(L"[TableView] Applying a header tooltip failed (HRESULT 0x%08X).",
                static_cast<unsigned int>(winrt::to_hresult()));
        }
    }
}
