// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StackPanel.g.h"

namespace DirectUI
{
    // Represents a StackPanel control.
    //
    PARTIAL_CLASS(StackPanel)
    {
        protected:
            StackPanel();
            ~StackPanel() override;

        public:
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
                _In_opt_ IScrollInfo* pScrollInfo,
                _Inout_ INT* pResult) override;
            IFACEMETHOD(GetItemsPerPage)(
                _In_ IScrollInfo* pScrollInfo,
                _Out_ DOUBLE* pItemsPerPage) override;

        public:
            // Called when snap points have changed.
            _Check_return_ HRESULT NotifySnapPointsChanged(_In_ BOOLEAN isForHorizontalSnapPoints);

        private:
            // Called when horizontal snap points changed
            _Check_return_ HRESULT OnHorizontalSnapPointsChanged();

            // Called when vertical snap points changed
            _Check_return_ HRESULT OnVerticalSnapPointsChanged();

        private:
            // common method to find closestIndex or InsertionIndex
            _Check_return_ HRESULT GetClosestOrInsertionIndex(
                _In_ wf::Point position,
                _In_ bool isInsertionIndex,
                _Out_ INT* returnValue);
    };
}
