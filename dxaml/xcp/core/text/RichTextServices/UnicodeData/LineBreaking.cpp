// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LineBreaking.h"

namespace RichTextServices
{
//----------------------------------------------------------------------------------
// Strategy for leveraging DWrite line break information in Line Services
// ---------------------------------------------------------------------------------
// 
// DWrite line breakpoint analysis returns an array of DWRITE_LINE_BREAKPOINT 
// (aka PALText::LineBreakpoint) structs with break information for each character. 
// In reality the break information is about each *pair* of characters, and so the
// information about a single character tells whether you can break before or after
// the character.  For any pair of characters, the first's breakConditionAfter will
// match the second's breakConditionBefore.  This may seem redundant, but it turns 
// out to be useful.
//
// Line Services up front wants from us the raw breaking tables -- a BreakingTable 
// where each entry is an index into the BreakingInfo.  This is because LS was 
// designed for us to give them the unicode data tables and they expect to apply 
// something like the Unicode Line Breaking Algorithm (http://www.unicode.org/reports/tr14).
// (In fairness, they were doing line breaking before the ULBA existed).
//
// However, this has some distinct disadvantages for us. First is that carrying along
// the unicode data tables requires that we bundle them in our dll (non-trivial 
// binary size cost) and keep them up to date (extra maintenance overhead).  Second
// is that there are customizations in the ULBA that Line Services does not do, such
// as proper line breaking for Thai (which requires a dictionary to line break).
//
// Leveraging DWrite simplifies things for us, and returning the information in a
// form that LS expects is fairly straightforward.
//
// There are a few things to know about what LS asks us about:
//
// * LS will call us with GetBreakingClasses to query information about character
//   positions where it wants to know about break opportunities.  We return to LS
//   a "breaking class" for the break choice between this character and the previous 
//   one ("pbrkclsFirst") and the break choice between this character and the next
//   one ("pbrkclsSecond").  Since DWrite has already done the heavy lifting, these
//   break classes are simply "is a break allowed between these two chars?".
//
// * LS won't call us for whitespace. They expect that they can make the decision
//   for us based on the breaking class of the trailing character's BreakingInfo
//   (which says fBreak and fBreakAcrossSpaces).  This is ok, we already knew if
//   a particular character was after a space (our breaking classes are very 
//   contextual, obviously) so NoBreak doesn't break across spaces and BreakAllowed
//   does break across spaces.
//
// * LS will call us back with CanBreakBeforeChar/CanBreakAfterChar to translate
//   a breaking class to a break condition in the case that the character was
//   at the beginning or end of a run where LS doesn't know the adjacent character.
//   Again, because we gave all the text to DWrite we already know the answer
//   about the adjacent character so the breaking class can be directly translated
//   into an LS break condition.
//
// * An LS break condition is one of the following:
//   - "Can"   : Neutral break.  Break only happens if adjacent is "Please", but not with another "Can".
//   - "Please": Break allowed with adjacent, as long as adjacent is "Can" or "Please".
//   - "Never" : Break never allowed even with adjacent "Please".
//   - "Must"  : Always break regardless of adjacent.  LS strongly recommends NOT using this, so we do not.
//
// * We map DWrite's "can break" condition to "Please" and "don't break" condition
//   to "Never".  DWrite sometimes returns a "must break" (like before "\n") but
//   with how we use LS, we've already broken runs on these must break boundaries.
//   So in most cases we only see "can break" but, just in case, we map "must break"
//   to "Please" as well.
//
// * Inline objects don't participate in the breaking *class* scheme; they only get 
//   to return an LS break condition.  In general we would like inline objects to
//   allow breaks before and after.  It's easy for an app to rescind the break opportunity 
//   with a Word Joiner or other "Glue" character, but this is uncommon.  When asking 
//   DWrite to analyze breakpoints we give character U+FFFC ("object replacement character")
//   and so our line break information for text adjacent to inline objects is really 
//   where the decision about breaks with inline objects comes from.  Inline objects 
//   are populated with breakBefore/breakAfter conditions that come from the text around 
//   them during the layout, so in regards to breaking they behave the same as text.
//
// ---------------------------------------------------------------------------------

const Ptls6::LSBRK LineBreaking::s_simpleBreakingInfo[2] = 
{
    {0, 0}, // NoBreak
    {1, 1}, // BreakAllowed
};

// Two classes -- NoBreak, BreakAllowed
const XUINT8 LineBreaking::s_simpleBreakTable[2][2] =
{
      // NB BA
/*NB*/ { 0, 1 },
/*BA*/ { 1, 1 },
};


//-----------------------------------------------------------------------
//
//  Member:
//      LineBreaking::GetDWriteBreakingInfo
//
//  Synopsis:
//      Gets the breaking classes and units information.
//
//-----------------------------------------------------------------------
void LineBreaking::GetDWriteBreakingInfo(
    _Out_ XUINT32 *pBreakUnitsCount,
        // Number of breaking units
    _Out_ XUINT32 *pBreakClassesCount,
        // Number of breaking classes
    _Outptr_result_buffer_(*pBreakUnitsCount) const Ptls6::LSBRK **ppBreakUnitsInfo,
        // Array of breaking unit information
    _Outptr_result_buffer_((*pBreakClassesCount) * (*pBreakClassesCount)) const XUINT8 **ppBreakClassesInfo
        // Square array mapping breaking class pairs to breaking units
    )
{
    *pBreakUnitsCount   = ARRAY_SIZE(s_simpleBreakingInfo);
    *pBreakClassesCount = ARRAY_SIZE(s_simpleBreakTable);
    *ppBreakUnitsInfo   = s_simpleBreakingInfo;
    *ppBreakClassesInfo = reinterpret_cast<XUINT8 const*>(s_simpleBreakTable);
}

//---------------------------------------------------------------------------
//
//  Member:
//      DWriteBreakToSimpleBreakClass
//
//  Synopsis:
//      Helper to translate a break condition from DWrite into a breaking
//      class for LS.  See LineBreaking.cpp for more information.
//
//---------------------------------------------------------------------------

Ptls6::BRKCLS LineBreaking::DWriteBreakToSimpleBreakClass(XUINT8 dwriteBreakCondition)
{
    switch (dwriteBreakCondition)
    {
        case PALText::LineBreakingCondition::CanBreak:
        case PALText::LineBreakingCondition::MustBreak:
            return (Ptls6::BRKCLS)SimpleBreakClass::BreakAllowed;

        case PALText::LineBreakingCondition::Neutral:
        case PALText::LineBreakingCondition::MayNotBreak:
            return (Ptls6::BRKCLS)SimpleBreakClass::NoBreak;
            
        default:
            ASSERT(false);
            return (Ptls6::BRKCLS)SimpleBreakClass::NoBreak;
    }
}


//---------------------------------------------------------------------------
//
//  Member:
//      SimpleBreakClassToLSBrkCond
//
//  Synopsis:
//      Translate a simple break class to a break condition.
//
//---------------------------------------------------------------------------

Ptls6::BRKCOND LineBreaking::SimpleBreakClassToLSBrkCond(Ptls6::BRKCLS brkcls)
{
    return (brkcls == (Ptls6::BRKCLS)SimpleBreakClass::BreakAllowed) 
                    ? Ptls6::brkcondPlease : Ptls6::brkcondNever;
}

//---------------------------------------------------------------------------
//
//  Member:
//      DWriteBreakToLSBrkCond
//
//  Synopsis:
//      Helper to translate a break condition from DWrite into a break
//      condition for LS.
//
//---------------------------------------------------------------------------

Ptls6::BRKCOND LineBreaking::DWriteBreakToLSBrkCond(XUINT8 dwriteBreakCondition)
{
    return SimpleBreakClassToLSBrkCond(DWriteBreakToSimpleBreakClass(dwriteBreakCondition));
}

}