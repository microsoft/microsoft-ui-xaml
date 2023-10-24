// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <StringConversions.h>

using namespace RichTextServices;

bool CBlock::ValidateMargin(
    _In_ const CValue *pValue
    )
{
    bool isMarginValid = false;
    bool validateValue = false;
    XTHICKNESS thickness = { 0 };

    if (pValue->GetType() == valueObject)
    {
        CThickness *pThickness = do_pointer_cast<CThickness>(pValue->AsObject());
        thickness = pThickness->m_thickness;
        validateValue = TRUE;
    }
    else if (pValue->GetType() == valueString)
    {
        if (SUCCEEDED(ThicknessFromString(pValue->AsString(), &thickness)))
        {
            validateValue = TRUE;
        }
    }
    else if (pValue->GetType() == valueThickness)
    {
        thickness = *pValue->AsThickness();
        validateValue = TRUE;
    }

    // Negative margins are not allowed.
    if (validateValue)
    {
        if (thickness.left >= 0.0f &&
            thickness.right >= 0.0f &&
            thickness.top >= 0.0f &&
            thickness.bottom >= 0.0f)
        {
            isMarginValid = TRUE;
        }
    }

    return isMarginValid;
}

//------------------------------------------------------------------------
//  Summary:
//      SetValue override
//------------------------------------------------------------------------
_Check_return_ HRESULT CBlock::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Block_Margin)
    {
        if (!ValidateMargin(&args.m_value))
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    IFC_RETURN(CTextElement::SetValue(args));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Creates an instance of a Paragraph element and the backing
//      text container for it.
//------------------------------------------------------------------------
_Check_return_ HRESULT CParagraph::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_        CREATEPARAMETERS   *pCreate)
{
    HRESULT                hr                        = S_OK;
    CParagraph             *pParagraph               = NULL;

    *ppObject  = NULL;
    pParagraph = new CParagraph(pCreate->m_pCore);

    *ppObject = pParagraph;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//  Summary:
//      Destructor
//------------------------------------------------------------------------
CParagraph::~CParagraph()
{
    // Always release the collection object. The TextElementCollection dtor doesn't clear its content.
    ReleaseInterface(m_pInlines);
}

//------------------------------------------------------------------------
//  Summary:
//      GetValue override
//------------------------------------------------------------------------
_Check_return_ HRESULT CParagraph::GetValue(
    _In_  const CDependencyProperty *pdp,
    _Out_ CValue *pValue)
{
    if (pdp->GetIndex() == KnownPropertyIndex::Paragraph_Inlines &&
        m_pInlines == nullptr)
    {
        IFC_RETURN(CreateInlineCollection());
    }

    IFC_RETURN(CBlock::GetValue(pdp, pValue));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Creates an inline collection and sets that as a property.
//------------------------------------------------------------------------
_Check_return_ HRESULT CParagraph::CreateInlineCollection()
{
    HRESULT hr = S_OK;
    CInlineCollection   *pInlines = NULL;
    CREATEPARAMETERS createParameters(GetContext());

    IFCEXPECT(m_pInlines == NULL);

    IFC(CreateDO(&pInlines, &createParameters));
    IFC(SetValueByKnownIndex(KnownPropertyIndex::Paragraph_Inlines, pInlines));

Cleanup:
    ReleaseInterface(pInlines);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Applies whitespace compression rules on Paragraph contents as it enters the
//      tree.
//------------------------------------------------------------------------
_Check_return_ HRESULT CParagraph::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
         EnterParams        params)
{
    IFC_RETURN(CBlock::EnterImpl(pNamescopeOwner, params));
    IFC_RETURN(CompressInlinesWhitespace(m_pInlines));

    return S_OK;
}

//------------------------------------------------------------------------
//    Summary:
//        Retrieves the number of positions in the span.
//
//------------------------------------------------------------------------
void CParagraph::GetPositionCount(_Out_ XUINT32 *pcPositions)
{
    *pcPositions = 0;

    if (m_pInlines && !m_pInlines->empty())
    {
        m_pInlines->GetPositionCount(pcPositions);
    }
    else
    {
        *pcPositions = 2;
    }
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves a text run from a given character position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CParagraph::GetRun(
    _In_ XUINT32           characterPosition,
    _Out_opt_ const TextFormatting **ppTextFormatting,
    _Out_opt_ const InheritedProperties **ppInheritedProperties,
    _Out_opt_ TextNestingType  *pNestingType,
    _Out_opt_ CTextElement    **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR **ppCharacters,
    _Out_ XUINT32 *pcCharacters
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
                IFC_RETURN(m_pInlines->GetPositionCount(&length));
                ASSERT(characterPosition == 0 || characterPosition == length - 1);
#endif
                *ppNestedElement = this;
            }
        }
    }
    else
    {
        // Empty Paragraph just has start/end tag positions. This is the same behavior as Span.
        // We could return formatting here, but it is pointless since nested runs will pick up anything inherited.
        if (characterPosition >= 2)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        *ppCharacters = NULL;
        *pcCharacters = 1;

        if (ppTextFormatting)
        {
            *ppTextFormatting = NULL;
        }

        if (ppInheritedProperties)
        {
            *ppInheritedProperties = NULL;
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
//    Summary:
//        Retrieves the containing nested text element from a character position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CParagraph::GetContainingElement(
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
        // If the paragraph either doesn't have inlines or couldn't match a containing element in
        // the inline collection, hit test paragraph itself.
        IFC_RETURN(CTextElement::GetContainingElement(
                characterPosition,
                &pElement
            ));
    }

    *ppContainingElement = pElement;

    return S_OK;
}

 _Check_return_ HRESULT CParagraph::GetElementEdgeOffset(
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
        // Try nested inlines.
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
//  Synopsis:
//      Performs any modifications to the visual tree necessary as side effects when the
//      object is being cleared from the tree. The Paragraph itself has none, but if it has
//      InlineUIContainers they do.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParagraph::Shutdown()
{
    HRESULT hr = S_OK;
    CDependencyObject *pInline = NULL;

    if (m_pInlines)
    {
        XUINT32 numBlocks = m_pInlines->GetCount();
        for (XUINT32 i = 0; i < numBlocks; i++)
        {
            pInline = static_cast<CDependencyObject*>(m_pInlines->GetItemWithAddRef(i));

            CInlineUIContainer *pInlineUIENoRef = do_pointer_cast<CInlineUIContainer>(pInline);
            if (pInlineUIENoRef)
            {
                IFC(pInlineUIENoRef->Shutdown());
            }

            ReleaseInterface(pInline);
        }
    }

Cleanup:
    ReleaseInterface(pInline);

    RRETURN(hr);
}

