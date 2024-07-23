// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsPresenter.g.h"
#include "IManipulationDataProvider.h"
#include "IScrollOwner.h"
#include "ScrollData.h"

namespace DirectUI
{
    class ContentControl;

    PARTIAL_CLASS(ItemsPresenter)
        , public IManipulationDataProvider
        , public IScrollOwner
    {

     public:

        // Used by GetFirstVisiblePart callers.
        enum ItemsPresenterParts
        {
            ItemsPresenterParts_Header,
            ItemsPresenterParts_Panel,
            ItemsPresenterParts_Footer
        };

     private:

        // Special indices used when GetSentinelItemSize is called.
        enum SpecialSentinelIndices
        {
            PanelSentinelIndex = -3,
            FooterSentinelIndex = -2,
            TrailingPaddingSentinelIndex = -1,
            LeadingPaddingSentinelIndex = 0,
            HeaderSentinelIndex = 1
        };

        ScrollData m_ScrollData;

        TrackerPtr<ContentControl> m_tpHeader;
        TrackerPtr<ContentControl> m_tpFooter;

        BOOLEAN m_bCanLoadFooter;                       // Specifies whether we can load the footer.
        ctl::WeakRefPtr m_wrScrollViewer;               // Weak ref to our ScrollViewer.

        // Specifies how close from the footer we should be before
        // we load it.
        static const DOUBLE FooterDelayLoadOffset;

        // Used to retrieve the location of the header for GetFirstVisiblePart.
        wf::Rect m_lastHeaderArrangeRect;
        // Used to retrieve the location of the footer for the footer delay loading
        // feature and for GetFirstVisiblePart.
        wf::Rect m_lastFooterArrangeRect;
        // Alongside m_lastHeaderArrangeRect and m_lastFooterArrangeRect, used to determine
        // if we need to notify that the snap points might have changed.
        wf::Rect m_lastPanelArrangeRect;

        BOOLEAN m_bNotifyHorizontalSnapPointsChanges;     // True when NotifySnapPointsChanged needs to be called when horizontal snap points change
        BOOLEAN m_bNotifyVerticalSnapPointsChanges;       // True when NotifySnapPointsChanged needs to be called when vertical snap points change
        BOOLEAN m_bNotifiedHorizontalSnapPointsChanges;   // True when NotifySnapPointsChanged was already called once and horizontal snap points have not been accessed since
        BOOLEAN m_bNotifiedVerticalSnapPointsChanges;     // True when NotifySnapPointsChanged was already called once and vertical snap points have not been accessed since
        BOOLEAN m_bAreSnapPointsKeysHorizontal;           // True when the snap point keys are for horizontal snap points
        DOUBLE m_leadingSnapPointKey;                     // Top/left padding dimension used to compute regular and irregular snap points
        DOUBLE m_trailingSnapPointKey;                    // Bottom/right padding dimension used to compute regular and irregular snap points
        DOUBLE m_leadingMarginSnapPointKey;               // Top/left panel's margin dimension used to compute regular and irregular snap points
        DOUBLE m_trailingMarginSnapPointKey;              // Bottom/right panel's margin dimension used to compute regular and irregular snap points
        FLOAT m_irregularSnapPointKeysOffset;             // Dimension of the unrealized children ahead of the realized children ones
        FLOAT m_regularSnapPointKey;                      // Unique identifier for regular snap points (independent of snap point alignment)
        INT32 m_cIrregularSnapPointKeys;                  // Number of irregular snap point keys
        FLOAT* m_pIrregularSnapPointKeys;                 // Unique identifiers for irregular snap points (independent of snap point alignment)

        // this field will be used later as we finish discovering all scenarios where we need to propagate new zoom factor.
        // IScrollOwner.GetZoomFactor API will go away. And for now we use result from that API with ASSERT comparing with the field.
        FLOAT m_fZoomFactor;                              // Zoom factor from SetZoomFactor

        // IScrollSnapPointsInfo implementation of the ItemsPresenter's panel
        TrackerPtr<xaml_controls::IPanel> m_tpScrollSnapPointsInfo;

        // Event handlers for the IPSPI HorizontalSnapPointsChanged/VerticalSnapPointsChanged events
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> m_spHorizontalSnapPointsChangedEventHandler;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> m_spVerticalSnapPointsChangedEventHandler;

        // Event token for the IPSPI HorizontalSnapPointsChanged/VerticalSnapPointsChanged events
        EventRegistrationToken m_HorizontalSnapPointsChangedToken;
        EventRegistrationToken m_VerticalSnapPointsChangedToken;

        // Attach to the ScrollViewer on loaded event and unattach on unloaded.
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epLoadedHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epUnloadedHandler;
        ctl::EventPtr<ViewChangedEventCallback> m_epScrollViewerViewChangedHandler;

    public:

        // IScrollInfo interface implementation

        _Check_return_ HRESULT get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_CanVerticallyScrollImpl(_In_ BOOLEAN value);

        _Check_return_ HRESULT get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_CanHorizontallyScrollImpl(_In_ BOOLEAN value);

        _Check_return_ HRESULT get_ExtentWidthImpl(_Out_ DOUBLE* pValue);

        _Check_return_ HRESULT get_ExtentHeightImpl(_Out_ DOUBLE* pValue);

        _Check_return_ HRESULT get_ViewportWidthImpl(_Out_ DOUBLE* pValue);

        _Check_return_ HRESULT get_ViewportHeightImpl(_Out_ DOUBLE* pValue);

        _Check_return_ HRESULT get_HorizontalOffsetImpl(_Out_ DOUBLE* pValue);

        _Check_return_ HRESULT get_VerticalOffsetImpl(_Out_ DOUBLE* pValue);

        // MinHorizontalOffset is the minimal horizontal offset of the scrolled content
        _Check_return_ HRESULT get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pValue);

        // MinVerticalOffset is the minimal vertical offset of the scrolled content
        _Check_return_ HRESULT get_MinVerticalOffsetImpl(_Out_ DOUBLE* pValue);

        _Check_return_ HRESULT get_ScrollOwnerImpl(_Outptr_ IInspectable** ppValue);
        _Check_return_ HRESULT put_ScrollOwnerImpl(_In_opt_ IInspectable* pValue);

        _Check_return_ HRESULT LineUpImpl();

        _Check_return_ HRESULT LineDownImpl();

        _Check_return_ HRESULT LineLeftImpl();

        _Check_return_ HRESULT LineRightImpl();

        _Check_return_ HRESULT PageUpImpl();

        _Check_return_ HRESULT PageDownImpl();

        _Check_return_ HRESULT PageLeftImpl();

        _Check_return_ HRESULT PageRightImpl();

        IFACEMETHOD(MouseWheelUp)(_In_ UINT mouseWheelDelta) override;

        IFACEMETHOD(MouseWheelDown)(_In_ UINT mouseWheelDelta) override;

        IFACEMETHOD(MouseWheelLeft)(_In_ UINT mouseWheelDelta) override;

        IFACEMETHOD(MouseWheelRight)(_In_ UINT mouseWheelDelta) override;

        _Check_return_ HRESULT SetHorizontalOffsetImpl(_In_ DOUBLE offset);

        _Check_return_ HRESULT SetVerticalOffsetImpl(_In_ DOUBLE offset);

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

        // End of IScrollInfo interface implementation

        // IScrollSnapPointsInfo interface implementation:

        _Check_return_ HRESULT get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue);

        _Check_return_ HRESULT get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue);

        _Check_return_ HRESULT GetIrregularSnapPointsImpl(
            // The direction of the requested snap points.
            _In_ xaml_controls::Orientation orientation,
            // The alignment used by the caller when applying the requested snap points.
            _In_ xaml_primitives::SnapPointsAlignment alignment,
            // The read-only collection of snap points.
            _Outptr_ wfc::IVectorView<FLOAT>** ppValue);

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

        // IManipulationDataProvider methods accessed by scrolling owner to support DirectManipulation.

        _Check_return_ IFACEMETHOD(get_PhysicalOrientation)(
            _Out_ xaml_controls::Orientation* pValue) override;


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

        _Check_return_ HRESULT GetSizeOfContainer(
            _Out_ wf::Size& size);

        // End of IManipulationDataProvider methods accessed by scrolling owner to support DirectManipulation.

        // IScrollOwner interface implementation

        // This function is called by an IScrollInfo attached to this
        // scrolling owner when any values of scrolling properties (Offset, Extent,
        // and ViewportSize) change.  The function schedules invalidation of
        // other elements like ScrollBars that are dependant on these
        // properties.
        _Check_return_ HRESULT InvalidateScrollInfoImpl() override;

        // Called to notify the scroll owner that the first ArrangeOverride occurred after a
        // IManipulationDataProvider::UpdateInManipulation(TRUE /*isInManipulation*/, ...) call.
        // Marks the transition to 'manipulation arrangement'.
        _Check_return_ HRESULT NotifyLayoutRefreshed() override;

        // Called to notify the scroll owner of a new horizontal offset request.
        _Check_return_ HRESULT NotifyHorizontalOffsetChanging(
            _In_ DOUBLE targetHorizontalOffset,
            _In_ DOUBLE targetVerticalOffset) override;

        // Called to notify the scroll owner of a new vertical offset request.
        _Check_return_ HRESULT NotifyVerticalOffsetChanging(
            _In_ DOUBLE targetHorizontalOffset,
            _In_ DOUBLE targetVerticalOffset) override;

        // Scrolls the content within the scroll owner to the specified
        // horizontal offset position.
        _Check_return_ HRESULT ScrollToHorizontalOffsetImpl(
            _In_ DOUBLE offset) override;

        // Scrolls the content within the scroll owner to the specified vertical
        // offset position.
        _Check_return_ HRESULT ScrollToVerticalOffsetImpl(
            _In_ DOUBLE offset) override;

        // Sets reference to the IScrollInfo implementation
        _Check_return_ HRESULT put_ScrollInfo(
            _In_opt_ IScrollInfo* pValue) override;

        // Gets reference to the IScrollInfo implementation
        _Check_return_ HRESULT get_ScrollInfo(
            _Outptr_result_maybenull_ IScrollInfo** ppValue) override;

        // Returns zoom factor
        _Check_return_ HRESULT get_ZoomFactorImpl(
            _Out_ FLOAT* pValue) override;

        // Called when this DM container wants the DM handler to process the current
        // pure inertia input message, by forwarding it to DirectManipulation.
        _Check_return_ HRESULT ProcessPureInertiaInputMessage(
            _In_ ZoomDirection zoomDirection) override;

        // Tells whether owner is in direct manipulation or not
        _Check_return_ HRESULT IsInDirectManipulationZoom(
            _Out_ BOOLEAN& bIsInDirectManipulationZoom) override;

        // We cannot invalidate the grandchild directly. So this property is
        // informing that we are invalidating the child so the grandchild can
        // use it.
        _Check_return_ HRESULT IsInChildInvalidateMeasure(
            _Out_ BOOLEAN& bIsInChildInvalidateMeasure) override;

        // end of IScrollOwner interface implementation

        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* pReturnValue)
            override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size finalSize,
            _Out_ wf::Size* pReturnValue)
            override;

        IFACEMETHOD(OnApplyTemplate)() override;

        _Check_return_ HRESULT get_HeaderContainer(_Outptr_ ContentControl** ppHeaderContainer);

        _Check_return_ HRESULT get_FooterContainer(_Outptr_ ContentControl** ppFooterContainer);

        // Loads the footer if it's in the viewport.
        // The current viewport is retrieved from the ScrollViewer unless a future viewport is specified.
        // Future viewports are used to pre-load the footer before a scroll operation.
        _Check_return_ HRESULT DelayLoadFooter();
        _Check_return_ HRESULT DelayLoadFooter(
            _In_opt_ wf::Rect* pFutureViewportRect,
            _In_ BOOLEAN updateLayout);

        // Force footer to load if loading is being delayed.
        _Check_return_ HRESULT LoadFooter(_In_ BOOLEAN updateLayout);

        _Check_return_ HRESULT ScrollIntoView(
            _In_ UINT index,
            _In_ BOOLEAN isGroupItemIndex,
            _In_ BOOLEAN isHeader,
            _In_ BOOLEAN forceSynchronous,
            _In_ DOUBLE pixelOffsetHint,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment);

        // Called by HorizontalSnapPointsChangedHandler and
        // VerticalSnapPointsChangedHandler when snap points changed
        _Check_return_ HRESULT OnSnapPointsChanged(_In_ DMMotionTypes motionType);

        static DOUBLE OffsetToIndex(_In_ DOUBLE offset)
        {
            return MAX(0, offset - 2);
        }

        static DOUBLE IndexToOffset(_In_ INT index)
        {
            return index >= 0 ? index + 2 : 0;
        }

        _Check_return_ HRESULT get_PaddingInternal(_Out_ xaml::Thickness* pValue);

        // Called when an ItemsControl's ContentTemplate changes.
        // Allows a hook for us to clean up the old ItemsPresenter before layout is run with the
        // new template and new ItemsPresenter.
        // If Header is a UIElement, we must remove its association with m_tpHeader here.
        // Otherwise, applying the new template and measuring the new ItemsPresenter will throw.
        static _Check_return_ HRESULT Dispose(_In_ CItemsPresenter* pNativeItemsPresenter);

        // Scrollviewer looks at its scrollbarvisibility settings and will substitute infinite
        // if those are set to visible or auto. This is an archaic design that is hard to change
        // However, certain features such as the new modern virtualizingpanels do not appreciate this.
        // They lack the extra communication that scrolldata presents to IScrollInfo implementors, so
        // in those cases we wish to go with a newer more modern approach of actually trusting layout to
        // pass in the correct values.
        BOOLEAN WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(_In_ xaml_controls::Orientation orientation) override;
        BOOLEAN WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(
            _In_ xaml_controls::Orientation orientation,
            _In_ ctl::ComPtr<xaml_controls::IPanel>& spPanel);

        bool EvaluateAndSetNonClippingBehavior(bool isNotBeingPassedInfinity);

        _Check_return_ HRESULT GetHeaderSize(
            _Out_ wf::Size& headerSize);

        // Returns the first visible part (header, panel or footer) for this ItemsPresenter.
        _Check_return_ HRESULT GetFirstVisiblePart(
            _Out_ ItemsPresenterParts* pPart,
            _Out_ DOUBLE* pOffset);

    protected:
        ItemsPresenter();
        ~ItemsPresenter() override;

        // Supports the IManipulationDataProvider interface.
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;

        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT OnLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Called when our ScrollViewer's has changed. Load the footer if necessary.
        _Check_return_ HRESULT OnScrollViewChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pArgs);

    private:

        _Check_return_ HRESULT CalculateArrangeRect(
            _In_ wf::Size availableSize,
            _In_ ctl::ComPtr<xaml_controls::IPanel>& spPanel,
            _Out_ wf::Rect* pHeaderArrangeRect,
            _Out_ wf::Rect* pPanelArrangeRect,
            _Out_ wf::Rect* pFooterArrangeRect);

        _Check_return_ HRESULT IsHorizontal(
            _Out_ BOOLEAN& isHorizontal);

        _Check_return_ HRESULT UpdateScrollData(
            _In_ BOOLEAN isHorizontal,
            _In_ DOUBLE availableDimension);

        _Check_return_ HRESULT SetAndVerifyScrollingData(
            _In_ wf::Size viewport,
            _In_ wf::Size extent,
            _In_ ScrollVector offset);

        _Check_return_ HRESULT OnScrollChange();

        // Check whether we should defer our scroll wheel handling to scroll owner
        _Check_return_ HRESULT get_ShouldPassWheelMessageToScrollOwner(_Out_ BOOLEAN &shouldPass);

        // Allow our scroll owner to handle the last mouse wheel message.
         _Check_return_ HRESULT PassWheelMessageToScrollOwner(_In_ ZoomDirection zoomDirection);

        _Check_return_ HRESULT get_Panel(_Outptr_ xaml_controls::IPanel** ppPanel);

        _Check_return_ HRESULT GetPanelSize(
            _Out_ wf::Size& panelSize);

        _Check_return_ HRESULT GetFooterSize(
            _Out_ wf::Size& footerSize);

        _Check_return_ HRESULT GetHeaderAndLeadingPaddingPixelSize(
            _In_ BOOLEAN isHorizontal,
            _Out_ DOUBLE& size);

        _Check_return_ HRESULT GetHeaderAndLeadingPaddingPixelSize(
            _In_ BOOLEAN isHorizontal,
            _In_ DOUBLE logicalOffset,
            _Out_ DOUBLE& size);

        _Check_return_ HRESULT TranslatePixelDeltaToOffset(
            _In_ DOUBLE currentOffset,
            _In_ BOOLEAN isHorizontal,
            _Inout_ DOUBLE& delta,
            _Out_ DOUBLE& value);

        _Check_return_ HRESULT TranslateVerticalPixelDeltaToOffset(
            _In_ DOUBLE currentOffset,
            _Inout_ DOUBLE& delta,
            _Out_ DOUBLE& value);

        _Check_return_ HRESULT TranslateHorizontalPixelDeltaToOffset(
            _In_ DOUBLE currentOffset,
            _Inout_ DOUBLE& delta,
            _Out_ DOUBLE& value);

        _Check_return_ HRESULT GetSentinelItemSize(
            _In_ INT index,
            _In_ BOOLEAN isHorizontal,
            _Out_ DOUBLE& size);

        _Check_return_ HRESULT GetZoomFactor(
            _Out_ FLOAT* pZoomFactor);

        _Check_return_ HRESULT ComputeAlignmentSize(
            _In_ wf::Size availableSize,
            _In_ wf::Size headerSize,
            _Out_ wf::Size* pStartingSize);

        _Check_return_ HRESULT ComputeStartingOffset(
            _In_ INT alignment,
            _In_ DOUBLE availableSize,
            _In_ DOUBLE requiredSize,
            _Out_ DOUBLE* pStartingOffset);

        _Check_return_ HRESULT GetInnerPanelOffset(
            _In_ BOOLEAN isHorizontal,
            _Out_ DOUBLE* pOffset);

        _Check_return_ HRESULT GetInnerPanelsScrollableDimension(
            _In_ BOOLEAN isHorizontal,
            _Out_ DOUBLE* pValue);

        _Check_return_ HRESULT DecreaseSizeForPaddingInNonVirtualizingDimension(
            _Inout_ wf::Size* pSize);

        _Check_return_ HRESULT IncreaseSizeForPaddingInNonVirtualizingDimension(
            _Inout_ wf::Size* pSize);

        // Called when the inner panel has been updated
        _Check_return_ HRESULT NotifySnapPointsInfoPanelChanged();

        // Determines whether the ItemsPresenter must call NotifySnapPointsChanged
        // when snap points change or not.
        _Check_return_ HRESULT SetSnapPointsChangeNotificationsRequirement(
            _In_ BOOLEAN isForHorizontalSnapPoints,
            _In_ BOOLEAN notifyChanges);

        _Check_return_ HRESULT NotifySnapPointChanges(_In_ BOOLEAN isForHorizontalSnapPoints);

        _Check_return_ HRESULT RefreshRegularSnapPointKeys();
        _Check_return_ HRESULT RefreshIrregularSnapPointKeys();

        _Check_return_ HRESULT ResetSnapPointKeys();

        _Check_return_ HRESULT GetRegularSnapPointKeys(
            _In_ BOOLEAN isForHorizontalSnapPoints,
            _Out_ FLOAT* pSnapPointKey,
            _Out_ DOUBLE* pLeadingSnapPointKey,
            _Out_ DOUBLE* pTrailingSnapPointKey,
            _Out_ DOUBLE* pLeadingMarginSnapPointKey,
            _Out_ DOUBLE* pTrailingMarginSnapPointKey);

        _Check_return_ HRESULT GetIrregularSnapPointKeys(
            _In_ BOOLEAN isForHorizontalSnapPoints,
            _Outptr_result_buffer_(*pcSnapPointKeys) FLOAT** ppSnapPointKeys,
            _Out_ INT32* pcSnapPointKeys,
            _Out_ FLOAT* pSnapPointKeysOffset,
            _Out_ DOUBLE* pLeadingSnapPointKey,
            _Out_ DOUBLE* pTrailingSnapPointKey,
            _Out_ DOUBLE* pLeadingMarginSnapPointKey,
            _Out_ DOUBLE* pTrailingMarginSnapPointKey);

        // Determines the common keys for regular and irregular snap points.
        // Those keys are the left/right margins for a horizontal panel,
        // or the top/bottom margins for a vertical panel
        _Check_return_ HRESULT GetCommonSnapPointKeys(
            _Out_ DOUBLE* pLeadingSnapPointKey,
            _Out_ DOUBLE* pTrailingSnapPointKey,
            _Out_ DOUBLE* pLeadingMarginSnapPointKey,
            _Out_ DOUBLE* pTrailingMarginSnapPointKey);

        _Check_return_ HRESULT AreScrollSnapPointsRegular(
            _In_ BOOLEAN isForHorizontalSnapPoints,
            _Out_ BOOLEAN* pAreScrollSnapPointsRegular);

        // Hooks up the snap points change event
        _Check_return_ HRESULT HookScrollSnapPointsInfoEvents(_In_ BOOLEAN isForHorizontalSnapPoints);

        // Unhooks the snap points change event
        _Check_return_ HRESULT UnhookScrollSnapPointsInfoEvents(_In_ BOOLEAN isForHorizontalSnapPoints);

        // Called when horizontal snap points changed
        _Check_return_ HRESULT OnHorizontalSnapPointsChanged();

        // Called when vertical snap points changed
        _Check_return_ HRESULT OnVerticalSnapPointsChanged();

        _Check_return_ HRESULT NotifySnapPointsChanges();

    };
}
