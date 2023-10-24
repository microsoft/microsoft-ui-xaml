// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a OrientedVirtualizingPanel.
//      It is a base class for VirtualizingStackPanel and WrapGrid
//      Responsibilities:
//      - Scrolling (Implements IScrollInfo)
//      - Generating containers, clearing them, reclycling themainers which are focused
//        or visible. Also it recuses the containers for better performance
//      - Generates only those cont
//      - Insert, add, remove containers
//      - Implements IOrientedPanel interface for logical and physical Orientation

#pragma once

#include "IManipulationDataProvider.h"
#include "OrientedVirtualizingPanel.g.h"
#include "ScrollData.h"
#include "IScrollOwner.h"

namespace DirectUI
{
    class OffsetMemento;
    struct ScrollVector;

    // Represents a OrientedVirtualizingPanel.
    //
    PARTIAL_CLASS(OrientedVirtualizingPanel),
        public IManipulationDataProvider
    {

        protected:
            OffsetMemento* m_pTranslatedOffsetState;

            // Scrolling and virtualization data.  Only used when this is the scrolling panel (IsScrolling is true).
            // When OVP is in pixel mode _scrollData is in units of pixels.  Otherwise the units are logical.
            ScrollData m_ScrollData;

            // Screen Height is 800 pix. We will use this value for vertical and horizontal (in case when app in in horizontal mode.
            // We don't have reliable way to get this values from RootVisual is always in 800x480 size no matter what orientation is.
            // In case when plugin shows splash screen we got 0x0 as actual size of RootVisual
            DOUBLE m_dPrecacheWindowSize;
            DOUBLE m_dPrecacheBeforeTrailSize;
            DOUBLE m_dPrecacheAfterTrailSize;

            BOOLEAN m_bInMeasure;

            wf::Rect m_arrangedItemsRect;

            // Allow item based scrolling
            BOOLEAN m_bItemBasedScrolling;

            INT m_iVisibleStart;                  // index of of the first visible data item
            INT m_iVisibleCount;                  // count of the number of data items visible in the viewport
            UINT m_nItemsCount;                   // count of te data items during measure of VSP
            INT m_iCacheStart;                    // index of the first data item in the container cache.  This is always <= _visibleStart

            // UIElement collection index of the first visible child container.  This is NOT the data item index. If the first visible container
            // is the 3rd child in the visual tree and contains data item 312, _firstVisibleChildIndex will be 2, while _visibleStart is 312.
            // This is useful because could be several live containers in the collection offscreen (maybe we cleaned up lazily, they couldn't be virtualized, etc).
            // This actually maps directly to realized containers inside the Generator.  It's the index of the first visible realized container.
            // Note that when RecyclingMode is active this is the index into the _realizedChildren collection, not the Children collection.
            INT m_iFirstVisibleChildIndex;

            // Cleanup
            INT m_iBeforeTrail;
            INT m_iAfterTrail;

            // Items per line
            // in case of stack panel, this will be always 1
            // while in case of wrap grid, this will be set by WrapGrid
            // This is basically the number of rows if scroll side is Horizontal
            XUINT32 m_itemsPerLine;

            // Number of lines in grid
            // This is basically the number of columns if scroll side is Horizontal
            // else it will be number of rows
            XUINT32 m_lineCount;

            // Virtualization state
            xaml_controls::VirtualizationMode m_VirtualizationMode;

            BOOLEAN m_bIsVirtualizing;

            // Used by the Recycling mode to maintain the list of actual realized children (a realized child is one that the ItemContainerGenerator has
            // generated).  We need a mapping between children in the UIElementCollection and realized containers in the generator.  In standard virtualization
            // mode these lists are identical; in recycling mode they are not. When a container is recycled the Generator removes it from its realized list, but
            // for perf reasons the panel keeps these containers in its UIElement collection.  This list is the actual realized children -- i.e. the InternalChildren
            // list minus all recycled containers.
            TrackerPtr<wfc::IVector<xaml::UIElement*>> m_tpRealizedChildren;

            BOOLEAN m_bInManipulation;                  // True when a DirectManipulation manip is in progress
            BOOLEAN m_bNotifyLayoutRefresh;                   // True when m_bInManipulation is changed and the next ArrangeOverride needs to call IScrollOwner::NotifyLayoutRefreshed
            BOOLEAN m_bNotifyHorizontalSnapPointsChanges;     // True when NotifySnapPointsChanged needs to be called when horizontal snap points change
            BOOLEAN m_bNotifyVerticalSnapPointsChanges;       // True when NotifySnapPointsChanged needs to be called when vertical snap points change
            BOOLEAN m_bNotifiedHorizontalSnapPointsChanges;   // True when NotifySnapPointsChanged was already called once and horizontal snap points have not been accessed since
            BOOLEAN m_bNotifiedVerticalSnapPointsChanges;     // True when NotifySnapPointsChanged was already called once and vertical snap points have not been accessed since
            BOOLEAN m_bAreSnapPointsKeysHorizontal;           // True when the snap point keys are for horizontal snap points
            FLOAT m_lowerMarginSnapPointKey;                  // Top/left margin dimension used to compute regular and irregular snap points
            FLOAT m_upperMarginSnapPointKey;                  // Bottom/right margin dimension used to compute regular and irregular snap points
            FLOAT m_irregularSnapPointKeysOffset;             // Dimension of the unrealized children ahead of the realized children ones
            FLOAT m_regularSnapPointKey;                      // Unique identifier for regular snap points (independent of snap point alignment)
            INT32 m_cIrregularSnapPointKeys;                  // Number of irregular snap point keys

            // this field will be used later as we finish discovering all scenarios where we need to propagate new zoom factor.
            // IScrollOwner.GetZoomFactor API will go away. And for now we use result from that API with ASSERT comparing with the field.
            FLOAT m_fZoomFactor;                              // Zoom factor from SetZoomFactor

            OrientedVirtualizingPanel();
            ~OrientedVirtualizingPanel() override;

            // Supports the IManipulationDataProvider interface.
            _Check_return_ HRESULT QueryInterfaceImpl(
                _In_ REFIID iid,
                _Outptr_ void** ppObject) override;

        public:
            INT GetFirstCacheIndex()
            {
                return m_nItemsCount == 0 ? -1 : m_iCacheStart;
            }

            INT GetLastCacheIndex()
            {
                return m_nItemsCount == 0 ? -1 : MAX(0, m_iCacheStart + m_iBeforeTrail + m_iVisibleCount - 1);
            }

            // OrientedVirtualizingPanel reacts to this property by changing its child measurement algorithm.
            // If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
            _Check_return_ HRESULT get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_CanVerticallyScrollImpl(_In_ BOOLEAN value);

            // OrientedVirtualizingPanel reacts to this property by changing its child measurement algorithm.
            // If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
            _Check_return_ HRESULT get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_CanHorizontallyScrollImpl(_In_ BOOLEAN value);

            // ExtentWidth contains the horizontal size of the scrolled content element
            _Check_return_ HRESULT get_ExtentWidthImpl(_Out_ DOUBLE* pValue);

            // ExtentHeight contains the vertical size of the scrolled content element
            _Check_return_ HRESULT get_ExtentHeightImpl(_Out_ DOUBLE* pValue);

            // ViewportWidth contains the horizontal size of content's visible range
            _Check_return_ HRESULT get_ViewportWidthImpl(_Out_ DOUBLE* pValue);

            // ViewportHeight contains the vertical size of content's visible range
            _Check_return_ HRESULT get_ViewportHeightImpl(_Out_ DOUBLE* pValue);

            // HorizontalOffset is the horizontal offset of the scrolled content
            _Check_return_ HRESULT get_HorizontalOffsetImpl(_Out_ DOUBLE* pValue);

            // VerticalOffset is the vertical offset of the scrolled content
            _Check_return_ HRESULT get_VerticalOffsetImpl(_Out_ DOUBLE* pValue);

            // MinHorizontalOffset is the minimal horizontal offset of the scrolled content
            _Check_return_ HRESULT get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pValue);

            // MinVerticalOffset is the minimal vertical offset of the scrolled content
            _Check_return_ HRESULT get_MinVerticalOffsetImpl(_Out_ DOUBLE* pValue);

            // ScrollOwner is the container that controls any scrollbars, headers, etc... that are dependant
            // on this IScrollInfo's properties.
            _Check_return_ HRESULT get_ScrollOwnerImpl(_Outptr_ IInspectable** pValue);
            _Check_return_ HRESULT put_ScrollOwnerImpl(_In_opt_ IInspectable* value);

            // Scroll content by one line to the top.
            // Subclasses can override this method and call SetVerticalOffset to change
            // the behavior of what "line" means.
            _Check_return_ HRESULT LineUpImpl();

            // Scroll content by one line to the bottom.
            // Subclasses can override this method and call SetVerticalOffset to change
            // the behavior of what "line" means.
            _Check_return_ HRESULT LineDownImpl();

            // Scroll content by one line to the left.
            // Subclasses can override this method and call SetHorizontalOffset to change
            // the behavior of what "line" means.
            _Check_return_ HRESULT LineLeftImpl();

            // Scroll content by one line to the right.
            // Subclasses can override this method and call SetHorizontalOffset to change
            // the behavior of what "line" means.
            _Check_return_ HRESULT LineRightImpl();

            // Scroll content by one page to the top.
            // Subclasses can override this method and call SetVerticalOffset to change
            // the behavior of what "page" means.
            _Check_return_ HRESULT PageUpImpl();

            // Scroll content by one page to the bottom.
            // Subclasses can override this method and call SetVerticalOffset to change
            // the behavior of what "page" means.
            _Check_return_ HRESULT PageDownImpl();

            // Scroll content by one page to the left.
            // Subclasses can override this method and call SetHorizontalOffset to change
            // the behavior of what "page" means.
            _Check_return_ HRESULT PageLeftImpl();

            // Scroll content by one page to the right.
            // Subclasses can override this method and call SetHorizontalOffset to change
            // the behavior of what "page" means.
            _Check_return_ HRESULT PageRightImpl();

            // Scroll content by one mousewheel click to the top.
            // Subclasses can override this method and call SetVerticalOffset to change
            // the behavior of the mouse wheel increment.
            _Check_return_ HRESULT MouseWheelUpImpl();

            // Scroll content by one mousewheel click to the bottom.
            // Subclasses can override this method and call SetVerticalOffset to change
            // the behavior of the mouse wheel increment.
            _Check_return_ HRESULT MouseWheelDownImpl();

            // Scroll content by one mousewheel click to the left.
            // Subclasses can override this method and call SetHorizontalOffset to change
            // the behavior of the mouse wheel increment.
            _Check_return_ HRESULT MouseWheelLeftImpl();

            // Scroll content by one mousewheel click to the right.
            // Subclasses can override this method and call SetHorizontalOffset to change
            // the behavior of the mouse wheel increment.
            _Check_return_ HRESULT MouseWheelRightImpl();

            // IScrollInfo mouse wheel scroll implementations, based on delta value.
            IFACEMETHOD(MouseWheelUp)(_In_ UINT mouseWheelDelta) override;
            IFACEMETHOD(MouseWheelDown)(_In_ UINT mouseWheelDelta) override;
            IFACEMETHOD(MouseWheelLeft)(_In_ UINT mouseWheelDelta) override;
            IFACEMETHOD(MouseWheelRight)(_In_ UINT mouseWheelDelta) override;

            // Set the HorizontalOffset to the passed value.
            _Check_return_ HRESULT SetHorizontalOffsetImpl(_In_ DOUBLE offset);

            // Set the VerticalOffset to the passed value.
            _Check_return_ HRESULT SetVerticalOffsetImpl(_In_ DOUBLE offset);

            // OrientedVirtualizingPanel implementation of its public MakeVisible method.
            // Does not animate the move by default.
            _Check_return_ HRESULT MakeVisibleImpl(
                // The element that should become visible.
                _In_ xaml::IUIElement* visual,
                // A rectangle representing in the visual's coordinate space to
                // make visible.
                wf::Rect rectangle,
                _Out_ wf::Rect* returnValue);

            // OrientedVirtualizingPanel implementation of IScrollInfo.MakeVisible
            // The goal is to change offsets to bring the child into view,
            // and return a rectangle in our space to make visible.
            // The rectangle we return is in the physical dimension the input target rect
            // transformed into our pace.
            // In the logical dimension, it is our immediate child's rect.
            _Check_return_ HRESULT MakeVisibleImpl(
                // The element that should become visible.
                _In_ xaml::IUIElement* visual,
                // A rectangle representing in the visual's coordinate space to
                // make visible.
                wf::Rect rectangle,
                // When set to True, the DManip ZoomToRect method is invoked.
                BOOLEAN useAnimation,
                DOUBLE horizontalAlignmentRatio,
                DOUBLE verticalAlignmentRatio,
                DOUBLE offsetX,
                DOUBLE offsetY,
                _Out_ wf::Rect* resultRectangle,
                _Out_opt_ DOUBLE* appliedOffsetX = nullptr,
                _Out_opt_ DOUBLE* appliedOffsetY = nullptr);

            // IScrollSnapPointsInfo interface implementation:

            // Returns True when the horizontal snap points are equidistant.
            _Check_return_ HRESULT get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue);

            // Returns True when the vertical snap points are equidistant.
            _Check_return_ HRESULT get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue);

            // Returns a read-only collection of numbers representing the snap points for
            // the provided orientation. Returns an empty collection when no snap points are present.
            _Check_return_ HRESULT GetIrregularSnapPointsImpl(
                // The direction of the requested snap points.
                _In_ xaml_controls::Orientation orientation,
                // The alignment used by the caller when applying the requested snap points.
                _In_ xaml_primitives::SnapPointsAlignment alignment,
                // The read-only collection of snap points.
                _Outptr_ wfc::IVectorView<FLOAT>** pValue);

            // Returns an original offset and interval for equidistant snap points for
            // the provided orientation. Returns 0 when no snap points are present.
            _Check_return_ HRESULT GetRegularSnapPointsImpl(
                // The direction of the requested snap points.
                _In_ xaml_controls::Orientation orientation,
                // The alignment used by the caller when applying the requested snap points.
                _In_ xaml_primitives::SnapPointsAlignment alignment,
                // The offset of the first snap point.
                _Out_ FLOAT* pOffset,
                // The interval between the regular snap points.
                _Out_ FLOAT* pInterval);

            // Events.
            IFACEMETHOD(add_HorizontalSnapPointsChanged)(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
            IFACEMETHOD(remove_HorizontalSnapPointsChanged)(_In_ EventRegistrationToken tToken) override;
            IFACEMETHOD(add_VerticalSnapPointsChanged)(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
            IFACEMETHOD(remove_VerticalSnapPointsChanged)(_In_ EventRegistrationToken tToken) override;
            // End of IScrollSnapPointsInfo interface implementation

            // Logical Orientation override
            IFACEMETHOD(get_LogicalOrientation)(
                _Out_ xaml_controls::Orientation* pValue)
                override;

            // Physical Orientation override
            IFACEMETHOD(get_PhysicalOrientation)(
                _Out_ xaml_controls::Orientation* pValue)
                override;

            // Get the closest element information to the point.
            IFACEMETHOD(GetClosestElementInfo)(
                _In_ wf::Point position,
                _Out_ xaml_primitives::ElementInfo* returnValue)
                override;

            // Get the index where an item should be inserted if it were dropped at
            // the given position. This will be used by live reordering.
            IFACEMETHOD(GetInsertionIndex)(
                _In_ wf::Point position,
                _Out_ INT* returnValue)
                override;

            // Gets a series of BOOLEAN values indicating whether a given index is
            // positioned on the leftmost, topmost, rightmost, or bottommost
            // edges of the layout.  This can be useful for both determining whether
            // to tilt items at the edges of rows or columns as well as providing
            // data for portal animations.
            IFACEMETHOD(IsLayoutBoundary)(
                _In_ INT index,
                _Out_ BOOLEAN* isLeftBoundary,
                _Out_ BOOLEAN* isTopBoundary,
                _Out_ BOOLEAN* isRightBoundary,
                _Out_ BOOLEAN* isBottomBoundary)
                override;

            IFACEMETHOD(GetItemsBounds)(
                _Out_ wf::Rect* returnValue)
                override;

            // IInsertionPanel
            // Get the indexes where an item should be inserted if it were dropped at
            // the given position
            _Check_return_ HRESULT GetInsertionIndexesImpl(
                _In_ wf::Point position,
                _Out_ INT* pFirst,
                _Out_ INT* pSecond);

            // IPaginatedPanel
            IFACEMETHOD(GetLastItemIndexInViewport)(
                _In_ IScrollInfo* pScrollInfo,
                _Inout_ INT* pResult) override;
            IFACEMETHOD(GetItemsPerPage)(
                _In_ IScrollInfo* pScrollInfo,
                _Out_ DOUBLE* pItemsPerPage) override;

            // IManipulationDataProvider methods accessed by scrolling owner to support DirectManipulation.
            // Called when a manipulation is starting or ending.
            _Check_return_ HRESULT UpdateInManipulation(
                _In_ BOOLEAN isInManipulation,
                _In_ BOOLEAN isInLiveTree,
                _In_ DOUBLE nonVirtualizingOffset) override;

            // Updates the zoom factor
            _Check_return_ HRESULT SetZoomFactor(
                _In_ FLOAT newZoomFactor) override;

            // Gets the scrolling extent in pixels even for logical scrolling scenarios.
            _Check_return_ HRESULT ComputePixelExtent(
                _In_ bool ignoreZoomFactor,
                _Out_ DOUBLE& extent) override;

            // Gets the offset in pixels even for logical scrolling scenarios.
            _Check_return_ HRESULT ComputePixelOffset(
                _In_ BOOLEAN isForHorizontalOrientation,
                _Out_ DOUBLE& offset) override;

            // Gets the logical offset given a pixel delta.
            _Check_return_ HRESULT ComputeLogicalOffset(
                _In_ BOOLEAN isForHorizontalOrientation,
                _Inout_ DOUBLE& pixelDelta,
                _Out_ DOUBLE& logicalOffset) override;

            _Check_return_ HRESULT GetSizeOfFirstVisibleChild(
                _Out_ wf::Size& size) override;

            virtual _Check_return_ HRESULT AreScrollSnapPointsRegular(
                _Out_ BOOLEAN* pAreScrollSnapPointsRegular);


            // Returns the list of childen that have been realized by the Generator.
            // We must use this method whenever we interact with the Generator's index.
            // In FindFocusedChildInRealizedChildren mode the Children collection also contains recycled containers and thus does
            // not map to the Generator's list.
            _Check_return_ HRESULT get_RealizedChildren(
                _Outptr_ wfc::IVector<xaml::UIElement*>** ppRealizedChildren);

            // Computes the estimated dimension of the unrealized children ahead of the realized ones.
            virtual _Check_return_ HRESULT ComputeUnrealizedChildrenEstimatedDimension(
                _Out_ FLOAT& dimension);

        protected:
            virtual _Check_return_ HRESULT OnCleanUpVirtualizedItemProtected(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e);

            // Called when the Items collection associated with the containing ItemsControl changes.
            IFACEMETHOD(OnItemsChanged)(
                _In_ IInspectable* sender,
                _In_ xaml_primitives::IItemsChangedEventArgs* args) override;

            // Called when the UI collection of children is cleared by the base Panel class.
            IFACEMETHOD(OnClearChildren)() override;

            _Check_return_ HRESULT MeasureChild(
                _In_ xaml::IUIElement* pChild,
                _In_ wf::Size layoutSlotSize,
                _Out_opt_ wf::Size* returnValue);

            _Check_return_ HRESULT ResetScrolling();

            _Check_return_ HRESULT get_IsScrolling(_Out_ BOOLEAN* pbIsScrolling);

            // Gets the offset in pixels even for logical scrolling scenarios.
            // When bUseProvidedLogicalOffset Offset is True, the logical offset provided is used.
            _Check_return_ HRESULT ComputePixelOffset(
                _In_ BOOLEAN isForHorizontalOrientation,
                _In_ BOOLEAN bUseProvidedLogicalOffset,
                _In_ DOUBLE logicalOffset,
                _Out_ DOUBLE& offset);

            // Returns the extent in logical units in the stacking direction.
            _Check_return_ HRESULT ComputeLogicalExtent(
                _In_ wf::Size stackDesiredSize,
                _In_ BOOLEAN isHorizontal,
                _Out_ wf::Size& logicalExtent);

            // Calculate the scroll Offset for arrange
            _Check_return_ HRESULT ComputeScrollOffsetForArrange(
                _Out_ wf::Point* pOffset);

            // Called when we ran out of children before filling up the viewport.
            _Check_return_ HRESULT GeneratePreviousItems(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _Inout_ DOUBLE& logicalVisibleSpace,
                _Inout_ wf::Size& stackDesiredSize,
                _In_ wf::Size layoutSlotSize,
                _In_ BOOLEAN isHorizontal,
                _In_ BOOLEAN adjustPositions,
                _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle);

            // Called when we ran out of children before filling up the viewport.
            _Check_return_ HRESULT FillRemainingSpace(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _Inout_ DOUBLE& logicalVisibleSpace,
                _Inout_ wf::Size& stackDesiredSize,
                _In_ wf::Size layoutSlotSize,
                _In_ BOOLEAN isHorizontal,
                _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle);

            // Updates ScrollData's offset, extent, and viewport in logical units.
            _Check_return_ HRESULT UpdateLogicalScrollData(
                _Inout_ wf::Size& stackDesiredSize,
                _In_ wf::Size constraint,
                _In_ DOUBLE logicalVisibleSpace,
                _In_ wf::Size extent,
                _In_ INT lastViewport,
                _In_ BOOLEAN isHorizontal);

            // Returns the index of the first item visible (even partially) in the viewport.
            INT ComputeIndexOfFirstVisibleItem(
                _In_ BOOLEAN isHorizontal,
                _Out_ DOUBLE& firstItemOffset);

            // Takes a container returned from Generator.GenerateNext() and places it in the visual tree if necessary.
            // Takes into account whether the container is new, recycled, or already realized.
            _Check_return_ HRESULT AddContainerFromGenerator(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _In_ BOOLEAN newlyRealized,
                _Inout_ BOOLEAN& visualOrderChanged);

            _Check_return_ HRESULT GeneratePreviousChild(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT childIndex,
                _In_ wf::Size layoutSlotSize,
                _Outptr_ xaml::IUIElement** ppChild);

            void AdjustCacheWindow();

            BOOLEAN IsOutsideCacheWindow(_In_ INT itemIndex);

            _Check_return_ HRESULT IsInsideViewWindowLocationBased(
                _In_ IUIElement* pContainer,
                _In_ wf::Size constraint,
                _Out_ BOOLEAN& result);

            BOOLEAN IsInsideViewWindowIndexBased(
                _In_ INT containerIndex);

            // Determine whether the item on given Index will be the first Item in Line
            bool IsFirstItemInLine(_In_ INT itemIndex);

            // returns the Visible start item Index
            INT VisibleStartItemIndex();

            // Immediately cleans up any containers that have gone offscreen.  Called by MeasureOverride.
            // When recycling this runs before generating and measuring children; otherwise it runs after.
            virtual _Check_return_ HRESULT CleanupContainers(
                _In_ xaml_controls::IItemsControl* pItemsControl,
                _In_ wf::Size constraint);

#if DBG
            // Debug method that ensures the _realizedChildren list matches the realized containers in the Generator.
            _Check_return_ HRESULT debug_VerifyRealizedChildren();
            _Check_return_ HRESULT debug_AssertRealizedChildrenEqualVisualChildren();
            _Check_return_ HRESULT debug_DumpRealizedChildren();
            _Check_return_ HRESULT debug_DumpVisualChildren();
#endif
            // Takes an index from the realized list and returns the corresponding index in the Children collection
            _Check_return_ HRESULT ChildIndexFromRealizedIndex(
                _In_ INT realizedChildIndex,
                _Out_ INT& childIndex);

            // Recycled containers still in the Children collection at the end of Measure should be moved to the end of Children collection
            _Check_return_ HRESULT CollectRecycledContainers();

            _Check_return_ HRESULT IndexToGeneratorPositionForStart(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT index,
                _Out_ INT& childIndex,
                _Out_ xaml_primitives::GeneratorPosition& position);

            _Check_return_ HRESULT NotifyCleanupItem(
                _In_ IInspectable* pItem,
                _In_ xaml::IUIElement* pChild,
                _In_ xaml_controls::IItemsControl* pItemsControl,
                _Out_ BOOLEAN& bCanceled);

            // OnScrollChange is an override called whenever the IScrollInfo exposed scrolling state changes on this element.
            // At the time this method is called, scrolling state is in its new, valid state.
            _Check_return_ HRESULT OnScrollChange();

            _Check_return_ HRESULT ComputePhysicalFromLogicalOffset(
                _In_ INT logicalOffset,
                _In_ DOUBLE fractionalItemOffset,
                _In_ BOOLEAN isHorizontal,
                _Out_ DOUBLE& physicalOffset);

            _Check_return_ HRESULT GetGeneratedIndex(
                _In_ INT childIndex,
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _Out_ INT& generatedIndex);

            _Check_return_ HRESULT FindFocusedChildInRealizedChildren(
                _Out_ INT& focusedChild);

            BOOLEAN InRecyclingMode()
            {
                return m_VirtualizationMode == xaml_controls::VirtualizationMode_Recycling;
            }

            virtual _Check_return_ HRESULT TranslateHorizontalPixelDeltaToOffset(
                _Inout_ DOUBLE& delta,
                _Out_ DOUBLE& value);

            virtual _Check_return_ HRESULT TranslateVerticalPixelDeltaToOffset(
                _Inout_ DOUBLE& delta,
                _Out_ DOUBLE& value);

            // Returns the desired Size of a Container
            // if the Items are fixed Size, it uses the size from GetItemSize() method
            virtual _Check_return_ HRESULT GetDesiredSize(
                _In_ IUIElement* pChild,
                _Out_ wf::Size* pDesiredSize);

            _Check_return_ HRESULT CleanupRange(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT startIndex,
                _In_ INT count);

    private:
            // Check whether we should defer our scroll wheel handling to scroll owner
            _Check_return_ HRESULT get_ShouldPassWheelMessageToScrollOwner(_Out_ BOOLEAN &shouldPass);

            // Allow our scroll owner to handle the last mouse wheel message.
            _Check_return_ HRESULT PassWheelMessageToScrollOwner(_In_ ZoomDirection zoomDirection);

            // Inserts a new container in the visual tree
            _Check_return_ HRESULT InsertNewContainer(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _Inout_ BOOLEAN& visualOrderChanged);

            /// Inserts a recycled container in the visual tree
            _Check_return_ HRESULT InsertRecycledContainer(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _Inout_ BOOLEAN& visualOrderChanged);

            // Inserts a container into the Children collection.  The container is either new or recycled.
            _Check_return_ HRESULT InsertContainer(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _In_ BOOLEAN isRecycled,
                _Inout_ BOOLEAN& visualOrderChanged);

            _Check_return_ HRESULT PreviousChildIsGenerated(
                _In_opt_ xaml_controls::IItemsControl* pItemsControlHint,
                _In_ INT childIndex,
                _Out_ BOOLEAN& isGenerated);

            void AdjustFirstVisibleChildIndex(_In_ INT startIndex, _In_ INT count);

            _Check_return_ HRESULT EnsureRealizedChildren();

            _Check_return_ HRESULT OnItemsAdd(
                _In_ xaml_primitives::IItemsChangedEventArgs* args);

            _Check_return_ HRESULT OnItemsRemove(
                _In_ xaml_primitives::IItemsChangedEventArgs* args);

            _Check_return_ HRESULT OnItemsReplace(
                _In_ xaml_primitives::IItemsChangedEventArgs* args);

            _Check_return_ HRESULT RemoveChildRange(
                _In_ xaml_primitives::GeneratorPosition position,
                _In_ INT itemCount,
                _In_ INT itemUICount);

    protected:

            // Since we don't disconnect/distroy the containers, there may be extra visual child/container
            // where there is no associated items. These containers are arranged outside view
            _Check_return_ HRESULT ArrangeExtraContainers(
                // is Horizontal Orientation
                _In_ bool isHorizontal);

            _Check_return_ HRESULT SetAndVerifyScrollingData(
                _In_ wf::Size viewport,
                _In_ wf::Size extent,
                _In_ ScrollVector offset);

            // Returns a read-only collection of numbers representing the snap points for
            // the provided orientation. Returns an empty collection when no snap points are present.
            virtual _Check_return_ HRESULT GetIrregularSnapPointsInternal(
                // The direction of the requested snap points.
                _In_ xaml_controls::Orientation orientation,
                // The alignment used by the caller when applying the requested snap points.
                _In_ xaml_primitives::SnapPointsAlignment alignment,
                // The read-only collection of snap points.
                _Outptr_ wfc::IVectorView<FLOAT>** pValue);

            // Computes the total dimension of all realized children
            virtual _Check_return_ HRESULT ComputeTotalRealizedChildrenDimension(
                _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nCount);

            // Called when the AreScrollSnapPointsRegular property changed
            virtual _Check_return_ HRESULT OnAreScrollSnapPointsRegularChanged();

            virtual _Check_return_ HRESULT NotifySnapPointsChanges(
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount);

            virtual _Check_return_ HRESULT ResetSnapPointKeys();

            // Determines whether the StackPanel must call NotifySnapPointsChanged
            // when snap points change or not.
            virtual _Check_return_ HRESULT SetSnapPointsChangeNotificationsRequirement(
                _In_ BOOLEAN isForHorizontalSnapPoints,
                _In_ BOOLEAN notifyChanges);

            _Check_return_ HRESULT NotifySnapPointChanges(_In_ BOOLEAN isForHorizontalSnapPoints);

            _Check_return_ HRESULT RefreshRegularSnapPointKeys();

            _Check_return_ HRESULT GetRegularSnapPointKeys(
                _In_ xaml_controls::Orientation orientation,
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount,
                _Out_ FLOAT* pSnapPointKey,
                _Out_ FLOAT* pLowerMarginSnapPointKey,
                _Out_ FLOAT* pUpperMarginSnapPointKey);

            // Determines the common keys for regular and irregular snap points.
            // Those keys are the left/right margins for a horizontal panel,
            // or the top/bottom margins for a vertical panel.
            _Check_return_ HRESULT GetCommonSnapPointKeys(
                _Out_ FLOAT* pLowerMarginSnapPointKey,
                _Out_ FLOAT* pUpperMarginSnapPointKey);

            virtual _Check_return_ HRESULT MeasureChildForItemsChanged(
                _In_ xaml::IUIElement* pChild)
            {
                RRETURN(S_OK);
            }

            _Check_return_ HRESULT IsHorizontal(
                _Out_ BOOLEAN& bIsHorizontal);

            _Check_return_ HRESULT GetZoomFactor(
                _Out_ FLOAT* zoomFactor);

            _Check_return_ HRESULT IsInDirectManipulationZoom(
                _Out_ BOOLEAN& bIsInDirectManipulationZoom);

            _Check_return_ HRESULT RaiseVirtualizedCollectionUpdatedEvent(
                _In_ wf::Rect contentBounds);

    private:
            // Called when horizontal snap points changed
            _Check_return_ HRESULT OnHorizontalSnapPointsChanged();

            // Called when vertical snap points changed
            _Check_return_ HRESULT OnVerticalSnapPointsChanged();

            // Updates the non-virtualized offset irrespective of the extent, before the coming MeasureOverride
            // execution updates the extent based on the new zoom factor.
            _Check_return_ HRESULT SetNonVirtualizingOffset(
                _In_ DOUBLE offset);
    };
}
