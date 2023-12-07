// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DOCollection.h>
#include <DOPointerCast.h>
#include <TextBlock.h>
#include <RichTextBlock.h>
#include "TextHighlighterCollection.h"

void CTextHighlighterCollection::OnCollectionChanged()
{
    auto owner = GetOwner();

    if (owner != nullptr)
    {
        if (auto textBlockOwner = do_pointer_cast<CTextBlock>(owner))
        {
            textBlockOwner->InvalidateRender();
        }
        else if (auto richTextBlockOwner = do_pointer_cast<CRichTextBlock>(owner))
        {
            richTextBlockOwner->InvalidateRender();
        }
        else
        {
            // TextHighlighterCollection has unsupported owner.  If TextHighlighterCollection is added
            // to new types in the future, it must be appropriate cast here to notify the parent type
            // of collection changes.
            ASSERT(FALSE);
        }
    }
}

_Check_return_ HRESULT
CTextHighlighterCollection::OnAddToCollection(
    _In_ CDependencyObject* dependencyObject)
{
    IFC_RETURN(CDOCollection::OnAddToCollection(dependencyObject));

    OnCollectionChanged();

    return S_OK;
}

_Check_return_ HRESULT
CTextHighlighterCollection::OnRemoveFromCollection(
    _In_ CDependencyObject* dependencyObject,
    int32_t previousIndex)
{
    IFC_RETURN(CDOCollection::OnRemoveFromCollection(dependencyObject, previousIndex));

    OnCollectionChanged();

    return S_OK;
}

_Check_return_ HRESULT
CTextHighlighterCollection::OnClear()
{
    IFC_RETURN(CDOCollection::OnClear());

    OnCollectionChanged();

    return S_OK;
}
