// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a VariableSizeWrapGrid
//      VariableSizeWrapGrid is used to create variable size tiles inside the grid.
//
//      It has OccupancyBlock and OccupancyMap nested classes
//      OccupancyMap: This class is used to arrange all items of variableSizeGrid
//      Since the size of OccupancyMap can be Infinite, OccupancyBlock is used to
//      allocate block of Memory
//
//      OccupancyMap always arrange items vertically(row), whether user sets
//      Orientation=Vertical or Horizontal. OccupancyMap's public APIs take care of
//      providing correct Orientation. Similarly OccupancyBlock has fixed rows while
//      it is growing on Column direction
//
//      Both classes are nested inside the VariableSizedWrapGrid class

#pragma once

#include "VariableSizedWrapGrid.g.h"

namespace DirectUI
{
    struct OrientedSize;

    // Represents a VariableSizeWrapGrid.
    PARTIAL_CLASS(VariableSizedWrapGrid)
    {
    public:

    // OccupancyBlock class
    // This class contains the block of 2 dimensional Array which will be used to map used/empty
    // tiles in VariableSizedWrapGrid Panel
    // BLOCK_SIZE is the size of the block it allocates.
    // This has link to another block of Data, so if total size of grid is larger then current Block,
    // it uses next Block
    class OccupancyBlock final
    {
    private:
        // Maximum number of Rows in Variable Size Grid
        XUINT32 m_maxRow;
        // Maximum number of Columns in Variable Size Grid
        // If isHorizontalOrientation is false, this will be Grid Width
        // OR if width is infinite, this will be maximum tiles needs all items layout Horizontally
        // If isHorizontalOrientation is true, this will be Grid Height
        // OR if height is infinite, this will be maximum tiles needs all items layout vertically
        XUINT32 m_maxCol;

        // 2 dimensional Array which will be used to map used/empty tiles in VariableSizedWrapGrid Panel
        // it stores UIElementIndex + 1 to specify which element has occupied particular area
        // value 0 is reserved for default, which means it is empty.
        XUINT32** m_pOccupancyArray;

        // Pointer to next OccupancyBlock
        OccupancyBlock* m_pNextBlock;

        // static BLOCK_SIZE, default is 50 for now
        static const XUINT32 BLOCK_SIZE = 50;

        //private constructor
        OccupancyBlock();

    public:
        // Static method used to create OccupancyBlock
        static _Check_return_ HRESULT CreateOccupancyBlock(
            _In_ XUINT32 maxRow,
            _In_ XUINT32 maxCol,
            _Outptr_ OccupancyBlock** ppOccupancyBlock);

        // GetItem from given location
        _Check_return_ HRESULT GetItem(
            _In_ XUINT32 rowIndex,
            _In_ XUINT32 colIndex,
            _Out_ XUINT32* pElementIndex);

        // SetItem to given Location
        _Check_return_ HRESULT SetItem(
            _In_ XUINT32 rowIndex,
            _In_ XUINT32 colIndex,
            _In_ XUINT32 elementIndex);

        // returns whether the given location is occupied or not
        _Check_return_ HRESULT IsOccupied(
            _In_ XUINT32 rowIndex,
            _In_ XUINT32 colIndex,
            _Out_ bool* pIsOccupied);

        // This method go through all blocks and return the block which has given ColumnIndex
        // It goes only on forward direction, it doesn't go on Backward direction
        _Check_return_ HRESULT GetBlock(
            _In_ XUINT32* colIndex,
            _Outptr_ OccupancyBlock** ppBlock);

        virtual ~OccupancyBlock();
    };

    // This class contains the occupancyMap for VariableSizeWrapGrid Panel
    // OccupancyMap always arrange items vertically(row), whether user sets
    // Orientation=Vertical or Horizontal. OccupancyMap's public APIs take care of
    // providing correct Orientation.
    class OccupancyMap final
    {
    private:
        // Internal struct used to keep track of row and colIndex
        // This will be used to track the location of each element in OccupancyMap
        struct MapLocation
        {
            XUINT32 ColIndex;
            XUINT32 RowIndex;
        };

        // Maximum number of Rows in Variable Size Grid
        XUINT32 m_maxRow;
        // Maximum number of Columns in Variable Size Grid
        // If isHorizontalOrientation is false, this will be Grid Width
        // OR if width is infinite, this will be maximum tiles needs all items layout Horizontally
        // If isHorizontalOrientation is true, this will be Grid Height
        // OR if height is infinite, this will be maximum tiles needs all items layout vertically
        XUINT32 m_maxCol;
        // Max number of rows used by arranged items
        XUINT32 m_maxUsedRow;
        // Max number of Columns used by arranged items
        XUINT32 m_maxUsedCol;

        bool m_isHorizontalOrientation;
        XUINT32 m_currentRowIndex;
        XUINT32 m_currentColumnIndex;
        wf::Size m_itemSize{};
        OccupancyBlock* m_pOccupancyBlock;

        // contains the list of locations (rowIndex,colIndex) for each Element
        // This way it is easy to find out the row/colIndex for given Element
        MapLocation* m_pElementLocation;

        // number of elements in grid
        XUINT32 m_elementCount;

        OccupancyMap();

    public:
        // static method creates OccupancyMap
        static _Check_return_ HRESULT CreateOccupancyMap(
            _In_ XUINT32 maxrow,
            _In_ XUINT32 maxcol,
            _In_ bool isHorizontalOrientation,
            _In_ wf::Size itemSize,
            _In_ XUINT32 itemCount,
            _Outptr_ OccupancyMap** ppOccupancyMap);

        virtual ~OccupancyMap();

        // This method finds next available Rect and occupies it
        _Check_return_ HRESULT FindAndFillNextAvailableRect(
            _In_ XUINT32 rowSpan,
            _In_ XUINT32 columnSpan,
            _In_ XUINT32 elementIndex,
            _Out_ bool* isMapFull);

        // This method finds the used grid size
        // This method is used by Measure to find required size
        _Check_return_ HRESULT FindUsedGridSize(
            _Out_ XUINT32* pMaxColIndex,
            _Out_ XUINT32* pMaxRowIndex);

        // Returns currentItemRectangle
        // From the current item indexes, this method finds the rowSpan and ColumnSpan
        // and using rowSpan, columnSpan, itemHeight and itemWidth, it finds the elementRect
        _Check_return_ HRESULT GetCurrentItemRect(
            _Out_ wf::Rect* pElementRect);

        // For a given Row and Column Index, this returns the ElementIndex
        _Check_return_ HRESULT GetCurrentElementIndex(
            _Out_ XUINT32* pElementIndex);

        // This will set currentItem Indexes for given elementIndex
        // if not succeeded, this will set current Indexes to 0
        _Check_return_ HRESULT SetCurrentItem(
            _In_ UINT32 elementIndex,
            _Out_ bool* isHandled);

        // Update the ElementCount in Map
        // This is required in the case when Map gets full before arranging all items
        _Check_return_ HRESULT UpdateElementCount(
            _In_ UINT32 elementCount);

        // Starting from sourceIndex, returns the index in the given direction.
        // OccupancyMap internally remembers the currentRow and ColumnIndex
        // if the sourceIndex at currentRow/Column Position is same as the sourceIndex (param),
        // it uses currentRow/ColumnIndex,
        // If not, it uses row/columnIndex from sourceIndex (topLeft tile)
        // This method returns the pAdjacentElementIndex based on FocusNavigationDirection
        //
        // pActionValidForSourceIndex can be FALSE in following conditions:
        // If sourceIndex is 0 and Key is Up or Left
        // If sourceIndex is maxRow -1, maxCol -1 and Key is Right or Down
        // if Key is not Up, Down, Right or Left
        _Check_return_ HRESULT GetTargetIndexFromNavigationAction(
            _In_ UINT sourceIndex,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ BOOLEAN allowWrap,
            _Out_ UINT* pComputedTargetIndex,
            _Out_ BOOLEAN* pActionValidForSourceIndex);

    private:
        // Internal method, finds next available rectangle
        _Check_return_ HRESULT FindNextAvailableRectInternal(
            _In_ XUINT32 rowSpan,
            _In_ XUINT32 columnSpan,
            _Out_ bool* pIsMapFull);

        // Internal method, Occupies current Rectangle with given elementIndex
        _Check_return_ HRESULT FillCurrentItemRect(
            _In_ XUINT32 rowSpan,
            _In_ XUINT32 columnSpan,
            _In_ XUINT32 elementIndex);

        // Common method for MoveUp and Left
        _Check_return_ HRESULT MoveUpOrLeft(
            _In_ XUINT32& moveDirectionIndex,
            _Out_ BOOLEAN* pIsHandled);

        // Common method for MoveDown and Right
        _Check_return_ HRESULT MoveDownOrRight(
            _In_ XUINT32& moveDirectionIndex,
            _In_ XUINT32 maxCountInMoveDirection,
            _Out_ BOOLEAN* pIsHandled);

    };

    private:
        wf::Size m_itemSize;

        //Occupancy Map arrange all items in 2 dimensional map
        // it is used to find the ElementRect for particular item
        OccupancyMap* m_pMap;

        // Saves the RowSpan and ColumnSpans attached properties value for each item
        // this is used in OccupancyMap
        XUINT32* m_pRowSpans;
        XUINT32* m_pColumnSpans;
        XUINT32 m_itemsPerLine;
        XUINT32 m_lineCount;

        // This method creates Occupancy Map, arrange children and returns ItemsPerLine
        // and LineCount
        _Check_return_ HRESULT CalculateOccupancyMap(
                _In_ UINT32 itemCount,
                _In_ ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren,
                _In_ bool isHorizontalOrientation,
                _In_ OrientedSize* pItemSize,
                _In_ OrientedSize* pMaximumSize,
                _Out_ UINT32* pItemsPerLine,
                _Out_ UINT32* pLineCount);

        // Computes the bounds used to layout the items.
        _Check_return_ HRESULT ComputeBounds(
                _In_ wf::Size availableSize,
                _Out_ OrientedSize* pTileSize,
                _Out_ OrientedSize* pMaximumPanelSize,
                _Out_ bool* pAreItemsOriented);

        // for a given child this method returns the RowSpan and ColumnSpan property
        _Check_return_ HRESULT GetRowAndColumnSpan(
                _In_ ctl::ComPtr<IUIElement> spItem,
                _Out_ XUINT32* pRowSpan,
                _Out_ XUINT32* pColumnSpan);

        // Computes Horizontal and vertical alignments
            _Check_return_ HRESULT ComputeHorizontalAndVerticalAlignment(
                _In_ wf::Size availableSize,
                _Out_ OrientedSize* pStartingSize,
                _Out_ OrientedSize* pJustificationSize);

    protected:
        // Handle the custom property changed event and call the
        // OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args)
                override;

    public:
        VariableSizedWrapGrid();
        ~VariableSizedWrapGrid() override;

        // MeasureOverride
        IFACEMETHOD(MeasureOverride)(
                            _In_ wf::Size availableSize,
                          _Out_ wf::Size* pReturnValue)
                            override;

        // ArrangeOverride
        IFACEMETHOD(ArrangeOverride)(
                            _In_ wf::Size arrangeSize,
                            _Out_ wf::Size* pReturnValue)
                            override;

        IFACEMETHOD(SupportsKeyNavigationAction)(
                _In_ xaml_controls::KeyNavigationAction action,
                _Out_ BOOLEAN* pSupportsAction)
                override;

        // Starting from elementIndex, returns the index in the given direction.
        // OccupancyMap internally remembers the currentRow and ColumnIndex
        // if the elementIndex at currentRow/Column Position is same as the elementIndex (param),
        // it uses currentRow/ColumnIndex,
        // If not, it uses row/columnIndex from elementIndex (topLeft tile)
        // This method returns the pAdjacentElementIndex based on FocusNavigationDirection
        //
        // pActionValidForSourceIndex can be FALSE in following conditions:
        // If elementIndex is 0 and Key is Up or Left
        // If elementIndex is maxRow -1, maxCol -1 and Key is Right or Down
        // if Key is not Up, Down, Right or Left
        IFACEMETHOD(GetTargetIndexFromNavigationAction)(
            _In_ UINT elementIndex,
            _In_ xaml_controls::ElementType elementType,
            _In_ xaml_controls::KeyNavigationAction action,
            _In_ BOOLEAN allowWrap,
            _In_ UINT itemIndexHintForHeaderNavigation,
            _Out_ UINT* computedTargetIndex,
            _Out_ xaml_controls::ElementType* computedTargetElementType,
            _Out_ BOOLEAN* actionValidForSourceIndex) override;

        // Logical Orientation override
        IFACEMETHOD(get_LogicalOrientation)(
            _Out_ xaml_controls::Orientation* pValue)
            override;

        // Physical Orientation override
        IFACEMETHOD(get_PhysicalOrientation)(
            _Out_ xaml_controls::Orientation* pValue)
            override;
    };
}
