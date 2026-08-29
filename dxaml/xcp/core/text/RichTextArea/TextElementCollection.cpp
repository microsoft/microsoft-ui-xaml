// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//---------------------------------------------------------------------------
//
//  Member:
//      CTextElementCollection Constructor
//
//---------------------------------------------------------------------------
CTextElementCollection::CTextElementCollection(_In_ CCoreServices *pCore)
    : CDOCollection(pCore)
{}

//---------------------------------------------------------------------------
//
//  Member:
//      CTextElementCollection Destructor
//
//---------------------------------------------------------------------------
CTextElementCollection::~CTextElementCollection()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      CTextElementCollection::AppendImpl
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextElementCollection::AppendImpl(
    _In_ CDependencyObject *pObject,
    _Out_ XUINT32 *pnIndex
    )
{
    CTextElement *pElement = do_pointer_cast<CTextElement>(pObject);
    if (pElement == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(ValidateTextElement(pElement));

    IFC_RETURN(CDOCollection::AppendImpl(pObject, pnIndex));

    IFC_RETURN(MarkDirty(nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CTextElementCollection::InsertImpl
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextElementCollection::InsertImpl(
    _In_ XUINT32 nIndex,
    _In_ CDependencyObject *pObject
    )
{
    CTextElement *pElement = do_pointer_cast<CTextElement>(pObject);
    if (pElement == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(ValidateTextElement(pElement));

    IFC_RETURN(CDOCollection::InsertImpl(nIndex, pObject));

    IFC_RETURN(MarkDirty(nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CTextElementCollection::RemoveAtImpl
//
//---------------------------------------------------------------------------
_Check_return_ CDependencyObject* CTextElementCollection::RemoveAtImpl(
    _In_ XUINT32 nIndex
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CDependencyObject* pResult = NULL;

    pResult = CDOCollection::RemoveAtImpl(nIndex);

    IFC(MarkDirty(nullptr));

Cleanup:
    return pResult;
}

//------------------------------------------------------------------------
//  Summary:
//      Marks the collection as dirty on clear.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElementCollection::OnClear()
{
    IFC_RETURN(CDOCollection::OnClear());

    IFC_RETURN(MarkDirty(nullptr));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when the content of the collection is changed. Notifies the
//      collection's parent that the inline has changed.
//
//  Notes:
//      Takes ownership of pLocalValueRecord on success only.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT CTextElementCollection::MarkDirty(
    _In_opt_ const CDependencyProperty *pdp
    )
{
    CDependencyObject *pParent = GetParentInternal(false);

    if (pParent != NULL)
    {
        if (pParent->OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            CTextBlock *pTextBlock = static_cast<CTextBlock*>(pParent);
            IFC_RETURN(pTextBlock->OnContentChanged(pdp));
        }
        else if (pParent->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock*>(pParent);
            IFC_RETURN(pRichTextBlock->OnContentChanged(pdp));
        }
        else if (pParent->OfTypeByIndex<KnownTypeIndex::TextElement>())
        {
            CTextElement *pElement = static_cast<CTextElement*>(pParent);
            IFC_RETURN(pElement->MarkDirty(pdp));
        }
        else if (pParent->OfTypeByIndex<KnownTypeIndex::TextElementCollection>())
        {
            CTextElementCollection *pCollection = static_cast<CTextElementCollection*>(pParent);
            IFC_RETURN(pCollection->MarkDirty(pdp));
        }
        else
        {
            // This assertion fires on unknown parent types. Add an explicit clause to
            // handle the parent type above.
            ASSERT(FALSE);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Validate a new TextElement that wants to enter this collection
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextElementCollection::ValidateTextElement(_In_ CTextElement *pTextElement)
{
    HRESULT hr = S_OK;
    RRETURN(hr);
}
