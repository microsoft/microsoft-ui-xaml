// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StackingLayoutStrategy.g.h"
#include "StackingLayoutStrategyImpl.h"

namespace DirectUI
{
    PARTIAL_CLASS(StackingLayoutStrategy)
    {
    protected:
        StackingLayoutStrategy() = default;
        _Check_return_ HRESULT Initialize() override;

    public:

        _Check_return_ HRESULT SetLayoutDataInfoProviderImpl(_In_ xaml_controls::ILayoutDataInfoProvider* pProvider);

        #pragma region Layout related methods

        _Check_return_ HRESULT BeginMeasureImpl();
        _Check_return_ HRESULT EndMeasureImpl();

        // returns the size we should use to measure a container or header with
        // itemIndex - indicates an index of valid item or -1 for general, non-special items
        _Check_return_ HRESULT GetElementMeasureSizeImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Size* pReturnValue);

        _Check_return_ HRESULT GetElementBoundsImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ wf::Size containerDesiredSize,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Rect* pReturnValue);

        _Check_return_ HRESULT GetElementArrangeBoundsImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ wf::Rect containerBounds,
            _In_ wf::Rect windowConstraint,
            _In_ wf::Size finalSize,
            _Out_ wf::Rect* pReturnValue);

        _Check_return_ HRESULT ShouldContinueFillingUpSpaceImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ wf::Rect windowToFill,
            _Out_ BOOLEAN* pReturnValue);

        _Check_return_ HRESULT GetPositionOfFirstElementImpl(_Out_ wf::Point* returnValue);

        #pragma endregion

        #pragma region Estimation and virtualization related methods.

        _Check_return_ HRESULT GetVirtualizationDirectionImpl(
            _Out_ xaml_controls::Orientation* pReturnValue);

        _Check_return_ HRESULT EstimateElementIndexImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pTargetRect,
            _Out_ INT* pReturnValue);

        _Check_return_ HRESULT EstimateElementBoundsImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ xaml_controls::EstimationReference headerReference,
            _In_ xaml_controls::EstimationReference containerReference,
            _In_ wf::Rect window,
            _Out_ wf::Rect* pReturnValue);

        _Check_return_ HRESULT EstimatePanelExtentImpl(
            _In_ xaml_controls::EstimationReference lastHeaderReference,
            _In_ xaml_controls::EstimationReference lastContainerReference,
            _In_ wf::Rect windowConstraint,
            _Out_ wf::Size* pExtent);

        #pragma endregion

        #pragma region IItemLookupPanel related

        // Estimates the index or the insertion index closest to the given point.
        _Check_return_ HRESULT EstimateIndexFromPointImpl(
            _In_ BOOLEAN requestingInsertionIndex,
            _In_ wf::Point point,
            _In_ xaml_controls::EstimationReference reference,
            _In_ wf::Rect windowConstraint,
            _Out_ xaml_controls::IndexSearchHint* pSearchHint,
            _Out_ xaml_controls::ElementType* pElementType,
            _Out_ INT* pElementIndex);

        // Based on current element's index/type and action, return the next element index/type.
        _Check_return_ HRESULT GetTargetIndexFromNavigationActionImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ wf::Rect windowConstraint,
            _In_ int itemIndexHintForHeaderNavigation,
            _Out_ xaml_controls::ElementType* targetElementType,
            _Out_ INT* targetElementIndex);

        // Determines whether or not the given item index
        // is a layout boundary.
        _Check_return_ HRESULT IsIndexLayoutBoundaryImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ wf::Rect windowConstraint,
            _Out_ BOOLEAN* isLeftBoundary,
            _Out_ BOOLEAN* isTopBoundary,
            _Out_ BOOLEAN* isRightBoundary,
            _Out_ BOOLEAN* isBottomBoundary);

        #pragma endregion

        #pragma region Snap points related

        _Check_return_ HRESULT GetRegularSnapPointsImpl(
            _Out_ FLOAT* pNearOffset,
            _Out_ FLOAT* pFarOffset,
            _Out_ FLOAT* pSpacing,
            _Out_ BOOLEAN* pHasRegularSnapPoints);

        _Check_return_ HRESULT HasIrregularSnapPointsImpl(
            _In_ xaml_controls::ElementType elementType,
            _Out_ BOOLEAN* returnValue);

        _Check_return_ HRESULT HasSnapPointOnElementImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _Out_ BOOLEAN* returnValue);

        #pragma endregion

        // returns true if we are wrapping, needed for transitions
        _Check_return_ HRESULT GetIsWrappingStrategyImpl(_Out_ BOOLEAN* returnValue);

        _Check_return_ HRESULT GetElementTransitionsBoundsImpl(
            _In_ xaml_controls::ElementType elementType,
            _In_ INT elementIndex,
            _In_ wf::Rect windowConstraint, 
            _Out_ wf::Rect* pReturnValue);

        // internal configuration
        void SetVirtualizationDirection(xaml_controls::Orientation direction) { _layoutStrategyImpl.SetVirtualizationDirection(direction); }
        void SetGroupPadding(xaml::Thickness padding) { _layoutStrategyImpl.SetGroupPadding(padding); }

        //
        // Special elements methods
        //
        
        bool NeedsSpecialItem() const;
        bool NeedsSpecialGroup() const;

        int GetSpecialItemIndex() const;
        int GetSpecialGroupIndex() const;

        _Check_return_ HRESULT RegisterSpecialContainerSize(_In_ INT itemIndex, _In_ wf::Size containerDesiredSize);
        _Check_return_ HRESULT RegisterSpecialHeaderSize(_In_ INT groupIndex, _In_ wf::Size headerDesiredSize);

        // Notify the layout strategy of the new header placement.
        void SetGroupHeaderStrategy(_In_ GroupHeaderStrategy strategy);
        

    private:
        // This is where the actual implementation of the layout strategy exists,
        Components::Moco::StackingLayoutStrategyImpl _layoutStrategyImpl;

        ctl::ComPtr<xaml_controls::ILayoutDataInfoProvider> _spDataInfoProvider;
    };
}
