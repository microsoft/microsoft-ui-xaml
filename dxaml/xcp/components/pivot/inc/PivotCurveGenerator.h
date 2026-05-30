// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PivotCommon.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

// PivotCurveGenerator handles the generation of the both the parametric curves
// for positioning the headers, and the related beginning offset of the PivotHeaderPanel.
// It was created to centralize all the logic that was being split between PivotHeaderManager, Pivot,
// and PivotHeaderPanel to help handle the advanced scenarios where we're trying to prevent visual
// glitching by keeping the curve continously applied and maintaining an offset that depends not
// only on the current widths and position of the viewport, but of the previously valid widths as well.
//
// While the positioning of the curves is declaritive in nature, that is it's clearly defined for
// every point the viewport could possibily be at, independent of past positions, we can't
// be completely stateless here because there exists scenarios where the developer will change
// the collection of headers or items at a point in time where we can't simply refresh the entire
// curve without causing an annoying visual glitch.
class PivotCurveGenerator
{
public:
    PivotCurveGenerator();

    // Updates the current offsets based on the direction of the index change
    // and the cached widths of the items
    _Check_return_ HRESULT SelectedIndexChangedEvent(_In_ INT32 idx, _In_ PivotAnimationDirection animationHint);

    // When not under a manipulation we can change the offset of the item
    // in an act of normalization. This function will update the current offsets
    // to reflect this. If we are in a manipulation this will cause a jump until
    // the UI thread submission occurs.
    _Check_return_ HRESULT SyncSelectedItemOffset(_In_ DOUBLE newOffset);

    // This function is NOT designed to do anything other than update the section width
    // in response to a complete relayout.
    // Behavior during a manipulation is currently undefined.
    _Check_return_ HRESULT SyncSectionWidth(_In_ DOUBLE newWidth);

    // Record the new vector of item sizes which will
    // be used to generate future curves. Until the new curve is submitted
    // to DManip the previous sizes will be used to compute the
    // current offsets.
    _Check_return_ HRESULT SyncItemsSizeAndOrder(_In_ const std::vector<DOUBLE>& sizes, _In_ DOUBLE totalSize);

    // Within some parameters the curves don't need to be refreshed. When
    // we haven't wondered a large number of section away from the midpoind
    // and when no size changes have taken place we don't need to generate new
    // DManip curves. Use this method as an optimization to prevent submitting
    // new curves to DManip, which can cause jumps in scrolling.
    bool AreCurvesDirty(bool usingDynamicHeaders);

    // Returns the curve we use for the secondary relationship on dynamic headers.
    void GetDynamicCurveSegments(
        _Inout_ std::vector<DOUBLE>& primaryOffsets,
        _Inout_ std::vector<DOUBLE>& secondaryOffsets,
        _In_ const unsigned panelMultiplier);

    // Returns the curve we use for the secondary relationship on static headers.
    void GetStaticCurveSegments(
        _Inout_ std::vector<DOUBLE>& primaryOffsets,
        _Inout_ std::vector<DOUBLE>& secondaryOffsets,
        _In_ const float viewportSize);

    // Returns the offset by which we need to shift the static header in order to achieve
    // the scrolling effect.
    //      a is the amount of space before the selected item
    //      b is the width of the selected item
    //      c is the amount of space after the selected item
    //      v is the size of the viewport
    static double GetHeaderShiftOffset(double a, double b, double c, double v);

    double GetHeaderPanelOffset() const
    {
        // NOTE: Because there's no notification when this value changes it's important
        // that we make sure the Arrange of the HeaderPanel is dirtied when this value
        // changes.
        return m_currentHeaderPanelOffset;
    }

    double GetCurrentSectionOffset() const
    {
        // NOTE: Because there's no notification when this value changes it's important
        // that we make sure the Arrange of the HeaderPanel is dirtied when this value
        // changes.
        return m_currentOffset;
    }

private:
    _Check_return_ HRESULT CalculateInitialOffsets();
    void FlushPendingHeaderItemWidthChange();

    // How many duplicate groups of parametric curve
    // segments we wish to create for over/underrun.
    static constexpr UINT c_overrun = 3;

    bool m_curvesDirty;
    INT m_currentIdx;
    DOUBLE m_currentOffset;
    DOUBLE m_currentHeaderPanelOffset;
    DOUBLE m_currentParametricCurveOffset;
    DOUBLE m_sectionWidth;
    DOUBLE m_lastCurveCenterpoint;
    bool m_pendingHeaderItemWidthChange;
    std::vector<DOUBLE> m_currentHeaderItemWidths;
    std::vector<DOUBLE> m_pendingHeaderItemWidths;
    DOUBLE m_currentHeaderItemWidthTotalSize;
    DOUBLE m_pendingHeaderItemWidthTotalSize;
};

} } } } XAML_ABI_NAMESPACE_END
