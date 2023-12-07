// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LineServicesCallbacks.h"
#include "TextSource.h"
#include "LsTextFormatter.h"
#include "TextRun.h"
#include "TextRunProperties.h"
#include "TxMath.h"
#include "TextDpi.h"
#include "LsRun.h"
#include "LsSpan.h"
#include "LsRunCache.h"
#include "LsClient.h"
#include "TextStore.h"
#include "XcpAllocation.h"
#include "XcpAllocationDebug.h"

using namespace DirectUI;
using namespace Ptls6;
using namespace RichTextServices;
using namespace RichTextServices::Internal;


//---------------------------------------------------------------------------
//
//  Function:
//      MakeClipRectForGlyphRun
//
//  Synopsis:
//      Convert a glyph rect provided by Line Services (for eltClipped) and
//      turn it into a clip rect to pass to drawing.
//
//---------------------------------------------------------------------------

XRECTF_RB MakeClipRectForGlyphRun(
    _In_ PCLSRECT pRectClip,
    _In_ PCLSPOINT pptRun,
    _In_ XUINT8 bidiLevel
    )
{
    XRECTF_RB rectfClip;

    ASSERT((pRectClip == NULL) || ((pRectClip->top == XINT32_MIN) && (pRectClip->bottom == XINT32_MAX)));

    if ((pRectClip != NULL) && ((pRectClip->left != XINT32_MIN) || (pRectClip->right != XINT32_MAX)))
    {
        //
        // LTR runs will only ever be clipped on the right and RTL ones on the left,
        // but LS passes us clips for both sides.  Ignore the superfluous one.
        if ((bidiLevel & 1) == 0)
        {
            rectfClip.left = XFLOAT_MIN;
            rectfClip.right = TextDpi::FromTextDpi(pRectClip->right - pptRun->x);
        }
        else
        {
            rectfClip.left = TextDpi::FromTextDpi(pRectClip->left - pptRun->x);
            rectfClip.right = XFLOAT_MAX;
        }
        rectfClip.top = XFLOAT_MIN;
        rectfClip.bottom = XFLOAT_MAX;
    }
    else
    {
        rectfClip.left = XFLOAT_MIN;
        rectfClip.top = XFLOAT_MIN;
        rectfClip.right = XFLOAT_MAX;
        rectfClip.bottom = XFLOAT_MAX;
    }

    return rectfClip;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesNewPtr
//
//  Synopsis:
//      Callback used by LS to request memory allocation.
//
//---------------------------------------------------------------------------
__declspec(allocator) void * RichTextServices::LineServicesNewPtr(
    _In_ POLS pols,
        // LS host
    _In_ LONG cSize
        // Number of bytes requested
    )
{
    #if XCP_MONITOR
        return ::XcpDebugAllocate(cSize, AllocationFlat);
    #else
        return XcpAllocation::OSMemoryAllocateFailFast(cSize);
    #endif
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesDisposePtr
//
//  Synopsis:
//      Callback used by LS to release memory.
//
//---------------------------------------------------------------------------
void RichTextServices::LineServicesDisposePtr(
    _In_ POLS pols,
        // LS host
    _In_ void *pAddress
        // Pointer to the memory block to be released
    )
{
    #if XCP_MONITOR
        ::XcpDebugFree(pAddress, AllocationFlat);
    #else
        XcpAllocation::OSMemoryFree(pAddress);
    #endif
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesReallocPtr
//
//  Synopsis:
//      Callback used by LS to reallocate memory.
//
//---------------------------------------------------------------------------
__declspec(allocator) void * RichTextServices::LineServicesReallocPtr(
    _In_ POLS pols,
        // LS host
    _In_ void *pAddress,
        // Pointer to the memory block to be reallocated
    _In_ LONG cSize
        // Number of bytes requested
    )
{
    #if XCP_MONITOR
        return ::XcpDebugResize(pAddress, cSize, AllocationFlat);
    #else
        return XcpAllocation::OSMemoryResize(pAddress, cSize);
    #endif
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesFetchRun
//
//  Synopsis:
//      Callback used by LS to obtain the next run of characters.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesFetchRun(
    _In_ POLS pols,
        // LS host
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph info
    _In_ LSSPAN lsspanMaster,
        // Master span of current subline
    _In_ LSCP cpCurrent,
        // Identifies the position in the document of the first character to fetch.
        // If this is negative it indicates that Line Services is requesting an autonumber run.
    _In_ BOOL fStyle,
        // Flag indicating whether LS is passing the client style information
        // (used for math runs)
    _In_opt_ PLSSTYLE pstyle,
        // Pointer to math style information
    _In_ LSFETCHPOSITION fetchpos,
        // Context provided by LS to disambiguate the fetching position.
    _Out_ LSFETCHRESULT* pfetchres
        // Data representing the run requested by LS.
    )
{
    Result::Enum txhr = Result::Success;
    LsHostContext *pLsHostContext;
    LsRun *pLsRun = NULL;
    LsSpan *pMasterLsSpan = NULL;
    LsSpan *pNewMasterLsSpan = NULL;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    pMasterLsSpan = reinterpret_cast<LsSpan *>(lsspanMaster.lsspq);
    IFC_EXPECT_RTS(pMasterLsSpan);

    IFC_EXPECT_RTS(pLsHostContext->pTextStore);

    // Try to get TextRun from the cache first.

    // If a run is being requested from before the start of the line, it indicates that Line Services
    // is re-opening reverse objects from a previously broken line to restore the appropriate bidi level.
    // LS uses the break record from the previous line to restore these reverse objects.
    // This is the only scenario where this type of request for a run before the start position occurs.
    // In this case, we perform an explicit search for the appropriate opening reverse object from the cache.
    // Note:  This behavior is new for LS6.  See notes below for LS5 behavior.
    if ((XUINT32)cpCurrent < pLsHostContext->lineStartIndex)
    {
        pLsRun = pLsHostContext->pTextStore->GetRunCache()->FindReverseObjectOpen(cpCurrent, pMasterLsSpan);

        // The run must be in our cache, otherwise LS would not have a break record with the cp at this position.
        ASSERT(pLsRun != NULL);
    }
    else if ((XUINT32)cpCurrent == pLsHostContext->lineStartIndex)
    {
        // Note:  This block of code is partially obsolete due to a behavior change that occurred between LS5 and LS6.
        // In LS5, LineServices would never request a run from before the start of the line,
        // instead it would request runs from exactly at the start of the line to re-open reverse objects.
        // This block of code is being left in place as it would introduce more risk to change this code at this
        // point in the Blue cycle.  For Blue + 1, we should consider re-working this code.
        //
        // If we are at the start of the line, spans from the previous line may need to be reopened,
        // if the line was broken in the middle of a span. This happen if a break record was not passed in as input.
        // In this case, walk the span tree starting at the current span and reopen appropriate spans before
        // returning the first significant run (non-zero length).
        // If master spans are in sync, pass NULL for fetchpos to retrieve the first significant run at beginning of the line.
        pMasterLsSpan->GetOpenChildSpan(cpCurrent, &pNewMasterLsSpan);
        if (pNewMasterLsSpan != NULL)
        {
            // Master span has been updated. Fetch the correct open span run for it from the cache.
            pLsRun = pLsHostContext->pTextStore->GetRunCache()->GetLsRun(pNewMasterLsSpan->GetCharacterIndex(), &fetchpos);

            // This run must exist, or the span would not have been updated.
            ASSERT(pLsRun != NULL);
        }
        else
        {
            // Master span has not been updated. Fetch first significant run from cache.
            pLsRun = pLsHostContext->pTextStore->GetRunCache()->GetLsRun(cpCurrent, NULL);
        }
    }
    else
    {
        pLsRun = pLsHostContext->pTextStore->GetRunCache()->GetLsRun(cpCurrent, &fetchpos);
    }

    if (pLsRun == NULL)
    {
        IFCTEXT(pLsHostContext->pTextStore->FetchRun(cpCurrent, pMasterLsSpan, &pLsRun));
    }

    ASSERT(pLsRun != NULL);
    pLsRun->InitializeRunData(cpCurrent, lsspanMaster, pfetchres);

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesFetchTabs
//
//  Synopsis:
//      Callback used by LS to fetch tab properties.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesFetchTabs(
    _In_ POLS pols,
        // LS host.
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph info.
    _Out_ PLSTABS plstabs,
        // Tabs array.
    _Out_ BOOL* pfHangingTab,
        // True if there is a hanging tab.
    _Out_ LONG* pdurHangingTab,
        // Hanging tab width.
    _Out_ WCHAR* pwchHangingTabLeader
        // Hanging tab leader.
    )
{
    Result::Enum txhr = Result::Success;
    LsHostContext *pLsHostContext;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    IFC_EXPECT_RTS(pLsHostContext->pParagraphProperties);

    // Hanging tabs not supported.
    *pfHangingTab = FALSE;
    *pdurHangingTab = 0;
    *pwchHangingTabLeader = '\0';

    // Set default tab width value to default value from para props.
    memset(plstabs, 0, sizeof(LSTABS));
    plstabs->durIncrementalTab = TextDpi::ToTextDpi(pLsHostContext->pParagraphProperties->GetDefaultIncrementalTab());
    plstabs->iTabUserDefMac = 0;
    plstabs->pTab = NULL;

Cleanup:
    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesReleaseTabsBuffer
//
//  Synopsis:
//      Callback used by LS to release buffer for used-defined tab stops.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesReleaseTabsBuffer(
    _In_ POLS pols,
        // LS host.
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph info.
    _In_ LSTBD* plstbd
        // Array of user-defined tab stops.
    )
{
    // Currently we don't support user-defined tab stops.
    ASSERT(plstbd == NULL);
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetBreakThroughTab
//
//  Synopsis:
//      Callback used by LS to adjust right margin in case of a
//      user-defined tab stop past the right margin..
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetBreakThroughTab(
    _In_ POLS pols,
        // LS host.
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph info.
    _In_ LONG urRightMargin,
        // Right margin.
    _In_ LONG urTabPos,
        // Position of user-defined tab stop.
    _Out_ LONG* purRightMarginNew
        // Adjusted right margin accounting for user's tab stop.
    )
{
    // There is a user-defined tab stop beyond the right margin; switch to infinite
    // right margin so as to respect the user's tab stop
    *purRightMarginNew = uLsInfiniteRM;
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetLastLineJustification
//
//  Synopsis:
//      Callback used by LS to determine justification and alignment preferences for the last line in a
//      paragraph.
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetLastLineJustification(
    _In_ POLS pols,
        // LS Host.
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph into.
    _In_ LSKJUST lskj,
        // Justification preference.
    _In_ BOOL fJustify,
        // True if justification is turned on.
    _In_ LSKALIGN lskal,
        // Alignment preference of client paragraph.
    _In_ ENDRES endr,
        // Line end result.
    _Out_ BOOL* pfJustifyLastLine,
        // True if the last line should be justified.
    _Out_ LSKALIGN* plskalLine
        // Alignment preference for last line.
    )
{
    *pfJustifyLastLine = FALSE;

    // Set alignment to input alignment
    *plskalLine = lskal;
    if (endr == endrNormal || endr == endrAutoHyphenated || endr == endrManuallyHyphenated)
    {
        // Check end result. If normal - soft break or hyphenated break -
        // justify as specified. Otherwise, don't justify.
        *pfJustifyLastLine = fJustify;
    }
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetRunCharacterWidths
//
//  Synopsis:
//      Callback used by LS to retrieve the widths for all of
//      the characters of a given run.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetRunCharacterWidths(
    _In_                  POLS     pols,          // LS host
    _In_                  PLSRUN   plsrun,        // Pointer to client run information
    _In_                  LSDEVICE lsdev,         // Presentation or reference device
    _In_reads_(cwchRun)  LPCWSTR  rgwchRun,      // Run of characters
    _In_                  LONG     cwchRun,       // Number of characters in run
    _In_                  LONG     duAvailable,   // Available space for characters
    _In_                  LSTFLOW  lstflow,       // Text direction and orientation
    _Out_writes_(cwchRun) LONG    *rgdu,          // Pointer to array of widths of characters
    _Out_                 LONG    *pduTotal,      // Total of widths returned in rgdu
    _Out_                 LONG    *pcwchConsumed  // Number of character widths fetched
    )
{
    Result::Enum txhr = Result::Success;
    LsRun   *pLsRun;
    XUINT32 *pCodePoints = NULL;
    bool   *pGlyphMap = NULL;
    bool   *pSpaceabilityMap = NULL;
    XUINT16 *pGlyphIndices = NULL;
    XUINT16 *pClusterMap = NULL;
    XINT32  *pGlyphAdvances = NULL;
    XINT32  *pGlyphAdvanceAdjustments = NULL;
    PALText::IGlyphAnalyzer *pGlyphAnalyzer = NULL;
    PALText::ShapingGlyphProperties *pGlyphProps = NULL;
    LsHostContext *pLsHostContext;
    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    IFC_EXPECT_RTS(pLsHostContext->pFontAndScriptServices);
    pLsRun = reinterpret_cast<LsRun *>(plsrun);
    ASSERT(cwchRun > 0 && static_cast<XUINT32>(cwchRun) <= pLsRun->GetTextRun()->GetLength());

    // Character widths are only needed for plain text characters
    if (pLsRun->GetFontFace() != NULL)
    {
        XUINT32                  glyphIndex;
        XUINT32                  charIndex;
        XUINT32                  charCount          = static_cast<XUINT32>(cwchRun);
        XUINT32                  glyphCount         = 0;
        FssFontMetrics           fontMetrics;
        XFLOAT                   scaleFactor;
        XINT32                   totalWidth         = 0;
        XINT32                   glyphExpansion     = 0;
        bool spaceLastCharacter = false;
        const TextCharactersRun *pTextRun;

        // Always return at least one character.
        if (duAvailable < 0)
        {
            duAvailable = 0;
        }

        // Convert UTF16 codepoints to UTF32 codepoints and get corresponding glyph indices.
        // Glyph based runs may include surrogates, so use DecodeUtf16Character in this case.

        IFC_OOM_RTS(pCodePoints   = new XUINT32[charCount]);
        IFC_OOM_RTS(pGlyphIndices = new XUINT16[charCount]);
        if (!pLsRun->IsGlyphBased())
        {
            glyphCount = charCount;
            for (glyphIndex = 0; glyphIndex < charCount; glyphIndex++)
            {
                pCodePoints[glyphIndex] = rgwchRun[glyphIndex];
            }
        }
        else
        {
            IFC_OOM_RTS(pGlyphMap = new bool[charCount]);
            memset(pGlyphMap, TRUE, sizeof(bool) * charCount);
            for (glyphIndex = 0, charIndex = 0; charIndex < charCount; charIndex++, glyphIndex++)
            {
                XUINT32 cCodeUnits;
                XUINT32 character;
                IFC_FROM_HRESULT_RTS(DecodeUtf16Character(rgwchRun, charCount, charIndex, &cCodeUnits, &character));
                pCodePoints[glyphIndex] = character;
                if (cCodeUnits > 1)
                {
                    charIndex++;
                    pGlyphMap[charIndex] = FALSE;
                }
            }
            glyphCount = glyphIndex;
        }
        IFC_FROM_HRESULT_RTS(pLsRun->GetFontFace()->GetGlyphIndices(pCodePoints, glyphCount, pGlyphIndices));

        // Get glyph advances in font design units.
        // Apply also pair-kerning values for character based runs only, if pair-kerning is supported by the font.
        // For glyph based runs this process is unnecessary since appropriate glyph mapping will be done in GetGlyphs callback.

        IFC_OOM_RTS(pGlyphAdvances = new XINT32[glyphCount]);

        ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);
        pTextRun = reinterpret_cast<const TextCharactersRun*>(pLsRun->GetTextRun());
        if (    pTextRun->GetProperties()->GetInheritedProperties()->m_textOptions.GetMeasuringMode() == MeasuringMode::GdiClassic
            &&  pLsRun->GetFontFace()->HasTrueTypeOutlines())  // We only support display mode formatting when we can use our rasterizer.
        {
            CMILMatrix renderTransform;

            // Display glyph advances are only used at axis-aligned transforms.
            renderTransform.SetToIdentity();

            IFC_FROM_HRESULT_RTS(pLsRun->GetFontFace()->GetGdiCompatibleGlyphAdvances(
                pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale(),
                1.0f,   // 1 pixel per dip - pixels = dips in Silverlight
               &renderTransform,
               &pTextRun->GetProperties()->GetInheritedProperties()->m_textOptions,
                lstflow & fVDirection,
                glyphCount,
                pGlyphIndices,
                pGlyphAdvances
            ));
        }
        else
        {
            IFC_FROM_HRESULT_RTS(pLsRun->GetFontFace()->GetDesignGlyphAdvances(
                glyphCount,
                pGlyphIndices,
                pGlyphAdvances,
                lstflow & fVDirection
            ));
        }

        if (!pLsRun->IsGlyphBased())
        {
            // We need ZWSP to always measure as zero as we have to use it
            // in an otherwise empty run to enable line services to get the
            // height of lines containing empty runs right.
            for (XUINT32 i = 0; i < glyphCount; i++)
            {
                if (pCodePoints[i] == UNICODE_ZERO_WIDTH_SPACE)
                {
                    pGlyphAdvances[i] = 0;
                }
            }
        }

        if (!pLsRun->IsGlyphBased() && pLsRun->GetFontFace()->HasKerningPairs())
        {
            IFC_OOM_RTS(pGlyphAdvanceAdjustments = new XINT32[glyphCount]);
            IFC_FROM_HRESULT_RTS(pLsRun->GetFontFace()->GetKerningPairAdjustments(glyphCount, pGlyphIndices, pGlyphAdvanceAdjustments));
        }

        // Calculate scaling factor for conversion from font design units to text measurement units.

        pLsRun->GetFontFace()->GetMetrics(&fontMetrics);
        scaleFactor = pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale() * TEXTDPI_SCALE / fontMetrics.DesignUnitsPerEm;

        // Character spacing support.
        // Pre-fetch values used for characters spacing processing, including uniform
        // glyph expansion value (in font design units).

        if (pTextRun->GetProperties()->GetCharacterSpacing() != 0)
        {
            glyphExpansion = Math::Round(pTextRun->GetProperties()->GetCharacterSpacing() * fontMetrics.DesignUnitsPerEm / CharacterSpacingScale);

            IFC_OOM_RTS(pClusterMap = new XUINT16[glyphCount]);
            IFC_OOM_RTS(pGlyphProps = new PALText::ShapingGlyphProperties[glyphCount]);
            IFC_OOM_RTS(pSpaceabilityMap = new bool[glyphCount]);

            for (XUINT16 i = 0; i < glyphCount; i++)
            {
                pClusterMap[i] = i;
                pGlyphProps[i].Justification = 2;   // SCRIPT_JUSTIFY_CHARACTER
                pGlyphProps[i].IsClusterStart = TRUE;
                pGlyphProps[i].IsDiacritic = 0;
                pGlyphProps[i].IsZeroWidthSpace = 0;
                pGlyphProps[i].Reserved = 0;
            }
            memset(pSpaceabilityMap, TRUE, sizeof(bool) * glyphCount);

            IFC_FROM_HRESULT_RTS(pLsHostContext->pFontAndScriptServices->CreateGlyphAnalyzer(&pGlyphAnalyzer));
            IFC_FROM_HRESULT_RTS(pGlyphAnalyzer->GetCharacterSpaceability(
                &(pLsRun->GetScriptAnalysis()),
                charCount,
                rgwchRun,
                pClusterMap,
                glyphCount,
                pGlyphProps,
                pSpaceabilityMap));

            spaceLastCharacter = pLsRun->GetSpaceLastCharacter(rgwchRun + (charCount-1));
        }

        // Fill out glyph advances array (in text measurement units) using
        // original glyph advances and adjustments (kerning + character spacing).

        for (charIndex = 0, glyphIndex = 0; (charIndex < charCount) && (totalWidth <= duAvailable); charIndex++, glyphIndex++)
        {
            XINT32 advanceWidth;

            advanceWidth = pGlyphAdvances[glyphIndex];

            // Adjusts a glyph width according to pair-kerning values.
            if (pGlyphAdvanceAdjustments != NULL)
            {
                advanceWidth += pGlyphAdvanceAdjustments[glyphIndex];
            }

            // Adjusts a glyph width according to the character spacing property.
            if (glyphExpansion != 0)
            {
                if (advanceWidth > 0 &&
                    (!pSpaceabilityMap || pSpaceabilityMap[glyphIndex]) &&
                    (spaceLastCharacter || (charIndex < charCount - 1)))
                {
                    advanceWidth += glyphExpansion;
                    // Do not allow CharacterSpacing to reduce advance width below zero
                    if (advanceWidth < 0)
                    {
                        advanceWidth = 0;
                    }
                }
            }

            // Apply conversion from font design units to text measuring units.
            advanceWidth = Math::Round(advanceWidth * scaleFactor);
            rgdu[charIndex] = advanceWidth;
            totalWidth += advanceWidth;

            // Use glyph map to properly adjust position in the UTF16 buffers.
            if (pGlyphMap != NULL)
            {
                if ((charIndex < charCount - 1) && pGlyphMap[charIndex+1] == FALSE)
                {
                    // Fill in the zero width for the second 16 bit codepoint of a surrogate pair.
                    rgdu[++charIndex] = 0;
                }
            }
        }

        *pduTotal = totalWidth;
        *pcwchConsumed = charIndex;
    }
    else if (cwchRun > 0 && (rgwchRun[0] == UNICODE_LINE_SEPARATOR || rgwchRun[0] == UNICODE_PARAGRAPH_SEPARATOR))
    {
        // As of LS6, LS calls back for the width of the "visi" character for endPara and endLineInPara characters.
        // Text config sets these to 0x2029 and 0x2028. LS5 assumed a width of 1 LS unit for endPara and endLineInPara.
        // To maintain parity we define visiEndPara1 and visiEndLineInPara in the text config to be the same as non-visi
        // equivalents, i.e. 0x2029 and 0x2028, and check for them in this callback and return width of 1.
        ASSERT(cwchRun == 1);
        if (duAvailable > 0)
        {
            *pcwchConsumed = 1;
            *pduTotal = 1;
            rgdu[0] = 1;
        }
        else
        {
            *pcwchConsumed = 1;
            *pduTotal = 0;
            rgdu[0] = 0;
        }
    }
    else
    {
        *pcwchConsumed = 1;
        *pduTotal = 0;
        rgdu[0] = 0;
    }

    ASSERT(*pcwchConsumed > 0);
    ASSERT((*pcwchConsumed == cwchRun) || (*pduTotal >= duAvailable));

Cleanup:
    ReleaseInterface(pGlyphAnalyzer);
    delete [] pGlyphAdvanceAdjustments;
    delete [] pGlyphAdvances;
    delete [] pGlyphIndices;
    delete [] pGlyphMap;
    delete [] pCodePoints;
    delete [] pSpaceabilityMap;
    delete [] pClusterMap;
    delete [] pGlyphProps;

    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetRunTextMetrics
//
//  Synopsis:
//      Callback used by LS to obtain text metrics for a run.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetRunTextMetrics(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ LSDEVICE lsdev,
        // Presentation or reference device
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _Out_ PLSTXM plstxm
        // Pointer to structure containing the text metric information
    )
{
    Result::Enum txhr = Result::Success;
    LsHostContext *pLsHostContext;
    LsRun *pLsRun;
    const TextCharactersRun *pTextRun;
    XUINT32 twipsAscent;
    XUINT32 twipsDescent;
    XUINT32 lineHeight;
    TextLineBounds textLineBounds;

    pLsRun = reinterpret_cast<LsRun *>(plsrun);

    // Use run properties to get metrics only for plain text runs.
    if (pLsRun->GetTextRun() != NULL && pLsRun->GetTextRun()->GetType() == TextRunType::Text)
    {
        XFLOAT baseline, lineSpacing;

        // NOTE: Cannot use actual font face resolved during font fallback because it might have
        // different metrics. Use Composite font family instead, which will return uniform
        // values independent on actual font picked during font fallback.

        pTextRun = reinterpret_cast<const TextCharactersRun *>(pLsRun->GetTextRun());
        pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
        textLineBounds = pLsHostContext->pParagraphProperties->GetTextLineBounds();

        IFC_FROM_HRESULT_RTS(pTextRun->GetProperties()->GetFontTypeface()->GetCompositeFontFamily()->GetTextLineBoundsMetrics(
                                textLineBounds,
                                &baseline,
                                &lineSpacing));

        twipsAscent  = TextDpi::ToTextDpi(baseline * pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale());
        lineHeight   = TextDpi::ToTextDpi(lineSpacing * pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale());
        twipsDescent = lineHeight - twipsAscent;
    }
    else
    {
        twipsAscent = 0;
        twipsDescent = 0;
        lineHeight = 0;
    }

    // Initialize text metrics to default values.
    memset(plstxm, 0, sizeof(LSTXM));
    plstxm->dvAscent            = twipsAscent;
    plstxm->dvDescent           = twipsDescent;
    plstxm->dvMultiLineHeight   = lineHeight;
    //plstxm->fMonospaced         = FALSE;

Cleanup:
    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetRunUnderlineInfo
//
//  Synopsis:
//      Callback used by LS to obtain underline information for a run.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetRunUnderlineInfo(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ PCHEIGHTS heightsPres,
        // Pointer to heights data structure
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _Out_ PLSULINFO plsulnfo
        // Pointer to underline information for the run
    )
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun;
    FssFontMetrics fontMetrics;
    XFLOAT scaleFactor;
    const TextCharactersRun *pTextRun;

    pLsRun = reinterpret_cast<LsRun *>(plsrun);

    // Since LS won't query underline metrics unless the run properties indicate that it is underlined,
    // we are safe in casting to TextCharactersRun without checking type. Assert just to be careful.
    ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);
    ASSERT(pLsRun->GetFontFace() != NULL);

    pTextRun = reinterpret_cast<const TextCharactersRun *>(pLsRun->GetTextRun());
    pLsRun->GetFontFace()->GetMetrics(&fontMetrics);
    scaleFactor = pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale() / fontMetrics.DesignUnitsPerEm;

    // Initialize underline metrics to default values.
    memset(plsulnfo, 0, sizeof(LSULINFO));
    plsulnfo->cNumberOfLines = 1;
    plsulnfo->dvpFirstUnderlineOffset = TextDpi::ToTextDpi(static_cast<XFLOAT>(-fontMetrics.UnderlinePosition) * scaleFactor);
    plsulnfo->dvpFirstUnderlineSize = TextDpi::ToTextDpi(static_cast<XFLOAT>(fontMetrics.UnderlineThickness) * scaleFactor);

    return LSErrFromResult(txhr);
}

LSERR RichTextServices::LineServicesGetRunStrikethroughInfo(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ PCHEIGHTS heightsPres,
        // Pointer to heights data structure
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _Out_ PLSSTINFO plsulnfo
        // Pointer to underline information for the run
    )
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun;
    FssFontMetrics fontMetrics;
    XFLOAT scaleFactor;
    const TextCharactersRun *pTextRun;

    pLsRun = reinterpret_cast<LsRun *>(plsrun);

    // Since LS won't query underline metrics unless the run properties indicate that it is underlined,
    // we are safe in casting to TextCharactersRun without checking type. Assert just to be careful.
    ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);
    ASSERT(pLsRun->GetFontFace() != NULL);

    pTextRun = reinterpret_cast<const TextCharactersRun *>(pLsRun->GetTextRun());
    pLsRun->GetFontFace()->GetMetrics(&fontMetrics);
    scaleFactor = pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale() / fontMetrics.DesignUnitsPerEm;

    // Initialize underline metrics to default values.
    memset(plsulnfo, 0, sizeof(*plsulnfo));
    plsulnfo->cNumberOfLines = 1;
    plsulnfo->dvpLowerStrikethroughOffset = TextDpi::ToTextDpi(static_cast<XFLOAT>(fontMetrics.StrikethroughPosition) * scaleFactor);
    plsulnfo->dvpLowerStrikethroughSize = TextDpi::ToTextDpi(static_cast<XFLOAT>(fontMetrics.StrikethroughThickness) * scaleFactor);

    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesReleaseRunBuffer
//
//  Synopsis:
//      Callback used by LS to notify client that the run buffer is not
//      being used any more.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesReleaseRunBuffer(
    _In_ POLS pols,
        // LS host
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph info
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ LPWSTR rgwchRun
        // Pointer to run buffer.
    )
{
    // Character buffer associated with a run points directly to the backing store,
    // hence there is not need to release it.
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesDrawUnderline
//
//  Synopsis:
//      Callback used by LS to draw an underline.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesDrawUnderline(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ UINT kUlbase,
        // Specifies kind of line to be drawn (e.g. dashed, dotted, etc.)
    _In_ PCLSPOINT pptStart,
        // Starting position (top left)
    _In_ LONG dupFromStartLine,
        // dup from start line to starting position of underline
    _In_ LONG dupUL,
        // Underline width
    _In_ LONG dvpUL,
        // Underline thickness
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _In_ UINT kDisp,
        // Display mode - opaque (0), transparent (1)
    _In_ PCLSRECT pRectClip
        // Clipping rectangle
    )
{
    Result::Enum txhr = Result::Success;
    LsHostContext *pLsHostContext;
    LsRun *pLsRun;
    const TextCharactersRun *pTextRun;
    XPOINTF position;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    pLsRun = reinterpret_cast<LsRun *>(plsrun);

    IFC_EXPECT_RTS(pLsHostContext->pDrawingContext);

    // Since LS won't call DrawUnderline unless the run properties indicate that it is underlined,
    // we are safe in casting to TextCharactersRun without checking type. Assert just to be careful.
    ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);
    pTextRun = reinterpret_cast<const TextCharactersRun *>(pLsRun->GetTextRun());

    position.x = TextDpi::FromTextDpi(pptStart->x);
    position.y = TextDpi::FromTextDpi(pptStart->y);

    IFCTEXT(pLsHostContext->pDrawingContext->DrawLine(
        position,
        TextDpi::FromTextDpi(dupUL),
        TextDpi::FromTextDpi(dvpUL),
        RtlFromLSTFLOW(lstflow),
        pTextRun->GetProperties()->GetForegroundBrushSource(),
        &MakeClipRectForGlyphRun(pRectClip, pptStart, pLsRun->GetBidiLevel())
        ));

Cleanup:
    return LSErrFromResult(txhr);
}


LSERR RichTextServices::LineServicesDrawStrikethrough(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ UINT kUlbase,
        // Specifies kind of line to be drawn (e.g. dashed, dotted, etc.)
    _In_ PCLSPOINT pptStart,
        // Starting position (top left)
    _In_ LONG dupUL,
        // Underline width
    _In_ LONG dvpUL,
        // Underline thickness
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _In_ UINT kDisp,
        // Display mode - opaque (0), transparent (1)
    _In_ PCLSRECT pRectClip
        // Clipping rectangle
    )
{
    return LineServicesDrawUnderline(pols,
        plsrun,
        kUlbase,
        pptStart,
        0,
        dupUL,
        dvpUL,
        lstflow,
        kDisp,
        pRectClip
        );
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesFInterruptUnderline
//
//  Synopsis:
//      Callback used by LS to to find out if the client wants the underline
//      to be broken between the two runs.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesFInterruptUnderline(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrunFirst,
        // Run pointer for the previous run
    _In_ PLSRUN plsrunSecond,
        // Run pointer for the current run
    _Out_ BOOL* pfInterruptUnderline
        // Flag indicating if to interrupt drawing of the underline between these runs
    )
{
    Result::Enum txhr = Result::Success;
    LsRun *pFirstLsRun= reinterpret_cast<LsRun *>(plsrunFirst);
    LsRun *pSecondLsRun = reinterpret_cast<LsRun *>(plsrunSecond);

    // Since LS won't ask about underline interruption unless both runs are underlined,
    // we are safe in casting to TextCharactersRun without checking type. Assert just to be careful.
    ASSERT(pFirstLsRun->GetTextRun()->GetType() == TextRunType::Text);
    ASSERT(pSecondLsRun->GetTextRun()->GetType() == TextRunType::Text);

    const TextCharactersRun *pFirstTextRun = reinterpret_cast<const TextCharactersRun *>(pFirstLsRun->GetTextRun());
    const TextCharactersRun *pSecondTextRun = reinterpret_cast<const TextCharactersRun *>(pSecondLsRun->GetTextRun());

    // TODO: call the host to make the decision if the underline needs to be interrupted or not
    *pfInterruptUnderline = (pFirstTextRun->GetProperties()->GetForegroundBrushSource() != pSecondTextRun->GetProperties()->GetForegroundBrushSource());

    return LSErrFromResult(txhr);
}

LSERR RichTextServices::LineServicesFInterruptStrikethrough(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrunFirst,
        // Run pointer for the previous run
    _In_ PLSRUN plsrunSecond,
        // Run pointer for the current run
    _Out_ BOOL* pfInterruptStrikethrough
        // Flag indicating if to interrupt drawing of the underline between these runs
    )
{
    return LineServicesFInterruptUnderline(pols, plsrunFirst, plsrunSecond, pfInterruptStrikethrough);

}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesDrawTextRun
//
//  Synopsis:
//      Callback used by LS to to tell client to draw a text run.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesDrawTextRun(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Pointer to client run information
    _In_ BOOL fScaleDownFromRef,
        // Reference units are used for presentation*/
    _In_ PCLSPOINT pptText,
        // Starting point for the text output (untrimmed characters)
    _In_reads_(cwchRun) const WCHAR* rgwchRun,
        // Run of characters.
    _In_reads_(cwchRun) const LONG* rgdupRun,
        // Pointer to array containing character widths
    _In_ LONG cwchRun,
        // Number of characters in run
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _In_ UINT kDisp,
        // Display mode opaque (0), transparent (1)
    _In_ PCLSPOINT pptRun,
        // Starting point of the run, differs from pptText only
        // when the the left portion of the leading character is trimmed.
        // In that case pptRun represents the starting point of the trimmed first letter.
    _In_reads_(cwchRun) PCHEIGHTS heightsPres,
        // Heights data for this run in presentation units.
    _In_ LONG dupRun,
        // Presentation width for this run.
    _In_ PCLSRECT pRectClip
        // Pointer to clipping rectangle.
    )
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun;
    LsHostContext *pLsHostContext;
    FssGlyphRun *pGlyphRun = NULL;
    XUINT32 *pCodePoints = NULL;
    XUINT16 *pGlyphIndices = NULL;
    XFLOAT  *pGlyphAdvances = NULL;

    pLsRun = reinterpret_cast<LsRun *>(plsrun);
    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    IFC_EXPECT_RTS(pLsHostContext->pDrawingContext);
    ASSERT(cwchRun > 0 && static_cast<XUINT32>(cwchRun) <= pLsRun->GetTextRun()->GetLength());

    if (pLsRun->GetFontFace() != NULL)
    {
        const TextCharactersRun *pTextRun;
        XUINT32 glyphCount = static_cast<XUINT32>(cwchRun);
        XUINT32 glyphIndex;
        XPOINTF position;

        ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);
        pTextRun = reinterpret_cast<const TextCharactersRun*>(pLsRun->GetTextRun());

        // Covert UTF16 codepoints to UTF32 codepoints and get corresponding glyph indices.
        // There is no need to use DecodeUtf16Character in this case, because it gets
        // called only for character based runs.
        // Convert also glyph advances from text measurement units to display units.

        IFC_OOM_RTS(pCodePoints    = new XUINT32[glyphCount]);
        IFC_OOM_RTS(pGlyphIndices  = new XUINT16[glyphCount]);
        IFC_OOM_RTS(pGlyphAdvances = new XFLOAT [glyphCount]);
        for (glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++)
        {
            pCodePoints[glyphIndex]    = rgwchRun[glyphIndex];
            pGlyphAdvances[glyphIndex] = TextDpi::FromTextDpi(rgdupRun[glyphIndex]);
        }
        IFC_FROM_HRESULT_RTS(pLsRun->GetFontFace()->GetGlyphIndices(pCodePoints, glyphCount, pGlyphIndices));

        // Initialize glyph run structure and pass it to the drawing context.

        IFC_OOM_RTS(pGlyphRun = new FssGlyphRun());
        pGlyphRun->FontFace      = pLsRun->GetFontFace();
        pGlyphRun->FontEmSize    = pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale();
        pGlyphRun->GlyphCount    = glyphCount;
        pGlyphRun->GlyphIndices  = pGlyphIndices;
        pGlyphRun->GlyphAdvances = pGlyphAdvances;
        pGlyphRun->GlyphOffsets  = NULL;
        pGlyphRun->IsSideways    = lstflow & fVDirection;
        pGlyphRun->BidiLevel     = pLsRun->GetBidiLevel();

        AddRefInterface(pGlyphRun->FontFace);
        pGlyphIndices  = NULL;
        pGlyphAdvances = NULL;

        position.x = TextDpi::FromTextDpi(pptText->x);
        position.y = TextDpi::FromTextDpi(pptText->y);

        IFC_FROM_HRESULT_RTS(pLsHostContext->pDrawingContext->DrawGlyphRun(
                position,
                TextDpi::FromTextDpi(dupRun),
                pGlyphRun,
                pTextRun->GetProperties()->GetForegroundBrushSource(),
                &MakeClipRectForGlyphRun(pRectClip, pptText, pLsRun->GetBidiLevel())
                ));
        pGlyphRun = NULL;
    }

Cleanup:
    delete [] pCodePoints;
    delete [] pGlyphIndices;
    delete [] pGlyphAdvances;
    if (pGlyphRun != NULL)
    {
        delete [] pGlyphRun->GlyphIndices;
        delete [] pGlyphRun->GlyphAdvances;
        ReleaseInterface(pGlyphRun->FontFace);
        delete pGlyphRun;
    }

    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesFInterruptShaping
//
//  Synopsis:
//      Callback used by LS to find out whether two adjacent runs should be
//      shaped together.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesFInterruptShaping(
    _In_ POLS pols,
        // LS host
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _In_ PLSRUN plsrunFirst,
        // Run pointer for the previous run
    _In_ PLSRUN plsrunSecond,
        // Run pointer for the current run
    _Out_ BOOL* pfInterruptShaping
        // Flag indicating whether to interrupt character shaping between these runs.
    )
{
    Result::Enum  txhr  = Result::Success;
    LsHostContext *pLsHostContext = NULL;
    LsRun *pLsRunFirst = NULL;
    LsRun *pLsRunSecond = NULL;
    const TextCharactersRun *pTextRunFirst = NULL;
    const TextCharactersRun *pTextRunSecond = NULL;
    bool merge = false;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    pLsRunFirst = reinterpret_cast<LsRun *>(plsrunFirst);
    pLsRunSecond = reinterpret_cast<LsRun *>(plsrunSecond);

    if (pLsRunFirst->HasEqualProperties(pLsRunSecond))
    {
        pTextRunFirst = reinterpret_cast<const TextCharactersRun*>(pLsRunFirst->GetTextRun());
        pTextRunSecond = reinterpret_cast<const TextCharactersRun*>(pLsRunSecond->GetTextRun());
        merge = pTextRunFirst->GetProperties()->EqualsForShaping(pTextRunSecond->GetProperties());
    }

    *pfInterruptShaping = !merge;

    return LSErrFromResult(txhr);
}

#pragma prefast(push)
#pragma prefast(disable: 26030, "Disable prefast false positive on assignment of prggprop and prcindex")
//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetGlyphs
//
//  Synopsis:
//      Callback used by LS to obtain glyphs and character-to-glyph mapping
//      information for glyph-based runs.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetGlyphs(
    _In_ POLS pols,
        // LS host
    _In_reads_(crun) const PLSRUN* rgplsrun,
        // Array of run pointers
    _In_reads_(crun) const LONG* rgdwchRun,
        // Array of number of chars per run
    _In_ LONG crun,
        // Number of runs
    _In_reads_(cwch) LPCWSTR rgwch,
        // Pointer to the string of character codes
    _In_ LONG cwch,
        // Number of characters to be shaped
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _Out_writes_(cwch) PGMAP rggmap,
        // Parallel to the char codes mapping wch->glyph info
    _Out_writes_(cwch) PCHPROP rgchprop,
        // Array of output char properties
    _Out_writes_(cwch) BOOL* rgfCanGlyphAlone,
        // Parallel to the char codes: glyphing does not depend on neighbors?
    _Outptr_result_buffer_(*pcgindex) PGINDEX* prggindex,
        // Array of output glyph indices
    _Outptr_result_buffer_(*pcgindex) PGPROP* prggprop,
        // Array of output glyph properties
    _Out_ LONG* pcgindex
        // Number of output glyph indices
    )
{
    Result::Enum  txhr  = Result::Success;
    LsHostContext *pLsHostContext = NULL;
    LsRun *pLsRun = NULL;
    PALText::IGlyphAnalyzer *pGlyphAnalyzer = NULL;
    FssTypographicFeatures *pTypographicFeatures = NULL;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    IFC_EXPECT_RTS(pLsHostContext->pFontAndScriptServices);

    // LineServicesFInterruptShaping guarantees adjacent run compatibility in terms of properties,
    // so it appropriate to use the first TextRun's properties for the shaping purposes.
    pLsRun = reinterpret_cast<LsRun *>(rgplsrun[0]);

    ASSERT((pLsRun->GetBidiLevel() & 1) == RtlFromLSTFLOW(lstflow));

    if (pLsRun->GetFontFace() != NULL)
    {
        XINT32 charIndex;
        XUINT32 glyphCount;
        XUINT16 *pGlyphIndices = NULL;
        FssShapingTextProperties *pTextProperties = NULL;
        FssShapingGlyphProperties *pGlyphProperties = NULL;
        const TextCharactersRun *pTextRun = reinterpret_cast<const TextCharactersRun*>(pLsRun->GetTextRun());
        XUINT32 featureRangeLength = static_cast<XUINT32>(cwch);
        const InheritedProperties *pInheritedProperties = NULL;

        ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);

        pTextProperties = reinterpret_cast<FssShapingTextProperties*>(rgchprop);
        IFC_FROM_HRESULT_RTS(pLsHostContext->pFontAndScriptServices->CreateGlyphAnalyzer(&pGlyphAnalyzer));

        pInheritedProperties = pTextRun->GetProperties()->GetInheritedProperties();
        if (pInheritedProperties != NULL  &&  !pInheritedProperties->m_typography.IsTypographyDefault())
        {
            XLONG featureCount;
            IFC_OOM_RTS(pTypographicFeatures = new FssTypographicFeatures());

            IFC_FROM_HRESULT_RTS(pInheritedProperties->m_typography.GetOpenTypeFeatureSelections(
                &featureCount,
                reinterpret_cast<OpenTypeFeatureSelection**>(&pTypographicFeatures->Features)
            ));
            pTypographicFeatures->FeatureCount = static_cast<XUINT32>(featureCount);
        }

        IFC_FROM_HRESULT_RTS(pGlyphAnalyzer->GetGlyphs(
            rgwch,
            cwch,
            pLsRun->GetFontFace(),
            lstflow & fVDirection,
            RtlFromLSTFLOW(lstflow),
            &pLsRun->GetScriptAnalysis(),
            pTextRun->GetProperties()->GetCultureInfo().GetBuffer(),
            pLsRun->GetNumberSubstitution(),
            (pTypographicFeatures != NULL) ? const_cast<const FssTypographicFeatures**>(&pTypographicFeatures) : NULL,
            (pTypographicFeatures != NULL) ? &featureRangeLength : NULL,
            (pTypographicFeatures != NULL) ? 1 : 0,
            rggmap,
            pTextProperties,
            &pGlyphIndices,
            &pGlyphProperties,
            &glyphCount
            ));
         // There is a size mismatch between LineServices and DWrite for the
         // glyph property array. DWrite uses 16 bit ShapingGlyphProperties,
         // while LineServices uses 32 bit PGPROP. This mismatch will cause app
         // to crash under appverifer because the memory allocated on heap is
         // smaller.
         // The ideal fix here would be changing the size of PGPROP to 16bits.
         // The work around here is to allocated a buffer 2 times big and
         // copy over the array from DWrite to the first half of it. Then we
         // pass this larger array to LineServices.
         GPROP* pGlyphProperties32 = new GPROP[glyphCount];
         IFC_OOM_RTS(pGlyphProperties32);

         memcpy(pGlyphProperties32, pGlyphProperties, glyphCount *
         sizeof(FssShapingGlyphProperties));

         SAFE_DELETE_ARRAY(pGlyphProperties);

        *pcgindex = static_cast<XLONG>(glyphCount);
        *prggindex = pGlyphIndices;
        *prggprop = pGlyphProperties32;

        for (charIndex = 0; charIndex < cwch; charIndex++)
        {
            rgfCanGlyphAlone[charIndex] = pTextProperties[charIndex].IsShapedAlone;
        }

        pLsHostContext->hasMultiCharacterClusters |= HasMultiCharacterClusters(cwch, *pcgindex, rggmap);
    }

Cleanup:
    ReleaseInterface(pGlyphAnalyzer);
    if (pTypographicFeatures != NULL)
    {
        delete [] pTypographicFeatures->Features;
        delete pTypographicFeatures;
    }

    return LSErrFromResult(txhr);
}
#pragma prefast(pop)

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetGlyphPositions
//
//  Synopsis:
//      Callback used by LS to obtain the widths and offsets of the glyphs
//      that were produced in pfnGetGlyphs.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetGlyphPositions(
    _In_ POLS pols,
        // LS host
    _In_reads_(crun) const PLSRUN* rgplsrun,
        // Array of run pointers
    _In_reads_(crun) const LONG* rgdwchRun,
        // Array of number of chars per run
    _In_ LONG crun,
        // Number of runs
    _In_ LSDEVICE lsdev,
        // Presentation or reference device?
    _In_reads_(cwch) LPWSTR rgwch,
        // Pointer to the string of character codes
    _In_reads_(cwch) PCGMAP rggmap,
        // Array of wch->glyph mapping
    _In_reads_(cwch) PCCHPROP rgchprop,
        // Array of char properties
    _In_ LONG cwch,
        // Number of characters to be shaped
    _In_reads_(cgindex) PCGINDEX rggindex,
        // Array of glyph indices
    _In_reads_(cgindex) PCGPROP rggprop,
        // Array of glyph properties
    _In_ LONG cgindex,
        // Number glyph indices
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _Out_writes_(cgindex) LONG* rgdu,
        // Array of widths of glyphs
    _Out_writes_(cgindex) PLSGOFFSET rgGoffset
        // Array of offsets of glyphs
    )
{
    Result::Enum txhr = Result::Success;
    LsHostContext *pLsHostContext;
    LsRun *pLsRun;
    PALText::IGlyphAnalyzer *pGlyphAnalyzer = NULL;
    XFLOAT *pGlyphAdvances = NULL;
    FssGlyphOffset *pGlyphOffsets = NULL;
    FssTypographicFeatures *pTypographicFeatures = NULL;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    IFC_EXPECT_RTS(pLsHostContext->pFontAndScriptServices);

    // LineServicesFInterruptShaping guarantees adjacent run compatibility in terms of properties,
    // so it appropriate to use the first TextRun's properties for the shaping purposes.
    pLsRun = reinterpret_cast<LsRun *>(rgplsrun[0]);

    ASSERT((pLsRun->GetBidiLevel() & 1) == RtlFromLSTFLOW(lstflow));

    if (pLsRun->GetFontFace() != NULL)
    {
        const FssShapingTextProperties *pTextProperties = NULL;
        const FssShapingGlyphProperties *pGlyphProperties = NULL;
        const TextCharactersRun *pTextRun = reinterpret_cast<const TextCharactersRun*>(pLsRun->GetTextRun());
        XINT32 glyphIndex;
        XUINT32 featureRangeLength = static_cast<XUINT32>(cwch);
        const InheritedProperties *pInheritedProperties = NULL;

        ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);

        IFC_OOM_RTS(pGlyphAdvances = new XFLOAT[cgindex]);
        IFC_OOM_RTS(pGlyphOffsets = new FssGlyphOffset[cgindex]);
        pTextProperties = reinterpret_cast<const FssShapingTextProperties*>(rgchprop);
        pGlyphProperties = reinterpret_cast<const FssShapingGlyphProperties*>(rggprop);
        IFC_FROM_HRESULT_RTS(pLsHostContext->pFontAndScriptServices->CreateGlyphAnalyzer(&pGlyphAnalyzer));

        pInheritedProperties = pTextRun->GetProperties()->GetInheritedProperties();
        if (     pInheritedProperties != NULL
            &&  !pInheritedProperties->m_typography.IsTypographyDefault())
        {
            XLONG featureCount;
            IFC_OOM_RTS(pTypographicFeatures = new FssTypographicFeatures());

            IFC_FROM_HRESULT_RTS(pInheritedProperties->m_typography.GetOpenTypeFeatureSelections(
                &featureCount,
                reinterpret_cast<OpenTypeFeatureSelection**>(&pTypographicFeatures->Features)
            ));
            pTypographicFeatures->FeatureCount = static_cast<XUINT32>(featureCount);
        }

        IFC_FROM_HRESULT_RTS(pGlyphAnalyzer->GetGlyphPlacements(
            rgwch,
            rggmap,
            const_cast<FssShapingTextProperties*>(pTextProperties),
            cwch,
            rggindex,
            pGlyphProperties,
            cgindex,
            pLsRun->GetFontFace(),
            pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale(),
            lstflow & fVDirection,
            RtlFromLSTFLOW(lstflow),
           &pInheritedProperties->m_textOptions,
           &pLsRun->GetScriptAnalysis(),
            pTextRun->GetProperties()->GetCultureInfo().GetBuffer(),
            (pTypographicFeatures != NULL) ? const_cast<const FssTypographicFeatures**>(&pTypographicFeatures) : NULL,
            (pTypographicFeatures != NULL) ? &featureRangeLength : NULL,
            (pTypographicFeatures != NULL) ? 1 : 0,
            pGlyphAdvances,
            pGlyphOffsets
        ));

        // Apply any initial spacing.
        // Note that we do not support negative (compression) CharacterSpacing that
        // has been transferred to initial spacing following a reverse direction
        // run because we have no way to ensure that large negative CharacterSpacing
        // values do not move the text to before the margin (which triggers an
        // ASSERT in LS).

        if (pLsRun->GetInitialSpacing() > 0)
        {
            // Offset the initial base character by adjusting both its initial
            // offset and advance width.
            XFLOAT initialExpantion = pLsRun->GetInitialSpacing() * pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale() / CharacterSpacingScale;
            pGlyphAdvances[0] += initialExpantion;
            pGlyphOffsets[0].AdvanceOffset += initialExpantion;
        }

        // Apply character spacing
        if (pTextRun->GetProperties()->GetCharacterSpacing() != 0)
        {
            LsRun *pLastLsRun = reinterpret_cast<LsRun *>(rgplsrun[crun-1]);
            IFC_FROM_HRESULT_RTS(pGlyphAnalyzer->ApplyCharacterSpacing(
                pTextRun->GetProperties()->GetCharacterSpacing() * pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale() / CharacterSpacingScale,
                pLastLsRun->GetSpaceLastCharacter(),
                &pLsRun->GetScriptAnalysis(),
                cwch,
                rgwch,
                rggmap,
                cgindex,
                pGlyphProperties,
                pGlyphAdvances,
                pGlyphOffsets
                ));
        }

        for (glyphIndex = 0; glyphIndex < cgindex; glyphIndex++)
        {
            rgdu[glyphIndex]         = TextDpi::ToTextDpi(pGlyphAdvances[glyphIndex]);
            rgGoffset[glyphIndex].du = TextDpi::ToTextDpi(pGlyphOffsets[glyphIndex].AdvanceOffset);
            rgGoffset[glyphIndex].dv = TextDpi::ToTextDpi(pGlyphOffsets[glyphIndex].AscenderOffset);
        }
    }

Cleanup:
    ReleaseInterface(pGlyphAnalyzer);
    delete [] pGlyphAdvances;
    delete [] pGlyphOffsets;
    if (pTypographicFeatures != NULL)
    {
        delete [] pTypographicFeatures->Features;
        delete pTypographicFeatures;
    }

    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesDrawGlyphs
//
//  Synopsis:
//      Callback used by LS to draw glyphs.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesDrawGlyphs(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Run pointer of the first run
    _In_ BOOL fScaleDownFromRef,
        // Reference units are used for presentation
    _In_reads_(cwch) LPWSTR rgwch,
        // Pointer to the string of character codes
    _In_reads_(cwch) PCGMAP rggmap,
        // Array of wch->glyph mapping
    _In_reads_(cwch) PCCHPROP rgchprop,
        // Array of char properties
    _In_ LONG cwch,
        // Number of characters to be shaped
    _In_reads_(cglyph) PCGINDEX pglyph,
        // Array of glyph indices
    _In_reads_(cglyph) const LONG* rgdu,
        // Array of widths of glyphs
    _In_reads_(cglyph) const LONG* rgduBeforeJust,
        // Array of widths of glyphs before justification
    _In_reads_(cglyph) PCLSGOFFSET rgGoffset,
        // Array of offsets of glyphs
    _In_reads_(cglyph) PCGPROP rggprop,
        // Array of glyph properties
    _In_reads_(cglyph) PCEXPTYPE rgExpType,
        // Array of glyph expansion types
    _In_ LONG cglyph,
        // Number glyph indices
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _In_ UINT kDisp,
        // Display mode - opaque, transparent
    _In_ PCLSPOINT pptRun,
        // Starting point of the run
    _In_ PCHEIGHTS heightsPres,
        // Presentation heights for this run
    _In_ LONG dupRun,
        // Presentation width for this run
    _In_ PCLSRECT pRectClip
        // Clipping rectangle
    )
{
#if SKIP_SHAPING_ENGINE_DRAW
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun;
    LsHostContext *pLsHostContext;
    FssGlyphRun *pGlyphRun = NULL;
    XUINT16 *pGlyphIndices = NULL;
    XFLOAT *pGlyphAdvances = NULL;
    FssGlyphOffset *pGlyphOffsets = NULL;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    pLsRun = reinterpret_cast<LsRun *>(plsrun);
    IFC_EXPECT_RTS(pLsHostContext->pDrawingContext);

    if (pLsRun->GetFontFace() != NULL)
    {
        XPOINTF position;
        XUINT32 glyphCount = static_cast<XUINT32>(cglyph);
        XUINT32 glyphIndex;
        const TextCharactersRun *pTextRun = reinterpret_cast<const TextCharactersRun*>(pLsRun->GetTextRun());

        ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);

        // Convert glyph advances and positions from text measurement units to display units.

        IFC_OOM_RTS(pGlyphIndices  = new XUINT16[glyphCount]);
        IFC_OOM_RTS(pGlyphAdvances = new XFLOAT [glyphCount]);
        IFC_OOM_RTS(pGlyphOffsets  = new FssGlyphOffset[glyphCount]);
        for (glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++)
        {
            pGlyphIndices [glyphIndex]                = pglyph[glyphIndex];
            pGlyphAdvances[glyphIndex]                = TextDpi::FromTextDpi(rgdu[glyphIndex]);
            pGlyphOffsets [glyphIndex].AdvanceOffset  = TextDpi::FromTextDpi(rgGoffset[glyphIndex].du);
            pGlyphOffsets [glyphIndex].AscenderOffset = TextDpi::FromTextDpi(rgGoffset[glyphIndex].dv);
        }

        // Initialize glyph run structure and pass it to the drawing context.

        IFC_OOM_RTS(pGlyphRun = new FssGlyphRun());
        pGlyphRun->FontFace      = pLsRun->GetFontFace();
        pGlyphRun->FontEmSize    = pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale();
        pGlyphRun->GlyphCount    = glyphCount;
        pGlyphRun->GlyphIndices  = pGlyphIndices;
        pGlyphRun->GlyphAdvances = pGlyphAdvances;
        pGlyphRun->GlyphOffsets  = pGlyphOffsets;
        pGlyphRun->IsSideways    = lstflow & fVDirection;
        pGlyphRun->BidiLevel     = pLsRun->GetBidiLevel();

        AddRefInterface(pGlyphRun->FontFace);
        pGlyphIndices  = NULL;
        pGlyphAdvances = NULL;
        pGlyphOffsets  = NULL;

        position.x = TextDpi::FromTextDpi(pptRun->x);
        position.y = TextDpi::FromTextDpi(pptRun->y);

        IFC(pLsHostContext->pDrawingContext->DrawGlyphRun(position, TextDpi::FromTextDpi(dupRun), pGlyphRun, pTextRun->GetProperties()->GetPropertiesSource()));
        pGlyphRun = NULL;
    }

Cleanup:
    delete [] pGlyphIndices;
    delete [] pGlyphAdvances;
    delete [] pGlyphOffsets;
    if (pGlyphRun != NULL)
    {
        delete [] pGlyphRun->GlyphIndices;
        delete [] pGlyphRun->GlyphAdvances;
        delete [] pGlyphRun->GlyphOffsets;
        ReleaseInterface(pGlyphRun->FontFace);
        delete pGlyphRun;
    }

    return LSErrFromResult(txhr);
#else
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun;
    LsHostContext *pLsHostContext;

    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);
    pLsRun = reinterpret_cast<LsRun *>(plsrun);
    IFC_EXPECT_RTS(pLsHostContext->pDrawingContext);

    if (pLsRun->GetFontFace() != NULL)
    {
        XPOINT position;
        const TextCharactersRun *pTextRun = reinterpret_cast<const TextCharactersRun*>(pLsRun->GetTextRun());

        ASSERT(pLsRun->GetTextRun()->GetType() == TextRunType::Text);

        position.x = pptRun->x;
        position.y = pptRun->y;

        IFC_FROM_HRESULT_RTS(pLsHostContext->pDrawingContext->DrawGlyphs(
            position,
            dupRun,
            rgwch,
            rggmap,
            reinterpret_cast<const FssShapingTextProperties*>(rgchprop),
            static_cast<XUINT32>(cwch),
            pglyph,
            reinterpret_cast<const FssShapingGlyphProperties*>(rggprop),
            rgdu,
            rgduBeforeJust,
            reinterpret_cast<const XPOINT*>(rgGoffset),
            static_cast<XUINT32>(cglyph),
            pLsRun->GetFontFace(),
            pTextRun->GetProperties()->GetFontSize() * pLsRun->GetFontScale(),
            lstflow & fVDirection,
            pLsRun->GetBidiLevel(),
            &pLsRun->GetScriptAnalysis(),
            pTextRun->GetProperties()->GetForegroundBrushSource(),
            &MakeClipRectForGlyphRun(pRectClip, pptRun, pLsRun->GetBidiLevel())
            ));
    }

Cleanup:
    return LSErrFromResult(txhr);
#endif
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesReleaseGlyphBuffers
//
//  Synopsis:
//      Callback used by LS to notify the client that the glyph indices and
//      glyph property buffers are no longer being used.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesReleaseGlyphBuffers(
    _In_ POLS pols,
        // LS host
    _In_ PGINDEX rggindex,
        // Array of output glyph indices
    _In_ PGPROP rggprop
        // Array of glyph properties
    )
{
    delete [] rggindex;
    delete [] rggprop;
    return lserrNone;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetBreakingClasses
//
//  Synopsis:
//      Callback used by LS to obtain the breaking classes of a character.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetBreakingClasses(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Run pointer for the char
    _In_ LSCP cp,
        // Character position of the character
    _In_ WCHAR wch,
        // Character
    _Out_ BRKCLS* pbrkclsFirst,
        //Breaking class for this char as the leading one in a pair
    _Out_ BRKCLS* pbrkclsSecond
        // Breaking class for this char as the following one in a pair
    )
{
    LsHostContext *pLsHostContext = NULL;
    pLsHostContext = reinterpret_cast<LsHostContext *>(pols);

    if (pLsHostContext->useEmergencyBreaking)
    {
        // When emergency breaking is turned on, we provide no break opportunities and rely on forced breaking.
        // See LsTextLine::FormatCollapsed
        *pbrkclsSecond = (Ptls6::BRKCLS)SimpleBreakClass::NoBreak;
        *pbrkclsFirst = (Ptls6::BRKCLS)SimpleBreakClass::NoBreak;
    }
    else
    {
        LsRun *pLsRun = reinterpret_cast<LsRun *>(plsrun);

        ASSERT(pLsRun->GetTextRun()->GetType() != TextRunType::Hidden);
        ASSERT(static_cast<XUINT32>(cp) >= pLsRun->GetCharacterIndex());

        XUINT32 characterOffset = cp - pLsRun->GetCharacterIndex();
        PALText::LineBreakpoint breakpoint = pLsRun->GetLineBreakpoint(characterOffset);

        // "pbrkclsSecond" is the breaking class of the char if it was the trailing
        // one in a pair, which maps to DWrite's breakConditionBefore.  Similarly,
        // "pbrkclsFirst" is if the char was the leading one in a pair, so the break
        // class is derived from DWrite's breakConditionAfter.
        *pbrkclsSecond = LineBreaking::DWriteBreakToSimpleBreakClass(breakpoint.breakConditionBefore);
        *pbrkclsFirst = LineBreaking::DWriteBreakToSimpleBreakClass(breakpoint.breakConditionAfter);
    }

    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesCanBreakBeforeChar
//
//  Synopsis:
//      Callback used by LS to determine if a line break is permissible
//      between preceding object and text.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesCanBreakBeforeChar(
    _In_ POLS pols,
        // LS host
    _In_ BRKCLS brkcls,
        // Breaking class for the char as the following one in a pair
    _Out_ BRKCOND* pbrktxtBefore
        // Break condition before the character
    )
{
    *pbrktxtBefore = LineBreaking::SimpleBreakClassToLSBrkCond(brkcls);

    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesCanBreakAfterChar
//
//  Synopsis:
//      Callback used by LS to determine if a line break is permissible
//      between text and a following object.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesCanBreakAfterChar(
    _In_ POLS pols,
        // LS host
    _In_ BRKCLS brkcls,
        // Breaking class for the char as the leading one in a pair
    _Out_ BRKCOND* pbrktxtAfter
        // Break text condition after the character
    )
{
    *pbrktxtAfter = LineBreaking::SimpleBreakClassToLSBrkCond(brkcls);

    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetDurMaxExpandRagged
//
//  Synopsis:
//      TODO
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetDurMaxExpandRagged(
    _In_ POLS pols,
        // LS host
    _In_ PLSRUN plsrun,
        // Run pointer as returned from FetchRun
    _In_ LSTFLOW lstflow,
        // Text direction and orientation
    _Out_ LONG* pdurMaxExpandRagged
        // Maximum "good" amount of empty space for the ragged case
    )
{
    // TODO: Remove it if not necessary (LS bug?)
    // NOTE: PTLS sets it to 1 by default.
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetPriorityForGoodTypography
//
//  Synopsis:
//      Callback used by LS to divide justification priorities into bad and good.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetPriorityForGoodTypography(
    _In_ POLS pols,
        // LS host
    _Out_ LONG* pcPriorityForGoodTypographyLim
        // Indicates which justification priorities are still at the "good typography" level.
    )
{
    // TODO: Remove it if not necessary (LS bug?)
    *pcPriorityForGoodTypographyLim = 0;
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesEnumText
//
//  Synopsis:
//      Callback used by LS to provide the client with the contents of a text dnode.
//
//---------------------------------------------------------------------------
//LSERR RichTextServices::LineServicesEnumText(
//    _In_ POLS pols,
//        // LS host
//    _In_ PLSRUN plsrun,
//        // Pointer to client run information
//    _In_ LSCP cpFirst,
//        // Offset of first character from dnode
//    _In_ LSDCP dcp,
//        // Dcp from dnode
//    _In_reads_(cwch) LPCWSTR rgwch,
//        // Array of characters
//    _In_ LONG cwch,
//        // Nnumber of characters
//    _In_ LSTFLOW lstflow,
//        // Text flow
//    _In_ BOOL fReverseOrder,
//        // Flag indicating enumerate is in reverse order
//    _In_ BOOL fGeometryProvided,
//        // Flag indicating whether text is prepared for display
//    _In_opt_ PCLSPOINT pptStart,
//        // Starting position (only if fGeometryProvided)
//    _In_opt_ PCHEIGHTS pheightsPres,
//        // Height in presentation units from dnode (only if fGeometryProvided)
//    _In_ LONG dupRun,
//        // Run width from DNODE (only if fGeometryProvided)
//    _In_ BOOL fGlyphs,
//        // Flag indicating whether the run is glyph-based
//    _In_reads_(cwch) LONG* rgdupChars,
//        // Array of character widths, if !fGlyphs
//    _In_reads_(cwch) PCGMAP rggmap,
//        // Array of wch->glyph mapping (only if fGlyphs)
//    _In_reads_(cwch) PCCHPROP rgchprop,
//        // Array of char properties (only if fGlyphs)
//    _In_reads_(cglyph) PCGINDEX rgglyph,
//        // Array of glyph indices (only if fGlyphs)
//    _In_reads_(cglyph) const LONG* rgdupGlyphs,
//        // Array of widths of glyphs (only if fGlyphs)
//    _In_reads_(cglyph) PLSGOFFSET rgGoffset,
//        // Array of offsets of glyphs (only if fGlyphs)
//    _In_reads_(cglyph) PGPROP rggprop,
//        // Array of glyph properties (only if fGlyphs)
//    _In_ LONG cglyph
//        // Number glyph indices (only if fGlyphs)
//    )
//{
//    return lserrNotImplemented;
//}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesEnumTab
//
//  Synopsis:
//      Callback used by LS to inform client that there is a tab present on the line.
//
//---------------------------------------------------------------------------
//LSERR RichTextServices::LineServicesEnumTab(
//    _In_ POLS pols,
//        // LS host
//    _In_ PLSRUN plsrun,
//        // Pointer to client run information
//    _In_ LSCP cpFirst,
//        // Offset of first character, from dnode
//    _In_ LPCWSTR rgwch,
//        // Pointer to one Tab character
//    _In_ WCHAR wchTabLeader,
//        // Tab leader
//    _In_ LSTFLOW lstflow,
//        // Text flow
//    _In_ BOOL fReverseOrder,
//        // Flag indicating that enumeration should occur in reverse order
//    _In_ BOOL fGeometryProvided,
//        // Flag indicating that the line is already prepared for display
//    _In_opt_ PCLSPOINT pptStart,
//        // Starting position, iff fGeometryProvided
//    _In_opt_ PCHEIGHTS pheightsPres,
//        // From DNODE, relevant iff fGeometryProvided
//    _In_ LONG dupRun
//        // From DNODE, relevant iff fGeometryProvided
//    )
//{
//    return lserrNotImplemented;
//}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetObjectHandlerInfo
//
//  Synopsis:
//      Callback used by LS to get object handler data.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetObjectHandlerInfo(
    _In_ POLS pols,
        // LS host.
    _In_ LONG idObj,
        // ID of the object.
    _Out_ void* pObjectInfo
        // Handler initialization data.
    )
{
    LSERR hr = lserrNone;
    LsObjectId::Enum objectId = static_cast<LsObjectId::Enum>(idObj);
    REVERSEINIT *pReverseInit = NULL;

    switch(objectId)
    {
        case LsObjectId::ReverseObject:
            pReverseInit = reinterpret_cast<REVERSEINIT *>(pObjectInfo);
            pReverseInit->pfnEnum = reinterpret_cast<PFNREVERSEENUM>(LineServicesReverseEnum);
            pReverseInit->pfnGetRobjInfo = reinterpret_cast<PFNREVERSEGETINFO>(LineServicesGetReverseObjectInfo);
            pReverseInit->fUseMasterSpans = TRUE;
            pReverseInit->wchEndReverse = '\0'; // Unused when fUseMasterSpans == TRUE.
            pReverseInit->wchUnused1 = 0;
            break;

        default:
            ASSERT(FALSE);
            hr = lserrInvalidParameter;
            break;
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesAssertFailed
//
//  Synopsis:
//      Callback used by LS as an assert handler.
//
//  Notes:
//      Only in Debugging API.
//
//---------------------------------------------------------------------------
void RichTextServices::LineServicesAssertFailed(
    _In_ const char* pMessage,
        // Message string from LS
    _In_ const char* pFilename,
        // Source file name where assert occurred
    _In_ int line,
        // Line number in source file where assert occurred
    _In_ DWORD err
        // Error code
    )
{
#if DBG && XCP_MONITOR
    // Temporary copy of file name and message as WCHAR
    WCHAR message16[100];
    WCHAR filename16[100];

    XUINT32 i=0;
    while (i < (ARRAY_SIZE(message16)-1) && pMessage[i])
    {
        message16[i] = pMessage[i];
        i++;
    }
    message16[i] = 0;

    i=0;
    while (i < (ARRAY_SIZE(filename16)-1) && pFilename[i])
    {
        filename16[i] = pFilename[i];
        i++;
    }
    filename16[i] = 0;

    gps->XcpTrace(MonitorAssert, filename16, line, TRUE, message16, NULL);

#endif
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesReverseEnum
//
//  Synopsis:
//      Callback used by LS to enumerate reverse objects.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesReverseEnum(
    _In_ POLS pols,
        // LS host.
    _In_ PLSRUN plsrun,
        // Pointer to client's run information.
    _In_ PCLSCHP plschp,
        // Run's character properties.
    _In_ LSSPAN lsspanMaster,
        // Master span of object.
    _In_ LSCP cp,
        // Run's character position.
    _In_ LSDCP dcp,
        // Run length, in cp.
    _In_ LSTFLOW lstflow,
        // Text flow.
    _In_ BOOL fReverse,
        // Enumerate in reverse order.
    _In_ BOOL fGeometryNeeded,
        // Flag for geometry inforation.
    _In_ PCLSPOINT pt,
        // Starting position of the object, iff fGeometryNeeded.
    _In_ PCLSPOINT ptSubline,
        // Starting position of the subline, iff fGeometryNeeded.
    _In_ PCHEIGHTS pcheights,
        // Presentation heights, relevant iff fGeometryNeeded.
    _In_ LONG dupRun,
        // Presentation width of run, relevant iff fGeometryNeeded.
    _In_    LSTFLOW lstflowSubline,
        // Lstflow of subline in reverse object.
    _In_ PLSSUBL plssubl
        // Subline in reverse object.
        )
{
    if (plssubl != NULL)
    {
        // Continue to enumerate subline
        return LsEnumSubline(plssubl, fReverse, fGeometryNeeded, pt);
    }

    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetReverseObjectInfo
//
//  Synopsis:
//      Callback used by LS to get reverse object formatting preferences.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetReverseObjectInfo(
    _In_ POLS pols,
        // LS host.
    _In_ LSCP cp,
        // Run character position.
    _In_ LSSPAN lsspanMaster,
        // Master span of object.
    _In_ PLSRUN plsrun,
        // Pointer to client run information.
    _Out_ BOOL *fDontBreakAround,
        // Whether reverse chunk should be broken around.
    _Out_ BOOL *fSuppressTrailingSpaces
        // Whether to suppress trailing spaces.
    )
{
    *fDontBreakAround        = TRUE;
    *fSuppressTrailingSpaces = TRUE;
    return lserrNone;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGlyphAlignmentHelper
//
//  Synopsis:
//      Helper function called by:
//          LineServicesGetGlyphAlignmentStartLine,
//          LineServicesGetGlyphAlignmentEndLine,
//          LineServicesGetCharAlignmentStartLine,
//          LineServicesGetCharAlignmentEndLine
//
//---------------------------------------------------------------------------
LSERR LineServicesGetAlignmentHelper(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ GINDEX gindex,
        _In_ GPROP gprop,
        _In_ BOOL fStartLine,
        _Out_ LONG* pduDelta)
{
    Result::Enum txhr = Result::Success;
    PALText::GlyphMetrics glyphMetrics;
    FssFontMetrics fontMetrics;
    XFLOAT scaleFactor = 0.0;
    LONG delta = 0;

    LsRun *pRun = reinterpret_cast<LsRun *>(plsrun);
    const TextCharactersRun *pTextRun = reinterpret_cast<const TextCharactersRun *>(pRun->GetTextRun());

    IFC_FROM_HRESULT_RTS(pRun->GetFontFace()->GetDesignGlyphMetrics(&gindex, 1, &glyphMetrics, FALSE /* isSideways */));
    pRun->GetFontFace()->GetMetrics(&fontMetrics);

    scaleFactor = pTextRun->GetProperties()->GetFontSize() * pRun->GetFontScale() / fontMetrics.DesignUnitsPerEm;

    // only lstflowWS and lstflowES can be passed in through LsTextLine::SetupLsPap
    switch (lstflow)
    {
        case lstflowES:
            delta = (fStartLine ? glyphMetrics.LeftSideBearing : glyphMetrics.RightSideBearing);
            break;

        case lstflowWS:
            delta = (fStartLine ? glyphMetrics.RightSideBearing : glyphMetrics.LeftSideBearing);
            break;

        default:
            ASSERT(FALSE);
    }

Cleanup:

    // There are glyphs with negative side bearings in lowercase Segoe UI, certain sets of Gabriola, etc.
    // In these cases, we do not want to apply the adjustment.  Only apply when delta > 0.
    if (delta > 0)
    {
        *pduDelta = TextDpi::ToTextDpi(static_cast<XFLOAT>(delta)  * scaleFactor);
    }
    else
    {
        *pduDelta = 0;
    }

    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetGlyphAlignmentStartLine
//
//  Synopsis:
//      Callback used by LS to get front bearing
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetGlyphAlignmentStartLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ GINDEX gindex,
        _In_ GPROP gprop,
        _Out_ LONG* pduDelta)
{
    Result::Enum txhr = Result::Success;

    IFC_FROM_HRESULT_RTS(LineServicesGetAlignmentHelper(pols, lsdev, lstflow, plsrun, gindex, NULL, TRUE /* start line */, pduDelta));

Cleanup:
    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetGlyphAlignmentEndLine
//
//  Synopsis:
//      Callback used by LS to get end bearing
//
//---------------------------------------------------------------------------

LSERR RichTextServices::LineServicesGetGlyphAlignmentEndLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ GINDEX gindex,
        _In_ GPROP gprop,
        _Out_ LONG* pduDelta)
{
    Result::Enum txhr = Result::Success;

    IFC_FROM_HRESULT_RTS(LineServicesGetAlignmentHelper(pols, lsdev, lstflow, plsrun, gindex, NULL, FALSE /* start line */, pduDelta));

Cleanup:
    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetCharAlignmentStartLine
//
//  Synopsis:
//      Callback used by LS to get front bearing
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesGetCharAlignmentStartLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ WCHAR wch,
        _Out_ LONG* pduDelta)
{
    Result::Enum txhr = Result::Success;
    GINDEX gindex;
    XUINT32 codePoint = (XUINT32) wch;
    LsRun *pRun;

    pRun = reinterpret_cast<LsRun *>(plsrun);
    IFC_FROM_HRESULT_RTS(pRun->GetFontFace()->GetGlyphIndices(&codePoint, 1, &gindex));

    IFC_FROM_HRESULT_RTS(LineServicesGetAlignmentHelper(pols, lsdev, lstflow, plsrun, gindex, NULL, TRUE /* start line */, pduDelta));

Cleanup:
    return LSErrFromResult(txhr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesGetCharAlignmentEndLine
//
//  Synopsis:
//      Callback used by LS to get end bearing
//
//---------------------------------------------------------------------------

LSERR RichTextServices::LineServicesGetCharAlignmentEndLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ WCHAR wch,
        _Out_ LONG* pduDelta)
{
    Result::Enum txhr = Result::Success;
    GINDEX gindex;
    XUINT32 codePoint = (XUINT32)wch;
    LsRun *pRun;

    pRun = reinterpret_cast<LsRun *>(plsrun);
    IFC_FROM_HRESULT_RTS(pRun->GetFontFace()->GetGlyphIndices(&codePoint, 1, &gindex));

    IFC_FROM_HRESULT_RTS(LineServicesGetAlignmentHelper(pols, lsdev, lstflow, plsrun, gindex, NULL, FALSE /* start line */, pduDelta));

Cleanup:
    return LSErrFromResult(txhr);
}
