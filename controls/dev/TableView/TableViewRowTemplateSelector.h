// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewRowTemplateSelector.g.h"

// Chooses between a TableViewRow and a TableViewGroupHeader container template. Assigned to
// PART_RowsRepeater.ItemTemplate, so ItemsRepeater wraps it in ItemTemplateWrapper and the
// framework supplies the per-template recycle pools and owner-aware unparenting.
class TableViewRowTemplateSelector :
    public winrt::implementation::TableViewRowTemplateSelectorT<TableViewRowTemplateSelector>
{
public:
    TableViewRowTemplateSelector() = default;

    // ItemTemplateWrapper calls the single-arg form; the two-arg overload delegates to it so
    // the two cannot drift.
    winrt::DataTemplate SelectTemplateCore(winrt::IInspectable const& item);
    winrt::DataTemplate SelectTemplateCore(winrt::IInspectable const& item, winrt::DependencyObject const& container);

    void SetOwningTableViewInternal(winrt::TableView const& owner);

    // Must run from ~TableView while it still holds the selector. Once anything has been
    // recycled the pools hang off the cached templates and close the cycle
    // repeater -> wrapper -> selector -> template -> pool -> repeater through plain C++
    // references the reference tracker cannot walk, so nothing is collected and the selector's
    // own destructor never runs. Dropping the pools breaks the one edge we own.
    void Detach();

private:
    winrt::DataTemplate ResolveTemplateFromMarkup(std::wstring_view markup);
    void EnsureTemplates();

    winrt::weak_ref<winrt::TableView> m_owningTableView{ nullptr };

    // Cached: ItemTemplateWrapper keys the recycle pool by DataTemplate instance, so the same
    // instance must come back every call. Resolved lazily -- no resources exist at construction.
    winrt::DataTemplate m_rowTemplate{ nullptr };
    winrt::DataTemplate m_groupHeaderTemplate{ nullptr };
    bool m_templatesResolved{ false };

    // Markup rather than a resource key: these would live in the control's generic dictionary,
    // whose root LookupElementResource does not walk, so both keys resolve to null and the null
    // template fails realization. ViewManager and ItemsView use XamlReader::Load for the same
    // reason. HorizontalAlignment=Left is required: the band sizes itself from MeasureOverride,
    // and Stretch centres a narrower desired width, leaving a gap at both ends.
    static constexpr std::wstring_view s_rowContainerMarkup{
        L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L" xmlns:tv='using:Microsoft.UI.Xaml.Controls.Tabular'>"
        L"<tv:TableViewRow/></DataTemplate>" };

    static constexpr std::wstring_view s_groupHeaderContainerMarkup{
        L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L" xmlns:tv='using:Microsoft.UI.Xaml.Controls.Tabular'>"
        L"<tv:TableViewGroupHeader HorizontalAlignment='Left'/></DataTemplate>" };
};
