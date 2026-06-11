// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockView.h"
#include "LinkedRichTextBlockView.h"
#include "ParagraphNode.h"
#include "PageNode.h"
#include "RichTextBlockOverflow.h"

LinkedRichTextBlockView::LinkedRichTextBlockView(
    _In_ CRichTextBlock *pMaster
    ) :
    m_pMaster(pMaster)
{
}

void LinkedRichTextBlockView::SetInputContextView(_In_ RichTextBlockView *pView)
{
    m_pInputContextView = pView;
}

_Check_return_ HRESULT LinkedRichTextBlockView::TextRangeToTextBounds(
    _In_ XUINT32 startOffset,
    _In_ XUINT32 endOffset,
    _Out_ XUINT32 *pcRectangles, 
    _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles
    )
{
    // In current scenarios TextRangeToTextBounds is never called from any
    // place where a linked view is needed. It is only called from text UIElements 
    // to generate highlight and focus rects. A text UIElement will have it's own view, even if it's linked,
    // and can use that.
    ASSERT(FALSE);
    IFC_RETURN(E_NOTIMPL);
    return S_OK;
}

_Check_return_ HRESULT LinkedRichTextBlockView::TextSelectionToTextBounds(
    _In_ IJupiterTextSelection *pSelection,   
    _Out_ XUINT32 *pcRectangles, 
    _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles  
)
{
    // In current scenarios TextSelectionToTextBounds is never called from TextSelection or any
    // other place where a linked view is needed. It is only called from text UIElements 
    // to generate highlight rects. A text UIElement will have it's own view, even if it's linked,
    // and can use that.
    ASSERT(FALSE);
    IFC_RETURN(E_NOTIMPL);
    return S_OK;
}

_Check_return_ HRESULT LinkedRichTextBlockView::IsAtInsertionPosition(
    _In_  XUINT32 iTextPosition,
    _Out_ bool  *pfIsAtInsertionPosition
)
{
    RichTextBlockView *pView = m_pMaster->GetSingleElementTextView();
    CRichTextBlockOverflow *pOverflow = static_cast<CRichTextBlockOverflow *>(m_pMaster->GetNext());
    bool contains = false;

    while (pView != nullptr)
    {
        IFC_RETURN(pView->ContainsPosition(
            iTextPosition,
            LineForwardCharacterForward,
            &contains));

        if (contains)
        {
            IFC_RETURN(pView->IsAtInsertionPosition(iTextPosition, pfIsAtInsertionPosition));
            break;
        }

        if (pOverflow != nullptr)
        {
            pView = pOverflow->GetSingleElementTextView();
            pOverflow = static_cast<CRichTextBlockOverflow *>(pOverflow->GetNext());
        }
        else
        {
            pView = nullptr;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT LinkedRichTextBlockView::PixelPositionToTextPosition(
    _In_ XPOINTF pixelCoordinate, 
    _In_ XUINT32 bIncludeNewline,
    _Out_ XUINT32 *piTextPosition,
    _Out_opt_ TextGravity *peGravity
)
{
    // The input context view should have been set by the element receiving input.
    if (m_pInputContextView != nullptr)
    {
        IFC_RETURN(m_pInputContextView->PixelPositionToTextPosition(
            pixelCoordinate,
            bIncludeNewline,
            piTextPosition,
            peGravity));
    }

    return S_OK;
}

_Check_return_ HRESULT LinkedRichTextBlockView::TextPositionToPixelPosition(
    _In_      XUINT32      iTextPosition,
    _In_      TextGravity  eGravity,
    _Out_opt_ XFLOAT      *pePixelOffset,      // Relative to origin of line
    _Out_opt_ XFLOAT      *peCharacterTop,     // Relative to TextView top
    _Out_opt_ XFLOAT      *peCharacterHeight,
    _Out_opt_ XFLOAT      *peLineTop,          // Relative to TextView top
    _Out_opt_ XFLOAT      *peLineHeight,
    _Out_opt_ XFLOAT      *peLineBaseline,
    _Out_opt_ XFLOAT      *peLineOffset        // Padding and alignment offset
)
{
    RichTextBlockView *pView = m_pMaster->GetSingleElementTextView();
    CRichTextBlockOverflow *pOverflow = static_cast<CRichTextBlockOverflow *>(m_pMaster->GetNext());
    bool contains = false;

    while (pView != nullptr)
    {
        IFC_RETURN(pView->ContainsPosition(
            iTextPosition,
            eGravity,
            &contains));

        if (contains)
        {
            IFC_RETURN(pView->TextPositionToPixelPosition(
                iTextPosition,                
                eGravity,
                pePixelOffset, 
                peCharacterTop, 
                peCharacterHeight,
                peLineTop,
                peLineHeight,
                peLineBaseline,
                peLineOffset));
            break;
        }

        if (pOverflow != nullptr)
        {
            pView = pOverflow->GetSingleElementTextView();
            pOverflow = static_cast<CRichTextBlockOverflow *>(pOverflow->GetNext());
        }
        else
        {
            pView = nullptr;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT LinkedRichTextBlockView::GetUIScopeForPosition(
    _In_ XUINT32 iTextPosition,
    _In_ TextGravity eGravity,
    _Outptr_ CFrameworkElement **ppUIScope
)
{
    RichTextBlockView *pView = m_pMaster->GetSingleElementTextView();
    CRichTextBlockOverflow *pOverflow = static_cast<CRichTextBlockOverflow *>(m_pMaster->GetNext());
    bool contains = false;

    while (pView != nullptr)
    {
        IFC_RETURN(pView->ContainsPosition(
            iTextPosition,
            eGravity,
            &contains));

        if (contains)
        {
            IFC_RETURN(pView->GetUIScopeForPosition(
                iTextPosition,
                eGravity,
                ppUIScope));
            break;
        }

        if (pOverflow != nullptr)
        {
            pView = pOverflow->GetSingleElementTextView();
            pOverflow = static_cast<CRichTextBlockOverflow *>(pOverflow->GetNext());
        }
        else
        {
            pView = nullptr;
        }
    }

    return S_OK;
}

XUINT32 LinkedRichTextBlockView::GetContentStartPosition()
{
    RichTextBlockView *pView = m_pMaster->GetSingleElementTextView();
    return pView->GetContentStartPosition();
}

XUINT32 LinkedRichTextBlockView::GetContentLength()
{
    RichTextBlockView *pView = m_pMaster->GetSingleElementTextView();
    CRichTextBlockOverflow *pOverflow = static_cast<CRichTextBlockOverflow *>(m_pMaster->GetNext());
    XUINT32 contentLength = 0;
    while (pView != nullptr)
    {
        contentLength += pView->GetContentLength();

        if (pOverflow != nullptr)
        {
            pView = pOverflow->GetSingleElementTextView();
            pOverflow = static_cast<CRichTextBlockOverflow *>(pOverflow->GetNext());
        }
        else
        {
            pView = nullptr;
        }
    }
    return contentLength;
}

int LinkedRichTextBlockView::GetAdjustedPosition(int charIndex)
{
    // RichTextBlockOverflow gets its highlighting positions from RichTextBlock
    ASSERT(false);
    return 0;
}

// UIA navigation will retrieve a character index from an overflowed RTB here
int LinkedRichTextBlockView::GetCharacterIndex(int position)
{
    return m_pMaster->GetSingleElementTextView()->GetCharacterIndex(position);
}

_Check_return_ HRESULT LinkedRichTextBlockView::ContainsPosition(
    _In_ XUINT32 iTextPosition,
    _In_ TextGravity gravity,
    _Out_ bool *pContains
    )
{
    RichTextBlockView *pView = m_pMaster->GetSingleElementTextView();
    CRichTextBlockOverflow *pOverflow = static_cast<CRichTextBlockOverflow*>(m_pMaster->GetNext());
    bool contains = false;

    *pContains = FALSE;

    while (pView != nullptr)
    {
        IFC_RETURN(pView->ContainsPosition(
            iTextPosition,
            gravity,
            &contains));
        
        if (contains)
        {
            *pContains = contains;
            break;
        }

        if (pOverflow != nullptr)
        {
            pView = pOverflow->GetSingleElementTextView();
            pOverflow = static_cast<CRichTextBlockOverflow *>(pOverflow->GetNext());
        }
        else
        {
            pView = nullptr;
        }
    }

    return S_OK;
}
