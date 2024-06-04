// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      VariableSizeWrapGrid definition.
//      VariableSizeWrapGrid is used to create variable size tiles inside the grid.
//
//      It has OccupancyBlock and OccupancyMap nested classes
//      OccupancyMap: This class is used to arrange all items of variableSizeGrid
//      Since the size of OccupancyMap can be Infinite, OccupancyBlock is used to
//      allocate block of Memory
//
//      OccupncyMap always arrange items vertically(row), whether user sets
//      Orientation=Vertical or Horizontal. OccupancyMap's public APIs take care of
//      providing correct Orientation. Similarly OccupancyBlock has fixed rows while
//      it is growing on Column direction
//
//      Both classes are nested inside the VariableSizedWrapGrid class

#include "precomp.h"
#include "VariableSizedWrapGrid.g.h"
#include "PropertyMetadata.g.h"
#include "WrapGrid.g.h"
#include <math.h>
#include "OrientedSize.h"

#define ToAdjustedIndex(value) ((value) + 1)
#define FromAdjustedIndex(value) ((value) - 1)

using namespace DirectUI;

// Handle the custom property changed event and call the OnPropertyChanged2
// methods.
_Check_return_ HRESULT VariableSizedWrapGrid::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(VariableSizedWrapGridGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::VariableSizedWrapGrid_ItemHeight:
        case KnownPropertyIndex::VariableSizedWrapGrid_ItemWidth:
        case KnownPropertyIndex::VariableSizedWrapGrid_ColumnSpan:
        case KnownPropertyIndex::VariableSizedWrapGrid_RowSpan:
        case KnownPropertyIndex::VariableSizedWrapGrid_Orientation:
        case KnownPropertyIndex::VariableSizedWrapGrid_HorizontalChildrenAlignment:
        case KnownPropertyIndex::VariableSizedWrapGrid_VerticalChildrenAlignment:
        case KnownPropertyIndex::VariableSizedWrapGrid_MaximumRowsOrColumns:
        {
            IFC(InvalidateMeasure());
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

VariableSizedWrapGrid::VariableSizedWrapGrid() :
    m_pMap(NULL),
    m_pRowSpans(NULL),
    m_pColumnSpans(NULL),
    m_lineCount(0),
    m_itemsPerLine(0)
{
}

VariableSizedWrapGrid::~VariableSizedWrapGrid()
{
    delete m_pMap;
    delete[] m_pRowSpans;
    delete[] m_pColumnSpans;
}

// ComputeBounds
_Check_return_ HRESULT VariableSizedWrapGrid::ComputeBounds(
                        _In_ wf::Size availableSize,
                        _Out_ OrientedSize* pItemSize,
                        _Out_ OrientedSize* pMaximumPanelSize,
                        _Out_ bool* pAreItemsOriented)
{
    HRESULT hr = S_OK;

    OrientedSize maximumSize;
    XDOUBLE itemWidth = 0.0;
    XDOUBLE itemHeight = 0.0;
    bool hasFixedWidth = false;
    bool hasFixedHeight = false;
    OrientedSize itemSize;
    XUINT32 itemCount = 0;

    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren = NULL;
    ctl::ComPtr<IUIElement> spFirstItem = NULL;
    wf::Size firstItemDesiredSize = {0.0, 0.0};
    wf::Size firstItemMeasureSize = {0.0, 0.0};
    XUINT32 firstItemRowSpan = 1;
    XUINT32 firstItemColumnSpan = 1;

    IFCPTR(pItemSize);
    IFCPTR(pMaximumPanelSize);
    IFCPTR(pAreItemsOriented);

    IFC(get_Orientation(&orientation));
    IFC(get_ItemWidth(&itemWidth));
    IFC(get_ItemHeight(&itemHeight));

    maximumSize.put_Orientation(orientation);
    maximumSize.put_Width(availableSize.Width);
    maximumSize.put_Height(availableSize.Height);

    hasFixedWidth = !DoubleUtil::IsNaN(itemWidth);
    hasFixedHeight = !DoubleUtil::IsNaN(itemHeight);
    itemSize.put_Orientation(orientation);
    itemSize.put_Width(hasFixedWidth ? itemWidth : availableSize.Width);
    itemSize.put_Height(hasFixedHeight ? itemHeight : availableSize.Height);
    IFC(itemSize.AsUnorientedSize(&m_itemSize));
    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&itemCount));

    if(itemCount > 0)
    {
        // We always need to measure the first item.
        IFC(spChildren->GetAt(0, &spFirstItem));
        IFCPTR(spFirstItem);
        IFC(GetRowAndColumnSpan(spFirstItem, &firstItemRowSpan, &firstItemColumnSpan));
        // Calculate the size that first item will be measured
        firstItemMeasureSize.Width = hasFixedWidth ? (m_itemSize.Width * firstItemColumnSpan) : m_itemSize.Width;
        firstItemMeasureSize.Height = hasFixedHeight ? (m_itemSize.Height * firstItemRowSpan) : m_itemSize.Height;
        IFC(spFirstItem->Measure(firstItemMeasureSize));
        IFC(spFirstItem->get_DesiredSize(&firstItemDesiredSize));
    }

    // If item sizes aren't specified, use the size of the first item
    if (!hasFixedWidth || !hasFixedHeight)
    {
        if (!hasFixedWidth)
        {
            itemSize.put_Width(firstItemDesiredSize.Width);
        }

        if (!hasFixedHeight)
        {
            itemSize.put_Height(firstItemDesiredSize.Height);
        }
    }
    *pAreItemsOriented = orientation != xaml_controls::Orientation_Vertical;
    *pItemSize = itemSize;
    *pMaximumPanelSize = maximumSize;

Cleanup:
    RRETURN(hr);
}

// MeasureOverride, this creates OccupancyMap and find the size of the Map
// which is occupied
IFACEMETHODIMP VariableSizedWrapGrid::MeasureOverride(
                _In_ wf::Size availableSize,
                _Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    OrientedSize maximumSize;
    OrientedSize itemSize;
    XUINT32 itemCount = 0;
    OrientedSize totalSize;
    wf::Size size = {};
    bool isHorizontalOrientation = false;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren = NULL;
    OrientedSize startingSize;
    OrientedSize justificationSize;

    m_itemsPerLine = 0;
    m_pRowSpans = NULL;
    m_pColumnSpans = NULL;

    IFCPTR(pReturnValue);
    pReturnValue->Width = 0;
    pReturnValue->Height = 0;

    IFC(ComputeBounds(availableSize, &itemSize, &maximumSize, &isHorizontalOrientation));
    IFC(itemSize.AsUnorientedSize(&m_itemSize));
    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&itemCount));
    IFC(CalculateOccupancyMap(itemCount, spChildren, isHorizontalOrientation, &itemSize, &maximumSize, &m_itemsPerLine, &m_lineCount));
    IFC(itemSize.AsUnorientedSize(&m_itemSize));

    // Measure the 2nd and later children (ComputeBounds measures the first one).
    for (XUINT32 index=1; index < itemCount; index++)
    {
        ctl::ComPtr<IUIElement> spItem = NULL;
        IFC(spChildren->GetAt(index, &spItem));
        IFCPTR(spItem);
        size.Width = m_itemSize.Width*m_pColumnSpans[index];
        size.Height = m_itemSize.Height*m_pRowSpans[index];
        IFC(spItem->Measure(size));
    }

    // Compute and return the total amount of size used
    if(isHorizontalOrientation)
    {
        orientation = xaml_controls::Orientation_Horizontal;
    }

    IFC(ComputeHorizontalAndVerticalAlignment(availableSize, &startingSize, &justificationSize));
    totalSize.put_Orientation(orientation);

    totalSize.put_Direct(
    itemSize.get_Direct() * m_itemsPerLine +
    justificationSize.get_Direct() * (m_itemsPerLine + 1) +
    startingSize.get_Direct() * 2.0);

    totalSize.put_Indirect(
    itemSize.get_Indirect() * m_lineCount +
    justificationSize.get_Indirect() * (m_lineCount + 1) +
    startingSize.get_Indirect() * 2.0);

    IFC(totalSize.AsUnorientedSize(pReturnValue));

Cleanup:
    delete [] m_pRowSpans;
    delete [] m_pColumnSpans;
    m_pRowSpans = NULL;
    m_pColumnSpans = NULL;

    RRETURN(hr);
}

// This method creare Occupancy Map, layout children and find Occupied size
_Check_return_ HRESULT VariableSizedWrapGrid::CalculateOccupancyMap(
                        _In_ XUINT32 itemCount,
                        _In_ ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren,
                        _In_ bool isHorizontalOrientation,
                        _In_ OrientedSize* pItemSize,
                        _In_ OrientedSize* pMaximumSize,
                        _Out_ XUINT32* pItemsPerLine,
                        _Out_ XUINT32* pLineCount)
{
    HRESULT hr = S_OK;
    XUINT32 index = 0;
    XUINT32 maxRow =0;
    XUINT32 maxCol =0;
    XINT32 maxItemsPerLine = -1;
    XUINT32 rowTileCount = 0;
    XUINT32 columnTileCount = 0;
    XUINT32 rowSpan =0;
    XUINT32 columnSpan =0;
    bool isPanelFull = false;
    ctl::ComPtr<IUIElement> spItem = NULL;

    IFCPTR(spChildren);
    IFCPTR(pItemSize);
    IFCPTR(pMaximumSize);
    IFCPTR(pLineCount);
    IFCPTR(pItemsPerLine);

    if(itemCount <= 0)
    {
        *pLineCount = 0;
        *pItemsPerLine = 0;
        goto Cleanup;
    }

    IFC(get_MaximumRowsOrColumns(&maxItemsPerLine));

    // Find out maxCol and maxRow
    m_pRowSpans = new XUINT32[itemCount];
    m_pColumnSpans = new XUINT32[itemCount];

    for (index = 0; index < itemCount; index++)
    {
        IFC(spChildren->GetAt(index, &spItem));
        IFCPTR(spItem);

        IFC(GetRowAndColumnSpan(spItem, &rowSpan, &columnSpan));
        m_pRowSpans[index] = rowSpan;
        m_pColumnSpans[index] = columnSpan;

        //This may go out of XUINT32 size, is total items are very large
        rowTileCount += rowSpan;
        columnTileCount += columnSpan;
    }

    // Determine the number of items that will fit per line
    *pItemsPerLine = DoubleUtil::IsInfinity(pMaximumSize->get_Direct()) ?
        (isHorizontalOrientation ? columnTileCount : rowTileCount) :
        static_cast<XUINT32>(pMaximumSize->get_Direct() / pItemSize->get_Direct());
    *pItemsPerLine = MAX(1, *pItemsPerLine);
    if (maxItemsPerLine > 0)
    {
        *pItemsPerLine = MIN(*pItemsPerLine, static_cast<UINT>(maxItemsPerLine));
    }

    // if growing size is Infinity, we will use tileCount for maxCol
    // else we will use maxSize.Direct/itemSize.Direct
    maxCol = isHorizontalOrientation ? rowTileCount : columnTileCount;
    if(!DoubleUtil::IsInfinity(pMaximumSize->get_Indirect()))
    {
        maxCol = __min(static_cast<UINT>(floor(pMaximumSize->get_Indirect()/ pItemSize->get_Indirect())), maxCol);
    }
    maxCol = MAX(1, maxCol);

    maxRow = *pItemsPerLine;

    // If map is not null, delete existing Map
    if(m_pMap != NULL)
    {
        delete m_pMap;
    }
    IFC(OccupancyMap::CreateOccupancyMap(maxRow, maxCol, isHorizontalOrientation, m_itemSize, itemCount, &m_pMap));

    //Now fill the items in OccupancyMap
    for (UINT i = 0; i < itemCount; i++)
    {
        rowSpan = m_pRowSpans[i];
        columnSpan = m_pColumnSpans[i];
        IFC(m_pMap->FindAndFillNextAvailableRect(rowSpan, columnSpan, ToAdjustedIndex(i), &isPanelFull));
        if(isPanelFull)
        {
            IFC(m_pMap->UpdateElementCount(i));
            break;
        }
    }

    // Find Used GridSize
    IFC(m_pMap->FindUsedGridSize(pLineCount, pItemsPerLine));

Cleanup:
    RRETURN(hr);
}

// ArrangeOverride
// Uses OccupancyMap to arrange elements
IFACEMETHODIMP VariableSizedWrapGrid::ArrangeOverride(
                    _In_ wf::Size finalSize,
                    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;
    XUINT32 itemCount = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren = NULL;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    wf::Size startingSize = {0.0, 0.0};
    wf::Size justificationSize = {0.0, 0.0};
    OrientedSize orientedStartingSize;
    OrientedSize orientedJustificationSize;
    wf::Size totalJustificationSize = {0.0, 0.0};

    IFCPTR(returnValue);
    IFC(get_Orientation(&orientation));
    IFC(get_Children(&spChildren));
    IFC(spChildren->get_Size(&itemCount));

    *returnValue = finalSize;
    // if itemCount > 0 then only we need to validate m_pMap
    if(itemCount > 0)
    {
        IFCEXPECT(m_pMap);

        IFC(ComputeHorizontalAndVerticalAlignment(finalSize, &orientedStartingSize, &orientedJustificationSize));
        IFC(orientedJustificationSize.AsUnorientedSize(&justificationSize));
        IFC(orientedStartingSize.AsUnorientedSize(&startingSize));

        // gothrough each Children and Arrange Them
        for (XUINT32 i = 0; i < itemCount; i++)
        {
            bool isHandled = false;
            ctl::ComPtr<IUIElement> spItem = NULL;
            wf::Rect elementRect = {0, 0, 0, 0};
            IFC(spChildren->GetAt(i, &spItem));
            IFCPTR(spItem);

            // If Map was full before laying down all items, then there is possibility that
            // perticular element is not in the map, in that case we won't arrange remaining items
            IFC(m_pMap->SetCurrentItem(i, &isHandled));
            if(!isHandled)
            {
                break;
            }

            IFC(m_pMap->GetCurrentItemRect(&elementRect));

            // If we have MaximumRowsOrColumns=4, enough space for 8 rows, and user passes a 5x5 tile, we should still show 5x5 tile
            // The elementRect calculated by OccupancyMap will force clipping if total RowSpan > MaximumRowsOrColumns
            // Force Clipping only  occurs when the item starts from rowIndex = 0 or ColIndex=0 based on Orientation
            // The following code avoid the force clipping
            if(elementRect.Y == 0 || elementRect.X == 0)
            {
                XUINT32 rowSpan = 0;
                XUINT32 colSpan = 0;
                IFC(GetRowAndColumnSpan(spItem, &rowSpan, &colSpan));

                // Check the possibility of force Clipping
                if(orientation == xaml_controls::Orientation_Vertical
                    && elementRect.Y == 0
                    && rowSpan >= m_itemsPerLine)
                {
                    XDOUBLE height = __min(rowSpan*m_itemSize.Height, finalSize.Height);
                    elementRect.Height = static_cast<FLOAT>(__max(elementRect.Height, height));
                }
                else if(orientation == xaml_controls::Orientation_Horizontal
                    && elementRect.X == 0
                    && colSpan >= m_itemsPerLine)
                {
                    XDOUBLE width = __min(colSpan*m_itemSize.Width, finalSize.Width);
                    elementRect.Width = static_cast<FLOAT>(__max(elementRect.Width, width));
                }
            }

            if(i % m_itemsPerLine == 0)
            {
                if(orientation == xaml_controls::Orientation_Horizontal)
                {
                    totalJustificationSize.Width = 0;
                    totalJustificationSize.Height = justificationSize.Height * ((XUINT32) (i/m_itemsPerLine) + 1);
                }
                else
                {
                    totalJustificationSize.Height = 0;
                    totalJustificationSize.Width = justificationSize.Width * ((XUINT32) (i/m_itemsPerLine) + 1);
                }
            }

            if(orientation == xaml_controls::Orientation_Horizontal)
            {
                totalJustificationSize.Width += justificationSize.Width;
            }
            else
            {
                totalJustificationSize.Height += justificationSize.Height;
            }

            elementRect.X += startingSize.Width + totalJustificationSize.Width;
            elementRect.Y += startingSize.Height + totalJustificationSize.Height;

            IFC(spItem->Arrange(elementRect));
        }
    }

Cleanup:
    RRETURN(hr);
}

// for a given UIElement, this method returns ColumnSpan and RowSpan
_Check_return_ HRESULT VariableSizedWrapGrid::GetRowAndColumnSpan(
                            _In_ ctl::ComPtr<IUIElement> spItem,
                            _Out_ XUINT32* rowSpan,
                            _Out_ XUINT32* columnSpan)
{
    HRESULT hr = S_OK;
    INT rowspan = 0;
    INT columnspan = 0;

    ctl::ComPtr<xaml_controls::IVariableSizedWrapGridStatics> spStatics;
    ctl::ComPtr<IActivationFactory> spActivationFactory;

    IFCPTR(spItem);
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::VariableSizedWrapGridFactory>::CreateActivationFactory());

    IFC(spActivationFactory.As(&spStatics));

    IFC(spStatics->GetRowSpan(spItem.Get(), &rowspan));
    IFC(spStatics->GetColumnSpan(spItem.Get(), &columnspan));

    if(columnspan <= 0)
    {
        columnspan = 1;
    }

    if(rowspan <= 0)
    {
        rowspan=1;
    }

    *columnSpan = static_cast<XUINT32>(columnspan);
    *rowSpan = static_cast<XUINT32>(rowspan);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VariableSizedWrapGrid::SupportsKeyNavigationAction(
            _In_ xaml_controls::KeyNavigationAction action,
            _Out_ BOOLEAN* pSupportsAction)
{
    // Let the Selector handle Next/Previous.
    *pSupportsAction =
        (action != xaml_controls::KeyNavigationAction_Next) &&
        (action != xaml_controls::KeyNavigationAction_Previous);

    RRETURN(S_OK);
}

// Calls the GetTargetIndexFromNavigationAction from OccupancyMap
_Check_return_ HRESULT VariableSizedWrapGrid::GetTargetIndexFromNavigationAction(
            _In_ UINT sourceIndex,
            _In_ xaml_controls::ElementType /*sourceType*/,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ BOOLEAN allowWrap,
            _In_ UINT /*itemIndexHintForHeaderNavigation*/,
            _Out_ UINT* computedTargetIndex,
            _Out_ xaml_controls::ElementType* /*pComputedTargetElementType*/,
            _Out_ BOOLEAN* actionValidForSourceIndex)
{
    HRESULT hr = S_OK;
    UINT newElementIndex = 0;
    IFCPTR(actionValidForSourceIndex);
    IFCPTR(computedTargetIndex);

    *computedTargetIndex = 0;
    *actionValidForSourceIndex = FALSE;
    IFC(m_pMap->GetTargetIndexFromNavigationAction(sourceIndex, action, allowWrap, &newElementIndex, actionValidForSourceIndex));
    if(*actionValidForSourceIndex)
    {
        // since internally we store sourceIndex + 1, need to -1 to
        // return correct sourceIndex
        *computedTargetIndex = static_cast<INT>(FromAdjustedIndex(newElementIndex));
    }

Cleanup:
    RRETURN(hr);
}

// Computes the bounds used to layout the items.
_Check_return_ HRESULT VariableSizedWrapGrid::ComputeHorizontalAndVerticalAlignment(
    // The available size we have to layout the child items
    _In_ wf::Size availableSize,
    // The calculated amount of space we'll reserve from the top/left corner for
    // alignment (this is really being used more like an OrientedPoint, but
    // there's no need to create a separate struct just for this)
    _Out_ OrientedSize* pStartingSize,
    // The calculated amount of space we'll reserve between each item for
    // alignment if we're justifying the items
    _Out_ OrientedSize* pJustificationSize)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    OrientedSize maximumSize;
    XDOUBLE startingOffset = 0.0;
    XDOUBLE justificationOffset = 0.0;
    xaml::HorizontalAlignment horizontalAlign = xaml::HorizontalAlignment_Left;
    xaml::VerticalAlignment verticalAlign = xaml::VerticalAlignment_Top;
    bool isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    IFCPTR(pStartingSize);
    IFCPTR(pJustificationSize);

    IFC(get_Orientation(&orientation));

    // Orient the maximum size
    maximumSize.put_Orientation(orientation);
    maximumSize.put_Width(availableSize.Width);
    maximumSize.put_Height(availableSize.Height);

    pStartingSize->put_Orientation(orientation);
    pJustificationSize->put_Orientation(orientation);
    IFC(get_HorizontalChildrenAlignment(&horizontalAlign));
    IFC(get_VerticalChildrenAlignment(&verticalAlign));

    // Determine how to adjust the items for horizontal content alignment
    IFC(WrapGrid::ComputeAlignmentOffsets(
        static_cast<XUINT32>(isHorizontal? verticalAlign : horizontalAlign),
        maximumSize.get_Width(),
        m_itemSize.Width * (isHorizontal ? m_itemsPerLine : m_lineCount),
        isHorizontal ? m_itemsPerLine : m_lineCount,
        &startingOffset,
        &justificationOffset));
    pStartingSize->put_Width(startingOffset);
    pJustificationSize->put_Width(justificationOffset);

    // Determine how to adjust the items for vertical content alignment
    IFC(WrapGrid::ComputeAlignmentOffsets(
        static_cast<XUINT32>(isHorizontal ? horizontalAlign : verticalAlign),
        maximumSize.get_Height(),
        m_itemSize.Height * (isHorizontal ? m_lineCount : m_itemsPerLine),
        isHorizontal ? m_lineCount : m_itemsPerLine,
        &startingOffset,
        &justificationOffset));
    pStartingSize->put_Height(startingOffset);
    pJustificationSize->put_Height(justificationOffset);

Cleanup:
    RRETURN(hr);
}

// Logical Orientation override
_Check_return_ HRESULT VariableSizedWrapGrid::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    *pValue = orientation;

Cleanup:
    RRETURN(hr);
}

// Physical Orientation override
_Check_return_ HRESULT VariableSizedWrapGrid::get_PhysicalOrientation(
     _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    if(orientation == xaml_controls::Orientation_Vertical)
    {
        *pValue = xaml_controls::Orientation_Horizontal;
    }
    else
    {
        *pValue = xaml_controls::Orientation_Vertical;
    }

Cleanup:
    RRETURN(hr);
}
