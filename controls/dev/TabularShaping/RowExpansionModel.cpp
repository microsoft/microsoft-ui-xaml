// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RowExpansionModel.h"

namespace ShapingHelpers
{
    void RowExpansionModel::SetDefaultExpanded(bool expanded)
    {
        if (m_defaultExpanded == expanded && m_nonDefault.empty())
        {
            return;
        }

        // Whether any key's resolved state actually moves depends on the exceptions: with none,
        // only a real default change matters. Decide before mutating, so the notification
        // describes what happened rather than what was requested.
        const bool defaultMoved = m_defaultExpanded != expanded;
        const bool hadExceptions = !m_nonDefault.empty();

        m_defaultExpanded = expanded;
        m_nonDefault.clear();

        if (!defaultMoved && !hadExceptions)
        {
            return;
        }

        Change change;
        change.AffectsAllKeys = true;
        change.IsExpanded = expanded;
        RaiseChanged(std::move(change));
    }

    bool RowExpansionModel::IsExpanded(winrt::hstring const& key) const
    {
        const bool defaultExpanded = m_defaultExpanded;
        if (key.empty())
        {
            // An empty key cannot be stored as an exception (nothing could ever clear it
            // selectively), so it always reads as the default rather than silently sharing one
            // bucket with every other unkeyed group.
            return defaultExpanded;
        }

        return m_nonDefault.find(key) != m_nonDefault.end() ? !defaultExpanded : defaultExpanded;
    }

    void RowExpansionModel::SetExpanded(winrt::hstring const& key, bool isExpanded)
    {
        if (key.empty() || IsExpanded(key) == isExpanded)
        {
            return;
        }

        const bool defaultExpanded = m_defaultExpanded;
        if (isExpanded == defaultExpanded)
        {
            // Back to the baseline: drop the exception rather than record agreement with it.
            // Recording it would let the set grow by one entry per toggle cycle.
            m_nonDefault.erase(key);
        }
        else
        {
            m_nonDefault.insert(key);
        }

        Change change;
        change.Keys.push_back(key);
        change.IsExpanded = isExpanded;
        RaiseChanged(std::move(change));
    }

    void RowExpansionModel::SetAllExpanded(bool isExpanded)
    {
        SetDefaultExpanded(isExpanded);
    }

    void RowExpansionModel::RetainOnly(std::unordered_set<winrt::hstring> const& liveKeys)
    {
        for (auto it = m_nonDefault.begin(); it != m_nonDefault.end();)
        {
            it = liveKeys.find(*it) == liveKeys.end() ? m_nonDefault.erase(it) : std::next(it);
        }
    }

    void RowExpansionModel::RetainOnlyUnder(
        std::unordered_set<winrt::hstring> const& liveKeys,
        std::unordered_set<winrt::hstring> const& enumeratedPrefixes,
        std::function<winrt::hstring(winrt::hstring const&)> const& parentPrefixOf)
    {
        if (!parentPrefixOf)
        {
            // Without the extractor there is no way to tell which keys the caller actually looked
            // at, so nothing can be dropped safely. Retaining everything leaks a little; falling
            // back to RetainOnly's whole-set semantics would silently delete intent for every
            // unenumerated subtree, which is the precise bug this overload exists to prevent.
            return;
        }

        for (auto it = m_nonDefault.begin(); it != m_nonDefault.end();)
        {
            if (liveKeys.find(*it) != liveKeys.end())
            {
                // The caller saw this key this pass. Keep it.
                it = std::next(it);
                continue;
            }

            // Absent from the live set means one of two very different things, and only the
            // caller's prefix report can tell them apart: some ancestor of the key WAS enumerated
            // and did not produce the branch leading to it (prune), or no enumerated ancestor
            // exists, so the key's absence is just laziness (keep -- it gets its chance next time
            // that part of the structure is walked).
            //
            // Testing the IMMEDIATE parent only is not enough, and the gap leaks without bound:
            // if A was enumerated and its child B has vanished, then intent for A/B/C is absent
            // and its immediate parent A/B was never enumerated either (B is gone), so C's intent
            // would survive every prune forever. Walking up finds A -- enumerated, and B is not
            // live -- which proves the whole branch died.
            it = IsProvablyDead(*it, liveKeys, enumeratedPrefixes, parentPrefixOf)
                ? m_nonDefault.erase(it)
                : std::next(it);
        }
    }

    bool RowExpansionModel::IsProvablyDead(
        winrt::hstring const& key,
        std::unordered_set<winrt::hstring> const& liveKeys,
        std::unordered_set<winrt::hstring> const& enumeratedPrefixes,
        std::function<winrt::hstring(winrt::hstring const&)> const& parentPrefixOf)
    {
        // Walk key -> parent -> grandparent, looking for the first ancestor-or-self that the caller
        // demonstrably looked FOR and did not find: its own parent prefix was enumerated, yet it is
        // not live. That ancestor is gone, so everything below it is gone with it.
        winrt::hstring current = key;

        // The walk is bounded independently of the extractor's behaviour. A malformed or adversarial
        // key format that never reduces to a fixed point must not spin here.
        for (int32_t depth = 0; depth < c_maxPrefixWalk; ++depth)
        {
            winrt::hstring parent;
            try
            {
                parent = parentPrefixOf(current);
            }
            catch (...)
            {
                // A throwing extractor cannot be allowed to decide that a key is dead. Treat it as
                // "not looked at", which is the non-destructive answer.
                return false;
            }

            if (enumeratedPrefixes.find(parent) != enumeratedPrefixes.end() &&
                liveKeys.find(current) == liveKeys.end())
            {
                return true;
            }

            // A fixed point (or an empty result) means the extractor has run out of ancestors --
            // `current` is a root in the caller's format. Nothing above it left to consult.
            if (parent.empty() || parent == current)
            {
                return false;
            }

            current = parent;
        }

        return false;
    }

    void RowExpansionModel::Clear()
    {
        if (m_nonDefault.empty())
        {
            return;
        }

        m_nonDefault.clear();

        Change change;
        change.AffectsAllKeys = true;
        change.IsExpanded = m_defaultExpanded;
        RaiseChanged(std::move(change));
    }

    void RowExpansionModel::RaiseChanged(Change change) const
    {
        if (m_changed)
        {
            m_changed(change);
        }
    }
}
