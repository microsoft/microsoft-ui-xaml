// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingStackPanel.g.h"

namespace DirectUI
{
    // Represents a VirtualizingStackPanel.
    //
    PARTIAL_CLASS(VirtualizingStackPanel)
    {
        private:
            // DirectManipulation-related fields:
            FLOAT m_irregularSnapPointKeysOffset;             // Dimension of the unrealized children ahead of the realized children ones
            FLOAT* m_pIrregularSnapPointKeys;                 // Unique identifiers for irregular snap points (independent of snap point alignment)
            INT32 m_cIrregularSnapPointKeys;                  // Number of irregular snap point keys

            FLOAT m_newItemsStartPosition;                    // position from which new items still start transitioning in
            DOUBLE m_viewPortSizeInPixels;

        protected:
            VirtualizingStackPanel();
            ~VirtualizingStackPanel() override;

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

            // Handle the custom property changed event and call the
            // OnPropertyChanged methods.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

             // Returns a read-only collection of numbers representing the snap points for
            // the provided orientation. Returns an empty collection when no snap points are present.
            _Check_return_ HRESULT GetIrregularSnapPointsInternal(
                // The direction of the requested snap points.
                _In_ xaml_controls::Orientation orientation,
                // The alignment used by the caller when applying the requested snap points.
                _In_ xaml_primitives::SnapPointsAlignment alignment,
                // The read-only collection of snap points.
                _Outptr_ wfc::IVectorView<FLOAT>** ppValue) override;

            _Check_return_ HRESULT AreScrollSnapPointsRegular(_Out_ BOOLEAN* pAreScrollSnapPointsRegular) override;

            _Check_return_ HRESULT NotifySnapPointsChanges(
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount) override;

            _Check_return_ HRESULT ResetSnapPointKeys() override;

            // Determines whether the VirtualizingStackPanel must call NotifySnapPointsChanged
            // when snap points change or not.
            _Check_return_ HRESULT SetSnapPointsChangeNotificationsRequirement(
                _In_ BOOLEAN isForHorizontalSnapPoints,
                _In_ BOOLEAN notifyChanges) override;

            // Computes the total dimension of all realized children
            _Check_return_ HRESULT ComputeTotalRealizedChildrenDimension(
                _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nCount) override;

            _Check_return_ HRESULT MeasureChildForItemsChanged(
                _In_ xaml::IUIElement* pChild) override;

        public:
            static _Check_return_ HRESULT GetVirtualizationModePropertyMetadata(_Out_ xaml::IPropertyMetadata **ppMetadata);

            // Virtual methods.
            _Check_return_ HRESULT OnCleanUpVirtualizedItemImpl(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e);

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
            // the given position.  This will be used by live reordering.
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

            // Scroll the given ItemIndex into the view
            _Check_return_ HRESULT ScrollIntoView(
                _In_ UINT index,
                _In_ BOOLEAN isGroupItemIndex,
                _In_ xaml_controls::ScrollIntoViewAlignment alignment)
                override;

            static _Check_return_ HRESULT ComputeLayoutBoundary(
                _In_ INT index,
                _In_ INT itemCount,
                _In_ bool isHorizontal,
                _Out_ BOOLEAN* isLeftBoundary,
                _Out_ BOOLEAN* isTopBoundary,
                _Out_ BOOLEAN* isRightBoundary,
                _Out_ BOOLEAN* isBottomBoundary);

        private:
            _Check_return_ HRESULT RefreshIrregularSnapPointKeys();

            _Check_return_ HRESULT GetIrregularSnapPointKeys(
                _In_ xaml_controls::Orientation orientation,
                _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
                _In_ UINT32 nCount,
                _Outptr_result_buffer_(*pcSnapPointKeys) FLOAT** ppSnapPointKeys,
                _Out_ INT32* pcSnapPointKeys,
                _Out_ FLOAT* pSnapPointKeysOffset,
                _Out_ FLOAT* pLowerMarginSnapPointKey,
                _Out_ FLOAT* pUpperMarginSnapPointKey);

            _Check_return_ HRESULT GetIrregularSnapPoints(
                _In_ BOOLEAN isForHorizontalSnapPoints,  // True when horizontal snap points are requested.
                _In_ BOOLEAN isForLeftAlignment,         // True when requested snap points will align to the left/top of the children
                _In_ BOOLEAN isForRightAlignment,        // True when requested snap points will align to the right/bottom of the children
                _Outptr_opt_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints,    // Placeholder for returned array
                _Out_ UINT32* pcSnapPoints);                                   // Number of snap points returned

            // Sets up IsVirtualizing, VirtualizationMode
            //
            // IsVirtualizing is true if turned on via the items control and if the panel has a viewport.
            // VSP has a viewport if it's either the scrolling panel or it was given MeasureData.
            _Check_return_ HRESULT SetVirtualizationState(
                _In_ xaml_controls::IItemsControl* pItemsControl);

            // Immediately cleans up any containers that have gone offscreen.  Called by MeasureOverride.
            // When recycling this runs before generating and measuring children; otherwise it runs after.
            _Check_return_ HRESULT CleanupContainers(
                _In_ xaml_controls::IItemsControl* pItemsControl,
                _In_ wf::Size constraint)
                override;

            _Check_return_ HRESULT FindFocusedChildInRealizedChildren(
                _Out_ INT& focusedChild,
                _Out_ INT& previousFocusable,
                _Out_ INT& nextFocusable);

            _Check_return_ HRESULT put_IsVirtualizing(_In_ BOOLEAN isVirtualizing);

            _Check_return_ HRESULT GetClosestOrInsertionIndex(
                _In_ wf::Point position,
                _In_ bool isInsertionIndex,
                _Out_ INT* returnValue);

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
    };
}
