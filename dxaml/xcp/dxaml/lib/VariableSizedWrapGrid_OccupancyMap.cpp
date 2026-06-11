// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a OccupancyBlock and OccupancyMap
//      OccupancyMap: This class is used to arrange all items of variableSizeGrid
//      Since the size of OccupancyMap can be Infinite, OccupancyBlock is used to 
//      allocate block of Memory
//
//      OccupncyMap always arrange items vertically(row), whether user sets 
//      Orientation=Vertical or Horizontal. OccupancyMap's public APIs take care of
//      providing correct Orientation. Similarly OccupancyBlock has fixed rows while
//      it is gorwing on Column direction
//      Both classes are nested inside the VariableSizedWrapGrid class

#include "precomp.h"
#include "VariableSizedWrapGrid.g.h"
#define FromAdjustedIndex(value) ((value) - 1)

using namespace DirectUI;

// Private Constructor
VariableSizedWrapGrid::OccupancyBlock::OccupancyBlock()
    : m_pNextBlock(NULL),
    m_pOccupancyArray(NULL),
    m_maxRow(0),
    m_maxCol(0)
{
}

//Static Method, Creates an instance of OccupancyBlock
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyBlock::CreateOccupancyBlock(
        _In_ XUINT32 maxRow, 
        _In_ XUINT32 maxCol, 
        _Outptr_ VariableSizedWrapGrid::OccupancyBlock** ppOccupancyBlock)
{
    HRESULT hr = S_OK;
    XUINT32 columnSize = 0;
    OccupancyBlock* pOccupancyBlock = NULL;

    IFCPTR(ppOccupancyBlock);
    pOccupancyBlock = new OccupancyBlock();

    pOccupancyBlock->m_maxRow = maxRow;
    pOccupancyBlock->m_maxCol = maxCol;

    // Allocates memeory to block
    // Row is fixed size while column is either BlockSize or Column size
    // whicheve is minimum
    columnSize = maxCol > BLOCK_SIZE ? BLOCK_SIZE : maxCol;
    pOccupancyBlock->m_pOccupancyArray = new XUINT32*[maxRow];
    for(XUINT32 i = 0; i < maxRow; i++)
    {
        pOccupancyBlock->m_pOccupancyArray[i] = new XUINT32[columnSize];
        ZeroMemory(pOccupancyBlock->m_pOccupancyArray[i], columnSize*sizeof(UINT32));
    }
    *ppOccupancyBlock = pOccupancyBlock;

Cleanup:
    RRETURN(hr);
}

//destructor
VariableSizedWrapGrid::OccupancyBlock::~OccupancyBlock()
{
    delete m_pNextBlock;

    for(UINT i = 0; i < m_maxRow; ++i)
    {
        delete[] m_pOccupancyArray[i]; 
    }

    delete[]  m_pOccupancyArray;
}

// saves elementIndex in 2 dimensional OccupancyArray
 _Check_return_ HRESULT VariableSizedWrapGrid::OccupancyBlock::SetItem(
     _In_ XUINT32 rowIndex,                    
     _In_ XUINT32 colIndex, 
     _In_ XUINT32 elementIndex)
 {
    HRESULT hr = S_OK;
    OccupancyBlock* pBlock = NULL;

    IFCEXPECT(colIndex < m_maxCol);
    IFCEXPECT(rowIndex < m_maxRow);
    IFC(GetBlock(&colIndex, &pBlock));
    pBlock->m_pOccupancyArray[rowIndex][colIndex] = elementIndex;

Cleanup:
    RRETURN(hr);
 }

 // IsOccupied
 // checkes whether the given position is already occupied or not
 _Check_return_ HRESULT VariableSizedWrapGrid::OccupancyBlock::IsOccupied(
     _In_ XUINT32 rowIndex, 
     _In_ XUINT32 colIndex, 
     _Out_ bool* pIsOccupied)
 {
     HRESULT hr = S_OK;
     XUINT32 elementIndex = 0;

     IFCPTR(pIsOccupied);
     IFC(GetItem(rowIndex, colIndex, &elementIndex));
     *pIsOccupied = (elementIndex != 0);

Cleanup:
     RRETURN(hr);
 }

 // GetItem
 _Check_return_ HRESULT VariableSizedWrapGrid::OccupancyBlock::GetItem(
     _In_ XUINT32 rowIndex, 
     _In_ XUINT32 colIndex, 
     _Out_ XUINT32* pElementIndex)
 {
    HRESULT hr = S_OK;
    OccupancyBlock* pBlock = NULL;

    IFCPTR(pElementIndex);
    IFCEXPECT(colIndex < m_maxCol);
    IFCEXPECT(rowIndex < m_maxRow);
    IFC(GetBlock(&colIndex, &pBlock));
    *pElementIndex = pBlock->m_pOccupancyArray[rowIndex][colIndex];

Cleanup:
    RRETURN(hr);
 }
 
// For a given ColIndex, this methods returns the Block which contains this colIndex
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyBlock::GetBlock(
      _Inout_ XUINT32* pColIndex,
      _Outptr_ OccupancyBlock** ppBlock)
{
    HRESULT hr = S_OK;
    OccupancyBlock* pBlock = NULL;
    UINT32 maxCol = m_maxCol;

    IFCPTR(ppBlock);
    IFCPTR(pColIndex);

    pBlock = this;
    while(*pColIndex >= BLOCK_SIZE)
    {
        *pColIndex = *pColIndex - BLOCK_SIZE;
        if(pBlock->m_pNextBlock == NULL)
        {
            maxCol = maxCol - BLOCK_SIZE;
            IFC(OccupancyBlock::CreateOccupancyBlock(m_maxRow, maxCol, &pBlock->m_pNextBlock));
        }
        pBlock = pBlock->m_pNextBlock;
    }

    *ppBlock = pBlock;
Cleanup:
    RRETURN(hr);
}

// Constructor
VariableSizedWrapGrid::OccupancyMap::OccupancyMap()
    :m_isHorizontalOrientation(FALSE),
     m_maxCol(0),
     m_maxRow(0),
     m_currentRowIndex(0),
     m_currentColumnIndex(0),
     m_pOccupancyBlock(NULL),
     m_pElementLocation(NULL),
     m_elementCount(0),
     m_maxUsedRow(0),
     m_maxUsedCol(0)
{
}

VariableSizedWrapGrid::OccupancyMap::~OccupancyMap()
{
    delete [] m_pElementLocation;
    delete m_pOccupancyBlock;
}

// Static Method creates OccupancyMap
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::CreateOccupancyMap(
            _In_ XUINT32 maxRow, 
            _In_ XUINT32 maxCol, 
            _In_ bool isHorizontalOrientation,
            _In_ wf::Size itemSize,
            _In_ XUINT32 itemCount, 
            _Outptr_ OccupancyMap** ppOccupancyMap)
{
    HRESULT hr = S_OK;
    VariableSizedWrapGrid::OccupancyMap* pOccupancyMap = NULL;

    IFCPTR(ppOccupancyMap);

    pOccupancyMap = new VariableSizedWrapGrid::OccupancyMap();
    pOccupancyMap->m_isHorizontalOrientation = isHorizontalOrientation;
    pOccupancyMap->m_maxCol = maxCol;
    pOccupancyMap->m_maxRow = maxRow;
    pOccupancyMap->m_itemSize = itemSize;
    pOccupancyMap->m_elementCount = itemCount;

    IFC(OccupancyBlock::CreateOccupancyBlock(maxRow, maxCol, &pOccupancyMap->m_pOccupancyBlock));
    pOccupancyMap->m_pElementLocation = new MapLocation[pOccupancyMap->m_elementCount];

    *ppOccupancyMap = pOccupancyMap;
    pOccupancyMap = NULL;
Cleanup:
    delete pOccupancyMap;
    RRETURN(hr);
}

// For a given Row and Column Index, this returns the ElementIndex
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::GetCurrentElementIndex(
                        _Out_ XUINT32* pElementIndex)
{
    HRESULT hr = S_OK;
    IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, pElementIndex));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::UpdateElementCount(_In_ UINT32 elementCount)
{
    HRESULT hr = S_OK;
    m_elementCount = elementCount;

    RRETURN(hr);
}

// This will set currentItem Indexes for given elementIndex
// if not succeeded, this will set current Indexes to 0
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::SetCurrentItem(
        _In_ UINT32 elementIndex,
        _Out_ bool* pIsHandled)
{
    HRESULT hr = S_OK;
    IFCPTR(pIsHandled);
    
    *pIsHandled = FALSE;
    m_currentColumnIndex = 0;
    m_currentRowIndex = 0;

    if(m_elementCount > elementIndex)
    {
        MapLocation point = m_pElementLocation[elementIndex];
        m_currentColumnIndex = point.ColIndex;
        m_currentRowIndex = point.RowIndex;
        *pIsHandled = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// This method find the CurrentItem rectangle
// it uses currentItemIndexes and find the row and column span
// and uses them to find ItemRectangle 
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::GetCurrentItemRect(
                            _Out_ wf::Rect* pElementRect)
{
    HRESULT hr = S_OK;
    
    XUINT32 value = 0;
    XUINT32 outValue = 0;
    XINT32 colLeftIndex = m_currentColumnIndex;
    XINT32 colRightIndex = m_currentColumnIndex;
    XINT32 rowTopIndex = m_currentRowIndex;
    XINT32 rowBottomIndex = m_currentRowIndex;
    XUINT32 colSpan = 0;
    XUINT32 rowSpan = 0;
    
    IFCPTR(pElementRect);

    // Initialize pElementRect to 0
    pElementRect->X = 0;
    pElementRect->Y = 0;
    pElementRect->Width = 0;
    pElementRect->Height = 0;
    IFC(GetCurrentElementIndex(&value));
    
    // Find Column Left Index
    outValue = value;
    while(value == outValue)
    {
        colLeftIndex -= 1;
         if(colLeftIndex < 0)
         {
             break;
         }
        IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, colLeftIndex, &outValue));
    }
    colLeftIndex += 1;

    // Find Column Right Index
    outValue = value;
    while(value == outValue)
    {
        colRightIndex += 1;
        if(colRightIndex >= static_cast<INT>(m_maxCol))
        {
            break;
        }

        IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, colRightIndex, &outValue));
    }
    colRightIndex -= 1;

    // Find Row Top Index
    outValue = value;
    while(value == outValue)
    {
        rowTopIndex -= 1;
        if(rowTopIndex < 0)
        {
            break;
        }
        IFC(m_pOccupancyBlock->GetItem(rowTopIndex, m_currentColumnIndex, &outValue));
    }
    rowTopIndex += 1;
    
    // Find Row Bottom Index
    outValue = value;
    while(value == outValue)
    {
        rowBottomIndex += 1;
        if(rowBottomIndex >= static_cast<INT>(m_maxRow))
        {
            break;
        }
        IFC(m_pOccupancyBlock->GetItem(rowBottomIndex, m_currentColumnIndex, &outValue));
    }
    rowBottomIndex -= 1;
    
    // Calculate Row and Column Span
    colSpan = colRightIndex - colLeftIndex + 1;
    rowSpan = rowBottomIndex - rowTopIndex + 1;
    if(m_isHorizontalOrientation)
    {
        pElementRect->X = m_itemSize.Width * m_currentRowIndex;
        pElementRect->Y = m_itemSize.Height * m_currentColumnIndex;
        pElementRect->Width = m_itemSize.Width * rowSpan;
        pElementRect->Height = m_itemSize.Height * colSpan;
    }
    else
    {
        pElementRect->X = m_itemSize.Width * m_currentColumnIndex;
        pElementRect->Y = m_itemSize.Height * m_currentRowIndex;
        pElementRect->Width = m_itemSize.Width * colSpan;
        pElementRect->Height = m_itemSize.Height * rowSpan;
    }

Cleanup:
    RRETURN(hr);
}

 // This method occupies the current Rectangle
//explanin::: RowSpan and ColumnSpan will always striped.
 _Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::FillCurrentItemRect(
                                    _In_ XUINT32 rowSpan, 
                                    _In_ XUINT32 columnSpan,
                                    _In_ XUINT32 elementIndex)
{
    HRESULT hr = S_OK;
    MapLocation point = {0,0};

    for (XUINT32 column = 0; column < columnSpan; ++column)
    {
        for (XUINT32 row = 0; row < rowSpan; ++row)
        {
            if (m_currentColumnIndex + column < m_maxCol &&
                m_currentRowIndex + row < m_maxRow)
            {
                IFC(m_pOccupancyBlock->SetItem(m_currentRowIndex + row, m_currentColumnIndex + column, elementIndex));
            }
        }
    }

    // Saving the RowIndex and ColIndex for given element
    point.RowIndex = m_currentRowIndex;
    point.ColIndex = m_currentColumnIndex;
    m_pElementLocation[FromAdjustedIndex(elementIndex)] = point;

    m_currentRowIndex += rowSpan;
    if(m_currentRowIndex >= m_maxRow)
    {
        m_currentRowIndex = 0;
        m_currentColumnIndex += 1;
    }

Cleanup:
    RRETURN(hr);
}

// This calls FindNextAvailableRectIndex to find next available size and occupies it
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::FindAndFillNextAvailableRect(
                                            _In_ XUINT32 rowSpan, 
                                            _In_ XUINT32 columnSpan, 
                                            _In_ XUINT32 value,
                                            _Out_ bool* pIsMapFull)
{
    HRESULT hr = S_OK;
    XUINT32 rowSpanOriented = rowSpan;
    XUINT32 columnSpanOriented = columnSpan;

    IFCPTR(pIsMapFull);

    if(m_isHorizontalOrientation)
    {
        rowSpanOriented = columnSpan;
        columnSpanOriented = rowSpan;
    }

    
    IFC(FindNextAvailableRectInternal(rowSpanOriented, columnSpanOriented, pIsMapFull));
    if (!*pIsMapFull)
    {
        IFC(FillCurrentItemRect(rowSpanOriented, columnSpanOriented, value));
    }

Cleanup:
    RRETURN(hr);
}

// This method gothrough the grid from current Item Index and file the next availalbe position which can
// fit the RowSpan and ColumnSpan
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::FindNextAvailableRectInternal(
                                            _In_ XUINT32 rowSpan, 
                                            _In_ XUINT32 columnSpan, 
                                            _Out_ bool* pIsMapFull)
{
    HRESULT hr = S_OK;
    IFCPTR(pIsMapFull);

    if (rowSpan > m_maxRow)
    {
        rowSpan = m_maxRow;
    }

    if (columnSpan > m_maxCol)
    {
        columnSpan = m_maxCol;
    }

    // for each cell (row, col), this loop checks whether the cell is available or not,
    // if it is not available, this goes to next cell
    // if it is available, this loops through rowSpan and Column spans and verify whether
    // required cells for row and column span are available or now
    while (m_currentColumnIndex < m_maxCol)
    {
        bool isOccupied = true;

        if (m_currentRowIndex == m_maxRow)
        {
            m_currentRowIndex = 0;
            m_currentColumnIndex += 1;
        }

        for (XUINT32 j = 0; j < columnSpan; ++j)
        {
            // If currentIndex + colSpan is > maxCol, we need to reduce the ColSpan
            if (j + m_currentColumnIndex >= m_maxCol)
            {
                columnSpan = m_maxCol - m_currentColumnIndex;
                break;
            }

            // Go through all rows and check whether the position is available to not
            // If not, this will get out from RowSpan loop
            for (XUINT32 i = 0; i < rowSpan; ++i)
            {
                isOccupied = TRUE;
                if (m_currentRowIndex + i < m_maxRow && m_currentColumnIndex + j < m_maxCol)
                {
                    IFC(m_pOccupancyBlock->IsOccupied(m_currentRowIndex + i, m_currentColumnIndex + j, &isOccupied));
                }

                if(isOccupied)
                {
                    break;
                }
            }
            
            if (isOccupied)
            {
                break;
            }
        }

        // If the current position is occupied, or it is out of maxRow or maxCol range, 
        // go to the next Position (increase rowIndex)
        // if rowIndex is out of maxRow, goto next column, first position
        if (isOccupied)
        {
            m_currentRowIndex++;
            if (m_currentRowIndex == m_maxRow || m_currentRowIndex + rowSpan > m_maxRow)
            {
                m_currentRowIndex = 0;
                m_currentColumnIndex++;
            }
        }
        else
        {
            break;
        }
    }
    
    if (m_currentColumnIndex >= m_maxCol || m_currentRowIndex >= m_maxRow)
    {
        *pIsMapFull = TRUE;
        m_currentColumnIndex = 0;
        m_currentRowIndex = 0;
    }

Cleanup:
    RRETURN(hr);
}

// This method returns the used/occupied gidsize from current Items arrangement
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::FindUsedGridSize(
                            _Out_ XUINT32* pMaxColIndex, 
                            _Out_ XUINT32* pMaxRowIndex)
{
    HRESULT hr = S_OK;
    XUINT32 colindex = 0;
    XUINT32 rowindex = 0;
    bool isOccupied;
    IFCPTR(pMaxColIndex);
    IFCPTR(pMaxRowIndex);

    for (XUINT32 j = 0; j < m_maxCol; ++j)
    {
        for (XUINT32 i = 0; i < m_maxRow; ++i)
        {
            IFC(m_pOccupancyBlock->IsOccupied(i, j, &isOccupied));
            if (isOccupied)
            {
                colindex = j;
                break;
            }
        }
    }

    for (XUINT32 j = 0; j < m_maxRow; ++j)
    {
        for (XUINT32 i = 0; i < m_maxCol; ++i)
        {
            IFC(m_pOccupancyBlock->IsOccupied(j, i, &isOccupied));
            if (isOccupied)
            {
                rowindex = j;
                break;
            }
        }
    }

    if(m_isHorizontalOrientation)
    {
        *pMaxColIndex = colindex + 1;
        *pMaxRowIndex = rowindex + 1;
    }
    else
    {
        *pMaxRowIndex = rowindex + 1;
        *pMaxColIndex = colindex + 1;
    }

    // Saving the used Row and Col Index
    m_maxUsedRow = rowindex + 1;
    m_maxUsedCol = colindex + 1;
Cleanup:
    RRETURN(hr);
}

// Starting from sourceIndex, returns the index in the given direction.
// OccupancyMap internally remembers the currentRow and ColumnIndex
// if the sourceIndex at currentRow/Column Position is same as the sourceIndex (param),
// it uses currentRow/ColumnIndex, 
// If not, it uses row/columnIndex from sourceIndex (topLeft tile)
// This method returns the pAdjacentElementIndex based on KeyNavigationAction
//
// pActionValidForSourceIndex can be FALSE in following conditions:
// If sourceIndex is 0 and action is Up or Left
// If sourceIndex is maxRow -1, maxCol -1 and action is Right or Down
// if action is not Up, Down, Right or Left
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap:: GetTargetIndexFromNavigationAction(
    _In_ UINT sourceIndex, 
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ BOOLEAN allowWrap,
    _Out_ UINT* pComputedTargetIndex, 
    _Out_ BOOLEAN* pActionValidForSourceIndex)
{
    HRESULT hr = S_OK;
    UINT currentElementIndex = 0;
    MapLocation point = {0,0};

    *pActionValidForSourceIndex = FALSE;
    *pComputedTargetIndex = 0;
    
    if(sourceIndex > m_elementCount)
    {
        goto Cleanup;
    }

    IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, &currentElementIndex));
     
    // If sourceIndex at current Position is not same as required, need to update currentRow/ColumnIndex
    if(currentElementIndex != sourceIndex + 1)
    {
        point = m_pElementLocation[sourceIndex];
        m_currentRowIndex = point.RowIndex;
        m_currentColumnIndex = point.ColIndex;
    }

    if(action == xaml_controls::KeyNavigationAction_First ||
        action == xaml_controls::KeyNavigationAction_Last)
    {
        if (action == xaml_controls::KeyNavigationAction_First)
        {
            // if there are items top Left position will always filled up
            m_currentRowIndex = 0;
            m_currentColumnIndex = 0;
            *pActionValidForSourceIndex = TRUE;
        }
        else
        {
            // get the last element and returns it's locations
            point = m_pElementLocation[m_elementCount -1];
            m_currentRowIndex = point.RowIndex;
            m_currentColumnIndex = point.ColIndex;
            *pActionValidForSourceIndex = TRUE;
        }
    }
    else
    {
         // If horizontal Orientation, swap the Keys
         if(m_isHorizontalOrientation)
         {
            switch(action)
            {
                case xaml_controls::KeyNavigationAction_Left:
                {
                    IFC(MoveUpOrLeft(m_currentRowIndex, pActionValidForSourceIndex));
                    break;
                }
                case xaml_controls::KeyNavigationAction_Right:
                {
                     IFC(MoveDownOrRight (m_currentRowIndex, m_maxUsedRow, pActionValidForSourceIndex));
                     break;
                }
                case xaml_controls::KeyNavigationAction_Up:
                {
                    IFC(MoveUpOrLeft(m_currentColumnIndex, pActionValidForSourceIndex));
                     break;
                }
                case xaml_controls::KeyNavigationAction_Down:
                {
                     IFC(MoveDownOrRight (m_currentColumnIndex, m_maxUsedCol, pActionValidForSourceIndex));
                     break;
                }
            }
         }
         else
         {
              switch(action)
            {
                case xaml_controls::KeyNavigationAction_Left:
                {
                    IFC(MoveUpOrLeft(m_currentColumnIndex, pActionValidForSourceIndex));
                    break;
                }
                case xaml_controls::KeyNavigationAction_Right:
                {
                     IFC(MoveDownOrRight (m_currentColumnIndex, m_maxUsedCol, pActionValidForSourceIndex));
                     break;
                }
                case xaml_controls::KeyNavigationAction_Up:
                {
                     IFC(MoveUpOrLeft(m_currentRowIndex, pActionValidForSourceIndex));
                     break;
                }
                case xaml_controls::KeyNavigationAction_Down:
                {
                     IFC(MoveDownOrRight (m_currentRowIndex, m_maxUsedRow, pActionValidForSourceIndex));
                     break;
                }
            }
         }
    }
     
    if(*pActionValidForSourceIndex)
    {
        IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, pComputedTargetIndex));
    }

Cleanup:
    RRETURN(hr);
}

// this is common Method used by MoveUp and MoveLeft
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::MoveUpOrLeft(
        _In_ XUINT32& moveDirectionIndex,
        _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    XUINT32 elementIndex = 0;
    XUINT32 currentElementIndex = 0;
    
    IFCPTR(pIsHandled);
    *pIsHandled = FALSE;

    IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, &currentElementIndex));
    elementIndex = currentElementIndex;
    
    // if the currentElement is same as elementIndex or there is no element on current position
    while (elementIndex == currentElementIndex  || currentElementIndex == 0)
    {
        if(moveDirectionIndex == 0)
        {
            // reached the first element in the line
            goto Cleanup;
        }
        else
        {
            --moveDirectionIndex;
        }

        IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, &currentElementIndex));
    }

    *pIsHandled = TRUE;

Cleanup:
    RRETURN(hr);
}

// Common method for MoveDown and Right
_Check_return_ HRESULT VariableSizedWrapGrid::OccupancyMap::MoveDownOrRight(
            _In_ XUINT32& moveDirectionIndex,
            _In_ XUINT32 maxCountInMoveDirection,
            _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    XUINT32 elementIndex = 0;
    XUINT32 currentElementIndex = 0;

    ASSERT(moveDirectionIndex < maxCountInMoveDirection);
    IFCPTR(pIsHandled);
    *pIsHandled = FALSE;
    IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, &currentElementIndex));
    elementIndex = currentElementIndex;
    
    // if the currentElement is same as elementIndex or there is no element on current position
    while (elementIndex == currentElementIndex || currentElementIndex == 0 )
    {
        if (moveDirectionIndex >= maxCountInMoveDirection - 1)
        {
            // reached the last element in the line
            goto Cleanup;
        }
        else
        {
            ++moveDirectionIndex;
        }

        IFC(m_pOccupancyBlock->GetItem(m_currentRowIndex, m_currentColumnIndex, &currentElementIndex));
    }

    *pIsHandled = TRUE;

Cleanup:
    RRETURN(hr);
}
