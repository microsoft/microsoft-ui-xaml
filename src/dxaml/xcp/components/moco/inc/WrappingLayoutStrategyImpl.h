// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a wrapping strategy for new style virtualizing panels.

#pragma once

#include "LayoutStrategyBase.h"

namespace DirectUI { namespace Components { namespace Moco {

    class WrappingLayoutStrategyImpl
        : public LayoutStrategyBase
    {
    public:

        WrappingLayoutStrategyImpl();

        // implementation of interface

        #pragma region Layout related methods

        // The wrapping layout doesn't need to do any book keeping.
        void BeginMeasure() {}
        void EndMeasure() {}

        // returns the size we should use to measure a container or header with
        // itemIndex - indicates an index of valid item or -1 for general, non-special items
        wf::Size GetElementMeasureSize(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Rect windowConstraint);

        _Check_return_ HRESULT GetElementBounds(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Size containerDesiredSize,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Rect* pReturnValue);


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

        wf::Point GetPositionOfFirstElement();

        #pragma endregion

        #pragma region Estimation and virtualization related methods.

        _Check_return_ HRESULT EstimateElementIndex(
            _In_ xaml_controls::ElementType elementType,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pTargetRect,
            _Out_ int& result);

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

        // Estimates the index or the insertion index closest to the given point.
        _Check_return_ HRESULT EstimateIndexFromPoint(
            _In_ bool requestingInsertionIndex,
            _In_ wf::Point point,
            _In_ xaml_controls::EstimationReference reference,
            _In_ wf::Rect windowConstraint,
            _Out_ xaml_controls::IndexSearchHint* pSearchHint,
            _Out_ xaml_controls::ElementType* pElementType,
            _Out_ INT* pElementIndex);

        // Based on current element's index/type and action, return the next element index/type.
        _Check_return_ HRESULT GetTargetIndexFromNavigationAction(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ wf::Rect windowConstraint,
            _In_ int itemIndexHintForHeaderNavigation,
            _Out_ xaml_controls::ElementType* pTargetElementType,
            _Out_ INT* pTargetElementIndex);

        // Determines whether or not the given item index is a layout boundary.
        _Check_return_ HRESULT IsIndexLayoutBoundary(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Rect windowConstraint,
            _Out_ bool* pIsLeftBoundary,
            _Out_ bool* pIsTopBoundary,
            _Out_ bool* pIsRightBoundary,
            _Out_ bool* pIsBottomBoundary);

        #pragma endregion

        #pragma region Snap points related

        bool GetRegularSnapPoints(
            _Out_ float* pNearOffset,
            _Out_ float* pFarOffset,
            _Out_ float* pSpacing);

        bool HasIrregularSnapPoints(
            _In_ xaml_controls::ElementType elementType);

        bool HasSnapPointOnElement(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex)
        {
            // IWG always has snap point on element.
            return true;
        }

        #pragma endregion

        _Check_return_ HRESULT GetElementTransitionsBounds(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Rect* pReturnValue);

        double GetItemsPerPage(_In_ wf::Rect window) const;

        // internal configuration
        void SetMaximumRowsOrColumns(_In_ int maxRowsOrColumns) { m_maxRowsOrColumns = maxRowsOrColumns; }
        void SetItemWidth(_In_ double itemWidth);
        void SetItemHeight(_In_ double itemHeight);

        void InvalidateGroupCache();

        //
        // Special elements methods
        // wrapgrid always needs a correct size for the first container, since it is special
        // The desiredsize there will determine the size for all
        //

        // If we've set an explicit size, we don't need this
        bool NeedsSpecialItem() const { return !IsItemSizePropertySet(); }
        bool NeedsSpecialGroup() const { return IsGrouping(); }

        int GetSpecialItemIndex() const { return c_specialItemIndex; }
        int GetSpecialGroupIndex() const { return IsGrouping() ? c_specialGroupIndex : -1; }

        void RegisterSpecialContainerSize(_In_ int itemIndex, _In_ wf::Size containerDesiredSize);
        void RegisterSpecialHeaderSize(_In_ int groupIndex, _In_ wf::Size headerDesiredSize);

    private:
        const float GetDistanceBetweenGroups() const;
        void DetermineLineInformation(_In_ const wf::Rect &windowConstraint, _In_ int indexInGroup, _Out_ int* pStackingLines, _Out_ int* pVirtualizingLine, _Out_ int* pStackingLine) const;
        int DetermineMaxStackingLine(_In_ const wf::Rect& windowConstraint) const;

        // Our items are fixed sizes, so it makes sense to have a method to get the extent of a known-size group
        // separate from an average group extent
        float GetVirtualizedGroupExtent(_In_ int itemsInGroup, _In_ int maxStackingLine, _In_ float headerExtent) const;
        float GetAverageHeaderExtent() const;
        float GetVirtualizedExtentOfItems(_In_ int itemCount, _In_ int maxStackingLine) const;
        float GetItemStackingPosition(_In_ int stackingLine) const;

        bool IsItemWidthPropertySet() const;
        bool IsItemHeightPropertySet() const;
        bool IsItemSizePropertySet() const;

        // Used by GetElementBounds for each element type.
        wf::Rect GetContainerBounds(
            _In_ int indexInItems,
            _In_ int indexInGroup,
            _In_ wf::Size containerDesiredSize,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowConstraint);
        wf::Rect GetHeaderBounds(
            _In_ int groupIndex,
            _In_ wf::Size headerDesiredSize,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowConstraint);

        // Used by EstimateElementIndex for each element type.
        _Check_return_ HRESULT EstimateItemIndexFromWindow(
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pTargetRect,
            _Out_ int& result);
        _Check_return_ HRESULT EstimateGroupIndexFromWindow(
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pTargetRect,
            _Out_ int& targetGroupIndex);

        // Used by EstimateElementBounds for each element type.
        _Check_return_ HRESULT EstimateContainerLocation(
            _In_ int targetItemIndex,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect& result);
        _Check_return_ HRESULT EstimateHeaderLocation(
            _In_ int targetGroupIndex,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect& targetRect);

        // Used by EstimatePanelExtent for each element type.
        _Check_return_ HRESULT EstimateNonGroupedExtent(
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Size& extent);
        _Check_return_ HRESULT EstimateGroupedExtent(
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Size& extent);
        _Check_return_ HRESULT EnsureGroupCache(
            _In_ int maxStackingLines);

        // Used by GetTargetIndexFromNavigationAction for item containers.
        _Check_return_ HRESULT GetTargetItemIndexInNextLineWithGrouping(
            _In_ int currItemIndex,
            _In_ int step,
            _In_ int maxLineLength,
            _Out_ int& targetIndex);

        // Used by GetTargetIndexFromNavigationAction for item containers.
        _Check_return_ HRESULT TryGetTargetHeaderIndexWithItemNavigation(
            _In_ int currentItemIndex,
            _In_ int targetItemIndex,
            _In_ int step,
            _In_ int maxLineLength,
            _Out_ int& targetGroupHeaderIndex);

        // Used by GetTargetIndexFromNavigationAction for group headers.
        _Check_return_ HRESULT TryGetTargetItemIndexWithHeaderNavigation(
            _In_ int currentGroupIndex,
            _In_ int targetGroupIndex,
            _In_ int step,
            _In_ int maxLineLength,
            _In_ int lastTargetItemIndexHint,
            _Out_ int& targetItemIndex);

    private:

        // wrapgrid is a fixed size panel, meaning that all the containers
        // get the same size, whether they want to or not
        wf::Size m_cellSize;
        bool m_cellSizeSet;

        // also, headers are constrained to the size of the first header
        wf::Size m_headerSize;
        bool m_headerSizeSet;

        int m_maxRowsOrColumns;

        // ItemWidth and ItemHeight properties
        double m_itemWidthFromPanel;
        double m_itemHeightFromPanel;

        // Cached data about the size of groups
        std::vector<float> m_cachedGroupLocations;
        int m_cachedStackingLines;
        wf::Size m_cachedCellSize;

    };

} } }
