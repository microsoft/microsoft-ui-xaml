// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Windows.Foundation.h>

#include "ShapingDescriptions.h"
#include "ShapingHelpers.h"

// Layer 2 of the Tabular shaping stack: the engine that owns the *current* set of shaping verbs
// and applies them to a materialized row vector.
//
// Layer 1 (ShapingDescriptions) says what a shape is and how to diff two of them. This layer
// owns the mutable, ordered accumulation of verbs a fluent caller builds up over time -- which
// sort axes are active, in what order they were declared relative to each other and to the group
// verb, and which filter predicates are in force -- and turns that into the two operations
// every projection needs: filter a vector, and sort a vector (or a slice of the axes).
//
// It holds no collection, no dispatcher, no notification state and no tabular vocabulary. The
// owner keeps the projection; this keeps the recipe. That split is what lets the projection layer
// above be about incremental change and marshaling rather than about shaping.
namespace ShapingHelpers
{
    // One active sort axis. Storage order is declaration order; PRECEDENCE is by Order, earliest
    // declared first -- see ActiveSortAxes.
    struct SortAxis
    {
        // Identifies the axis for replace/clear (e.g. a column token). Empty is legal and means
        // the axis is identified only by its evaluator.
        winrt::hstring AxisToken;
        KeySelector Key{ nullptr };
        // Stable identity of the evaluator, used to match axes when AxisToken is empty.
        // std::function is not equality-comparable, so a caller that wants un-tokenized replace
        // semantics supplies the underlying delegate here as an IUnknown. Comparing the
        // interface (rather than a raw address) keeps the original delegate-equality semantics,
        // which match two different interface pointers on the same COM object.
        winrt::Windows::Foundation::IUnknown KeyIdentity{ nullptr };
        // The property path this axis sorts on, when the caller named one. Empty means the key is
        // a delegate that no path expresses (computed, multi-field, identity, or a ranked custom
        // comparer). Purely descriptive - the pipeline never evaluates it - but it is what lets a
        // consumer read an axis back and say which column it is about.
        winrt::hstring SortMemberPath;
        winrt::SortDirection Direction{ winrt::SortDirection::None };
        // Position in the single verb sequence shared with the group verb. Sorts declared before
        // GroupBy order the groups; sorts declared after it order rows within a group.
        int32_t Order{};
        // Stable diff identity of this axis, minted whenever the axis is declared or re-declared.
        // A layer-1 SortDescription is compared by PropertyName, and the fluent surface offers no
        // property path to compare, so the pipeline supplies one. It is deliberately NOT the
        // AxisToken: the same token can be re-declared at the same direction carrying a different
        // key delegate, which naming by token would diff as "unchanged". Re-minting per
        // declaration makes the diff conservative in the safe direction — it can report a change
        // that turned out to be identical, never the reverse.
        winrt::hstring DescriptionId;
    };

    // One active filter axis. All axes are conjunctive and all of them run before any grouping
    // or sorting, so unlike SortAxis there is no verb Order here: declaration order is only a
    // tie-break for evaluation cost, never for meaning.
    struct FilterAxis
    {
        // Identifies the axis for replace/clear (e.g. a column token). Empty is legal and is the
        // single-filter shorthand's axis, so a caller that never names its filters keeps exactly
        // the old one-predicate behaviour.
        winrt::hstring AxisToken;
        Predicate Predicate{ nullptr };
        // Stable diff identity of this axis, re-minted on every declaration. A predicate's
        // parameters (operator, threshold, selected values) live inside the delegate and are
        // invisible to a diff, so without this the commonest mutation -- same column, different
        // threshold -- would diff as "no change" and silently keep the old membership.
        winrt::hstring CriterionId;
    };

    class ShapingPipeline
    {
    public:
        // -- filter ---------------------------------------------------------------------------
        // Filters are conjunctive and always run before sort/group shaping. Axes are identified
        // by token so a caller with several independent filter sources (a column filter and a
        // search box, say) can declare and retract them independently.
        //
        // The untokenized overload is the single-filter shorthand: it declares the empty-token
        // axis, so calling it repeatedly replaces one predicate exactly as it always did.
        void SetFilter(Predicate const& predicate);
        // A null predicate removes the axis rather than declaring an always-false one.
        void SetFilter(winrt::hstring const& axisToken, Predicate const& predicate);
        void ClearFilter() noexcept;
        void ClearFilter(winrt::hstring const& axisToken);
        bool HasFilter() const noexcept { return !m_filters.empty(); }
        // A predicate that throws excludes the item (fail-safe). Returns true when no filter is
        // set, so callers can gate unconditionally.
        bool PassesFilter(winrt::IInspectable const& item) const;
        void ApplyFilter(std::vector<winrt::IInspectable>& rows) const;

        // -- sort -----------------------------------------------------------------------------
        // Declares or replaces a sort axis. When previousAxisToken is non-empty and differs from
        // axisToken, axes carrying it are dropped first (a column re-keying itself). A direction
        // of None removes the axis instead of adding it. sortMemberPath is descriptive only and
        // may be empty; see SortAxis::SortMemberPath.
        void SetSort(
            winrt::hstring const& previousAxisToken,
            winrt::hstring const& axisToken,
            KeySelector const& key,
            winrt::Windows::Foundation::IUnknown const& keyIdentity,
            winrt::hstring const& sortMemberPath,
            winrt::SortDirection direction);
        void ClearSorts() noexcept { m_sorts.clear(); }
        void ClearSort(winrt::hstring const& axisToken);
        // Drops every axis EXCEPT the one carrying axisToken. An untokenized axis (empty
        // AxisToken) can only be cleared this way: ClearSort("") means clear-all, so a caller
        // holding one token cannot address the axes it does not own by token alone.
        void ClearSortsExcept(winrt::hstring const& axisToken) noexcept;
        bool HasActiveSort() const noexcept;

        // Axes with a live key and direction whose Order falls strictly between the bounds. A
        // negative bound is unbounded, so ActiveSortAxes(-1, -1) is every active axis. Returned in
        // PRECEDENCE order -- earliest declared first -- so index 0 is the primary sort.
        std::vector<SortAxis> ActiveSortAxes(int32_t afterOrder, int32_t beforeOrder) const;
        void ApplySort(std::vector<winrt::IInspectable>& rows, int32_t afterOrder = -1, int32_t beforeOrder = -1) const;

        // Where a newly-arrived row belongs, plus whether the sort alone actually decides that.
        struct SortedInsertPlacement
        {
            uint32_t Index{ 0 };
            // True when a row already in the projection compares equal on EVERY active axis, so
            // the sort keys alone do not determine where the new row goes: the tie has to be
            // broken by source order, which the projection does not carry. A caller that cannot
            // establish the source order of the tie group must rebuild rather than guess.
            bool TiedWithExistingRow{ false };
        };

        SortedInsertPlacement SortedInsertPlacementFor(
            winrt::IInspectable const& item,
            uint32_t count,
            std::function<winrt::IInspectable(uint32_t index)> const& getRow) const;

        // Compares `item` against `row` on the active axes alone, applying each axis's direction.
        // 0 means the two are indistinguishable to the sort, i.e. their relative order is decided
        // by the stable sort's source-order tiebreak rather than by any key.
        int CompareItemToRow(winrt::IInspectable const& item, winrt::IInspectable const& row) const;

        // -- verb ordering --------------------------------------------------------------------
        int32_t NextVerbOrder() noexcept { return m_nextVerbOrder++; }
        // Places the group verb at the end of the current verb sequence and returns its order.
        // The key is optional and is carried only so the spec can describe the grouping axis;
        // this class never evaluates it.
        int32_t MarkGroupVerb(KeySelector const& key = nullptr);
        void ClearGroupVerb() noexcept;
        int32_t GroupOrder() const noexcept { return m_groupOrder; }

        // -- spec -----------------------------------------------------------------------------
        // The layer-1 description of the verbs currently held: what shape is wanted, expressed as
        // values rather than as accumulated calls. Sort axes appear in the same order ApplySort
        // applies them, so a spec and this pipeline always describe the same projection.
        //
        // Every description carries a minted PropertyName rather than a real property path. The
        // fluent surface takes delegates, not paths, so there is nothing else to name an axis by;
        // a minted id keeps the spec diffable (an empty name would force a full reshape for any
        // change, which is exactly the outcome the diff exists to avoid) and stays honest because
        // the id changes whenever the underlying delegate is re-declared.
        ShapingSpec BuildSpec() const;

        // Diffs the current verbs against the spec adopted by the previous call, adopts the new
        // one, and returns the work the change requires. The first call always reports a full
        // reshape, since there is no prior shape to have moved from.
        ShapingDelta CommitSpec();

        ShapingSpec const& CommittedSpec() const noexcept { return m_committedSpec; }

    private:
        // Mints a process-unique-per-pipeline description id, e.g. "s3" for the fourth axis
        // declared. Only ever compared for equality, never parsed.
        winrt::hstring MintDescriptionId(wchar_t prefix);


        // Compares an item's precomputed axis keys against a row whose keys are evaluated on the
        // fly. Lets the binary search hoist axis filtering and item-key evaluation out of the loop.
        static int CompareKeysToRow(
            std::vector<SortAxis> const& axes,
            std::vector<winrt::IInspectable> const& itemKeys,
            winrt::IInspectable const& row);

        // Declaration-ordered filter axes, all conjunctive.
        std::vector<FilterAxis> m_filters;
        std::vector<SortAxis> m_sorts;
        int32_t m_nextVerbOrder{ 0 };
        int32_t m_groupOrder{ -1 };
        // Diff identity and evaluator of the group verb, held only so BuildSpec can describe it.
        winrt::hstring m_groupDescriptionId;
        KeySelector m_groupKey{ nullptr };
        ShapingSpec m_committedSpec;
        // False until the first CommitSpec. Distinguishes "committed an empty spec" (a source
        // with no verbs, where a later verb is a real change) from "never committed" (where the
        // caller has no projection yet and must do the full reshape regardless).
        bool m_hasCommittedSpec{ false };
        uint32_t m_nextDescriptionId{ 0 };
    };
}
