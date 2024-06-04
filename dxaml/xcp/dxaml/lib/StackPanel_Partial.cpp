// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StackPanel.g.h"
#include "VirtualizingStackPanel.g.h"

using namespace DirectUI;

StackPanel::StackPanel()
{
}

StackPanel::~StackPanel()
{
}

//  IScrollSnapPointsInfo implementation

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::get_AreHorizontalSnapPointsRegular
//
//  Synopsis:
//    Returns True when the horizontal snap points are equidistant
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel::get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    xaml_controls::Orientation orientation;

    IFCPTR(pValue);
    *pValue = FALSE;

    IFC(StackPanelGenerated::get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        // We use the StackPanel's AreScrollSnapPointsRegular property to answer the question.
        IFC(StackPanelGenerated::get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
        *pValue = areScrollSnapPointsRegular;
    }

    // When the orientation is vertical, there are no horizontal snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::get_AreVerticalSnapPointsRegular
//
//  Synopsis:
//    Returns True when the vertical snap points are equidistant
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel::get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    xaml_controls::Orientation orientation;

    IFCPTR(pValue);
    *pValue = FALSE;

    IFC(StackPanelGenerated::get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
    {
        // We use the StackPanel's AreScrollSnapPointsRegular property to answer the question.
        IFC(StackPanelGenerated::get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
        *pValue = areScrollSnapPointsRegular;
    }

    // When the orientation is horizontal, there are no vertical snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::GetIrregularSnapPoints
//
//  Synopsis:
//    Returns a read-only collection of numbers representing the snap points for
//    the provided orientation. Returns an empty collection when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel::GetIrregularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The read-only collection of snap points.
    _Outptr_ wfc::IVectorView<FLOAT>** pValue)
{
    HRESULT hr = S_OK;
    UINT32 cSnapPoints = 0;
    FLOAT* pSnapPoints = NULL;
    ctl::ComPtr<ValueTypeView<FLOAT>> spSnapPointVTV;

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::StackPanel_GetIrregularSnapPoints(
        static_cast<CStackPanel*>(GetHandle()),
        orientation == xaml_controls::Orientation_Horizontal,
        alignment == xaml_primitives::SnapPointsAlignment_Near,
        alignment == xaml_primitives::SnapPointsAlignment_Far,
        &pSnapPoints,
        &cSnapPoints));

    IFC(ctl::make<ValueTypeView<FLOAT>>(&spSnapPointVTV));
    IFC(spSnapPointVTV->SetView(pSnapPoints, cSnapPoints));

    IFC(spSnapPointVTV.CopyTo(pValue));

Cleanup:
    delete [] pSnapPoints;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::GetRegularSnapPoints
//
//  Synopsis:
//    Returns an original offset and interval for equidistant snap points for
//    the provided orientation. Returns 0 when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT StackPanel::GetRegularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The offset of the first snap point.
    _Out_ FLOAT* pOffset,
    // The interval between the regular snap points.
    _Out_ FLOAT* pInterval)
{
    HRESULT hr = S_OK;
    FLOAT offset = 0.0;
    FLOAT interval = 0.0;

    IFCPTR(pOffset);
    *pOffset = 0.0;
    IFCPTR(pInterval);
    *pInterval = 0.0;

    IFC(CoreImports::StackPanel_GetRegularSnapPoints(
        static_cast<CStackPanel*>(GetHandle()),
        orientation == xaml_controls::Orientation_Horizontal,
        alignment == xaml_primitives::SnapPointsAlignment_Near,
        alignment == xaml_primitives::SnapPointsAlignment_Far,
        &offset,
        &interval));

    *pOffset = offset;
    *pInterval = interval;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::add_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
StackPanel::add_HorizontalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(CoreImports::StackPanel_SetSnapPointsChangeNotificationsRequirement(
            static_cast<CStackPanel*>(GetHandle()),
            true /*bIsForHorizontalSnapPoints*/,
            true /*bNotifyChanges*/));
    }

    IFC(StackPanelGenerated::add_HorizontalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::remove_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
StackPanel::remove_HorizontalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(StackPanelGenerated::remove_HorizontalSnapPointsChanged(tToken));

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (pEventSource->HasHandlers())
    {
        IFC(CoreImports::StackPanel_SetSnapPointsChangeNotificationsRequirement(
            static_cast<CStackPanel*>(GetHandle()),
            true /*bIsForHorizontalSnapPoints*/,
            false /*bNotifyChanges*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::add_VerticalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
StackPanel::add_VerticalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(CoreImports::StackPanel_SetSnapPointsChangeNotificationsRequirement(
            static_cast<CStackPanel*>(GetHandle()),
            false /*bIsForHorizontalSnapPoints*/,
            true /*bNotifyChanges*/));
    }

    IFC(StackPanelGenerated::add_VerticalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::remove_VerticalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
StackPanel::remove_VerticalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(StackPanelGenerated::remove_VerticalSnapPointsChanged(tToken));

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (pEventSource->HasHandlers())
    {
        IFC(CoreImports::StackPanel_SetSnapPointsChangeNotificationsRequirement(
            static_cast<CStackPanel*>(GetHandle()),
            false /*bIsForHorizontalSnapPoints*/,
            false /*bNotifyChanges*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::OnHorizontalSnapPointsChanged
//
//  Synopsis:
//    Raises the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
StackPanel::OnHorizontalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make<EventArgs>(&spArgs));

    // Raise the event
    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::OnVerticalSnapPointsChanged
//
//  Synopsis:
//    Raises the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
StackPanel::OnVerticalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make<EventArgs>(&spArgs));

    // Raise the event
    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   StackPanel::NotifySnapPointsChanged
//
//  Synopsis:
//    Called when a snap points change needs to raise an event.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
StackPanel::NotifySnapPointsChanged(_In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;

    if (isForHorizontalSnapPoints)
    {
        // Raise HorizontalSnapPointsChanged event.
        IFC(OnHorizontalSnapPointsChanged());
    }
    else
    {
        // Raise VerticalSnapPointsChanged event.
        IFC(OnVerticalSnapPointsChanged());
    }

Cleanup:
    RRETURN(hr);
}

// Get the closest element information to the point.
_Check_return_ HRESULT StackPanel::GetClosestElementInfo(
    _In_ wf::Point position,
    _Out_ xaml_primitives::ElementInfo* returnValue)
{
    HRESULT hr = S_OK;

    IFC(GetClosestOrInsertionIndex(position, FALSE, &returnValue->m_childIndex));

Cleanup:
    RRETURN(hr);
}

// Get the index where an item should be inserted if it were dropped at
// the given position.  This will be used by live reordering.
// In case of stackPanel, the position is position from the very first Item
// StackPanel goes through all children and finds the insertion position
_Check_return_ HRESULT StackPanel::GetInsertionIndex(
    _In_ wf::Point position,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;
    IFC(GetClosestOrInsertionIndex(position, TRUE, returnValue));

Cleanup:
    RRETURN(hr);
}

 // Gets a series of BOOLEAN values indicating whether a given index is
// positioned on the leftmost, topmost, rightmost, or bottommost
// edges of the layout.  This can be useful for both determining whether
// to tilt items at the edges of rows or columns as well as providing
// data for portal animations.
_Check_return_ HRESULT StackPanel::IsLayoutBoundary(
    _In_ INT index,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    bool isHorizontal = true;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    UINT32 nCount = 0;

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&nCount));

    IFC(get_Orientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    // ComputeLayoutBoundary computation is same for VSP and StackPanel
    IFC(VirtualizingStackPanel::ComputeLayoutBoundary(
        index,
        nCount,
        isHorizontal,
        pIsLeftBoundary,
        pIsTopBoundary,
        pIsRightBoundary,
        pIsBottomBoundary));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT StackPanel::GetItemsBounds(
    _Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    DOUBLE result = 0.0;
    wf::Rect rect = {};
    IFCPTR(returnValue);

    IFC(get_ActualWidth(&result));
    rect.Width = static_cast<FLOAT>(result);

    IFC(get_ActualHeight(&result));
    rect.Height = static_cast<FLOAT>(result);

    *returnValue = rect;

Cleanup:
    RRETURN(hr);
}

// This method is used for getting closest as well as Insertion Index
_Check_return_ HRESULT StackPanel::GetClosestOrInsertionIndex(
    _In_ wf::Point position,
    _In_ bool isInsertionIndex,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;
    bool isHorizontal = true;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    wf::Size totalSize = {0, 0}; // itemSize which include Justification wf::Size
    INT insertionOrClosestIndex = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT32 nCount = 0;
    wf::Size childDesiredSize = {};

    IFCPTR(returnValue);
    *returnValue = -1;

    // Get Orientation
    IFC(get_Orientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&nCount));

    for (UINT32 childIndex = 0;  childIndex < nCount; childIndex++)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;

        IFC(spChildren->GetAt(childIndex, &spChild));
        if (spChild)
        {
            IFC(spChild->get_DesiredSize(&childDesiredSize));
        }

        // Calculate ArrangeOffset
        if(isHorizontal)
        {
            totalSize.Width += childDesiredSize.Width;
            if(DoubleUtil::LessThanOrClose(position.X, totalSize.Width))
            {
                insertionOrClosestIndex = childIndex;
                if(isInsertionIndex &&
                    position.X > totalSize.Width - childDesiredSize.Width/2)
                {
                    insertionOrClosestIndex += 1;
                }
                break;
            }
        }
        else
        {
            totalSize.Height += childDesiredSize.Height;
            if(DoubleUtil::LessThanOrClose(position.Y, totalSize.Height))
            {
                insertionOrClosestIndex = childIndex;
                if(isInsertionIndex &&
                    position.Y > totalSize.Height - childDesiredSize.Height/2)
                {
                    insertionOrClosestIndex += 1;
                }
                break;
            }
        }
    }

    // Special condition for the case when alignment is Left and there is extra space on the right
    if(isHorizontal && DoubleUtil::GreaterThanOrClose(position.X, totalSize.Width))
    {
        insertionOrClosestIndex = (isInsertionIndex) ? nCount : nCount - 1;
    }
    else if(!isHorizontal && DoubleUtil::GreaterThanOrClose(position.Y, totalSize.Height))
    {
        insertionOrClosestIndex = (isInsertionIndex) ? nCount : nCount - 1;
    }

    *returnValue = insertionOrClosestIndex;

Cleanup:
    RRETURN(hr);
}

// Get the indexes where an item should be inserted if it were dropped at
// the given position
_Check_return_ HRESULT
StackPanel::GetInsertionIndexesImpl(
    _In_ wf::Point position,
    _Out_ INT* pFirst,
    _Out_ INT* pSecond)
{
    int insertionIndex = -1;
    BOOLEAN isLeftBoundary = FALSE;
    BOOLEAN isRightBoundary = FALSE;
    BOOLEAN isTopBoundary = FALSE;
    BOOLEAN isBottomBoundary = FALSE;
    BOOLEAN firstCheck = FALSE;
    BOOLEAN secondCheck = FALSE;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR_RETURN(pFirst);
    IFCPTR_RETURN(pSecond);

    *pFirst = -1;
    *pSecond = -1;

    IFC_RETURN(GetInsertionIndex(position, &insertionIndex));
    IFC_RETURN(IsLayoutBoundary(insertionIndex, &isLeftBoundary, &isTopBoundary, &isRightBoundary, &isBottomBoundary));

    *pFirst = insertionIndex - 1;
    *pSecond = insertionIndex;

    IFC_RETURN(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        firstCheck = isTopBoundary;
        secondCheck = isBottomBoundary;
    }
    else
    {
        firstCheck = isLeftBoundary;
        secondCheck = isRightBoundary;
    }

    // make sure we're not at the edges of the panel
    if (firstCheck)
    {
        *pFirst = -1;
    }
    else if (secondCheck)
    {
        *pSecond = -1;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the index of the last element visible in the viewport
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
StackPanel::GetLastItemIndexInViewport(
    IScrollInfo* pScrollInfo,
    INT* pResult)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren = NULL;
    ctl::ComPtr<IUIElement> spResultChild = NULL;
    unsigned itemCount = 0;
    xaml_controls::Orientation orientation;
    DOUBLE viewPortOffset = 0;
    UINT low, mid, high = 0;
    XFLOAT offsetX = {0}, offsetY, orientedOffset = 0;

    *pResult = -1;

    // Get the offset of the edge of the viewport
    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        DOUBLE viewPortHeight;
        DOUBLE verticalOffset;

        IFC(pScrollInfo->get_ViewportHeight(&viewPortHeight));
        IFC(pScrollInfo->get_VerticalOffset(&verticalOffset));
        viewPortOffset = viewPortHeight + verticalOffset;
    }
    else
    {
        DOUBLE viewPortWidth;
        DOUBLE horizontalOffset;

        IFC(pScrollInfo->get_ViewportWidth(&viewPortWidth));
        IFC(pScrollInfo->get_HorizontalOffset(&horizontalOffset));
        viewPortOffset = viewPortWidth + horizontalOffset;
    }

    IFC(get_Children(spChildren.GetAddressOf()));
    IFC(spChildren->get_Size(&itemCount));

    if (itemCount)
    {
        // Use binary search of iterate over the children to find the
        // last visible element in the viewport.
        low = 0;
        high = itemCount-1;
        while (low < high)
        {
            ctl::ComPtr<IUIElement> spChild = NULL;

            mid = (low+high)/2;
            IFC(spChildren->GetAt(mid, spChild.GetAddressOf()));
            IFC(CoreImports::UIElement_GetVisualOffset(
                static_cast<CUIElement*>(static_cast<UIElement*>(spChild.Get())->GetHandle()),
                &offsetX,
                &offsetY));
            orientedOffset = orientation == xaml_controls::Orientation_Horizontal ? offsetX : offsetY;

            if (orientedOffset > viewPortOffset)
            {
                high = mid - 1;
            }
            else
            {
                low = mid + 1;
            }
        }

        IFC(spChildren->GetAt(low, spResultChild.GetAddressOf()));
        IFC(CoreImports::UIElement_GetVisualOffset(
            static_cast<CUIElement*>(static_cast<UIElement*>(spResultChild.Get())->GetHandle()),
            &offsetX,
            &offsetY));
        orientedOffset = orientation == xaml_controls::Orientation_Horizontal ? offsetX : offsetY;
        *pResult = orientedOffset > viewPortOffset ? low-1 : low;
    }

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates items per page based on the current viewport size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
StackPanel::GetItemsPerPage(
    _In_ IScrollInfo* pScrollInfo,
    _Out_ DOUBLE* pItemsPerPage)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT itemCount = 0;
    UINT itemAverageCount = 0;
    xaml_controls::Orientation orientation;
    DOUBLE viewPortSize = 0;
    DOUBLE totalItemSize = 0;

    IFC(get_Children(spChildren.GetAddressOf()));
    IFC(spChildren->get_Size(&itemCount));
    IFC(get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
    {
        IFC(pScrollInfo->get_ViewportHeight(&viewPortSize));
    }
    else
    {
        IFC(pScrollInfo->get_ViewportWidth(&viewPortSize));
    }

    for (UINT i = 0; i < itemCount; i++)
    {
        ctl::ComPtr<IUIElement> spChild;

        IFC(spChildren->GetAt(i, &spChild));
        if (spChild)
        {
            wf::Size itemSize = {0, 0};
            IFC(spChild->get_DesiredSize(&itemSize));

            if (orientation == xaml_controls::Orientation_Vertical)
            {
                totalItemSize += itemSize.Height;
            }
            else
            {
                totalItemSize += itemSize.Width;
            }

            itemAverageCount ++;
        }
    }

    // setup a default
    *pItemsPerPage = 0; // I wonder if we should choose an arbritray number here like 5.

    // but we can do better
    if (itemAverageCount > 0)
    {
        DOUBLE averageItemSize = totalItemSize / itemAverageCount;
        if (averageItemSize > 1)    // Items smaller than 1 pixels are not useful for calculation
        {
            *pItemsPerPage = viewPortSize / averageItemSize;
        }
    }

Cleanup:
    RRETURN(hr);
}


