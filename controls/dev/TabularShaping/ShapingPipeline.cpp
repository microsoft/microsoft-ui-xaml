// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapingPipeline.h"

#include <algorithm>
#include <string>
#include <utility>

namespace ShapingHelpers
{
    using SortDirection = winrt::SortDirection;

    winrt::hstring ShapingPipeline::MintDescriptionId(wchar_t prefix)
    {
        return winrt::hstring{ std::wstring{ prefix } + std::to_wstring(m_nextDescriptionId++) };
    }

    void ShapingPipeline::SetFilter(Predicate const& predicate)
    {
        SetFilter(winrt::hstring{}, predicate);
    }

    void ShapingPipeline::SetFilter(winrt::hstring const& axisToken, Predicate const& predicate)
    {
        if (!predicate)
        {
            ClearFilter(axisToken);
            return;
        }

        auto const existing = std::find_if(
            m_filters.begin(),
            m_filters.end(),
            [&axisToken](FilterAxis const& axis) { return axis.AxisToken == axisToken; });

        // Re-minted even when replacing in place: the delegate cannot reveal that its threshold
        // moved, so a re-declaration must always read as a change.
        auto const criterionId = MintDescriptionId(L'f');

        if (existing != m_filters.end())
        {
            existing->Predicate = predicate;
            existing->CriterionId = criterionId;
            return;
        }

        m_filters.push_back(FilterAxis{ axisToken, predicate, criterionId });
    }

    void ShapingPipeline::ClearFilter() noexcept
    {
        m_filters.clear();
    }

    void ShapingPipeline::ClearFilter(winrt::hstring const& axisToken)
    {
        m_filters.erase(
            std::remove_if(
                m_filters.begin(),
                m_filters.end(),
                [&axisToken](FilterAxis const& axis) { return axis.AxisToken == axisToken; }),
            m_filters.end());
    }

    void ShapingPipeline::MarkGroupVerb(KeySelector const& key)
    {
        m_groupKey = key;
        m_groupDescriptionId = MintDescriptionId(L'g');
        m_hasGroup = true;
    }

    void ShapingPipeline::ClearGroupVerb() noexcept
    {
        m_hasGroup = false;
        m_groupKey = nullptr;
        m_groupDescriptionId = {};
    }

    void ShapingPipeline::RestoreDeclaration(ShapingPipeline const& snapshot)
    {
        auto const faults = m_swallowedFaults;
        auto const nextDescriptionId = m_nextDescriptionId;
        auto const nextVerbOrder = m_nextVerbOrder;
        *this = snapshot;
        m_swallowedFaults = faults;
        m_nextDescriptionId = nextDescriptionId;
        m_nextVerbOrder = nextVerbOrder;
    }

    ShapingSpec ShapingPipeline::BuildSpec() const
    {
        ShapingSpec spec;

        for (auto const& axis : m_filters)
        {
            // The untokenized shorthand has no column notion to name, so it keeps the synthetic
            // path it always had; a named axis is described by its own token, which is what lets
            // two filter axes diff independently. Either way CriterionId, not the path, is what
            // distinguishes two criteria on the same column.
            spec.Filters.push_back(FilterDescription{
                axis.AxisToken.empty() ? winrt::hstring{ L"(predicate)" } : axis.AxisToken,
                axis.CriterionId,
                axis.Predicate });
        }

        if (m_hasGroup)
        {
            spec.Groups.push_back(GroupDescription{ m_groupDescriptionId, m_groupKey, m_groupSortDirection });
        }

        // ActiveSortAxes, not m_sorts: it applies exactly the filtering (live key, live direction)
        // and yields exactly the order that ApplySort uses, so the spec cannot describe a
        // different projection than the pipeline produces.
        for (auto const& axis : ActiveSortAxes())
        {
            // Named by the minted id, NOT by the AxisToken. A column can re-declare the same
            // token at the same direction while supplying a DIFFERENT key delegate — a custom
            // comparer whose ranks were just repopulated does exactly that — and naming the
            // description by the token would diff that as "unchanged" and silently skip the
            // re-sort the caller asked for. The id is re-minted by every declaration, so a
            // re-declaration always reads as a change: conservative in the safe direction.
            spec.Sorts.push_back(SortDescription{ axis.DescriptionId, axis.Direction, axis.Key });
        }

        return spec;
    }

    ShapingDelta ShapingPipeline::CommitSpec()
    {
        auto next = BuildSpec();

        ShapingDelta delta;
        if (m_hasCommittedSpec)
        {
            delta = m_committedSpec.Diff(next);
        }
        else
        {
            delta.FilterChanged = true;
            delta.GroupingChanged = true;
            delta.SortChanged = true;
            delta.RequiredWork = ShapingWork::FullReshape;
            m_hasCommittedSpec = true;
        }

        m_committedSpec = std::move(next);
        return delta;
    }

    bool ShapingPipeline::PassesFilter(winrt::IInspectable const& item) const
    {
        // Conjunctive, and short-circuiting: an item excluded by the first axis is never handed
        // to the rest, so an expensive predicate declared later costs nothing on rows already out.
        for (auto const& axis : m_filters)
        {
            try
            {
                if (!axis.Predicate(item))
                {
                    return false;
                }
            }
            catch (...)
            {
                m_swallowedFaults.Note();
                return false;
            }
        }

        return true;
    }

    void ShapingPipeline::ApplyFilter(std::vector<winrt::IInspectable>& rows) const
    {
        if (m_filters.empty())
        {
            return;
        }

        // One pass over the rows regardless of axis count: the axes are conjunctive, so they
        // compose into a single predicate rather than into N successive erase passes. PassesFilter
        // also counts a predicate that throws.
        ApplyPredicateFilter(rows, [this](winrt::IInspectable const& item) { return PassesFilter(item); });
    }

    void ShapingPipeline::SetSort(
        winrt::hstring const& previousAxisToken,
        winrt::hstring const& axisToken,
        KeySelector const& key,
        winrt::Windows::Foundation::IUnknown const& keyIdentity,
        winrt::hstring const& sortMemberPath,
        SortDirection direction)
    {
        if (!previousAxisToken.empty() && previousAxisToken != axisToken)
        {
            ClearSort(previousAxisToken);
        }

        for (auto it = m_sorts.begin(); it != m_sorts.end(); ++it)
        {
            const bool sameAxis = axisToken.empty()
                ? (it->AxisToken.empty() && it->KeyIdentity && it->KeyIdentity == keyIdentity)
                : (it->AxisToken == axisToken);
            if (!sameAxis)
            {
                continue;
            }

            if (direction == SortDirection::None)
            {
                m_sorts.erase(it);
            }
            else
            {
                it->Key = key;
                it->KeyIdentity = keyIdentity;
                it->SortMemberPath = sortMemberPath;
                it->Direction = direction;
                // Match WPF DataGrid.DefaultSort: re-sorting an already-sorted axis replaces it
                // in place and keeps its existing precedence slot (Order is left untouched), rather
                // than moving the re-touched column. Since precedence is earliest-first, keeping
                // Order keeps a primary axis primary.
                // The evaluator was replaced wholesale, so the previous description no longer
                // describes this axis.
                it->DescriptionId = MintDescriptionId(L's');
            }
            return;
        }

        if (direction != SortDirection::None)
        {
            // A brand-new axis takes the next (largest) Order, which under earliest-first
            // precedence appends it as the least significant tie-break -- the analog of WPF's
            // SortDescriptions.Add.
            m_sorts.push_back({ axisToken, key, keyIdentity, sortMemberPath, direction, NextVerbOrder(), MintDescriptionId(L's') });
        }
    }

    void ShapingPipeline::ReplaceSorts(
        winrt::hstring const& axisToken,
        KeySelector const& key,
        winrt::hstring const& sortMemberPath,
        SortDirection direction)
    {
        m_sorts.clear();
        if (direction != SortDirection::None)
        {
            m_sorts.push_back({ axisToken, key, nullptr, sortMemberPath, direction, NextVerbOrder(), MintDescriptionId(L's') });
        }
    }

    void ShapingPipeline::ThenSort(
        winrt::hstring const& axisToken,
        KeySelector const& key,
        winrt::hstring const& sortMemberPath,
        SortDirection direction)
    {
        auto const existing = std::find_if(
            m_sorts.begin(),
            m_sorts.end(),
            [&](SortAxis const& axis)
            {
                return (!axisToken.empty() && axis.AxisToken == axisToken) ||
                    (!sortMemberPath.empty() && axis.SortMemberPath == sortMemberPath);
            });
        if (existing != m_sorts.end())
        {
            existing->Direction = direction;
            existing->DescriptionId = MintDescriptionId(L's');
            return;
        }

        // No key identity: a key-selector level always appends, even when the same delegate is passed again.
        m_sorts.push_back({ axisToken, key, nullptr, sortMemberPath, direction, NextVerbOrder(), MintDescriptionId(L's') });
    }

    void ShapingPipeline::ClearSort(winrt::hstring const& axisToken)
    {
        if (axisToken.empty())
        {
            ClearSorts();
            return;
        }

        m_sorts.erase(
            std::remove_if(
                m_sorts.begin(),
                m_sorts.end(),
                [&axisToken](SortAxis const& axis) { return axis.AxisToken == axisToken; }),
            m_sorts.end());
    }

    void ShapingPipeline::ClearSortsExcept(winrt::hstring const& axisToken) noexcept
    {
        if (axisToken.empty())
        {
            // No axis to preserve: an empty token identifies no single axis.
            ClearSorts();
            return;
        }

        m_sorts.erase(
            std::remove_if(
                m_sorts.begin(),
                m_sorts.end(),
                [&axisToken](SortAxis const& axis) { return axis.AxisToken != axisToken; }),
            m_sorts.end());
    }

    bool ShapingPipeline::HasActiveSort() const noexcept
    {
        for (auto const& axis : m_sorts)
        {
            if (axis.Key && axis.Direction != SortDirection::None)
            {
                return true;
            }
        }
        return false;
    }

    bool ShapingPipeline::HasSortAxis(winrt::hstring const& axisToken) const noexcept
    {
        return std::any_of(
            m_sorts.begin(),
            m_sorts.end(),
            [&axisToken](SortAxis const& axis) { return axisToken.empty() || axis.AxisToken == axisToken; });
    }

    bool ShapingPipeline::HasSortOtherThan(winrt::hstring const& axisToken) const noexcept
    {
        return std::any_of(
            m_sorts.begin(),
            m_sorts.end(),
            [&axisToken](SortAxis const& axis) { return axisToken.empty() || axis.AxisToken != axisToken; });
    }

    bool ShapingPipeline::HasMatchingSort(
        winrt::hstring const& axisToken,
        winrt::Windows::Foundation::IUnknown const& keyIdentity) const noexcept
    {
        return std::any_of(
            m_sorts.begin(),
            m_sorts.end(),
            [&](SortAxis const& axis)
            {
                return axisToken.empty() ?
                    (axis.AxisToken.empty() && axis.KeyIdentity && axis.KeyIdentity == keyIdentity) :
                    axis.AxisToken == axisToken;
            });
    }

    std::vector<SortAxis> ShapingPipeline::ActiveSortAxes() const
    {
        std::vector<SortAxis> active;
        for (auto const& axis : m_sorts)
        {
            if (axis.Key && axis.Direction != SortDirection::None)
            {
                active.push_back(axis);
            }
        }

        // Precedence is by declaration ORDER, earliest first -- matching WPF DataGrid, whose
        // SortDescriptions collection makes index 0 (the first column sorted) the primary sort and
        // each later column a tie-break. The axis with the SMALLEST Order is primary; a NEWLY
        // declared axis gets the largest Order (SetSort push_back) and is therefore appended as the
        // least significant tie-break, exactly like SortDescriptions.Add. Re-sorting an EXISTING
        // axis leaves its Order untouched (see SetSort), so its precedence slot is preserved --
        // matching WPF's in-place SortDescriptions replace. Consequence, also shared with WPF: when
        // the primary axis has unique keys, later axes never get to break a tie, so they have no
        // visible effect until the primary produces a tie.
        std::stable_sort(
            active.begin(),
            active.end(),
            [](SortAxis const& a, SortAxis const& b) { return a.Order < b.Order; });

        return active;
    }

    void ShapingPipeline::ApplySort(std::vector<winrt::IInspectable>& rows) const
    {
        auto const active = ActiveSortAxes();
        if (active.empty())
        {
            return;
        }

        StableSortByKeys(
            rows,
            active.size(),
            [this, &active](winrt::IInspectable const& item, size_t axisIndex) -> winrt::IInspectable
            {
                try
                {
                    return active[axisIndex].Key(item);
                }
                catch (...)
                {
                    m_swallowedFaults.Note();
                    return nullptr;
                }
            },
            [&active](size_t axisIndex)
            {
                return active[axisIndex].Direction;
            });
    }

    int ShapingPipeline::CompareKeysToRow(
        std::vector<SortAxis> const& axes,
        std::vector<winrt::IInspectable> const& itemKeys,
        winrt::IInspectable const& row) const
    {
        for (size_t i = 0; i < axes.size(); ++i)
        {
            auto const& axis = axes[i];
            winrt::IInspectable rowKey{ nullptr };
            try { rowKey = axis.Key(row); } catch (...) { m_swallowedFaults.Note(); rowKey = nullptr; }
            const int cmp = ValueComparer::Compare(itemKeys[i], rowKey);
            if (cmp != 0)
            {
                return axis.Direction == SortDirection::Ascending ? cmp : -cmp;
            }
        }
        return 0;
    }

    int ShapingPipeline::CompareItemToRow(winrt::IInspectable const& item, winrt::IInspectable const& row) const
    {
        auto const active = ActiveSortAxes();
        std::vector<winrt::IInspectable> itemKeys;
        itemKeys.reserve(active.size());
        for (auto const& axis : active)
        {
            winrt::IInspectable key{ nullptr };
            try { key = axis.Key(item); } catch (...) { m_swallowedFaults.Note(); key = nullptr; }
            itemKeys.push_back(key);
        }

        return CompareKeysToRow(active, itemKeys, row);
    }

    ShapingPipeline::SortedInsertPlacement ShapingPipeline::SortedInsertPlacementFor(
        winrt::IInspectable const& item,
        uint32_t count,
        std::function<winrt::IInspectable(uint32_t index)> const& getRow) const
    {
        // Hoist the active axes and the incoming item's keys out of the binary-search loop:
        // otherwise every step re-filters m_sorts (allocating a vector) and re-evaluates the
        // item's key selectors. The row's keys still must be evaluated per step.
        auto const active = ActiveSortAxes();
        std::vector<winrt::IInspectable> itemKeys;
        itemKeys.reserve(active.size());
        for (auto const& axis : active)
        {
            winrt::IInspectable key{ nullptr };
            try { key = axis.Key(item); } catch (...) { m_swallowedFaults.Note(); key = nullptr; }
            itemKeys.push_back(key);
        }

        SortedInsertPlacement placement;
        placement.Index = UpperBoundInsertIndex(
            count,
            [&](uint32_t mid) { return CompareKeysToRow(active, itemKeys, getRow(mid)); });

        // Equal-key rows are contiguous in a sorted projection and the upper bound lands just past
        // them, so the row immediately before the insertion point is the only one that has to be
        // probed to know whether the item joined a tie group.
        placement.TiedWithExistingRow =
            placement.Index > 0 &&
            CompareKeysToRow(active, itemKeys, getRow(placement.Index - 1)) == 0;

        return placement;
    }
}
