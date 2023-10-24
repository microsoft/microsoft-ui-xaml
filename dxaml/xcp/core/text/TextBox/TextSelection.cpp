// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <TextSelectionNotify.h>

//------------------------------------------------------------------------
//  Summary:
//      Constructor.
//------------------------------------------------------------------------
CTextSelection::CTextSelection(
    _In_ ITextContainer *pContainer,
    _In_ ITextView      *pTextView,
    _In_opt_ ITextSelectionNotify * textSelectionNotify
    )
{
    m_pContainer     = pContainer;
    m_pTextView      = pTextView;
    m_eCursorGravity = LineForwardCharacterBackward;
    m_movingPosition = CPlainTextPosition(pContainer, 0, LineForwardCharacterForward);
    m_anchorPosition = CPlainTextPosition(pContainer, 0, LineForwardCharacterForward);
    m_textSelectionNotify = textSelectionNotify;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelection::Create
//
//  Synopsis: Static create method.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::Create(
    _In_        ITextContainer  *pContainer,
    _In_        ITextView       *pTextView,
    _In_opt_ ITextSelectionNotify * textSelectionNotify,
    _Outptr_ CTextSelection **ppSelection
)
{
    HRESULT hr = S_OK;

    // Create an instance of selection object.

    CTextSelection *pSelection = new CTextSelection(
        pContainer, 
        pTextView,
        textSelectionNotify
        );

    // Assign outgoing object.

    *ppSelection = pSelection;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the 'moving' end of the selection.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetMovingTextPosition(_Out_ CTextPosition *pPosition) const 
{
    *pPosition = m_movingPosition;
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the 'anchor' (fixed) end of the selection.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetAnchorTextPosition(_Out_ CTextPosition *pPosition) const
{
    *pPosition = m_anchorPosition;
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the 'start' of the selection.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetStartTextPosition(_Out_ CTextPosition *pPosition) const
{
    RRETURN(m_movingPosition < m_anchorPosition ? GetMovingTextPosition(pPosition) : GetAnchorTextPosition(pPosition));
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the 'end' of the selection.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetEndTextPosition(_Out_ CTextPosition *pPosition)  const
{
    RRETURN(m_movingPosition > m_anchorPosition ? GetMovingTextPosition(pPosition) : GetAnchorTextPosition(pPosition));
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text gravity of the cursor, set through <Select>.
//------------------------------------------------------------------------
TextGravity CTextSelection::GetCursorGravity() const
{
    return m_eCursorGravity;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text gravity of the start position.
//------------------------------------------------------------------------
TextGravity CTextSelection::GetStartGravity() const
{
    return IsEmpty() ? m_eCursorGravity : LineForwardCharacterForward;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text gravity of the end position.
//------------------------------------------------------------------------
TextGravity CTextSelection::GetEndGravity() const
{
    return IsEmpty() ? m_eCursorGravity : LineBackwardCharacterBackward;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text gravity of the moving position.
//------------------------------------------------------------------------
TextGravity CTextSelection::GetMovingGravity() const
{
    if (IsEmpty())
    {
        // Moving position is a cursor (empty selection)
        return m_eCursorGravity;
    }
    else if (m_movingPosition < m_anchorPosition)
    {
        // Moving position is start of a non-empty selection
        return LineForwardCharacterForward;
    }
    else
    {
        // Moving position is end of a non-empty selection
        return LineBackwardCharacterBackward;
    }
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the selection is empty (i.e. represents a caret).
//------------------------------------------------------------------------
_Check_return_ bool CTextSelection::IsEmpty() const
{
    return m_anchorPosition == m_movingPosition;
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieves the length of the selection (== 0 for a caret).
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetLength(_Out_ XUINT32 *pLength) const
{
    XUINT32       startOffset        = 0;
    XUINT32       endOffset          = 0;
    CTextPosition startTextPosition;
    CTextPosition endTextPosition;

    IFC_RETURN(GetStartTextPosition(&startTextPosition));
    IFC_RETURN(GetEndTextPosition(&endTextPosition));

    IFC_RETURN(startTextPosition.GetOffset(&startOffset));
    IFC_RETURN(endTextPosition.GetOffset(&endOffset));
    
    *pLength = endOffset - startOffset;

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Resets the selection to cover the given range. XUINT32-based overload.
//
//  Remarks:
//      Anchor position and moving position can be the same.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::Select(
        _In_ XUINT32 iAnchorTextPosition,
        _In_ XUINT32 iMovingTextPosition, 
        _In_ TextGravity    eCursorGravity)
{
    IFC_RETURN(CTextSelection::NormalizeSelection(
        m_pContainer,
        m_pTextView,
        &iAnchorTextPosition,
        &iMovingTextPosition));

    // TODO: Figure out correct gravity for anchor. Moving uses cursor gravity.
    m_anchorPosition = CPlainTextPosition(GetContainer(), iAnchorTextPosition, LineForwardCharacterBackward);
    m_movingPosition = CPlainTextPosition(GetContainer(), iMovingTextPosition, eCursorGravity);

    m_eCursorGravity = eCursorGravity;

    if (m_textSelectionNotify != nullptr)
    {
        m_textSelectionNotify->OnSelectionChanged();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Resets the selection to cover the given range. CTextPosition-based overload.
//
//  Remarks:
//      Anchor position and moving position can be the same.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::Select(
    _In_ const CTextPosition &anchorTextPosition,
    _In_ const CTextPosition &movingTextPosition, 
    _In_ TextGravity          eCursorGravity
)
{
    XUINT32 anchorOffset = 0;
    XUINT32 movingOffset = 0;

    // Even though anchor/moving positions will have gravity associated, do not use it to select since selection will be normalized.
    IFC_RETURN(anchorTextPosition.GetOffset(&anchorOffset));
    IFC_RETURN(movingTextPosition.GetOffset(&movingOffset));

    IFC_RETURN(Select(anchorOffset, movingOffset, eCursorGravity));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Resets the selection to represent a caret at the text position indicated
//      by the given point.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::SetCaretPositionFromPoint(_In_ XPOINTF point)
{
    XUINT32             iHitPosition       = NULL;
    TextGravity         eHitGravity        = LineForwardCharacterForward;

    IFC_RETURN(m_pTextView->PixelPositionToTextPosition(
        point,        // pixel offset
        FALSE,        // hit after newline treated as before newline
       &iHitPosition,
       &eHitGravity
    ));

    IFC_RETURN(Select(
        iHitPosition, // Anchor position
        iHitPosition, // Moving position
        eHitGravity
    ));

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Extends the selection by moving the 'moving position' to the text
//      position indicated by the given point.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::ExtendSelectionByMouse(_In_ XPOINTF point)
{
    XUINT32             iHitPosition       = 0;
    XUINT32             iAnchorPosition    = 0;
    TextGravity         eHitGravity        = LineForwardCharacterForward;

    IFC_RETURN(m_anchorPosition.GetOffset(&iAnchorPosition));
    
    IFC_RETURN(m_pTextView->PixelPositionToTextPosition(
        point,   // pixel offset
        TRUE,    // Allow selection to include newline
       &iHitPosition,
        NULL
    ));

    // Update the selection moving position to the new hit 
    // position, while keeping the anchor position unchanged.

    IFC_RETURN(Select(
        iAnchorPosition, // Old anchor position
        iHitPosition,    // New moving position
        eHitGravity
    ));

    return S_OK;
}


//------------------------------------------------------------------------
//  Summary:
//      Selects the word that falls under the given point, if any.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::SelectWord(_In_ XPOINTF point)
{
    ASSERT(FALSE);

    // This capability is currently living at CTextEditor::SelectWord() because it
    // is common code between selection types, and CTextEditor::SelectWord()
    // is the only client so we have code in one place instead of two.

    // This does indeed break from WPF precedent, where it lives in TextPointer.
    // Re-evaluate once our own TextPosition is up and running.

    RRETURN(E_NOTIMPL);    
}


//------------------------------------------------------------------------
//  Summary:
//      Retrieves the text underlying the selection. If the selection is empty,
//      returns a NULL string with a length of 0.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetText(
    _Out_ xstring_ptr* pstrText
    ) const
{
    HRESULT hr          = S_OK;
    XUINT32 startOffset = 0;   
    XUINT32 endOffset   = 0;   
    XUINT32 cCharacters = 0;
    const WCHAR *pCharacters = NULL;
    bool takeOwnerShip = false;

    // Fill in default answer
    pstrText->Reset();

    if(!IsEmpty())
    {
        CTextPosition startTextPosition;
        CTextPosition endTextPosition;

        IFC(GetStartTextPosition(&startTextPosition));
        IFC(GetEndTextPosition(&endTextPosition));

        IFC(startTextPosition.GetOffset(&startOffset));
        IFC(endTextPosition.GetOffset(&endOffset));

        // pCharacters is a direct pointer into the container.
        IFC(m_pContainer->GetText(
            startOffset, 
            endOffset,
            TRUE /*insertNewlines*/,
            &cCharacters, 
            &pCharacters,
            &takeOwnerShip
        ));

        // For TextBlock and RichTextBlock selection is not normalized.
        // So it is possible that the moving and anchor positions are not equal
        // yet there is no text in between.
        if (cCharacters > 0)
        {
            XStringBuilder textBuilder;

            IFC(textBuilder.Initialize(pCharacters, cCharacters));
            ASSERT(textBuilder.IsNullTerminated());
            IFC(textBuilder.DetachString(pstrText));
        }
    }
    
Cleanup:

    if (takeOwnerShip)
    {
        delete[] pCharacters;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//  Summary:
//      Returns a XAML string describing the object tree of the selection.
//
//  Remarks:
//      Not supported in plain text.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::GetXaml(
    _Out_ xstring_ptr* pstrXaml
    ) const
{
    pstrXaml->Reset();

    RRETURN(E_NOT_SUPPORTED);
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelection::MoveToInsertionPosition
//
//  Synopsis: This is a no-op if current offset is a valid caret stop (aka) insertion 
//            position. If not, updates this position's offset to the next 
//            valid caret stop position in requested direction.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextSelection::MoveToInsertionPosition(
    _In_    ITextContainer   *pContainer,
    _In_    ITextView        *pTextView,
    _Inout_ XUINT32          *pTextPosition,
    _In_    DirectUI::LogicalDirection  eDirection
)
{
    bool bIsAtInsertionPosition = false;

    bool bMoved = false;
    CPlainTextPosition position = CPlainTextPosition(pContainer, *pTextPosition, LineForwardCharacterForward);

    // Check whether we are already at an insertion position
    IFC_RETURN(position.IsAtInsertionPosition(&bIsAtInsertionPosition));

    if (!bIsAtInsertionPosition)
    {
        // *pTextPosition is not an insertion position.
        // Find the nearest insertion position in the direction requested.

        if (eDirection == DirectUI::LogicalDirection::Forward)
        {
            IFC_RETURN(position.GetNextInsertionPosition(&bMoved, &position));
        }
        else
        {
            IFC_RETURN(position.GetPreviousInsertionPosition(&bMoved, &position));
        }

        // Return the insertion position to our caller.
        IFC_RETURN(position.GetOffset(pTextPosition));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelection::NormalizeRange (static)
//
//  Synopsis: Static method to normalize the moving position and anchor
//  offset of a range so that it starts and ends on an insertion position.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextSelection::NormalizeRange(
    _In_    ITextContainer  *pContainer,
    _In_    ITextView       *pTextView,
    _Inout_ XUINT32         *piStartTextPosition,
    _Inout_ XUINT32         *piEndTextPosition
)
{
    // Normalize cursor or selection start position

    IFC_RETURN(CTextBoxHelpers::VerifyPositionPair(
        pContainer,
        *piStartTextPosition, 
        *piEndTextPosition
    ));

    if (*piStartTextPosition == *piEndTextPosition)
    {
        // Normalize cursor (a zero length selection)
        IFC_RETURN(MoveToInsertionPosition(pContainer, pTextView, piStartTextPosition, DirectUI::LogicalDirection::Backward));
        *piEndTextPosition = *piStartTextPosition;
    }
    else
    {
        // Normalize non-empty selection
        IFC_RETURN(MoveToInsertionPosition(pContainer, pTextView, piStartTextPosition, DirectUI::LogicalDirection::Backward));
        IFC_RETURN(MoveToInsertionPosition(pContainer, pTextView, piEndTextPosition, DirectUI::LogicalDirection::Forward));
    }

    return S_OK;
}




//------------------------------------------------------------------------
//
//  Method:   CTextSelection::NormalizeSelection (static)
//
//  Synopsis: Static method to normalize the moving position and anchor
//  offset of a range so that it starts and ends on an insertion position.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextSelection::NormalizeSelection(
    _In_    ITextContainer  *pContainer,
    _In_    ITextView       *pTextView,
    _Inout_ XUINT32         *piAnchorTextPosition,
    _Inout_ XUINT32         *piMovingTextPosition
)
{
    XUINT32  iStartTextPosition = 0;
    XUINT32  iEndTextPosition   = 0; 


    // Determine start position (lowest offset)

    if (*piAnchorTextPosition <= *piMovingTextPosition)
    {
        iStartTextPosition = *piAnchorTextPosition;
        iEndTextPosition   = *piMovingTextPosition;
    }
    else
    {
        iStartTextPosition = *piMovingTextPosition;
        iEndTextPosition   = *piAnchorTextPosition;
    }


    // Normalize range

    IFC_RETURN(CTextSelection::NormalizeRange(pContainer, pTextView, &iStartTextPosition, &iEndTextPosition));


    // Return to caller as moving and anchor positions

    if (*piAnchorTextPosition <= *piMovingTextPosition)
    {
        *piAnchorTextPosition = iStartTextPosition;
        *piMovingTextPosition = iEndTextPosition;  
    }
    else
    {
        *piMovingTextPosition = iStartTextPosition;
        *piAnchorTextPosition = iEndTextPosition;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelection::ExtendSelectionByMouse
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::ExtendSelectionByMouse(_In_ const CTextPosition &cursorPosition)
{
    XUINT32       uCursorPosition    = 0;
    XUINT32       uAnchorPosition = 0;
    TextGravity   hitGravity      = LineForwardCharacterForward;
    CTextPosition anchorPosition;

    IFC_RETURN(GetAnchorTextPosition(&anchorPosition));
    IFC_RETURN(anchorPosition.GetOffset(&uAnchorPosition));
    
    IFC_RETURN(cursorPosition.GetOffset(&uCursorPosition));
    

    // Update the selection moving position to the new hit 
    // position, while keeping the anchor position unchanged.
    IFC_RETURN(Select(
        uAnchorPosition,
        uCursorPosition,
        hitGravity
    ));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelection::SetCaretPositionFromTextPosition
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::SetCaretPositionFromTextPosition(_In_ const CTextPosition &position)
{
    XUINT32     uPosition = 0;
    TextGravity hitGravity   = LineForwardCharacterForward;

    IFC_RETURN(position.GetOffset(&uPosition));
    IFC_RETURN(position.GetPlainPosition().GetGravity(&hitGravity));

    IFC_RETURN(Select(
        uPosition, // Anchor position
        uPosition, // Moving position
        hitGravity
    ));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelection::SelectWord
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelection::SelectWord(_In_ const CTextPosition &position)
{
   RRETURN(E_UNEXPECTED);
}

void CTextSelection::ResetSelection()
{
    m_movingPosition = CPlainTextPosition(m_pContainer, 0, LineForwardCharacterForward);
    m_anchorPosition = CPlainTextPosition(m_pContainer, 0, LineForwardCharacterForward);
}
