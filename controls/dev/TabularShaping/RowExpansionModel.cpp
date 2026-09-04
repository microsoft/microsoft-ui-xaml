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
