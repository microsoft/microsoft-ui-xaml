// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewSource.h"

namespace
{
    // The class name is compared, never reported.
    uint32_t ColumnKindOf(winrt::TableViewColumn const& column)
    {
        uint32_t kinds{};
        if (column.try_as<winrt::TableViewTemplateColumn>()) { kinds |= TableViewTelemetry::ColumnKind::Template; }
        else if (column.try_as<winrt::TableViewTextColumn>()) { kinds |= TableViewTelemetry::ColumnKind::Text; }
        auto const className = winrt::get_class_name(column);
        std::wstring_view const name{ className };
        if (name != winrt::name_of<winrt::TableViewTextColumn>() && name != winrt::name_of<winrt::TableViewTemplateColumn>() &&
            name != winrt::name_of<winrt::TableViewColumn>())
        {
            kinds |= TableViewTelemetry::ColumnKind::AppDerived;
        }
        return kinds;
    }
}

// TableView_Instance's two sample points: Loaded and Unloaded. Reads TableView's own state; no app code runs beyond
// the columns' QueryInterface and class name, and an app-supplied Columns vector.
void TableView::SampleInstanceTelemetry() noexcept
{
    if (!m_telemetrySession.active || !TableViewTelemetry::ShouldSample()) { return; }
    try
    {
        TableViewTelemetry::Sample sample;
        sample.sourceKinds = m_telemetrySourceKinds;
        sample.rightToLeft = FlowDirection() == winrt::FlowDirection::RightToLeft;
        bool anySortable{};
        bool anyEditable{};
        if (auto const columns = Columns())
        {
            for (auto const& column : columns)
            {
                if (!column) { continue; }
                sample.columnKinds |= ColumnKindOf(column);
                anySortable = anySortable || column.CanSort();
                anyEditable = anyEditable || !column.IsReadOnly();
            }
        }
        if (CanUserSortColumns() && anySortable) { sample.enabledFeatures |= TableViewTelemetry::Feature::Sorting; }
        if (!IsReadOnly() && anyEditable) { sample.enabledFeatures |= TableViewTelemetry::Feature::Editing; }
        if (auto const source = m_activeSource.get())
        {
            auto const* const sourceImpl = winrt::get_self<::TableViewSource>(source);
            if (sourceImpl->HasFilter()) { sample.enabledFeatures |= TableViewTelemetry::Feature::Filtering; }
            if (sourceImpl->HasGroupBy()) { sample.enabledFeatures |= TableViewTelemetry::Feature::Grouping; }
            if (sourceImpl->HasParentBy()) { sample.enabledFeatures |= TableViewTelemetry::Feature::Hierarchy; }
            if (sourceImpl->IsLiveShaping()) { sample.enabledFeatures |= TableViewTelemetry::Feature::LiveShaping; }
            sample.dataRows = sourceImpl->DataRowCount();
            if (sample.dataRows == 0)
            {
                TableViewTelemetry::NoteContent(m_telemetrySession, TableViewTelemetry::ContentShown::Empty);
            }
        }
        if (auto const repeater = m_rowsRepeater.get())
        {
            // A cleared container stays the repeater's child in its recycle pool, so this is the containers created.
            sample.peakRealizedRows = static_cast<uint32_t>(winrt::VisualTreeHelper::GetChildrenCount(repeater));
        }
        TableViewTelemetry::AddSample(m_telemetrySession, sample);
    }
    catch (...)
    {
        // Telemetry never throws into the app; a sample that could not be read leaves the session as it was.
    }
}