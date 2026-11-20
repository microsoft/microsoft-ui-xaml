// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <WrappingLayoutStrategyImpl.h>

#include "WrappingLayoutStrategyUnitTests.h"
#include "OrientationBasedMeasures.h"

#include <limits>

using namespace DirectUI::Components::Moco;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

void WrappingLayoutStrategyUnitTests::ItemsPerPageHonorsLimitWithUnboundedStackingExtent()
{
    const float infinity = std::numeric_limits<float>::infinity();

    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.SetMaximumRowsOrColumns(2);
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        const wf::Rect window = orientation.IsHorizontal()
            ? wf::Rect{ 0.0f, 0.0f, 350.0f, infinity }
            : wf::Rect{ 0.0f, 0.0f, infinity, 350.0f };

        VERIFY_ARE_EQUAL(8.0, strategy.GetItemsPerPage(window));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageRequiresLimitWithUnboundedStackingExtent()
{
    const float infinity = std::numeric_limits<float>::infinity();

    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        const wf::Rect window = orientation.IsHorizontal()
            ? wf::Rect{ 0.0f, 0.0f, 350.0f, infinity }
            : wf::Rect{ 0.0f, 0.0f, infinity, 350.0f };

        VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage(window));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageRequiresLimitAfterClearingMaximumRowsOrColumns()
{
    const float infinity = std::numeric_limits<float>::infinity();

    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        for (const int clearedLimit : { -1, 0 })
        {
            WrappingLayoutStrategyImpl strategy;
            strategy.SetVirtualizationDirection(orientation.GetOrientation());
            strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });
            strategy.SetMaximumRowsOrColumns(2);

            const wf::Rect unboundedWindow = orientation.IsHorizontal()
                ? wf::Rect{ 0.0f, 0.0f, 350.0f, infinity }
                : wf::Rect{ 0.0f, 0.0f, infinity, 350.0f };
            const wf::Rect finiteWindow = orientation.IsHorizontal()
                ? wf::Rect{ 0.0f, 0.0f, 350.0f, 600.0f }
                : wf::Rect{ 0.0f, 0.0f, 600.0f, 350.0f };

            VERIFY_ARE_EQUAL(8.0, strategy.GetItemsPerPage(unboundedWindow));

            strategy.SetMaximumRowsOrColumns(clearedLimit);
            VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage(unboundedWindow));
            VERIFY_ARE_EQUAL(24.0, strategy.GetItemsPerPage(finiteWindow));

            strategy.SetMaximumRowsOrColumns(2);
            VERIFY_ARE_EQUAL(8.0, strategy.GetItemsPerPage(unboundedWindow));
        }
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageHonorsLimitWithFiniteStackingExtent()
{
    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.SetMaximumRowsOrColumns(2);
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        const wf::Rect window = orientation.IsHorizontal()
            ? wf::Rect{ 0.0f, 0.0f, 350.0f, 600.0f }
            : wf::Rect{ 0.0f, 0.0f, 600.0f, 350.0f };

        VERIFY_ARE_EQUAL(8.0, strategy.GetItemsPerPage(window));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageCountsCompleteStackingLines()
{
    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        const wf::Rect window = orientation.IsHorizontal()
            ? wf::Rect{ 0.0f, 0.0f, 350.0f, 250.0f }
            : wf::Rect{ 0.0f, 0.0f, 250.0f, 350.0f };

        VERIFY_ARE_EQUAL(8.0, strategy.GetItemsPerPage(window));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageAccountsForGroupPaddingAndParallelHeader()
{
    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.SetGroupHeaderStrategy(DirectUI::GroupHeaderStrategy::Parallel);
        strategy.SetGroupPadding({ 25.0, 25.0, 25.0, 25.0 });
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });
        strategy.RegisterSpecialHeaderSize(0, { 100.0f, 100.0f });

        VERIFY_ARE_EQUAL(8.0, strategy.GetItemsPerPage({ 0.0f, 0.0f, 350.0f, 350.0f }));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPagePreservesFiniteUncappedEstimate()
{
    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        VERIFY_ARE_EQUAL(12.0, strategy.GetItemsPerPage({ 0.0f, 0.0f, 300.0f, 400.0f }));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageAllowsOneOversizedItem()
{
    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        const wf::Rect window = orientation.IsHorizontal()
            ? wf::Rect{ 0.0f, 0.0f, 350.0f, 50.0f }
            : wf::Rect{ 0.0f, 0.0f, 50.0f, 350.0f };

        VERIFY_ARE_EQUAL(4.0, strategy.GetItemsPerPage(window));
    }
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageRequiresMeasuredCell()
{
    WrappingLayoutStrategyImpl strategy;
    strategy.SetMaximumRowsOrColumns(2);

    VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage({ 0.0f, 0.0f, 350.0f, 600.0f }));
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageRequiresMeasuredGroupHeader()
{
    WrappingLayoutStrategyImpl strategy;
    strategy.SetGroupHeaderStrategy(DirectUI::GroupHeaderStrategy::Parallel);
    strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

    VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage({ 0.0f, 0.0f, 350.0f, 350.0f }));
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageRequiresNonemptyViewport()
{
    WrappingLayoutStrategyImpl strategy;
    strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

    VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage({ 0.0f, 0.0f, 350.0f, 0.0f }));
    VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage({ 0.0f, 0.0f, 0.0f, 350.0f }));
}

void WrappingLayoutStrategyUnitTests::ItemsPerPageRequiresFiniteVirtualizingExtent()
{
    const float infinity = std::numeric_limits<float>::infinity();

    for (const OrientationBasedMeasures& orientation : OrientationBasedMeasures::GetOrientations())
    {
        WrappingLayoutStrategyImpl strategy;
        strategy.SetVirtualizationDirection(orientation.GetOrientation());
        strategy.SetMaximumRowsOrColumns(2);
        strategy.RegisterSpecialContainerSize(0, { 100.0f, 100.0f });

        const wf::Rect window = orientation.IsHorizontal()
            ? wf::Rect{ 0.0f, 0.0f, infinity, 350.0f }
            : wf::Rect{ 0.0f, 0.0f, 350.0f, infinity };

        VERIFY_ARE_EQUAL(0.0, strategy.GetItemsPerPage(window));
    }
}

} } } } }
