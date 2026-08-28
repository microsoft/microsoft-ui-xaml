// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewTemplateColumn.h"

TableViewTemplateColumn::TableViewTemplateColumn()
{
}

winrt::FrameworkElement TableViewTemplateColumn::GenerateElementCore(const winrt::IInspectable& /*dataItem*/)
{
    // Use a presenter so templates bind against the row data item.
    winrt::ContentPresenter presenter;
    presenter.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
    presenter.VerticalAlignment(winrt::VerticalAlignment::Stretch);

    // Only wire content when a template exists; otherwise leave the presenter empty instead of
    // rendering ToString().
    if (auto cellTemplate = CellTemplate())
    {
        presenter.ContentTemplate(cellTemplate);
        // Content is wired by TableViewRow::RebuildCells to a binding against the cell WRAPPER's
        // inherited DataContext, so recycled rows update reactively. It deliberately is NOT bound to
        // the presenter's own DataContext: ContentPresenter pins its DataContext to its Content, so a
        // self-referential Content binding would freeze after the first item (stale cells on recycle).
    }

    return presenter;
}

void TableViewTemplateColumn::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.Property() == s_CellTemplateProperty)
    {
        // Realized cells were generated from the previous template.
        NotifyCellContentChanged();
        return;
    }

    __super::OnPropertyChanged(args);
}
