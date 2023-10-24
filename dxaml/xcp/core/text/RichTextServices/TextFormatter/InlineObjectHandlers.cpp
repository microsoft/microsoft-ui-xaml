// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InlineObjectHandlers.h"
#include "TxMath.h"
#include "TextDpi.h"
#include "LsTextLine.h"
#include "LsHostContext.h"
#include "TextRun.h"
#include "LsRun.h"

using namespace Ptls6;
using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      LineServicesInlineCreateILSObj
//
//  Synopsis:
//      Callback used by line services to create inline object context.
//
//---------------------------------------------------------------------------
LSERR RichTextServices::LineServicesInlineCreateILSObj(
    _In_ POLS pols,
        // LS host.
    _In_ PLSC plsc,
        // LS context.
    _In_ PCLSCBK pclscbk,
        // Application callbacks.
    _In_ LONG idObj,
        // Installed object ID.
    _Out_ PILSOBJ *ppilsobj
        // Object context.
    )
{
    Result::Enum txhr = Result::Success;
    LineServicesEmbeddedObjectContext *pObjectContext = NULL;
    LsObjectId::Enum objectId = static_cast<LsObjectId::Enum>(idObj);
    PILSOBJ pilsobj = NULL;

    switch (objectId)
    {    
    case LsObjectId::TextEmbeddedObject:
        IFCTEXT(ResultFromLSErr(LsAllocMemory(plsc, sizeof(*(pObjectContext)), reinterpret_cast<void **>(&pObjectContext))));
        pObjectContext = new (pObjectContext) LineServicesEmbeddedObjectContext();
        pObjectContext->pols = pols;
        pObjectContext->plsc = plsc;
        pilsobj = static_cast<PILSOBJ>(pObjectContext);
        *ppilsobj = pilsobj;
        pilsobj = NULL;
        pObjectContext = NULL;
        break;

    default:
        // Unexpected obj ID.
        txhr = Result::Unexpected;
        break;
    }
    Cleanup:
    if (pObjectContext != NULL)
    {
            LsDestroyMemory(pObjectContext->plsc, pObjectContext);
    }
    return LSErrFromResult(txhr);
}

void LineServicesEmbeddedObjectContext::Destroy() 
{
    LsDestroyMemory(this->plsc, this);
}

LSERR LineServicesEmbeddedObjectContext::CreateLNObj(
    _In_ PLSPARACLIENT plsparaclient,
        // Pointer to client's paragraph info.
    _In_ PCLSDEVINFO plsdevinf,
        // Presentation-from-reference info.
    _Out_ BOOL& fSublinesInside,
        // True if our object can contain nested sublines. FALSE in our case.
    _Out_ BOOL& fProhibitHyphenation,
        // Is hyphenation permitted within this object. LS will only use this value if fSublinesInside=TRUE, which is not our case. 
    _Out_ BOOL& fTransparentForSpans,
        // Whether this object can contain nested spans. LS will only use this value if fSublinesInside=TRUE, which is not our case. 
    _Out_ BOOL& fRestrictBreakByIdObj,
        // Use break opportunities only inside or around of my idobj
    _Outptr_ CLsObjectLineContext** pplnobj)
{
    Result::Enum txhr = Result::Success;
    LineServicesEmbeddedObjectLineContext *pObjectLineContext = NULL;

    IFCTEXT(ResultFromLSErr(LsAllocMemory(plsc, sizeof(*(pObjectLineContext)), reinterpret_cast<void **>(&pObjectLineContext))));
    pObjectLineContext = new (pObjectLineContext) LineServicesEmbeddedObjectLineContext();
    pObjectLineContext->pObjectContext = this;
    pObjectLineContext->plsparaclient = plsparaclient;
    *pplnobj = static_cast<CLsObjectLineContext *>(pObjectLineContext);
    fSublinesInside = fFalse;
    fProhibitHyphenation = fTrue; // Shouldn't be used since there are no sublines.
    fTransparentForSpans = fTrue; // Shouldn't be used since there are no sublines.
    fRestrictBreakByIdObj = fFalse;

    pObjectLineContext = NULL;

Cleanup:
    if (pObjectLineContext != NULL)
    {
        LsDestroyMemory(pObjectLineContext->pObjectContext->plsc, pObjectLineContext);
    }
    return LSErrFromResult(txhr);
}

void LineServicesEmbeddedObjectLineContext::Destroy() 
{
    LsDestroyMemory(this->pObjectContext->plsc, this);
}

LSERR LSAPI LineServicesEmbeddedObjectLineContext::FormatSimple(
    _In_ const FMTIN& fmtinput
    )
{
    Result::Enum txhr = Result::Success;
    ObjectRunMetrics metrics;
    LineServicesEmbeddedObject *pObject = NULL; 
    XFLOAT rightMargin = 0;
    XFLOAT remainingWidth = 0;
    LsRun *pLsRun = NULL;
    ObjectRun *pObjectRun = NULL;
    LsTextLine *pLine = NULL;
    LsHostContext *pLsHostContext = NULL;
    XPOINTF position;
    XFLOAT extraSpace = 0.0f;


    pLsRun = reinterpret_cast<LsRun *>(fmtinput.lsfrun.plsrun);
    IFC_EXPECT_RTS(pLsRun);
    pObjectRun = reinterpret_cast<ObjectRun *>(const_cast<TextRun *>(pLsRun->GetTextRun()));
    IFC_EXPECT_RTS(pObjectRun);

    pLine = reinterpret_cast<LsTextLine *>(plsparaclient);
    IFC_EXPECT_RTS(pLine);

    pLsHostContext = reinterpret_cast<LsHostContext *>(pObjectContext->pols);
    IFC_EXPECT_RTS(pLsHostContext);

    IFCTEXT(ResultFromLSErr(LsAllocMemory(pObjectContext->plsc, sizeof(*(pObject)), reinterpret_cast<void **>(&pObject))));
    pObject = new (pObject) LineServicesEmbeddedObject();
    pObject->pObjectContext = pObjectContext;
    pObject->penPositionX = fmtinput.lsfgi.urPen;
    pObject->penPositionY = fmtinput.lsfgi.vrPen;
    pObject->plsparaclient = plsparaclient;

    // Calculate remaining width.
    rightMargin = TextDpi::FromTextDpi(fmtinput.lsfgi.urColumnMax);
    remainingWidth = pLine->GetFormattingWidth() - rightMargin;

    // Format embedded object.
    position.x = TextDpi::FromTextDpi(fmtinput.lsfgi.urPen);
    position.y = TextDpi::FromTextDpi(fmtinput.lsfgi.vrPen);
    IFCTEXT(pObjectRun->Format(pLsHostContext->pTextSource,
                            remainingWidth,
                            position,
                            &metrics));     

    // Save breaking info, object dimensions.
    pObject->isPenPositionUsed = !(pObjectRun->HasFixedSize());

    // Inline objects occupy 2 characters, return the "break before" from the first and the
    // "break after" from the second.
    ASSERT(pLsRun->GetLength() == 2);
    pObject->breakBefore = LineBreaking::DWriteBreakToLSBrkCond(pLsRun->GetLineBreakpoint(0).breakConditionBefore);
    pObject->breakAfter = LineBreaking::DWriteBreakToLSBrkCond(pLsRun->GetLineBreakpoint(1).breakConditionAfter);

    pObject->objdim.dur = TextDpi::ToTextDpi(metrics.width);
    pObject->objdim.heightsRef.dvMultiLineHeight = TextDpi::ToTextDpi(metrics.height);
    pObject->objdim.heightsRef.dvAscent = TextDpi::ToTextDpi(metrics.baseline);
    pObject->objdim.heightsRef.dvDescent = pObject->objdim.heightsRef.dvMultiLineHeight - pObject->objdim.heightsRef.dvAscent;
    pObject->objdim.heightsPres = pObject->objdim.heightsRef;

    // Adjust for any required leading spacing and character spacing.
    extraSpace = pLsRun->GetInitialSpacing() / CharacterSpacingScale
                * pObjectRun->GetProperties()->GetFontSize();

    if (pLsRun->GetSpaceLastCharacter())
    {
        extraSpace += pObjectRun->GetProperties()->GetCharacterSpacing() / CharacterSpacingScale
                    * pObjectRun->GetProperties()->GetFontSize();
    }
    
    if (extraSpace != 0.0f)
    {
        pObject->objdim.dur += TextDpi::ToTextDpi(extraSpace);
    }

    // Call LsdnFinishRegular to create dnode.
    // NOTE: ls will assume that an object run has length 1.
    IFCTEXT(ResultFromLSErr(LsdnFinishWordRegular(fmtinput.plsdnTop,
                                            pObjectRun->GetLength(),
                                            fmtinput.lsfrun.plsrun,
                                            fmtinput.lsfrun.plschp,
                                            static_cast<PDOBJ>(pObject),
                                        &pObject->objdim,
                                            FALSE, // fStopped
                                            FALSE, // fFillLine
                                            pObject->isPenPositionUsed ? 1 : 0
                                            )));

    pObject = NULL;

Cleanup:
    if (pObject != NULL)
    {
        LsDestroyMemory(pObject->pObjectContext->plsc, pObject);
        delete pObject;
    }
    return LSErrFromResult(txhr);
}

LSERR LineServicesEmbeddedObject::Display(
    _In_ PCDISPIN pdispin
    )
{
    // Inline objects do not render content directly. 
    // They are rendering though UIElement tree render walk.
    return lserrNone;
}

void LineServicesEmbeddedObject::Destroy()
{
    LsDestroyMemory(this->pObjectContext->plsc, this);
}

LSERR LineServicesEmbeddedObject::ProposeBreakAfter(
    _Out_ BRKCOND* pbrkcond)
{
    *pbrkcond = breakAfter;
    return lserrNone;
}

LSERR LineServicesEmbeddedObject::ProposeBreakBefore(
    _Out_ BRKCOND* pbrkcond)
{
    *pbrkcond = breakBefore;
    return lserrNone;
}


