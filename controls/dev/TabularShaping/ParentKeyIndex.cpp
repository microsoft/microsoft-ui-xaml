// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ParentKeyIndex.h"
#include "ShapingHelpers.h"

#include <bit>
#include <string>

namespace ShapingHelpers
{
namespace
{
    constexpr wchar_t c_nodePrefix[] = L"node:";
    constexpr size_t c_nodePrefixLength = std::size(c_nodePrefix) - 1;
    constexpr std::wstring_view c_valueKeyPrefix{ L"value:" };
    constexpr std::wstring_view c_objectKeyPrefix{ L"object:" };

    bool IsNoKey(winrt::IInspectable const& key)
    {
        if (!key) return true;
        if (auto pv = key.try_as<winrt::Windows::Foundation::IPropertyValue>())
        {
            if (pv.Type() == winrt::Windows::Foundation::PropertyType::Empty) return true;
            if (pv.Type() == winrt::Windows::Foundation::PropertyType::String && pv.GetString().empty()) return true;
        }
        return false;
    }

    winrt::IInspectable SafeSelect(KeySelector const& selector, winrt::IInspectable const& item)
    {
        try { return selector(item); } catch (...) { return nullptr; }
    }

    // Value-typed boxes the shared lookup-key formatter does not cover. A WinRT enum boxes as an
    // IPropertyValue of type OtherType whose integer getters still read the value, so they key by
    // (type, value); a fresh box per selector call must not change the key. Returns empty when the
    // value is not one of these, leaving the shared formatter to decide.
    std::wstring TryFormatExtraValueKey(winrt::IInspectable const& key)
    {
        auto const pv = key.try_as<winrt::Windows::Foundation::IPropertyValue>();
        if (!pv)
        {
            return {};
        }

        wchar_t buffer[64];
        switch (pv.Type())
        {
        case winrt::Windows::Foundation::PropertyType::OtherType:
        {
            std::wstring value;
            try
            {
                value = std::to_wstring(pv.GetInt64());
            }
            catch (...)
            {
                try
                {
                    value = L"u" + std::to_wstring(pv.GetUInt64());
                }
                catch (...)
                {
                    return {};
                }
            }

            std::wstring typeName;
            try { typeName = winrt::get_class_name(key); } catch (...) {}
            return std::wstring{ c_valueKeyPrefix } + L"e:" + typeName + L":" + value;
        }
        case winrt::Windows::Foundation::PropertyType::Point:
        {
            const auto p = pv.GetPoint();
            swprintf_s(buffer, L"pt:%08X,%08X", std::bit_cast<uint32_t>(p.X), std::bit_cast<uint32_t>(p.Y));
            return std::wstring{ c_valueKeyPrefix } + buffer;
        }
        case winrt::Windows::Foundation::PropertyType::Size:
        {
            const auto s = pv.GetSize();
            swprintf_s(buffer, L"sz:%08X,%08X", std::bit_cast<uint32_t>(s.Width), std::bit_cast<uint32_t>(s.Height));
            return std::wstring{ c_valueKeyPrefix } + buffer;
        }
        case winrt::Windows::Foundation::PropertyType::Rect:
        {
            const auto r = pv.GetRect();
            swprintf_s(buffer, L"rc:%08X,%08X,%08X,%08X",
                std::bit_cast<uint32_t>(r.X), std::bit_cast<uint32_t>(r.Y),
                std::bit_cast<uint32_t>(r.Width), std::bit_cast<uint32_t>(r.Height));
            return std::wstring{ c_valueKeyPrefix } + buffer;
        }
        default:
            return {};
        }
    }

    bool SafeFilter(ParentKeyFilter const& filter, winrt::IInspectable const& item)
    {
        try { return filter(item); } catch (...) { return false; }
    }

    // User-facing form of a node key for error text: the raw value for a value key (the internal
    // "value:<type-tag>:" prefix stripped), or a placeholder for an object key, whose lookup form
    // is only a pointer.
    winrt::hstring DescribeNodeKey(std::wstring const& nodeKey)
    {
        std::wstring_view view{ nodeKey };
        if (view.starts_with(c_nodePrefix))
        {
            view.remove_prefix(c_nodePrefixLength);
        }

        if (view.starts_with(c_objectKeyPrefix))
        {
            return L"(object key)";
        }

        if (view.starts_with(c_valueKeyPrefix))
        {
            view.remove_prefix(c_valueKeyPrefix.size());
            const bool isEnum = view.starts_with(L"e:");
            if (const auto tagEnd = view.find(L':'); tagEnd != std::wstring_view::npos)
            {
                view.remove_prefix(tagEnd + 1);
            }

            // An enum key is "<type>:<value>"; the value alone is what the app would recognise.
            if (isEnum)
            {
                if (const auto valueStart = view.rfind(L':'); valueStart != std::wstring_view::npos)
                {
                    view.remove_prefix(valueStart + 1);
                }
            }
        }

        return winrt::hstring{ view };
    }
}

std::wstring MakeNodeKey(winrt::IInspectable const& key)
{
    if (IsNoKey(key)) return {};
    if (auto extra = TryFormatExtraValueKey(key); !extra.empty())
    {
        return std::wstring{ c_nodePrefix } + extra;
    }
    return std::wstring{ c_nodePrefix } + std::wstring{ ValueKey::ToObjectLookupKey(key, false) };
}

const std::vector<winrt::IInspectable>* ParentKeyIndex::TryGetChildren(std::wstring const& nodeKey) const
{
    auto it = Children.find(nodeKey);
    return it == Children.end() ? nullptr : &it->second;
}

std::wstring const* ParentKeyIndex::TryGetKey(winrt::IInspectable const& item) const
{
    auto it = KeyByItem.find(winrt::get_abi(item));
    return it == KeyByItem.end() ? nullptr : &it->second;
}

bool BuildParentKeyIndex(
    std::vector<winrt::IInspectable> const& rows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentKeyFilter const& filter,
    ParentKeyIndex& out,
    winrt::hstring& error)
{
    constexpr size_t c_root = SIZE_MAX;
    const size_t n = rows.size();
    std::vector<std::wstring> nodeKeys(n);
    std::vector<size_t> parentIndex(n, c_root);
    std::unordered_map<std::wstring, size_t> indexByKey;
    indexByKey.reserve(n);

    // An object key's lookup form is its address, so every key and parent key is held for the
    // whole build: a released temporary's address could be reused by the next one and alias it.
    std::vector<winrt::IInspectable> keepAlive;
    keepAlive.reserve(n * 2);

    // Pass 1: keys, duplicates, null keys.
    for (size_t i = 0; i < n; ++i)
    {
        auto key = SafeSelect(keySelector, rows[i]);
        nodeKeys[i] = MakeNodeKey(key);
        keepAlive.push_back(std::move(key));
        if (nodeKeys[i].empty())
        {
            error = L"WithParent: key selector returned null or empty for item at source position " + winrt::to_hstring(i) + L".";
            return false;
        }
        if (!indexByKey.emplace(nodeKeys[i], i).second)
        {
            error = L"WithParent: duplicate key '" + DescribeNodeKey(nodeKeys[i]) + L"'.";
            return false;
        }
    }

    // Pass 2: resolve parents; self-parent is an error, unknown parent = orphan root.
    for (size_t i = 0; i < n; ++i)
    {
        auto parentKeyValue = SafeSelect(parentKeySelector, rows[i]);
        auto parentKey = MakeNodeKey(parentKeyValue);
        keepAlive.push_back(std::move(parentKeyValue));
        if (parentKey.empty()) continue;
        if (parentKey == nodeKeys[i])
        {
            error = L"WithParent: item with key '" + DescribeNodeKey(nodeKeys[i]) + L"' is its own parent.";
            return false;
        }
        if (auto it = indexByKey.find(parentKey); it != indexByKey.end())
        {
            parentIndex[i] = it->second;
        }
    }

    // Child lists in sorted order.
    std::vector<std::vector<size_t>> childIdx(n);
    std::vector<size_t> rootIdx;
    for (size_t i = 0; i < n; ++i)
    {
        (parentIndex[i] == c_root ? rootIdx : childIdx[parentIndex[i]]).push_back(i);
    }

    // Cycle check: everything must be reachable from a root.
    size_t reached = 0;
    {
        std::vector<size_t> stack(rootIdx.rbegin(), rootIdx.rend());
        while (!stack.empty())
        {
            const size_t i = stack.back(); stack.pop_back();
            ++reached;
            stack.insert(stack.end(), childIdx[i].rbegin(), childIdx[i].rend());
        }
    }
    if (reached != n)
    {
        std::vector<bool> seen(n, false);
        std::vector<size_t> stack(rootIdx.begin(), rootIdx.end());
        while (!stack.empty()) { const size_t i = stack.back(); stack.pop_back(); seen[i] = true; stack.insert(stack.end(), childIdx[i].begin(), childIdx[i].end()); }
        for (size_t i = 0; i < n; ++i)
        {
            if (!seen[i])
            {
                // An unreached node may only hang BELOW a cycle. Its parent chain never reaches a
                // root, so walking it must revisit a node, and the first one revisited is on the
                // cycle itself -- the key worth naming.
                std::vector<bool> onPath(n, false);
                size_t p = i;
                while (!onPath[p])
                {
                    onPath[p] = true;
                    p = parentIndex[p];
                }
                error = L"WithParent: cycle detected involving key '" + DescribeNodeKey(nodeKeys[p]) + L"'.";
                return false;
            }
        }
    }

    // Filter: 0 = out, 1 = match, 2 = context (ancestor of a match).
    std::vector<uint8_t> state(n, 1);
    if (filter)
    {
        for (size_t i = 0; i < n; ++i) state[i] = SafeFilter(filter, rows[i]) ? 1 : 0;
        for (size_t i = 0; i < n; ++i)
        {
            if (state[i] != 1) continue;
            for (size_t p = parentIndex[i]; p != c_root && state[p] == 0; p = parentIndex[p]) state[p] = 2;
        }
    }

    ParentKeyIndex result;
    for (size_t i : rootIdx) if (state[i]) result.Roots.push_back(rows[i]);
    for (size_t i = 0; i < n; ++i)
    {
        result.UnfilteredKeys.insert(nodeKeys[i]);
        if (!state[i]) continue;
        result.KeyByItem.emplace(winrt::get_abi(rows[i]), nodeKeys[i]);
        if (state[i] == 2) result.ContextKeys.insert(nodeKeys[i]);
        std::vector<winrt::IInspectable> kids;
        for (size_t c : childIdx[i]) if (state[c]) kids.push_back(rows[c]);
        if (!kids.empty()) result.Children.emplace(nodeKeys[i], std::move(kids));
    }
    out = std::move(result);
    return true;
}
}
