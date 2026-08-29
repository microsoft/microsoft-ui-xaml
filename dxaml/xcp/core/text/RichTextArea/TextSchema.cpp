// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ bool CTextSchema::IsInlineUIContainer(_In_ CTextElement *pElement)
{
    return pElement->OfTypeByIndex<KnownTypeIndex::InlineUIContainer>();
}

_Check_return_ bool CTextSchema::IsHyperlink(_In_ CTextElement *pElement)
{
    return pElement->OfTypeByIndex<KnownTypeIndex::Hyperlink>();
}

_Check_return_ bool CTextSchema::IsRun(_In_ CTextElement *pElement)
{
    return pElement->OfTypeByIndex<KnownTypeIndex::Run>();
}

_Check_return_ bool CTextSchema::IsSpan(_In_ CTextElement *pElement)
{
    return pElement->OfTypeByIndex<KnownTypeIndex::Span>();
}


//------------------------------------------------------------------------
//  Summary:
//      Checks whether a given TextElement can be part of a TextBlock's content tree.
//
//  Remarks:
//      InlineUIContainers and Hyperlinks are not supported in TextBlocks.
//
//      If the given element is a span, we walk its content (recursively, if necessary)
//      to ensure that it doesn't contain any unsupported elements.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSchema::TextBlockSupportsElement(
    _In_  CTextElement *pTextElement,
    _Out_ bool        *pResult)
{
    HRESULT            hr          = S_OK;
    CInlineCollection *pCollectionNoRef = NULL;
    CInline           *pInline     = NULL;

    // Assume it's not supported
    *pResult = FALSE;

    // Trivial case: hyperlinks or inline UI containers.
    if (IsInlineUIContainer(pTextElement))
    {
        goto Cleanup;
    }

    // Spans can contain unsupported elements, so we have to walk their inlines.
    if (IsSpan(pTextElement))
    {
        CSpan   *pSpan      = do_pointer_cast<CSpan>(pTextElement);
        XUINT32  numInlines = 0;
        CValue   value;

        IFC(pSpan->GetValue(pSpan->GetPropertyByIndexInline(KnownPropertyIndex::Span_Inlines), &value));
        pCollectionNoRef = do_pointer_cast<CInlineCollection>(value.AsObject());

        numInlines = pCollectionNoRef->GetCount();
        for (XUINT32 i = 0; i < numInlines; ++i)
        {
            pInline = do_pointer_cast<CInline>(pCollectionNoRef->GetItemDOWithAddRef(i));
            IFC(TextBlockSupportsElement(pInline, pResult));
            if (!*pResult)
            {
                goto Cleanup;
            }
            ReleaseInterface(pInline);
        }
    }

    // If we've come this far, we've encountered no unsupported elements.
    *pResult = TRUE;

Cleanup:
    ReleaseInterface(pInline);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether a given TextElement can be part of a Hyperlink's content tree.
//
//  Remarks:
//      Only runs and non-Hyperlink spans are supported in Hyperlinks.
//
//      If the given element is a span, we walk its content (recursively, if necessary)
//      to ensure that it doesn't contain any unsupported elements.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSchema::HyperlinkSupportsElement(
    _In_  CTextElement *pTextElement,
    _Out_ bool        *pResult)
{
    HRESULT            hr          = S_OK;
    CInlineCollection *pCollectionNoRef = NULL;
    CInline           *pInline     = NULL;

    // Assume it's not supported
    *pResult = FALSE;

    if (IsRun(pTextElement))
    {
        *pResult = TRUE;
    }
    else if (IsSpan(pTextElement) && !IsHyperlink(pTextElement))
    {
        // Spans can contain unsupported elements, so we have to walk their inlines.
        CSpan   *pSpan      = do_pointer_cast<CSpan>(pTextElement);
        XUINT32  numInlines = 0;
        CValue   value;

        IFC(pSpan->GetValue(pSpan->GetPropertyByIndexInline(KnownPropertyIndex::Span_Inlines), &value));
        pCollectionNoRef = do_pointer_cast<CInlineCollection>(value.AsObject());

        numInlines = pCollectionNoRef->GetCount();
        for (XUINT32 i = 0; i < numInlines; ++i)
        {
            pInline = do_pointer_cast<CInline>(pCollectionNoRef->GetItemDOWithAddRef(i));
            IFC(HyperlinkSupportsElement(pInline, pResult));
            if (!*pResult)
            {
                goto Cleanup;
            }
            ReleaseInterface(pInline);
        }

        // If we've come this far, we've encountered no unsupported elements in the span.
        *pResult = TRUE;
    }

Cleanup:
    ReleaseInterface(pInline);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the given TextElement can be added to the given InlineCollection.
//
//  Remarks:
//      If the InlineCollection is part of a TextBlock, it's subject to the constraints
//      on TextBlock elements (see <TextBlockSupportsElement>)
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSchema::InlineCollectionSupportsElement(
    _In_  CInlineCollection *pCollection,
    _In_  CTextElement      *pTextElement,
    _Out_ bool             *pResult)
{
    // Assume the inline is supported.
    *pResult = TRUE;

    if (IsInlineCollectionInElement(pCollection, KnownTypeIndex::TextBlock))
    {
        IFC_RETURN(TextBlockSupportsElement(pTextElement, pResult));
    }
    else if (IsInlineCollectionInElement(pCollection, KnownTypeIndex::Hyperlink))
    {
        IFC_RETURN(HyperlinkSupportsElement(pTextElement, pResult));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the given inline collection is part of the content tree
//      of an element with the given type.
//
//------------------------------------------------------------------------
_Check_return_ bool CTextSchema::IsInlineCollectionInElement(
    _In_ CInlineCollection *pCollection,
    _In_ KnownTypeIndex elementTypeIndex)
{
    CDependencyObject *pParent = pCollection->GetParentInternal(false);
    while (pParent)
    {
        if (pParent->OfTypeByIndex(elementTypeIndex))
        {
            return true;
        }

        pParent = pParent->GetParentInternal(false);
    }

    return false;
}

