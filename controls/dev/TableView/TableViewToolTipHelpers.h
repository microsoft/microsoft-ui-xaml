// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GlobalDependencyProperty.h"
#include "TVDiag.h"

#include <optional>

namespace TableViewDetails
{
    // The published HelpText is recorded, not re-derived: it is not always the tooltip's content.
    // An element whose value is null or empty is detached, not just disabled: a registered owner still
    // wins ToolTipService's innermost-owner choice and would block an ancestor's tooltip. Recycle-out only
    // neutralizes in place, which is cheaper than re-registering the owner's input handlers.
    struct CellToolTipRecord : winrt::implements<CellToolTipRecord, winrt::IInspectable>
    {
        winrt::ToolTip ToolTip{ nullptr };
        winrt::hstring PublishedHelpText{};
        bool Detached{ false };
        bool Neutralized{ false };
        bool SuppressedForEdit{ false };
    };

    inline GlobalDependencyProperty s_cellToolTipRecordProperty{ nullptr };

    // Holds the evaluated CellToolTipBinding or RowToolTipBinding value. The binding lives on the cell
    // wrapper or the row, so the row's DataContext drives it and a recycled row re-resolves through
    // ordinary inheritance.
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

    inline bool IsAttachedOwned(const winrt::FrameworkElement& element, const CellToolTipRecord& record)
    {
        return record.ToolTip && !record.Detached &&
            record.ToolTip == winrt::ToolTipService::GetToolTip(element).try_as<winrt::ToolTip>();
    }

    // Close before dropping content: clearing an open tooltip's content removes a live popup's child,
    // the shape behind the reentrant CPopup::RemoveChild crash.
    inline void Quiesce(const winrt::ToolTip& toolTip)
    {
        toolTip.IsEnabled(false);
        if (toolTip.IsOpen())
        {
            toolTip.IsOpen(false);
        }
        toolTip.Content(nullptr);
    }

    // Recycle-out: the element stays registered; recycle-in or the next value revives it.
    inline bool NeutralizeOwnedToolTip(const winrt::FrameworkElement& element)
    {
        auto const record = GetRecord(element);
        if (!record)
        {
            return false;
        }

        RetractPublishedHelpText(element, *record);
        if (IsAttachedOwned(element, *record))
        {
            Quiesce(record->ToolTip);
            record->Neutralized = true;
            return true;
        }
        return false;
    }

    inline void ClearOwnedToolTip(const winrt::FrameworkElement& element)
    {
        auto const record = GetRecord(element);
        if (!record)
        {
            return;
        }

        RetractPublishedHelpText(element, *record);

        if (IsAttachedOwned(element, *record))
        {
            Quiesce(record->ToolTip);
            // A released (pooled) element stays registered; RefreshToolTips detaches it if it is reused empty.
            if (record->Neutralized)
            {
                return;
            }
            record->Detached = true;
            // ClearValue, not SetToolTip(null): a local null would mask a tooltip the app's style sets.
            element.ClearValue(winrt::ToolTipService::ToolTipProperty());
            return;
        }

        bool const alreadyDetached = record->Detached && !winrt::ToolTipService::GetToolTip(element);
        if (!alreadyDetached && !record->SuppressedForEdit)
        {
            ForgetRecord(element);
        }
    }

    // Closes in place, leaving the tooltip attached: ToolTipService reopens it on the next enter or focus.
    inline bool TryCloseOpenOwnedToolTip(const winrt::FrameworkElement& element)
    {
        auto const record = GetRecord(element);
        if (record && IsAttachedOwned(element, *record) && record->ToolTip.IsOpen())
        {
            record->ToolTip.IsOpen(false);
            return true;
        }

        return false;
    }

    // Focus can sit on a TextElement such as an inline Hyperlink, which has no visual parent of its own.
    inline winrt::DependencyObject GetFocusWalkParent(const winrt::DependencyObject& node)
    {
        if (auto const textElement = node.try_as<winrt::Microsoft::UI::Xaml::Documents::TextElement>())
        {
            auto const start = textElement.ContentStart();
            return start ? start.VisualParent() : nullptr;
        }

        return winrt::VisualTreeHelper::GetParent(node);
    }

    // Unregistering runs ToolTipService's owner-leave path, which cancels a pending open (or closes an
    // open one); the same ToolTip is then re-attached so pointer hover keeps working.
    inline void CancelOwnedToolTipOpen(const winrt::FrameworkElement& element)
    {
        auto const record = GetRecord(element);
        if (record && IsAttachedOwned(element, *record))
        {
            element.ClearValue(winrt::ToolTipService::ToolTipProperty());
            winrt::ToolTipService::SetToolTip(element, record->ToolTip);
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

        // The value stays in the bound DP; ending the edit re-applies it.
        if (record && record->SuppressedForEdit)
        {
            ClearOwnedToolTip(element);
            return false;
        }

        // The raw value, not a ToolTip-narrowed one: a bare string would otherwise read as empty.
        auto const existingValue = winrt::ToolTipService::GetToolTip(element);
        auto const owned = (record && record->ToolTip &&
            (record->Detached ? !existingValue : record->ToolTip == existingValue.try_as<winrt::ToolTip>()))
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
            Quiesce(owned);
            owned.Content(content);
            owned.Placement(placement);
            owned.IsEnabled(true);
            record->Neutralized = false;
            if (record->Detached)
            {
                winrt::ToolTipService::SetToolTip(element, owned);
                record->Detached = false;
            }
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

        // Both paths report string content only.
        if (!text)
        {
            TVDiag::DbgLogF(L"[TableView] %ls content is not a string; it shows on hover but is "
                L"not reported to assistive technology.", propertyName);
        }

        return true;
    }

    inline const wchar_t* GetToolTipBindingPropertyName(const winrt::FrameworkElement& element)
    {
        return element.try_as<winrt::TableViewRow>() ? L"RowToolTipBinding" : L"CellToolTipBinding";
    }

    inline void RefreshOwnedToolTip(const winrt::FrameworkElement& element);

    // While a cell edit is open its row and cell keep no tooltip, whatever the bound value does, so the
    // first Escape reaches the edit. Ending the edit re-applies the bound value.
    inline void SetOwnedToolTipSuppressedForEdit(const winrt::FrameworkElement& element, bool suppressed)
    {
        if (!element)
        {
            return;
        }

        auto record = GetRecord(element);
        if (suppressed)
        {
            // No tooltip binding has ever been applied, so there is nothing to withhold.
            if (!record && !s_cellToolTipValueProperty)
            {
                return;
            }

            EnsureCellToolTipRecordProperty();
            if (!record)
            {
                record = winrt::make_self<CellToolTipRecord>();
                element.SetValue(s_cellToolTipRecordProperty, *record);
            }
            record->SuppressedForEdit = true;
            ClearOwnedToolTip(element);
        }
        else if (record && record->SuppressedForEdit)
        {
            record->SuppressedForEdit = false;
            // Contained: callers run on edit teardown paths, some inside a layout pass.
            try
            {
                RefreshOwnedToolTip(element);
            }
            catch (...)
            {
                TVDiag::LogRetailF(L"[TableView] Restoring a %ls tooltip after an edit failed (HRESULT 0x%08X).",
                    GetToolTipBindingPropertyName(element), static_cast<unsigned int>(winrt::to_hresult()));
            }
        }
    }

    // The bound value did not change, so only an explicit re-apply restores a tooltip an edit retracted.
    inline void RefreshOwnedToolTip(const winrt::FrameworkElement& element)
    {
        if (!element)
        {
            return;
        }

        if (auto const value = s_cellToolTipValueProperty ? element.GetValue(s_cellToolTipValueProperty) : nullptr)
        {
            SetOwnedToolTip(element, value, winrt::PlacementMode::Mouse, true /* publishHelpText */,
                GetToolTipBindingPropertyName(element));
        }
        else
        {
            ClearOwnedToolTip(element);
        }
    }

    // Recycle-in: the element is on screen again, so an empty value now detaches instead of staying pooled.
    inline void ReviveOwnedToolTip(const winrt::FrameworkElement& element)
    {
        if (auto const record = GetRecord(element))
        {
            record->Neutralized = false;
        }
        RefreshOwnedToolTip(element);
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
                SetOwnedToolTip(element, value, winrt::PlacementMode::Mouse, true /* publishHelpText */,
                    GetToolTipBindingPropertyName(element));
            }
            else
            {
                ClearOwnedToolTip(element);
            }
        }
        catch (...)
        {
            TVDiag::LogRetailF(L"[TableView] Applying a %ls value failed (HRESULT 0x%08X).",
                GetToolTipBindingPropertyName(element), static_cast<unsigned int>(winrt::to_hresult()));
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

    // Set once when the cell or row is created; the binding then tracks the element's DataContext.
    inline void ApplyCellToolTipBinding(
        const winrt::FrameworkElement& element,
        const winrt::Microsoft::UI::Xaml::Data::Binding& binding)
    {
        if (element && binding)
        {
            winrt::BindingOperations::SetBinding(element, EnsureCellToolTipValueProperty(), binding);
        }
    }

    inline void ClearToolTipBinding(const winrt::FrameworkElement& element)
    {
        if (!element || !s_cellToolTipValueProperty)
        {
            return;
        }

        element.ClearValue(s_cellToolTipValueProperty);
        // ClearValue raises no callback when the bound value was already null.
        ClearOwnedToolTip(element);
    }

    // Headers rebuild wholesale, so there is no binding or refresh path. Contained: both call sites
    // are fail-fast, and app content can throw (a UIElement already parented by another header).
    inline void ApplyHeaderToolTip(
        const winrt::FrameworkElement& element,
        const winrt::IInspectable& content)
    {
        try
        {
            SetOwnedToolTip(
                element,
                content,
                winrt::PlacementMode::Mouse,
                false /* publishHelpText */,
                L"HeaderToolTip");
        }
        catch (...)
        {
            TVDiag::LogRetailF(L"[TableView] Applying a header tooltip failed (HRESULT 0x%08X).",
                static_cast<unsigned int>(winrt::to_hresult()));
        }
    }
}
