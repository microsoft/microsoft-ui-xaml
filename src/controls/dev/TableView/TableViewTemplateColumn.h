// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewColumn.h"
#include "TableViewTemplateColumn.g.h"
#include "TableViewTemplateColumn.properties.h"

class TableViewTemplateColumn :
    public winrt::implementation::TableViewTemplateColumnT<TableViewTemplateColumn, TableViewColumn>,
    public TableViewTemplateColumnProperties
{
public:
    // Resolve MI ambiguity — TemplateColumn's Properties shadows Column's.
    using TableViewTemplateColumnProperties::EnsureProperties;
    using TableViewTemplateColumnProperties::ClearProperties;

    ForwardRefToBaseReferenceTracker(TableViewColumn)

    TableViewTemplateColumn();

    // Override
    winrt::FrameworkElement GenerateElementCore(const winrt::IInspectable& dataItem) override;

    // CellTemplate routes through here rather than being special-cased by the base class, so a
    // third-party column can do the same. CellEditingTemplate is handled by the base.
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
