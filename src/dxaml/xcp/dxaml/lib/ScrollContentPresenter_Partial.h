// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollContentPresenter.g.h"

namespace DirectUI
{
    class Page;
    class ScrollData;
    interface IScrollOwner;

    // Displays the content of a ScrollViewer control.
    PARTIAL_CLASS(ScrollContentPresenter)
    {
        private:
            // Clipping rectangle used to replace WPF's GetLayoutClip virtual
            TrackerPtr<xaml_media::IRectangleGeometry> m_tpClippingRectangle;

            // Flag indicating whether the Clip has been set.
            BOOLEAN m_isClipPropertySet;

            // Flags indicating whether the individual headers were added as
            // children of this ScrollContentPresenter.
            BOOLEAN m_isTopLeftHeaderChild;
            BOOLEAN m_isTopHeaderChild;
            BOOLEAN m_isLeftHeaderChild;

            BOOLEAN m_isTabIndexSet;
            INT m_tabIndex;

            // Tracked references to the three header elements.
            TrackerPtr<xaml::IUIElement> m_trTopLeftHeader;
            TrackerPtr<xaml::IUIElement> m_trTopHeader;
            TrackerPtr<xaml::IUIElement> m_trLeftHeader;

            // Reference to the main scrollable region being presented (it could
            // be this).
            ctl::WeakRefPtr m_wrScrollInfo;

            // The state necessary to scroll the content.
            ScrollData* m_pScrollData;

            // InputPaneThemeTransition*
            TrackerPtr<xaml_animation::ITransition> m_tpInputPaneThemeTransition;

            // Flag indicating whether the m_isInputPaneShow has been set.
            BOOLEAN m_isInputPaneShow;

            // this field will be used later as we finish discovering all scenarios where we need to propagate new zoom factor.
            // IScrollOwner.GetZoomFactor API will go away. And for now we use result from that API with ASSERT comparing with the field.
            // Zoom factor from SetZoomFactor
            FLOAT m_fZoomFactor;

            // Zoom factor applied in the most recent layout pass.
            FLOAT m_fLastZoomFactorApplied;

            // Flag indicating whether this instance is used by a semantic zoom control
            BOOLEAN m_isSemanticZoomPresenter;

            // Extents that were not pushed to the ScrollViewer in ScrollContentPresenter::MeasureOverride because
            // the m_isChildActualWidthUsedAsExtent/m_isChildActualWidthUsedAsExtent flags were set to True.
            wf::Size m_unpublishedExtentSize;

            // Set to False by default. Exceptionally set to True when the Content's actual size is used as the IScrollInfo extent.
            bool m_isChildActualWidthUsedAsExtent;
            bool m_isChildActualHeightUsedAsExtent;

            // Set to True by default. Exceptionally set to False when m_isChildActualWidthUsedAsExtent is set to True and the IScrollInfo extent width
            // is not yet up-to-date when ScrollViewer::InvalidateScrollInfo is invoked.
            bool m_isChildActualWidthUpdated;
            // Set to True by default. Exceptionally set to False when m_isChildActualHeightUsedAsExtent is set to True and the IScrollInfo extent height
            // is not yet up-to-date when ScrollViewer::InvalidateScrollInfo is invoked.
            bool m_isChildActualHeightUpdated;

            // When a scroll offset change is requested, the primary notification mechanism for carrying out the change
            // is by invalidating arrange. However, if a child requests a scroll during arrange, the invalidation is never
            // seen, as the flag is cleared in the core after our arrange completes. This allows us to make another arrange pass.
            BOOLEAN m_scrollRequested;

        protected:
            // Initializes a new instance of the ScrollContentPresenter class.
            ScrollContentPresenter();

            // Destroys an instance of the ScrollContentPresenter class.
            ~ScrollContentPresenter() override;

            // Hooks up the Unloaded event handler
            _Check_return_ HRESULT Initialize() override;

            // Called when a ScrollContentPresenter dependency property changed.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Called when the children of the ScrollContentPresenter have been cleared.
            // Clears the flags that record the headers as children, and marks the headers as associated if present.
            _Check_return_ HRESULT OnChildrenCleared() override;

        public:
            // Override the default tab-based navigation order when headers are present such that
            // the tab order is top-left header -> top header -> left header -> content.
            // Handle scenarios where the default behavior is to exit the ScrollContentPresenter or remain inside.
            _Check_return_ HRESULT ProcessTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_opt_ DependencyObject* pCandidateTabStopElement,
                const bool isBackward,
                const bool didCycleFocusAtRootVisualScope,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsTabStopOverridden) override;

            // Override the default tab-based navigation order when headers are present such that
            // the tab order is top-left header -> top header -> left header -> content.
            // Handle scenarios where the default behavior is to enter the ScrollContentPresenter from the outside.
            _Check_return_ HRESULT ProcessCandidateTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_ DependencyObject* pCandidateTabStopElement,
                _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
                const bool isBackward,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsCandidateTabStopOverridden) override;

            // Returns the first focusable element among the headers and content with a TabIndex equal to m_tabIndex.
            _Check_return_ HRESULT GetFirstFocusableElementOverride(
                _Outptr_ DependencyObject** ppFirstFocusable) override;

            // Returns the last focusable element among the headers and content with a TabIndex equal to m_tabIndex.
            _Check_return_ HRESULT GetLastFocusableElementOverride(
                _Outptr_ DependencyObject** ppLastFocusable) override;

            // Overriding this method and returning TRUE in order to navigate among automation children
            // of content and headers in reverse order.
            BOOLEAN AreAutomationPeerChildrenReversed() override
            {
                return TRUE;
            }

            // Property that controls how ScrollContentPresenter measures its
            // Child during layout.  If true, it measures child at infinite
            // space in this dimension.
            _Check_return_ HRESULT get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_CanVerticallyScrollImpl(_In_ BOOLEAN value);

            // Property that controls how ScrollContentPresenter measures its
            // Child during layout.  If true, it measures child at infinite
            // space in this dimension.
            _Check_return_ HRESULT get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_CanHorizontallyScrollImpl(_In_ BOOLEAN value);

            // Gets the horizontal size of the extent.
            _Check_return_ HRESULT get_ExtentWidthImpl(_Out_ DOUBLE* pValue);

            // Gets the vertical size of the extent.
            _Check_return_ HRESULT get_ExtentHeightImpl(_Out_ DOUBLE* pValue);

            // Gets the horizontal size of the viewport for this content.
            _Check_return_ HRESULT get_ViewportWidthImpl(_Out_ DOUBLE* pValue);

            // Gets the vertical size of the viewport for this content.
            _Check_return_ HRESULT get_ViewportHeightImpl(_Out_ DOUBLE* pValue);

            // Gets the horizontal offset of the scrolled content.
            _Check_return_ HRESULT get_HorizontalOffsetImpl(_Out_ DOUBLE* pValue);

            // Gets the vertical offset of the scrolled content.
            _Check_return_ HRESULT get_VerticalOffsetImpl(_Out_ DOUBLE* pValue);

            // Gets the minimal horizontal offset of the scrolled content.
            _Check_return_ HRESULT get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pValue);

            // Gets the minimal vertical offset of the scrolled content.
            _Check_return_ HRESULT get_MinVerticalOffsetImpl(_Out_ DOUBLE* pValue);

            // ScrollOwner is the container that controls any scrollbars,
            // headers, etc... that are dependent on this IScrollInfo's
            // properties.  Implementers of IScrollInfo should call
            // InvalidateScrollInfo() on this object when properties change.
            _Check_return_ HRESULT get_ScrollOwnerImpl(
                _Outptr_ IInspectable** ppValue);

            _Check_return_ HRESULT put_ScrollOwnerImpl(
                _In_opt_ IInspectable* pValue);

            // Scroll content by one line to the top.
            _Check_return_ HRESULT LineUpImpl();

            // Scroll content by one line to the bottom.
            _Check_return_ HRESULT LineDownImpl();

            // Scroll content by one line to the left.
            _Check_return_ HRESULT LineLeftImpl();

            // Scroll content by one line to the right.
            _Check_return_ HRESULT LineRightImpl();

            // Scroll content by one page to the top.
            _Check_return_ HRESULT PageUpImpl();

            // Scroll content by one page to the bottom.
            _Check_return_ HRESULT PageDownImpl();

            // Scroll content by one page to the left.
            _Check_return_ HRESULT PageLeftImpl();

            // Scroll content by one page to the right.
            _Check_return_ HRESULT PageRightImpl();

            // Scroll content by one line to the top.
            _Check_return_ HRESULT MouseWheelUpImpl();

            // Scroll content by one line to the bottom.
            _Check_return_ HRESULT MouseWheelDownImpl();

            // Scroll content by one page to the left.
            _Check_return_ HRESULT MouseWheelLeftImpl();

            // Scroll content by one page to the right.
            _Check_return_ HRESULT MouseWheelRightImpl();

            // IScrollInfo mouse wheel scroll implementations, based on delta value.
            IFACEMETHOD(MouseWheelUp)(_In_ UINT mouseWheelDelta) override;
            IFACEMETHOD(MouseWheelDown)(_In_ UINT mouseWheelDelta) override;
            IFACEMETHOD(MouseWheelLeft)(_In_ UINT mouseWheelDelta) override;
            IFACEMETHOD(MouseWheelRight)(_In_ UINT mouseWheelDelta) override;

            // Set the HorizontalOffset to the passed value.
            _Check_return_ HRESULT SetHorizontalOffsetImpl(
                _In_ DOUBLE offset);

            // Set the VerticalOffset to the passed value.
            _Check_return_ HRESULT SetVerticalOffsetImpl(
                _In_ DOUBLE offset);

            // ScrollContentPresenter implementation of its public MakeVisible method.
            // Does not animate the move by default.
            _Check_return_ HRESULT MakeVisibleImpl(
                // The element that should become visible.
                _In_ xaml::IUIElement* visual,
                // A rectangle representing in the visual's coordinate space to
                // make visible.
                wf::Rect rectangle,
                // A rectangle in the IScrollInfo's coordinate space that has
                // been made visible.  Other ancestors to in turn make this new
                // rectangle visible.  The rectangle should generally be a
                // transformed version of the input rectangle.  In some cases,
                // like when the input rectangle cannot entirely fit in the
                // viewport, the return value might be smaller.
                _Out_ wf::Rect* resultRectangle);

            // This scrolls to make the rectangle in the UIElement's coordinate
            // space visible.
            _Check_return_ HRESULT MakeVisibleImpl(
                // The element that should become visible.
                _In_ xaml::IUIElement* visual,
                // A rectangle representing in the visual's coordinate space to
                // make visible.
                wf::Rect rectangle,
                // When set to True, the DManip ZoomToRect method is invoked.
                BOOLEAN useAnimation,
                // A rectangle in the IScrollInfo's coordinate space that has
                // been made visible.  Other ancestors to in turn make this new
                // rectangle visible.  The rectangle should generally be a
                // transformed version of the input rectangle.  In some cases,
                // like when the input rectangle cannot entirely fit in the
                // viewport, the return value might be smaller.
                DOUBLE horizontalAlignmentRatio,
                DOUBLE verticalAlignmentRatio,
                DOUBLE offsetX,
                DOUBLE offsetY,
                _Out_ wf::Rect* resultRectangle,
                _Out_opt_ DOUBLE* appliedOffsetX = nullptr,
                _Out_opt_ DOUBLE* appliedOffsetY = nullptr);

            // Determine how down we need to scroll to accommodate the desired
            // view.
            static _Check_return_ HRESULT ComputeScrollOffsetWithMinimalScroll(
                _In_ FLOAT topView,
                _In_ FLOAT bottomView,
                _In_ FLOAT topChild,
                _In_ FLOAT bottomChild,
                _Out_ FLOAT* pOffset);

            // Ensure the offset we're scrolling to is valid.
            static _Check_return_ HRESULT ValidateInputOffset(
                _In_ DOUBLE offset,
                _In_ DOUBLE minOffset,
                _In_ DOUBLE maxOffset,
                _Out_ DOUBLE* pValidatedOffset);

            // Apply a template to the ScrollContentPresenter.
            IFACEMETHOD(OnApplyTemplate)();

            // Returns an offset coerced into the [0, Extent - Viewport] range.
            static DOUBLE CoerceOffset(
                _In_ DOUBLE offset,
                _In_ DOUBLE extent,
                _In_ DOUBLE viewport);

            // Provides the behavior for the Measure pass of layout. Classes can
            // override this method to define their own Measure pass behavior.
            IFACEMETHOD(MeasureOverride)(
                // Measurement constraints, a control cannot return a size
                // larger than the constraint.
                _In_ wf::Size availableSize,
                // The desired size of the control.
                _Out_ wf::Size* pReturnValue);

            // Provides the behavior for the Arrange pass of layout.  Classes
            // can override this method to define their own Arrange pass
            // behavior.
            IFACEMETHOD(ArrangeOverride)(
                // The computed size that is used to arrange the content.
                _In_ wf::Size finalSize,
                // The size of the control.
                _Out_ wf::Size* pReturnValue);

            // Gets a value indicating whether the current
            // ScrollContentPresenter is a scrolling client.
            _Check_return_ HRESULT IsScrollClient(
                _Out_ BOOLEAN* pbIsScrollClient);

            // Gets a value indicating whether the current
            // ScrollData's m_Offset and m_ComputedOffset are in sync or not.
            _Check_return_ HRESULT AreScrollOffsetsInSync(
                _Out_ BOOLEAN& areScrollOffsetsInSync);

            // Called when InputPane is showing.
            void NotifyInputPaneStateChange(_In_ BOOLEAN isInputPaneShow);
            _Check_return_ HRESULT ApplyInputPaneTransition(_In_ BOOLEAN isEnableThemeTransition);

            // Helper method to get our owner and its scrolling
            // content talking.  Method introduces the current owner/content,
            // and clears a from any previous content.
            _Check_return_ HRESULT HookupScrollingComponents();

            // register this instance as under control of a semanticzoom control
            _Check_return_ HRESULT RegisterAsSemanticZoomPresenter();

            // Updates the zoom factor
            _Check_return_ HRESULT SetZoomFactor(
                _In_ FLOAT newZoomFactor);

            // Returns the zoom factor applied in the most recent layout pass.
            FLOAT GetLastZoomFactorApplied()
            {
                return m_fLastZoomFactorApplied;
            }

            // Set the HorizontalOffset and VerticalOffset to the passed values, using the provided extents to determine the upper boundaries.
            _Check_return_ HRESULT SetOffsetsWithExtents(
                _In_ DOUBLE offsetX,
                _In_ DOUBLE offsetY,
                _In_ DOUBLE extentWidth,
                _In_ DOUBLE extentHeight);

            // Sets the weak references to the header elements
            _Check_return_ HRESULT put_TopLeftHeader(
                _In_opt_ const ctl::ComPtr<IUIElement>& spTopLeftHeader,
                _In_ ScrollViewer* pOwningScrollViewer);
            _Check_return_ HRESULT put_TopHeader(
                _In_opt_ const ctl::ComPtr<IUIElement>& spTopHeader,
                _In_ ScrollViewer* pOwningScrollViewer);
            _Check_return_ HRESULT put_LeftHeader(
                _In_opt_ const ctl::ComPtr<IUIElement>& spLeftHeader,
                _In_ ScrollViewer* pOwningScrollViewer);

            // Returns the owning quadrant of the provided element and whether or not it is a direct child.
            _Check_return_ HRESULT GetHeaderOwnership(
                _In_ DependencyObject* pElement,
                _Out_ BOOLEAN* pIsElementDirectChild,
                _Out_ BOOLEAN* pIsElementInTopLeftHeader,
                _Out_ BOOLEAN* pIsElementInTopHeader,
                _Out_ BOOLEAN* pIsElementInLeftHeader,
                _Out_ BOOLEAN* pIsElementInContent);

            BOOLEAN IsTopLeftHeaderChild()
            {
                return m_isTopLeftHeaderChild;
            }

            BOOLEAN IsTopHeaderChild()
            {
                return m_isTopHeaderChild;
            }

            BOOLEAN IsLeftHeaderChild()
            {
                return m_isLeftHeaderChild;
            }

            // Removes the headers from the Clears the flags that record the headers as children, and marks the headers as associated if present.
            _Check_return_ HRESULT UnparentHeaders();

            // Called by the owning ScrollViewer when the Content property is changing.
            _Check_return_ HRESULT OnContentChanging(
                _In_ IInspectable* pOldContent);

            // Called when the parent of this ScrollContentPresenter changed.
            _Check_return_ HRESULT OnTreeParentUpdated(
                _In_opt_ CDependencyObject* pNewParent,
                _In_ BOOLEAN isParentAlive) override;

#ifdef DBG
            // Traces value of all ScrollData fields for debugging purposes
            void TraceScrollData();
#endif

            // Called when a criteria for the CanUseActualWidthAsExtent or CanUseActualHeightAsExtent evaluation changed.
            // Calls InvalidateMeasure when the evaluation actually changes so the special
            // mode can be entered or exited.
            _Check_return_ HRESULT RefreshUseOfActualSizeAsExtent(
                _In_ UIElement* pManipulatedElement);

            // Return true if the child's actual Width size is used for the
            // extent exposed through IScrollInfo
            bool IsChildActualWidthUsedAsExtent()
            {
                return m_isChildActualWidthUsedAsExtent;
            }

            // Return true if the child's actual Height size is used for the
            // extent exposed through IScrollInfo
            bool IsChildActualHeightUsedAsExtent()
            {
                return m_isChildActualHeightUsedAsExtent;
            }

            // Called from the ScrollViewer's InvalidateScrollInfo() implementation when IsChildActualWidthUsedAsExtent() above returned True.
            // Is used to determine whether the IScrollInfo extent width is already up-to-date or not. It may not be up-to-date in a MeasureOverride VerifyScrollData call.
            bool IsChildActualWidthUpdated()
            {
                ASSERT(m_isChildActualWidthUsedAsExtent);
                return m_isChildActualWidthUpdated;
            }

            // Called from the ScrollViewer's InvalidateScrollInfo() implementation when IsChildActualHeightUsedAsExtent() above returned True.
            // Is used to determine whether the IScrollInfo extent height is already up-to-date or not. It may not be up-to-date in a MeasureOverride VerifyScrollData call.
            bool IsChildActualHeightUpdated()
            {
                ASSERT(m_isChildActualHeightUsedAsExtent);
                return m_isChildActualHeightUpdated;
            }

        private:
            // Unloaded event handler.
            _Check_return_ HRESULT OnUnloaded(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Determine a specialized clip for text scenarios.
            _Check_return_ HRESULT CalculateTextBoxClipRect(
                _In_ wf::Size availableSize,
                _Out_ wf::Rect* pClipRect);

            // ScrollContentPresenter clips its content to arrange size.
            _Check_return_ HRESULT UpdateClip(
                _In_ wf::Size availableSize);

            // Enters the mode where the child's actual size is used for
            // the extent exposed through IScrollInfo.
            void StartUseOfActualWidthAsExtent();

            // Leaves the mode where the child's actual size is used for
            // the extent exposed through IScrollInfo.
            void StopUseOfActualWidthAsExtent();

            // Enters the mode where the child's actual size is used for
            // the extent exposed through IScrollInfo.
            void StartUseOfActualHeightAsExtent();

            // Leaves the mode where the child's actual size is used for
            // the extent exposed through IScrollInfo.
            void StopUseOfActualHeightAsExtent();

            // Determines whether the mode that uses the child's actual size for the IScrollInfo
            // extent is applicable or not.
            // The answer is partially evaluated with temporary regkeys.
            static _Check_return_ HRESULT CanUseActualWidthAsExtent(
                _In_opt_ IScrollOwner* pScrollOwner,
                _In_opt_ xaml_controls::IScrollViewer* pScrollViewer,
                _In_opt_ xaml::IFrameworkElement* pContentFE,
                _Out_ bool* pCanUseActualWidthAsExtent);

            static _Check_return_ HRESULT CanUseActualHeightAsExtent(
                _In_opt_ IScrollOwner* pScrollOwner,
                _In_opt_ xaml_controls::IScrollViewer* pScrollViewer,
                _In_opt_ xaml::IFrameworkElement* pContentFE,
                _Out_ bool* pCanUseActualHeightAsExtent);

            _Check_return_ HRESULT SetHorizontalOffsetPrivate(
                double offset, _Out_opt_ bool* isScrollRequested = nullptr, _Out_opt_ double* currentOffset = nullptr, _Out_opt_ double* requestedOffset = nullptr);
            _Check_return_ HRESULT SetVerticalOffsetPrivate(
                double offset, _Out_opt_ bool* isScrollRequested = nullptr, _Out_opt_ double* currentOffset = nullptr, _Out_opt_ double* requestedOffset = nullptr);

            // Verifies scrolling data using the passed viewport and extent as
            // newly computed values.  Checks the X/Y offset and coerces them
            // into the range [0, Extent - ViewportSize].  If extent, viewport,
            // or the newly coerced offsets are different than the existing
            // offset, caches are updated and InvalidateScrollInfo() is called.
            _Check_return_ HRESULT VerifyScrollData(
                _In_ wf::Size viewport,
                _In_ wf::Size extent);

            // Coerce both of the offests using CoerceOffset method and store
            // them as the new computed offsets if they've changed.
            _Check_return_ HRESULT CoerceOffsets(
                _Out_ BOOLEAN* pIsValid);

            // Get (or create on demand) the ScrollContentPresenter's scrolling
            // state.
            _Check_return_ HRESULT get_ScrollData(
                _Outptr_ ScrollData** ppScrollData);

            _Check_return_ HRESULT get_TopLeftHeader(
                _Outptr_result_maybenull_ IUIElement** ppTopLeftHeader);

            _Check_return_ HRESULT get_TopHeader(
                _Outptr_result_maybenull_ IUIElement** ppTopHeader);

            _Check_return_ HRESULT get_LeftHeader(
                _Outptr_result_maybenull_ IUIElement** ppLeftHeader);

            // Returns the size of the potential headers, taking the zoom factor into account.
            _Check_return_ HRESULT GetZoomedHeadersSize(
                _Out_ XSIZEF* pSize);

            // Retrieves the primary child as a IUIElement, taking the potential headers into account.
            _Check_return_ HRESULT GetPrimaryChild(
                _Outptr_ xaml::IUIElement** ppChild);

            // Adds a header to this ScrollContentPresenter's children.
            _Check_return_ HRESULT AddHeader(
                _In_ const ctl::ComPtr<xaml_controls::IScrollViewer>& spScrollViewer,
                _In_opt_ const ctl::ComPtr<IUIElement>& spTopLeftHeader,
                _In_opt_ const ctl::ComPtr<IUIElement>& spTopHeader,
                _In_opt_ const ctl::ComPtr<IUIElement>& spLeftHeader,
                _In_ BOOLEAN isTopHeader,
                _In_ BOOLEAN isLeftHeader);

            // Removes the top-left header from this ScrollContentPresenter's children and notifies the owning ScrollViewer.
            _Check_return_ HRESULT RemoveTopLeftHeader(
                _In_opt_ ScrollViewer* pScrollViewer,
                _In_ bool removeFromChildrenCollection);

            // Removes the top header from this ScrollContentPresenter's children and notifies the owning ScrollViewer.
            _Check_return_ HRESULT RemoveTopHeader(
                _In_opt_ ScrollViewer* pScrollViewer,
                _In_ bool removeFromChildrenCollection);

            // Removes the left header from this ScrollContentPresenter's children and notifies the owning ScrollViewer.
            _Check_return_ HRESULT RemoveLeftHeader(
                _In_opt_ ScrollViewer* pScrollViewer,
                _In_ bool removeFromChildrenCollection);

            // Inserts the provided element in the children collection at the specified index.
            _Check_return_ HRESULT InsertChildInternal(
                _In_ UINT index,
                _In_ const ctl::ComPtr<IUIElement>& spChild);

            // Removes the provided element from the children collection.
            _Check_return_ HRESULT RemoveChildInternal(
                _In_ const ctl::ComPtr<IUIElement>& spChild);

            // Discards the GlobalScaleFactor sparse storage of the primary child so that the layout
            // rounding method falls back to just using the plateau scale. Invoked when the last header
            // element is unparented by this ScrollContentPresenter.
            _Check_return_ HRESULT ResetPrimaryChildGlobalScaleFactor();

            // Determines if a direct child has a custom TabIndex value set, while TabStop is True.
            _Check_return_ HRESULT HasDirectChildWithTabIndexSet(
                _Out_ BOOLEAN* pHasDirectChildWithTabIndexSet);

            // Handles tab-based navigation when a custom TabIndex value is set for a header or the content.
            _Check_return_ HRESULT ProcessTabStopPrivate(
                _In_opt_ DependencyObject* pFocusedElement,
                const bool isBackward,
                BOOLEAN isFocusedElementInTopLeftHeader,
                BOOLEAN isFocusedElementInTopHeader,
                BOOLEAN isFocusedElementInLeftHeader,
                BOOLEAN isFocusedElementInContent,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsTabStopOverridden);

            // Determines the location of the first focusable element in scenarios where a custom TabIndex value is set for a header or the content.
            _Check_return_ HRESULT GetFirstFocusableElementPrivate(
                _Out_ BOOLEAN* pIsTopLeftHeader,
                _Out_ BOOLEAN* pIsTopHeader,
                _Out_ BOOLEAN* pIsLeftHeader,
                _Out_ BOOLEAN* pIsContent);

            // Determines the location of the last focusable element in scenarios where a custom TabIndex value is set for a header or the content.
            _Check_return_ HRESULT GetLastFocusableElementPrivate(
                _Out_ BOOLEAN* pIsTopLeftHeader,
                _Out_ BOOLEAN* pIsTopHeader,
                _Out_ BOOLEAN* pIsLeftHeader,
                _Out_ BOOLEAN* pIsContent);

            // Determines the next focusable element among the headers and content for scenarios that involve a custom TabIndex value.
            _Check_return_ HRESULT GetNextFocusableElementPrivate(
                _In_ BOOLEAN isFocusedElementInTopLeftHeader,
                _In_ BOOLEAN isFocusedElementInTopHeader,
                _In_ BOOLEAN isFocusedElementInLeftHeader,
                _In_ BOOLEAN isFocusedElementInContent,
                _Out_ BOOLEAN* pIsTopLeftHeader,
                _Out_ BOOLEAN* pIsTopHeader,
                _Out_ BOOLEAN* pIsLeftHeader,
                _Out_ BOOLEAN* pIsContent,
                _Out_ ctl::ComPtr<IDependencyObject>* pspNextFocusable);

            // Determines the previous focusable element among the headers and content for scenarios that involve a custom TabIndex value.
            _Check_return_ HRESULT GetPreviousFocusableElementPrivate(
                _In_ BOOLEAN isFocusedElementInTopLeftHeader,
                _In_ BOOLEAN isFocusedElementInTopHeader,
                _In_ BOOLEAN isFocusedElementInLeftHeader,
                _In_ BOOLEAN isFocusedElementInContent,
                _Out_ BOOLEAN* pIsTopLeftHeader,
                _Out_ BOOLEAN* pIsTopHeader,
                _Out_ BOOLEAN* pIsLeftHeader,
                _Out_ BOOLEAN* pIsContent,
                _Out_ ctl::ComPtr<IDependencyObject>* pspPreviousFocusable);

            // Determines where the currently focused element and new candidate are in relation to the headers and content.
            _Check_return_ HRESULT AnalyzeTabbingElements(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_opt_ DependencyObject* pCandidateTabStopElement,
                _Out_ BOOLEAN* pIsFocusedElementInTopLeftHeader,
                _Out_ BOOLEAN* pIsFocusedElementInTopHeader,
                _Out_ BOOLEAN* pIsFocusedElementInLeftHeader,
                _Out_ BOOLEAN* pIsFocusedElementInContent,
                _Out_ BOOLEAN* pIsCandidateElementInTopLeftHeader,
                _Out_ BOOLEAN* pIsCandidateElementInTopHeader,
                _Out_ BOOLEAN* pIsCandidateElementInLeftHeader,
                _Out_ BOOLEAN* pIsCandidateElementInContent);

            // Returns the requested direct child as a dependency object.
            _Check_return_ HRESULT GetDirectChild(
                _In_ BOOLEAN topLeftHeader,
                _In_ BOOLEAN topHeader,
                _In_ BOOLEAN leftHeader,
                _In_ BOOLEAN content,
                _Out_ ctl::ComPtr<IDependencyObject>* pspChild);

            // Determines if an element is focusable or has a focusable child.
            _Check_return_ HRESULT GetTabIndex(
                _In_ ctl::ComPtr<IDependencyObject> spElement,
                _Out_opt_ BOOLEAN* pIsTabStop,
                _Out_ INT* pTabIndex);

            _Check_return_ HRESULT IsAncestorOfAndMostAncestorPageBetween(
                _In_ DependencyObject* pElement,
                _Out_ BOOLEAN* pIsAncestor,
                _Outptr_result_maybenull_ Page** ppMostAncestorPageBetween);

            _Check_return_ HRESULT GetFullScreenPageBottomAppBarHeight(
                _In_ Page* pPage,
                _Out_ DOUBLE* pBottomAppBarHeight);

            void StoreLayoutCycleWarningContext(
                float oldViewportWidth,
                float oldViewportHeight,
                float oldExtentWidth,
                float oldExtentHeight,
                _In_ ScrollData* scrollData);
    };
}

