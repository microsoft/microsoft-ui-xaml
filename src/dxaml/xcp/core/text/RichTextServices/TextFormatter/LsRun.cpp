// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines LsRun represented by a plsrun value dispatched to LS during FetchRun.

#include "precomp.h"
#include "LsRun.h"
#include "LsSpan.h"
#include "TextRunData.h"
#include "FontAndScriptServices.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::CreateFromTextRun
//
//  Synopsis:
//      Creates run from a TextRun
//
//---------------------------------------------------------------------------
Result::Enum LsRun::CreateFromTextRun(
    _In_ XUINT32 characterIndex,
    _In_ const TextRunData *pRunData,
    _In_ LsSpan *pMasterLsSpan,
    _Outptr_ LsRun **ppLsRun
)
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun = NULL;
    LsSpan *pLsSpan;
    Ptls6::LSDCP dcpRunOffset;
    TextRun *pTextRun = pRunData->GetTextRun();
    TextCharactersRun *pTextCharactersRun;
    XINT32 lsRunInitialSpacing = 0;

    dcpRunOffset = characterIndex - pTextRun->GetCharacterIndex();
    ASSERT(dcpRunOffset >= 0 &&
           (dcpRunOffset < static_cast<LONG>(pTextRun->GetLength()) || pTextRun->GetLength() == 0));

    if (dcpRunOffset == 0)
    {
        // pRunData->m_initialSPacing only applies at the first character
        // position described by pRunData.
        lsRunInitialSpacing = pRunData->m_initialSpacing;
    }

    IFC_OOM_RTS(pLsRun = new LsRun(
        characterIndex,
        pTextRun->GetLength() - dcpRunOffset,
        pTextRun,
        pRunData->m_pFontFace,
        pRunData->m_fontScale,
        pRunData->m_scriptAnalysis,
        pMasterLsSpan->GetBidiLevel(),
        pRunData->m_glyphBased,
        lsRunInitialSpacing,
        pRunData->m_spaceLastCharacter,
        pRunData->m_pNumberSubstitution
    ));

    // Copy any line breaking conditions into this LSRun.

    if (pRunData->m_lineBreakpoints != NULL)
    {
        IFC_OOM_RTS(pLsRun->m_lineBreakpoints = new PALText::LineBreakpoint[pLsRun->m_length]);
        memcpy(
            pLsRun->m_lineBreakpoints,
            pRunData->m_lineBreakpoints,
            sizeof(PALText::LineBreakpoint) * pLsRun->m_length
        );
    }

    switch (pTextRun->GetType())
    {
    case TextRunType::Text:
        pTextCharactersRun = reinterpret_cast<TextCharactersRun *>(pTextRun);
        // scriptAnalysis.shapes is an enum flag
        // Control characters should be treated as hidden, except for tab.
        if ((static_cast<XUINT32>(pRunData->m_scriptAnalysis.shapes) & static_cast<XUINT32>(FssScriptShapes::NoVisual)) &&
            !pTextCharactersRun->IsTab())
        {
            ASSERT(dcpRunOffset == 0);
            pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeHidden;
            pLsRun->m_lsrun.fetchout.hidden.dcpHidden           = pLsRun->m_length;
        }
        else
        {
            pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeText;
            pLsRun->m_lsrun.fetchout.text.prgwchRun             = const_cast<LPWSTR>(pTextCharactersRun->GetCharacters() + dcpRunOffset);
            pLsRun->m_lsrun.fetchout.text.cchRun                = pLsRun->m_length;
            pLsRun->m_lsrun.fetchout.text.plsrun                = reinterpret_cast<Ptls6::PLSRUN>(pLsRun);
            pLsRun->m_lsrun.fetchout.text.lschp.fGlyphBased     = pRunData->m_glyphBased;
            pLsRun->m_lsrun.fetchout.text.lschp.fUnderline      = pTextCharactersRun->GetProperties()->HasUnderline();
            pLsRun->m_lsrun.fetchout.text.lschp.fStrike         = pTextCharactersRun->GetProperties()->HasStrikethrough();

            // The fRetainShapes flag controls whether or not LS will break a cluster when it force breaks a line.
            // We'll set this TRUE for all glyph-based runs.  This will keep graphemes from being broken, but it will also keep ligatures together.
            // For a future version we may consider a more granular approach but this would require support from DWrite.
            pLsRun->m_lsrun.fetchout.text.lschp.fRetainShapes   = pRunData->m_glyphBased;

            ASSERT(pLsRun->m_bidiLevel == pRunData->m_bidiLevel);
        }
        break;

    case TextRunType::Hidden:
    case TextRunType::DirectionalControl:
        ASSERT(dcpRunOffset == 0);
        pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeHidden;
        pLsRun->m_lsrun.fetchout.hidden.dcpHidden           = pLsRun->m_length;
        break;

    case TextRunType::EndOfLine:
        ASSERT(dcpRunOffset == 0);
        ASSERT(pTextRun->GetLength() == 1);
        pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeText;
        pLsRun->m_lsrun.fetchout.text.cchRun                = pLsRun->m_length;
        pLsRun->m_lsrun.fetchout.text.prgwchRun             = const_cast<LPWSTR>(L"\x2028");    // CPP COMPLIANCE BUG: 19219316
        pLsRun->m_lsrun.fetchout.text.plsrun                = reinterpret_cast<Ptls6::PLSRUN>(pLsRun);
        break;

    case TextRunType::EndOfParagraph:
        ASSERT(dcpRunOffset == 0);
        ASSERT(pMasterLsSpan->GetType() == LsSpanType::Paragraph);

        pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeCloseSpan;
        pLsRun->m_lsrun.fetchout.spanclose.dcpTag           = pLsRun->m_length;
        pLsRun->m_lsrun.fetchout.spanclose.lsspan.cpFirst   = pMasterLsSpan->GetCharacterIndex();
        pLsRun->m_lsrun.fetchout.spanclose.lsspan.lsspq     = reinterpret_cast<Ptls6::LSSPANQUALIFIER>(pMasterLsSpan);
        pLsRun->m_lsrun.fetchout.spanclose.fActiveAfterPara = FALSE;

        // Master span is being closed, set its length.
        ASSERT(characterIndex >= pMasterLsSpan->GetCharacterIndex());
        pMasterLsSpan->SetLength(characterIndex - pMasterLsSpan->GetCharacterIndex());
        break;

    case TextRunType::Object:
        ASSERT(dcpRunOffset == 0);
        IFC_OOM_RTS(pLsSpan = new LsSpan(characterIndex, LsSpanType::TextEmbeddedObject, pMasterLsSpan));
        pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeLSObject;
        pLsRun->m_lsrun.fetchout.lsobject.dcpTag            = pLsRun->m_length;
        pLsRun->m_lsrun.fetchout.lsobject.lsspqMaster       = reinterpret_cast<Ptls6::LSSPANQUALIFIER>(pLsSpan);
        pLsRun->m_lsrun.fetchout.lsobject.idobj             = LsObjectId::TextEmbeddedObject;
        pLsRun->m_lsrun.fetchout.lsobject.plsrun            = reinterpret_cast<Ptls6::PLSRUN>(pLsRun);

        // Master spans for inline objects have 0 length, since there is no further nesting at this point.
        pLsSpan->SetLength(pLsRun->m_length);
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    *ppLsRun = pLsRun;
    pLsRun = NULL;

Cleanup:
    delete pLsRun;
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::CreateReverseObjectOpen
//
//  Synopsis:
//      Creates run indicating start of Reverse Object.
//
//---------------------------------------------------------------------------
Result::Enum LsRun::CreateReverseObjectOpen(
    _In_ XUINT32 characterIndex,
    _In_ XUINT8 bidiLevel,
    _Inout_ LsSpan **ppMasterLsSpan,
    _Outptr_ LsRun **ppLsRun
)
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun = NULL;
    LsSpan *pLsSpan = NULL;
    XUINT8 bidiSpanId;

    bidiSpanId = LsSpanType::BidiBase + bidiLevel;

    IFC_OOM_RTS(pLsRun = new LsRun(characterIndex, 0, bidiLevel));
    IFC_OOM_RTS(pLsSpan = new LsSpan(characterIndex, static_cast<LsSpanType::Enum>(bidiSpanId), *ppMasterLsSpan));
    pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeLSObject;
    pLsRun->m_lsrun.fetchout.lsobject.dcpTag            = 0;
    pLsRun->m_lsrun.fetchout.lsobject.lsspqMaster       = reinterpret_cast<Ptls6::LSSPANQUALIFIER>(pLsSpan);
    pLsRun->m_lsrun.fetchout.lsobject.idobj             = LsObjectId::ReverseObject;
    pLsRun->m_lsrun.fetchout.lsobject.plsrun            = reinterpret_cast<Ptls6::PLSRUN>(pLsRun);

    *ppLsRun = pLsRun;
    *ppMasterLsSpan = pLsSpan;
    pLsRun = NULL;
    pLsSpan = NULL;

Cleanup:
    delete pLsRun;
    delete pLsSpan;
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::CreateReverseObjectClose
//
//  Synopsis:
//      Creates run indicating end of Reverse Object.
//
//---------------------------------------------------------------------------
Result::Enum LsRun::CreateReverseObjectClose(
    _In_ XUINT32 characterIndex,
    _In_ XUINT8 bidiLevel,
    _Inout_ LsSpan **ppMasterLsSpan,
    _Outptr_ LsRun **ppLsRun
)
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun;

    ASSERT(LsSpanType::BidiBase + bidiLevel == (*ppMasterLsSpan)->GetType());

    IFC_OOM_RTS(pLsRun = new LsRun(characterIndex, 0, bidiLevel));
    pLsRun->m_lsrun.fetchruntype                        = Ptls6::fetchruntypeCloseSpan;
    pLsRun->m_lsrun.fetchout.spanclose.dcpTag           = 0;
    pLsRun->m_lsrun.fetchout.spanclose.lsspan.cpFirst   = (*ppMasterLsSpan)->GetCharacterIndex();
    pLsRun->m_lsrun.fetchout.spanclose.lsspan.lsspq     = reinterpret_cast<Ptls6::LSSPANQUALIFIER>(*ppMasterLsSpan);
    pLsRun->m_lsrun.fetchout.spanclose.fActiveAfterPara = FALSE;

    // Master span is closing, set its length.
    ASSERT(characterIndex > (*ppMasterLsSpan)->GetCharacterIndex());
    (*ppMasterLsSpan)->SetLength(characterIndex - (*ppMasterLsSpan)->GetCharacterIndex());

    *ppLsRun = pLsRun;
    *ppMasterLsSpan = (*ppMasterLsSpan)->GetParent();

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::LsRun
//
//  Synopsis:
//      Constructor.
//
//  Notes:
//      LsRun does not AddRef/Release GlyphTypeface, it assumes it is
//      valid as long as TextCore and GlyphTypefaceCache are alive.
//      GlyphTypfaceCache explicitly *deletes* all typefaces when it is torn
//      down, and since LsRuns are cached by text controls we can't guarantee
//      in what order they will be destroyed relative to the
//      GlyphTypefaceCache. Releasing a GlyphTypeface after the
//      typeface cache has deleted it will cause AV.

//
//---------------------------------------------------------------------------
LsRun::LsRun(
    _In_ XUINT32 characterIndex,
    _In_ XUINT32 length,
    _In_opt_ TextRun *pTextRun,
    _In_opt_ IFssFontFace *pFontFace,
    _In_ XFLOAT scale,
    _In_ const FssScriptAnalysis &scriptAnalysis,
    _In_ XUINT8 bidiLevel,
    _In_ bool glyphBased,
    _In_ XINT32 initialSpacing,
    _In_ bool  spaceLastCharacter,
    _In_opt_ IDWriteNumberSubstitution* pNumberSubstitution
    ) :
    m_pTextRun(pTextRun),
    m_pFontFace(pFontFace),
    m_fontScale(scale),
    m_characterIndex(characterIndex),
    m_length(length),
    m_scriptAnalysis(scriptAnalysis),
    m_bidiLevel(bidiLevel),
    m_lineBreakpoints(NULL),
    m_glyphBased(!!glyphBased),
    m_initialSpacing(initialSpacing),
    m_spaceLastCharacter(!!spaceLastCharacter),
    m_pPrevious(NULL),
    m_pNext(NULL)
{
    AddRefInterface(m_pFontFace);
    SetInterface(m_pNumberSubstitution, pNumberSubstitution);
    memset(&m_lsrun, 0, sizeof(m_lsrun));
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::LsRun
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
LsRun::LsRun(
    _In_ XUINT32 characterIndex,
    _In_ XUINT32 length,
    _In_ XUINT8 bidiLevel
    ) :
    m_pTextRun(NULL),
    m_pFontFace(NULL),
    m_characterIndex(characterIndex),
    m_length(length),
    m_fontScale(1.0f),
    m_bidiLevel(bidiLevel),
    m_lineBreakpoints(NULL),
    m_glyphBased(false),
    m_initialSpacing(0),
    m_spaceLastCharacter(TRUE),
    m_pPrevious(NULL),
    m_pNext(NULL),
    m_pNumberSubstitution(NULL)
{
    m_scriptAnalysis.script = 0;
    m_scriptAnalysis.shapes = FssScriptShapes::Default;

    memset(&m_lsrun, 0, sizeof(m_lsrun));
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::~LsRun
//
//  Synopsis:
//      Destructor.
//
//  Notes:
//      LsRun does not AddRef/Release GlyphTypeface, it assumes it is
//      valid as long as TextCore and GlyphTypefaceCache are alive.
//      GlyphTypfaceCache explicitly *deletes* all typefaces when it is torn
//      down, and since LsRuns are cached by text controls we can't guarantee
//      in what order they will be destroyed relative to the
//      GlyphTypefaceCache. Releasing a GlyphTypeface after the
//      typeface cache has deleted it will cause AV.
//
//---------------------------------------------------------------------------
LsRun::~LsRun()
{
    ReleaseInterface(m_pTextRun);
    ReleaseInterface(m_pFontFace);
    ReleaseInterface(m_pNumberSubstitution);
    delete[] m_lineBreakpoints;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::InitializeRunData
//
//  Synopsis:
//      Initialize data representing the run.
//
//---------------------------------------------------------------------------
void LsRun::InitializeRunData(
    _In_ Ptls6::LSCP cpCurrent,
    _In_ Ptls6::LSSPAN lsSpanMaster,
    _Out_ Ptls6::LSFETCHRESULT* pfetchres
)
{
    Ptls6::LSDCP dcpRunOffset;

    dcpRunOffset = cpCurrent - m_characterIndex;
    *pfetchres = m_lsrun;

    if (dcpRunOffset > 0)
    {
        // This may happen for text or open LS object span, if a line was terminated in the middle of a reverse object span.
        ASSERT(m_lsrun.fetchruntype == Ptls6::fetchruntypeText || m_lsrun.fetchruntype == Ptls6::fetchruntypeLSObject);
        if (m_lsrun.fetchruntype == Ptls6::fetchruntypeText)
        {
            ASSERT(dcpRunOffset >= 0
                && (dcpRunOffset < static_cast<LONG>(m_length) || m_length == 0)
                   );

            pfetchres->fetchout.text.prgwchRun  = m_lsrun.fetchout.text.prgwchRun + dcpRunOffset;
            pfetchres->fetchout.text.cchRun     = m_lsrun.fetchout.text.cchRun - dcpRunOffset;
        }
        else if (m_lsrun.fetchruntype == Ptls6::fetchruntypeLSObject)
        {
            // This should happen only for reverse objects, not inline objects.
            ASSERT(m_lsrun.fetchout.lsobject.idobj == LsObjectId::ReverseObject);
            pfetchres->fetchout.lsobject.dcpTag = dcpRunOffset;
        }
    }

    // For reverse object closing spans, the cpFirst of the master span should be updated to match the opening LS object span.
    // The same span qualifier may be used repeatedly if a span is broken across multiple lines, but cpFirst will not match.
    // Since LsRunCache only uses span qualifier to match spans, as long as that matches, first cp may be adjusted.
    if (m_lsrun.fetchruntype == Ptls6::fetchruntypeCloseSpan)
    {
        ASSERT(lsSpanMaster.lsspq == m_lsrun.fetchout.spanclose.lsspan.lsspq);
        pfetchres->fetchout.spanclose.lsspan.cpFirst = lsSpanMaster.cpFirst;
    }
}

//---------------------------------------------------------------------------
//
//  Returns:
//      Whether to apply CharacterSpacing after the last character in pCharacters.
//
//---------------------------------------------------------------------------
bool LsRun::GetSpaceLastCharacter(const WCHAR *pLastCharacter) const
{
    bool spaceLastCharacter = true; // Space the last character unless we determine otherwise

    if (    m_lsrun.fetchruntype == Ptls6::fetchruntypeText
        &&  pLastCharacter == m_lsrun.fetchout.text.prgwchRun + (m_length-1))
    {
        spaceLastCharacter = m_spaceLastCharacter;
    }

    return spaceLastCharacter;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRun::HasEqualProperties
//
//  Returns:
//      TRUE iff another LsRun has equal formatting properties.
//
//---------------------------------------------------------------------------
bool LsRun::HasEqualProperties(_In_ const LsRun *pRun) const
{
    return GetScriptAnalysis().script == pRun->GetScriptAnalysis().script &&
           GetScriptAnalysis().shapes == pRun->GetScriptAnalysis().shapes &&
           GetBidiLevel() == pRun->GetBidiLevel() &&
           GetTextRun() != NULL && pRun->GetTextRun() != NULL &&
           GetTextRun()->GetType() == TextRunType::Text && pRun->GetTextRun()->GetType() == TextRunType::Text &&
           GetFontFace()->Equals(pRun->GetFontFace());
}
