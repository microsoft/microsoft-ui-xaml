// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewColumn.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TVDiag.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

namespace
{
    std::atomic<uint32_t> s_nextAutomationIdentity{ 1 };

    bool TryFocusEditor(winrt::FrameworkElement const& editor)
    {
        if (editor.Focus(winrt::FocusState::Programmatic))
        {
            return true;
        }
        if (auto const target = winrt::FocusManager::FindFirstFocusableElement(editor))
        {
            if (auto const control = target.try_as<winrt::Control>())
            {
                return control.Focus(winrt::FocusState::Programmatic);
            }
            if (auto const element = target.try_as<winrt::UIElement>())
            {
                return element.Focus(winrt::FocusState::Programmatic);
            }
        }
        return false;
    }

    // Realizing and focusing a CellEditingTemplate's content runs the app's code, so a throw there leaves tagged as the
    // app's; the rest of the editing hooks is TableView's own code.
    template <class Call>
    auto RunEditorContent(bool appContent, Call&& call)
    {
        try
        {
            return call();
        }
        catch (...)
        {
            if (appContent) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Factory); }
            throw;
        }
    }

}

TableViewColumn::TableViewColumn()
{
    // Defaults let initial ActualWidth equal Width without clamping.
}

winrt::FrameworkElement TableViewColumn::GenerateDisplayElement(const winrt::IInspectable& dataItem)
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

    // A CellEditingTemplate's content is the app's (see GenerateEditingElementCore); TableView's own editor is not.
    bool const appContent = CellEditingTemplate() != nullptr;

    // Focus so typing goes straight into the editor; otherwise the row keeps focus and the user has
    // to click the editor they just opened. The editing root is not necessarily focusable - a
    // template column produces a ContentPresenter - so fall back to its first focusable descendant.
    auto const root = editingElement.XamlRoot();
    auto const initialFocus = root ? winrt::FocusManager::GetFocusedElement(root) : nullptr;
    if (!RunEditorContent(appContent, [&] { return TryFocusEditor(editingElement); }))
    {
        if (root && winrt::FocusManager::GetFocusedElement(root) != initialFocus)
        {
            return nullptr;
        }
        auto const owner = GetOwningTableView();
        auto const editingParent = winrt::VisualTreeHelper::GetParent(editingElement);
        winrt::TableViewRow row{ nullptr };
        for (auto parent = editingParent; parent;
            parent = winrt::VisualTreeHelper::GetParent(parent))
        {
            if (auto const candidate = parent.try_as<winrt::TableViewRow>())
            {
                row = candidate;
                break;
            }
        }
        auto const item = row ? row.DataContext() : nullptr;

        // The editing ContentPresenter may not have instantiated its DataTemplate.
        // Realize it before returning from F2 so the first keystroke has a target.
        RunEditorContent(appContent, [&] { editingElement.UpdateLayout(); });

        // Layout can run application code and recycle the row or end the edit.
        if (owner && (GetOwningTableView() != owner ||
            winrt::get_self<TableView>(owner)->CurrentEditingElement() != editingElement ||
            !editingParent || winrt::VisualTreeHelper::GetParent(editingElement) != editingParent ||
            !row || winrt::get_self<TableViewRow>(row)->GetOwningTableView() != owner ||
            !TableView::SameInspectableIdentity(row.DataContext(), item)))
        {
            return nullptr;
        }
        if (root && winrt::FocusManager::GetFocusedElement(root) != initialFocus)
        {
            // A Loaded/focus handler chose another target; preserve that choice.
            return nullptr;
        }
        if (!RunEditorContent(appContent, [&] { return TryFocusEditor(editingElement); }))
        {
            TVDiag::LogRetailF(L"[TableView] The realized cell editor has no available keyboard focus target.");
        }
    }

    // The base has no single value to snapshot - cancel is served by re-pulling from the source
    // (see CancelCellEditCore), which needs nothing carried across.
    return nullptr;
}

bool TableViewColumn::CommitCellEdit(const winrt::FrameworkElement& editingElement)
{
    // Projected dispatch so an app override runs, as for GenerateElementCore.
    try
    {
        return overridable().CommitCellEditCore(editingElement);
    }
    catch (...)
    {
        if (!TableViewDetails::IsAppDerivedColumn(*this))
        {
            throw;
        }
        // An app override that throws is a failed commit: the edit stays open.
        TVDiag::LogRetailF(L"[TableView] CommitCellEditCore threw (HRESULT 0x%08X); the edit stays open.",
            static_cast<unsigned int>(winrt::to_hresult()));
        return false;
    }
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

uint32_t TableViewColumn::AutomationIdentity()
{
    if (m_automationIdentity == 0)
    {
        m_automationIdentity = s_nextAutomationIdentity.fetch_add(1);
    }
    return m_automationIdentity;
}

void TableViewColumn::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto property = args.Property();

    if (property == s_WidthProperty ||
        property == s_MinWidthProperty ||
        property == s_MaxWidthProperty)
    {
        // Only an app-driven assignment replaces the authored width; a user resize does not.
        if (property == s_WidthProperty && !m_inUserResize)
        {
            m_authoredWidth = ValueHelper<winrt::GridLength>::CastOrUnbox(args.NewValue());
        }

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
        // Realized editors were generated from the previous template. An editor does not change
        // what the cell displays, so the Auto width keeps its history.
        NotifyCellContentChangedInternal(false /* resetAutoWidth */);
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
    else if (property == s_HeaderToolTipProperty)
    {
        // Not the header branch above: that recomputes Auto width, which a tooltip cannot change.
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnHeaderToolTipChanged(*this);
        }
    }
    else if (property == s_HeaderStyleProperty)
    {
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->QueueRebuildHeaders();
        }
    }
    else if (property == s_FrozenEdgeProperty)
    {
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnFrozenEdgeChanged(*this);
        }
    }
    else if (property == s_CanUserResizeProperty)
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
    else if (property == s_CanUserSortProperty)
    {
        // The sort affordance is stamped at header-build time, so opting a column in or out at
        // runtime has to rebuild the headers for the chevron to appear or disappear.
        if (auto owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnColumnCanUserSortChanged(*this);
        }
    }
    else if (property == s_SortDirectionProperty)
    {
        if (auto owner = GetOwningTableView())
        {
            auto const ownerImpl = winrt::get_self<TableView>(owner);
            if (!m_isSettingSortStateInternal)
            {
                ownerImpl->OnColumnSortDirectionSetByApp(*this, SortDirection());
            }
            ownerImpl->RefreshSortIndicators();
            ownerImpl->RaiseColumnSortStateAutomationChanged(
                *this, winrt::unbox_value_or<winrt::SortDirection>(args.OldValue(), winrt::SortDirection::None));
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
    // A direction the app set, or the app's own source sort, is not this table's to re-sort.
    if (auto owner = GetOwningTableView(); owner && SortDirection() != winrt::SortDirection::None &&
        !winrt::get_self<TableView>(owner)->IsSortLitOnlyByAppInternal())
    {
        // One request, no intermediate clear: a comparer that throws while ranking leaves the
        // previous order and arrow in place.
        winrt::get_self<TableView>(owner)->ResortColumnInternal(*this);
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

    // The control's own write must not read as the app setting the displayed direction.
    m_isSettingSortStateInternal = true;
    auto const resetGuard = gsl::finally([this]() { m_isSettingSortStateInternal = false; });
    SetValue(s_SortDirectionProperty, winrt::box_value(direction));
}

// Lets a derived column refresh its realized cells when one of ITS OWN properties changes.
//
// The base deliberately does not enumerate derived types' dependency properties: a third-party
// column would then have no way to invalidate its cells without editing this file. This mirrors
// WPF, where DataGridColumn owns the notification and each derived column routes its own property
// callbacks into it.
void TableViewColumn::NotifyCellContentChanged()
{
    NotifyCellContentChangedInternal(true /* resetAutoWidth */);
}

void TableViewColumn::NotifyCellContentChangedInternal(bool resetAutoWidth)
{
    if (auto owner = GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->OnColumnCellTemplateChanged(*this, resetAutoWidth);
    }
}

winrt::Microsoft::UI::Xaml::Data::Binding TableViewColumn::CellToolTipBinding()
{
    return m_cellToolTipBinding.get();
}

void TableViewColumn::CellToolTipBinding(const winrt::Microsoft::UI::Xaml::Data::Binding& value)
{
    m_cellToolTipBinding.set(value);

    // Realized cells carry the previous binding. A tooltip does not change what the cell displays,
    // so the Auto width keeps its history.
    NotifyCellContentChangedInternal(false /* resetAutoWidth */);
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

void TableViewColumn::SetDesiredWidthInternal(double rowsDesiredWidth, double headerDesiredWidth)
{
    m_rowsDesiredWidth = std::max(0.0, rowsDesiredWidth);
    m_headerDesiredWidth = std::max(0.0, headerDesiredWidth);
}

void TableViewColumn::ResetDesiredWidthInternal()
{
    m_rowsDesiredWidth = 0.0;
    m_headerDesiredWidth = 0.0;
}

void TableViewColumn::UpdateActualWidth()
{
    // Pixel/Auto resolve to a fixed width here; Star gets a provisional default and is later
    // resolved against the viewport by the owning TableView (TableView::ResolveColumnWidths).
    const auto width = Width();
    const double widthPixels =
        width.GridUnitType == winrt::GridUnitType::Pixel
            ? NonNegativeFiniteOrDefault(width.Value)
            : c_widthDefault.Value;

    // Reject malformed widths and bounds the same way layout does, so provisional ActualWidth and
    // resolved layout widths cannot drift.
    const double lo = NonNegativeFiniteOrZero(MinWidth());
    const double hi = MaxWidthForColumn(*this, lo);
    const double clamped = std::clamp(widthPixels, lo, hi);

    // ActualWidth uses the SetValue-via-key read-only DP convention.
    if (std::abs(clamped - ActualWidth()) > 0.0001)
    {
        SetValue(s_ActualWidthProperty, winrt::box_value(clamped));
    }
}
