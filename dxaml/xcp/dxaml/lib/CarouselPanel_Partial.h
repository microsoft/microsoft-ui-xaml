// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IManipulationDataProvider.h"
#include "CarouselPanel.g.h"
#include "ScrollData.h"

namespace DirectUI
{
    // Represents a CarouselPanel.
    //
    PARTIAL_CLASS(CarouselPanel),
        public IManipulationDataProvider
    {
        private:

            // Scrolling and virtualization data.  Only used when this is the scrolling panel (IsScrolling is true).
            // When VSP is in pixel mode _scrollData is in units of pixels.  Otherwise the units are logical.
            ScrollData m_ScrollData;

            // Virtualization state
            xaml_controls::VirtualizationMode m_VirtualizationMode;
            INT m_iVisibleStart;                 // index of of the first visible data item
            INT m_iVisibleCount;                 // count of the number of data items visible in the viewport
            INT m_nItemsCount;                   // count of the data items during measure of CarouselPanel

            // UIElement collection index of the first visible child container.  This is NOT the data item index. If the first visible container
            // is the 3rd child in the visual tree and contains data item 312, _firstVisibleChildIndex will be 2, while _visibleStart is 312.
            // This is useful because could be several live containers in the collection offscreen (maybe we cleaned up lazily, they couldn't be virtualized, etc).
            // This actually maps directly to realized containers inside the Generator.  It's the index of the first visible realized container.
            // Note that when RecyclingMode is active this is the index into the _realizedChildren collection, not the Children collection.
            INT m_iFirstVisibleChildIndex;

            // Used by the Recycling mode to maintain the list of actual realized children (a realized child is one that the ItemContainerGenerator has
            // generated).  We need a mapping between children in the UIElementCollection and realized containers in the generator.  In standard virtualization
            // mode these lists are identical; in recycling mode they are not. When a container is recycled the Generator removes it from its realized list, but
            // for perf reasons the panel keeps these containers in its UIElement collection.  This list is the actual realized children -- i.e. the InternalChildren
            // list minus all recycled containers.
            TrackerPtr<DirectUI::TrackerCollection<xaml::UIElement*>> m_tpRealizedChildren;

            // Cleanup
            INT m_iBeforeTrail;
            INT m_iAfterTrail;

            // Screen Height is 800 pix. We will use this value for vertical and horizontal (in case when app in in horizontal mode.
            // We don't have reliable way to get this values from RootVisual is always in 800x480 size no matter what orientation is.
            // In case when plugin shows splash screen we got 0x0 as actual size of RootVisual.
            // Note that the below values are in px. We'll realize as many items as we can fit.
            DOUBLE m_dPrecacheWindowSize; // Governs the virtualization window in which we realize items that are not in the popup.
            DOUBLE m_dPrecacheBeforeTrailSize;
            DOUBLE m_dPrecacheAfterTrailSize;
            double m_minimumDesiredWindowWidth;

            // Allow item based scrolling
            BOOLEAN m_bItemBasedScrolling;

            //private BOOLEAN DisableRenderScrolling = false;

            //private Storyboard _horizontalFlickSB;
            //private Storyboard _verticalFlickSB;

            OffsetMemento* m_pTranslatedOffsetState;
            BOOLEAN m_bIsVirtualizing;
            BOOLEAN m_bInMeasure;

            BOOLEAN m_bNotifyHorizontalSnapPointsChanges;     // True when NotifySnapPointsChanged needs to be called when horizontal snap points change
            BOOLEAN m_bNotifyVerticalSnapPointsChanges;       // True when NotifySnapPointsChanged needs to be called when vertical snap points change
            BOOLEAN m_bNotifiedHorizontalSnapPointsChanges;   // True when NotifySnapPointsChanged was already called once and horizontal snap points have not been accessed since
            BOOLEAN m_bNotifiedVerticalSnapPointsChanges;     // True when NotifySnapPointsChanged was already called once and vertical snap points have not been accessed since
            BOOLEAN m_bAreSnapPointsKeysHorizontal;           // True when the snap point keys are for horizontal snap points
            FLOAT m_lowerMarginSnapPointKey;                  // Top/left margin dimension used to compute regular and irregular snap points
            //Use once horizontal carousel is enabled
            //FLOAT m_upperMarginSnapPointKey;                // Bottom/right margin dimension used to compute regular and irregular snap points
            FLOAT m_irregularSnapPointKeysOffset;             // Dimension of the unrealized children ahead of the realized children ones
            FLOAT m_regularSnapPointKey;                      // Unique identifier for regular snap points (independent of snap point alignment)
            FLOAT* m_pIrregularSnapPointKeys;                 // Unique identifiers for irregular snap points (independent of snap point alignment)
            INT32 m_cIrregularSnapPointKeys;                  // Number of irregular snap point keys

            // this field will be used later as we finish discovering all scenarios where we need to propagate new zoom factor.
            // IScrollOwner.GetZoomFactor API will go away. And for now we use result from that API with ASSERT comparing with the field.
            FLOAT m_fZoomFactor;                              // Zoom factor from SetZoomFactor

            // this field is updated during measure and in sync with ComboBox's m_bShouldCarousel
            BOOLEAN m_bShouldCarousel;

        protected:
            CarouselPanel();
            ~CarouselPanel() override;

            // Supports the IManipulationDataProvider interface.
            _Check_return_ HRESULT QueryInterfaceImpl(
                _In_ REFIID iid,
                _Outptr_ void** ppObject) override;

            // Provides the behavior for the Measure pass of layout. Classes can
            // override this method to define their own Measure pass behavior.
            IFACEMETHOD(MeasureOverride)(
                // Measurement constraints, a control cannot return a size
                // larger than the constraint.
                _In_ wf::Size availableSize,
                // The desired size of the control.
                _Out_ wf::Size* returnValue) override;

            // Provides the behavior for the Arrange pass of layout.  Classes
            // can override this method to define their own Arrange pass
            // behavior.
            IFACEMETHOD(ArrangeOverride)(
                // The computed size that is used to arrange the content.
                _In_ wf::Size arrangeSize,
                // The size of the control.
                _Out_ wf::Size* returnValue) override;

            // Called when the Items collection associated with the containing ItemsControl changes.
            IFACEMETHOD(OnItemsChanged)(
                _In_ IInspectable* sender,
                _In_ xaml_primitives::IItemsChangedEventArgs* args) override;

            // Called when the UI collection of children is cleared by the base Panel class.
            IFACEMETHOD(OnClearChildren)() override;

            _Check_return_ HRESULT GetZoomFactor(
                _Out_ FLOAT* zoomFactor);

            _Check_return_ HRESULT IsInDirectManipulationZoom(
                _Out_ BOOLEAN& bIsInDirectManipulationZoom);

        public:
            // IScrollInfo interface implementation:

            // CarouselPanel reacts to this property by changing its child measurement algorithm.
            // If scrolling in a dimension, infinite space is allowed the child; otherwise, available size is preserved.
            _Check_return_ HRESULT get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_CanVerticallyScrollImpl(_In_ BOOLEAN value);

            // CarouselPanel reacts to this property by changing its child measurement algorithm.
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

            // CarouselPanel implementation of its public MakeVisible method.
            // Does not animate the move by default.
            _Check_return_ HRESULT MakeVisibleImpl(
                // The element that should become visible.
                _In_ xaml::IUIElement* visual,
                // A rectangle representing in the visual's coordinate space to
                // make visible.
                wf::Rect rectangle,
                _Out_ wf::Rect* resultRectangle);

            // CarouselPanel implementation of IScrollInfo.MakeVisible
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

            // Virtual methods.
            IFACEMETHOD(OnCleanUpVirtualizedItem)(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e); //TODO Cleanup for CSP override;

            // Protected methods.
            virtual _Check_return_ HRESULT OnCleanUpVirtualizedItemProtected(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e);//TODO Cleanup for CSP override;

            _Check_return_ HRESULT GetEstimatedRealizedChildSize(
                _Out_ wf::Size& childSize);

            _Check_return_ HRESULT GetRealizedFirstChild(
                _Outptr_ xaml::IUIElement**  ppRealizedFirstChild);

            void ResetOffsetLoop();
            void ResetMinimumDesiredWindowWidth();

            DOUBLE GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanel();

            _Check_return_ HRESULT ScrollIntoView(
                _In_ UINT index,
                _In_ BOOLEAN isGroupItemIndex,
                _In_ xaml_controls::ScrollIntoViewAlignment alignment)
                override;

            // ScrollViewer uses it to determinate the fact that the CarouselPanel is in carousel mode.
            BOOLEAN GetShouldCarousel()
            {
                return m_bShouldCarousel;
            }

        private:
            // Returns the extent in logical units in the stacking direction.
            _Check_return_ HRESULT ComputeLogicalExtent(
                _In_ wf::Size stackDesiredSize,
                _In_ BOOLEAN isHorizontal,
                _Out_ wf::Size& logicalExtent);

            // Updates ScrollData's offset, extent, and viewport in logical units.
            _Check_return_ HRESULT UpdateLogicalScrollData(
                _Inout_ wf::Size& stackDesiredSize,
                _In_ wf::Size constraint,
                _In_ DOUBLE logicalVisibleSpace,
                _In_ wf::Size extent,
                _In_ INT lastViewport);

            // Returns the index of the first item visible (even partially) in the viewport.
            INT ComputeIndexOfFirstVisibleItem(
                _Out_ DOUBLE& firstItemOffset);

            // Inserts a new container in the visual tree
            _Check_return_ HRESULT InsertNewContainer(
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _Inout_ BOOLEAN& visualOrderChanged);

            /// Inserts a recycled container in the visual tree
            _Check_return_ HRESULT InsertRecycledContainer(
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _Inout_ BOOLEAN& visualOrderChanged);

            // Inserts a container into the Children collection.  The container is either new or recycled.
            _Check_return_ HRESULT InsertContainer(
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _In_ BOOLEAN isRecycled,
                _Inout_ BOOLEAN& visualOrderChanged);

            // Takes a container returned from Generator.GenerateNext() and places it in the visual tree if necessary.
            // Takes into account whether the container is new, recycled, or already realized.
            _Check_return_ HRESULT AddContainerFromGenerator(
                _In_ INT childIndex,
                _In_ xaml::IUIElement* pChild,
                _In_ BOOLEAN newlyRealized,
                _Inout_ BOOLEAN& visualOrderChanged);

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

            // Immediately cleans up any containers that have gone offscreen.  Called by MeasureOverride.
            // When recycling this runs before generating and measuring children; otherwise it runs after.
            _Check_return_ HRESULT CleanupContainers(
                _In_ xaml_controls::IItemsControl* pItemsControl);

            _Check_return_ HRESULT EnsureRealizedChildren();

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

            // Recycled containers still in the Children collection at the end of Measure should be disconnected
            // from the visual tree.  Otherwise they're still visible to things like Arrange, keyboard navigation, etc.
            _Check_return_ HRESULT DisconnectRecycledContainers();

            _Check_return_ HRESULT IndexToGeneratorPositionForStart(
                _In_ INT index,
                _Out_ INT& childIndex,
                _Out_ xaml_primitives::GeneratorPosition& position);

            _Check_return_ HRESULT NotifyCleanupItem(
                _In_ IInspectable* pItem,
                _In_ xaml::IUIElement* pChild,
                _In_ xaml_controls::IItemsControl* pItemsControl,
                _Out_ BOOLEAN& bCanceled);

            _Check_return_ HRESULT CleanupRange(
                _In_ INT startIndex,
                _In_ INT count);

            void AdjustFirstVisibleChildIndex(_In_ INT startIndex, _In_ INT count);

            // Sets up IsVirtualizing, VirtualizationMode
            //
            // IsVirtualizing is true if turned on via the items control and if the panel has a viewport.
            // VSP has a viewport if it's either the scrolling panel or it was given MeasureData.
            _Check_return_ HRESULT SetVirtualizationState(
                _In_ xaml_controls::IItemsControl* pItemsControl);

            _Check_return_ HRESULT MeasureChild(
                _In_ xaml::IUIElement* pChild,
                _In_ wf::Size layoutSlotSize,
                _Out_opt_ wf::Size* returnValue);

            _Check_return_ HRESULT ResetScrolling();

            // OnScrollChange is an override called whenever the IScrollInfo exposed scrolling state changes on this element.
            // At the time this method is called, scrolling state is in its new, valid state.
            _Check_return_ HRESULT OnScrollChange();

            _Check_return_ HRESULT SetAndVerifyScrollingData(
                _In_ wf::Size viewport,
                _In_ wf::Size extent,
                _In_ ScrollVector offset);

            // Computes the total dimension of all realized children
            _Check_return_ HRESULT ComputeTotalRealizedChildDimension(
                _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nCount);

            // Computes the estimated dimension of the unrealized children ahead of the realized ones.
            _Check_return_ HRESULT ComputeUnrealizedChildrenEstimatedDimension(
                _Out_ FLOAT& dimension);

            // Computes the estimated dimension of the unrealized and realized children.
            _Check_return_ HRESULT ComputeChildrenEstimatedDimension(
                _Out_ FLOAT& dimension);

            _Check_return_ HRESULT ComputePhysicalFromLogicalOffset(
                _In_ INT logicalOffset,
                _In_ DOUBLE fractionalItemOffset,
                _In_ BOOLEAN isHorizontal,
                _Out_ DOUBLE& physicalOffset);

            _Check_return_ HRESULT GetGeneratedIndex(
                _In_ INT childIndex,
                _Out_ INT& generatedIndex);

            _Check_return_ HRESULT FindFocusedChildInRealizedChildren(
                _Out_ INT& focusedChild,
                _Out_ INT& previousFocusable,
                _Out_ INT& nextFocusable);

            _Check_return_ HRESULT TranslateHorizontalPixelDeltaToOffset(
                _In_ DOUBLE delta,
                _Out_ DOUBLE& value);

            _Check_return_ HRESULT TranslateVerticalPixelDeltaToOffset(
                _In_ DOUBLE delta,
                _Out_ DOUBLE& value);

            BOOLEAN InRecyclingMode()
            {
                return m_VirtualizationMode == xaml_controls::VirtualizationMode_Recycling;
            }

            _Check_return_ HRESULT get_IsScrolling(_Out_ BOOLEAN* pbIsScrolling);

            _Check_return_ HRESULT put_IsVirtualizing(_In_ BOOLEAN isVirtualizing);

            // Returns the list of childen that have been realized by the Generator.
            // We must use this method whenever we interact with the Generator's index.
            // In recycling mode the Children collection also contains recycled containers and thus does
            // not map to the Generator's list.
            _Check_return_ HRESULT get_RealizedChildren(
                _Outptr_ wfc::IVector<xaml::UIElement*>** ppRealizedChildren);

            // Called when the AreScrollSnapPointsRegular property changed
            _Check_return_ HRESULT OnAreScrollSnapPointsRegularChanged();

            // Called when horizontal snap points changed
            _Check_return_ HRESULT OnHorizontalSnapPointsChanged();

            // Called when vertical snap points changed
            _Check_return_ HRESULT OnVerticalSnapPointsChanged();

            _Check_return_ HRESULT GetIrregularSnapPoints(
                _In_ BOOLEAN isForHorizontalSnapPoints,  // True when horizontal snap points are requested.
                _In_ BOOLEAN isForLeftAlignment,         // True when requested snap points will align to the left/top of the children
                _In_ BOOLEAN isForRightAlignment,        // True when requested snap points will align to the right/bottom of the children
                _Outptr_opt_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints,    // Placeholder for returned array
                _Out_ UINT32* pcSnapPoints);                                   // Number of snap points returned

            _Check_return_ HRESULT GetIrregularSnapPointKeys(
                _In_ xaml_controls::Orientation orientation,
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount,
                _Outptr_result_buffer_(*pcSnapPointKeys) FLOAT** ppSnapPointKeys,
                _Out_ INT32* pcSnapPointKeys,
                _Out_ FLOAT* pSnapPointKeysOffset,
                _Out_ FLOAT* pLowerMarginSnapPointKey);
                //_Out_ FLOAT* pUpperMarginSnapPointKey); Use once horizontal carousel is enabled

            _Check_return_ HRESULT GetRegularSnapPointKeys(
                _In_ xaml_controls::Orientation orientation,
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount,
                _Out_ FLOAT* pSnapPointKey,
                _Out_ FLOAT* pLowerMarginSnapPointKey);
                //_Out_ FLOAT* pUpperMarginSnapPointKey); Use once horizontal carousel is enabled

            _Check_return_ HRESULT GetCommonSnapPointKeys(
                _Out_ FLOAT* pLowerMarginSnapPointKey);
                //_Out_ FLOAT* pUpperMarginSnapPointKey); Use once horizontal carousel is enabled

            _Check_return_ HRESULT NotifySnapPointsChanges(
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount);

            _Check_return_ HRESULT NotifySnapPointsChanges(_In_ BOOLEAN isForHorizontalSnapPoints);

            _Check_return_ HRESULT RefreshIrregularSnapPointKeys();

            // Refreshes the m_regularSnapPointKey field based on a single child.
            // Refreshes also the m_lowerMarginSnapPointKey/m_upperMarginSnapPointKey fields based
            // on the current margins.
            _Check_return_ HRESULT RefreshRegularSnapPointKeys();

            _Check_return_ HRESULT ResetSnapPointKeys();

            // Determines whether the StackPanel must call NotifySnapPointsChanged
            // when snap points change or not.
            _Check_return_ HRESULT SetSnapPointsChangeNotificationsRequirement(
                _In_ BOOLEAN isForHorizontalSnapPoints,
                _In_ BOOLEAN notifyChanges);

            _Check_return_ HRESULT get_Orientation(
                _Out_ xaml_controls::Orientation* orientation);

            _Check_return_ HRESULT get_AreScrollSnapPointsRegular(
                _Out_ BOOLEAN* retValue);

            _Check_return_ HRESULT GenerateAndMeasureItemsInViewport(
                _In_ BOOLEAN isScrolling,
                _Inout_ INT& lastViewport,
                _In_ DOUBLE firstItemOffset,
                _In_ wf::Size availableSize,
                _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle,
                _Inout_ wf::Size& stackDesiredSize,
                _In_ wf::Size layoutSlotSize,
                _Inout_ DOUBLE& logicalVisibleSpace,
                _Out_ INT& lastGeneratedChildIndex);

            _Check_return_ HRESULT GenerateAndMeasureItemsInBuffer(
                _In_ INT firstUnrealizedContainerIndex,
                _In_ DOUBLE unusedBuffer,
                _In_ BOOLEAN bIsAfterBuffer,
                _In_ INT maxItemsLeftToGenerateOrRealize,
                _In_ wf::Size layoutSlotSize,
                _In_ INT maxNumOfItemsToGenerateInCurrentMeasureCycle,
                _Inout_ wf::Size& stackDesiredSize,
                _Inout_ DOUBLE& logicalVisibleSpace,
                _Out_ INT& newRealizedCount);

            _Check_return_ HRESULT EnsureOrderedList();

#if DBG
            _Check_return_ HRESULT debug_CheckRealizedChildrenCount();
#endif
            BOOLEAN IsOutOfVisibleRange(
                _In_ INT itemIndex);

            INT WrapIndex(
                _In_ INT index);

            _Check_return_ HRESULT CalculateOffsetAndSize(
                _In_ INT index,
                _Inout_ wf::Size& viewport,
                _In_ wf::Size extent,
                _Inout_ DOUBLE& offset);

            // Provides physical orientation of content
            IFACEMETHOD(get_PhysicalOrientation)(
                _Out_ xaml_controls::Orientation* orientation) override;

            // Called when a manipulation is starting or ending.
            _Check_return_ HRESULT UpdateInManipulation(
                _In_ BOOLEAN isInManipulation,
                _In_ BOOLEAN isInLiveTree,
                _In_ DOUBLE nonVirtualizingOffset) override;

            // Updates the zoom factor
            _Check_return_ HRESULT SetZoomFactor(
                _In_ FLOAT newZoomFactor) override;

            // Methods accessed by scrolling owner to support DirectManipulation.
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

            _Check_return_ HRESULT ComputePixelOffset(
                _In_ BOOLEAN isForHorizontalOrientation,
                _In_ BOOLEAN bUseProvidedLogicalOffset,
                _In_ DOUBLE logicalOffset,
                _Out_ DOUBLE& offset);

            _Check_return_ HRESULT EnsureRealizedItemsNotPartOfOderedListAreNotInVisibleArea(
                _In_ wf::Rect rcChild);

            DOUBLE getInternalOffset();

            _Check_return_ HRESULT ResetScrollData();

            _Check_return_ HRESULT ComputePixelOffsetOfChildAtIndex(
                _In_ INT index,
                _Out_ DOUBLE& offset);

            _Check_return_ HRESULT GetEstimatedOffsetForScrollIntoView(
                _In_ DOUBLE index,
                _In_ DOUBLE viewportSize,
                _In_ BOOLEAN bHorizontal,
                _Out_ DOUBLE& scrollToOffset,
                _Out_ INT& indexToEnsureInView);

            _Check_return_ HRESULT CorrectOffsetForScrollIntoView(
                _In_ DOUBLE viewportSize,
                _In_ BOOLEAN bHorizontal,
                _In_ xaml_controls::IItemsControl* pItemsControl,
                _In_ wf::Size layoutSlotSize,
                _Out_ DOUBLE& firstItemOffset);

            // Updates the non-virtualized offset irrespective of the extent, before the coming MeasureOverride
            // execution updates the extent based on the new zoom factor.
            _Check_return_ HRESULT SetNonVirtualizingOffset(
                _In_ DOUBLE offset);

            INT m_iCurrentLoop;
            DOUBLE m_InternalOffset;
            BOOLEAN m_bFirstMeasureHasBeenCalled;
            BOOLEAN m_bInManipulation;                  // True when a DirectManipulation manip is in progress
            BOOLEAN m_bNotifyLayoutRefresh;             // True when m_bInManipulation is changed and the next ArrangeOverride needs to call IScrollOwner::NotifyLayoutRefreshed
            DOUBLE m_SizeOfVisualsInPopup;
            TrackerPtr<DirectUI::TrackerCollection<xaml::UIElement*>> m_tpOrderedChildrenList;
            INT m_iNullItemPosition;
            DOUBLE m_LastComputedPixelExtent;
            DOUBLE m_LastComputedPixelOffset;
            static const UINT32 m_cSnapPoints = 4;
            static const INT32 m_cSnapPointKeys = 1;

            static const UINT m_DMExtentMultiplier = 401;
            static const UINT m_CarouselOffsetStart = 200;

            public:
            static const UINT m_InitialMeasureHeight = 100;
    };
}
