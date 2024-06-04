// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StackingLayoutStrategy.g.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_controls;

_Check_return_ HRESULT StackingLayoutStrategy::Initialize()
{
    _layoutStrategyImpl.Initialize();

    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategy::SetLayoutDataInfoProviderImpl(_Out_ xaml_controls::ILayoutDataInfoProvider* pProvider) /*override*/
{
    _spDataInfoProvider = pProvider;
    _layoutStrategyImpl.SetLayoutDataInfoProviderNoRef(pProvider);
    
    return S_OK;
}

#pragma region Layout related methods

_Check_return_ HRESULT 
StackingLayoutStrategy::BeginMeasureImpl() /*override*/
{
    _layoutStrategyImpl.BeginMeasure();
    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::EndMeasureImpl() /*override*/
{
    _layoutStrategyImpl.EndMeasure();
    return S_OK;
}

// returns the size we should use to measure a container or header with
// itemIndex - indicates an index of valid item or -1 for general, non-special items
_Check_return_ HRESULT 
StackingLayoutStrategy::GetElementMeasureSizeImpl(_Out_ xaml_controls::ElementType elementType, _Out_ INT elementIndex, _Out_ wf::Rect windowConstraint, _Out_ wf::Size* pReturnValue) /*override*/
{
    *pReturnValue = {};
    *pReturnValue = _layoutStrategyImpl.GetElementMeasureSize(elementType, elementIndex, windowConstraint);

    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::GetElementBoundsImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Size containerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Rect* pReturnValue) /*override*/
{
    *pReturnValue = {};
    IFC_RETURN(_layoutStrategyImpl.GetElementBounds(
        elementType,
        elementIndex,
        containerDesiredSize,
        referenceInformation,
        windowConstraint,
        pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategy::GetElementArrangeBoundsImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Rect containerBounds,
    _In_ wf::Rect windowConstraint,
    _In_ wf::Size finalSize,
    _Out_ wf::Rect* pReturnValue) /*override*/
{
    *pReturnValue = {};
    *pReturnValue = _layoutStrategyImpl.GetElementArrangeBounds(
        elementType,
        elementIndex,
        containerBounds,
        windowConstraint,
        finalSize);

    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::ShouldContinueFillingUpSpaceImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowToFill,
    _Out_ BOOLEAN* pReturnValue) /*override*/
{
    *pReturnValue = {};
    *pReturnValue = !!_layoutStrategyImpl.ShouldContinueFillingUpSpace(
        elementType,
        elementIndex,
        referenceInformation,
        windowToFill);

    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::GetPositionOfFirstElementImpl(_Out_ wf::Point* returnValue)
{
    *returnValue = {};
    *returnValue = _layoutStrategyImpl.GetPositionOfFirstElement();

    return S_OK;
}

#pragma endregion

#pragma region Estimation and virtualization related methods.

_Check_return_ HRESULT 
StackingLayoutStrategy::GetVirtualizationDirectionImpl(
    _Out_ xaml_controls::Orientation* pReturnValue)
{
    *pReturnValue = _layoutStrategyImpl.GetVirtualizationDirection();
    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::EstimateElementIndexImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _In_ wf::Rect* pTargetRect,
    _Out_ INT* pReturnValue) /*override*/
{
    *pReturnValue = {};
    IFC_RETURN(_layoutStrategyImpl.EstimateElementIndex(
        elementType,
        headerReference,
        containerReference,
        window,
        pTargetRect,
        pReturnValue));

    return S_OK;
}

// Estimate the location of an anchor group, using items-per-group to estimate an average group extent.
_Check_return_ HRESULT 
StackingLayoutStrategy::EstimateElementBoundsImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pReturnValue) /*override*/
{
    *pReturnValue = {};
    IFC_RETURN(_layoutStrategyImpl.EstimateElementBounds(
        elementType,
        elementIndex,
        headerReference,
        containerReference,
        window,
        pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategy::EstimatePanelExtentImpl(
    _In_ xaml_controls::EstimationReference lastHeaderReference,
    _In_ xaml_controls::EstimationReference lastContainerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size* pExtent) /*override*/
{
    *pExtent = {};
    IFC_RETURN(_layoutStrategyImpl.EstimatePanelExtent(
        lastHeaderReference,
        lastContainerReference,
        windowConstraint,
        pExtent));

    return S_OK;
}

#pragma endregion

#pragma region IItemLookupPanel related

// Estimates the index or the insertion index closest to the given point.
_Check_return_ HRESULT
StackingLayoutStrategy::EstimateIndexFromPointImpl(
    _In_ BOOLEAN requestingInsertionIndex,
    _In_ wf::Point point,
    _In_ xaml_controls::EstimationReference reference,
    _In_ wf::Rect windowConstraint,
    _Out_ xaml_controls::IndexSearchHint* pSearchHint,
    _Out_ xaml_controls::ElementType* pElementType,
    _Out_ INT* pElementIndex) /*override*/
{
    *pSearchHint = xaml_controls::IndexSearchHint_NoHint;
    *pElementType = xaml_controls::ElementType_ItemContainer;
    *pElementIndex = 0;
    IFC_RETURN(_layoutStrategyImpl.EstimateIndexFromPoint(
        !!requestingInsertionIndex,
        point,
        reference,
        windowConstraint,
        pSearchHint,
        pElementType,
        pElementIndex));

    return S_OK;
}

// Based on current element's index/type and action, return the next element index/type.
_Check_return_ HRESULT
StackingLayoutStrategy::GetTargetIndexFromNavigationActionImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ wf::Rect windowConstraint,
    _In_ int /*itemIndexHintForHeaderNavigation*/,
    _Out_ xaml_controls::ElementType* targetElementType,
    _Out_ INT* targetElementIndex) /*override*/
{
    IFC_RETURN(_layoutStrategyImpl.GetTargetIndexFromNavigationAction(
        elementType,
        elementIndex,
        action,
        windowConstraint,
        targetElementType,
        targetElementIndex));

    return S_OK;
}

// Determines whether or not the given item index
// is a layout boundary.
_Check_return_ HRESULT 
StackingLayoutStrategy::IsIndexLayoutBoundaryImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Rect windowConstraint,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary) /*override*/
{
    bool isLeftBoundary, isTopBoundary, isRightBoundary, isBottomBoundary;
    IFC_RETURN(_layoutStrategyImpl.IsIndexLayoutBoundary(
        elementType,
        elementIndex,
        windowConstraint,
        &isLeftBoundary,
        &isTopBoundary,
        &isRightBoundary,
        &isBottomBoundary));

    *pIsLeftBoundary = isLeftBoundary;
    *pIsTopBoundary = isTopBoundary;
    *pIsRightBoundary = isRightBoundary;
    *pIsBottomBoundary = isBottomBoundary;

    return S_OK;
}

#pragma endregion

#pragma region Snap points related

_Check_return_ HRESULT 
StackingLayoutStrategy::GetRegularSnapPointsImpl(
    _Out_ FLOAT* pNearOffset,
    _Out_ FLOAT* pFarOffset,
    _Out_ FLOAT* pSpacing,
    _Out_ BOOLEAN* pHasRegularSnapPoints) /*override*/
{
    *pHasRegularSnapPoints = FALSE;
    *pHasRegularSnapPoints = !!_layoutStrategyImpl.GetRegularSnapPoints(
        pNearOffset, 
        pFarOffset, 
        pSpacing);

    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategy::HasIrregularSnapPointsImpl(
    _In_ xaml_controls::ElementType elementType,
    _Out_ BOOLEAN* returnValue) /*override*/
{
    *returnValue = FALSE;
    *returnValue = !!_layoutStrategyImpl.HasIrregularSnapPoints(elementType);

    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategy::HasSnapPointOnElementImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = FALSE;
    *returnValue = _layoutStrategyImpl.HasSnapPointOnElement(elementType, elementIndex);

    return S_OK;
}

#pragma endregion

_Check_return_ HRESULT 
StackingLayoutStrategy::GetIsWrappingStrategyImpl(_Out_ BOOLEAN* returnValue) /*override*/
{
    *returnValue = false;
    *returnValue = !!_layoutStrategyImpl.GetIsWrappingStrategy();

    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::GetElementTransitionsBoundsImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Rect* pReturnValue) /*override*/
{
    *pReturnValue = {};
    *pReturnValue = _layoutStrategyImpl.GetElementTransitionsBounds(
        elementType,
        elementIndex,
        windowConstraint);

    return S_OK;
}

#pragma region Special elements methods

bool StackingLayoutStrategy::NeedsSpecialItem() const
{
    return _layoutStrategyImpl.NeedsSpecialItem();
}

bool StackingLayoutStrategy::NeedsSpecialGroup() const
{
    return _layoutStrategyImpl.NeedsSpecialGroup();
}

int StackingLayoutStrategy::GetSpecialItemIndex() const
{
    return _layoutStrategyImpl.GetSpecialItemIndex();
}

int StackingLayoutStrategy::GetSpecialGroupIndex() const
{
    return _layoutStrategyImpl.GetSpecialGroupIndex();
}

_Check_return_ HRESULT
StackingLayoutStrategy::RegisterSpecialContainerSize(_Out_ INT itemIndex, _Out_ wf::Size containerDesiredSize)
{
    _layoutStrategyImpl.RegisterSpecialContainerSize(itemIndex, containerDesiredSize);

    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategy::RegisterSpecialHeaderSize(_Out_ INT groupIndex, _Out_ wf::Size headerDesiredSize)
{
    _layoutStrategyImpl.RegisterSpecialHeaderSize(groupIndex, headerDesiredSize);

    return S_OK;
}

#pragma endregion

// Notify the layout strategy of the new header placement.
void StackingLayoutStrategy::SetGroupHeaderStrategy(_Out_ GroupHeaderStrategy strategy)
{
    return _layoutStrategyImpl.SetGroupHeaderStrategy(strategy);
}