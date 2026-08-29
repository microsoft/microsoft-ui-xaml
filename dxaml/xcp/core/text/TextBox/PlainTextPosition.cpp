// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//------------------------------------------------------------------------
//  Summary:
//      Creates an invalid position.
//------------------------------------------------------------------------
CPlainTextPosition::CPlainTextPosition()
{
    m_pContainer = nullptr;
    m_offset = 0;
    m_gravity = LineForwardCharacterForward;
}

//------------------------------------------------------------------------
//  Summary:
//      Creates a position with the given offset.
//------------------------------------------------------------------------
CPlainTextPosition::CPlainTextPosition(
     _In_ ITextContainer *pContainer,
     _In_ XUINT32         offset,
     _In_ TextGravity     gravity
     )
{
    m_pContainer = pContainer;
    m_offset = offset;
    m_gravity = gravity;
}

//------------------------------------------------------------------------
//  Summary:
//      Copy constructor.
//------------------------------------------------------------------------
CPlainTextPosition::CPlainTextPosition(
    const CPlainTextPosition &initializer
    )
{
    m_pContainer = initializer.m_pContainer;
    m_offset = initializer.m_offset;
    m_gravity = initializer.m_gravity;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text position integral offset.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetOffset(_Out_ XUINT32 *pOffset) const
{
    IFC_RETURN(CheckValid());
    *pOffset = m_offset;
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text position gravity.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetGravity(_Out_ TextGravity *pGravity) const
{
    IFC_RETURN(CheckValid());
    *pGravity = m_gravity;
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text position gravity.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetCharacterRect(
    _In_ TextGravity gravity,
    _Out_ XRECTF *pRect
    ) const
{
    XRECTF rect;
    XFLOAT lineOffset;
    bool contains = false;
    ITextView *pTextView  = nullptr;
    EmptyRectF(&rect);
    IFC_RETURN(CheckValid());

    pTextView = GetTextView();
    if (pTextView)
    {
        IFC_RETURN(pTextView->ContainsPosition(
            m_offset,
            gravity,
            &contains));

        // TextView may not contain this position, e.g. if layout hasn't happened yet or it doesn't
        // fit in available space. In that case we want to return empty rect, not default to something.

        if (contains)
        {
            IFC_RETURN(pTextView->TextPositionToPixelPosition(
                m_offset,
                gravity,
               &rect.X,
                NULL,
                NULL,
               &rect.Y,
               &rect.Height,
                NULL,
               &lineOffset));
            rect.X += lineOffset;
        }
    }
    *pRect = rect;

    return S_OK;
}

ITextContainer *CPlainTextPosition::GetTextContainer() const
{
    if (IsValid())
    {
        return m_pContainer;
    }
    return nullptr;
}

_Check_return_ HRESULT CPlainTextPosition::GetLogicalParent(
    _Outptr_ CDependencyObject **ppParent
    ) const
{
    CTextElement *pTextElementParent = nullptr;
    CDependencyObject *pParent = nullptr;

    IFC_RETURN(CheckValid());

    IFC_RETURN(m_pContainer->GetContainingElement(m_offset, &pTextElementParent));
    pParent = pTextElementParent;

    if (pParent == nullptr)
    {
        // If the container could not find a parent, return it's owning element.
        pParent = m_pContainer->GetOwnerUIElement();
    }

    *ppParent = pParent;

    return S_OK;
}

_Check_return_ HRESULT CPlainTextPosition::GetVisualParent(
    _Outptr_ CFrameworkElement **ppParent
    ) const
{
    CFrameworkElement *pParent = nullptr;
    ITextView *pTextView = nullptr;
    bool contains = false;
    IFC_RETURN(CheckValid());

    pTextView = GetTextView();
    if (pTextView)
    {
        IFC_RETURN(pTextView->ContainsPosition(
            m_offset,
            m_gravity,
            &contains));

        if (contains)
        {
            IFC_RETURN(pTextView->GetUIScopeForPosition(
                m_offset,
                m_gravity,
                &pParent));
        }
    }

    *ppParent = pParent;

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the position is valid.
//
//  Remarks:
//      A text position is valid if:
//          1) It's been provided with a text container in its constructor.
//          2) Its offset falls within the container's range
//------------------------------------------------------------------------
_Check_return_ bool CPlainTextPosition::IsValid() const
{
    return m_pContainer && m_offset <= CTextBoxHelpers::GetMaxTextPosition(m_pContainer);
}

//------------------------------------------------------------------------
//  Summary:
//      Returns TRUE if both positions are equal.
//------------------------------------------------------------------------
_Check_return_ bool CPlainTextPosition::Equals(_In_ const CPlainTextPosition &other) const
{
    XUINT32 otherOffset = 0;
    if (!IsValid() || FAILED(other.GetOffset(&otherOffset)))
    {
        return false;
    }

    return m_offset == otherOffset;
}

//------------------------------------------------------------------------
//  Summary:
//      Returns TRUE if 'this' is less than 'other'.
//------------------------------------------------------------------------
_Check_return_ bool CPlainTextPosition::LessThan(_In_ const CPlainTextPosition &other) const
{
    XUINT32 otherOffset;
    if (!IsValid() || FAILED(other.GetOffset(&otherOffset)))
    {
        return false;
    }

    return m_offset < otherOffset;
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the CPlainTextPosition represents a valid insertion position.
//
//  Remarks:
//      A position is a valid insertion position
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::IsAtInsertionPosition(
    _Out_ bool *pIsAtInsertionPosition) const
{
    ITextView *pTextView = nullptr;
    bool contains = false;
    IFC_RETURN(CheckValid());

    *pIsAtInsertionPosition = FALSE;

    // We may be called without a textboxview or formatting parameters during
    // control creation. Provide sensible results for this case.
    pTextView = GetTextView();
    if (pTextView)
    {
        IFC_RETURN(pTextView->ContainsPosition(
            m_offset,
            m_gravity,
            &contains));

        if (contains)
        {
            IFC_RETURN(pTextView->IsAtInsertionPosition(m_offset, pIsAtInsertionPosition));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the next insertion position, if any.
//
//  Parameters:
//      pFoundPosition - Set to TRUE if a next insertion position is found.
//      pPosition      - The next position, if found.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetNextInsertionPosition(
    _Out_ bool              *pFoundPosition,
    _Out_ CPlainTextPosition *pPosition) const
{
    *pPosition = *this;
    IFC_RETURN(pPosition->MoveToNextInsertionPosition(pFoundPosition));
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the previous insertion position, if any.
//
//  Parameters:
//      pFoundPosition - Set to TRUE if a next insertion position is found.
//      pPosition      - The previous position, if found.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetPreviousInsertionPosition(
    _Out_ bool              *pFoundPosition,
    _Out_ CPlainTextPosition *pPosition) const
{
    *pPosition = *this;
    IFC_RETURN(pPosition->MoveToPreviousInsertionPosition(pFoundPosition));
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the backspace position, if any.  This can be distinct
//  from the "previous insertion position" for some languages.
//
//  Parameters:
//      pFoundPosition - Set to TRUE if a next insertion position is found.
//      pPosition      - The previous position, if found.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetBackspacePosition(
    _Out_ bool              *pFoundPosition,
    _Out_ CPlainTextPosition *pPosition) const
{
    *pPosition = *this;
    IFC_RETURN(pPosition->MoveToBackspacePosition(pFoundPosition));
    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the position at the specified offset.
//
//  Parameters:
//      pFoundPosition - Set to TRUE if a position at the offset is found.
//      pPosition      - The position, if found.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::GetPositionAtOffset(
    _In_ XINT32 offset,
    _In_ TextGravity gravity,
    _Out_ bool *pFoundPosition,
    _Out_ CPlainTextPosition *pPosition) const
{
    *pPosition = *this;
    IFC_RETURN(pPosition->MoveByOffset(offset, gravity, pFoundPosition));
    return S_OK;
}

_Check_return_ HRESULT CPlainTextPosition::Clone(_Out_ CPlainTextPosition *pPosition) const
{
    *pPosition = *this;
    return S_OK; // RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the given position is inside a \r\n sequence.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::IsInsideLineBreak(_Out_ bool *pIsInsideLineBreak) const
{
    XUINT32  numCharacters = 0;
    const WCHAR *pCharacters = nullptr;

    IFC_RETURN(CheckValid());

    *pIsInsideLineBreak = FALSE;

    IFC_RETURN(m_pContainer->GetRun(m_offset, nullptr, nullptr, nullptr, nullptr, &pCharacters, &numCharacters));
    if (numCharacters && pCharacters != nullptr)
    {
        if (pCharacters[0] == UNICODE_LINE_FEED && m_offset > 0)
        {
            // There's a LF here.  Go back one character and look for CR.
            const WCHAR *pLookForCR = nullptr;
            XUINT32 cLookForCR = 0;

            IFC_RETURN(m_pContainer->GetRun(m_offset - 1, nullptr, nullptr, nullptr, nullptr, &pLookForCR, &cLookForCR));
            if (cLookForCR && pLookForCR != NULL && pLookForCR[0] == UNICODE_CARRIAGE_RETURN)
            {
                // We are in the middle of a CR+LF sequence.
                *pIsInsideLineBreak = TRUE;
            }
        }
    }

    return S_OK;
}
//------------------------------------------------------------------------
//  Summary:
//      Moves to the next insertion position.
//
//  Parameters:
//      pFoundPosition - Indicates whether an insertion position was found. If FALSE,
//                       the call did not change the position.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::MoveToNextInsertionPosition(
    _Out_ bool *pFoundPosition)
{
    HRESULT hr                    = S_OK;
    XUINT32 numPositions          = 0;
    XUINT32 oldOffset             = m_offset;
    bool isAtInsertionPosition = false;

    IFC(CheckValid());

    m_pContainer->GetPositionCount(&numPositions);
    while (m_offset < numPositions && !isAtInsertionPosition)
    {
        ++m_offset;
        IFC(IsAtInsertionPosition(&isAtInsertionPosition));
    }

    *pFoundPosition = isAtInsertionPosition;

Cleanup:
    if (FAILED(hr))
    {
        // Failure should not leave the object in an invalid state. Revert to the old
        // position
        m_offset = oldOffset;
    }
    return hr;
}

//------------------------------------------------------------------------
//  Summary:
//      Moves to the previous insertion position.
//
//  Parameters:
//      pFoundPosition - Indicates whether an insertion position was found. If FALSE,
//                       the call did not change the position.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::MoveToPreviousInsertionPosition(
    _Out_ bool *pFoundPosition)
{
    HRESULT hr                    = S_OK;
    XUINT32 numPositions          = 0;
    XUINT32 oldOffset             = m_offset;
    bool isAtInsertionPosition = false;

    IFC(CheckValid());

    m_pContainer->GetPositionCount(&numPositions);
    while (m_offset > 0 && !isAtInsertionPosition)
    {
        --m_offset;
        IFC(IsAtInsertionPosition(&isAtInsertionPosition));
    }

    *pFoundPosition = isAtInsertionPosition;

Cleanup:
    if (FAILED(hr))
    {
        // Failure should not leave the object in an invalid state. Revert to the old
        // position
        m_offset = oldOffset;
    }
    return hr;
}

//------------------------------------------------------------------------
//  Summary:
//      Moves to a position corresponding to backspace.
//
//  Parameters:
//      pFoundPosition - Indicates whether an insertion position was found. If FALSE,
//                       the call did not change the position.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::MoveToBackspacePosition(
    _Out_ bool *pFoundPosition)
{
    HRESULT hr                    = S_OK;
    XUINT32 oldOffset             = m_offset;
    bool isAtBackspacePosition = false;

    IFC(CheckValid());

    while (m_offset > 0 && !isAtBackspacePosition)
    {
        --m_offset;
        IFC(CTextBoxHelpers::IsNotInSurrogateCRLF(m_pContainer, m_offset, &isAtBackspacePosition));
    }

    *pFoundPosition = isAtBackspacePosition;

Cleanup:
    if (FAILED(hr))
    {
        // Failure should not leave the object in an invalid state. Revert to the old
        // position
        m_offset = oldOffset;
    }
    return hr;
}

//------------------------------------------------------------------------
//  Summary:
//      Moves by the specified offset.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::MoveByOffset(
    _In_ XINT32 offset,
    _In_ TextGravity gravity,
    _Out_ bool *pFoundPosition
    )
{
    HRESULT hr = S_OK;
    uint32_t positionCount = 0;
    uint32_t positionsMoved = 0;
    uint32_t absoluteOffset = 0;
    uint32_t oldOffset = m_offset;

    IFC(CheckValid());

    m_pContainer->GetPositionCount(&positionCount);

    if (offset >= 0)
    {
        absoluteOffset = offset;
        while (m_offset < positionCount &&
               positionsMoved < absoluteOffset)
        {
            ++m_offset;
            positionsMoved++;
        }
    }
    else
    {
        absoluteOffset = -offset;
        while (m_offset > 0 &&
               positionsMoved < absoluteOffset)
        {
            --m_offset;
            positionsMoved++;
        }
    }

    *pFoundPosition = (positionsMoved == absoluteOffset);
    m_gravity = gravity;

Cleanup:
    if (FAILED(hr))
    {
        // Failure should not leave the object in an invalid state. Revert to the old
        // position
        m_offset = oldOffset;
    }
    return hr;
}

ITextView *CPlainTextPosition::GetTextView() const
{
    if (IsValid())
    {
        if (CUIElement* pUIElement = m_pContainer->GetOwnerUIElement())
        {
            if (auto pRichTextBlock = do_pointer_cast<CRichTextBlock>(pUIElement))
            {
                return pRichTextBlock->GetTextView();
            }
            else if (auto pTextBlock = do_pointer_cast<CTextBlock>(pUIElement))
            {
                return pTextBlock->GetTextView();
            }
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------
//  Summary:
//      Fails if <IsValid> returns FALSE.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPlainTextPosition::CheckValid() const
{
    RRETURN(IsValid()? S_OK : E_FAIL);
}
