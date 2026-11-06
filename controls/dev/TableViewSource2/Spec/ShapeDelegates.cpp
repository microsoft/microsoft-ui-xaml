// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Spec/ShapeDelegates.h"

namespace
{
    // Canonical COM identity, so the same delegate reached through another interface pointer maps
    // to the same id. The address is only compared, never dereferenced.
    void const* CanonicalAddress(winrt::Windows::Foundation::IUnknown const& value)
    {
        return winrt::get_abi(value.as<winrt::Windows::Foundation::IUnknown>());
    }

    // True when any spec in use refers to `id` from a delegate slot. Path sort levels have no
    // delegate. Allocates nothing.
    bool IsReferenced(DelegateId id, std::span<ShapeSpec const* const> specsInUse) noexcept
    {
        for (ShapeSpec const* const spec : specsInUse)
        {
            if (!spec)
            {
                continue;
            }

            for (FilterSpec const& filter : spec->Filters)
            {
                if (filter.Predicate.Id == id)
                {
                    return true;
                }
            }

            for (GroupSpec const& group : spec->Groups)
            {
                if (group.Key.Id == id)
                {
                    return true;
                }
            }

            for (SortSpec const& sort : spec->Sorts)
            {
                if (sort.SortKind != SortSpec::Kind::Path && sort.Delegate.Id == id)
                {
                    return true;
                }
            }
        }

        return false;
    }

    // Finds or adds the entry for `value`; returns its id. Both maps are updated together: if the
    // second insert throws, the first is undone.
    template <typename T, typename Entries, typename IdByAddress, typename MakeId>
    DelegateId RegisterTyped(
        ITrackerHandleManager const* owner,
        Entries& entries,
        IdByAddress& idByAddress,
        T const& value,
        MakeId&& makeId)
    {
        void const* const address = CanonicalAddress(value);
        if (auto const known = idByAddress.find(address); known != idByAddress.end())
        {
            // The same object registered again must be the same kind of delegate.
            MUX_ASSERT(entries.contains(known->second));
            MUX_ASSERT(entries.at(known->second).Delegate.has_value());
            MUX_ASSERT(std::holds_alternative<tracker_ref<T>>(*entries.at(known->second).Delegate));
            return known->second;
        }

        // Build the tracked reference before touching either map, so a throw here changes nothing.
        typename Entries::mapped_type entry;
        entry.Delegate.emplace(std::in_place_type<tracker_ref<T>>, owner, value);

        DelegateId const id = makeId();
        entries.emplace(id, std::move(entry));
        auto undoEntry = wil::scope_exit([&]() noexcept
        {
            entries.erase(id);
        });
        idByAddress.emplace(address, id);
        undoEntry.release();
        return id;
    }

    // The delegate `id` names, copied out so a re-entrant registry change cannot release it while
    // it runs.
    template <typename T, typename Entries>
    T Lookup(Entries const& entries, DelegateId id)
    {
        auto const it = entries.find(id);
        tracker_ref<T> const* const typed = (it != entries.end() && it->second.Delegate) ?
            std::get_if<tracker_ref<T>>(&*it->second.Delegate) :
            nullptr;
        if (!typed)
        {
            // An id Retain already released, or one taken from the wrong spec slot.
            MUX_ASSERT(false);
            throw winrt::hresult_error(E_UNEXPECTED);
        }

        return typed->get();
    }
}

ShapeDelegates::ShapeDelegates(ITrackerHandleManager const* trackerOwner) noexcept :
    m_trackerOwner(trackerOwner)
{
}

ShapeDelegates::~ShapeDelegates() = default;

DelegateId ShapeDelegates::NextId() noexcept
{
    return static_cast<DelegateId>(m_nextId++);
}

uint64_t ShapeDelegates::NextVersion() noexcept
{
    return m_nextVersion++;
}

DelegateRef ShapeDelegates::Register(winrt::TableViewPredicate const& predicate)
{
    DelegateId const id = RegisterTyped(m_trackerOwner, m_entries, m_idByAddress, predicate, [this]() noexcept { return NextId(); });
    return DelegateRef{ id, NextVersion() };
}

DelegateRef ShapeDelegates::Register(winrt::TableViewKeySelector const& keySelector)
{
    DelegateId const id = RegisterTyped(m_trackerOwner, m_entries, m_idByAddress, keySelector, [this]() noexcept { return NextId(); });
    return DelegateRef{ id, NextVersion() };
}

DelegateRef ShapeDelegates::Register(winrt::TableViewComparer2 const& comparer)
{
    DelegateId const id = RegisterTyped(m_trackerOwner, m_entries, m_idByAddress, comparer, [this]() noexcept { return NextId(); });
    return DelegateRef{ id, NextVersion() };
}

void ShapeDelegates::Retain(std::span<ShapeSpec const* const> specsInUse)
{
    // Only entries that exist now are candidates. Releasing a delegate can run app code (its last
    // Release), which could register a new delegate; ids only grow, so anything registered during
    // this call is at or above `limit` and is left alone.
    auto const limit = static_cast<DelegateId>(m_nextId);

    // Addresses first: erasing them runs no app code, and a released delegate can no longer be
    // found by a re-entrant Register.
    for (auto it = m_idByAddress.begin(); it != m_idByAddress.end();)
    {
        it = IsReferenced(it->second, specsInUse) ? std::next(it) : m_idByAddress.erase(it);
    }

    // Then the entries, one at a time. The node is detached before it is destroyed, and the scan
    // restarts after each release, so a re-entrant change to m_entries never invalidates an
    // iterator in use. The registry holds a handful of entries, so the restart costs nothing.
    for (;;)
    {
        auto it = m_entries.begin();
        while (it != m_entries.end() && (it->first >= limit || IsReferenced(it->first, specsInUse)))
        {
            ++it;
        }

        if (it == m_entries.end())
        {
            break;
        }

        auto released = m_entries.extract(it);
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
    // Destroys every tracker_ref while the owner is alive; never calls get().
    m_idByAddress.clear();
    m_entries.clear();
    m_trackerOwner = nullptr;
}
