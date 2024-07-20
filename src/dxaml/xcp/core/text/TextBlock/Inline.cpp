// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//  Summary:
//      Base class implementation of GetElementEdgeOffset for flat inlines.
//      Inlines supporting nesting (Span) need to override to search
//      their nested collections.
//------------------------------------------------------------------------
_Check_return_ HRESULT CInline::GetElementEdgeOffset(
    _In_ CTextElement *pElement,
    _In_ ElementEdge edge,
    _Out_ XUINT32 *pOffset,
    _Out_ bool *pFound
)
{
    XUINT32 offset = 0;
    bool found = false;

    if (pElement == this)
    {
        found = TRUE;
        IFC_RETURN(GetOffsetForEdge(edge, &offset));
    }

    *pOffset = offset;
    *pFound = found;

    return S_OK;
}

_Check_return_ HRESULT
CRun::Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;
    CRun* _this = NULL;

    _this = new CRun(pCreate->m_pCore);
    if (pCreate->m_value.GetType() == valueString)
    {
        IFC(_this->SetText(
            pCreate->m_value.AsString(),
            RunFlags(
                RunFlagsSpaceDefault |  // Compress whitespace
                RunFlagsContent)));         // This run text came from xml content

    }
    else
    {
        // Note: We're also allowing valueBool because
        // CTextElementRecord::CreateInstance will pass TRUE to indicate to
        // certain text elements (like CParagraph) that they shouldn't
        // create a backing store.  As you can see from the standard
        // DECLARE_CREATE code copied below, CRun used to ignore anything
        // that wasn't a string.  We're going to continue being conservative
        // here and only allow Any, String, and Bool.
        IFCEXPECT(
            pCreate->m_value.GetType() == valueAny ||
            pCreate->m_value.GetType() == valueBool);
    }

    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Constructor.
//------------------------------------------------------------------------
CSpan::CSpan(_In_ CCoreServices *pCore) : CInline(pCore)
{
    m_pInlines = NULL;
}

//------------------------------------------------------------------------
//  Summary:
//      Destructor.
//------------------------------------------------------------------------
CSpan::~CSpan()
{
    // Need to clear the inlines collection only if this span is not in a backing store
    // (i.e. only when it's not in a RichTextBox).
    if (m_pInlines)
    {
        IGNOREHR(m_pInlines->Clear());
        VERIFYHR(m_pInlines->RemoveParent(this));
    }

    ReleaseInterface(m_pInlines);
}

//------------------------------------------------------------------------
//  Summary:
//      GetValue override used to lazily create m_pInlines on retrieval.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSpan::GetValue(
    _In_  const CDependencyProperty *pdp,
    _Out_ CValue *pValue
)
{
    if (pdp->GetIndex() == KnownPropertyIndex::Span_Inlines &&
        m_pInlines == nullptr)
    {
        IFC_RETURN(CreateInlines());
    }

    IFC_RETURN(CInline::GetValue(pdp, pValue));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the number of positions in the span.
//
//------------------------------------------------------------------------
void CSpan::GetPositionCount(_Out_ XUINT32 *pcPositions)
{
    if (m_pInlines && !m_pInlines->empty())
    {
        m_pInlines->GetPositionCount(pcPositions);
    }
    else
    {
        // A completely empty inline has two reserved positions
        *pcPositions = 2;
    }
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves a text run from a given character position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSpan::GetRun(
    _In_                              XUINT32               characterPosition,
    _Out_opt_                   const TextFormatting      **ppTextFormatting,
    _Out_opt_                   const InheritedProperties **ppInheritedProperties,
    _Out_opt_                         TextNestingType      *pNestingType,
    _Out_opt_                         CTextElement        **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
    _Out_                             XUINT32              *pcCharacters
)
{
    if (m_pInlines && !m_pInlines->empty())
    {
        IFC_RETURN(m_pInlines->GetRun(
            characterPosition,
            ppTextFormatting,
            ppInheritedProperties,
            pNestingType,
            ppNestedElement,
            ppCharacters,
            pcCharacters
        ));

        // Inline collection for its boundaries cannot set ppNestedElement, since it does not
        // have information about its parent. Set ppNestedElement, if it wasn't already set.
        if (ppNestedElement != NULL)
        {
            if (*ppNestedElement == NULL &&
                pNestingType &&
                (*pNestingType == OpenNesting || *pNestingType == CloseNesting))
            {
#if DEBUG
                XUINT32 length;
                m_pInlines->GetPositionCount(&length);
                ASSERT(characterPosition == 0 || characterPosition == length - 1);
#endif
                *ppNestedElement = this;
            }
        }
    }
    else
    {
        // A completely empty inline has two reserved positions
        if (characterPosition >= 2)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        *ppCharacters = NULL;
        *pcCharacters = 1;

        if (ppTextFormatting)
        {
            IFC_RETURN(GetTextFormatting(ppTextFormatting));
        }

        if (ppInheritedProperties)
        {
            IFC_RETURN(GetInheritedProperties(ppInheritedProperties));
        }

        if (pNestingType)
        {
            *pNestingType = ((characterPosition == 0) ? OpenNesting : CloseNesting);
        }

        if (ppNestedElement)
        {
            *ppNestedElement = this;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves a text run from a given character position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSpan::GetContainingElement(
    _In_ XUINT32 characterPosition,
    _Outptr_ CTextElement **ppContainingElement
    )
{
    CTextElement *pElement = NULL;

    if (m_pInlines)
    {
        IFC_RETURN(m_pInlines->GetContainingElement(
            characterPosition,
            &pElement
        ));
    }

    if (pElement == NULL)
    {
        // If the span either doesn't have inlines or couldn't match a containing element in
        // the inline collection, hit test span itself.
        IFC_RETURN(CTextElement::GetContainingElement(
                characterPosition,
                &pElement
            ));
    }

    *ppContainingElement = pElement;

    return S_OK;
}

 _Check_return_ HRESULT CSpan::GetElementEdgeOffset(
    _In_ CTextElement *pElement,
    _In_ ElementEdge edge,
    _Out_ XUINT32 *pOffset,
    _Out_ bool *pFound
)
{
    XUINT32 offset = 0;
    bool found = false;

    if (pElement == this)
    {
        found = TRUE;
        IFC_RETURN(GetOffsetForEdge(edge, &offset));
    }
    else
    {
        // Try nested inlines - no need to offset by 1, since even though Span reserves the
        // first position, InlineCollection does too.
        if (m_pInlines)
        {
            IFC_RETURN(m_pInlines->GetElementEdgeOffset(
                pElement,
                edge,
                &offset,
                &found));
        }
    }

    *pOffset = offset;
    *pFound = found;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Creates the inlines property, using SetValue to ensure
//            that the property system knows the property is not defaulted.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSpan::CreateInlines()
{
    HRESULT              hr                  = S_OK;
    CInlineCollection   *pInlines            = NULL;
    CREATEPARAMETERS     createParameters(GetContext());

    IFCEXPECT_ASSERT(m_pInlines == NULL);

    IFC(CreateDO(&pInlines, &createParameters));
    IFC(SetValueByKnownIndex(KnownPropertyIndex::Span_Inlines, pInlines));

Cleanup:
    ReleaseInterface(pInlines);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Creates an Underline element.
//
//  Remarks:
//      We set the underline's TextDecorations property using SetValue
//      so that it's marked as set (for inheritance).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUnderline::Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate
    )
{
    HRESULT              hr                    = S_OK;
    CUnderline          *pUnderline            = NULL;
    CValue               decorationsUnderline;

    pUnderline = new CUnderline(pCreate->m_pCore);

    decorationsUnderline.Set(DirectUI::TextDecorations::Underline);
    IFC(pUnderline->SetValueByKnownIndex(KnownPropertyIndex::TextElement_TextDecorations, decorationsUnderline));

    *ppObject = pUnderline;
    pUnderline = NULL;

Cleanup:
    ReleaseInterface(pUnderline);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Creates an Italic element.
//
//  Remarks:
//      We set the italic's FontStyle property using SetValue
//      so that it's marked as set (for inheritance).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CItalic::Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate
    )
{
    HRESULT              hr           = S_OK;
    CItalic             *pItalic      = NULL;
    CValue               styleItalic;

    pItalic = new CItalic(pCreate->m_pCore);

    styleItalic.Set(DirectUI::FontStyle::Italic);
    IFC(pItalic->SetValueByKnownIndex(KnownPropertyIndex::TextElement_FontStyle, styleItalic));

    *ppObject = pItalic;
    pItalic = NULL;

Cleanup:
    ReleaseInterface(pItalic);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Creates a Bold element.
//
//  Remarks:
//      We set the bold's FontWeight property using SetValue
//      so that it's marked as set (for inheritance).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBold::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_        CREATEPARAMETERS   *pCreate
    )
 {
    HRESULT              hr            = S_OK;
    CBold               *pBold         = NULL;
    CValue               weightBold;

    pBold = new CBold(pCreate->m_pCore);

    weightBold.Set(DirectUI::CoreFontWeight::Bold);
    IFC(pBold->SetValueByKnownIndex(KnownPropertyIndex::TextElement_FontWeight, weightBold));

    *ppObject = pBold;
    pBold = NULL;

Cleanup:
    ReleaseInterface(pBold);
    RRETURN(hr);

 }
