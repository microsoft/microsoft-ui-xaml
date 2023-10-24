// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::CBlockCollection
//
//------------------------------------------------------------------------
CBlockCollection::CBlockCollection(
    _In_ CCoreServices *pCore
    ) : CTextElementCollection(pCore),
    m_pLengths(NULL),
    m_length(0)
{
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::~CBlockCollection
//
//------------------------------------------------------------------------
CBlockCollection::~CBlockCollection()
{
    delete [] m_pLengths;
    m_pLengths = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetPositionCount
//
//  Synopsis: Returns the sum of the nested block position counts.
//
//------------------------------------------------------------------------
void CBlockCollection::GetPositionCount(
    _Out_ XUINT32 *pcPositions
    )
{
    if (m_pLengths == NULL)
    {
        CacheLengths();
    }

    *pcPositions = m_length;
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetRun
//
//  Synopsis: Returns a run of characters all of the same format starting
//            at the given position.
//
//  If the startPosition corresponds to the (reserved) start or end
//  position of an element, GetRun returns
//    1) *ppCharacters == NULL,
//    2) *pFormatting == NULL,
//    3) *pcCharacters == number of reserved positions (1 for start or end of Inline).
//
//  If the startPosition corresponds to a (UTF-16) code unit, GetRun
//  returns
//    1) *ppCharacters points to that code unit,
//    2) *pcCharacters is set to the length of the longest character run
//       that is contiguous in memory, and for which all code units share
//       the same formatting,
//    3) *pFormatting points to the format shared by the code units at
//       *ppCharacters. Inheritance of formatting properties is already
//       resolved.
//
//   If the start position is beyond the end of the collection, fail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBlockCollection::GetRun(
    _In_ XUINT32 characterPosition,
    _Out_opt_ const TextFormatting **ppTextFormatting,
    _Out_opt_ const InheritedProperties **ppInheritedProperties,
    _Out_opt_ TextNestingType *pNestingType,
    _Out_opt_ CTextElement **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR **ppCharacters,
    _Out_ XUINT32 *pcCharacters
    )
{
    XUINT32 blockIndex = 0;
    XUINT32 blockOffset = characterPosition;

    if (m_pLengths == NULL)
    {
        CacheLengths();
    }

    // If the collection is empty, m_pLengths may still be NULL.
    if (m_pLengths != NULL)
    {
        IFCEXPECT_RETURN(characterPosition < m_length);

        while (blockOffset >= m_pLengths[blockIndex])
        {
            blockOffset -= m_pLengths[blockIndex];
            blockIndex++;
            ASSERT(blockIndex < GetCount());
        }

        IFC_RETURN(static_cast<CBlock*>(GetCollection()[blockIndex])->GetRun(
            blockOffset,
            ppTextFormatting,
            ppInheritedProperties,
            pNestingType,
            ppNestedElement,
            ppCharacters,
            pcCharacters));
    }
    else
    {
        *ppCharacters = NULL;
        *pcCharacters = 0;

        if (ppTextFormatting)
        {
            *ppTextFormatting = NULL;
        }

        if (ppInheritedProperties)
        {
            *ppInheritedProperties = NULL;
        }

        if (ppNestedElement)
        {
            *ppNestedElement = NULL;
        }

        if (pNestingType)
        {
            *pNestingType = NestedContent;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetContainingElement
//
//  Synopsis: return the nested block containing a text position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBlockCollection::GetContainingElement(
    _In_ XUINT32 characterPosition,
    _Outptr_ CTextElement **ppContainingElement
    )
{
    XUINT32   blockIndex = 0;
    XUINT32   blockOffset = characterPosition;
    CTextElement *pElement = NULL;

    if (m_pLengths == NULL)
    {
        CacheLengths();
    }

    // If the collection is empty, m_pLengths may still be NULL.
    if (m_pLengths != NULL)
    {
        IFCEXPECT_RETURN(characterPosition <= m_length);

        if (characterPosition < m_length)
        {
            while (blockOffset >= m_pLengths[blockIndex])
            {
                blockOffset -= m_pLengths[blockIndex];
                blockIndex++;
                ASSERT(blockIndex < GetCount());
            }

            if (!empty())
            {
                IFC_RETURN(static_cast<CBlock*>(GetCollection()[blockIndex])->GetContainingElement(
                    blockOffset,
                    &pElement));
            }
        }
    }

    *ppContainingElement = pElement;

    return S_OK;
}

_Check_return_ HRESULT CBlockCollection::GetElementEdgeOffset(
    _In_ CTextElement *pElement,
    _In_ ElementEdge edge,
    _Out_ XUINT32 *pOffset,
    _Out_ bool *pFound
    )
{
    XUINT32 blockIndex = 0;
    bool found = false;
    XUINT32 count = GetCount();
    XUINT32 offsetInBlock = 0;
    XUINT32 blockLength = 0;

    // Start with an offset of 0, BlockCollection doesn't reserve any positions.
    XUINT32 offset = 0;

    while (blockIndex < count &&
        !found)
    {
        IFC_RETURN(static_cast<CBlock*>(GetCollection()[blockIndex])->GetElementEdgeOffset(
            pElement,
            edge,
            &offsetInBlock,
            &found));

        if (found)
        {
            offset += offsetInBlock;
            break;
        }
        else
        {
            static_cast<CBlock*>(GetCollection()[blockIndex])->GetPositionCount(&blockLength);
            offset += blockLength;
        }
        blockIndex++;
    }

    *pFound = found;
    *pOffset = offset;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetText
//
//  Synopsis: Concatenates all text content into a single flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBlockCollection::GetText(
    _In_ XUINT32 charPosition1,
    _In_ XUINT32 charPosition2,
    _In_ bool   insertNewlines,
    _Out_ CString **ppString
    )
{
    RRETURN(CTextBoxHelpers::GetText(this, charPosition1, charPosition2, insertNewlines, ppString));
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetText
//
//  Synopsis: Concatenates all text content into a single flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBlockCollection::GetText(
    _In_  bool    insertNewlines,
    _Out_ XUINT32 *pcCharacters,
    _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
    _Out_                                 bool  *pIsTakeOwnership
    )
{
    HRESULT hr = S_OK;
    CString *pText = NULL;
    XUINT32 length;

    GetPositionCount(&length);
    IFC(GetText(0, length, insertNewlines, &pText));

    *pcCharacters = pText->m_strString.GetCount();
    *ppCharacters = pText->m_strString.MakeBufferCopy();
    *pIsTakeOwnership = TRUE;

Cleanup:
    ReleaseInterface(pText);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetText
//
//  Synopsis: Concatenates all text content between 2 offsets into a flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBlockCollection::GetText(
    _In_ XUINT32  iTextPosition1,
    _In_ XUINT32  iTextPosition2,
    _In_ bool    insertNewlines,
    _Out_ XUINT32 *pcCharacters,
    _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
    _Out_                                 bool  *pIsTakeOwnership
    )
{
    HRESULT hr = S_OK;
    CString *pText = NULL;

    IFC(GetText(iTextPosition1, iTextPosition2, insertNewlines, &pText));

    *pcCharacters = pText->m_strString.GetCount();
    *ppCharacters = pText->m_strString.MakeBufferCopy();
    *pIsTakeOwnership = TRUE;

Cleanup:
    ReleaseInterface(pText);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBlockCollection::GetOwnerUIElement
//
//  Synopsis: return the owner UIElement.
//            used to trigger a re-layout when a font download complete
//
//------------------------------------------------------------------------
CUIElement *CBlockCollection::GetOwnerUIElement()
{
    return do_pointer_cast<CUIElement>(GetParentInternal(false));
}

//------------------------------------------------------------------------
//
//  CBlockCollection::MarkDirty
//
//  Synopsis:
//      Called when the content of the collection is changed. Invalidates
//      cached lengths, and forwards to <CTextElementCollection.MarkDirty()>
//
//------------------------------------------------------------------------
_Check_return_
HRESULT CBlockCollection::MarkDirty(
    _In_opt_ const CDependencyProperty *pdp
    )
{
    if (m_pLengths != NULL)
    {
        delete [] m_pLengths;
        m_pLengths = NULL;
    }
    m_length = 0;

    IFC_RETURN(CTextElementCollection::MarkDirty(pdp));

    return S_OK;
}

//-----------------------------------------------------------------------
//
//  Method:   CBlockCollection::CachePositionCounts
//
//  Synopsis: Fill in the array of element lengths, each length
//  corresponds to one contained CBlock.
//
//------------------------------------------------------------------------
void CBlockCollection::CacheLengths()
{
    XUINT32   blockCount;

    ASSERT(m_pLengths == NULL);
    m_length = 0;

    if (!empty())
    {
        blockCount = GetCount();

        m_pLengths = new XUINT32[blockCount];

        for (XUINT32 i = 0; i < blockCount; i++)
        {
            static_cast<CBlock*>(GetCollection()[i])->GetPositionCount(&(m_pLengths[i]));
            m_length += m_pLengths[i];
        }
    }
}

