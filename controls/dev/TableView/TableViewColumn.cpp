// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewColumn.h"
#include "TableView.h"

#include <algorithm>
#include <cmath>

TableViewColumn::TableViewColumn()
{
    // Defaults let initial ActualWidth equal Width without clamping.
}

winrt::FrameworkElement TableViewColumn::GenerateElement(const winrt::IInspectable& dataItem)
{
    // Forward to the overridable display-cell factory.
    return overridable().GenerateElementCore(dataItem);
}

winrt::FrameworkElement TableViewColumn::GenerateElementCore(const winrt::IInspectable& /*dataItem*/)
{
    // Base columns have no display surface.
    return nullptr;
}

winrt::FrameworkElement TableViewColumn::GenerateEditingElement(const winrt::IInspectable& dataItem)
{
    // Forward to the overridable editing-cell factory.
    return GenerateEditingElementCore(dataItem);
}

winrt::FrameworkElement TableViewColumn::GenerateEditingElementCore(const winrt::IInspectable& /*dataItem*/)
{
    // A CellEditingTemplate is the column-agnostic way to supply an editor, so the base honours it
    // for every column type. Without one a base column declines the edit: returning null is how a
    // column says "this cell cannot be edited", rather than opening an empty editor over it.
    auto const editingTemplate = CellEditingTemplate();
    if (!editingTemplate)
    {
        return nullptr;
    }

    winrt::ContentPresenter presenter;
    presenter.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
    presenter.VerticalAlignment(winrt::VerticalAlignment::Stretch);
    presenter.ContentTemplate(editingTemplate);

    // Unlike the display cell - whose Content is bound by TableViewRow::RebuildCells so it survives
    // recycle - the editing presenter lives only for the duration of one edit on one item, so its
    // Content is bound here against its own inherited DataContext.
    winrt::Microsoft::UI::Xaml::Data::Binding contentBinding;
    contentBinding.Path(winrt::PropertyPath{ L"DataContext" });
    contentBinding.RelativeSource(winrt::Microsoft::UI::Xaml::Data::RelativeSource{});
    contentBinding.RelativeSource().Mode(winrt::Microsoft::UI::Xaml::Data::RelativeSourceMode::Self);
    winrt::BindingOperations::SetBinding(presenter, winrt::ContentPresenter::ContentProperty(), contentBinding);

    return presenter;
}

winrt::IInspectable TableViewColumn::PrepareCellForEdit(const winrt::FrameworkElement& editingElement, const winrt::RoutedEventArgs& editingEventArgs)
{
    return PrepareCellForEditCore(editingElement, editingEventArgs);
}

winrt::IInspectable TableViewColumn::PrepareCellForEditCore(const winrt::FrameworkElement& editingElement, const winrt::RoutedEventArgs& /*editingEventArgs*/)
{
    if (!editingElement)
    {
        return nullptr;
    }

    // Focus so typing goes straight into the editor; otherwise the row keeps focus and the user has
    // to click the editor they just opened. The editing root is not necessarily focusable - a
    // template column produces a ContentPresenter - so fall back to its first focusable descendant.
    if (!editingElement.Focus(winrt::FocusState::Programmatic))
    {
        if (auto const focusable = winrt::FocusManager::FindFirstFocusableElement(editingElement))
        {
            if (auto const focusableElement = focusable.try_as<winrt::Control>())
            {
                focusableElement.Focus(winrt::FocusState::Programmatic);
            }
            else if (auto const focusableUi = focusable.try_as<winrt::UIElement>())
            {
                focusableUi.Focus(winrt::FocusState::Programmatic);
            }
        }
    }

    // The base has no single value to snapshot - cancel is served by re-pulling from the source
    // (see CancelCellEditCore), which needs nothing carried across.
    return nullptr;
}

bool TableViewColumn::CommitCellEdit(const winrt::FrameworkElement& editingElement)
{
    return CommitCellEditCore(editingElement);
}

// The editing bindings use UpdateSourceTrigger::Explicit, so this is what actually moves the typed
// value onto the data item.
//
// Resolved HERE rather than when the edit opened: a CellEditingTemplate's ContentPresenter has not
// stamped its template at begin time, so a walk then finds nothing. By commit time the editor is
// realized. WPF sidesteps the same ordering problem with an UpdateLayout() call in BeginEdit.
//
// Returns false when nothing could be written, so the control keeps the edit open instead of
// reporting a commit that never reached the item.
bool TableViewColumn::CommitCellEditCore(const winrt::FrameworkElement& editingElement)
{
    auto const expressions = TableViewColumn::CollectEditingBindingExpressions(editingElement);
    if (expressions.empty())
    {
        return false;
    }

    bool wroteEverything = true;
    for (auto const& expression : expressions)
    {
        try
        {
            expression.UpdateSource();
        }
        catch (...)
        {
            // A setter that throws is the app rejecting the value, not a control failure.
            wroteEverything = false;
        }
    }

    return wroteEverything;
}

void TableViewColumn::CancelCellEdit(const winrt::FrameworkElement& editingElement, const winrt::IInspectable& uneditedValue)
{
    CancelCellEditCore(editingElement, uneditedValue);
}

// Cancel needs to do nothing in the general case, and deliberately so.
//
// The editing binding is UpdateSourceTrigger::Explicit, so a cancelled edit never reached the data
// item: the source still holds the pre-edit value. The editor is then discarded and the display
// element - bound to that same untouched source - is put back. WinUI's BindingExpression has no
// UpdateTarget(), but none is needed, because the target being refreshed is thrown away.
//
// A column whose editor writes outside its binding must override this.
void TableViewColumn::CancelCellEditCore(const winrt::FrameworkElement& /*editingElement*/, const winrt::IInspectable& /*uneditedValue*/)
{
}

// Binding expressions on the editor subtree, for the properties an editor realistically writes.
// Used by the base commit so a CellEditingTemplate works without the column knowing what the app
// put in it. A column that knows its own editor should override and answer exactly.
std::vector<winrt::BindingExpression> TableViewColumn::CollectEditingBindingExpressions(const winrt::FrameworkElement& element)
{
    std::vector<winrt::BindingExpression> expressions;
    if (!element)
    {
        return expressions;
    }

    std::vector<winrt::DependencyProperty> properties;
    if (element.try_as<winrt::TextBox>())
    {
        properties.push_back(winrt::TextBox::TextProperty());
    }
    if (element.try_as<winrt::AutoSuggestBox>())
    {
        properties.push_back(winrt::AutoSuggestBox::TextProperty());
    }
    if (element.try_as<winrt::ToggleSwitch>())
    {
        properties.push_back(winrt::ToggleSwitch::IsOnProperty());
    }
    if (element.try_as<winrt::ToggleButton>())
    {
        properties.push_back(winrt::ToggleButton::IsCheckedProperty());
    }
    if (element.try_as<winrt::Selector>())
    {
        properties.push_back(winrt::Selector::SelectedItemProperty());
        properties.push_back(winrt::Selector::SelectedIndexProperty());
        properties.push_back(winrt::Selector::SelectedValueProperty());
    }
    if (element.try_as<winrt::Slider>())
    {
        properties.push_back(winrt::RangeBase::ValueProperty());
    }
    if (element.try_as<winrt::NumberBox>())
    {
        properties.push_back(winrt::NumberBox::ValueProperty());
    }

    for (auto const& property : properties)
    {
        if (auto const expression = element.GetBindingExpression(property))
        {
            expressions.push_back(expression);
        }
    }

    const auto childCount = winrt::VisualTreeHelper::GetChildrenCount(element);
    for (int32_t i = 0; i < childCount; ++i)
    {
        if (auto const child = winrt::VisualTreeHelper::GetChild(element, i).try_as<winrt::FrameworkElement>())
        {
            auto childExpressions = TableViewColumn::CollectEditingBindingExpressions(child);
            expressions.insert(expressions.end(), childExpressions.begin(), childExpressions.end());
        }
    }

    return expressions;
}

// Typed accessor for the owning TableView.
winrt::TableView TableViewColumn::GetOwningTableView()
{
    // Keep the owner weak; callers acquire a strong ref only for synchronous work.
    return m_owningTableView.get();
}

void TableViewColumn::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto property = args.Property();

    if (property == s_WidthProperty ||
        property == s_MinWidthProperty ||
        property == s_MaxWidthProperty)
    {
        UpdateActualWidth();
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnWidthChanged(*this);
        }
    }
    else if (property == s_IsReadOnlyProperty)
    {
        // Closing an edit is the control's job - it owns the edit state - but the column is where
        // the property change surfaces. Without this the editor stays open on a cell that has just
        // declared itself read-only.
        if (auto const owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnIsReadOnlyChanged(*this);
        }
    }
    else if (property == s_CellEditingTemplateProperty)
    {
        // Realized editors were generated from the previous template.
        NotifyCellContentChanged();
    }
    else if (property == s_HeaderTemplateProperty ||
             property == s_HeaderTemplateSelectorProperty ||
             property == s_HeaderProperty)
    {
        // Re-render headers and recompute Auto widths after header content changes.
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnHeaderChanged(*this);
        }
    }
    else if (property == s_FrozenEdgeProperty)
    {
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnFrozenEdgeChanged(*this);
        }
    }
    else if (property == s_CanResizeProperty)
    {
        // Adds or removes this column's gripper, so the header band has to be rebuilt.
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->QueueRebuildHeaders();
        }
    }
    else if (property == s_VisibilityProperty)
    {
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnVisibilityChanged(*this);
        }
    }
    else if (property == s_CanSortProperty)
    {
        // The sort affordance is stamped at header-build time, so opting a column in or out at
        // runtime has to rebuild the headers for the chevron to appear or disappear.
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnCanSortChanged(*this);
        }
    }
}

void TableViewColumn::CustomSortComparer(winrt::ITableViewSortComparer const& value)
{
    if (m_customSortComparer == value)
    {
        return;
    }

    m_customSortComparer = value;

    // Swapping the comparer invalidates the order the column is currently sorted in, so re-apply
    // the active direction rather than leaving the rows in the old comparer's order.
    if (auto owner = GetOwningTableView(); owner && SortDirection() != winrt::SortDirection::None)
    {
        auto const direction = SortDirection();
        auto ownerImpl = winrt::get_self<TableView>(owner);
        // Force a re-sort: SortByColumn no-ops when the column already carries this direction.
        ownerImpl->SortByColumn(*this, winrt::SortDirection::None);
        ownerImpl->SortByColumn(*this, direction);
    }
}

winrt::hstring TableViewColumn::GetSortMemberPathCore(){
    return SortMemberPath();
}

void TableViewColumn::SetSortStateInternal(winrt::SortDirection direction)
{
    if (SortDirection() == direction)
    {
        return;
    }

    // Read-only DP: same SetValue-via-key convention as ActualWidth.
    SetValue(s_SortDirectionProperty, winrt::box_value(direction));

    if (auto owner = GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->RefreshSortIndicators();
    }
}

// Lets a derived column refresh its realized cells when one of ITS OWN properties changes.
//
// The base deliberately does not enumerate derived types' dependency properties: a third-party
// column would then have no way to invalidate its cells without editing this file. This mirrors
// WPF, where DataGridColumn owns the notification and each derived column routes its own property
// callbacks into it.
void TableViewColumn::NotifyCellContentChanged()
{
    if (auto owner = GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->OnColumnCellTemplateChanged(*this);
    }
}

winrt::Microsoft::UI::Xaml::Data::Binding TableViewColumn::CellToolTipBinding()
{
    return m_cellToolTipBinding.get();
}

void TableViewColumn::CellToolTipBinding(const winrt::Microsoft::UI::Xaml::Data::Binding& value)
{
    m_cellToolTipBinding.set(value);

    // Realized cells carry the previous binding. The only invalidation this feature needs.
    NotifyCellContentChanged();
}

bool TableViewColumn::SetOwningTableViewInternal(winrt::TableView const& owner)
{
    // Keep the owner weak to avoid TableView -> Columns -> Column -> TableView cycles.
    if (owner)
    {
        // Single-owner: detach (owner cleared to null) from the first TableView before attaching to another.
        if (auto const existing = m_owningTableView.get(); existing && existing != owner)
        {
            // Runs inside a Columns VectorChanged callback; an escaping throw would failfast.
            // Assert in debug, keep the existing owner in retail.
            MUX_ASSERT_MSG(false, "TableViewColumn re-owned without removing it from the previous TableView.Columns first");
            return false;
        }
        m_owningTableView = winrt::make_weak(owner);
        return true;
    }
    else
    {
        m_owningTableView = nullptr;
        return true;
    }
}

void TableViewColumn::SetResolvedActualWidthInternal(double width)
{
    // TableView resolved this column against the viewport / desired content. Uses the same
    // SetValue-via-key read-only DP convention as UpdateActualWidth; the caller has already
    // clamped to Min/MaxWidth.
    if (std::abs(width - ActualWidth()) > 0.0001)
    {
        SetValue(s_ActualWidthProperty, winrt::box_value(width));
    }
}

void TableViewColumn::SetDesiredWidthInternal(double desiredWidth)
{
    m_desiredWidth = std::max(0.0, desiredWidth);
}

void TableViewColumn::ResetDesiredWidthInternal()
{
    m_desiredWidth = 0.0;
}

void TableViewColumn::UpdateActualWidth()
{
    // Pixel/Auto resolve to a fixed width here; Star gets a provisional default and is later
    // resolved against the viewport by the owning TableView (TableView::ResolveColumnWidths).
    const auto width = Width();
    const double widthPixels =
        width.GridUnitType == winrt::GridUnitType::Pixel
            ? width.Value
            : c_widthDefault.Value;

    // Keep std::clamp well-defined even when MinWidth exceeds MaxWidth.
    const double lo = MinWidth();
    const double hi = std::max(lo, MaxWidth());
    const double clamped = std::clamp(widthPixels, lo, hi);

    // ActualWidth uses the SetValue-via-key read-only DP convention.
    if (std::abs(clamped - ActualWidth()) > 0.0001)
    {
        SetValue(s_ActualWidthProperty, winrt::box_value(clamped));
    }
}

