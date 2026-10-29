// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Spec/ShapeDelegates.h"

#include <iterator>
#include <unordered_set>

namespace
{
    // Canonical COM identity, so the same delegate reached through another interface pointer
    // still maps to its id. Address only, never dereferenced after this.
    void const* CanonicalAddress(winrt::Windows::Foundation::IUnknown const& value)
    {
        return winrt::get_abi(value.as<winrt::Windows::Foundation::IUnknown>());
    }
}

ShapeDelegates::ShapeDelegates(ITrackerHandleManager const* trackerOwner) noexcept
    : m_trackerOwner(trackerOwner)
{
}

ShapeDelegates::~ShapeDelegates() = default;

DelegateId ShapeDelegates::NextId() noexcept
{
    return static_cast<DelegateId>(m_nextId++);
}

uint32_t ShapeDelegates::NextVersion() noexcept
{
    return m_nextVersion++;
}

namespace
{
    template <typename T, typename Entries, typename Ids, typename MakeId>
    DelegateId RegisterTyped(
        ITrackerHandleManager const* owner,
        Entries& entries,
        Ids& idByAddress,
        T const& value,
        MakeId&& makeId)
    {
        void const* const address = CanonicalAddress(value);
        if (auto const it = idByAddress.find(address); it != idByAddress.end())
        {
            MUX_ASSERT(entries.contains(it->second) && entries.at(it->second).Delegate &&
                std::holds_alternative<tracker_ref<T>>(*entries.at(it->second).Delegate));
            return it->second;
        }

        // Reserve both maps first so the two inserts below cannot leave one without the other.
        entries.reserve(entries.size() + 1);
        idByAddress.reserve(idByAddress.size() + 1);

        DelegateId const id = makeId();
        auto& entry = entries[id];
        entry.Delegate.emplace(std::in_place_type<tracker_ref<T>>, owner, value);
        idByAddress.emplace(address, id);
        return id;
    }

    // The delegate an id names, copied out so a re-entrant registry change cannot free it mid-call.
    template <typename T, typename Entries>
    T Lookup(Entries const& entries, DelegateId id)
    {
        auto const it = entries.find(id);
        auto const* const typed = it != entries.end() && it->second.Delegate ?
            std::get_if<tracker_ref<T>>(&*it->second.Delegate) : nullptr;
        if (!typed)
        {
            // An id from the wrong spec slot, or one Retain already released.
            MUX_ASSERT(false);
            throw winrt::hresult_error(E_UNEXPECTED);
        }
        return typed->get();
    }
}

DelegateRef ShapeDelegates::Register(winrt::TableViewPredicate const& predicate)
{
    DelegateId const id = RegisterTyped(m_trackerOwner, m_entries, m_idByAddress, predicate, [this] { return NextId(); });
    return DelegateRef{ id, NextVersion() };
}

DelegateRef ShapeDelegates::Register(winrt::TableViewKeySelector const& keySelector)
{
    DelegateId const id = RegisterTyped(m_trackerOwner, m_entries, m_idByAddress, keySelector, [this] { return NextId(); });
    return DelegateRef{ id, NextVersion() };
}

DelegateRef ShapeDelegates::Register(winrt::TableViewComparer2 const& comparer)
{
    DelegateId const id = RegisterTyped(m_trackerOwner, m_entries, m_idByAddress, comparer, [this] { return NextId(); });
    return DelegateRef{ id, NextVersion() };
}

void ShapeDelegates::Retain(std::span<ShapeSpec const* const> specsInUse)
{
    std::unordered_set<DelegateId> used;
    for (ShapeSpec const* const spec : specsInUse)
    {
        if (!spec)
        {
            continue;
        }
        for (auto const& filter : spec->Filters)
        {
            used.insert(filter.Predicate.Id);
        }
        for (auto const& group : spec->Groups)
        {
            used.insert(group.Key.Id);
        }
        for (auto const& sort : spec->Sorts)
        {
            if (sort.SortKind != SortSpec::Kind::Path)
            {
                used.insert(sort.Delegate.Id);
            }
        }
    }

    for (auto it = m_idByAddress.begin(); it != m_idByAddress.end();)
    {
        it = used.contains(it->second) ? std::next(it) : m_idByAddress.erase(it);
    }

    for (auto it = m_entries.begin(); it != m_entries.end();)
    {
        it = used.contains(it->first) ? std::next(it) : m_entries.erase(it);
    }
}

bool ShapeDelegates::Passes(DelegateId predicate, winrt::IInspectable const& item) const
{
    return Lookup<winrt::TableViewPredicate>(m_entries, predicate)(item);
}

winrt::IInspectable ShapeDelegates::Key(DelegateId keySelector, winrt::IInspectable const& item) const
{
    return Lookup<winrt::TableViewKeySelector>(m_entries, keySelector)(item);
}

int32_t ShapeDelegates::Compare(DelegateId comparer, winrt::IInspectable const& x, winrt::IInspectable const& y) const
{
    return Lookup<winrt::TableViewComparer2>(m_entries, comparer)(x, y);
}

void ShapeDelegates::Clear() noexcept
{
    // Drops every tracked delegate while the owner is alive; never calls get().
    m_entries.clear();
    m_idByAddress.clear();
    m_trackerOwner = nullptr;
}
