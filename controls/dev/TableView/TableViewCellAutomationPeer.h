// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableView.h"
#include "TableViewCellAutomationPeer.g.h"
#include "TableViewAutomationHelpers.h"
#include <optional>
#include <vector>

// UIA peer for a realized TableView cell; supplies the cell name and grid/table item coordinates.
// Weak row/column refs avoid extending recycled rows or removed columns.
class TableViewCellAutomationPeer :
    public ReferenceTracker<TableViewCellAutomationPeer, winrt::implementation::TableViewCellAutomationPeerT, winrt::IVirtualizedItemProvider>
{
public:
    TableViewCellAutomationPeer(
        winrt::FrameworkElement const& cell,
        winrt::TableViewRow const& row,
        winrt::TableViewColumn const& column,
        int32_t columnIndex);
    ~TableViewCellAutomationPeer();

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    hstring GetNameCore();
    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();
    hstring GetHelpTextCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    hstring GetLocalizedControlTypeCore();

    // Overrides base IsTabStop-derived focusability for the two-level body navigation model.
    bool IsKeyboardFocusableCore();
    void SetFocusCore();

    // IGridItemProvider — per-cell coordinates in the owning TableView.
    int32_t Row();
    int32_t Column();
    int32_t RowSpan();
    int32_t ColumnSpan();
    winrt::IRawElementProviderSimple ContainingGrid();

    // ITableItemProvider — returns this cell's column header; rows have no headers.
    winrt::com_array<winrt::IRawElementProviderSimple> GetRowHeaderItems();
    winrt::com_array<winrt::IRawElementProviderSimple> GetColumnHeaderItems();

    // IValueProvider — lets assistive technology read the cell text and set it without a pointer.
    // SetValue drives the same public edit lifecycle a user does (BeginEdit / write / CommitEdit),
    // so BeginningEdit / CellEditEnding handlers and validation all still run.
    winrt::hstring Value();
    bool IsReadOnly();
    void SetValue(winrt::hstring const& value);

    // IVirtualizedItemProvider — available only after the owning row has been recycled out.
    void Realize();

    winrt::hstring ReadNameForEdit();
    void BeginEditName();
    void EndEditName();
    void ResetEditName();
    void UpdateNameItem(winrt::IInspectable const& item);

private:
    // True when this cell is editable AND its column produces a TextBox editor, which is the only
    // editor SetValue can drive today.
    bool SupportsValuePattern();

    // Resolves the row index from the owning ItemsRepeater.
    int32_t GetRowIndex();
    bool IsVirtualized();
    void RealizeCore();
    winrt::UIElement GetRealizedCellFromRow(winrt::TableViewRow const& row);
    winrt::TableView GetTrackedTableForRow(winrt::TableViewRow const& row);
    winrt::IInspectable GetTrackedItem() const;
    int32_t GetTrackedItemIndex(winrt::TableView const& tableView);
    int32_t GetItemOccurrenceAtIndex(winrt::TableView const& tableView, winrt::IInspectable const& item, int32_t targetIndex);
    bool IsTrackedRow(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    void TrackRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    [[noreturn]] static void ThrowElementNotAvailable();

    // Resolves the column's stringified Header.
    winrt::hstring GetColumnHeaderText();

    // Resolves displayed text from the TextBlock or content peer name.
    winrt::hstring GetCellValueText();
    winrt::hstring ReadDisplayName(winrt::FrameworkElement const& display = nullptr);
    void PrepareAutomationContentView(winrt::FrameworkElement const& cell);
    bool GetCachedHasInteractiveCellContent(winrt::FrameworkElement const& cell);
    winrt::IInspectable GetCurrentAutomationContentItem();
    bool IsAutomationContentCacheValid(
        winrt::FrameworkElement const& content,
        winrt::IInspectable const& item) const;
    bool CachedAutomationContentItemMatches(winrt::IInspectable const& item) const;
    bool HasInteractiveCellContentAndRegisterCallbacks(
        winrt::UIElement const& element,
        uint32_t depthBudget = 8,
        uint32_t* remainingBudget = nullptr);
    void RegisterAutomationContentPropertyCallbacks(winrt::UIElement const& element);
    void InvalidateAutomationContentViewCache();
    void ResetAutomationContentViewCache() noexcept;
    void QueueFinalName(uint64_t generation);

    struct AutomationContentPropertyChangedRevoker
    {
        AutomationContentPropertyChangedRevoker() noexcept = default;
        AutomationContentPropertyChangedRevoker(AutomationContentPropertyChangedRevoker const&) = delete;
        AutomationContentPropertyChangedRevoker& operator=(AutomationContentPropertyChangedRevoker const&) = delete;

        AutomationContentPropertyChangedRevoker(AutomationContentPropertyChangedRevoker&& other) noexcept
        {
            MoveFrom(other);
        }

        AutomationContentPropertyChangedRevoker& operator=(AutomationContentPropertyChangedRevoker&& other) noexcept
        {
            MoveFrom(other);
            return *this;
        }

        AutomationContentPropertyChangedRevoker(
            winrt::DependencyObject const& object,
            winrt::DependencyProperty const& property,
            int64_t token) :
            m_object(object),
            m_property(property),
            m_token(token)
        {
        }

        ~AutomationContentPropertyChangedRevoker() noexcept
        {
            Revoke();
        }

        void Revoke() noexcept
        {
            if (auto const object = m_object.get())
            {
                try
                {
                    object.UnregisterPropertyChangedCallback(m_property, m_token);
                }
                catch (...)
                {
                }
            }

            m_object = nullptr;
            m_property = nullptr;
            m_token = 0;
        }

    private:
        void MoveFrom(AutomationContentPropertyChangedRevoker& other) noexcept
        {
            if (this != &other)
            {
                Revoke();
                m_object = other.m_object;
                m_property = other.m_property;
                m_token = other.m_token;
                other.m_object = nullptr;
                other.m_property = nullptr;
                other.m_token = 0;
            }
        }

        winrt::weak_ref<winrt::DependencyObject> m_object{ nullptr };
        winrt::DependencyProperty m_property{ nullptr };
        int64_t m_token{ 0 };
    };

    winrt::weak_ref<winrt::TableViewRow> m_row{ nullptr };
    winrt::weak_ref<winrt::TableViewColumn> m_column{ nullptr };
    winrt::weak_ref<winrt::TableView> m_lastOwningTable{ nullptr };
    TableViewTrackedItemIdentity m_item;
    TableViewTrackedItemIdentity m_automationContentItem;
    // Construction-time fallback only; Column() recomputes from the live cell host.
    int32_t m_columnIndex{ -1 };
    int32_t m_lastKnownRowIndex{ -1 };
    int32_t m_trackedItemOccurrence{ -1 };
    tracker_ref<winrt::IInspectable> m_nameItem{ this };
    std::optional<winrt::hstring> m_lastName;
    // Holds the pre-edit Name only; read paths must not write it or live cell names freeze.
    std::optional<winrt::hstring> m_editName;
    uint64_t m_nameGeneration{ 0 };
    winrt::FrameworkElement::LayoutUpdated_revoker m_nameLayoutUpdatedRevoker{};
    winrt::weak_ref<winrt::FrameworkElement> m_automationContent{ nullptr };
    std::optional<bool> m_hasInteractiveAutomationContent;
    std::vector<AutomationContentPropertyChangedRevoker> m_automationContentPropertyChangedRevokers;
};
