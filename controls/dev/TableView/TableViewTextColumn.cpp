// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewTextColumn.h"
#include "TableView.h"

TableViewTextColumn::TableViewTextColumn()
{
}

winrt::Microsoft::UI::Xaml::Data::Binding TableViewTextColumn::Binding()
{
    return m_binding.get();
}

void TableViewTextColumn::Binding(const winrt::Microsoft::UI::Xaml::Data::Binding& value)
{
    m_binding.set(value);
    // Rebuild realized cells so the new Binding is applied immediately;
    // virtualized rows pick it up when realized.
    if (auto owner = GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->OnColumnCellTemplateChanged(*this);
    }
}

winrt::FrameworkElement TableViewTextColumn::GenerateElementCore(const winrt::IInspectable& /*dataItem*/)
{
    // Let XAML binding pick up the row data item from the cell DataContext.
    winrt::TextBlock textBlock;
    auto const owner = GetOwningTableView();
    // Cell content is left-aligned and vertically centered within the row (standard grid look).
    textBlock.VerticalAlignment(winrt::VerticalAlignment::Center);
    // FontWeight stays unset so a TableView/app value inherits.
    textBlock.FontSize(owner ? winrt::get_self<TableView>(owner)->GetCellFontSize() : 14.0);
    // Density-aware built-in cell padding (Standard = 8,4,8,4 = unchanged default).
    if (owner)
    {
        textBlock.Padding(winrt::get_self<TableView>(owner)->GetDensityCellPadding());
    }
    else
    {
        textBlock.Padding(winrt::ThicknessHelper::FromLengths(8, 4, 8, 4));
    }
    textBlock.TextTrimming(winrt::TextTrimming::CharacterEllipsis);

    if (auto binding = m_binding.get())
    {
        winrt::BindingOperations::SetBinding(textBlock, winrt::TextBlock::TextProperty(), binding);
    }

    return textBlock;
}

winrt::hstring TableViewTextColumn::GetSortMemberPathCore()
{
    // An explicit SortMemberPath always wins: it is how a consumer sorts on a field the cell does
    // not display (e.g. show a formatted name, sort on a sequence number).
    if (auto const explicitPath = SortMemberPath(); !explicitPath.empty())
    {
        return explicitPath;
    }

    // Otherwise sort on whatever the cell shows, but only for bindings against the row data item: a
    // binding against an explicit source names a path on THAT object, and sorting the rows by it
    // would be meaningless.
    if (auto const binding = m_binding.get())
    {
        if (binding.Source() || binding.RelativeSource() || !binding.ElementName().empty())
        {
            return {};
        }

        if (auto const path = binding.Path())
        {
            return path.Path();
        }
    }

    return {};
}
