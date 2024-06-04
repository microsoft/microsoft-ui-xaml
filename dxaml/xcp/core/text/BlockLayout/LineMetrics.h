// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    class TextLineBreak;
    class TextLine;
}

//---------------------------------------------------------------------------
//
//  LineMetrics
//
//  Stores formatting results for one line of text.
//
//---------------------------------------------------------------------------
struct LineMetrics
{
    XUINT32 FirstCharIndex; // TODO: could get this value from the line break
    XUINT32 Length;
    XRECTF Rect;
    XFLOAT VerticalAdvance;
    XFLOAT BaselineOffset;
    RichTextServices::TextLineBreak *LineBreak;
    RichTextServices::TextLine *Line;
    bool HasMultiCharacterClusters;
};
