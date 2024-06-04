// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef _LINE_METRICS_CACHE_H
#define _LINE_METRICS_CACHE_H

namespace RichTextServices
{
    class TextLine;
};

//---------------------------------------------------------------------------
//
//  Line Metrics Cache
//
//     Holds cached information about lines that were laid out for
//     display after an arrange pass.
// 
//     This information can be used for speeding up hit testing/incremental
//     layout.
//
//---------------------------------------------------------------------------
struct CLineMetricsCache
{
public:

    //------------------------------------------------------------------------
    //  Summary:
    //      Constructor
    //------------------------------------------------------------------------
    CLineMetricsCache();

    //------------------------------------------------------------------------
    //  Summary:
    //      Sets primary properties of the cache.
    //------------------------------------------------------------------------
    void Set(
        _In_ XRECTF  layoutBox,
        _In_ XUINT32 length,
        _In_ XUINT32 newlineLength,
        _In_ XFLOAT  baseline,
        _In_ bool   hasMultiCharacterClusters
        );
 
    //------------------------------------------------------------------------
    //  Summary:
    //      Get layout box.
    //------------------------------------------------------------------------
    XRECTF GetLayoutBox() const;

    //------------------------------------------------------------------------
    //  Summary:
    //      Get length of line.
    //------------------------------------------------------------------------
    XUINT32 GetLength() const;

    //------------------------------------------------------------------------
    //  Summary:
    //      Get the number of characters used by any newline.
    //      0: Line has no newline, for example broken on a hyphen.
    //      1: Line end with a one character newline, such as LF.
    //      2: Line ends with a two character newline, such as CR/LF.
    //------------------------------------------------------------------------
    XUINT32 GetNewlineLength() const;

    //------------------------------------------------------------------------
    //  Summary:
    //      Get the baseline of the line.
    //------------------------------------------------------------------------
    XFLOAT GetBaseline() const;

    //------------------------------------------------------------------------
    //  Summary:
    //      Set alignment offset.
    //------------------------------------------------------------------------
    void SetOffset(
        _In_ XPOINTF alignmentOffset
        );

    _Check_return_ bool HasMultiCharacterClusters() const;

private:

    // layout box for the line
    XRECTF                      m_layoutBox;

    // length of characters in the line
    XUINT32                     m_length;

    // Number of characters taken by any trailing newline
    XUINT32                     m_newlineLength;

    XFLOAT                      m_baseline;

    bool                       m_hasMultiCharacterClusters;
};

#endif
