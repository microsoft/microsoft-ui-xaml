// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CalendarLayoutStrategyImpl.h>

#include "CalendarLayoutStrategyUnitTests.h"
#include "FakeLayoutDataInfoProvider.h"
#include "OrientationBasedMeasures.h"

using namespace DirectUI::Components::Moco;

#undef max
#undef min

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

#pragma region ILayoutStrategy.GetElementMeasureSize validation.

void CalendarLayoutStrategyUnitTests::CanCalculateMeasureSizeForContainer()
{

    CalendarLayoutStrategyImpl layoutStrategy;
    int rows = 6;
    int cols = 7;
    const wf::Rect windowConstraint = { 0.0f, 0.0f, 400.0f, 800.0f };
    const wf::Size viewportSize = { 100.0f, 100.0f };
    const wf::Size itemMinimumSize = { 50.0f, 50.0f };
    layoutStrategy.SetRows(rows);
    layoutStrategy.SetCols(cols);

    // No minimun size set.
    {
        const wf::Size actualMeasureSize = layoutStrategy.GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, 0, windowConstraint);
        VERIFY_ARE_EQUAL(actualMeasureSize.Width, 1.);
        VERIFY_ARE_EQUAL(actualMeasureSize.Height, 1.);
    }

    // viewport size set.
    {
        bool needsMeasure = false;
        layoutStrategy.SetViewportSize(viewportSize, &needsMeasure);

        VERIFY_IS_TRUE(needsMeasure);

        const wf::Size actualMeasureSize = layoutStrategy.GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, 0, windowConstraint);
        VERIFY_ARE_EQUAL(viewportSize.Width / cols, actualMeasureSize.Width);
        VERIFY_ARE_EQUAL(viewportSize.Height / rows, actualMeasureSize.Height);
    }

    // item minimum size set
    {
        bool needsMeasure = false;
        layoutStrategy.SetItemMinimumSize(itemMinimumSize, &needsMeasure);
        VERIFY_IS_TRUE(needsMeasure);

        const wf::Size actualMeasureSize = layoutStrategy.GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, 0, windowConstraint);
        VERIFY_ARE_EQUAL(std::max(viewportSize.Width / cols, itemMinimumSize.Width), actualMeasureSize.Width);
        VERIFY_ARE_EQUAL(std::max(viewportSize.Height / rows, itemMinimumSize.Height), actualMeasureSize.Height);
    }

}

#pragma endregion

#pragma region ILayoutStrategy.GetElementBounds validation.
void CalendarLayoutStrategyUnitTests::CanCalculateBoundsForContainer()
{
    int rows = 6;
    int cols = 7;
    const wf::Rect windowConstraint = { 0.0f, 0.0f, 400.0f, 800.0f };
    const wf::Size viewportSize = { 100.0f, 100.0f };
    const wf::Size itemMinimumSize = { 50.0f, 50.0f };
    int totalItemCount = 200;
    std::array<int, 3> itemIndices{ { 0, 24, 101 } };

    wrl::ComPtr<FakeLayoutDataInfoProvider> spDataInfoProvider;
    THROW_IF_FAILED(wrl::MakeAndInitialize<FakeLayoutDataInfoProvider>(&spDataInfoProvider));
    spDataInfoProvider->SetData(totalItemCount);

    for (const OrientationBasedMeasures& o : OrientationBasedMeasures::GetOrientations())
    {
        CalendarLayoutStrategyImpl layoutStrategy;
        layoutStrategy.SetLayoutDataInfoProviderNoRef(spDataInfoProvider.Get());
        layoutStrategy.SetVirtualizationDirection(o.GetOrientation());
        layoutStrategy.SetRows(rows);
        layoutStrategy.SetCols(cols);
        bool needsRemeasure = false;

        layoutStrategy.SetItemMinimumSize(itemMinimumSize, &needsRemeasure);

        for (int index : itemIndices)
        {
            const wf::Rect actualBounds = layoutStrategy.GetElementBounds(
                xaml_controls::ElementType_ItemContainer,
                index,
                wf::Size() /*unused*/,
                xaml_controls::LayoutReference()/*unused*/,
                windowConstraint);
            
            int stackLine = index % (o.GetOrientation() == xaml_controls::Orientation::Orientation_Vertical ? cols : rows);
            int virtualizatonLine = index / (o.GetOrientation() == xaml_controls::Orientation::Orientation_Vertical ? cols : rows);

            VERIFY_ARE_EQUAL(o.VirtualizingSize(itemMinimumSize) * virtualizatonLine, o.VirtualizingOffset(actualBounds));
            VERIFY_ARE_EQUAL(o.NonVirtualizingSize(itemMinimumSize) * stackLine, o.NonVirtualizingOffset(actualBounds));
            VERIFY_ARE_EQUAL(o.VirtualizingSize(itemMinimumSize), o.VirtualizingSize(actualBounds));
            VERIFY_ARE_EQUAL(o.NonVirtualizingSize(itemMinimumSize), o.NonVirtualizingSize(actualBounds));
        }
    }
}

#pragma endregion

#pragma region ILayoutStrategy.GetElementArrangeBounds validation.

void CalendarLayoutStrategyUnitTests::CanCalculateElementArrangeBounds()
{
    int rows = 6;
    int cols = 7;
    const wf::Rect windowConstraint = { 0.0f, 0.0f, 400.0f, 800.0f };
    const wf::Size viewportSize = { 100.0f, 100.0f };
    const wf::Size itemMinimumSize = { 50.0f, 50.0f };
    int totalItemCount = 200;
    std::array<int, 3> itemIndices{ { 0, 24, 101 } };

    wrl::ComPtr<FakeLayoutDataInfoProvider> spDataInfoProvider;
    THROW_IF_FAILED(wrl::MakeAndInitialize<FakeLayoutDataInfoProvider>(&spDataInfoProvider));
    spDataInfoProvider->SetData(totalItemCount);

    for (const OrientationBasedMeasures& o : OrientationBasedMeasures::GetOrientations())
    {
        CalendarLayoutStrategyImpl layoutStrategy;
        layoutStrategy.SetLayoutDataInfoProviderNoRef(spDataInfoProvider.Get());
        layoutStrategy.SetVirtualizationDirection(o.GetOrientation());
        layoutStrategy.SetRows(rows);
        layoutStrategy.SetCols(cols);
        bool needsRemeasure = false;

        layoutStrategy.SetItemMinimumSize(itemMinimumSize, &needsRemeasure);

        for (int index : itemIndices)
        {
            const wf::Rect measureBounds = layoutStrategy.GetElementBounds(
                xaml_controls::ElementType_ItemContainer,
                index,
                wf::Size() /*unused*/,
                xaml_controls::LayoutReference()/*unused*/,
                windowConstraint);

            const wf::Rect actualBounds = layoutStrategy.GetElementArrangeBounds(
                xaml_controls::ElementType_ItemContainer,
                index,
                measureBounds,
                windowConstraint,
                wf::Size() /*unused*/);
            
 
            VERIFY_ARE_EQUAL(measureBounds.X, actualBounds.X);
            VERIFY_ARE_EQUAL(measureBounds.Y, actualBounds.Y);
            VERIFY_ARE_EQUAL(measureBounds.Width, actualBounds.Width);
            VERIFY_ARE_EQUAL(measureBounds.Height, actualBounds.Height);
        }
    }
}

#pragma endregion

#pragma region ILayoutStrategy.HasSnapPointOnElement validation.

void CalendarLayoutStrategyUnitTests::CanHasSnapPointOnElement()
{
    CalendarLayoutStrategyImpl layoutStrategy;

    VERIFY_IS_FALSE(layoutStrategy.HasIrregularSnapPoints(xaml_controls::ElementType_ItemContainer));

    layoutStrategy.SetSnapPointFilterFunction(
        [](int index, bool* pHasSnapPoint)
    {
        *pHasSnapPoint = index % 7 == 0;
        return S_OK;
    });

    VERIFY_IS_TRUE(layoutStrategy.HasIrregularSnapPoints(xaml_controls::ElementType_ItemContainer));

    bool result = false;
    VERIFY_SUCCEEDED(layoutStrategy.HasSnapPointOnElement(xaml_controls::ElementType_ItemContainer, 0, result));
    VERIFY_IS_TRUE(result);
    
    VERIFY_SUCCEEDED(layoutStrategy.HasSnapPointOnElement(xaml_controls::ElementType_ItemContainer, 7, result));
    VERIFY_IS_TRUE(result);

    VERIFY_SUCCEEDED(layoutStrategy.HasSnapPointOnElement(xaml_controls::ElementType_ItemContainer, 1, result));
    VERIFY_IS_FALSE(result);

    VERIFY_SUCCEEDED(layoutStrategy.HasSnapPointOnElement(xaml_controls::ElementType_ItemContainer, 2, result));
    VERIFY_IS_FALSE(result);

    layoutStrategy.SetSnapPointFilterFunction(nullptr);

    VERIFY_IS_FALSE(layoutStrategy.HasIrregularSnapPoints(xaml_controls::ElementType_ItemContainer));
}

#pragma endregion


void CalendarLayoutStrategyUnitTests::VerifyIndexCorrection()
{
    int rows = 6;
    int cols = 7;
    const wf::Rect windowConstraint = { 0.0f, 0.0f, 400.0f, 800.0f };
    const wf::Size viewportSize = { 100.0f, 100.0f };
    const wf::Size itemMinimumSize = { 1.0f, 10.0f };
    int totalItemCount = 200;

    std::array<int, 4> actualIndices    { { 0, 24, 60, 101 } };

    std::array<int, 4> visualIndices    { { 2, 26, 65, 106 } };

    wrl::ComPtr<FakeLayoutDataInfoProvider> spDataInfoProvider;
    THROW_IF_FAILED(wrl::MakeAndInitialize<FakeLayoutDataInfoProvider>(&spDataInfoProvider));
    spDataInfoProvider->SetData(totalItemCount);

    for (const OrientationBasedMeasures& o : OrientationBasedMeasures::GetOrientations())
    {
        CalendarLayoutStrategyImpl layoutStrategy;
        layoutStrategy.SetLayoutDataInfoProviderNoRef(spDataInfoProvider.Get());
        layoutStrategy.SetVirtualizationDirection(o.GetOrientation());
        layoutStrategy.SetRows(rows);
        layoutStrategy.SetCols(cols);
        // item# [0, +Inf) while be shifted by 2 cells
        // item# [25, +Inf) will be shifted by 3 more cells.
        layoutStrategy.GetIndexCorrectionTable().SetCorrectionEntryForElementStartAt(2);
        layoutStrategy.GetIndexCorrectionTable().SetCorrectionEntryForSkippedDay(25, 3);

        bool needsRemeasure = false;

        layoutStrategy.SetItemMinimumSize(itemMinimumSize, &needsRemeasure);

        // verify ActualIndexToVisualIndex
        for (unsigned i = 0; i < actualIndices.size(); i++)
        {
            int index = actualIndices[i];
            int correctedIndex = visualIndices[i];
            const wf::Rect actualBounds = layoutStrategy.GetElementBounds(
                xaml_controls::ElementType_ItemContainer,
                index,
                wf::Size() /*unused*/,
                xaml_controls::LayoutReference()/*unused*/,
                windowConstraint);

            int stackLine = correctedIndex % (o.GetOrientation() == xaml_controls::Orientation::Orientation_Vertical ? cols : rows);
            int virtualizatonLine = correctedIndex / (o.GetOrientation() == xaml_controls::Orientation::Orientation_Vertical ? cols : rows);

            VERIFY_ARE_EQUAL(o.VirtualizingSize(itemMinimumSize) * virtualizatonLine, o.VirtualizingOffset(actualBounds));
            VERIFY_ARE_EQUAL(o.NonVirtualizingSize(itemMinimumSize) * stackLine, o.NonVirtualizingOffset(actualBounds));
            VERIFY_ARE_EQUAL(o.VirtualizingSize(itemMinimumSize), o.VirtualizingSize(actualBounds));
            VERIFY_ARE_EQUAL(o.NonVirtualizingSize(itemMinimumSize), o.NonVirtualizingSize(actualBounds));
        }

        // verify VisualIndexToActualIndex
        for (unsigned i = 0; i < actualIndices.size(); i++)
        {
            int index = actualIndices[i];
            int correctedIndex = visualIndices[i];
            int stackLine = correctedIndex % (o.GetOrientation() == xaml_controls::Orientation::Orientation_Vertical ? cols : rows);
            int virtualizatonLine = correctedIndex / (o.GetOrientation() == xaml_controls::Orientation::Orientation_Vertical ? cols : rows);

            wf::Rect actualBound = {};
            if (o.IsHorizontal())
            {
                actualBound.Width = o.VirtualizingSize(itemMinimumSize);
                actualBound.Height = o.NonVirtualizingSize(itemMinimumSize);
                actualBound.X = actualBound.Width * virtualizatonLine;
                actualBound.Y = actualBound.Height * stackLine;
            }
            else
            {
                actualBound.Height = o.VirtualizingSize(itemMinimumSize);
                actualBound.Width = o.NonVirtualizingSize(itemMinimumSize);
                actualBound.Y = actualBound.Height * virtualizatonLine;
                actualBound.X = actualBound.Width * stackLine;
            }
            wf::Rect bound = {};
            int actualIndex = -1;
            VERIFY_SUCCEEDED(layoutStrategy.EstimateElementIndex(
                xaml_controls::ElementType_ItemContainer,
                xaml_controls::EstimationReference()/*unused*/,
                xaml_controls::EstimationReference()/*unused*/,
                actualBound,
                &bound,
                &actualIndex));
            VERIFY_ARE_EQUAL(actualIndex, index);
        }
    }

}

} } } } }
