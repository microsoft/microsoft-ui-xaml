// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      New stacking strategy to present items in a stacking fashion

#pragma once

#include "LayoutStrategyBase.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {
                    class StackingLayoutStrategyUnitTests;
} } } } }

namespace DirectUI { namespace Components { namespace Moco {
    
    class StackingLayoutStrategyImpl
        : public LayoutStrategyBase
    {
        friend class Microsoft::UI::Xaml::Tests::Controls::StackingLayoutStrategyUnitTests;

    public:

        StackingLayoutStrategyImpl();
        void Initialize();

        #pragma region Layout related methods

        // Begin/End measure are used to keep tracking of the largest element size
        // in the non-virtualizing direction so that we can use it when we estimate
        // the panel's extent.
        void BeginMeasure() { m_extentInNonVirtualizingDirection = 0.0f; }
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
            _In_ wf::Rect windowConstraintBounds,
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

        // Stacking layout doesn't have regular snap points
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
            // ISP always has snap point on element.
            return true;
        }

        #pragma endregion

        wf::Rect GetElementTransitionsBounds(
            _In_ xaml_controls::ElementType elementType,
            _In_ int elementIndex,
            _In_ wf::Rect windowConstraint);

        //
        // Special elements methods
        //

        bool NeedsSpecialItem() const { return true; }
        bool NeedsSpecialGroup() const { return IsGrouping(); }

        int GetSpecialItemIndex() const { return c_specialItemIndex; }
        int GetSpecialGroupIndex() const { return IsGrouping() ? c_specialGroupIndex : -1; }

        void RegisterSpecialContainerSize(_In_ int itemIndex, _In_ wf::Size containerDesiredSize);
        void RegisterSpecialHeaderSize(_In_ int groupIndex, _In_ wf::Size headerDesiredSize);

        // Used to estimate tracked element reposition during items source updates.
        float GetAverageHeaderSize() const;
        float GetAverageContainerSize() const;

    private:
        const float GetDistanceBetweenGroups() const;

        float GetVirtualizedGroupExtent(_In_ float itemsInGroup, _In_ float headerExtent) const;
        float GetAverageVirtualizedGroupExtent(_In_ float averageItemsPerGroup) const;
        float GetAverageVirtualizedExtentOfItems(_In_ float itemCount) const;

        void RegisterSize(_In_ int index, _In_ bool isHeader, _In_ float size);

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
            _Out_ int& targetItemIndex);
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
            _Out_ wf::Rect& targetRect);
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
            _Out_ wf::Size& estimate);

        _Check_return_ HRESULT EstimateGroupedExtent(
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Size& extent);

        // Used by GetTargetIndexFromNavigationAction for item containers.
        _Check_return_ HRESULT TryGetTargetHeaderIndexWithItemNavigation(int currentItemIndex, int targetItemIndex, int step, _Out_ int& targetGroupHeaderIndex);

        // Used by GetTargetIndexFromNavigationAction for group headers.
        _Check_return_ HRESULT TryGetTargetItemIndexWithHeaderNavigation(int currentGroupIndex, int targetGroupIndex, int step, _Out_ int& targetItemIndex);

    private:

        // also, headers are constrained to the size of the first header
        wf::Size m_headerSize;
        bool m_headerSizeSet;
        
        // averaging vector. We keep it sized to c_totalSizesForAveraging indices. Each time we
        // take out a size and replace it with a new, we calculate a new total
        std::vector<float> m_containerSizes;
        std::vector<float> m_headerSizes;

        // the total amount of sizes to keep 
        static const int c_totalSizesForAveraging = 1000;

        // the total of all valid sizes in the vector
        double m_containerSizesTotal;
        double m_headerSizesTotal;
        // the number of valid sizes in the vector
        int m_containerSizesStoredTotal;
        int m_headerSizesStoredTotal;

        float m_extentInNonVirtualizingDirection;
    };

} } }
