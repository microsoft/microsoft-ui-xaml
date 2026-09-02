// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapingPipeline.h"

#include <algorithm>
#include <string>
#include <utility>

namespace TabularShapingHelpers
{
    using SortDirection = winrt::SortDirection;

    winrt::hstring ShapingPipeline::MintDescriptionId(wchar_t prefix)
    {
        return winrt::hstring{ std::wstring{ prefix } + std::to_wstring(m_nextDescriptionId++) };
    }

    void ShapingPipeline::SetFilter(TabularPredicate const& predicate)
    {
        SetFilter(winrt::hstring{}, predicate);
    }

    void ShapingPipeline::SetFilter(winrt::hstring const& axisToken, TabularPredicate const& predicate)
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

    int32_t ShapingPipeline::MarkGroupVerb(TabularKeySelector const& key)
    {
        m_groupKey = key;
        m_groupDescriptionId = MintDescriptionId(L'g');
        return m_groupOrder = NextVerbOrder();
    }

    void ShapingPipeline::ClearGroupVerb() noexcept
    {
        m_groupOrder = -1;
        m_groupKey = nullptr;
        m_groupDescriptionId = {};
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

        if (m_groupOrder >= 0)
        {
            spec.Groups.push_back(GroupDescription{ m_groupDescriptionId, m_groupKey });
        }

        // ActiveSortAxes, not m_sorts: it applies exactly the filtering (live key, live direction)
        // and yields exactly the order that ApplySort uses, so the spec cannot describe a
        // different projection than the pipeline produces.
        for (auto const& axis : ActiveSortAxes(-1, -1))
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
        // compose into a single predicate rather than into N successive erase passes.
        ApplyPredicateFilter(
            rows,
            [this](winrt::IInspectable const& item)
            {
                for (auto const& axis : m_filters)
                {
                    if (!axis.Predicate(item))
                    {
                        return false;
                    }
                }

                return true;
            });
    }

    void ShapingPipeline::SetSort(
        winrt::hstring const& previousAxisToken,
        winrt::hstring const& axisToken,
        TabularKeySelector const& key,
        winrt::Windows::Foundation::IUnknown const& keyIdentity,
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
            m_sorts.push_back({ axisToken, key, keyIdentity, direction, NextVerbOrder(), MintDescriptionId(L's') });
        }
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

    std::vector<SortAxis> ShapingPipeline::ActiveSortAxes(int32_t afterOrder, int32_t beforeOrder) const
    {
        std::vector<SortAxis> active;
        for (auto const& axis : m_sorts)
        {
            const bool afterMatches = afterOrder < 0 || axis.Order > afterOrder;
            const bool beforeMatches = beforeOrder < 0 || axis.Order < beforeOrder;
            if (axis.Key && axis.Direction != SortDirection::None && afterMatches && beforeMatches)
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

    void ShapingPipeline::ApplySort(std::vector<winrt::IInspectable>& rows, int32_t afterOrder, int32_t beforeOrder) const
    {
        auto const active = ActiveSortAxes(afterOrder, beforeOrder);
        if (active.empty())
        {
            return;
        }

        StableSortByKeys(
            rows,
            active.size(),
            [&active](winrt::IInspectable const& item, size_t axisIndex) -> winrt::IInspectable
            {
                try
                {
                    return active[axisIndex].Key(item);
                }
                catch (...)
                {
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
        winrt::IInspectable const& row)
    {
        for (size_t i = 0; i < axes.size(); ++i)
        {
            auto const& axis = axes[i];
            winrt::IInspectable rowKey{ nullptr };
            try { rowKey = axis.Key(row); } catch (...) { rowKey = nullptr; }
            const int cmp = TabularValueComparer::Compare(itemKeys[i], rowKey);
            if (cmp != 0)
            {
                return axis.Direction == SortDirection::Ascending ? cmp : -cmp;
            }
        }
        return 0;
    }

    int ShapingPipeline::CompareItemToRow(winrt::IInspectable const& item, winrt::IInspectable const& row) const
    {
        auto const active = ActiveSortAxes(-1, -1);
        std::vector<winrt::IInspectable> itemKeys;
        itemKeys.reserve(active.size());
        for (auto const& axis : active)
        {
            winrt::IInspectable key{ nullptr };
            try { key = axis.Key(item); } catch (...) { key = nullptr; }
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
        auto const active = ActiveSortAxes(-1, -1);
        std::vector<winrt::IInspectable> itemKeys;
        itemKeys.reserve(active.size());
        for (auto const& axis : active)
        {
            winrt::IInspectable key{ nullptr };
            try { key = axis.Key(item); } catch (...) { key = nullptr; }
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
