// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PivotCurveGenerator.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

//#define PVTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define PVTRACE(...)

#undef max
#undef min

PivotCurveGenerator::PivotCurveGenerator()
    : m_curvesDirty(true)
    , m_currentIdx(0)
    , m_currentOffset(0.0)
    , m_currentHeaderPanelOffset(0.0)
    , m_currentParametricCurveOffset(0.0)
    , m_sectionWidth(0.0)
    , m_lastCurveCenterpoint(0.0)
    , m_pendingHeaderItemWidthChange(false)
    , m_currentHeaderItemWidthTotalSize(0.0)
    , m_pendingHeaderItemWidthTotalSize(0.0)
{}

_Check_return_ HRESULT 
PivotCurveGenerator::SelectedIndexChangedEvent(_In_ INT32 idx, _In_ PivotAnimationDirection animationHint)
{
    INT itemsToShift = 0;
    const UINT itemCount = static_cast<UINT>(m_currentHeaderItemWidths.size());
    // There's two ways to move between any two indices, by shifting
    // the first item to the back, or the back item to the front. We have
    // to move a certain direction to keep the parametric curves synced
    // up with what the reality is.
    if (animationHint == PivotAnimationDirection_Right)
    {
        itemsToShift = PositiveMod(idx - m_currentIdx, itemCount); 
    }
    else if (animationHint == PivotAnimationDirection_Left)
    {
        itemsToShift = -static_cast<INT>((itemCount - PositiveMod(idx - m_currentIdx, itemCount)));
    }
    else if (animationHint == PivotAnimationDirection_Reset)
    {
        // If the animationHint is 'Reset' then we're specifying a new Header index
        // without the Pivot panel itself moving the correct distance to that new header.
        // This is an edge-case scenario that happens when the SelectedIndex changes 
        // while the Pivot is already in motion. In this case the curves are dirty.
        m_curvesDirty = true;
    }

    if (itemCount != 0)
    {
        const INT sign = itemsToShift < 0 ? -1 : 1;        
        const UINT visualOffsetIdx = static_cast<UINT>(m_currentOffset / m_sectionWidth + 0.5);
        const UINT baseIdx = (visualOffsetIdx - m_currentIdx) % itemCount;

        DOUBLE headerOffset = m_currentHeaderPanelOffset;
        DOUBLE curveOffset = m_currentParametricCurveOffset;
        DOUBLE panelOffset = m_currentOffset;

        for (UINT itemIdx = visualOffsetIdx; 
            itemIdx != (visualOffsetIdx + itemsToShift);
            itemIdx += sign)
        {
            if (sign < 0)
            {
                const DOUBLE itemWidth = m_currentHeaderItemWidths[(itemIdx - baseIdx - 1) % itemCount];
                headerOffset -= itemWidth;
                curveOffset -= (m_sectionWidth - itemWidth);
                panelOffset -= m_sectionWidth;
            }
            else
            {
                const DOUBLE itemWidth = m_currentHeaderItemWidths[(itemIdx - baseIdx) % itemCount];
                headerOffset += itemWidth;
                curveOffset += (m_sectionWidth - itemWidth);
                panelOffset += m_sectionWidth;
            }
        }

        PVTRACE(L"[PCG]: Shifting %d items", itemsToShift);
        PVTRACE(L"[PCG]: headerOffset from %f to %f", m_currentHeaderPanelOffset, headerOffset);
        PVTRACE(L"[PCG]: curveOffset from %f to %f", m_currentParametricCurveOffset, curveOffset);
        PVTRACE(L"[PCG]: panelOffset from %f to %f", m_currentOffset, panelOffset);
        m_currentParametricCurveOffset = curveOffset;
        m_currentHeaderPanelOffset = headerOffset;
        m_currentOffset = panelOffset;
        m_currentIdx = idx;
    }
    
    return S_OK;
}

_Check_return_ HRESULT 
PivotCurveGenerator::SyncSelectedItemOffset(_In_ DOUBLE newOffset)
{
    if (!AreClose(newOffset, m_currentOffset))
    {
        PVTRACE(L"[PCG]: Syncing selected item offset from %f to %f", m_currentOffset, newOffset);
        m_currentOffset = newOffset;
        IFC_RETURN(CalculateInitialOffsets());
    }

    return S_OK;
}

_Check_return_ HRESULT 
PivotCurveGenerator::SyncSectionWidth(_In_ DOUBLE newWidth)
{
    if (!AreClose(newWidth, m_sectionWidth))
    {
        PVTRACE(L"[PCG]: Syncing section width from %f to %f", m_sectionWidth, newWidth);
        m_sectionWidth = newWidth;
        IFC_RETURN(CalculateInitialOffsets());
    }

    return S_OK;
}

_Check_return_ HRESULT 
PivotCurveGenerator::CalculateInitialOffsets()
{
    if (m_pendingHeaderItemWidthChange)
    {
        m_currentHeaderItemWidths = std::move(m_pendingHeaderItemWidths);
        m_currentHeaderItemWidthTotalSize = m_pendingHeaderItemWidthTotalSize;
        m_pendingHeaderItemWidthChange = false;
    }
    m_curvesDirty = true;

    const UINT itemCount = static_cast<UINT>(m_currentHeaderItemWidths.size());

    if (itemCount > 0)
    {
        const UINT visualOffsetIdx = static_cast<UINT>(m_currentOffset / m_sectionWidth + 0.5);
        const UINT baseIdx = (visualOffsetIdx - m_currentIdx) % itemCount;
        const UINT repeatingGroupCount = (visualOffsetIdx - baseIdx - m_currentIdx) / itemCount;
        const DOUBLE totalSectionItemSize = m_sectionWidth * itemCount;

        DOUBLE headerPanelOffset = 0.0;
        DOUBLE curveOffset = 0.0;
        headerPanelOffset += repeatingGroupCount * m_currentHeaderItemWidthTotalSize;
        curveOffset += repeatingGroupCount * (totalSectionItemSize - m_currentHeaderItemWidthTotalSize);

        for (UINT itemIdx = 0; itemIdx < baseIdx; itemIdx++)
        {
            const DOUBLE itemWidth = m_currentHeaderItemWidths[itemCount - itemIdx - 1];
            headerPanelOffset += itemWidth;
            curveOffset += (m_sectionWidth - itemWidth);
        }
        for (UINT itemIdx = 0; itemIdx < (visualOffsetIdx - baseIdx) % itemCount; itemIdx++)
        {
            const DOUBLE itemWidth = m_currentHeaderItemWidths[itemIdx];
            headerPanelOffset += itemWidth;
            curveOffset += m_sectionWidth - itemWidth;
        }

        PVTRACE(L"[PCG]: Calc initial offsets. baseIdx: %d visualIdx: %d itemCount: %d", baseIdx, visualOffsetIdx, itemCount);
        PVTRACE(L"------ HeaderPanelOffset from %f to %f, CurveOffset from %f to %f", m_currentHeaderPanelOffset, headerPanelOffset, m_currentParametricCurveOffset, curveOffset);
        m_currentHeaderPanelOffset = headerPanelOffset;
        m_currentParametricCurveOffset = curveOffset;
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotCurveGenerator::SyncItemsSizeAndOrder(_In_ const std::vector<DOUBLE>& sizes, _In_ DOUBLE totalSize)
{
    if (m_currentHeaderItemWidths.size() == 0)
    {
        m_currentHeaderItemWidths = std::move(sizes);
        m_currentHeaderItemWidthTotalSize = totalSize;
        m_pendingHeaderItemWidthChange = false;
        IFC_RETURN(CalculateInitialOffsets());
        m_curvesDirty = true;
    }
    else if (m_currentHeaderItemWidths.size() != sizes.size() || 
        !std::equal(m_currentHeaderItemWidths.begin(), m_currentHeaderItemWidths.end(), sizes.begin()))
    {
        m_pendingHeaderItemWidths = std::move(sizes);
        m_pendingHeaderItemWidthTotalSize = totalSize;
        m_pendingHeaderItemWidthChange = true;
        m_curvesDirty = true;
    }

    return S_OK;
}

bool
PivotCurveGenerator::AreCurvesDirty(bool usingDynamicHeaders)
{
    PVTRACE(L"[PCG]: GetCurvesDirty: m_curvesDirty: %d", m_curvesDirty);
    return m_curvesDirty || 
           (usingDynamicHeaders && abs(m_currentOffset - m_lastCurveCenterpoint) > m_sectionWidth * c_overrun);
}

void PivotCurveGenerator::GetDynamicCurveSegments(
    _Inout_ std::vector<DOUBLE>& primaryOffsets,
    _Inout_ std::vector<DOUBLE>& secondaryOffsets,
    // We use this to infer the total viewport width. See PivotPanel for
    // details on how this is used in the measure pass.
    _In_ const unsigned panelMultiplier)
{
    FlushPendingHeaderItemWidthChange();

    const UINT itemCount = static_cast<UINT>(m_currentHeaderItemWidths.size());
    // We handle the degenerate case specially, the header curve definition
    // function implemented imperatively below isn't defined for itemCounts of 0.
    if (itemCount == 0)
    {
        const UINT offsetCount = 2;
        primaryOffsets.reserve(offsetCount);
        secondaryOffsets.reserve(offsetCount);

        primaryOffsets.push_back(0);
        primaryOffsets.push_back(m_currentOffset * 2);
        secondaryOffsets.push_back(m_currentOffset);
        secondaryOffsets.push_back(m_currentOffset);
    }
    else
    {
        const UINT visualOffsetIdx = static_cast<UINT>(m_currentOffset / m_sectionWidth + 0.5);
        const UINT baseIdx = (visualOffsetIdx - m_currentIdx) % itemCount;
        const UINT repeatingGroupCount = (visualOffsetIdx - baseIdx - m_currentIdx) / itemCount;
        const UINT itemGroupCount = 1 + c_overrun * 2;
        const DOUBLE totalSectionItemSize = m_sectionWidth * itemCount;
        // The extra three items are the start point, the trailing point 
        // of the last collection, and the end point.
        const UINT offsetCount = 3 + itemCount * itemGroupCount;
        primaryOffsets.reserve(offsetCount);
        secondaryOffsets.reserve(offsetCount);

        // Build up the primary offsets. Since these are just the positions
        // of the section items they are equally spaced. We skip the first point
        // and the last point because they are the ends of the ScrollViewer
        // primary content. 
        {
            // Terminal points of the primary offsets are straightforward.
            primaryOffsets.push_back(0);
            DOUBLE offsetPosition = totalSectionItemSize * (repeatingGroupCount - c_overrun) + baseIdx * m_sectionWidth;
            for (UINT primaryItemIdx = 1; primaryItemIdx < offsetCount - 1; primaryItemIdx++)
            {
                primaryOffsets.push_back(offsetPosition);
                offsetPosition += m_sectionWidth;
            }
            primaryOffsets.push_back(m_sectionWidth * panelMultiplier);
        }

        // Build the secondary offsets. Offsets are spaced at sectionWidth distance,
        // minus the width of the section to create the correct travel needed for the
        // headers to move at a slower speed.
        {
            DOUBLE offsetPosition = (totalSectionItemSize - m_currentHeaderItemWidthTotalSize) * (repeatingGroupCount - c_overrun);
            for (UINT itemIdx = 0; itemIdx < baseIdx; itemIdx++)
            {
                offsetPosition += m_sectionWidth - m_currentHeaderItemWidths[itemCount - itemIdx - 1];
            }

            DOUBLE expectedCurveOffset = offsetPosition;
            expectedCurveOffset += c_overrun * (totalSectionItemSize - m_currentHeaderItemWidthTotalSize);
            for (INT itemIdx = 0; itemIdx < m_currentIdx; itemIdx++)
            {
                expectedCurveOffset += m_sectionWidth - m_currentHeaderItemWidths[itemIdx];
            }
            const DOUBLE correctionOffset = m_currentParametricCurveOffset - expectedCurveOffset;

            offsetPosition += correctionOffset;

            secondaryOffsets.push_back(-offsetPosition);
            for (INT itemGroup = 0; itemGroup < itemGroupCount; itemGroup++)
            {
                for (UINT i = 0; i < itemCount; i++)
                {
                    secondaryOffsets.push_back(-offsetPosition);
                    offsetPosition += m_sectionWidth - m_currentHeaderItemWidths[i];
                }
            }
            secondaryOffsets.push_back(-offsetPosition);
            secondaryOffsets.push_back(-offsetPosition);
        }
    }

    m_curvesDirty = false;
    m_lastCurveCenterpoint = m_currentOffset;
}

void PivotCurveGenerator::GetStaticCurveSegments(
    _Inout_ std::vector<DOUBLE>& primaryOffsets,
    _Inout_ std::vector<DOUBLE>& secondaryOffsets,
    _In_ const float viewportSize)
{
    FlushPendingHeaderItemWidthChange();    

    const unsigned itemCount = static_cast<unsigned>(m_currentHeaderItemWidths.size());
    double accumulatedItemWidths = 0.0;

    primaryOffsets.reserve(itemCount + 2);
    secondaryOffsets.reserve(itemCount + 2);
    
    primaryOffsets.push_back(0.0);
    secondaryOffsets.push_back(0.0);

    for (unsigned i = 0; i < itemCount; ++i)
    {
        const double a = accumulatedItemWidths;         // amount of space at the left of the current item.
        const double b = m_currentHeaderItemWidths[i];  // width of the current item.
        const double c = std::max(m_currentHeaderItemWidthTotalSize - (a + b), 0.0);  // amount of space at the right of the current item.

        primaryOffsets.push_back(m_sectionWidth * i);
        secondaryOffsets.push_back(GetHeaderShiftOffset(a, b, c, viewportSize));
        accumulatedItemWidths += m_currentHeaderItemWidths[i];
    }

    primaryOffsets.push_back(primaryOffsets[itemCount]);
    secondaryOffsets.push_back(secondaryOffsets[itemCount]);

    m_curvesDirty = false;
    m_lastCurveCenterpoint = m_currentOffset;
}

/*static*/
double PivotCurveGenerator::GetHeaderShiftOffset(
    double a, // amount of space at the left of the current item.
    double b, // width of the current item.
    double c, // amount of space at the right of the current item.
    double v) // viewport size
{
    // This calculation aims to keep the proportions of what's visible before/after the selected item.
    // That's why we divide by a + c (the total space available before/after the selected item) after
    // multiplying by the viewport size that's not claimed by the selected item (v - std::min(b, v)).
    // A more comprehensive explanation can be found here:
    // https://microsoft.sharepoint.com/teams/specstore/Developer%20Platform%20Team%20DEP/Redstone/Scrollable%20Static%20Pivot%20Headers%20dev%20design.docx?web=1
    return a + c == 0.0 ? 0.0 : (std::max(a - a * (v - std::min(b, v)) / (a + c), 0.0));
}

void PivotCurveGenerator::FlushPendingHeaderItemWidthChange()
{
    if (m_pendingHeaderItemWidthChange)
    {
        m_currentHeaderItemWidths = std::move(m_pendingHeaderItemWidths);
        m_currentHeaderItemWidthTotalSize = m_pendingHeaderItemWidthTotalSize;
        m_pendingHeaderItemWidthChange = false;
    }
}

} } } } XAML_ABI_NAMESPACE_END
