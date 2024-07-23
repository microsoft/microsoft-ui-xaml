// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

CInlineCollection::~CInlineCollection()
{
    delete [] m_pPositionCounts;
    m_pPositionCounts = nullptr;
}

_Check_return_ HRESULT CInlineCollection::Create(
       _Outptr_ CDependencyObject **ppObject,
       _In_     CREATEPARAMETERS   *pCreate)
{
    CInlineCollection *pCollection = nullptr;

    *ppObject = nullptr;

    // Create an instance of the inline collection.
    pCollection = new CInlineCollection(pCreate->m_pCore);

    // Assign outgoing object.
    *ppObject = pCollection;
    return S_OK; //RRETURN_REMOVAL
}

//-----------------------------------------------------------------------
//
//  Method:   CInlineCollection::CachePositionCounts
//
//  Synopsis: Fill in the array of position counts, each position count
//  corresponds to one contained CInline.
//
//------------------------------------------------------------------------
void CInlineCollection::CachePositionCounts()
{
    XUINT32   cInlines;

    ENTERSECTION(InlineCollectionCachePositionCounts);

    ASSERT(m_pPositionCounts == nullptr);

    m_cCollectionPositions = 0;  // Initialize for no inlines present at all

    if (!empty())
    {
        cInlines = static_cast<XUINT32>(size());

        ASSERT(m_pPositionCounts == nullptr);
        m_pPositionCounts = new XUINT32[cInlines];
        m_cCollectionPositions = 2;  // Reserve two positions for the ends of the collection.

        for (XUINT32 i = 0; i<cInlines; i++)
        {
            static_cast<CInline*>((*this)[i])->GetPositionCount(&(m_pPositionCounts[i]));
            m_cCollectionPositions += m_pPositionCounts[i];
        }
    }

    LEAVESECTION(InlineCollectionCachePositionCounts);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when the content of the collection is changed. Invalidates
//      cached position counts, and forwards to <CTextElementCollection.MarkDirty()>
//
//  Notes:
//      Takes ownership of pLocalValueRecord on success only.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::MarkDirty(_In_opt_ const CDependencyProperty *pdp)
{
    // Invalidate any position counts we may have cached
    if (m_pPositionCounts != nullptr)
    {
        delete [] m_pPositionCounts;
        m_pPositionCounts = nullptr;
    }

    IFC_RETURN(CTextElementCollection::MarkDirty(pdp));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetPositionCount
//
//  Synopsis: Returns the sum of the nested inline position counts, plus
//            one each for the start and end of this collection.
//
//------------------------------------------------------------------------
void CInlineCollection::GetPositionCount(_Out_ XUINT32 *pcPositions)
{
    if (m_pPositionCounts == nullptr)
    {
        CachePositionCounts();
    }

    *pcPositions = m_cCollectionPositions;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::Insert
//
//  Synopsis: After calling the standard collection insert code,
//            sets the visual parent of the inline being inserted to
//            point to the inline collection itself
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::Insert(
    _In_ XUINT32            nIndex,
    _In_ CDependencyObject *pObject
)
{
    CInline *pInline = do_pointer_cast<CInline>(pObject);
    if (pInline == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(CTextElementCollection::Insert(nIndex, pObject));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::Append
//
//  Synopsis: If the value is a literal text CString, we call AppendText to add
//            it to the collection.  If the value isn't literal text, it must be
//            an Inline.
//            After calling the standard collection append code,
//            sets the visual parent of the inline being added to
//            point to the inline collection itself
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::Append(
    _In_ CDependencyObject *pObject,
    _Out_opt_ XUINT32 *pnIndex
)
{
    CString* pstrText = do_pointer_cast<CString>(pObject);
    if (pstrText)
    {
        IFC_RETURN(AppendText(pstrText->m_strString, pnIndex));
    }
    else
    {
        CInline *pInline = do_pointer_cast<CInline>(pObject);
        if (pInline == nullptr)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        IFC_RETURN(CTextElementCollection::Append(pObject, pnIndex));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::Append
//
//  Synopsis: If the value is literal text, we'll call AppendText to add it,
//            otherwise we'll defer to the default collection's Append that will
//            call our other Append overload with an Inline or CString.
//
//            We have marked the InlineCollection as having a content wrapper
//            for strings (see NativeTypeInfoProvider::GetContentWrapper) which
//            means we should wrap them in Runs in our Append methods.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::Append(
    _In_ CValue& value,
    _Out_opt_ XUINT32 *pnIndex
)
{
    if (value.GetType() == valueString)
    {
        IFC_RETURN(AppendText(value.AsString(), pnIndex));
    }
    else
    {
        // If type is valueObject, this call will end up at
        // CInlineCollection::Append(CDependencyObject, XUINT32)
        // So no need to duplicate work across both methods.

        IFC_RETURN(CTextElementCollection::Append(value, pnIndex));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::AppendText
//
//  Synopsis: Append literal text to the InlineCollection by wrapping it in a
//            new default Run.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::AppendText(
    _In_ const xstring_ptr& strText,
    _Out_opt_ XUINT32 *pnIndex
)
{
    // TODO: Optimize appending simple text content.
    // WPF's TextBlock includes a HasComplexContent property that basically
    // indicates whether we've had actual inlines added, or just a series of
    // strings.  If we have simple content, we can grab the ony Run and append
    // this text to the end of it.  Note that just checking for a single Run in
    // the collection isn't enough here because the following XAML is complex
    // but starts out looking simple:
    //  <TextBlock>
    //    Hello <Run>World</Run>
    //  </TextBlock>

    HRESULT hr = S_OK;
    CRun* pRun = nullptr;
    CDependencyObject* pdo = nullptr;
    CREATEPARAMETERS cp(GetContext());

    IFC(CRun::Create(&pdo, &cp));
    pRun = static_cast<CRun*>(pdo);
    IFC(pRun->SetText(strText, RunFlagsContent));

    IFC(Append(pdo, pnIndex));

Cleanup:
    ReleaseInterface(pRun);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetRun
//
//  Synopsis: Returns a run of characters all of the same format starting
//            at the given position.
//
//  If the startPosition corresponds to the (reserved) start or end
//  position of an Inline, GetRun returns
//    1) *ppCharacters == NULL,
//    2) *pTextFormatting == NULL,
//    3) *pcCharacters == number of reserved positions (1 for start or end of Inline).
//
//  If the startPosition corresponds to a (UTF-16) code unit, GetRun
//  returns
//    1) *ppCharacters points to that code unit,
//    2) *pcCharacters is set to the length of the longest character run
//       that is contiguous in memory, and for which all code units share
//       the same formatting,
//    3) *pTextFormatting points to the format shared by the code units at
//       *ppCharacters. Inheritance of formatting properties is already
//       resolved.
//
//   If the start position is beyond the end of the collection, fail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::GetRun(
    _In_                              XUINT32               characterPosition,
    _Out_opt_                   const TextFormatting      **ppTextFormatting,
    _Out_opt_                   const InheritedProperties **ppInheritedProperties,
    _Out_opt_                         TextNestingType      *pNestingType,
    _Out_opt_                         CTextElement        **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
    _Out_                             XUINT32              *pcCharacters
    )
{
    HRESULT hr = S_OK;

    ENTERMETHOD

    ENTERSECTION(InlineCollectionGetRun);

    if (m_pPositionCounts == nullptr)
    {
        CachePositionCounts();
    }

    IFCEXPECT(characterPosition <= m_cCollectionPositions);

    if (characterPosition == m_cCollectionPositions)
    {
        // Following the end of the paragraph, return a dummy
        // Unicode paragraph separator character.
        ENTERSECTION(InlineCollectionGetRunDummyEOP);
        CDependencyObject *pParent = GetParentInternal(false);

        // Only TextBlock is expected to call with the position following end of inline collection.
        CTextBlock *pTextBlock = do_pointer_cast<CTextBlock>(pParent);
        IFCEXPECT_ASSERT(pTextBlock != NULL);

        *ppCharacters = const_cast<WCHAR*>(L"\x2029"); //Warning! Casting away constness!
        *pcCharacters = 1;

        if (ppTextFormatting)
        {
            IFC(pTextBlock->GetTextFormatting(ppTextFormatting));
        }

        if (ppInheritedProperties)
        {
            IFC(pTextBlock->GetInheritedProperties(ppInheritedProperties));
        }

        if (pNestingType)
        {
            *pNestingType = NestedContent;
        }

        if (ppNestedElement)
        {
            *ppNestedElement = nullptr;
        }
    }
    else if (    characterPosition == 0
             ||  characterPosition == m_cCollectionPositions - 1)
    {
        // If the position corresponds to the start or end of this inline collection,
        // return a reserved run.
        ENTERSECTION(InlineCollectionGetRunStartEnd);

        *ppCharacters = nullptr;
        *pcCharacters = 1;

        if (ppTextFormatting)
        {
            *ppTextFormatting = nullptr;
        }

        if (ppInheritedProperties)
        {
            *ppInheritedProperties = nullptr;
        }

        if (pNestingType)
        {
            *pNestingType = (characterPosition == 0) ? OpenNesting : CloseNesting;
        }

        // The owner element is not known to the collection.
        // Hence the caller is responsible for setting ppNestedElement.
        if (ppNestedElement)
        {
            *ppNestedElement = nullptr;
        }
    }
    else
    {
        // The requested position is within one of our nested
        // inlines. Determine which.

        ENTERSECTION(InlineCollectionGetRunNested);

        XUINT32   inlineIndex = 0;  // Index of inline containing requested position
        XUINT32   inlineOffset = characterPosition; // Offset into inline

        inlineOffset -= 1; // Allow for initial reserved position

        while (inlineOffset >= m_pPositionCounts[inlineIndex])
        {
            inlineOffset -= m_pPositionCounts[inlineIndex];
            inlineIndex++;
            ASSERT(inlineIndex < m_cCollectionPositions);
        }

        ENTERSECTION(InlineCollectionGetRunGetCollection);
        LEAVESECTION(InlineCollectionGetRunGetCollection);

        IFC(static_cast<CInline*>((*this)[inlineIndex])->GetRun(
            inlineOffset,
            ppTextFormatting,
            ppInheritedProperties,
            pNestingType,
            ppNestedElement,
            ppCharacters,
            pcCharacters
        ));
    }

Cleanup:
    LEAVEMETHOD;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetText
//
//  Synopsis: Concatenates all text content between 2 offsets into a
//            flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::GetText(
    _In_  bool     insertNewlines,
    _Out_ CString **ppString)
{
    XUINT32 cPositions = 0;
    GetPositionCount(&cPositions);
    IFC_RETURN(GetText(0, cPositions, insertNewlines, ppString));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetText
//
//  Synopsis: Concatenates all text content into a single flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::GetText(
    _In_ XUINT32   iTextPosition1,
    _In_ XUINT32   iTextPosition2,
    _In_ bool     insertNewlines,
    _Out_ CString **ppString)
{
    return CTextBoxHelpers::GetText(this, iTextPosition1, iTextPosition2, insertNewlines, ppString);
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetText
//
//  Synopsis: Concatenates all text content into a single flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::GetText(
        _In_                                    bool      insertNewlines,
        _Out_                                   XUINT32   *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters)   const WCHAR **ppCharacters,
        _Out_                                   bool     *pIsTakeOwnership)
{
    HRESULT hr = S_OK;

    CString *pText = nullptr;

    IFC(GetText(insertNewlines, &pText));

    *pcCharacters = pText->m_strString.GetCount();
    *ppCharacters = pText->m_strString.MakeBufferCopy();
    *pIsTakeOwnership = TRUE;

Cleanup:
    ReleaseInterface(pText);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetText
//
//  Synopsis: Concatenates all text content between 2 offsets into a flat string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::GetText(
        _In_                                  XUINT32   iTextPosition1,
        _In_                                  XUINT32   iTextPosition2,
        _In_                                  bool     insertNewlines,
        _Out_                                 XUINT32  *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                                 bool     *pIsTakeOwnership)
{
    HRESULT hr = S_OK;

    CString *pText = nullptr;

    IFC(GetText(iTextPosition1, iTextPosition2, insertNewlines, &pText));

    *pcCharacters = pText->m_strString.GetCount();
    *ppCharacters = pText->m_strString.MakeBufferCopy();
    *pIsTakeOwnership = TRUE;

Cleanup:
    ReleaseInterface(pText);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetContainingElement
//
//  Synopsis: return the nested inline containing a text position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::GetContainingElement(
    _In_ XUINT32 characterPosition,
    _Outptr_ CTextElement **ppContainingElement
    )
{
    CTextElement *pElement = nullptr;

    if (m_pPositionCounts == nullptr)
    {
        CachePositionCounts();
    }

    IFCEXPECT_RETURN(characterPosition <= m_cCollectionPositions);

    if (characterPosition == 0 ||
        characterPosition >= m_cCollectionPositions-1)
    {
        // If the position corresponds to the start or end of this inline collection (reserved by inline collection),
        // there is no containing element. InlineCollection is not a containing element,
        // it's parent must deal with this case.
        pElement = nullptr;
    }
    else
    {
        // The requested position is within one of our nested
        // inlines. Determine which.
        XUINT32   inlineIndex = 0;  // Index of inline containing requested position
        XUINT32   inlineOffset = characterPosition; // Offset into inline

        inlineOffset -= 1; // Allow for initial reserved position

        while (inlineOffset >= m_pPositionCounts[inlineIndex])
        {
            inlineOffset -= m_pPositionCounts[inlineIndex];
            inlineIndex++;
            ASSERT(inlineIndex < m_cCollectionPositions);
        }

        if (!empty())
        {
            IFC_RETURN(static_cast<CInline*>((*this)[inlineIndex])->GetContainingElement(
                inlineOffset,
                &pElement
            ));
        }
    }

    *ppContainingElement = pElement;

    return S_OK;
}

_Check_return_ HRESULT CInlineCollection::GetElementEdgeOffset(
    _In_ CTextElement *pElement,
    _In_ ElementEdge edge,
    _Out_ XUINT32 *pOffset,
    _Out_ bool *pFound
    )
{
    XUINT32 inlineIndex = 0;
    bool found = false;
    XUINT32 count = GetCount();
    XUINT32 offsetInInline = 0;
    XUINT32 inlineLength = 0;

    // Start with an offset of 1 for the reserved start position.
    XUINT32 offset = 1;

    if (!empty())
    {
        while (inlineIndex < count &&
               !found)
        {
            IFC_RETURN(static_cast<CInline*>((*this)[inlineIndex])->GetElementEdgeOffset(
                pElement,
                edge,
                &offsetInInline,
                &found));

            if (found)
            {
                offset += offsetInInline;
                break;
            }
            else
            {
                static_cast<CInline*>((*this)[inlineIndex])->GetPositionCount(&inlineLength);
                offset += inlineLength;
            }
            inlineIndex++;
        }
    }

    *pFound = found;
    *pOffset = offset;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CInlineCollection::GetOwnerUIElement
//
//  Synopsis: return the owner UIElement.
//            used to trigger a re-layout when a font download complete
//
//------------------------------------------------------------------------
CUIElement *CInlineCollection::GetOwnerUIElement()
{
    return do_pointer_cast<CUIElement>(GetParentInternal(false));
}

CDependencyObject *CInlineCollection::GetOwnerDO() const
{
    return GetParentInternal(false /* publicParentOnly */);
}

//------------------------------------------------------------------------
//  Summary:
//      Validates that the given inline can be added to this InlineCollection.
//
//  Returns:
//      E_INVALIDARG if the inline cannot be added.
//      TODO bug 76608: This results in a "Value does not fall within the expected range"
//                      error. We should replace it with a clearer error message.
//
//  See Also:
//      <CTextSchema::InlineCollectionSupportsElement>.
//------------------------------------------------------------------------
_Check_return_ HRESULT CInlineCollection::ValidateTextElement(_In_ CTextElement *pTextElement)
{
    bool isInlineSupported = false;

    IFC_RETURN(CTextElementCollection::ValidateTextElement(pTextElement));

    IFC_RETURN(CTextSchema::InlineCollectionSupportsElement(this, pTextElement, &isInlineSupported));
    if (!isInlineSupported)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}
