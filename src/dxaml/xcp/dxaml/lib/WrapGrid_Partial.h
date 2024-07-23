// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      WrapGrid provides the default layout experience for the GridView
//      control. By default this is UI virtualized, it generates only those
//      containers which are being visible or focused

#pragma once

#include "WrapGrid.g.h"

namespace DirectUI
{
    // Forward declaration of the OrientedSize struct.
    struct OrientedSize;

    // WrapGrid provides the default layout experience for the GridView control.
    PARTIAL_CLASS(WrapGrid)
    {
        private:
            // ItemSize
            // This is calculated ItemSize using ItemWidth/ItemHeight and first child's Desired Size
            wf::Size m_itemSize;
            
            // Starting size
            // The extra spaces on TopLeft in the case when number of items are less then the WrapGrid Size and
            // HorizontalChildrenAlignment=Right or VerticalChildrenAlignment=Bottom
            wf::Size m_startingSize;
            
            // Justification size
            // The extra spaces around all items in the case when number of items are less then the WrapGrid Size and
            // HorizontalChildrenAlignment=Center or VerticalChildrenAlignment=Center
            wf::Size m_justificationSize;

            // position from which new items still start transitioning in
            wf::Point m_newItemsStartPosition;

            // number of lines to cache
            UINT m_preCacheLines;

            // whether to compute ItemSize or not
            // m_itemSize, m_itemsPerLine, m_lineCount will be compute on first time
            // It will be measured again when dataSource, alignment, ItemWidth, ItemHeight is changed.
            bool m_bShouldComputeItemSize;

            // The most recent size at which we arranged.
            // In data virtualization scenarios when we load items async, we need to know how many items can be loaded per page.
            wf::Size m_lastArrangeSize;

        protected:

#if PERF_TUNE
//Allows number of cached columns for wrapgrid to be set in registry
            //override Initialize() method to find m_perCachedLines from Registry
            _Check_return_ HRESULT Initialize()
                override;
#endif

            // Handle the custom property changed event and call the
            // OnPropertyChanged methods.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args)
                override;

            // returns the desired Size of a Container
            // In WrapGrid, this returns the ItemWidth and ItemHeight
            _Check_return_ HRESULT GetDesiredSize(
                _In_ IUIElement* pChild,
                _Out_ wf::Size* pDesiredSize)
                override;
            
            _Check_return_ HRESULT MeasureChildForItemsChanged(
                _In_ xaml::IUIElement* pChild) override;

            // Called when the Items collection associated with the containing ItemsControl changes.
            IFACEMETHOD(OnItemsChanged)(
                _In_ IInspectable* sender, 
                _In_ xaml_primitives::IItemsChangedEventArgs* args) override;

        public:
            WrapGrid();

            // Set the HorizontalOffset to the passed value.
            IFACEMETHOD(SetHorizontalOffset)(_In_ DOUBLE offset) override;
            
            // Set the VerticalOffset to the passed value.
            IFACEMETHOD(SetVerticalOffset)(_In_ DOUBLE offset) override;

            // Measures the items of a WrapGrid in anticipation of arranging
            // them during the ArrangeOverride pass.
            IFACEMETHOD(MeasureOverride)(
                _In_ wf::Size availableSize,
                _Out_ wf::Size* returnValue)
                override;

            // Arranges the items of a WrapGrid.
            IFACEMETHOD(ArrangeOverride)(
                _In_ wf::Size finalSize,
                _Out_ wf::Size* returnValue)
                override;
            
            _Check_return_ IFACEMETHOD(SupportsKeyNavigationAction)(
                _In_ xaml_controls::KeyNavigationAction action,
                _Out_ BOOLEAN* pSupportsAction)
                override;
            
            // Starting from sourceIndex, returns the index in the given direction.
            //
            // pActionValidForSourceIndex can be FALSE in following conditions:
            // If sourceIndex is 0 and Key is Up or Left
            // If sourceIndex is maxRow -1, maxCol -1 and Key is Right or Down
            // if Key is not Up, Down, Right or Left
            _Check_return_ IFACEMETHOD(GetTargetIndexFromNavigationAction)(
                _In_ UINT sourceIndex,
                _In_ xaml_controls::ElementType sourceType,
                _In_ xaml_controls::KeyNavigationAction action,
                _In_ BOOLEAN allowWrap,
                _In_ UINT itemIndexHintForHeaderNavigation,
                _Out_ UINT* computedTargetIndex,
                _Out_ xaml_controls::ElementType* computedTargetElementType,
                _Out_ BOOLEAN* actionValidForSourceIndex)
                override;
        
            // Logical Orientation override
            _Check_return_ IFACEMETHOD(get_LogicalOrientation)(
                _Out_ xaml_controls::Orientation* pValue) 
                override;
            
            // Physical Orientation override
            _Check_return_ IFACEMETHOD(get_PhysicalOrientation)(
                _Out_ xaml_controls::Orientation* pValue) 
                override;

            // Sets up IsVirtualizing, VirtualizationMode
            _Check_return_ HRESULT SetVirtualizationState(
                _In_ xaml_controls::IItemsControl* pItemsControl);

            // Computes the content alignment offsets.
            // Static method used by WrapGrid as well as VariableSizedWrapGrid
            static _Check_return_ HRESULT ComputeAlignmentOffsets(
                _In_ XUINT32 alignment,
                _In_ XDOUBLE availableSize,
                _In_ XDOUBLE requiredSize,
                _In_ XUINT32 totalLines,
                _Out_ XDOUBLE* pStartingOffset,
                _Out_ XDOUBLE* pJustificationOffset);

            // Get the closest element information to the point.
            _Check_return_ IFACEMETHOD(GetClosestElementInfo)(
                _In_ wf::Point position, 
                _Out_ xaml_primitives::ElementInfo* returnValue) 
                override;
        
            // Get the index where an item should be inserted if it were dropped at
            // the given position. This will be used by live reordering.
            _Check_return_ IFACEMETHOD(GetInsertionIndex)(
                _In_ wf::Point position, 
                _Out_ INT* returnValue) 
                override;

            // Gets a series of BOOLEAN values indicating whether a given index is
            // positioned on the leftmost, topmost, rightmost, or bottommost
            // edges of the layout.  This can be useful for both determining whether
            // to tilt items at the edges of rows or columns as well as providing
            // data for portal animations.
            _Check_return_ IFACEMETHOD(IsLayoutBoundary)(
                _In_ INT index, 
                _Out_ BOOLEAN* isLeftBoundary, 
                _Out_ BOOLEAN* isTopBoundary, 
                _Out_ BOOLEAN* isRightBoundary, 
                _Out_ BOOLEAN* isBottomBoundary) 
                override;
          
            // Scroll the given ItemIndex into the view
            _Check_return_ HRESULT ScrollIntoView(
                _In_ UINT index,
                _In_ BOOLEAN isGroupItemIndex,
                _In_ xaml_controls::ScrollIntoViewAlignment alignment)
                override;

            // Computes the total dimension of all realized children
            _Check_return_ HRESULT ComputeTotalRealizedChildrenDimension(
                _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nCount) override;
            
            // a wrapgrid is able to portal (AddDeleteTransition)
            _Check_return_ BOOLEAN IsPortallingSupported() override { return TRUE; }

            _Check_return_ IFACEMETHOD(GetItemsPerPage)(
                _In_ IScrollInfo* pScrollInfo, 
                _Out_ DOUBLE* pItemsPerPage) override;

        private:
            // Returns items per Line
            _Check_return_ HRESULT ComputeLayoutVariables(
                 _In_ XUINT32 childIndex,
                 _In_ wf::Size availableSize,
                 _In_ wf::Size desiredSize);

            // Provides the behavior for the Measure pass of layout. Classes can
            // override this method to define their own Measure pass behavior.
            _Check_return_ HRESULT MeasureChildren(
                // Measurement constraints, a control cannot return a size
                // larger than the constraint.
                _In_ wf::Size availableSize,
                // The desired size of the control.
                _Out_ wf::Size* returnValue);
                        
            // Arrange the Childrens, it is called from Arrange pass of layout.
            _Check_return_ HRESULT  ArrangeChildren(
                // The computed size that is used to arrange the content.
                _In_ wf::Size arrangeSize,
                // starting size in case of alignments
                _In_ wf::Size startingSize,
                // JustificationSize in case of alignment=Stretch
                _In_ wf::Size justificationSize,
                // is Horizontal Orientation
                _In_ bool isHorizontal,
                // arrange offset
                _In_ wf::Point arrangeOffset,
                // The size of the control.
                _Out_ wf::Size* returnValue);

            // Computes Horizontal and vertical alignments, used by Measure and Arrange pass of layout
            _Check_return_ HRESULT ComputeHorizontalAndVerticalAlignment(
                _In_ wf::Size availableSize,
                _Out_ OrientedSize* pStartingSize,
                _Out_ OrientedSize* pJustificationSize);
            
            // Private method used by GetTargetIndexFromNavigationAction
            // Returns index in direction assuming orientation is Vertical
            // In case of Horizontal Orientation, GetTargetIndexFromNavigationAction flips the direction and 
            // calls this method
            _Check_return_ HRESULT GetTargetIndexFromNavigationActionInVerticalOrientation(
                _In_ UINT sourceIndex, 
                _In_ xaml_controls::KeyNavigationAction action,
                _In_ BOOLEAN allowWrap,
                _Out_ UINT* pComputedTargetIndex,
                _Out_ BOOLEAN* pActionValidForSourceIndex);

            // common method to find closestIndex or InsertionIndex
            _Check_return_ HRESULT GetClosestOrInsertionIndex(
                _In_ wf::Point position, 
                _In_ bool isInsertionIndex,
                _Out_ INT* returnValue);

            _Check_return_ HRESULT IsFocusItemBeforeCacheOrAfterCache(
                _Out_ BOOLEAN* isBeforeCache,
                _Out_ BOOLEAN* isAfterCache);

#if PERF_TUNE
//Allows number of cached columns for wrapgrid to be set in registry
            // Read WrapGridCachedLinesCount registry
            _Check_return_ HRESULT GetWrapGridCachedLinesCount(
                _In_ XUINT32 *pValue);
#endif

            // Compute BeforeTrail, AfterTrail, total lines to realize as well as adjust the 
            // visible start when we reach at the end
            _Check_return_ HRESULT ComputeVirtualizationVariables(
                _In_ wf::Size availableSize,
                _In_ DOUBLE firstItemOffset,
                _In_ bool isHorizontal,
                _In_ bool isFirstTime,
                _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle,
                _Out_ XUINT32 *pRealizedLineCount,
                _Out_ INT *pLastViewPort,
                _Out_ wf::Size *pVisibleItemsSize, 
                _Out_ DOUBLE *pLogicalVisibleSpace);

            // compute the visible item count
            _Check_return_ HRESULT ComputeVisibleCount(
                _In_ XUINT32 realizedLineCount);

            _Check_return_ HRESULT ComputeLayoutSlotSize(
                _In_ wf::Size availableSize,
                _In_ bool isHorizontal,
                _In_ bool isScrolling);

            // Check whether the number of lines/columns in current visible space is more than first Measure
            // for example, if in first measure total columns visible on screen are 5 and after some scrolling, 
            // total items visible on screen are 6
            _Check_return_ HRESULT AreCurrentVisibleItemsAreMoreThanFirstMeasureVisibleItems(
                _In_ wf::Size availableSize,
                _In_ DOUBLE firstItemOffset,
                _In_ bool isHorizontal,
                _In_ UINT32 visibleLineInViewport,
                _Out_ bool *pIsMore);

            // This method computes total number of visible columns/lines on screen as well as the LastViewPort on screen
            // It also calculates how many pixels the last item is getting cut from the screen
            _Check_return_ HRESULT ComputeVisibleLinesAndLastViewPort(
                _In_ wf::Size availableSize,
                _In_ DOUBLE firstItemOffset,
                _In_ bool isHorizontal,
                _In_ XUINT32 visibleLineInViewport,
                _Out_ XUINT32 *pVisibleLines,
                _Out_ wf::Size *pVisibleItemsSize, 
                _Out_ INT *pLastViewPort,
                _Out_ DOUBLE *pLogicalVisibleSpace);
    };
}
