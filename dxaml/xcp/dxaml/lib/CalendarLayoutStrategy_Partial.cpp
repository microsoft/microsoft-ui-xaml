// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarLayoutStrategy.g.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_controls;


_Check_return_ HRESULT
CalendarLayoutStrategy::SetLayoutDataInfoProviderImpl(_In_ xaml_controls::ILayoutDataInfoProvider* pProvider) /*override*/
{
    _spDataInfoProvider = pProvider;
    _layoutStrategyImpl.SetLayoutDataInfoProviderNoRef(pProvider);

    return S_OK;
}

#pragma region Layout related methods

_Check_return_ HRESULT 
CalendarLayoutStrategy::BeginMeasureImpl() /*override*/
{
    _layoutStrategyImpl.BeginMeasure();
    return S_OK;
}

_Check_return_ HRESULT 
CalendarLayoutStrategy::EndMeasureImpl() /*override*/
{
    _layoutStrategyImpl.EndMeasure();
    return S_OK;
}

// returns the size we should use to measure a container or header with
// itemIndex - indicates an index of valid item or -1 for general, non-special items
_Check_return_ HRESULT 
CalendarLayoutStrategy::GetElementMeasureSizeImpl(_In_ xaml_controls::ElementType elementType, _In_ INT elementIndex, _In_ wf::Rect windowConstraint, _Out_ wf::Size* pReturnValue) /*override*/
{
    *pReturnValue = {};
    *pReturnValue = _layoutStrategyImpl.GetElementMeasureSize(elementType, elementIndex, windowConstraint);

    return S_OK;
}

_Check_return_ HRESULT 
CalendarLayoutStrategy::GetElementBoundsImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Size containerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Rect* pReturnValue) /*override*/
{
    *pReturnValue = {};
    *pReturnValue = _layoutStrategyImpl.GetElementBounds(
        elementType,
        elementIndex,
        containerDesiredSize,
        referenceInformation,
        windowConstraint);

    return S_OK;
}

_Check_return_ HRESULT
CalendarLayoutStrategy::GetElementArrangeBoundsImpl(
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
CalendarLayoutStrategy::ShouldContinueFillingUpSpaceImpl(
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
CalendarLayoutStrategy::GetPositionOfFirstElementImpl(_Out_ wf::Point* returnValue)
{
    *returnValue = {};
    *returnValue = _layoutStrategyImpl.GetPositionOfFirstElement();

    return S_OK;
}

#pragma endregion

#pragma region Estimation and virtualization related methods.

_Check_return_ HRESULT 
CalendarLayoutStrategy::GetVirtualizationDirectionImpl(
    _Out_ xaml_controls::Orientation* pReturnValue)
{
    *pReturnValue = _layoutStrategyImpl.GetVirtualizationDirection();
    return S_OK;
}

_Check_return_ HRESULT 
CalendarLayoutStrategy::EstimateElementIndexImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
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
CalendarLayoutStrategy::EstimateElementBoundsImpl(
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
CalendarLayoutStrategy::EstimatePanelExtentImpl(
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
CalendarLayoutStrategy::EstimateIndexFromPointImpl(
    _In_ BOOLEAN requestingInsertionIndex,
    _In_ wf::Point point,
    _In_ xaml_controls::EstimationReference reference,
    _In_ wf::Rect windowConstraint,
    _Out_ xaml_controls::IndexSearchHint* pSearchHint,
    _Out_ xaml_controls::ElementType* pElementType,
    _Out_ INT* pElementIndex) /*override*/
{
    return E_NOTIMPL;
}

// Based on current element's index/type and action, return the next element index/type.
_Check_return_ HRESULT
CalendarLayoutStrategy::GetTargetIndexFromNavigationActionImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ wf::Rect windowConstraint,
    _In_ int /*itemIndexHintForHeaderNavigation*/,
    _Out_ xaml_controls::ElementType* targetElementType,
    _Out_ INT* targetElementIndex) /*override*/
{
    *targetElementType = xaml_controls::ElementType_ItemContainer;
    *targetElementIndex = 0;
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
CalendarLayoutStrategy::IsIndexLayoutBoundaryImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Rect windowConstraint,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary) /*override*/
{
    return E_NOTIMPL;
}

#pragma endregion

#pragma region Snap points related

_Check_return_ HRESULT 
CalendarLayoutStrategy::GetRegularSnapPointsImpl(
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
CalendarLayoutStrategy::HasIrregularSnapPointsImpl(
    _In_ xaml_controls::ElementType elementType,
    _Out_ BOOLEAN* returnValue) /*override*/
{
    *returnValue = FALSE;
    *returnValue = !!_layoutStrategyImpl.HasIrregularSnapPoints(elementType);

    return S_OK;
}

_Check_return_ HRESULT
CalendarLayoutStrategy::HasSnapPointOnElementImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = FALSE;
    bool result = false;
    IFC_RETURN(_layoutStrategyImpl.HasSnapPointOnElement(elementType, elementIndex, result));
    *returnValue = result;

    return S_OK;
}
#pragma endregion

_Check_return_ HRESULT 
CalendarLayoutStrategy::GetIsWrappingStrategyImpl(_Out_ BOOLEAN* returnValue) /*override*/
{
    *returnValue = FALSE;
    *returnValue = !!_layoutStrategyImpl.GetIsWrappingStrategy();

    return S_OK;
}

_Check_return_ HRESULT 
CalendarLayoutStrategy::GetElementTransitionsBoundsImpl(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Rect* pReturnValue) /*override*/
{
    return E_NOTIMPL;
}


#pragma region Special elements methods

bool CalendarLayoutStrategy::NeedsSpecialItem() const
{
    return _layoutStrategyImpl.NeedsSpecialItem();
}

int CalendarLayoutStrategy::GetSpecialItemIndex() const
{
    return _layoutStrategyImpl.GetSpecialItemIndex();
}

#pragma endregion


wf::Size CalendarLayoutStrategy::GetDesiredViewportSize() const
{
    return _layoutStrategyImpl.GetDesiredViewportSize();
}

_Check_return_ HRESULT CalendarLayoutStrategy::SetSnapPointFilterFunction(
    _In_ std::function<HRESULT(_In_ int itemIndex, _Out_ bool* pHasSnapPoint)> func)
{
    _layoutStrategyImpl.SetSnapPointFilterFunction(func);

    return S_OK;
}

Components::Moco::CalendarLayoutStrategyImpl::IndexCorrectionTable& CalendarLayoutStrategy::GetIndexCorrectionTable()
{
    return _layoutStrategyImpl.GetIndexCorrectionTable();
}
