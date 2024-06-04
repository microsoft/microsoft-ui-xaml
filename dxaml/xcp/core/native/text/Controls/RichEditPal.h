// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Core/windows abstractions (in the spirit of PalTypes.h) of RichEdit
//      types.

#pragma once
#ifndef RICHEDIT_PAL
#define RICHEDIT_PAL

//---------------------------------------------------------------------------
//
// CHARFORMAT
//
//---------------------------------------------------------------------------
struct WCHARFORMAT
{
    XUINT32     Size;
    XUINT32     Mask;
    XUINT32     Effects;
    XINT32      Height;
    XINT32      Offset;
    XUINT32     TextColor;
    XBYTE       CharSet;
    XBYTE       PitchAndFamily;
    WCHAR       FaceName[32];
};

//---------------------------------------------------------------------------
//
// CHARFORMAT2
//
//---------------------------------------------------------------------------
#ifdef __cplusplus
struct WCHARFORMAT2 : WCHARFORMAT
{
    XINT16      Weight;             // Font weight (LOGFONT value)
    XINT16      Spacing;            // Amount to space between letters
    XUINT32     BackColor;          // Background color
    XUINT32     Lcid;               // Locale ID
    XUINT32     Reserved;           // Reserved. Must be 0
    XINT16      Style;              // Style handle
    XINT16      Kerning;            // Twip size above which to kern char pair
    XBYTE       UnderlineType;      // Underline type
    XBYTE       Animation;          // Animated text like marching ants
    XBYTE       RevAuthor;          // Revision author index
};
#endif // __cplusplus

//---------------------------------------------------------------------------
//
// PARAFORMAT
//
//---------------------------------------------------------------------------
struct XPARAFORMAT
{
    XUINT32 Size;
    XUINT32 Mask;
    XINT16  Numbering;
    XINT16  Effects;
    XINT32  StartIndent;
    XINT32  RightIndent;
    XINT32  Offset;
    XINT16  Alignment;
    XINT16  TabCount;
    XINT32  Tabs[32];
};

//---------------------------------------------------------------------------
//
// PARAFORMAT2
//
//---------------------------------------------------------------------------
#ifdef __cplusplus
struct XPARAFORMAT2 : XPARAFORMAT
{
    XINT32  SpaceBefore;           // Vertical spacing before para
    XINT32  SpaceAfter;            // Vertical spacing after para
    XINT32  LineSpacing;           // Line spacing depending on Rule
    XINT16  Style;                 // Style handle
    XBYTE   LineSpacingRule;       // Rule for line spacing (see tom.doc)
    XBYTE   OutlineLevel;          // Outline level
    XINT16  ShadingWeight;         // Shading in hundredths of a per cent
    XINT16  ShadingStyle;          // Nibble 0: style, 1: cfpat, 2: cbpat
    XINT16  NumberingStart;        // Starting value for numbering
    XINT16  NumberingStyle;        // Alignment, roman/arabic, (), ), ., etc.
    XINT16  NumberingTab;          // Space bet FirstIndent & 1st-line text
    XINT16  BorderSpace;           // Border-text spaces (nbl/bdr in pts)
    XINT16  BorderWidth;           // Pen widths (nbl/bdr in half pts)
    XINT16  Borders;               // Border styles (nibble/border)
};
#endif // __cplusplus

#endif // RICHEDIT_PAL
