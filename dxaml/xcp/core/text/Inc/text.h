// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Text support utility code

#ifndef __TEXT_H__
#define __TEXT_H__

#include "FontAndScriptServices.h"
#include <Indexes.g.h>

class CString;
class CInlineCollection;

// Keep in sync with /dxaml/lib/resource.h
#define TEXT_CONTEXT_MENU_CUT                   5034
#define TEXT_CONTEXT_MENU_COPY                  5035
#define TEXT_CONTEXT_MENU_PASTE                 5036
#define TEXT_CONTEXT_MENU_UNDO                  5037
#define TEXT_CONTEXT_MENU_REDO                  5038
#define TEXT_CONTEXT_MENU_SELECT_ALL            5039


// Unicode character codes

#define UNICODE_CHARACTER_TABULATION L'\x0009'
#define UNICODE_LINE_FEED            L'\x000A'
#define UNICODE_FORM_FEED            L'\x000C'
#define UNICODE_CARRIAGE_RETURN      L'\x000D'
#define UNICODE_SPACE                L'\x0020'
#define UNICODE_HYPHEN_MINUS         L'\x002D'
#define UNICODE_NEXT_LINE            L'\x0085'
#define UNICODE_SOFT_HYPHEN          L'\x00AD'
#define UNICODE_EN_SPACE             L'\x2002'
#define UNICODE_EM_SPACE             L'\x2003'
#define UNICODE_THIN_SPACE           L'\x2009'
#define UNICODE_ZERO_WIDTH_SPACE     L'\x200B'
#define UNICODE_LEFT_TO_RIGHT_MARK   L'\x200E'
#define UNICODE_RIGHT_TO_LEFT_MARK   L'\x200F'
#define UNICODE_LINE_SEPARATOR       L'\x2028'
#define UNICODE_PARAGRAPH_SEPARATOR  L'\x2029'
#define UNICODE_ELLIPSIS             L'\x2026'
#define UNICODE_IDEOGRAPHIC_SPACE    L'\x3000'


// Portable User Inferface Composite font description

extern const WCHAR pszCompositeFontWin8[];


//------------------------------------------------------------------------
//
//  Method: IsXamlWhitespace
//
//  Returns whether the passed characetr is space, tab, form feed or
//  carriage return.
//
//  Note: line feed and line separator are not classified as whitespace.
//
//------------------------------------------------------------------------

_Check_return_ XINT32 IsXamlWhitespace(XUINT32 character);

//------------------------------------------------------------------------
//
//  Method: IsXamlNewline
//
//  Returns whether the passed character requires breaking the line.
//
//  Returns true for:
//
//      U+000A  LF   Line feed
//      U+000B  VT   Line tabulation
//      U+000C  FF   Form feed
//      U+000D  CR   Carriage return
//      U+0085  NEL  New line
//      U+2028  LS   Line separator, aka LSEP
//      U+2029  PS   Paragraph separator, aka PSEP
//
//      This test is used in a number of tight character loops, so great
//      performance is vital.
//
//      Since most characters are not a form of newline, the test is optimized to
//      fail as quickly as possible.
//
//      Bit patterns recognised:
//
//      0000 0000 0000 1010
//      0000 0000 0000 1011
//      0000 0000 0000 1100
//      0000 0000 0000 1101
//      0000 0000 1000 0101
//      0010 0000 0010 1000
//      0010 0000 0010 1001
//      -------------------
//      00x0 0000 x0x0 xxxx
//
//      Steps:
//
//      1. Test that all the non-x bits are as expected: (ch & df50) == 0
//         This conveniently rejects the vast majority of characters,
//         including all printable ASCII characters except space.
//      2. CR, LF, VT and FF are sequential. Test that ch-000a < 4.
//      3. LS & PS are sequential. Test that ch-2028 < 2.
//      4. Finally test for NL.
//
//------------------------------------------------------------------------

_Check_return_ XINT32 IsXamlNewline(XUINT32 character);

//------------------------------------------------------------------------
//
//  Method: IsXamlBlockSeparator
//
//  Returns whether the passed character is to be considered one that separates blocks/paragraphs
//
//
//------------------------------------------------------------------------

_Check_return_ XINT32 IsXamlBlockSeparator(XUINT32 character);


//------------------------------------------------------------------------
//
//  Method:   LanguageToLCID
//
//  Synopsis: Conver a language string containing a xml:lang tab into a LCID
//
//------------------------------------------------------------------------

void
LanguageToLCID(
    _In_ XUINT32 cLanguageString,
    _In_reads_(cLanguageString) const WCHAR *pLanguageString,
    _Out_ XUINT32 *pLanguage
    );


//------------------------------------------------------------------------
//
//  Method:   CStringToLCID
//
//  Synopsis: Convert a CString containing an xml:lang tag into a LCID
//
//------------------------------------------------------------------------

_Check_return_ XUINT32 CStringToLcid(_In_ CString *pString);
_Check_return_ XUINT32 XStringPtrToLCID(_In_ const xstring_ptr& strString);

//------------------------------------------------------------------------
//
//  Method:   ValidateXmlLanguage
//
//  Synopsis:
//      Validate that the XmlLanguage string conform to RFC 3066
//
//------------------------------------------------------------------------

bool ValidateXmlLanguage(_In_ const CValue *pValue);

//------------------------------------------------------------------------
//
//  Method:   UseHighContrastSelection
//
//  Synopsis:
//
//  Determines whether selection should be rendered in high contrast mode
//  based on current theme. In high contrast mode theme colors are used for
//  selected text foreground and selection highlight and highlight is rendered
//  before content. Otherwise, selected text has the same foreground as
//  regular text, highlight is rendered semi transparent and over text.
//  Modes/Behavior:
//      HighContrast 1/2/Black/White: High contrast selection.
//      Anything else: Regular selection.
//
//------------------------------------------------------------------------
bool UseHighContrastSelection(
    _In_ CCoreServices *pCore
    );

_Check_return_ HRESULT CopyClearTypeOverscaleBitmap(
    _In_                               XUINT32  uOverscaleY,
    _Inout_                            XINT32  *puOriginY,
    _In_                               XUINT32  uWidth,
    _Inout_                            XUINT32 *puHeight,
    _In_                               XUINT32  uStride,
    _In_reads_bytes_((*puHeight * uStride)) XUINT8  *pSrcBits,
    _Outptr_                        XUINT8 **ppDstBits
);

_Check_return_ HRESULT CompressInlinesWhitespace(_Inout_opt_ CInlineCollection *pInlines);




//------------------------------------------------------------------------
//
//  Enum:  EditRectangleKind
//
//  Synopsis:
//      Used to select the kind of pixel snapping to be used on rectangles
//      in the Silverlight edit controls TextBox and RichTextBox.
//
//------------------------------------------------------------------------

enum EditRectangleKind {
    SelectionRectangle,
    MultiSelectionRectangle,
    CaretRectangle,
    FocusRectangle
};


bool IsForegroundPropertyIndex(KnownPropertyIndex propertyIndex);

#endif // ifdef __TEXT_H__

