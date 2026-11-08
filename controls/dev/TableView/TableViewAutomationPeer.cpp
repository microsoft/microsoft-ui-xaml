// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewGroupHeader.h"
#include "TableViewAutomationPeer.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewRowAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "GroupedEntry.h"
#include "TableViewAutomationPeer.properties.cpp"

#include <algorithm>

TableViewAutomationPeer::TableViewAutomationPeer(winrt::TableView const& owner)
    : ReferenceTracker(owner)
{
}

namespace
{
    // Marked composable like the IDL-generated peer bases, which ReferenceTracker relies on.
    template <typename D, typename... I>
    using ComposableAutomationPeerT = winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationPeerT<D, winrt::composable, I...>;

    // A realized row's name as composed from its text cells, read from the item without realizing the row.
    winrt::hstring ComposeTextColumnValues(TableView& table, winrt::IInspectable const& item)
    {
        std::wstring composed;
        auto const separator = LocalizedOrFallbackForTableViewAutomation(SR_TableViewCellTextSeparator, L", ");
        winrt::TextBlock probe{ nullptr };
        for (auto const& column : table.Columns())
        {
            auto const textColumn = IsVisibleColumn(column) ? column.try_as<winrt::TableViewTextColumn>() : nullptr;
            auto const binding = textColumn ? textColumn.Binding() : nullptr;
            // A detached probe cannot resolve element-relative sources.
            if (!binding || !binding.ElementName().empty() || binding.RelativeSource())
            {
                continue;
            }
            if (!probe)
            {
                probe = winrt::TextBlock{};
                // Converters and StringFormat format for the target's language, which a cell inherits.
                probe.Language(table.Language());
            }
            winrt::hstring text;
            try
            {
                winrt::BindingOperations::SetBinding(probe, winrt::TextBlock::TextProperty(), binding);
                probe.DataContext(item);
                text = probe.Text();
                probe.ClearValue(winrt::TextBlock::TextProperty());
                probe.DataContext(nullptr);
            }
            catch (winrt::hresult_error const&)
            {
                // An app converter that throws only costs that column's text.
            }
            if (text.empty())
            {
                continue;
            }
            if (!composed.empty())
            {
                composed += separator.c_str();
            }
            composed += text;
        }
        return winrt::hstring{ composed };
    }

    // A container-free data-item peer for a selected, unrealized row. Reads never realize it; once
    // realized it forwards to the row's peer but keeps its own RuntimeId.
    // A bound peer is instead the UIA element of one binding of a row to an item (the row peer's
    // EventsSource); once the row is cleared or rebound, every member throws ElementNotAvailable.
    class TableViewRowItemAutomationPeer :
        public ReferenceTracker<
            TableViewRowItemAutomationPeer,
            ComposableAutomationPeerT,
            winrt::ISelectionItemProvider,
            winrt::IVirtualizedItemProvider,
            winrt::IScrollItemProvider,
            winrt::IExpandCollapseProvider>
    {
    public:
        TableViewRowItemAutomationPeer(winrt::TableView const& table, winrt::IInspectable const& item, int32_t index, bool byIndex)
            : m_table(winrt::make_weak(table))
            // Boxed values have no stable identity: such a peer tracks its index while that row stays selected.
            , m_byIndex(byIndex)
            , m_lastIndex(index)
        {
            m_item.Track(item);
        }

        // Weak references only: this peer is not a FrameworkElementAutomationPeer, so no core pointer outlives the row.
        TableViewRowItemAutomationPeer(winrt::TableView const& table, winrt::TableViewRow const& row)
            : m_table(winrt::make_weak(table))
            , m_row(winrt::make_weak(row))
            , m_bound(true)
        {
        }

        int32_t IndexInternal() { return ItemIndex(); }

        // Rebinding to the selected index picks the right occurrence of a duplicated item.
        bool TryRebind(winrt::IInspectable const& item, int32_t index)
        {
            if (m_byIndex ? m_lastIndex != index : !m_item.SameIdentityAs(item))
            {
                return false;
            }
            m_lastIndex = index;
            return true;
        }

        winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface)
        {
            const int32_t index = Index();
            auto const rowPeer = RealizedRowPeer(index);
            // Served here so a cached pattern cannot act on the row's next item.
            if (patternInterface == winrt::PatternInterface::ExpandCollapse)
            {
                if (rowPeer && rowPeer.GetPattern(patternInterface))
                {
                    return *this;
                }
                return nullptr;
            }
            if (patternInterface == winrt::PatternInterface::SelectionItem ||
                patternInterface == winrt::PatternInterface::VirtualizedItem)
            {
                const bool offered = patternInterface == winrt::PatternInterface::SelectionItem
                    ? index >= 0 && winrt::get_self<TableView>(m_table.get())->CanSelectRows()
                    : index >= 0 && !rowPeer;
                if (offered)
                {
                    return *this;
                }
                return nullptr;
            }
            // The row peer's ScrollItem is a framework default that GetPattern does not return.
            if (patternInterface == winrt::PatternInterface::ScrollItem)
            {
                if (rowPeer)
                {
                    return *this;
                }
                return nullptr;
            }
            return rowPeer ? rowPeer.GetPattern(patternInterface) : __super::GetPatternCore(patternInterface);
        }

        winrt::IInspectable NavigateCore(winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection const& direction)
        {
            using Direction = winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection;
            // The framework's tree holds the row's own peer, not this one; children are the cells.
            if (m_bound && direction != Direction::FirstChild && direction != Direction::LastChild)
            {
                auto const rowPeer = RealizedRowPeer(RequireIndex());
                return rowPeer ? rowPeer.Navigate(direction) : nullptr;
            }
            if (direction == winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::Parent)
            {
                return RefreshParent();
            }
            return __super::NavigateCore(direction);
        }

        // Parented where realized rows are; re-resolved on use, as the template may have been reapplied.
        winrt::AutomationPeer RefreshParent()
        {
            auto const table = m_table.get();
            if (!table)
            {
                return nullptr;
            }
            winrt::AutomationPeer parent = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(table);
            if (auto const repeater = winrt::get_self<TableView>(table)->GetRowsRepeaterInternal();
                repeater && winrt::VisualTreeHelper::GetChildrenCount(repeater) > 0)
            {
                if (auto const rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(
                        winrt::VisualTreeHelper::GetChild(repeater, 0).try_as<winrt::UIElement>()))
                {
                    if (auto const rowParent = rowPeer.Navigate(
                            winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::Parent).try_as<winrt::AutomationPeer>())
                    {
                        parent = rowParent;
                    }
                }
            }
            SetParent(parent);
            return parent;
        }

        hstring GetClassNameCore() { return winrt::hstring_name_of<winrt::TableViewRow>(); }
        winrt::AutomationControlType GetAutomationControlTypeCore() { return winrt::AutomationControlType::DataItem; }
        bool IsContentElementCore() { return true; }
        bool IsControlElementCore() { return true; }

        hstring GetNameCore()
        {
            const int32_t index = RequireIndex();
            // A realized row composes its name from its cells, so it can differ from ItemToName.
            if (auto const rowPeer = RealizedRowPeer(index))
            {
                return rowPeer.GetName();
            }
            // Read from the source: m_item may be a weak reference to a box nothing else keeps alive.
            auto const tableImpl = winrt::get_self<TableView>(m_table.get());
            auto const item = tableImpl->UnwrapEditingDataItem(tableImpl->GetRowsRepeaterInternal().ItemsSourceView().GetAt(index));
            if (auto const composed = ComposeTextColumnValues(*tableImpl, item); !composed.empty())
            {
                return composed;
            }
            return ItemToName(item);
        }

        hstring GetAutomationIdCore() { auto const p = RealizedRowPeer(Index()); return p ? p.GetAutomationId() : hstring{}; }
        hstring GetHelpTextCore() { auto const p = RealizedRowPeer(Index()); return p ? p.GetHelpText() : hstring{}; }

        void ShowContextMenuCore()
        {
            if (auto const p = RealizedRowPeer(Index()))
            {
                p.ShowContextMenu();
            }
        }

        bool IsEnabledCore()
        {
            if (auto const p = RealizedRowPeer(Index()))
            {
                return p.IsEnabled();
            }
            auto const table = m_table.get();
            return !table || table.IsEnabled();
        }

        int32_t GetCultureCore()
        {
            winrt::AutomationPeer p = RealizedRowPeer(Index());
            if (!p)
            {
                if (auto const table = m_table.get())
                {
                    p = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(table);
                }
            }
            return p ? p.GetCulture() : __super::GetCultureCore();
        }

        bool IsOffscreenCore() { auto const p = RealizedRowPeer(Index()); return !p || p.IsOffscreen(); }
        winrt::Rect GetBoundingRectangleCore() { auto const p = RealizedRowPeer(Index()); return p ? p.GetBoundingRectangle() : winrt::Rect{}; }
        // A selected-item peer does not forward: the framework re-parents returned children, which would
        // steal the row's cell peers. A bound peer is the row to UIA, so the cells belong to it.
        winrt::IVector<winrt::AutomationPeer> GetChildrenCore()
        {
            auto const p = m_bound ? RealizedRowPeer(RequireIndex()) : nullptr;
            return p ? p.GetChildren() : nullptr;
        }
        winrt::Point GetClickablePointCore() { auto const p = RealizedRowPeer(Index()); return p ? p.GetClickablePoint() : winrt::Point{}; }
        bool HasKeyboardFocusCore() { auto const p = RealizedRowPeer(Index()); return p && p.HasKeyboardFocus(); }
        bool IsKeyboardFocusableCore() { auto const p = RealizedRowPeer(Index()); return p && p.IsKeyboardFocusable(); }
        void SetFocusCore() { if (auto const p = RealizedRowPeer(Index())) { p.SetFocus(); } }

        bool IsSelected()
        {
            if (m_bound)
            {
                return LiveRowPeer().IsSelected();
            }
            // Resolved first: it throws when the table is gone, before the table is dereferenced.
            const int32_t index = RequireIndex();
            return winrt::get_self<TableView>(m_table.get())->SelectedIndexInternal() == index;
        }

        winrt::IRawElementProviderSimple SelectionContainer()
        {
            RequireIndex();
            auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(m_table.get());
            return peer ? ProviderFromPeer(peer) : nullptr;
        }

        void AddToSelection()
        {
            if (m_bound)
            {
                LiveRowPeer().AddToSelection();
                return;
            }
            ApplySelectionItemAction(TableViewRowAutomationPeer::SelectionItemAction::Add);
        }

        void RemoveFromSelection()
        {
            if (m_bound)
            {
                LiveRowPeer().RemoveFromSelection();
                return;
            }
            ApplySelectionItemAction(TableViewRowAutomationPeer::SelectionItemAction::Remove);
        }

        void Select()
        {
            if (m_bound)
            {
                LiveRowPeer().Select();
                return;
            }
            ApplySelectionItemAction(TableViewRowAutomationPeer::SelectionItemAction::Select);
        }

        winrt::ExpandCollapseState ExpandCollapseState() { return LiveRowPeer().ExpandCollapseState(); }
        void Expand() { LiveRowPeer().Expand(); }
        void Collapse() { LiveRowPeer().Collapse(); }

        void Realize()
        {
            // An ElementPrepared handler can rebind this peer to another occurrence while the row is created.
            const int32_t index = RequireIndex();
            TableViewRowAutomationPeer::RealizeRowAt(m_table.get(), index,
                [this, index](winrt::TableViewRow const& row) { return IsItemRow(row) && ItemIndex() == index; });
        }

        void ScrollIntoView()
        {
            auto const row = RealizedRow(RequireIndex());
            if (!row)
            {
                ThrowElementNotAvailable();
            }
            winrt::BringIntoViewOptions options;
            options.AnimationDesired(false);
            row.StartBringIntoView(options);
        }

    private:
        void ApplySelectionItemAction(TableViewRowAutomationPeer::SelectionItemAction action)
        {
            // Enabled first: a realized row's app-derived peer can change the rows the index is resolved against.
            TableViewRowAutomationPeer::ThrowIfSelectionItemNotEnabled(IsEnabledCore());
            const int32_t index = RequireIndex();
            TableViewRowAutomationPeer::ApplySelectionItemActionAt(action, m_table.get(), index);
        }

        // -1 once the item left the source (or a boxed value's row was deselected); the table is alive when >= 0.
        int32_t ItemIndex()
        {
            auto const table = m_table.get();
            auto const tableImpl = table ? winrt::get_self<TableView>(table) : nullptr;
            auto const repeater = tableImpl ? tableImpl->GetRowsRepeaterInternal() : nullptr;
            auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
            if (!tableImpl || !view)
            {
                return -1;
            }

            if (m_bound)
            {
                auto const row = m_row.get();
                return row && IsBound(row) ? repeater.GetElementIndex(row) : -1;
            }

            const int32_t count = view.Count();
            const int32_t selectedIndex = tableImpl->SelectedIndexInternal();
            if (m_byIndex)
            {
                return m_lastIndex < count && m_lastIndex == selectedIndex ? m_lastIndex : -1;
            }

            auto const matches = [&](int32_t index)
            {
                return index >= 0 && index < count &&
                    m_item.SameIdentityAs(tableImpl->UnwrapEditingDataItem(view.GetAt(index)));
            };
            // The selected index first: after an insert, m_lastIndex can hold another occurrence of the item.
            for (const int32_t hint : { selectedIndex, m_lastIndex })
            {
                if (matches(hint))
                {
                    return m_lastIndex = hint;
                }
            }
            for (int32_t index = 0; index < count; ++index)
            {
                if (matches(index))
                {
                    return m_lastIndex = index;
                }
            }
            return -1;
        }

        int32_t RequireIndex()
        {
            const int32_t index = ItemIndex();
            if (index < 0)
            {
                ThrowElementNotAvailable();
            }
            return index;
        }

        // Stale bound peers throw from every member; a selected-item peer degrades instead.
        int32_t Index() { return m_bound ? RequireIndex() : ItemIndex(); }

        // The binding is current while the row's own peer still routes UIA to this peer.
        bool IsBound(winrt::TableViewRow const& row)
        {
            auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(row);
            auto const source = peer ? peer.EventsSource().try_as<winrt::IVirtualizedItemProvider>() : nullptr;
            return source && winrt::get_self<TableViewRowItemAutomationPeer>(source) == this;
        }

        TableViewRowAutomationPeer& LiveRowPeer()
        {
            auto const peer = RealizedRowPeer(RequireIndex()).try_as<winrt::TableViewRowAutomationPeer>();
            if (!peer)
            {
                ThrowElementNotAvailable();
            }
            return *winrt::get_self<TableViewRowAutomationPeer>(peer);
        }

        bool IsItemRow(winrt::TableViewRow const& row)
        {
            if (m_bound)
            {
                return row == m_row.get() && IsBound(row);
            }
            return m_byIndex ||
                m_item.SameIdentityAs(winrt::get_self<TableView>(m_table.get())->UnwrapEditingDataItem(row.DataContext()));
        }

        winrt::TableViewRow RealizedRow(int32_t index)
        {
            if (index < 0)
            {
                return nullptr;
            }
            auto const row = winrt::get_self<TableView>(m_table.get())->GetRowsRepeaterInternal()
                .TryGetElement(index).try_as<winrt::TableViewRow>();
            return row && IsItemRow(row) ? row : nullptr;
        }

        winrt::AutomationPeer RealizedRowPeer(int32_t index)
        {
            auto const row = RealizedRow(index);
            return row ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(row) : nullptr;
        }

        winrt::weak_ref<winrt::TableView> m_table{ nullptr };
        winrt::weak_ref<winrt::TableViewRow> m_row{ nullptr };
        bool m_bound{ false };
        TableViewTrackedItemIdentity m_item;
        bool m_byIndex{ false };
        int32_t m_lastIndex{ -1 };
    };

    // What UIA sees for a repeater element: a row's current item peer, else the element's own peer.
    winrt::AutomationPeer ElementItemPeer(winrt::UIElement const& element)
    {
        auto const peer = element ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(element) : nullptr;
        auto const source = peer ? peer.EventsSource() : nullptr;
        return source ? source : peer;
    }
}
template<typename TChildren>
static int32_t VisibleColumnToChildIndex(TChildren const& children, int32_t visibleColumnIndex)
{
    int32_t currentVisibleIndex = 0;
    const auto count = children.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const cellElement = children.GetAt(i).try_as<winrt::FrameworkElement>();
        winrt::TableViewColumn column{ nullptr };
        if (cellElement)
        {
            column = cellElement.Tag().try_as<winrt::TableViewColumn>();
        }
        if (!IsVisibleColumn(column))
        {
            continue;
        }

        if (currentVisibleIndex == visibleColumnIndex)
        {
            return static_cast<int32_t>(i);
        }
        ++currentVisibleIndex;
    }
    return -1;
}

// The body ScrollViewer's percent is computed against the repeater's estimated extent, which grows as
// rows near the end are realized, so a single SetScrollPercent(100) could land short. The peer itself is
// the Scroll provider, so every lookup returns the same object and every request goes through it.
winrt::IScrollProvider TableViewAutomationPeer::GetBodyScrollProvider()
{
    if (auto const impl = GetImpl())
    {
        if (auto const bodyScroller = impl->GetBodyScrollerInternal())
        {
            if (auto const svPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(bodyScroller))
            {
                return svPeer.GetPattern(winrt::PatternInterface::Scroll).try_as<winrt::IScrollProvider>();
            }
        }
    }
    return nullptr;
}

winrt::IScrollProvider TableViewAutomationPeer::BodyScrollProvider()
{
    if (auto const provider = GetBodyScrollProvider())
    {
        return provider;
    }
    throw winrt::hresult_error(UIA_E_ELEMENTNOTAVAILABLE);
}

bool TableViewAutomationPeer::HorizontallyScrollable() { return BodyScrollProvider().HorizontallyScrollable(); }
double TableViewAutomationPeer::HorizontalScrollPercent() { return BodyScrollProvider().HorizontalScrollPercent(); }
double TableViewAutomationPeer::HorizontalViewSize() { return BodyScrollProvider().HorizontalViewSize(); }
bool TableViewAutomationPeer::VerticallyScrollable() { return BodyScrollProvider().VerticallyScrollable(); }
double TableViewAutomationPeer::VerticalViewSize() { return BodyScrollProvider().VerticalViewSize(); }

void TableViewAutomationPeer::Scroll(winrt::ScrollAmount const& horizontalAmount, winrt::ScrollAmount const& verticalAmount)
{
    ++m_scrollRequest;
    BodyScrollProvider().Scroll(horizontalAmount, verticalAmount);
}

double TableViewAutomationPeer::VerticalScrollPercent()
{
    const double percent = BodyScrollProvider().VerticalScrollPercent();
    return percent != winrt::ScrollPatternIdentifiers::NoScroll() && IsLastRowAtBottom() ? 100.0 : percent;
}

void TableViewAutomationPeer::SetScrollPercent(double horizontalPercent, double verticalPercent)
{
    auto const inner = BodyScrollProvider();
    auto const impl = GetImpl();
    auto const scroller = impl ? impl->GetBodyScrollerInternal() : nullptr;
    // A request made during one of the layout passes below (by any caller) supersedes this one.
    auto const request = ++m_scrollRequest;
    if (!scroller || verticalPercent == winrt::ScrollPatternIdentifiers::NoScroll())
    {
        inner.SetScrollPercent(horizontalPercent, verticalPercent);
        return;
    }

    const double oldExtent = scroller.ExtentHeight();
    const bool toEnd = verticalPercent >= 100.0 - 1e-9;
    // The default element anchoring pulls the offset back whenever a re-measured row above the viewport
    // shifts the anchor, so aiming for the end could stall short of it. Anchor at the far edge meanwhile.
    auto const anchorRatioProperty = winrt::ScrollViewer::VerticalAnchorRatioProperty();
    auto const localAnchorRatio = scroller.ReadLocalValue(anchorRatioProperty);
    if (toEnd)
    {
        scroller.VerticalAnchorRatio(1.0);
    }
    auto restoreAnchorRatio = gsl::finally([&]()
    {
        if (!toEnd)
        {
            return;
        }
        if (localAnchorRatio == winrt::DependencyProperty::UnsetValue())
        {
            scroller.ClearValue(anchorRatioProperty);
        }
        else
        {
            scroller.SetValue(anchorRatioProperty, localAnchorRatio);
        }
    });
    inner.SetScrollPercent(horizontalPercent, verticalPercent);
    scroller.UpdateLayout();
    if (toEnd)
    {
        // Each pass realizes rows nearer the end, so the extent converges on the true one; keep going
        // while a pass still moves it. The cap only guards against a layout that never settles.
        double lastExtent = -1.0;
        double lastOffset = -1.0;
        for (int attempt = 0; attempt < 32 && request == m_scrollRequest && !IsLastRowAtBottom(); ++attempt)
        {
            const double extent = scroller.ExtentHeight();
            const double offset = scroller.VerticalOffset();
            if (!inner.VerticallyScrollable() || (extent == lastExtent && offset == lastOffset))
            {
                break;
            }
            lastExtent = extent;
            lastOffset = offset;
            inner.SetScrollPercent(winrt::ScrollPatternIdentifiers::NoScroll(), 100.0);
            scroller.UpdateLayout();
        }
    }
    else if (request == m_scrollRequest && std::abs(scroller.ExtentHeight() - oldExtent) >= 0.5)
    {
        inner.SetScrollPercent(winrt::ScrollPatternIdentifiers::NoScroll(), verticalPercent);
        scroller.UpdateLayout();
    }
}

bool TableViewAutomationPeer::IsLastRowAtBottom()
{
    auto const impl = GetImpl();
    auto const scroller = impl ? impl->GetBodyScrollerInternal() : nullptr;
    auto const repeater = impl ? impl->GetRowsRepeaterInternal() : nullptr;
    auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
    if (!scroller || !view || view.Count() == 0 || !(scroller.ViewportHeight() > 0.0))
    {
        return false;
    }

    auto const last = repeater.TryGetElement(view.Count() - 1).try_as<winrt::FrameworkElement>();
    if (!last || !(last.ActualHeight() > 0.0))
    {
        return false;
    }
    try
    {
        // Under half the range, so a sub-pixel range (e.g. 1px at 200%) never reads as both top and end.
        const double range = scroller.ScrollableHeight();
        const double tolerance = (std::min)(0.5, range / 2.0);
        const double bottom = last.TransformToVisual(scroller).TransformPoint({ 0.0f, 0.0f }).Y + last.ActualHeight();
        return range > 0.0 &&
            std::abs(bottom - scroller.ViewportHeight()) <= tolerance &&
            range - scroller.VerticalOffset() <= tolerance;
    }
    catch (winrt::hresult_error const&)
    {
        return false;
    }
}

winrt::IInspectable TableViewAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (Owner().try_as<winrt::TableView>())
    {
        // Grid + Table are always advertised — they describe shape, not state.
        if (patternInterface == winrt::PatternInterface::Grid ||
            patternInterface == winrt::PatternInterface::Table)
        {
            return *this;
        }

        // ItemContainer lets AT clients enumerate beyond realized rows.
        if (patternInterface == winrt::PatternInterface::ItemContainer)
        {
            return *this;
        }

        // Selection is advertised only when the control can actually select. Advertising it while
        // SelectionMode is None would tell an AT client the grid is selectable when every Select()
        // it issues would be refused.
        if (patternInterface == winrt::PatternInterface::Selection)
        {
            if (auto const impl = GetImpl(); impl && impl->CanSelectRows())
            {
                return *this;
            }
        }

        if (patternInterface == winrt::PatternInterface::Scroll && GetBodyScrollProvider())
        {
            return *this;
        }
    }

    return __super::GetPatternCore(patternInterface);
}

hstring TableViewAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TableView>();
}

winrt::AutomationControlType TableViewAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::DataGrid;
}

// The default bounds include cache rows the scroller clips; best-effort clip to the layout box.
winrt::Rect TableViewAutomationPeer::GetBoundingRectangleCore()
{
    auto const bounds = __super::GetBoundingRectangleCore();
    if (bounds.Width <= 0.0f || bounds.Height <= 0.0f)
    {
        return bounds;
    }

    auto const owner = Owner().try_as<winrt::FrameworkElement>();
    if (!owner || !owner.XamlRoot())
    {
        return bounds;
    }

    const winrt::Rect localBounds{
        0.0f,
        0.0f,
        static_cast<float>(owner.ActualWidth()),
        static_cast<float>(owner.ActualHeight()) };
    if (localBounds.Width <= 0.0f || localBounds.Height <= 0.0f)
    {
        return bounds;
    }

    winrt::Rect layoutBounds{};
    try
    {
        layoutBounds = SharedHelpers::ConvertDipsToPhysical(
            owner, owner.TransformToVisual(nullptr).TransformBounds(localBounds));
    }
    catch (winrt::hresult_error const&)
    {
        // TransformToVisual throws while the owner is leaving the tree (teardown, window close).
        return bounds;
    }

    // Intersect reports no overlap as RectHelper::Empty() (infinite extents); never return that.
    auto const clipped = winrt::RectHelper::Intersect(bounds, layoutBounds);
    if (!(clipped.Width > 0.0f) || !(clipped.Height > 0.0f) ||
        !std::isfinite(clipped.X) || !std::isfinite(clipped.Y) ||
        !std::isfinite(clipped.Width) || !std::isfinite(clipped.Height))
    {
        return bounds;
    }

    return clipped;
}

void TableViewAutomationPeer::ShowContextMenuCore()
{
    // The base raises ContextRequested on the TableView itself, which shows its own
    // ContextFlyout before the template-root suppression hook can see it.
    if (auto const impl = GetImpl(); impl && impl->AreContextMenusSuppressed())
    {
        return;
    }

    __super::ShowContextMenuCore();
}

void TableViewAutomationPeer::RaiseStructureChangedForSortChange()
{
    // Same children, new order.
    RaiseStructureChanged(winrt::AutomationStructureChangeType::ChildrenReordered);
}

void TableViewAutomationPeer::RaiseStructureChangedForVirtualizationReset()
{
    // Membership or bucketing changed, so any cached child could be gone.
    RaiseStructureChanged(winrt::AutomationStructureChangeType::ChildrenInvalidated);
}

void TableViewAutomationPeer::RaiseStructureChangedForGroupExpansion()
{
    RaiseStructureChanged(winrt::AutomationStructureChangeType::ChildrenInvalidated);
}

void TableViewAutomationPeer::RaiseStructureChangedForColumnsChange()
{
    // A column isn't a child of this peer, so ChildAdded/ChildRemoved don't apply.
    RaiseStructureChanged(winrt::AutomationStructureChangeType::ChildrenInvalidated);
}

void TableViewAutomationPeer::RaiseStructureChanged(winrt::AutomationStructureChangeType const& structureChangeType)
{
    RaiseStructureChangedEvent(structureChangeType, nullptr);
}

com_ptr<TableView> TableViewAutomationPeer::GetImpl()
{
    com_ptr<TableView> impl = nullptr;

    if (auto const tableView = Owner().try_as<winrt::TableView>())
    {
        impl = winrt::get_self<TableView>(tableView)->get_strong();
    }

    return impl;
}

int32_t TableViewAutomationPeer::RowCount()
{
    if (auto const impl = GetImpl())
    {
        return impl->GetRowCountInternal();
    }
    return 0;
}

int32_t TableViewAutomationPeer::ColumnCount()
{
    if (auto const tableView = Owner().try_as<winrt::TableView>())
    {
        if (auto const cols = tableView.Columns())
        {
            return CountVisibleColumns(cols);
        }
    }
    return 0;
}

winrt::IRawElementProviderSimple TableViewAutomationPeer::GetItem(int32_t row, int32_t column)
{
    if (row < 0 || column < 0)
    {
        return nullptr;
    }

    auto const impl = GetImpl();
    if (!impl)
    {
        return nullptr;
    }

    if (row >= impl->GetRowCountInternal() || column >= ColumnCount())
    {
        return nullptr;
    }

    auto repeater = impl->GetRowsRepeaterInternal();
    if (!repeater)
    {
        return nullptr;
    }

    winrt::UIElement element{ nullptr };
    try
    {
        element = repeater.TryGetElement(row);
    }
    catch (...)
    {
        return nullptr;
    }

    if (!element)
    {
        // Bounded realization — realize ONLY the single requested row (not a scan)
        // so IGridProvider.GetItem honors the UIA grid contract (Narrator cell addressing)
        // without force-realizing the whole dataset.
        // Full sweep-virtualization would require exposing VirtualizedItemPattern.
        try
        {
            element = repeater.GetOrCreateElement(row);
        }
        catch (...)
        {
            return nullptr;
        }
    }

    if (!element)
    {
        return nullptr;
    }

    if (auto const header = element.try_as<winrt::TableViewGroupHeader>())
    {
        if (auto const headerPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(header))
        {
            return ProviderFromPeer(headerPeer);
        }
        return nullptr;
    }

    auto const rowElement = element.try_as<winrt::TableViewRow>();
    if (!rowElement)
    {
        return nullptr;
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(rowElement);
    if (!rowImpl)
    {
        return nullptr;
    }

    auto cellsHost = rowImpl->GetCellsHostPanelInternal();
    if (!cellsHost)
    {
        return nullptr;
    }

    auto const cellChildren = cellsHost.Children();
    const int32_t childIndex = VisibleColumnToChildIndex(cellChildren, column);
    if (childIndex < 0)
    {
        return nullptr;
    }

    auto cellElement = cellChildren.GetAt(childIndex).try_as<winrt::UIElement>();
    if (!cellElement)
    {
        return nullptr;
    }

    if (auto const cellFE = cellElement.try_as<winrt::FrameworkElement>())
    {
        // Route through the row peer so Grid.GetItem and tree navigation share provider identity.
        if (auto const rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement)
                .try_as<winrt::TableViewRowAutomationPeer>())
        {
            if (auto const cellPeer = winrt::get_self<TableViewRowAutomationPeer>(rowPeer)
                    ->GetOrCreateCellPeer(cellFE))
            {
                return ProviderFromPeer(cellPeer);
            }
        }

        // A fresh peer would not match editing/tree navigation; custom row peers may expose no item.
    }
    return nullptr;
}

winrt::RowOrColumnMajor TableViewAutomationPeer::RowOrColumnMajor()
{
    // Tabular data flows row-by-row across columns — narrators reading
    // sequentially consume rows in order, so RowMajor matches the visual model.
    return winrt::RowOrColumnMajor::RowMajor;
}

winrt::com_array<winrt::IRawElementProviderSimple> TableViewAutomationPeer::GetRowHeaders()
{
    // TableView has no row-header element; data rows are reached through Grid.GetItem.
    return {};
}

// ----- ISelectionProvider -----

winrt::com_array<winrt::IRawElementProviderSimple> TableViewAutomationPeer::GetSelection()
{
    auto const impl = GetImpl();
    if (!impl || !impl->CanSelectRows())
    {
        m_selectedItemPeerCache.clear();
        return {};
    }

    const int32_t selectedIndex = impl->SelectedIndexInternal();
    if (selectedIndex < 0)
    {
        m_selectedItemPeerCache.clear();
        return {};
    }

    // A UIA read must not realize the row: it would become the repeater's anchor and move the viewport.
    std::vector<winrt::IRawElementProviderSimple> selection;
    std::vector<tracker_ref<winrt::AutomationPeer>> liveCache;
    if (auto const repeater = impl->GetRowsRepeaterInternal())
    {
        winrt::AutomationPeer peer{ nullptr };
        winrt::IInspectable item{ nullptr };
        if (auto const view = repeater.ItemsSourceView(); view && selectedIndex < view.Count())
        {
            item = impl->UnwrapEditingDataItem(view.GetAt(selectedIndex));
        }
        auto const rowElement = repeater.TryGetElement(selectedIndex);
        if (rowElement)
        {
            peer = ElementItemPeer(rowElement);
        }

        // Kept while its row is realized too, so a held item keeps one RuntimeId as it scrolls in and out.
        winrt::AutomationPeer itemPeer{ nullptr };
        if (item)
        {
            itemPeer = GetOrCreateSelectedItemPeer(*impl, item, selectedIndex, !rowElement /* create */);
        }
        if (itemPeer)
        {
            liveCache.emplace_back(this, itemPeer);
            if (!peer)
            {
                peer = itemPeer;
            }
        }

        // An empty selection is a valid answer; an array holding a null provider is not.
        if (peer)
        {
            if (auto const provider = ProviderFromPeer(peer))
            {
                selection.push_back(provider);
            }
        }
    }

    m_selectedItemPeerCache = std::move(liveCache);
    return winrt::com_array(selection);
}

winrt::AutomationPeer TableViewAutomationPeer::GetOrCreateSelectedItemPeer(TableView& table, winrt::IInspectable const& item, int32_t index, bool create)
{
    winrt::AutomationPeer peer{ nullptr };
    for (auto const& cached : m_selectedItemPeerCache)
    {
        // Only TableViewRowItemAutomationPeer instances are cached, so get_self on the provider is valid.
        if (auto const provider = cached.get().try_as<winrt::IVirtualizedItemProvider>();
            provider && winrt::get_self<TableViewRowItemAutomationPeer>(provider)->TryRebind(item, index))
        {
            peer = cached.get();
            break;
        }
    }

    if (!peer)
    {
        if (!create)
        {
            return nullptr;
        }
        auto const repeater = table.GetRowsRepeaterInternal();
        const bool byIndex = !TableView::SameInspectableIdentity(
            item, table.UnwrapEditingDataItem(repeater.ItemsSourceView().GetAt(index)));
        peer = winrt::make_self<TableViewRowItemAutomationPeer>(
            table.get_strong().as<winrt::TableView>(), item, index, byIndex).as<winrt::AutomationPeer>();
    }

    winrt::get_self<TableViewRowItemAutomationPeer>(peer.as<winrt::IVirtualizedItemProvider>())->RefreshParent();
    return peer;
}

winrt::AutomationPeer TableViewAutomationPeer::CreateRowItemPeerInternal(winrt::TableView const& table, winrt::TableViewRow const& row)
{
    return winrt::make_self<TableViewRowItemAutomationPeer>(table, row).as<winrt::AutomationPeer>();
}

bool TableViewAutomationPeer::CanSelectMultiple()
{
    // GetSelection below must return every selected row once this can be true.
    auto const impl = GetImpl();
    return impl && impl->CanSelectMultipleRows();
}

bool TableViewAutomationPeer::IsSelectionRequired()
{
    // Selection can always be empty: TableView never forces a row to stay selected.
    return false;
}

winrt::com_array<winrt::IRawElementProviderSimple> TableViewAutomationPeer::GetColumnHeaders()
{
    std::vector<winrt::IRawElementProviderSimple> headers;
    std::vector<ColumnHeaderPeerCacheEntry> liveCache;

    // Key off visible logical Columns() so headers enumerate before templates realize. Hidden column
    // headers report none, matching each cell's GetColumnHeaderItems.
    if (auto const tableView = Owner().try_as<winrt::TableView>();
        tableView && winrt::get_self<TableView>(tableView)->AreColumnHeadersShownInternal())
    {
        if (auto const columns = tableView.Columns())
        {
            const auto count = columns.Size();
            std::vector<winrt::TableViewColumn> seenColumns;
            headers.reserve(count);
            liveCache.reserve(count);
            seenColumns.reserve(count);
            for (uint32_t i = 0; i < count; i++)
            {
                auto const column = columns.GetAt(i);
                if (!IsVisibleColumn(column))
                {
                    continue;
                }
                if (std::find(seenColumns.begin(), seenColumns.end(), column) != seenColumns.end())
                {
                    continue;
                }
                seenColumns.push_back(column);

                auto headerPeer = GetOrCreateColumnHeaderPeer(tableView, column);
                if (!headerPeer)
                {
                    continue;
                }

                if (auto const headerAutomationPeer = headerPeer.try_as<winrt::TableViewColumnHeaderAutomationPeer>();
                    headerAutomationPeer &&
                    winrt::get_self<TableViewColumnHeaderAutomationPeer>(headerAutomationPeer)->IsTableViewOwned())
                {
                    liveCache.emplace_back(this, column, headerPeer);
                }

                // A provider array must not contain nulls - UIA marshals every element.
                if (auto const provider = ProviderFromPeer(headerPeer))
                {
                    headers.push_back(provider);
                }
            }
        }
    }

    // Replacing the cache wholesale drops peers for columns that are gone or no longer visible.
    m_columnHeaderPeerCache = std::move(liveCache);

    return winrt::com_array(headers);
}

winrt::AutomationPeer TableViewAutomationPeer::GetOrCreateColumnHeaderPeer(
    winrt::TableView const& tableView,
    winrt::TableViewColumn const& column)
{
    if (!tableView || !column ||
        winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != tableView)
    {
        return nullptr;
    }

    if (auto const host = winrt::get_self<TableView>(tableView)->GetHeaderHostInternal())
    {
        if (auto const header = TableViewCellsPanel::CellForColumn(host, column))
        {
            m_columnHeaderPeerCache.erase(
                std::remove_if(
                    m_columnHeaderPeerCache.begin(),
                    m_columnHeaderPeerCache.end(),
                    [column](ColumnHeaderPeerCacheEntry const& entry)
                    {
                        return !entry.peer || !entry.column.get() || entry.column.get() == column;
                    }),
                m_columnHeaderPeerCache.end());

            return winrt::FrameworkElementAutomationPeer::CreatePeerForElement(header);
        }
    }

    for (auto const& entry : m_columnHeaderPeerCache)
    {
        if (entry.peer && entry.column.get() == column)
        {
            return entry.peer.get();
        }
    }

    winrt::AutomationPeer const peer = winrt::make<TableViewColumnHeaderAutomationPeer>(tableView, column);
    peer.SetParent(*this);

    m_columnHeaderPeerCache.erase(
        std::remove_if(
            m_columnHeaderPeerCache.begin(),
            m_columnHeaderPeerCache.end(),
            [](ColumnHeaderPeerCacheEntry const& entry) { return !entry.peer || !entry.column.get(); }),
        m_columnHeaderPeerCache.end());

    m_columnHeaderPeerCache.emplace_back(this, column, peer);
    return peer;
}

static winrt::hstring StringPropertyValue(winrt::IInspectable const& value)
{
    if (auto const propValue = value.try_as<winrt::IPropertyValue>())
    {
        if (propValue.Type() == winrt::PropertyType::String)
        {
            return propValue.GetString();
        }
    }

    return {};
}

static bool TryGetControlTypePropertyValue(winrt::IInspectable const& value, winrt::AutomationControlType& controlType)
{
    if (auto const propValue = value.try_as<winrt::IPropertyValue>())
    {
        if (propValue.Type() == winrt::PropertyType::Int32)
        {
            controlType = static_cast<winrt::AutomationControlType>(propValue.GetInt32());
            return true;
        }
    }

    return false;
}

static bool MatchesItemName(
    TableView& table,
    winrt::IInspectable const& item,
    winrt::UIElement const& element,
    winrt::hstring const& requested)
{
    if (element)
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(element))
        {
            return peer.GetName() == requested;
        }
        return false;
    }
    auto const group = TryGetGroupedEntry(item);
    auto const name = group ? table.GetGroupHeaderNameCandidate(*group) : ItemToName(item);
    return !name.empty() && name == requested;
}

winrt::IRawElementProviderSimple TableViewAutomationPeer::FindItemByProperty(
    winrt::IRawElementProviderSimple const& startAfter,
    winrt::AutomationProperty const& property,
    winrt::IInspectable const& value)
{
    // Name can match virtualized rows from item data; AutomationId/ClassName/ControlType
    // only inspect realized row peers. Full support needs VirtualizedItemPattern.
    if (property == winrt::TogglePatternIdentifiers::ToggleStateProperty())
    {
        return nullptr;
    }

    // Walk the source so AT clients can navigate past realized rows.
    auto const impl = GetImpl();
    if (!impl)
    {
        return nullptr;
    }

    auto repeater = impl->GetRowsRepeaterInternal();
    if (!repeater)
    {
        return nullptr;
    }

    auto sourceView = repeater.ItemsSourceView();
    if (!sourceView)
    {
        return nullptr;
    }

    const int32_t count = sourceView.Count();
    if (count <= 0)
    {
        return nullptr;
    }

    int32_t startIndex = -1;
    if (startAfter)
    {
        // Any repeater child is a valid anchor. Matching only TableViewRow restarted grouping
        // enumeration at item 0, so clients never advanced past the first group header.
        bool resolved = false;
        auto const startPeer = PeerFromProvider(startAfter);
        if (auto const startElementPeer = startPeer.try_as<winrt::FrameworkElementAutomationPeer>())
        {
            if (auto const ownerElement = startElementPeer.Owner().try_as<winrt::UIElement>())
            {
                if (const int32_t index = repeater.GetElementIndex(ownerElement); index >= 0)
                {
                    startIndex = index;
                    resolved = true;
                }
            }
        }
        else if (startPeer)
        {
            // A row's item peer: found through the row still routing UIA to it, so a stale one is refused.
            auto const identity = startPeer.as<winrt::Windows::Foundation::IUnknown>();
            const int32_t children = winrt::VisualTreeHelper::GetChildrenCount(repeater);
            for (int32_t i = 0; i < children && !resolved; ++i)
            {
                auto const element = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::UIElement>();
                if (auto const peer = ElementItemPeer(element); peer && peer.as<winrt::Windows::Foundation::IUnknown>() == identity)
                {
                    startIndex = repeater.GetElementIndex(element);
                    resolved = startIndex >= 0;
                }
            }
        }

        // An unresolvable startAfter - virtualized away, or not one of ours - must not degrade to
        // "start from the beginning": that turns a wrong answer into an enumeration that never ends.
        if (!resolved)
        {
            return nullptr;
        }
    }

    // Null property means "return the next item" per IItemContainerProvider.
    const bool matchAny = !property;

    for (int32_t i = startIndex + 1; i < count; ++i)
    {
        winrt::IInspectable item{ nullptr };
        try
        {
            item = sourceView.GetAt(i);
        }
        catch (...)
        {
            // Source can throw on out-of-bounds during a concurrent edit; bail.
            continue;
        }

        bool isMatch = matchAny;
        if (!matchAny)
        {
            // Realized row peer names override item text for custom AutomationProperties.Name.
            if (property == winrt::AutomationElementIdentifiers::NameProperty())
            {
                isMatch = MatchesItemName(*impl, item, repeater.TryGetElement(i), StringPropertyValue(value));
            }
            // ValueValue — match by cell value (text content).
            else if (property == winrt::ValuePatternIdentifiers::ValueProperty())
            {
                hstring requested = StringPropertyValue(value);
                hstring candidate = ItemToName(item);
                isMatch = (candidate == requested);
            }
            // Match only explicit row AutomationId values; defaults are empty.
            else if (property == winrt::AutomationElementIdentifiers::AutomationIdProperty())
            {
                hstring requested = StringPropertyValue(value);

                hstring candidate;
                if (auto rowElement = repeater.TryGetElement(i))
                {
                    if (auto rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement))
                    {
                        candidate = rowPeer.GetAutomationId();
                    }
                }
                isMatch = (candidate == requested);
            }
            // ClassName — match by the row peer class name.
            else if (property == winrt::AutomationElementIdentifiers::ClassNameProperty())
            {
                hstring requested = StringPropertyValue(value);
                hstring candidate = winrt::hstring_name_of<winrt::TableViewRow>();
                if (auto rowElement = repeater.TryGetElement(i))
                {
                    if (auto rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement))
                    {
                        candidate = rowPeer.GetClassName();
                    }
                }
                isMatch = (candidate == requested);
            }
            // ControlType — match by the row peer control type.
            else if (property == winrt::AutomationElementIdentifiers::ControlTypeProperty())
            {
                winrt::AutomationControlType requested{};
                if (TryGetControlTypePropertyValue(value, requested))
                {
                    auto candidate = winrt::AutomationControlType::DataItem;
                    if (auto rowElement = repeater.TryGetElement(i))
                    {
                        if (auto rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement))
                        {
                            candidate = rowPeer.GetAutomationControlType();
                        }
                    }
                    isMatch = (candidate == requested);
                }
            }
            // ItemType — match by item type string (e.g., "DataRow").
            else if (property == winrt::AutomationElementIdentifiers::ItemTypeProperty())
            {
                hstring requested;
                if (auto const valueString = value.try_as<winrt::IPropertyValue>())
                {
                    if (valueString.Type() == winrt::PropertyType::String)
                    {
                        requested = valueString.GetString();
                    }
                }

                hstring candidate = L"DataRow";
                isMatch = (candidate == requested);
            }
            // Unsupported properties stop the scan rather than looping indefinitely.
            else
            {
                return nullptr;
            }
        }

        if (!isMatch)
        {
            continue;
        }

        auto rowElement = repeater.TryGetElement(i);
        if (!rowElement)
        {
            try
            {
                rowElement = repeater.GetOrCreateElement(i);
            }
            catch (...)
            {
                return nullptr;
            }
        }

        if (rowElement)
        {
            if (property == winrt::AutomationElementIdentifiers::NameProperty() &&
                !MatchesItemName(*impl, item, rowElement, StringPropertyValue(value)))
            {
                continue;
            }

            if (auto rowPeer = ElementItemPeer(rowElement))
            {
                return ProviderFromPeer(rowPeer);
            }
        }
        return nullptr;
    }

    return nullptr;
}
