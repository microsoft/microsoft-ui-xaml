// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewColumn.h"
#include "TableViewTextColumn.g.h"

class TableViewTextColumn :
    public winrt::implementation::TableViewTextColumnT<TableViewTextColumn, TableViewColumn>
{
public:
    ForwardRefToBaseReferenceTracker(TableViewColumn)

    TableViewTextColumn();

    // Setter lets XAML pass the Binding object through without evaluating it.
    winrt::Microsoft::UI::Xaml::Data::Binding Binding();
    void Binding(const winrt::Microsoft::UI::Xaml::Data::Binding& value);

    // Override
    winrt::hstring GetSortMemberPathCore() override;
    winrt::FrameworkElement GenerateElementCore(const winrt::IInspectable& dataItem) override;
    winrt::FrameworkElement GenerateEditingElementCore(const winrt::IInspectable& dataItem) override;
    winrt::IInspectable PrepareCellForEditCore(const winrt::FrameworkElement& editingElement, const winrt::RoutedEventArgs& editingEventArgs) override;
    bool CommitCellEditCore(const winrt::FrameworkElement& editingElement) override;
    void CancelCellEditCore(const winrt::FrameworkElement& editingElement, const winrt::IInspectable& uneditedValue) override;

    // Internal, not projected: the data field this column edits, derived from Binding rather than
    // authored separately. The public answer to "which field is this column about?" is the base
    // column's SortMemberPath, so this must never become a second public concept.
    winrt::hstring GetEditingPropertyPath() const;

private:
    tracker_ref<winrt::Microsoft::UI::Xaml::Data::Binding> m_binding{ this };
};
