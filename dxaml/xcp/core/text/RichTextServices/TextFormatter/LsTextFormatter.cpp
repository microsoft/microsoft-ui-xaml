// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LsTextFormatter.h"
#include "LsTextLine.h"
#include "TextStore.h"
#include "LineServicesCallbacks.h"
#include "InlineObjectHandlers.h"
#include "TextDpi.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;
using namespace Ptls6;

static const LSIMETHODS g_lsIMethodsInline =
{
    LineServicesInlineCreateILSObj
};

static LSIMETHODS g_objectHandlers[LsObjectId::ObjectIdMax];

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::LsTextFormatter
//
//  Synopsis:
//      Constructor.
//
//-----------------------------------------------------------------------
LsTextFormatter::LsTextFormatter(
    _In_ IFontAndScriptServices *pFontAndScriptServices
        // Provides an interface to access font and script specific data
) : m_lsHostContext(pFontAndScriptServices),
    m_pLsContext(NULL)
{
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::~LsTextFormatter
//
//  Synopsis:
//      Release resorces associated with the TextFormatter.
//
//-----------------------------------------------------------------------
LsTextFormatter::~LsTextFormatter()
{
    if (m_pLsContext != NULL)
    {
        ReleaseLsContext();
    }
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::FormatLine
//
//  Synopsis:
//      Creates a TextLine that is used for formatting and displaying text content.
//
//-----------------------------------------------------------------------
Result::Enum LsTextFormatter::FormatLine(
    _In_ TextSource *pTextSource, 
        // The text source for the line.
    _In_ XUINT32 firstCharIndex, 
        // A value that specifies the character index of the starting character in the line.
    _In_ XFLOAT wrappingWidth,
        // A value that specifies the width at which the line should wrap.
    _In_ TextParagraphProperties *pTextParagraphProperties, 
        // A value that represents paragraph properties, such as alignment, or indentation.
    _In_opt_ TextLineBreak *pPreviousLineBreak, 
        // A TextLineBreak value that specifies the text formatter state, in terms of where 
        // the previous line in the paragraph was broken by the text formatting process.
    _In_opt_ TextRunCache *pTextRunCache, 
        // Provides run caching services in order to improve performance.
    _Outptr_ TextLine **ppTextLine
        // A TextLine that represents a line of text that can be displayed.
     )
{
    Result::Enum txhr = Result::Success;
    LsTextLine *pLine;
    LsTextLineBreak *pPreviousLsLineBreak;
    TextStore *pTextStore = NULL;
    bool localRunCache = false;

    IFC_EXPECT_RTS(pTextSource);
    IFC_EXPECT_RTS(pTextParagraphProperties);

    if (m_pLsContext == NULL)
    {
        IFCTEXT(CreateLsContext());
    }

    // Assume that formatter was called with Ls line break.
    pPreviousLsLineBreak = reinterpret_cast<LsTextLineBreak *>(pPreviousLineBreak);

    if (pTextRunCache == NULL)
    {
        IFCTEXT(TextRunCache::Create(&pTextRunCache));
        localRunCache = TRUE;
    }
    pTextStore = reinterpret_cast<TextStore *>(pTextRunCache);
    IFC_EXPECT_RTS(pTextStore);
    IFCTEXT(pTextStore->Initialize(m_lsHostContext.pFontAndScriptServices, pTextSource, pTextParagraphProperties));

    // Validate wrapping width and calculate max width at which line is formatted. This may not be the same as 
    // the wrapping width, since line is formatted at max width constraint if wrapping is turned off.
    wrappingWidth = ValidateWrappingWidth(wrappingWidth);
    wrappingWidth = CalculateMaxLineWidth(wrappingWidth, pTextParagraphProperties);

    m_lsHostContext.pTextSource = pTextSource;
    m_lsHostContext.pTextStore = pTextStore;
    m_lsHostContext.pParagraphProperties = pTextParagraphProperties;
    m_lsHostContext.hasMultiCharacterClusters = FALSE;
    m_lsHostContext.lineStartIndex = firstCharIndex;
    m_lsHostContext.collapsedLine = FALSE;
    m_lsHostContext.useEmergencyBreaking = FALSE;
    m_lsHostContext.clipLastWordOnLine = FALSE;

    IFCTEXT(LsTextLine::Create(firstCharIndex, wrappingWidth, pPreviousLsLineBreak, this, &pLine));

    *ppTextLine = pLine;

Cleanup:
    m_lsHostContext.pTextSource = NULL;
    m_lsHostContext.pTextStore = NULL;
    m_lsHostContext.pParagraphProperties = NULL;
    m_lsHostContext.hasMultiCharacterClusters = FALSE;
    m_lsHostContext.lineStartIndex = 0;
    m_lsHostContext.collapsedLine = FALSE;
    m_lsHostContext.useEmergencyBreaking = FALSE;
    m_lsHostContext.clipLastWordOnLine = FALSE;
    
    if (localRunCache)
    {
        pTextStore->Release();
    }

    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::CreateLsContext
//
//  Synopsis:
//      Creates LS formatting context.
//
//---------------------------------------------------------------------------
Result::Enum LsTextFormatter::CreateLsContext()
{
    Result::Enum txhr = Result::Success;

    ASSERT(m_pLsContext == NULL);

    memset(&m_lsContextInfo, 0, sizeof(Ptls6::LSCONTEXTINFO));
    m_lsContextInfo.version                     = 6000;         // Version number
    m_lsContextInfo.pols                        = reinterpret_cast<Ptls6::POLS>(&m_lsHostContext); // Client data for this context
    m_lsContextInfo.fSpanFetchEnabled           = FALSE;        // Fetch with spans is allowed
    m_lsContextInfo.fCSSSpanFeatures            = FALSE;        // Enable CSS features with spans is allowed
    m_lsContextInfo.fDontSupportCompatTruncate  = TRUE;         // Do not use truncation even in truncation mode - recommended
    m_lsContextInfo.fDontReleaseRuns            = TRUE;         // Optimization - don't call pfnReleaseRun
    m_lsContextInfo.fCheckParaBoundaries        = FALSE;        // Don't call pfnCheckParaBoundaries
    m_lsContextInfo.fDrawTextRunsTogether       = FALSE;        // Display text runs using pfnDrawTextRunsTogether/pfnDrawGlyphRunsTogether callbacks.
                                                                // These callbacks were added in Ptls6 and are not implemented at present, so opt out.

    InitTextConfiguration(m_lsContextInfo.lstxtcfg);
    InitClientCallbacks(m_lsContextInfo.lscbk);
    IFCTEXT(InitObjectHandlers(&(m_lsContextInfo.cInstalledHandlers), &(m_lsContextInfo.pInstalledHandlers)));
    IFCTEXT(InitLineBreakingInfo(m_lsContextInfo.lsbrkinfo));
    IFCTEXT(ResultFromLSErr(Ptls6::LsCreateContext(&m_lsContextInfo, &m_pLsContext)));

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::ReleaseLsContext
//
//  Synopsis:
//      Releases LS formatting context.
//
//---------------------------------------------------------------------------
void LsTextFormatter::ReleaseLsContext()
{
    ASSERT(m_pLsContext != NULL);

    Ptls6::LsDestroyContext(m_pLsContext); 
    m_pLsContext = NULL;

    memset(&m_lsContextInfo, 0, sizeof(Ptls6::LSCONTEXTINFO));
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::InitLineBreakingInfo
//
//  Synopsis:
//      Initializes line breaking control for LS.
//
//---------------------------------------------------------------------------
Result::Enum LsTextFormatter::InitLineBreakingInfo(
    Ptls6::LSBREAKINGINFO &lsBreakingInfo
    ) const
{
    XUINT32 breakUnitsCount;
    XUINT32 breakClassesCount;
    const Ptls6::LSBRK *pBreakUnitsInfo;
    const XUINT8 *pBreakClassesInfo;

    LineBreaking::GetDWriteBreakingInfo(
        &breakUnitsCount,
        &breakClassesCount,
        &pBreakUnitsInfo,
        &pBreakClassesInfo
        );

    lsBreakingInfo.cbrk = breakUnitsCount;
    lsBreakingInfo.cbrkcls = breakClassesCount;
    lsBreakingInfo.rgbrk = pBreakUnitsInfo;
    lsBreakingInfo.rgibrk = pBreakClassesInfo;

    return Result::Success;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::InitClientCallbacks
//
//  Synopsis:
//      Initializes client callbacks for LS.
//
//---------------------------------------------------------------------------
void LsTextFormatter::InitClientCallbacks(
    Ptls6::LSCBK &lsCallbacks
    ) const
{
    Ptls6::LSCBK lsCbk = {
        LineServicesNewPtr,
        LineServicesDisposePtr,
        LineServicesReallocPtr,
        LineServicesFetchRun,
        NULL, // LineServicesGetAutoNumberInfo,
        NULL, // LineServicesGetNumericSeparators,
        NULL, // LineServicesCheckForDigit,
        LineServicesFetchTabs,
        LineServicesReleaseTabsBuffer,
        LineServicesGetBreakThroughTab,
        NULL, // LineServicesGetPosTabProps,
        LineServicesGetLastLineJustification, 
        NULL, // LineServicesCheckParaBoundaries,
        LineServicesGetRunCharacterWidths,
        NULL, // LineServicesCheckRunKernability,
        NULL, // LineServicesGetRunCharKerning,
        LineServicesGetRunTextMetrics,
        LineServicesGetRunUnderlineInfo,
        NULL, // LineServicesGetRunSecondaryUnderlineInfo,
        LineServicesGetRunStrikethroughInfo,
        NULL, // LineServicesGetBorderInfo,
        NULL, // LineServicesReleaseRun,
        LineServicesReleaseRunBuffer,
        NULL, // LineServicesHyphenate,
        NULL, // LineServicesGetPrevHyphenOpp,
        NULL, // LineServicesGetNextHyphenOpp,
        NULL, // LineServicesGetHyphenInfo,
        LineServicesDrawUnderline,
        NULL, // LineServicesDrawSecondaryUnderline,
        LineServicesDrawStrikethrough,
        NULL, // LineServicesDrawBorder,
        LineServicesFInterruptUnderline,
        NULL, // LineServicesFInterruptSecondaryUnderline,
        LineServicesFInterruptStrikethrough,
        NULL, // LineServicesFInterruptShade,
        NULL, // LineServicesFInterruptBorder,
        NULL, // LineServicesShadeRectangle,
        LineServicesDrawTextRun,
        NULL, // LineServicesDrawSplatLine,
        LineServicesFInterruptShaping,
        LineServicesGetGlyphs,
        LineServicesGetGlyphPositions,
        LineServicesDrawGlyphs,
        LineServicesReleaseGlyphBuffers,
        NULL, // LineServicesGetGlyphExpansionInfo,
        NULL, // LineServicesGetGlyphExpansionInkInfo,
        NULL, // LineServicesGetGlyphRunInk,
        NULL, // LineServicesInterruptDrawing
        NULL, // LineServicesDrawTextRunsTogether
        NULL, // LineServicesDrawGlyphRunsTogether
        NULL, // LineServicesGetEms,
        NULL, // LineServicesPunctStartLine,
        NULL, // LineServicesModWidthOnRun,
        NULL, // LineServicesModWidthSpace,
        NULL, // LineServicesCompOnRun,
        NULL, // LineServicesCompWidthSpace,
        NULL, // LineServicesExpOnRun,
        NULL, // LineServicesExpWidthSpace,
        NULL, // LineServicesGetModWidthClasses,
        LineServicesGetBreakingClasses,
        NULL, // LineServicesFTruncateBefore,
        LineServicesCanBreakBeforeChar,
        LineServicesCanBreakAfterChar,
        NULL, // LineServicesFHangingPunct,
        NULL, // LineServicesGetSnapGrid,
        NULL, // LineServicesDrawEffects,
        NULL, // LineServicesFCancelHangingPunct,
        NULL, // LineServicesModifyCompAtLastChar,
        LineServicesGetDurMaxExpandRagged,
        NULL, // LineServicesGetCharExpansionInfoFullMixed,
        NULL, // LineServicesGetGlyphExpansionInfoFullMixed,
        NULL, // LineServicesGetCharCompressionInfoFullMixed,
        NULL, // LineServicesGetGlyphCompressionInfoFullMixed,
        LineServicesGetCharAlignmentStartLine,
        LineServicesGetCharAlignmentEndLine,
        LineServicesGetGlyphAlignmentStartLine,
        LineServicesGetGlyphAlignmentEndLine,
        LineServicesGetPriorityForGoodTypography,
        NULL, // LineServicesFetchSpanProperties,
        NULL, // LineServicesGetSpanInlineMbp,
        NULL, // LineServicesReleaseSpanClient,
        NULL, // LineServicesDrawSpanBackground,
        NULL, // LineServicesDrawSpanBorder,
        NULL, // LineServicesGetSpanLineHeightProperties,
        NULL, // LineServicesGetSpanOffset,
        NULL, // LineServicesReleaseMasterSpanQualifier
        NULL, // LineServicesEnumText,
        NULL, // LineServicesEnumTab,
        NULL, // LineServicesEnumPen,
        LineServicesGetObjectHandlerInfo,
        NULL, // LineServicesGetEllipsisInfo
        NULL, // LineServicesDrawEllipsis
        LineServicesAssertFailed
    };

    lsCallbacks = lsCbk;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::InitObjectHandlers
//
//  Synopsis:
//      Initializes installed object handlers for LS.
//
//---------------------------------------------------------------------------
Result::Enum LsTextFormatter::InitObjectHandlers(
    _Out_ LONG* pCount, 
        // Number of installed handlers.
    _Outptr_ PCLSIMETHODS *ppHandlers
        // Array of installed handlers.
    ) const
{
    Result::Enum txhr = Result::Success;

    IFCTEXT(ResultFromLSErr(LsGetReverseLsimethods(&g_objectHandlers[LsObjectId::ReverseObject])));
    g_objectHandlers[LsObjectId::TextEmbeddedObject] = g_lsIMethodsInline;

    *pCount = LsObjectId::ObjectIdMax;
    *ppHandlers = g_objectHandlers;

Cleanup:
    return txhr;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::InitTextConfiguration
//
//  Synopsis:
//      Initializes straight-text configuration data for LS.
//
//---------------------------------------------------------------------------
void LsTextFormatter::InitTextConfiguration(
    Ptls6::LSTXTCFG &lsTextConfig
    ) const
{
    Ptls6::LSTXTCFG lstxtcfg = {
        3,                                  // JustPriorityLim : Used for input array size for all four FullMixed justification callbacks (cPriorityLevelMax)
        0x0001,                             // wchUndef
        0x0000,                             // wchNull
        UNICODE_SPACE,                      // wchSpace
        0x002D,                             // wchHyphen
        0x0009,                             // wchTab
        0x0001,                             // wchPosTab                (undef)
        UNICODE_PARAGRAPH_SEPARATOR,        // wchEndPara1              (Unicode PS)
        0x0001,                             // wchEndPara2;             (undef)
        0x0001,                             // wchAltEndPara            (undef)
        UNICODE_LINE_SEPARATOR,             // wchEndLineInPara         Word "CCRJ"
        0x0001,                             // wchColumnBreak           (undef)
        0x0001,                             // wchSectionBreak          (undef)
        0x0001,                             // wchPageBreak             (undef)
        0x00a0,                             // wchNonBreakSpace         char code of non-breaking space
        0x2011,                             // wchNonBreakHyphen
        0x0001,                             // wchNonReqHyphen          discretionary hyphen                (0x00ad)
        0x2014,                             // wchEmDash
        0x2013,                             // wchEnDash
        UNICODE_EM_SPACE,                   // wchEmSpace
        UNICODE_EN_SPACE,                   // wchEnSpace
        UNICODE_THIN_SPACE,                 // wchNarrowSpace
        0x0082,                             // wchOptBreak              (WPF uses undef)
        0x0083,                             // wchNoBreak               (WPF uses undef)
        UNICODE_IDEOGRAPHIC_SPACE,          // wchFESpace
        0x200d,                             // wchJoiner
        0x200c,                             // wchNonJoiner
        0x0001,                             // wchToReplace             (undef)
        0x0001,                             // wchReplace               (undef)
        0x2050,                             // wchVisiNull              visi char for wch==wchNull
        0x2051,                             // wchVisiAltEndPara        visi char for end "table cell"
        // As of Ptls6 LS will pass visi endPara/endLineInPara characters back to request widths in pfnGetRunCharWidths callback. Formerly
        // LS assumed width of 1 LS unit for those characters. To maintain parity, we must still return that value in the callback. We also must 
        // ensure that visi characters are safe to use and don't have other uses, since LS is actively using them now. It is safe to use the same
        // character as the non-visi version here since we don't require visual representation of these characters.
        UNICODE_LINE_SEPARATOR,             // wchVisiEndLineInPara     visi char for wchEndLineInPara
        UNICODE_PARAGRAPH_SEPARATOR,        // wchVisiEndPara           visi char for "end para"
        0x2054,                             // wchVisiSpace             visi char for "space"
        0x2055,                             // wchVisiNonBreakSpace     visi char for wchNonBreakSpace
        0x2056,                             // wchVisiNonBreakHyphen    visi char for wchNonBreakHyphen
        0x2057,                             // wchVisiNonReqHyphen      visi char for wchNonReqHyphen
        0x2058,                             // wchVisiTab               visi char for "tab"
        0x0001,                             // wchVisiPosTab            visi char for "pos tab"
        0x2059,                             // wchVisiEmSpace           visi char for wchEmSpace
        0x205A,                             // wchVisiEnSpace           visi char for wchEnSpace
        0x205B,                             // wchVisiNarrowSpace       visi char for wchNarrowSpace
        0x205C,                             // wchVisiOptBreak          visi char for wchOptBreak
        0x205D,                             // wchVisiNoBreak           visi char for wchNoBreak
        0x205E,                             // wchVisiFESpace           visi char for wchOptBreak
        0x2029,                             // wchEscAnmRun             (Unicode PS)
        0x0001                              // wchPad                   (undef)
    };

    lsTextConfig = lstxtcfg;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::ValidateWrappingWidth
//
//  Synopsis:
//      Validates wrapping width by adjusting it against min/max 
//      permitted values to calculate the max format width for
//      the line.
//
//---------------------------------------------------------------------------
XFLOAT LsTextFormatter::ValidateWrappingWidth(
    _In_ XFLOAT wrappingWidth
        // Width at which the line wraps.
    )
{
    const XFLOAT maxParaWidth = TextDpi::FromTextDpi(tsduRestriction);
    const XFLOAT minParaWidth = TextDpi::FromTextDpi(1);

    if (wrappingWidth < 0.0f || wrappingWidth > maxParaWidth)
    {
        wrappingWidth = maxParaWidth;
    }
    else if (wrappingWidth < minParaWidth)
    {
        wrappingWidth = minParaWidth;
    }

    return wrappingWidth;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextFormatter::CalculateMaxLineWidth
//
//  Synopsis:
//      Calculates the max line width based on paragraph properties.
//
//---------------------------------------------------------------------------
XFLOAT LsTextFormatter::CalculateMaxLineWidth(
    _In_ XFLOAT paragraphWidth,
        // Width of the paragraph.
    _In_ TextParagraphProperties *pTextParagraphProperties
        // Paragraph formatting properties.
    )
{
    const XFLOAT maxParaWidth = TextDpi::FromTextDpi(tsduRestriction);
    
    // LS does not support no wrap, for such case, we need to
    // simulate formatting a line within an infinite paragraph width.
    if (pTextParagraphProperties->GetTextWrapping() == DirectUI::TextWrapping::NoWrap)
    {
        paragraphWidth = maxParaWidth;
    }

    return paragraphWidth;
}
