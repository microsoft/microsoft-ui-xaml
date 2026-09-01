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
    // Fluent body text: theme font size / Normal.
    textBlock.FontSize(owner ? winrt::get_self<TableView>(owner)->GetCellFontSize() : 14.0);
    textBlock.FontWeight(winrt::FontWeights::Normal());
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

winrt::FrameworkElement TableViewTextColumn::GenerateEditingElementCore(const winrt::IInspectable& dataItem)
{
    // An explicit CellEditingTemplate wins: a text column is a convenience over the template path,
    // not a separate mechanism, so an app can replace the editor without subclassing.
    if (CellEditingTemplate())
    {
        return __super::GenerateEditingElementCore(dataItem);
    }

    // A column with no Binding has nothing to write back to, so it declines the edit rather than
    // opening a TextBox whose contents could never be committed.
    auto const binding = m_binding.get();
    if (!binding)
    {
        return nullptr;
    }

    winrt::TextBox textBox;
    auto const owner = GetOwningTableView();

    // Match the display cell's metrics so swapping the TextBlock for the TextBox does not shift the
    // text or resize the row as the edit opens.
    textBox.VerticalAlignment(winrt::VerticalAlignment::Center);
    textBox.FontSize(owner ? winrt::get_self<TableView>(owner)->GetCellFontSize() : 14.0);
    textBox.FontWeight(winrt::FontWeights::Normal());
    textBox.Padding(owner
        ? winrt::get_self<TableView>(owner)->GetDensityCellPadding()
        : winrt::ThicknessHelper::FromLengths(8, 4, 8, 4));

    // The shipping TextBox style carries its own MinHeight (32px), which is taller than a Compact
    // TableView row (30px). Left alone, opening an editor grows the row and shifts the rows below
    // it. Clamp to the row's own minimum so the swap is visually neutral.
    if (owner)
    {
        textBox.MinHeight(winrt::get_self<TableView>(owner)->GetDensityRowMinHeight());
    }

    // The consumer's Binding is reused rather than copied field-by-field so that Converter,
    // ConverterParameter, StringFormat and TargetNullValue all keep working in the editor.
    // Only the parts that editing requires are forced.
    winrt::Microsoft::UI::Xaml::Data::Binding editingBinding;
    editingBinding.Path(binding.Path());
    editingBinding.Converter(binding.Converter());
    editingBinding.ConverterParameter(binding.ConverterParameter());
    editingBinding.ConverterLanguage(binding.ConverterLanguage());
    editingBinding.TargetNullValue(binding.TargetNullValue());
    editingBinding.FallbackValue(binding.FallbackValue());

    // Carry the source selector across. Without this a display binding that targets an explicit
    // object or named element silently becomes a binding against the row's DataContext, and the
    // edit would be written to a different object than the one being displayed.
    if (auto const source = binding.Source())
    {
        editingBinding.Source(source);
    }
    if (auto const elementName = binding.ElementName(); !elementName.empty())
    {
        editingBinding.ElementName(elementName);
    }
    if (auto const relativeSource = binding.RelativeSource())
    {
        editingBinding.RelativeSource(relativeSource);
    }

    // TwoWay is required: a OneWay display binding would never write the edited value back.
    editingBinding.Mode(winrt::Microsoft::UI::Xaml::Data::BindingMode::TwoWay);

    // Explicit rather than the default PropertyChanged: the control decides when the value lands on
    // the item, so a cancel can restore the pre-edit value and a validation failure can hold the
    // edit open. With PropertyChanged every keystroke would already have mutated the item.
    editingBinding.UpdateSourceTrigger(winrt::Microsoft::UI::Xaml::Data::UpdateSourceTrigger::Explicit);

    winrt::BindingOperations::SetBinding(textBox, winrt::TextBox::TextProperty(), editingBinding);

    return textBox;
}

winrt::IInspectable TableViewTextColumn::PrepareCellForEditCore(const winrt::FrameworkElement& editingElement, const winrt::RoutedEventArgs& editingEventArgs)
{
    __super::PrepareCellForEditCore(editingElement, editingEventArgs);

    auto const textBox = editingElement.try_as<winrt::TextBox>();
    if (!textBox)
    {
        // A CellEditingTemplate replaced the built-in TextBox; the base already handled focus.
        return nullptr;
    }

    // Select-all matches the platform grid convention: the first keystroke replaces the value.
    textBox.SelectAll();

    // The pre-edit text, handed back to CancelCellEditCore. Not strictly needed while cancel is a
    // re-pull from the source, but it makes the column's revert independent of the binding.
    return winrt::box_value(textBox.Text());
}

// Exact, rather than the base's subtree walk: this column knows its editor is a TextBox bound on
// Text, so it can commit and revert without inspecting anything.
bool TableViewTextColumn::CommitCellEditCore(const winrt::FrameworkElement& editingElement)
{
    auto const textBox = editingElement.try_as<winrt::TextBox>();
    if (!textBox)
    {
        return __super::CommitCellEditCore(editingElement);
    }

    auto const expression = textBox.GetBindingExpression(winrt::TextBox::TextProperty());
    if (!expression)
    {
        return false;
    }

    try
    {
        expression.UpdateSource();
    }
    catch (...)
    {
        // The app rejected the value in its setter.
        return false;
    }

    return true;
}

void TableViewTextColumn::CancelCellEditCore(const winrt::FrameworkElement& editingElement, const winrt::IInspectable& uneditedValue)
{
    // Restoring the text is not strictly required - the editor is discarded and the display element
    // re-reads the source, which an Explicit binding never wrote - but it keeps the editor coherent
    // for anything that observes it during teardown.
    if (auto const textBox = editingElement.try_as<winrt::TextBox>())
    {
        if (auto const original = uneditedValue.try_as<winrt::hstring>())
        {
            textBox.Text(original.value());
            return;
        }
    }

    __super::CancelCellEditCore(editingElement, uneditedValue);
}

winrt::hstring TableViewTextColumn::GetSortMemberPathCore()
{
    // An explicit SortMemberPath always wins: it is how a consumer sorts on a field the cell does
    // not display (e.g. show a formatted name, sort on a sequence number).
    if (auto const explicitPath = SortMemberPath(); !explicitPath.empty())
    {
        return explicitPath;
    }

    // Otherwise sort on whatever the cell shows. The same source-qualification rule as editing
    // applies - a binding against an explicit source names a path on THAT object, and sorting the
    // rows by it would be meaningless.
    return GetEditingPropertyPath();
}

winrt::hstring TableViewTextColumn::GetEditingPropertyPath() const
{
    // Only reported for bindings against the row data item. When the binding names an explicit
    // source the path is relative to THAT object, and the control's snapshot/validation - which
    // resolve against the row item - would touch the wrong one. The editor's own binding expression
    // remains the commit mechanism there, and the empty answer keeps the control from guessing.
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
