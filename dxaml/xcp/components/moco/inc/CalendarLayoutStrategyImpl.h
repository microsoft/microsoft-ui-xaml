// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutStrategyBase.h"

namespace DirectUI { namespace Components { namespace Moco {

    class CalendarLayoutStrategyImpl
        : public LayoutStrategyBase
    {
    public:
        struct IndexCorrectionTable
        {
            void SetCorrectionEntryForSkippedDay(_In_ int index, _In_ int correction);
            void SetCorrectionEntryForElementStartAt(_In_ int correction);
            int VisualIndexToActualIndex(_In_ int visualIndex) const;
            int ActualIndexToVisualIndex(_In_ int actualIndex) const;
        private:
            // currently we have up to two index correction entries:
            //    1. MonthPanel can set ElementStartAt to push all items to the right
            //    2. MonthPanel can have up to one skipped day, where we should push all items after this date to the right
            // both correction should be positive number.
            std::array<std::pair<int, int>, 2> m_indexCorrectionTable;
        };

        CalendarLayoutStrategyImpl();

        // implementation of interface

        #pragma region Layout related methods
        
        void BeginMeasure() {}
        void EndMeasure() {}

        // returns the size we should use to measure a container or header with
        // itemIndex - indicates an index of valid item or -1 for general, non-special items
        wf::Size GetElementMeasureSize(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Rect windowConstraint);

        wf::Rect GetElementBounds(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Size containerDesiredSize,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowConstraint);

        wf::Rect GetElementArrangeBounds(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Rect containerBounds,
            _In_ wf::Rect windowConstraint,
            _In_ wf::Size finalSize);

        bool ShouldContinueFillingUpSpace(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowToFill);

        // CalendarPanel doesn't have a group
        wf::Point GetPositionOfFirstElement() { return { 0, 0 }; }

        #pragma endregion

        #pragma region Estimation and virtualization related methods.

        _Check_return_ HRESULT EstimateElementIndex(
            _In_ xaml_controls::ElementType elementType,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pTargetRect,
            _Out_ INT* pReturnValue);

        _Check_return_ HRESULT EstimateElementBounds(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pReturnValue);

        _Check_return_ HRESULT EstimatePanelExtent(
            _In_ xaml_controls::EstimationReference lastHeaderReference,
            _In_ xaml_controls::EstimationReference lastContainerReference,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Size* pExtent);

        #pragma endregion

        #pragma region IItemLookupPanel related

        // Based on current element's index/type and action, return the next element index/type.
        _Check_return_ HRESULT GetTargetIndexFromNavigationAction(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ wf::Rect windowConstraint,
            _Out_ xaml_controls::ElementType* pTargetElementType,
            _Out_ INT* pTargetIndex);

        #pragma endregion

        #pragma region Snap points related

        bool GetRegularSnapPoints(
            _Out_ float* pNearOffset,
            _Out_ float* pFarOffset,
            _Out_ float* pSpacing);

        bool HasIrregularSnapPoints(
            _In_ xaml_controls::ElementType elementType);

        _Check_return_ HRESULT HasSnapPointOnElement(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _Out_ bool& hasSnapPointOnElement);

        #pragma endregion

        // CalendarPanel don't have a special item.
        bool NeedsSpecialItem() const { return false; }
        int GetSpecialItemIndex() const { return c_specialItemIndex; }

        // set the viewport size and check if we need to remeasure (when item size changed).
        void SetViewportSize(_In_ wf::Size size, _Out_ bool* pNeedsRemeasure);

        void SetItemMinimumSize(_In_ wf::Size size, _Out_ bool* pNeedsRemeasure);

        void SetRows(_In_ int rows) { m_rows = rows; }

        void SetCols(_In_ int cols) { m_cols = cols; }

        // the desired viewport size, this is determined by the minimum item size.
        wf::Size GetDesiredViewportSize() const;

        void SetSnapPointFilterFunction(_In_ std::function<HRESULT(_In_ int itemIndex, _Out_ bool* pHasSnapPoint)> func) { m_snapPointFilterFunction = func; }

        IndexCorrectionTable& GetIndexCorrectionTable() { return m_indexCorrectionTable; }

    private:
        void DetermineLineInformation(_In_ int visualIndex, _Out_ int* pStackingLines, _Out_ int* pVirtualizingLine, _Out_ int* pStackingLine) const;
        int DetermineMaxStackingLine() const;
        float GetVirtualizedExtentOfItems(_In_ int itemCount, _In_ int maxStackingLine) const;
        float GetItemStackingPosition(_In_ int stackingLine) const;

        wf::Rect GetItemBounds(_In_ int index);

    private:
        // CalendarPanel is a fixed size panel, meaning that all the containers
        // get the same size, whether they want to or not
        // the cellSize will be default to {1,1}.
        // CalendarPanel will adjust this size based on the given viewport size
        // and trigger a measure/arrange pass again after the arrange pass, 
        // until the size is not changed.
        wf::Size m_cellSize;

        // minimum size.
        wf::Size m_cellMinSize;

        int m_rows;

        int m_cols;
        
        // When this is nullptr, we use the default regular snap point behavior.
        // When this is not null, we use the function to filter out unwanted snap point (and also we'll use irregular snap point behavior).
        std::function<HRESULT(_In_ int itemIndex, _Out_ bool* pHasSnapPoint)> m_snapPointFilterFunction;

        IndexCorrectionTable m_indexCorrectionTable;
    };

} } }
