// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSource.h"
#include "RowMetadataProvider.h"
#include "GroupedSourceAdapter.h"
#include "ShapedItemsSource.h"
#include "SortMemberPathResolver.h"
#include "SharedHelpers.h"

namespace
{
    namespace tabularPrimitives = winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation;
}

TableViewSource::~TableViewSource() = default;

TableViewSource::TableViewSource(winrt::IInspectable const& items) :
    m_engine(std::make_shared<::ShapedItemsSource>(items))
{
    // Diagnostics raised by the engine are contractual for apps written against this type, so
    // keep naming it even though the engine itself must not know about it.
    m_engine->DiagnosticName(L"TableViewSource");
    // Handlers are installed before Start() so the very first projection is observed like any
    // later one -- the control never has to special-case its own construction.
    m_engine->SetProjectionRebuiltHandler([this]() { OnProjectionRebuilt(); });
    m_engine->SetShapeSwappedHandler([this]() { NotifyOwnerProjectionChanged(); });
    m_engine->SetShapingChangedHandler([this](bool reorderOnly) { NotifyOwnerShapingChanged(reorderOnly); });
    m_engine->Start();
}

winrt::TableViewSource TableViewSource::From(winrt::IInspectable const& items)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }

    return winrt::make<::TableViewSource>(items);
}

winrt::TableViewSource TableViewSource::Filter(winrt::TableViewPredicate const& predicate)
{
    if (!predicate)
    {
        throw winrt::hresult_invalid_argument(L"predicate cannot be null.");
    }
    m_engine->SetFilter([predicate](winrt::IInspectable const& item) { return predicate(item); });
    return *this;
}

winrt::TableViewSource TableViewSource::GroupBy(winrt::TableViewKeySelector const& key)
{
    return GroupBy(key, nullptr);
}

winrt::TableViewSource TableViewSource::GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }

    RowIdentity::TabularIdentitySelector groupIdentity{ nullptr };
    if (groupIdentitySelector)
    {
        groupIdentity = [groupIdentitySelector](winrt::IInspectable const& item) { return groupIdentitySelector(item); };
    }

    m_engine->SetGroup([key](winrt::IInspectable const& item) { return key(item); }, groupIdentity);
    return *this;
}

winrt::TableViewSource TableViewSource::ClearFilter()
{
    m_engine->ClearFilter();
    return *this;
}

winrt::TableViewSource TableViewSource::ClearGroupBy()
{
    m_engine->ClearGroup();
    return *this;
}

winrt::TableViewSource TableViewSource::ClearSort()
{
    m_engine->ClearSorts();
    return *this;
}

winrt::TableViewSource TableViewSource::ClearSort(winrt::hstring const& sortAxisToken)
{
    m_engine->ClearSort(sortAxisToken);
    return *this;
}

winrt::TableViewSource TableViewSource::ClearSortsExcept(winrt::hstring const& sortAxisToken)
{
    m_engine->ClearSortsExcept(sortAxisToken);
    return *this;
}

std::vector<TableViewSource::ActiveSortAxisInfo> TableViewSource::ActiveSortAxisInfos() const
{
    std::vector<ActiveSortAxisInfo> infos;
    for (auto const& axis : m_engine->ActiveSortAxisInfos())
    {
        infos.push_back({ axis.AxisToken, axis.SortMemberPath, axis.Direction });
    }
    return infos;
}

winrt::hstring TableViewSource::SortAxisTokenForPath(winrt::hstring const& sortMemberPath)
{
    // Distinct from the control's "column:<address>" tokens, so a path-declared axis and a
    // column-declared axis are never mistaken for one another even when they sort the same
    // property. Tokenizing at all is what makes re-sorting the same path REPLACE its axis: an
    // untokenized axis is matched by delegate identity, and every call would synthesize a fresh
    // delegate, so Sort("Name", Asc) followed by Sort("Name", Desc) would stack two axes.
    return L"path:" + sortMemberPath;
}

winrt::TableViewSource TableViewSource::Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction)
{
    return SortCore({}, {}, key, {}, direction);
}

winrt::TableViewSource TableViewSource::Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (sortMemberPath.empty())
    {
        throw winrt::hresult_invalid_argument(L"sortMemberPath cannot be empty.");
    }

    // One resolver per axis, captured by the selector and reused across every comparison: the
    // binding is built once per path, so steady-state cost is a DataContext write plus a property
    // read rather than a fresh binding per item.
    auto resolver = std::make_shared<SortMemberPathResolver>(sortMemberPath);
    winrt::TableViewKeySelector key{
        [resolver](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            return resolver->Resolve(item);
        }
    };

    return SortCore({}, SortAxisTokenForPath(sortMemberPath), key, sortMemberPath, direction);
}

winrt::TableViewSource TableViewSource::SortReplacing(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    return SortCore(previousSortAxisToken, sortAxisToken, key, sortMemberPath, direction);
}

winrt::TableViewSource TableViewSource::SortCore(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }

    // A projected enum is just an int32 across the ABI, so an out-of-range value would otherwise
    // reach the comparator and be treated as Descending (anything that is not Ascending). None is
    // a defined value and removes this key's sort axis, per the IDL contract.
    if (direction != winrt::SortDirection::None &&
        direction != winrt::SortDirection::Ascending &&
        direction != winrt::SortDirection::Descending)
    {
        throw winrt::hresult_invalid_argument(L"direction must be a defined SortDirection value.");
    }

    // The engine stores a std::function, which cannot be compared, so the delegate itself is
    // handed over as the axis identity. The wrapping lambda holds a strong ref to that same
    // delegate, so the identity stays valid for as long as the axis lives.
    m_engine->SetSort(
        previousSortAxisToken,
        sortAxisToken,
        [key](winrt::IInspectable const& item) { return key(item); },
        key.as<winrt::Windows::Foundation::IUnknown>(),
        sortMemberPath,
        direction);
    return *this;
}

bool TableViewSource::IsGrouped() const
{
    return m_engine->IsProjectedAsGrouped();
}

winrt::ItemsSourceView TableViewSource::GetItemsSourceView()
{
    return m_itemsSourceView.get();
}

TableViewRowMetadataProvider TableViewSource::GetRowMetadata()
{
    return m_rowMetadata;
}

void TableViewSource::OnProjectionRebuilt()
{
    // The control-level reading of the projection: what an ItemsRepeater consumes, and how a row
    // at an index is described. The engine reports which shape it produced; deciding what that
    // shape means for a TableView is this class's only remaining projection responsibility.
    switch (m_engine->Kind())
    {
    case ::ShapedItemsSource::ProjectionKind::Grouped:
    {
        auto const adapter = m_engine->GroupedAdapter();
        // No wrap: the grouped view IS an ItemsSourceView, so ItemsRepeater consumes it directly.
        m_itemsSourceView.set(adapter->Entries());
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForGroupedRows(adapter, MakeIdentitySelector());
        break;
    }
    case ::ShapedItemsSource::ProjectionKind::Flat:
    {
        // tracker_ref, so set through .set(); read the local back into the metadata provider since
        // CreateForFlatRows takes the concrete ItemsSourceView, not the tracked slot.
        auto const flatView = winrt::ItemsSourceView{ m_engine->Rows() };
        m_itemsSourceView.set(flatView);
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForFlatRows(flatView, MakeIdentitySelector());
        break;
    }
    case ::ShapedItemsSource::ProjectionKind::Unshaped:
        // An unshaped mirror carries no shaped identity, so there is deliberately no row
        // metadata: consumers must not read sorted/identity semantics off a degraded projection.
        m_itemsSourceView.set(winrt::ItemsSourceView{ m_engine->Rows() });
        m_rowMetadata = nullptr;
        break;
    case ::ShapedItemsSource::ProjectionKind::None:
        m_itemsSourceView.set(nullptr);
        m_rowMetadata = nullptr;
        break;
    }
}

TableViewRowItemKeySelector TableViewSource::MakeIdentitySelector() const
{
    // Bridge the engine's Object-returning identity selector to the String-returning selector the
    // row-metadata provider consumes. The engine derives identity from each item's object identity
    // and already stringifies it, so this only unwraps the box.
    auto const keySelector = m_engine->IdentitySelector();
    if (!keySelector)
    {
        return {};
    }

    return [keySelector](winrt::IInspectable const& item) -> winrt::hstring
    {
        winrt::IInspectable key{ nullptr };
        try
        {
            key = keySelector(item);
        }
        catch (...)
        {
            return L"";
        }
        if (auto propertyValue = key.try_as<winrt::IPropertyValue>();
            propertyValue && propertyValue.Type() == winrt::PropertyType::String)
        {
            return propertyValue.GetString();
        }
        return {};
    };
}

void TableViewSource::SetOwningTableView(winrt::IInspectable const& owner)
{
    // Sharing one shaped TableViewSource across two TableView controls has no coherent
    // semantics: shaping verbs (Filter/Sort/GroupBy) mutate a single projection, and the two
    // controls would compete for it (last-writer-wins) while both cache the ItemsSourceView /
    // row-metadata / grouped-ness the first bind produced. Since only a single owner slot
    // exists, silently overwriting m_owningTableView here left the previous owner rendering
    // against stale shape metadata and never receiving projection-shape notifications.
    //
    // Contract: fail-fast if a second live TableView tries to bind to a TableViewSource that
    // is already owned by a different TableView. In chk this asserts (same shape as the
    // null-queue and identity fail-fasts in this file); in fre we keep the existing owner
    // intact and refuse the new binding rather than corrupt the projection state, so a
    // shipping app degrades gracefully instead of crashing. Callers must Unbind the source
    // from the previous TableView (assign a different ItemsSource) before binding it here.
    if (owner)
    {
        if (auto existingOwner = m_owningTableView.get())
        {
            if (existingOwner != owner)
            {
                MUX_ASSERT_MSG(false,
                    L"TableViewSource: attempted to bind an already-owned source to a second TableView. "
                    L"A TableViewSource has single-owner semantics; unbind it from the previous "
                    L"TableView (assign a different ItemsSource) before binding to another.");
                return;
            }
            // Same TableView re-registering itself (e.g. re-entrant ItemsSource re-assignment
            // with the same value). Idempotent.
        }
    }

    m_owningTableView = owner ? winrt::weak_ref<winrt::IInspectable>{ owner } : winrt::weak_ref<winrt::IInspectable>{ nullptr };

    if (!owner)
    {
        // Detaching: drop the owner's handlers with the owner itself, so a source that has been
        // swapped out of ItemsSource cannot drive the control it used to belong to.
        m_projectionChanged = nullptr;
        m_shapingChanged = nullptr;
    }
}

void TableViewSource::NotifyOwnerProjectionChanged()
{
    if (m_projectionChanged)
    {
        m_projectionChanged();
    }
}

void TableViewSource::NotifyOwnerShapingChanged(bool reorderOnly)
{
    if (m_shapingChanged)
    {
        m_shapingChanged(reorderOnly);
    }
}
