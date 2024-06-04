// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//------------------------------------------------------------------------
//
//  Method:   CRun::GetTextLength
//
//------------------------------------------------------------------------

XINT32 CRun::GetTextLength()
{
    return m_strText.GetCount();
}

//------------------------------------------------------------------------
//
//  Method:   CRun::GetPositionCount
//
//  Synopsis: Returns the number of character positions in this run,
//            including one cp each for start and end of the run.
//
//------------------------------------------------------------------------

void CRun::GetPositionCount(_Out_ XUINT32 *pcPositions)
{
    *pcPositions = GetTextLength() + 2;
}

//------------------------------------------------------------------------
//
//  Method:   CRun::GetText
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CRun::GetText(
    _Outptr_result_buffer_(*pcCharacters) const WCHAR **ppCharacters,
    _Out_                             XUINT32      *pcCharacters)
{
    HRESULT hr = S_OK;

    *ppCharacters = NULL;
    *pcCharacters = 0;

    if (!m_strText.IsNull())
    {
        *pcCharacters = m_strText.GetCount();
        *ppCharacters = m_strText.GetBuffer();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CRun::GetRun
//
//  Synopsis: Returns a run of characters all of the same format starting
//            at the given position.
//
//  If the startPosition corresponds to the (reserved) start or end
//  position of an Inline, GetRun returns
//    1) *ppCharacters == NULL,
//    2) *pcCharacters == number of reserved positions (1 for start or end of Inline).
//    3) if there is any text (i.e. cCharacters>2) then *ppTextFormatting is set to
//       null. In the special case that there is no text, just reserved
//       positions, then ppTextFormatting is resolved for position zero just as it
//       is for text below.
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
//   Note that resolving formatting may involve walking the tree for
//   inheritance and parsing properties such as language, so we avoid
//   resolving formatting more than once per run in normal formatting
//   scenarios.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CRun::GetRun(
    _In_                              XUINT32           characterPosition,
    _Out_opt_                   const TextFormatting  **ppTextFormatting,
    _Out_opt_                   const InheritedProperties      **ppInheritedProperties,
    _Out_opt_                         TextNestingType  *pNestingType,
    _Out_opt_                         CTextElement    **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR     **ppCharacters,
    _Out_                             XUINT32          *pcCharacters
)
{
    XUINT32 cPositions = 0;

    GetPositionCount(&cPositions);
    IFCEXPECT_RETURN(characterPosition < cPositions);

    if (    characterPosition == 0
        ||  characterPosition == cPositions - 1)
    {
        // If the position corresponds to the start or end of this run,
        // return a reserved run.

        *ppCharacters = NULL;
        *pcCharacters = 1;

        if (ppNestedElement)
        {
            *ppNestedElement = this;
        }

        if (pNestingType)
        {
            *pNestingType = ((characterPosition == 0) ? OpenNesting : CloseNesting);
        }
    }
    else
    {
        // The requested position is within our string.

        IFC_RETURN(GetText(ppCharacters, pcCharacters));

        // If we are here, we must have a non-empty run.

        ASSERT(*pcCharacters > 0);
        ASSERT(*ppCharacters != NULL);

        // Calculations below take into account the single reserved
        // character positions before and after the content text.

        *ppCharacters = *ppCharacters + characterPosition - 1;
        *pcCharacters = cPositions - characterPosition - 1;

        if (ppNestedElement)
        {
            *ppNestedElement = this;
        }

        if (pNestingType)
        {
            *pNestingType = NestedContent;
        }
    }

    if (ppTextFormatting)
    {
        IFC_RETURN(GetTextFormatting(ppTextFormatting));
    }

    if (ppInheritedProperties)
    {
        IFC_RETURN(GetInheritedProperties(ppInheritedProperties));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  CRun::SetText
//
//  Synopsis:
//      Called by:
//          CTextBlock::SetValue(property Text)
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CRun::SetText(
    _In_ const xstring_ptr& strCharacters,
    _In_ XUINT32 flags
)
{
    CValue   value;

    m_flags = flags;

    //Set the text into this run.
    value.SetString(strCharacters);

    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Run_Text, value));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CRun::GetValue
//
//  Synopsis:
//      Gets text property from the backing store if present.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRun::GetValue(_In_ const CDependencyProperty *pdp, _Out_ CValue *pValue)
{
    // Special case for FlowDirection
    if (pdp->GetIndex() == KnownPropertyIndex::Run_FlowDirection
        && IsPropertyDefault(pdp))
    {
        // FlowDirection is implemented on Run and on FrameworkElement, but not
        // on intervening levels.
        // Walk up to the enclosing framework element and return its
        // flowdirection property.
        CDependencyObject *pdo = this;
        while (pdo != nullptr && !pdo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            pdo = pdo->GetInheritanceParentInternal(FALSE);
        }

        if (pdo == nullptr)
        {
            pValue->SetEnum(static_cast<XUINT32>(DirectUI::FlowDirection::LeftToRight)); // Default value if not in a tree
        }
        else
        {
            pdp = pdo->GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_FlowDirection);
            IFC_RETURN(pdo->GetValue(pdp, pValue));
        }
    }
    else
    {
        IFC_RETURN(CInline::GetValue(pdp, pValue));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Fast get of RightToLeft flag
//
//  Needed for e.g. animation scenarios where rendering needs the RTL flag
//  without any other text properties.
//
//------------------------------------------------------------------------

bool CRun::IsRightToLeft()
{
    // The Silverlight rasterizer accesses the flow direction of ui elements
    // very frequently, so we avoid the relatively slow IsPropertyDefaultById
    // and use the faster IsPropertySetBySlot.

    // TODO: Re-enable this. This was temporarily disabled during the type table merge.
    ASSERT(FALSE);
    // On first use we need to cache the FE flow direction slot number.
    /*static XUINT32 g_RunFlowDirectionSlot
                 = GetContext()->GetPropertyByIndexInline(KnownPropertyIndex::Run_FlowDirection)->m_nSlot;

    // Assert that the slot number doesn't change across multiple Silverlight instances.
    ASSERT(   g_RunFlowDirectionSlot
           == GetContext()->GetPropertyByIndexInline(KnownPropertyIndex::Run_FlowDirection)->m_nSlot);


    // Fast result if FlowDirection is set locally, or if inherited properties
    // are up to date.

    if (     m_pTextFormatting != NULL
        &&  (    !m_pTextFormatting->IsOld()
             ||  IsPropertySetBySlot(g_RunFlowDirectionSlot)))
    {
        return m_pTextFormatting->m_nFlowDirection == RightToLeft;
    }
    else
    {
        // We'll need to get direction from our parent.

        CDependencyObject *pParent = GetInheritanceParentInternal();

        if (pParent != NULL)
        {
            return pParent->IsRightToLeft();
        }
    }*/

    return false;
}

bool CRun::IsInsideHyperlink(_Outptr_result_maybenull_ CHyperlink** ppHyperlink)
{
    CDependencyObject *pTempAsDO = static_cast<CDependencyObject*>(this);
    CHyperlink *hyperlink = nullptr;

    while (pTempAsDO
        && pTempAsDO->GetTypeIndex() != KnownTypeIndex::Hyperlink
        && pTempAsDO->OfTypeByIndex<KnownTypeIndex::FrameworkElement>() == false)
    {
        pTempAsDO = pTempAsDO->GetParentInternal(false);
    }
    if (pTempAsDO && pTempAsDO->GetTypeIndex() == KnownTypeIndex::Hyperlink)
    {
        hyperlink = static_cast<CHyperlink*>(pTempAsDO);
        *ppHyperlink = hyperlink;
        return true;
    }
    return false;
}