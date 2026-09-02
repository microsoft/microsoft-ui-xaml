// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RowIdentity.h"
#include "ShapingHelpers.h"
#include "TVDiag.h"

#include <winrt/Microsoft.UI.Xaml.Interop.h>

namespace
{
    void LogUntrackableRow(uint32_t index, wchar_t const* reason)
    {
#ifdef DBG
        TVDiag::DbgLogF(
            L"[RowIdentity] Row %u could not be given an identity (%ls) after validation had "
            L"already passed; it is absent from the identity map.\n",
            index,
            reason ? reason : L"unspecified reason");
#else
        UNREFERENCED_PARAMETER(index);
        UNREFERENCED_PARAMETER(reason);
#endif
    }
}

namespace RowIdentity
{
    TabularShapingHelpers::TabularKeySelector MakeObjectIdentitySelector()
    {
        return [](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            if (!item)
            {
                return nullptr;
            }

            // COM identity: the IUnknown obtained by QI is the canonical per-object pointer, so
            // two references to the same object always stringify identically and two distinct
            // objects never collide -- including two boxed copies of the same value, which are
            // separate objects and therefore separate rows.
            auto const unknown = item.as<winrt::Windows::Foundation::IUnknown>();
            auto const address = reinterpret_cast<uintptr_t>(winrt::get_abi(unknown));

            wchar_t buffer[2 + (sizeof(uintptr_t) * 2) + 1]{};
            swprintf_s(buffer, L"0x%zx", static_cast<size_t>(address));
            return winrt::box_value(winrt::hstring{ buffer });
        };
    }

    bool TryGetRequiredRowIdentity(
        winrt::IInspectable const& item,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        winrt::hstring& identity,
        wchar_t const*& reason)
    {
        identity = {};
        if (!item)
        {
            reason = L"null row item";
            return false;
        }

        if (!keySelector)
        {
            reason = L"missing row identity selector";
            return false;
        }

        winrt::IInspectable key{ nullptr };
        try
        {
            key = keySelector(item);
        }
        catch (...)
        {
            reason = L"row identity selector threw";
            return false;
        }

        if (!key)
        {
            reason = L"null row identity";
            return false;
        }

        if (auto propertyValue = key.try_as<winrt::IPropertyValue>();
            propertyValue && propertyValue.Type() == winrt::PropertyType::String)
        {
            identity = propertyValue.GetString();
            if (!identity.empty())
            {
                return true;
            }

            reason = L"empty row identity";
            return false;
        }

        reason = L"row identity selector returned a non-string value";
        return false;
    }

    bool ValidateRowIdentities(
        std::vector<winrt::IInspectable> const& rows,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        wchar_t const*& reason)
    {
        std::unordered_set<winrt::hstring> identities;
        identities.reserve(rows.size());

        for (auto const& item : rows)
        {
            winrt::hstring identity;
            if (!TryGetRequiredRowIdentity(item, keySelector, identity, reason))
            {
                return false;
            }

            if (!identities.insert(identity).second)
            {
                // Identity is the item's object address, so a repeat means one object is occupying
                // two rows -- not two items that merely look alike.
                reason = L"the same item object appears on more than one row";
                return false;
            }
        }

        return true;
    }

    void ClearFlatRowIdentityTracking(
        std::unordered_set<winrt::hstring>& identities,
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex)
    {
        identities.clear();
        identityToIndex.clear();
    }

    void RebuildFlatRowIdentityTracking(
        std::vector<winrt::IInspectable> const& rows,
        bool identityRequired,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        std::unordered_set<winrt::hstring>& identities,
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex)
    {
        ClearFlatRowIdentityTracking(identities, identityToIndex);
        if (!identityRequired)
        {
            return;
        }

        identities.reserve(rows.size());
        identityToIndex.reserve(rows.size());
        for (size_t index = 0; index < rows.size(); ++index)
        {
            winrt::hstring identity;
            wchar_t const* reason = nullptr;
            if (TryGetRequiredRowIdentity(rows[index], keySelector, identity, reason))
            {
                identities.insert(identity);
                identityToIndex.emplace(identity, static_cast<uint32_t>(index));
            }
            else
            {
                // Unreachable by construction: identity is required here, and both callers have
                // already proven every row can produce one. The full-rebuild path runs
                // ValidateRowIdentities immediately before this and throws on the first failure,
                // and the sort-only in-place path re-projects rows it just proved element-identical
                // to the previously validated projection. Reaching this branch therefore means an
                // upstream invariant broke, not that the source is merely awkward.
                //
                // Skip rather than throw, because the map only accelerates lookup: a missing entry
                // costs TryGetTrackedFlatRowIndex a failed find and the caller a linear scan, which
                // is a far better outcome in fre than tearing down a projection that is otherwise
                // intact. The cost is that the map is NOT guaranteed to hold one entry per row, so
                // its size must never be used as a row count -- use the row vector's size instead.
                LogUntrackableRow(static_cast<uint32_t>(index), reason);
                MUX_ASSERT(false);
            }
        }
    }

    bool TryGetTrackedFlatRowIndex(
        winrt::hstring const& identity,
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& rows,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        std::unordered_map<winrt::hstring, uint32_t> const& identityToIndex,
        uint32_t& index)
    {
        auto const it = identityToIndex.find(identity);
        if (it == identityToIndex.end() || !rows)
        {
            return false;
        }

        index = it->second;
        if (index >= rows.Size())
        {
            return false;
        }

        winrt::hstring projectedIdentity;
        wchar_t const* reason = nullptr;
        return TryGetRequiredRowIdentity(rows.GetAt(index), keySelector, projectedIdentity, reason) &&
            projectedIdentity == identity;
    }

    void ShiftTrackedFlatRowIndicesForInsert(
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex,
        uint32_t insertedIndex)
    {
        for (auto& entry : identityToIndex)
        {
            if (entry.second >= insertedIndex)
            {
                ++entry.second;
            }
        }
    }

    void ShiftTrackedFlatRowIndicesForRemove(
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex,
        uint32_t removedIndex)
    {
        for (auto& entry : identityToIndex)
        {
            if (entry.second > removedIndex)
            {
                --entry.second;
            }
        }
    }

    winrt::hstring StringifyKey(winrt::IInspectable const& key)
    {
        return TabularShapingHelpers::TabularValueKey::ToString(key);
    }
}
