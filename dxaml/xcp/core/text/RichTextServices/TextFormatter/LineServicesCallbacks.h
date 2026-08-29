// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Static methods declared by TextFormatter as callbacks for LS.

#pragma once

using namespace Ptls6;

namespace RichTextServices
{
    // Callback used by LS to request memory allocation.
    __declspec(allocator) void * LineServicesNewPtr(
        _In_ POLS pols, 
        _In_ LONG cSize
        );

    // Callback used by LS to release memory.
    void LineServicesDisposePtr(
        _In_ POLS pols, 
        _In_ void *pAddress
        );

    // Callback used by LS to reallocate memory.
    __declspec(allocator) void * LineServicesReallocPtr(
        _In_ POLS pols, 
        _In_ void *pAddress,
        _In_ LONG cSize
        );

    // Callback used by LS to obtain the next run of characters.
    LSERR LineServicesFetchRun(
        _In_ POLS pols,
        _In_ PLSPARACLIENT plsparaclient,
        _In_ LSSPAN lsspanMaster,
        _In_ LSCP cpCurrent,
        _In_ BOOL fStyle,
        _In_opt_ PLSSTYLE pstyle,
        _In_ LSFETCHPOSITION fetchpos,
        _Out_ LSFETCHRESULT* pfetchres
        );

    // Callback used by LS to fetch tab properties.
    LSERR LineServicesFetchTabs(
        _In_ POLS pols,
        _In_ PLSPARACLIENT plsparaclient,
        _Out_ PLSTABS plstabs,
        _Out_ BOOL* pfHangingTab,
        _Out_ LONG* pdurHangingTab,
        _Out_ WCHAR* pwchHangingTabLeader);

    // Callback used by LS to release buffer for used-defined tab stops.
    LSERR LineServicesReleaseTabsBuffer(
        _In_ POLS pols,
        _In_ PLSPARACLIENT plsparaclient,
        _In_ LSTBD* plstbd);

    // Callback used by LS to adjust right margin in case of a user-defined tab stop past the right margin.
    LSERR LineServicesGetBreakThroughTab(
        _In_ POLS pols,
        _In_ PLSPARACLIENT plsparaclient,
        _In_ LONG urRightMargin,
        _In_ LONG urTabPos,
        _Out_ LONG* purRightMarginNew);

    // Callback used by LS to determine justification and alignment preferences for the last line in a 
    // paragraph.
    LSERR LineServicesGetLastLineJustification(
        _In_ POLS pols,
        _In_ PLSPARACLIENT plsparaclient, 
        _In_ LSKJUST lskj,
        _In_ BOOL fJustify,
        _In_ LSKALIGN lskal,
        _In_ ENDRES endr,
        _Out_ BOOL* pfJustifyLastLine, 
        _Out_ LSKALIGN* plskalLine);

    // Callback used by LS to retrieve the widths for all of the characters of a given run.
    LSERR LineServicesGetRunCharacterWidths(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ LSDEVICE lsdev,
        _In_reads_(cwchRun) LPCWSTR rgwchRun,
        _In_ LONG cwchRun,
        _In_ LONG duAvailable,
        _In_ LSTFLOW lstflow,
        _Out_writes_(cwchRun) LONG* rgdu,
        _Out_ LONG* pduTotal,
        _Out_ LONG* pcwchConsumed
        );

    // Callback used by LS to obtain text metrics for a run.
    LSERR LineServicesGetRunTextMetrics(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _Out_ PLSTXM plstxm
        );

    // Callback used by LS to obtain underline information for a run.
    LSERR LineServicesGetRunUnderlineInfo(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ PCHEIGHTS heightsPres,
        _In_ LSTFLOW lstflow,
        _Out_ PLSULINFO plsulnfo
        );

    // Callback used by LS to obtain strikethrough information for a run.
    LSERR LineServicesGetRunStrikethroughInfo(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ PCHEIGHTS heightsPres,
        _In_ LSTFLOW lstflow,
        _Out_ PLSSTINFO plsulnfo
        );

    // Callback used by LS to notify client that the run buffer is not being used any more.
    LSERR LineServicesReleaseRunBuffer(
        _In_ POLS pols,
        _In_ PLSPARACLIENT plsparaclient,
        _In_ PLSRUN plsrun,
        _In_ LPWSTR rgwchRun
        );

    // Callback used by LS to draw an underline
    LSERR LineServicesDrawUnderline(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ UINT kUlbase,
        _In_ PCLSPOINT pptStart,
        _In_ LONG dupFromStartLine,  /*  dup from start line to starting position of underline */
        _In_ LONG dupUL,
        _In_ LONG dvpUL,
        _In_ LSTFLOW lstflow,
        _In_ UINT kDisp,
        _In_ PCLSRECT prcClip
        );

    // Callback used by LS to draw a Strikethrough
    LSERR LineServicesDrawStrikethrough(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ UINT kUlbase,
        _In_ PCLSPOINT pptStart,
        _In_ LONG dupUL,
        _In_ LONG dvpUL,
        _In_ LSTFLOW lstflow,
        _In_ UINT kDisp,
        _In_ PCLSRECT prcClip
        );

    // Callback used by LS to to find out if the client wants the strikethrough 
    // to be broken between the two runs.
    LSERR LineServicesFInterruptUnderline(
        _In_ POLS pols,
        _In_ PLSRUN plsrunFirst,
        _In_ PLSRUN plsrunSecond,
        _Out_ BOOL* pfInterruptUnderline
        );


    // Callback used by LS to to find out if the client wants the underline 
    // to be broken between the two runs.
    LSERR LineServicesFInterruptStrikethrough(
        _In_ POLS pols,
        _In_ PLSRUN plsrunFirst,
        _In_ PLSRUN plsrunSecond,
        _Out_ BOOL* pfInterruptUnderline
        );

    // Callback used by LS to tell client to draw a text run.
    LSERR LineServicesDrawTextRun(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ BOOL fScaleDownFromRef, 
        _In_ PCLSPOINT pptText,
        _In_reads_(cwchRun) const WCHAR* rgwchRun,
        _In_reads_(cwchRun) const LONG* rgdupRun,
        _In_ LONG cwchRun,
        _In_ LSTFLOW lstflow,
        _In_ UINT kDisp,
        _In_ PCLSPOINT pptRun,
        _In_reads_(cwchRun) PCHEIGHTS heightsPres,
        _In_ LONG dupRun,
        _In_ PCLSRECT pRectClip
        );

    // Callback used by LS to find out whether two adjacent runs should be shaped together.
    LSERR LineServicesFInterruptShaping(
        _In_ POLS pols,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrunFirst,
        _In_ PLSRUN plsrunSecond,
        _Out_ BOOL* pfInterruptShaping
        );

    // Callback used by LS to obtain glyphs and character-to-glyph mapping information for glyph-based runs.
    LSERR LineServicesGetGlyphs(
        _In_ POLS pols,
        _In_reads_(crun) const PLSRUN* rgplsrun,
        _In_reads_(crun) const LONG* rgdwchRun,
        _In_ LONG crun,
        _In_reads_(cwch) LPCWSTR rgwch,
        _In_ LONG cwch,
        _In_ LSTFLOW lstflow,
        _Out_writes_(cwch) PGMAP rggmap,
        _Out_writes_(cwch) PCHPROP rgchprop,
        _Out_writes_(cwch) BOOL* rgfCanGlyphAlone,
        _Outptr_result_buffer_(*pcgindex) PGINDEX* prggindex,
        _Outptr_result_buffer_(*pcgindex) PGPROP* prggprop,
        _Out_ LONG* pcgindex
        );

    // Callback used by LS to obtain the widths and offsets of the glyphs that were produced in pfnGetGlyphs.
    LSERR LineServicesGetGlyphPositions(
        _In_ POLS pols,
        _In_reads_(crun) const PLSRUN* rgplsrun,
        _In_reads_(crun) const LONG* rgdwchRun,
        _In_ LONG crun,
        _In_ LSDEVICE lsdev,
        _In_reads_(cwch) LPWSTR rgwch,
        _In_reads_(cwch) PCGMAP rggmap,
        _In_reads_(cwch) PCCHPROP rgchprop,
        _In_ LONG cwch,
        _In_reads_(cgindex) PCGINDEX rggindex,
        _In_reads_(cgindex) PCGPROP rggprop,
        _In_ LONG cgindex,
        _In_ LSTFLOW lstflow,
        _Out_writes_(cgindex) LONG* rgdu,
        _Out_writes_(cgindex) PLSGOFFSET rgGoffset
        );

    // Callback used by LS to draw glyphs.
    LSERR LineServicesDrawGlyphs(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ BOOL fScaleDownFromRef, 
        _In_reads_(cwch) LPWSTR rgwch,
        _In_reads_(cwch) PCGMAP rggmap,
        _In_reads_(cwch) PCCHPROP rgchprop,
        _In_ LONG cwch,
        _In_reads_(cglyph) PCGINDEX pglyph,
        _In_reads_(cglyph) const LONG* rgdu,
        _In_reads_(cglyph) const LONG* rgduBeforeJust,
        _In_reads_(cglyph) PCLSGOFFSET rgGoffset,
        _In_reads_(cglyph) PCGPROP rggprop,
        _In_reads_(cglyph) PCEXPTYPE rgExpType,
        _In_ LONG cglyph,
        _In_ LSTFLOW lstflow,
        _In_ UINT kDisp,
        _In_ PCLSPOINT pptRun,
        _In_ PCHEIGHTS heightsPres,
        _In_ LONG dupRun,
        _In_ PCLSRECT pRectClip
        );

    // Callback used by LS to notify the client that the glyph indices and glyph property buffers are no longer being used.
    LSERR LineServicesReleaseGlyphBuffers(
        _In_ POLS pols,
        _In_ PGINDEX rggindex,
        _In_ PGPROP rggprop
        );

    // Callback used by LS to obtain the breaking classes of a character.
    LSERR LineServicesGetBreakingClasses(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ LSCP cp,
        _In_ WCHAR wch,
        _Out_ BRKCLS* pbrkclsFirst,
        _Out_ BRKCLS* pbrkclsSecond
        );

    // Callback used by LS to determine if a line break is permissible between preceding object and text.
    LSERR LineServicesCanBreakBeforeChar(
        _In_ POLS pols,
        _In_ BRKCLS brkcls,
        _Out_ BRKCOND* pbrktxtBefore
        );

    // Callback used by LS to determine if a line break is permissible between text and a following object.
    LSERR LineServicesCanBreakAfterChar(
        _In_ POLS pols,
        _In_ BRKCLS brkcls,
        _Out_ BRKCOND* pbrktxtAfter
        );

    // LineServicesGetDurMaxExpandRagged
    LSERR LineServicesGetDurMaxExpandRagged(
        _In_ POLS pols,
        _In_ PLSRUN plsrun,
        _In_ LSTFLOW lstflow,
        _Out_ LONG* pdurMaxExpandRagged
        );

    LSERR LineServicesGetCharAlignmentStartLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ WCHAR wch,
        _Out_ LONG* pduDelta);

    LSERR LineServicesGetCharAlignmentEndLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ WCHAR wch,
        _Out_ LONG* pduDelta);
     
    LSERR LineServicesGetGlyphAlignmentStartLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ GINDEX gindex,
        _In_ GPROP gprop,
        _Out_ LONG* pduDelta);
    
    LSERR LineServicesGetGlyphAlignmentEndLine(
        _In_ POLS pols,
        _In_ LSDEVICE lsdev,
        _In_ LSTFLOW lstflow,
        _In_ PLSRUN plsrun,
        _In_ GINDEX gindex,
        _In_ GPROP gprop,
        _Out_ LONG* pduDelta);

    // Callback used by LS to divide justification priorities into bad and good.
    LSERR LineServicesGetPriorityForGoodTypography(
        _In_ POLS pols,
        _Out_ LONG* pcPriorityForGoodTypographyLim
        );

    // Callback used by LS to get object handler data.
    LSERR LineServicesGetObjectHandlerInfo(
        _In_ POLS pols,
        _In_ LONG idObj,
        _Out_ void* pObjectInfo
        );

    // Callback used by LS as an assert handler. Only in Debugging API.
    void LineServicesAssertFailed(
        _In_ const char* pMessage, 
        _In_ const char* pFilename, 
        _In_ int line,
        _In_ DWORD err
        );
}
