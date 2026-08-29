// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//------------------------------------------------------------------------
//  Summary:
//      Constructor
//------------------------------------------------------------------------
CLineMetricsCache::CLineMetricsCache()
{
    m_layoutBox.X = 0.0f;
    m_layoutBox.Y = 0.0f;
    m_layoutBox.Width = 0.0f;
    m_layoutBox.Height = 0.0f;

    m_length = 0;
    m_newlineLength = 0;
    m_baseline = 0.0f;

    m_hasMultiCharacterClusters = FALSE;
}

//------------------------------------------------------------------------
//  Summary:
//      Set
//------------------------------------------------------------------------
void CLineMetricsCache::Set(
         _In_ XRECTF  layoutBox,
         _In_ XUINT32 length,
         _In_ XUINT32 newlineLength,
         _In_ XFLOAT  baseline,
         _In_ bool   hasMultiCharacterClusters
        )
{
    m_layoutBox                 = layoutBox;
    m_length                    = length;
    m_newlineLength             = newlineLength;
    m_baseline                  = baseline;
    m_hasMultiCharacterClusters = hasMultiCharacterClusters;
}

//------------------------------------------------------------------------
//  Summary:
//      Get Layout Box for line
//------------------------------------------------------------------------
XRECTF CLineMetricsCache::GetLayoutBox() const
{
    return m_layoutBox;
}

//------------------------------------------------------------------------
//  Summary:
//      Get length of line
//------------------------------------------------------------------------
XUINT32 CLineMetricsCache::GetLength() const
{
    return m_length;
}

//------------------------------------------------------------------------
//  Summary:
//      Get the number of characters used by any newline.
//      Always returns 0, 1 or 2.
//------------------------------------------------------------------------
XUINT32 CLineMetricsCache::GetNewlineLength() const
{
    return m_newlineLength;
}

//------------------------------------------------------------------------
//  Summary:
//      Get the baseline of the line.
//------------------------------------------------------------------------
XFLOAT CLineMetricsCache::GetBaseline() const
{
    return m_baseline;
}

//------------------------------------------------------------------------
//  Summary:
//      SetOffset
//------------------------------------------------------------------------
void CLineMetricsCache::SetOffset(
               _In_ XPOINTF alignmentOffset
               )
{
    m_layoutBox.X = alignmentOffset.x;
    m_layoutBox.Y = alignmentOffset.y;
}

//------------------------------------------------------------------------
//  Summary:
//      Returns whether the line contains multi-character clusters.
//      See TextLine::HasMultiCharacterClusters.
//------------------------------------------------------------------------
_Check_return_ bool CLineMetricsCache::HasMultiCharacterClusters() const
{
    return m_hasMultiCharacterClusters;
}
