// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Custom types and static utilities for LS client.

#include "precomp.h"
#include "LsSpan.h"
#include "LsRun.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;
using namespace Ptls6;

//---------------------------------------------------------------------------
//
//  Member:
//      HasMultiCharacterClusters
//
//  Synopsis:
//      Helper for LineServicesGetGlyphs, returns TRUE iff a char->glyph map
//      contains entries where more than a single code point maps to any
//      number of glyphs.
//
//  Notes
//      For example, this function returns TRUE when a line contains one or
//      more surrogate pairs or combining characters.
//
//---------------------------------------------------------------------------
bool RichTextServices::Internal::HasMultiCharacterClusters(
    _In_ XUINT32 cwch,
        // Number of characters to be shaped
    _In_ XUINT32 cGlyphs,
        // Number of glyphs representing the characters
    _In_reads_(cwch) PGMAP rggmap
        // Parallel to the char codes mapping characters->glyph info
    )
{
    // Determine whether or not this line has combined code points.
    // If the character count exceeds the number of glyphs, code points are combined.
    bool hasMultiCharacterClusters = cwch > cGlyphs;
    // Otherwise, we need to walk the glyph map to be sure.
    if (!hasMultiCharacterClusters && cwch > 0)
    {
        // We're looking for any case where greater than one code point maps to a run of any
        // number of glyphs, implying combined code points.  A single character mapping to one
        // or many glyphs is uninteresting.  Consequative rggmap entries pointing
        // to the same glyph means combined code points.
        XUINT16 previous = rggmap[0];
        for (XUINT32 i=1; i < cwch; i++)
        {
            if (previous == rggmap[i])
            {
                hasMultiCharacterClusters = TRUE;
                break;
            }
            previous = rggmap[i];
        }
    }

    return hasMultiCharacterClusters;
}

//---------------------------------------------------------------------------
//
//  Member:
//      StopAtEachCharacterInCell
//
//  Synopsis:
//      For a given TextCell and the subline containing it, determines based
//      on script and cell contents, whether a caret stop is required
//      at each character in the cell. For cells with a cluster of
//      characters, depending on the script and characters, the entire
//      cell may be treated as a single caret stop.
//
//---------------------------------------------------------------------------
Result::Enum RichTextServices::Internal::StopAtEachCharacterInCell(
    _In_ PLSQSUBINFO pSublineInfo,
    _In_ LONG sublineCount,
    _In_ LSTEXTCELL textCell,
    _In_ IFontAndScriptServices *pFontAndScriptServices,
    _Out_ bool *pStopAtEveryCharacter
    )
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun = NULL;
    const TextCharactersRun *pTextCharactersRun = NULL;
    const WCHAR *pCharacters = NULL;
    PALText::IScriptAnalyzer *pScriptAnalyzer = NULL;

    // Assume stops at every character, and examine properties.
    bool stopAtEveryCharacter = true;

    IFC_FROM_HRESULT_RTS(pFontAndScriptServices->CreateScriptAnalyzer(&pScriptAnalyzer));

    switch (pSublineInfo->idobj)
    {
        case LsObjectId::TextEmbeddedObject:
            // Currently embedded objects are our only installed handlers. We don't have caret stops inside embedded objects.
            stopAtEveryCharacter = false;
            break;

        default:
        // Other options are text, reverse objects - both LS-owned handlers. Assume LS knows what to do here.
        {
            // If there is <= 1 character in the cell, the caret will have to stop at every code point.
            if (textCell.cCharsInCell > 1)
            {
                XINT32 indexIntoRun = 0;

                // Get the LsRun and TextRun for this cell.
                pLsRun = reinterpret_cast<LsRun *>(pSublineInfo[sublineCount - 1].plsrun);
                IFC_EXPECT_RTS(pLsRun);

                pTextCharactersRun = reinterpret_cast<const TextCharactersRun *>(pLsRun->GetTextRun());
                IFC_EXPECT_RTS(pTextCharactersRun);
                IFC_EXPECT_RTS(pTextCharactersRun->GetLength() > 0);

                indexIntoRun = textCell.cpStartCell - pTextCharactersRun->GetCharacterIndex();
                ASSERT(indexIntoRun < static_cast<XINT32>(pTextCharactersRun->GetLength()));
                pCharacters = pTextCharactersRun->GetCharacters();

                // Check the first character of the text cell for a surrogate.
                if (UnicodeData::CharacterProperties::IsExtendedCharacter(pCharacters[indexIntoRun]))
                {
                    // Never stop between surrogates.
                    stopAtEveryCharacter = false;
                }

                // If we're still stopping at every character, we don't have a surrogate. Check script for non divisible clusters.
                if (stopAtEveryCharacter)
                {
                    bool restrictCaretToClusters = false;
                    IFC_FROM_HRESULT_RTS(pScriptAnalyzer->IsRestrictCaretToClusters(&(pLsRun->GetScriptAnalysis()), &restrictCaretToClusters));
                    if (restrictCaretToClusters)
                    {
                        stopAtEveryCharacter = false;
                    }
                }

                // Finally, check for simple combining marks - characters w/ SimpleMark class are treated similar to surrogates, i.e.
                // as non divisible clusters regardless of script.
                if (stopAtEveryCharacter)
                {
                    for (XINT32 i = 0; i < textCell.cCharsInCell; i++)
                    {
                        // It's possible that textCell spans more than one run.  Check if we've run
                        // out of space in the current run, and if possible move on to the following.
                        if (indexIntoRun + i >= static_cast<XINT32>(pTextCharactersRun->GetLength()))
                        {
                            LsRun *pNextLsRun = pLsRun->GetNext();

                            if (NULL == pNextLsRun ||
                                !pLsRun->HasEqualProperties(pNextLsRun))
                            {
                                break;
                            }

                            pLsRun = pNextLsRun;
                            pTextCharactersRun = reinterpret_cast<const TextCharactersRun *>(pNextLsRun->GetTextRun());
                            IFC_EXPECT_RTS(pTextCharactersRun);
                            pCharacters = pTextCharactersRun->GetCharacters();
                            indexIntoRun = -i;
                        }

                        ASSERT(indexIntoRun + i >= 0);

                        if (UnicodeData::CharacterProperties::IsSimpleCombiningMark(pCharacters[indexIntoRun + i]))
                        {
                            stopAtEveryCharacter = false;
                            break;
                        }
                    }
                }
            }
            break;
        }
    }

    *pStopAtEveryCharacter = stopAtEveryCharacter;

Cleanup:
    ReleaseInterface(pScriptAnalyzer);
    return txhr;
}

