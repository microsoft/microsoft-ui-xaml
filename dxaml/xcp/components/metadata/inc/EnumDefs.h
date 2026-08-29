// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Central storage for enum typedefs only

#pragma once

// Keep in sync with DirectUI::InputScopeNameValue.
enum InputScopeNameValue : uint8_t
{
    InputScopeNameValueDefault = 0,
    InputScopeNameValueUrl = 1,
    InputScopeNameValueEmailSmtpAddress = 5,
    InputScopeNameValuePersonalFullName = 7,
    InputScopeNameValueCurrencyAmountAndSymbol = 20,
    InputScopeNameValueCurrencyAmount = 21,
    InputScopeNameValueDateMonthNumber = 23,
    InputScopeNameValueDateDayNumber = 24,
    InputScopeNameValueDateYear = 25,
    InputScopeNameValueDigits = 28,
    InputScopeNameValueNumber = 29,
    InputScopeNameValuePassword = 31,
    InputScopeNameValueTelephoneNumber = 32,
    InputScopeNameValueTelephoneCountryCode = 33,
    InputScopeNameValueTelephoneAreaCode = 34,
    InputScopeNameValueTelephoneLocalNumber = 35,
    InputScopeNameValueTimeHour = 37,
    InputScopeNameValueTimeMinutesOrSeconds = 38,
    InputScopeNameValueNumberFullWidth = 39,
    InputScopeNameValueAlphanumericHalfWidth = 40,
    InputScopeNameValueAlphanumericFullWidth = 41,
    InputScopeNameValueHiragana = 44,
    InputScopeNameValueKatakanaHalfWidth = 45,
    InputScopeNameValueKatakanaFullWidth = 46,
    InputScopeNameValueHanja = 47,
    InputScopeNameValueHangulHalfWidth = 48,
    InputScopeNameValueHangulFullWidth = 49,
    InputScopeNameValueSearch = 50,
    InputScopeNameValueFormula = 51,
    InputScopeNameValueSearchIncremental = 52,
    InputScopeNameValueChineseHalfWidth = 53,
    InputScopeNameValueChineseFullWidth = 54,
    InputScopeNameValueNativeScript = 55,
    InputScopeNameValueText = 57,
    InputScopeNameValueChat = 58,
    InputScopeNameValueNameOrPhoneNumber = 59,
    InputScopeNameValueEmailNameOrAddress = 60,
    InputScopeNameValueMaps = 62,
    InputScopeNameValueNumericPassword = 63,
    InputScopeNameValueNumericPin = 64,
    InputScopeNameValueAlphanumericPin = 65,
    InputScopeNameValueFormulaNumber = 67,
    InputScopeNameValueChatWithoutEmoji = 68,
};

//------------------------------------------------------------------------
//
//  Enums for AutomationPeer
//
//  Synopsis:
//      The set of Enums defined by AutomationPeer
//      Shared with Control, thus placed in own file
//
//------------------------------------------------------------------------
#include "UIAEnums.h"

#include "EnumDefs.g.h"

//------------------------------------------------------------------------
// UIElement enums
//------------------------------------------------------------------------

enum MouseCursor : uint8_t
{
    MouseCursorDefault = 0,
    MouseCursorArrow,
    MouseCursorHand,
    MouseCursorWait,
    MouseCursorIBeam,
    MouseCursorStylus,
    MouseCursorEraser,
    MouseCursorSizeNS,
    MouseCursorSizeWE,
    MouseCursorSizeNESW,
    MouseCursorSizeNWSE,
    MouseCursorNone,
    MouseCursorUnset, // special value for resetting cursor state
    MouseCursorPin,
    MouseCursorPerson
};

//------------------------------------------------------------------------
// FrameworkElement enums
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Enum:  TextGravity
//
//  Disambiguates the up to four places at which a given text position may be shown as a cursor on
//  screen.
//
//  The scenario that has the most possible alternate display positions is when the inter-character
//  position is a line break determined by breaking between two normal backing store characters,
//  such as after the hyphen in a hyphenated word, and when the word that's broken has the opposite 
//  flow direction to the paragraph.
//
//  There are 4 physical positions corresponding to this inter-character backing store position:
//
//  1)  On the trailing edge of the first character. Because the word is against the flow, this will
//      be in the middle of the first line.
//  2)  At the end of the first line.
//  3)  At the beginning of the second line.
//  4)  On the leading edge of the second character. Because he word is against the flow, this will
//      be in the middle of the second line.
//
//  Which of these displayed positions is shown depends on how the cursor got to this backing store
//  position: by using cursor keys, by using the cursor keys while selecting, by clicking in the 
//  margin, by pressing the home or end keys, by typing, by backspacing or deleting.
//
//  If we don't represent all 4 cases we'll find that over time we keep adding hacks to fix odd
//  behavior each time someone comes across it, often breaking other behavior that was working well
//  before.
//
//  (Note that although I referred to a hyphenated word, it is just as important in the common case
//  of breaking after space.)
//
//  Also see TextView.cpp CTextView::TextPositionToPixelPosition. (Note, the comments refer to
//  separate flags for line gravity and direction - the since have become one enum with line and
//  character direction.)
//
//  Here are some examples. In all cases the user has just pressed a key which has moved us into a
//  backing store position between lines, between the space at the end of one line and the first 
//  character of the first word of the next line.
//
//  Imagine the following left to right paragraph where uppercase latters represent RTL characters.
//  The backing store contains "abc DEF GHI jkl" and we've broken between the second space and the 
//  "G". (abc and jkl are English words, and DEF and GHI are Arabic words).
//
//  The text displays like this:
//      abc FED
//      IHG jkl
//
//  =========================================================
//  Result: LineBackward/CharacterBackward
//  Cursor forward (e.g. right arrow in an LTR para)
//              From
//                  abc F|ED
//                  IHG jkl
//              to
//                  abc |FED
//                  IHG jkl
//
//  The cursor should move to the trailing edge of the character crossed, in this case the "F" (last
//  character of the line).
//  LineBackward because the cursor must stay on the line before the linebreak.
//  CharacterBackward because the cursor must appear on the trailing edge of the last character of 
//  the line, not at the end of the whole line.
//
//  =========================================================
//  Result: LineBackward/CharacterForward
//  End (or click in the right margin)
//              From
//                  abc F|ED
//                  IHG jkl
//              to
//                  abc FED|
//                  IHG jkl
//
//  The cursor should move to the end of the whole line.
//  LineBackward because the cursor must stay on the line before the linebreak.
//  CharacterForward because the cursor must appear at the end of the whole line, not the trailing 
//  edge of the last character.
//  =========================================================
//  Result: LineForward/CharacterForward
//  Cursor backward (e.g. left arrow in an LTR para)
//              From
//                  abc FED
//                  IH|G jkl
//              to
//                  abc FED
//                  IHG| jkl
//
//  The cursor should move to the leading edge of the character crossed, in this case the "G" (first 
//  character of the line).
//  LineForward because the cursor must stay on the line after the linebreak.
//  CharacterBackward because the cursor must appear on the leading edge of the first character of
//  the line, not at the leading margin.
//  =========================================================
//  Result:LineForward/CharacterBackward
//  Home (or click in the left margin)
//              From
//                  abc FED
//                  IH|G jkl
//              to
//                  abc FED
//                  |IHG jkl
//  The cursor should move to the leading edge of the whole line.
//  LineForward because the cursor must stay on the line after the linebreak.
//  CharacterBackward because the cursor must appear at the end of line, not attached to the 
//  character before the end of line.
//
//  The enum naming refers to the direction in backing store to look to find the edge we're 
//  interested in. First which line then which character. When we're not at a line end, the 
//  character direction is enough and tells us whether to attach to the trailing edge of the 
//  previous character, or the leading edge of the next character.
//
//  Notice that cursor movement keys always generate equal line and character directions.
//
//  Opposite line and character directions occur when the cursor is between lines, and must appear 
//  at the end of or beginning of a whole line.
//------------------------------------------------------------------------
enum TextGravity : uint8_t {
    // The text gravity as flags:
    LineBackward      = 1,
    CharacterBackward = 2,
    // The four possible values explicitly:
    LineForwardCharacterForward   = 0, // E.g. selection start
    LineBackwardCharacterForward  = LineBackward, // E.g. end of whole line
    LineForwardCharacterBackward  = CharacterBackward, // E.g. start of whole line
    LineBackwardCharacterBackward = LineBackward|CharacterBackward // E.g. selection end
};

//------------------------------------------------------------------------
//
//  Enum:  TextNestingType
//
//  Distinguishes runs corresponding to nesting level open/close from
//  runs containing text or other content.
//
//------------------------------------------------------------------------

enum TextNestingType : uint8_t
{
    OpenNesting,   // This run opens a new nesting level
    NestedContent, // This run contains content at the current nesting level
    CloseNesting   // This run closes a nesting level
};
