// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewAutomationPeer.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewAutomationPeer.properties.cpp"

TableViewAutomationPeer::TableViewAutomationPeer(winrt::TableView const& owner)
    : ReferenceTracker(owner)
{
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

        // Forward Scroll to the body ScrollViewer's peer.
        if (patternInterface == winrt::PatternInterface::Scroll)
        {
            if (auto const impl = GetImpl())
            {
                if (auto bodyScroller = impl->GetBodyScrollerInternal())
                {
                    if (auto svPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(bodyScroller))
                    {
                        if (auto provider = svPeer.GetPattern(winrt::PatternInterface::Scroll))
                        {
                            return provider;
                        }
                    }
                }
            }
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

    winrt::TableViewRow rowElement{ nullptr };
    try
    {
        rowElement = repeater.TryGetElement(row).try_as<winrt::TableViewRow>();
    }
    catch (...)
    {
        return nullptr;
    }

    if (!rowElement)
    {
        // Bounded realization — realize ONLY the single requested row (not a scan)
        // so IGridProvider.GetItem honors the UIA grid contract (Narrator cell addressing)
        // without force-realizing the whole dataset.
        // Full sweep-virtualization would require exposing VirtualizedItemPattern.
        try
        {
            rowElement = repeater.GetOrCreateElement(row).try_as<winrt::TableViewRow>();
        }
        catch (...)
        {
            return nullptr;
        }
        if (!rowElement)
        {
            return nullptr;
        }
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
        // Return the same rich cell peer used for tree navigation.
        auto const owningColumn = rowImpl->GetCellOwningColumn(cellElement);
        winrt::AutomationPeer const cellPeer =
            winrt::make<TableViewCellAutomationPeer>(cellFE, rowElement, owningColumn, column);
        return ProviderFromPeer(cellPeer);
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
        return {};
    }

    const int32_t selectedIndex = impl->SelectedIndexInternal();
    if (selectedIndex < 0)
    {
        return {};
    }

    // Only a realized row has a provider. A selected row scrolled out of the realization window
    // reports as unrealized here rather than fabricating a peer for an element that does not
    // exist; AT clients reach it through ItemContainer.FindItemByProperty, which realizes it.
    if (auto const repeater = impl->GetRowsRepeaterInternal())
    {
        if (auto const rowElement = repeater.TryGetElement(selectedIndex))
        {
            if (auto const rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement))
            {
                // An empty selection is a valid answer; an array holding a null provider is not.
                if (auto const provider = ProviderFromPeer(rowPeer))
                {
                    std::vector<winrt::IRawElementProviderSimple> selection;
                    selection.push_back(provider);
                    return winrt::com_array(selection);
                }
            }
        }
    }

    return {};
}

bool TableViewAutomationPeer::CanSelectMultiple()
{
    // Single selection only in this release. Multiple must derive this from SelectionMode, and
    // GetSelection below must return every selected row rather than at most one.
    return false;
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

    // Key off visible logical Columns() so headers enumerate before templates realize.
    if (auto const tableView = Owner().try_as<winrt::TableView>())
    {
        if (auto const columns = tableView.Columns())
        {
            const auto count = columns.Size();
            headers.reserve(count);
            liveCache.reserve(count);
            for (uint32_t i = 0; i < count; i++)
            {
                auto const column = columns.GetAt(i);
                if (!IsVisibleColumn(column))
                {
                    continue;
                }

                auto headerPeer = GetOrCreateColumnHeaderPeer(tableView, column);
                if (!headerPeer)
                {
                    continue;
                }

                // The peer is cached even when it currently has no provider: ProviderFromPeer only
                // yields one for a peer UIA has connected, so a transiently unconnected peer must
                // keep its identity for the next enumeration rather than being rebuilt.
                liveCache.push_back({ winrt::make_weak(column), headerPeer });

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
    if (!tableView || !column)
    {
        return nullptr;
    }

    // Looked up against the cache as it stood at the previous enumeration, so a column that
    // survives keeps the very same peer - and therefore the same provider identity - across calls.
    for (auto const& entry : m_columnHeaderPeerCache)
    {
        if (entry.peer && entry.column.get() == column)
        {
            return entry.peer;
        }
    }

    // The TableView owns the peer so headers stay enumerable before their templates realize;
    // TableViewColumnHeaderAutomationPeer supplies its own per-column RuntimeId and AutomationId
    // to keep the headers distinguishable despite the shared owner.
    return winrt::make<TableViewColumnHeaderAutomationPeer>(tableView, column);
}

static winrt::hstring ItemToName(winrt::IInspectable const& item)
{
    // Boxed WinRT primitives surface as IPropertyValue, not IStringable.
    if (auto const propValue = item.try_as<winrt::IPropertyValue>())
    {
        switch (propValue.Type())
        {
        case winrt::PropertyType::String:  return propValue.GetString();
        case winrt::PropertyType::Boolean: return propValue.GetBoolean() ? winrt::hstring{ L"True" } : winrt::hstring{ L"False" };
        case winrt::PropertyType::Int16:   return winrt::to_hstring(static_cast<int32_t>(propValue.GetInt16()));
        case winrt::PropertyType::Int32:   return winrt::to_hstring(propValue.GetInt32());
        case winrt::PropertyType::Int64:   return winrt::to_hstring(propValue.GetInt64());
        case winrt::PropertyType::UInt8:   return winrt::to_hstring(static_cast<uint32_t>(propValue.GetUInt8()));
        case winrt::PropertyType::UInt16:  return winrt::to_hstring(static_cast<uint32_t>(propValue.GetUInt16()));
        case winrt::PropertyType::UInt32:  return winrt::to_hstring(propValue.GetUInt32());
        case winrt::PropertyType::UInt64:  return winrt::to_hstring(propValue.GetUInt64());
        case winrt::PropertyType::Single:  return winrt::to_hstring(propValue.GetSingle());
        case winrt::PropertyType::Double:  return winrt::to_hstring(propValue.GetDouble());
        default: break;
        }
    }

    if (auto const stringable = item.try_as<winrt::IStringable>())
    {
        return stringable.ToString();
    }

    return {};
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

    // Resolve startAfter through its owning realized TableViewRow.
    int32_t startIndex = -1;
    if (startAfter)
    {
        if (auto const startPeer = PeerFromProvider(startAfter).try_as<winrt::FrameworkElementAutomationPeer>())
        {
            if (auto const ownerRow = startPeer.Owner().try_as<winrt::TableViewRow>())
            {
                const int32_t rowIndex = repeater.GetElementIndex(ownerRow);
                if (rowIndex >= 0)
                {
                    startIndex = rowIndex;
                }
                else
                {
                    // Avoid restarting at item 0 after startAfter has been virtualized away.
                    return nullptr;
                }
            }
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
                hstring requested = StringPropertyValue(value);

                hstring candidate;
                if (auto rowElement = repeater.TryGetElement(i))
                {
                    if (auto rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement))
                    {
                        candidate = rowPeer.GetName();
                    }
                }
                if (candidate.empty())
                {
                    candidate = ItemToName(item);
                }

                isMatch = (candidate == requested);
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
            if (auto rowPeer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(rowElement))
            {
                return ProviderFromPeer(rowPeer);
            }
        }
        return nullptr;
    }

    return nullptr;
}
