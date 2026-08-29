// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#define HORIZONTALADJUSTER 0.75

//------------------------------------------------------------------------
//  Summary:
//      Destructor
//------------------------------------------------------------------------
CTextAdapter::~CTextAdapter()
{
}

//---------------------------------------------------------------------------------
// Gets a text range that encloses the main text of a document.
//---------------------------------------------------------------------------------
_Check_return_ HRESULT
CTextAdapter::GetDocumentRange(_Outptr_ CTextRangeAdapter **ppTextRangeAdapter)
{
    xref_ptr<CTextPointerWrapper> startTextPointer;
    xref_ptr<CTextPointerWrapper> endTextPointer;
    xref_ptr<CTextRangeAdapter> textRangeAdapter;

    *ppTextRangeAdapter = nullptr;
    IFC_RETURN(CTextAdapter::GetContentEndPointers(m_pTextOwner, startTextPointer.ReleaseAndGetAddressOf(), endTextPointer.ReleaseAndGetAddressOf()));
    if (startTextPointer && endTextPointer)
    {
        IFC_RETURN(CTextRangeAdapter::Create(GetContext(), this, startTextPointer, endTextPointer, m_pTextOwner, textRangeAdapter.ReleaseAndGetAddressOf()));
    }

    *ppTextRangeAdapter = textRangeAdapter.detach();

    return S_OK;
}

//-------------------------------------------------------------
// We only support single Selection here.
//-------------------------------------------------------------
_Check_return_ HRESULT CTextAdapter::GetSupportedTextSelection(_Out_ UIAXcp::SupportedTextSelection *pRetVal)
{
    IFCPTR_RETURN(pRetVal);

    TextSelectionManager* pTextSelectionManager = CTextAdapter::GetSelectionManager(m_pTextOwner);
    if (pTextSelectionManager != nullptr)
    {
        *pRetVal = UIAXcp::SupportedTextSelection_Single;
    }
    else
    {
        *pRetVal = UIAXcp::SupportedTextSelection_None;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// Returns an array of Text ranges of selections, In this case there's only single selection so it returns, one range of selected text.
//--------------------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextAdapter::GetSelection(_Outptr_result_buffer_all_(*pnCount) CTextRangeAdapter ***pppSelectedTextRangeAdapter, _Out_ XINT32* pnCount)
{
    HRESULT hr = S_OK;
    xref_ptr<CTextRangeAdapter> selectedRange;
    XINT32 startOffset;
    XINT32 endOffset;
    xref_ptr<CTextPointerWrapper> selectionStartTextPointer;
    xref_ptr<CTextPointerWrapper> selectionEndTextPointer;
    CTextRangeAdapter **ppSelectedTextRangeAdapter = nullptr;

    IFCPTR(pnCount);
    IFCPTR(pppSelectedTextRangeAdapter);
    *pnCount = 0;
    *pppSelectedTextRangeAdapter = nullptr;

    IFC(CTextAdapter::GetSelectionEndPointers(m_pTextOwner, selectionStartTextPointer.ReleaseAndGetAddressOf(), selectionEndTextPointer.ReleaseAndGetAddressOf()));

    if (selectionStartTextPointer && selectionEndTextPointer)
    {
        IFC(selectionStartTextPointer->GetOffset(&startOffset));
        IFC(selectionEndTextPointer->GetOffset(&endOffset));
        if (endOffset < startOffset)
        {
            std::swap(selectionStartTextPointer, selectionEndTextPointer);
        }
        IFC(CTextRangeAdapter::Create(GetContext(), this, selectionStartTextPointer, selectionEndTextPointer, m_pTextOwner, selectedRange.ReleaseAndGetAddressOf()));
        ppSelectedTextRangeAdapter = new CTextRangeAdapter *[1];
        ppSelectedTextRangeAdapter[0] = selectedRange.detach();
        *pppSelectedTextRangeAdapter  = ppSelectedTextRangeAdapter;
        ppSelectedTextRangeAdapter = nullptr;
        *pnCount = 1;
    }

Cleanup:
    delete [] ppSelectedTextRangeAdapter;
    return hr;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// Retrieves an array of disjoint text ranges from a text container. Each text range begins with the first partially visible line and ends
// with the last partially visible line.
//--------------------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextAdapter::GetVisibleRanges(_Outptr_result_buffer_all_(*pnCount) CTextRangeAdapter ***pppVisibleTextRangeAdapter, _Out_ XINT32* pnCount)
{
    HRESULT hr = S_OK;
    CTextRangeAdapter **ppVisibleTextRangeAdapter = nullptr;
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    ITextView* pTextView = nullptr;
    xref_ptr<CTextPointerWrapper> startPointer;
    xref_ptr<CTextPointerWrapper> endPointer;
    XINT32 startOffset;
    XINT32 endOffset;
    XUINT32 hitTestRects = 0;
    XUINT32 cRectangles = 0;
    XRECTF *pRectangles = nullptr;
    XPOINTF pixelCoordinate;
    XUINT32 textPosition;
    CPlainTextPosition plainTextPosition;
    TextGravity gravity;

    *pppVisibleTextRangeAdapter = nullptr;
    *pnCount = 0;

    IFC(CTextAdapter::GetContentEndPointers(m_pTextOwner, startPointer.ReleaseAndGetAddressOf(), endPointer.ReleaseAndGetAddressOf()));
    if (startPointer && endPointer)
    {
        IFC(startPointer->GetOffset(&startOffset));
        IFC(endPointer->GetOffset(&endOffset));

        pTextView = CTextAdapter::GetTextView(m_pTextOwner);
        if (pTextView)
        {
            auto core = GetContext();

            IFC(pTextView->TextRangeToTextBounds(startOffset, endOffset, &hitTestRects, &pRectangles));

            cRectangles = hitTestRects;
            // We need to exclude the empty rects, otherwise CTextRangeAdapter::ExpandToLine will fail to move forward.
            for (XUINT32 current = 0; current < hitTestRects; current++)
            {
                if (pRectangles[current].Width == 0 || pRectangles[current].Height == 0)
                {
                    cRectangles--;
                }
            }

            ppVisibleTextRangeAdapter  = new CTextRangeAdapter *[cRectangles];
            for (XUINT32 i =0; i < cRectangles; i++)
            {
                ppVisibleTextRangeAdapter[i] = nullptr;
            }

            // Create TextRanges for all visible bounding Rectangles.
            for (uint32_t current = 0, index = 0; current < hitTestRects; current++)
            {
                if (pRectangles[current].Width == 0 || pRectangles[current].Height == 0)
                {
                    continue;
                }

                // If Width is less than 1 having degenerate range to represent the line is the best option, especially this deals well with LineBreaks.
                if (pRectangles[current].Width < 1)
                {
                    pixelCoordinate.x = pRectangles[current].X;
                    // Sample the middle of the vertical line bound to get the character index.
                    pixelCoordinate.y = pRectangles[current].Y + pRectangles[current].Height / 2;
                    IFC(pTextView->PixelPositionToTextPosition(pixelCoordinate, TRUE, &textPosition, &gravity));
                    startOffset = textPosition;
                    plainTextPosition = CPlainTextPosition(CTextAdapter::GetTextContainer(m_pTextOwner), textPosition, gravity);
                    IFC(CTextPointerWrapper::Create(core, plainTextPosition, startPointer.ReleaseAndGetAddressOf()));
                    IFC(CTextPointerWrapper::Create(core, plainTextPosition, endPointer.ReleaseAndGetAddressOf()));
                }
                else
                {
                    // calculate start pointer for the range
                    pixelCoordinate.x = pRectangles[current].X + static_cast<XFLOAT>(HORIZONTALADJUSTER);
                    // Sample the middle of the vertical line bound to get the character index.
                    pixelCoordinate.y = pRectangles[current].Y + pRectangles[current].Height / 2;
                    IFC(pTextView->PixelPositionToTextPosition(pixelCoordinate, TRUE, &textPosition, &gravity));
                    startOffset = textPosition;
                    plainTextPosition = CPlainTextPosition(CTextAdapter::GetTextContainer(m_pTextOwner), textPosition, gravity);
                    IFC(CTextPointerWrapper::Create(core, plainTextPosition, startPointer.ReleaseAndGetAddressOf()));

                    // calculate end pointer for the range
                    pixelCoordinate.x = pRectangles[current].X + pRectangles[current].Width - static_cast<XFLOAT>(HORIZONTALADJUSTER);
                    // Sample the middle of the vertical line bound to get the character index.
                    pixelCoordinate.y = pRectangles[current].Y + pRectangles[current].Height / 2;
                    IFC(pTextView->PixelPositionToTextPosition(pixelCoordinate, TRUE, &textPosition, &gravity));
                    endOffset = textPosition;
                    plainTextPosition = CPlainTextPosition(CTextAdapter::GetTextContainer(m_pTextOwner), textPosition, gravity);
                    IFC(CTextPointerWrapper::Create(core, plainTextPosition, endPointer.ReleaseAndGetAddressOf()));
                }

                // In case the rectangle corresponds to BiDi endOffset corresponding to point(top, Right) will be less than
                // startOffset corresponding to point (top, left) and needs to be adjusted accordingly.
                if (endOffset < startOffset)
                {
                    std::swap(startPointer, endPointer);
                }
                IFC(CTextRangeAdapter::Create(core, this, startPointer, endPointer, m_pTextOwner, textRangeAdapter.ReleaseAndGetAddressOf()));
                ASSERT(index < cRectangles);
                ppVisibleTextRangeAdapter[index] = textRangeAdapter.detach();
                index++;
            }
        }
    }

    *pppVisibleTextRangeAdapter = ppVisibleTextRangeAdapter;
    ppVisibleTextRangeAdapter = nullptr;
    *pnCount = cRectangles;

Cleanup:
    delete [] pRectangles;
    if (ppVisibleTextRangeAdapter)
    {
        for (uint32_t current = 0; current < cRectangles; current++)
        {
            ReleaseInterface(ppVisibleTextRangeAdapter[current]);
        }
    }
    delete [] ppVisibleTextRangeAdapter;
    return hr;
}

//------------------------------------------------------------------------------------------------------------------------------
// Verify Child is actually child of the Text Owner for the TextAdapter, Find the containing range for the child.
//------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextAdapter::RangeFromChild(_In_ CAutomationPeer* pAP, _Outptr_ CTextRangeAdapter **ppTextRangeAdapter)
{
    HRESULT hr = S_OK;
    CAutomationPeer **ppAPChildren = nullptr;
    uint32_t childrenCount = 0;
    XPOINTF clickablePointAP = {};

    IFCPTR(ppTextRangeAdapter);
    *ppTextRangeAdapter = nullptr;
    IFCPTR(pAP);
    if (m_pTextOwner)
    {
        childrenCount = m_pTextOwner->GetAPChildren(&ppAPChildren);
        for (uint32_t i = 0; i < childrenCount; i++)
        {
            if (pAP == ppAPChildren[i])
            {
                CInlineUIContainer* iuc = GetParentInlineUIContainer(pAP);
                if (iuc != nullptr)
                {
                    // Using PixelPositionToTextPosition (which is used by RangeFromPoint) on an InlineUIContainer
                    // is unreliable, so have the InlineUIContainer find its own position within its parent instead.
                    IFC(RangeFromInlineUIContainer(iuc, ppTextRangeAdapter));
                }
                else if (pAP->OfTypeByIndex<KnownTypeIndex::HyperlinkAutomationPeer>())
                {
                    // ClickablePoints for links should be at the start of the link.
                    // Then, the returned range should be non-degenerated (non-empty).
                    // We can't rely on just the clickable point, we also need the AP, because if the link is in an overflow,
                    // RangeFromPoint will assume the point is relative to the master block, not the overflow.
                    IFC(pAP->GetClickablePoint(&clickablePointAP));
                    IFC(RangeFromLink(pAP, clickablePointAP, ppTextRangeAdapter));
                }
                else
                {
                    // Calculate clickable point on element with respect to owner, then do RangeFromPoint and
                    // finally expand the degenerated range to paragraph as paragraphs contains InlineUIContainers.
                    IFC(pAP->GetClickablePoint(&clickablePointAP));
                    IFC(RangeFromPoint(clickablePointAP, ppTextRangeAdapter));
                }
                break;
            }
        }
    }

Cleanup:
    delete [] ppAPChildren;
    return hr;
}

CInlineUIContainer* CTextAdapter::GetParentInlineUIContainer(_In_ CAutomationPeer* pAP)
{
    CInlineUIContainer* iuc = nullptr;
    CDependencyObject* obj = pAP->GetDONoRef();
    while (obj != nullptr && !obj->OfTypeByIndex<KnownTypeIndex::InlineUIContainer>())
    {
        if (obj->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            CFrameworkElement* fe = static_cast<CFrameworkElement*>(obj);
            obj = fe->GetLogicalParentNoRef();
        }
        else
        {
            obj = obj->GetParent();
        }
    }
    if (obj != nullptr)
    {
        iuc = static_cast<CInlineUIContainer*>(obj);
    }
    return iuc;
}

_Check_return_ HRESULT CTextAdapter::RangeFromLink(_In_ CAutomationPeer* pAP, _In_ XPOINTF point, _Outptr_ CTextRangeAdapter **ppTextRangeAdapter)
{
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    xref_ptr<CTextPointerWrapper> startPointer;
    xref_ptr<CTextPointerWrapper> endPointer;
    XPOINTF pixelCoordinate;
    uint32_t textPosition;
    CPlainTextPosition plainTextPosition;
    TextGravity gravity;
    xref_ptr<ITransformer> transformer;
    CFrameworkElement *pContentStartVisualParent;
    CHyperlink* hyperlink;
    ITextContainer *pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);
    auto core = GetContext();

    IFCPTR_RETURN(ppTextRangeAdapter);
    *ppTextRangeAdapter = nullptr;

    uint32_t linkLength = 0;
    int startEdge = 0;
    int endEdge = 0;
    xref_ptr<CTextPointerWrapper> start;
    xref_ptr<CTextPointerWrapper> end;

    CDependencyObject* obj = pAP->GetDONoRef();
    if (obj->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
    {
        hyperlink = static_cast<CHyperlink*>(obj);
        IFC_RETURN(hyperlink->GetTextContentStart(start.ReleaseAndGetAddressOf()));
        IFC_RETURN(hyperlink->GetTextContentEnd(end.ReleaseAndGetAddressOf()));
    }

    // If either of these are null, an automation peer for the link would never have been created to get here
    ASSERT(start != nullptr && end != nullptr);

    IFC_RETURN(start->GetOffset(&startEdge));
    IFC_RETURN(end->GetOffset(&endEdge));
    linkLength = endEdge - startEdge;

    // Get the visual parent's view in order to get the PixelPosition from the text view. If we don't use the visual parent,
    // links in RichTextBlockOverflows won't get the correct text position from the pixel.  Note: There are situations where text
    // has been truncated where there may be runs/hyperlinks/etc that are outside the available space and as such are not
    // laid out as part of the text view.  Since the text view is used to determine the parent, there won't be a parent found
    // for these items.  In this case we will just return null which will cause accessibility to ignore it. 
    IFC_RETURN(start->GetVisualParent(&pContentStartVisualParent));
    if (!pContentStartVisualParent)
    {
        return S_OK;
    }
    ITextView* pTextView = CTextAdapter::GetTextView(pContentStartVisualParent);

    IFC_RETURN(static_cast<CUIElement*>(m_pTextOwner)->TransformToRoot(transformer.ReleaseAndGetAddressOf()));
    IFC_RETURN(transformer->ReverseTransform(&point, &pixelCoordinate, 1));
    IFC_RETURN(pTextView->PixelPositionToTextPosition(pixelCoordinate, TRUE, &textPosition, &gravity));

    // Pixel position is the start of the range
    plainTextPosition = CPlainTextPosition(pTextContainer, (uint32_t)startEdge, gravity);
    IFC_RETURN(CTextPointerWrapper::Create(core, plainTextPosition, startPointer.ReleaseAndGetAddressOf()));
    // Add the length to the first character to get the ending text position
    plainTextPosition = CPlainTextPosition(pTextContainer, (uint32_t)startEdge + linkLength, gravity);
    IFC_RETURN(CTextPointerWrapper::Create(core, plainTextPosition, endPointer.ReleaseAndGetAddressOf()));

    IFC_RETURN(CTextRangeAdapter::Create(core, this, startPointer, endPointer, m_pTextOwner, textRangeAdapter.ReleaseAndGetAddressOf()));
    *ppTextRangeAdapter = textRangeAdapter.detach();

    return S_OK;
}

//------------------------------------------------------------------------------------------------------------------
// Retrieves a text range from the vicinity of a screen coordinate. It returns a degenerated TextRange.
//------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextAdapter::RangeFromPoint(_In_ XPOINTF point, _Outptr_ CTextRangeAdapter **ppTextRangeAdapter)
{
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    ITextView* pTextView = nullptr;
    xref_ptr<CTextPointerWrapper> startPointer;
    xref_ptr<CTextPointerWrapper> endPointer;
    XPOINTF pixelCoordinate;
    uint32_t textPosition;
    CPlainTextPosition plainTextPosition;
    TextGravity gravity;
    xref_ptr<ITransformer> transformer;

    IFCPTR_RETURN(ppTextRangeAdapter);
    *ppTextRangeAdapter = nullptr;

    pTextView = CTextAdapter::GetTextView(m_pTextOwner);
    pixelCoordinate = point;

    if (pTextView)
    {
        // Get the text position from the given pixel position
        auto core = GetContext();
        IFC_RETURN(static_cast<CUIElement*>(m_pTextOwner)->TransformToRoot(transformer.ReleaseAndGetAddressOf()));
        IFC_RETURN(transformer->ReverseTransform(&point, &pixelCoordinate, 1));
        IFC_RETURN(pTextView->PixelPositionToTextPosition(pixelCoordinate, TRUE, &textPosition, &gravity));
        plainTextPosition = CPlainTextPosition(CTextAdapter::GetTextContainer(m_pTextOwner), textPosition, gravity);
        IFC_RETURN(CTextPointerWrapper::Create(core, plainTextPosition, startPointer.ReleaseAndGetAddressOf()));
        IFC_RETURN(CTextPointerWrapper::Create(core, plainTextPosition, endPointer.ReleaseAndGetAddressOf()));

        IFC_RETURN(CTextRangeAdapter::Create(core, this, startPointer, endPointer, m_pTextOwner, textRangeAdapter.ReleaseAndGetAddressOf()));
        *ppTextRangeAdapter = textRangeAdapter.detach();
    }

    return S_OK;
}

_Check_return_ HRESULT CTextAdapter::RangeFromInlineUIContainer(_In_ CInlineUIContainer* iuc, _Outptr_ CTextRangeAdapter **ppTextRangeAdapter)
{
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    xref_ptr<CTextPointerWrapper> startPointer;
    xref_ptr<CTextPointerWrapper> endPointer;

    PageNode* pPageNode = CTextAdapter::GetPageNode(m_pTextOwner);
    uint32_t textPosition = pPageNode->FindInlineUIContainerOffset(iuc);
    CPlainTextPosition plainTextPosition = CPlainTextPosition(CTextAdapter::GetTextContainer(m_pTextOwner), textPosition, LineForwardCharacterBackward);

    auto core = GetContext();
    IFC_RETURN(CTextPointerWrapper::Create(core, plainTextPosition, startPointer.ReleaseAndGetAddressOf()));
    IFC_RETURN(CTextPointerWrapper::Create(core, plainTextPosition, endPointer.ReleaseAndGetAddressOf()));

    IFC_RETURN(CTextRangeAdapter::Create(core, this, startPointer, endPointer, m_pTextOwner, textRangeAdapter.ReleaseAndGetAddressOf()));
    *ppTextRangeAdapter = textRangeAdapter.detach();

    return S_OK;
}

_Check_return_ HRESULT CTextAdapter::GetContentEndPointers(
    _In_ CDependencyObject* pObject,
    _Outptr_ CTextPointerWrapper **ppStartTextPointerWrapper,
    _Outptr_ CTextPointerWrapper **ppEndTextPointerWrapper)
{
    xref_ptr<CTextPointerWrapper> startTextPointerWrapper;
    xref_ptr<CTextPointerWrapper> endTextPointerWrapper;

    *ppStartTextPointerWrapper = nullptr;
    *ppEndTextPointerWrapper = nullptr;

    if (pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
        {
            CTextBlock* pTbl = static_cast<CTextBlock*>(pObject);
            IFC_RETURN(pTbl->GetContentStart(startTextPointerWrapper.ReleaseAndGetAddressOf()));
            IFC_RETURN(pTbl->GetContentEnd(endTextPointerWrapper.ReleaseAndGetAddressOf()));
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            IFC_RETURN(pRTbl->GetContentStart(startTextPointerWrapper.ReleaseAndGetAddressOf()));
            IFC_RETURN(pRTbl->GetContentEnd(endTextPointerWrapper.ReleaseAndGetAddressOf()));
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            IFC_RETURN(pRTblo->GetContentStart(startTextPointerWrapper.ReleaseAndGetAddressOf()));
            IFC_RETURN(pRTblo->GetContentEnd(endTextPointerWrapper.ReleaseAndGetAddressOf()));
        }
    }
    *ppStartTextPointerWrapper = startTextPointerWrapper.detach();
    *ppEndTextPointerWrapper = endTextPointerWrapper.detach();

    return S_OK;
}

_Check_return_ HRESULT CTextAdapter::GetSelectionEndPointers(
    _In_ CDependencyObject* pObject,
    _Outptr_ CTextPointerWrapper **ppSelectionStartTextPointerWrapper,
    _Outptr_ CTextPointerWrapper **ppSelectionEndTextPointerWrapper)
{
    xref_ptr<CTextPointerWrapper> contentStartTextPointer;
    xref_ptr<CTextPointerWrapper> contentEndTextPointer;
    xref_ptr<CTextPointerWrapper> selectionStartTextPointer;
    xref_ptr<CTextPointerWrapper> selectionEndTextPointer;

    XINT32 selectionStartOffset = 0;
    XINT32 selectionEndOffset = 0;
    XINT32 contentStartOffset = 0;
    XINT32 contentEndOffset = 0;

    *ppSelectionStartTextPointerWrapper = nullptr;
    *ppSelectionEndTextPointerWrapper = nullptr;

    if (pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
        {
            CTextBlock* pTbl = static_cast<CTextBlock*>(pObject);
            IFC_RETURN(pTbl->GetSelectionStart(selectionStartTextPointer.ReleaseAndGetAddressOf()));
            IFC_RETURN(pTbl->GetSelectionEnd(selectionEndTextPointer.ReleaseAndGetAddressOf()));
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            IFC_RETURN(pRTbl->GetSelectionStart(selectionStartTextPointer.ReleaseAndGetAddressOf()));
            IFC_RETURN(pRTbl->GetSelectionEnd(selectionEndTextPointer.ReleaseAndGetAddressOf()));
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            CRichTextBlock* pRtbl = pRTblo->GetMaster();
            if(pRtbl)
            {
                IFC_RETURN(pRtbl->GetSelectionStart(selectionStartTextPointer.ReleaseAndGetAddressOf()));
                IFC_RETURN(pRtbl->GetSelectionEnd(selectionEndTextPointer.ReleaseAndGetAddressOf()));
            }
        }

        IFC_RETURN(CTextAdapter::GetContentEndPointers(pObject,
            contentStartTextPointer.ReleaseAndGetAddressOf(),
            contentEndTextPointer.ReleaseAndGetAddressOf()));

        // Essentially Selection extends between RichTextBlock and RichTextBlockOverflow.
        // But as on TextPattern side it was decided to treat them as individual TextAdapters
        // We want to make sure to cut off within current DocumentRange.
        if(selectionStartTextPointer && selectionEndTextPointer && contentStartTextPointer && contentEndTextPointer)
        {
            IFC_RETURN(selectionStartTextPointer->GetOffset(&selectionStartOffset));
            IFC_RETURN(selectionEndTextPointer->GetOffset(&selectionEndOffset));
            IFC_RETURN(contentStartTextPointer->GetOffset(&contentStartOffset));
            IFC_RETURN(contentEndTextPointer->GetOffset(&contentEndOffset));

            if (selectionStartOffset <= contentStartOffset)
            {
                IFC_RETURN(contentStartTextPointer->Clone(pObject, selectionStartTextPointer.ReleaseAndGetAddressOf()));
            }

            if (selectionEndOffset <= selectionStartOffset)
            {
                IFC_RETURN(selectionStartTextPointer->Clone(pObject, selectionEndTextPointer.ReleaseAndGetAddressOf()));
            }

            if (selectionStartOffset > contentEndOffset)
            {
                IFC_RETURN(contentEndTextPointer->Clone(pObject, selectionStartTextPointer.ReleaseAndGetAddressOf()));
            }

            if (selectionEndOffset > contentEndOffset)
            {
                IFC_RETURN(contentEndTextPointer->Clone(pObject, selectionEndTextPointer.ReleaseAndGetAddressOf()));
            }
        }
    }
    *ppSelectionStartTextPointerWrapper = selectionStartTextPointer.detach();
    *ppSelectionEndTextPointerWrapper = selectionEndTextPointer.detach();

    return S_OK;
}

// Helper to get ITextView specific to the different control. If returning the text view for
// a RTB or RTBOverflow, returns TextView for just this element that has no knowledge of linked layout.
_Check_return_ ITextView* CTextAdapter::GetTextView(_In_ CDependencyObject* pObject)
{
    ITextView* pTextView = nullptr;

    if(pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
        {
            CTextBlock* pTbl = static_cast<CTextBlock*>(pObject);
            pTextView = pTbl->GetTextView();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            pTextView = (ITextView*)pRTbl->GetSingleElementTextView();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            pTextView = (ITextView*)pRTblo->GetSingleElementTextView();
        }
    }

    return pTextView;
}

//----------------------------------------------------------------------------------------------
// Helper to get TextContainer specific to the different control.
//----------------------------------------------------------------------------------------------
_Check_return_ ITextContainer* CTextAdapter::GetTextContainer(_In_ CDependencyObject* pObject)
{
    ITextContainer* pTextContainer = nullptr;

    if(pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
        {
            CTextBlock* pTbl = static_cast<CTextBlock*>(pObject);
            pTextContainer = pTbl->GetTextContainer();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            pTextContainer = pRTbl->GetTextContainer();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            CRichTextBlock* pRtbl = pRTblo->GetMaster();
            if(pRtbl)
            {
                pTextContainer = pRtbl->GetTextContainer();
            }
        }
    }

    return pTextContainer;
}

_Check_return_ CBlockCollection* CTextAdapter::GetBlockCollection(_In_ CDependencyObject* pObject)
{
    CBlockCollection* pBlockCollection = nullptr;

    if(pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            pBlockCollection = pRTbl->GetBlockCollection();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            CRichTextBlock* pRtbl = pRTblo->GetMaster();
            if(pRtbl)
            {
                pBlockCollection = pRtbl->GetBlockCollection();
            }
        }
    }

    return pBlockCollection;
}

_Check_return_ PageNode* CTextAdapter::GetPageNode(_In_ CDependencyObject* pObject)
{
    PageNode* pPageNode = nullptr;

    if (pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            pPageNode = pRTbl->GetPageNode();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            pPageNode = pRTblo->GetPageNode();
        }
    }

    return pPageNode;
}

//----------------------------------------------------------------------------------------------
// Helper to get TextSelectionManager specific to the different control.
//----------------------------------------------------------------------------------------------
_Check_return_ TextSelectionManager* CTextAdapter::GetSelectionManager(_In_opt_ CDependencyObject* pObject)
{
    TextSelectionManager* pTextSelectionManager = nullptr;

    if (pObject)
    {
        if (pObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
        {
            CTextBlock* pTbl = static_cast<CTextBlock*>(pObject);
            pTextSelectionManager = pTbl->GetSelectionManager();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            CRichTextBlock* pRTbl = static_cast<CRichTextBlock*>(pObject);
            pTextSelectionManager = pRTbl->GetSelectionManager();
        }
        else if (pObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
        {
            CRichTextBlockOverflow* pRTblo = static_cast<CRichTextBlockOverflow*>(pObject);
            CRichTextBlock* pRtbl = pRTblo->GetMaster();
            if(pRtbl)
            {
                pTextSelectionManager = pRtbl->GetSelectionManager();
            }
        }
    }

    return pTextSelectionManager;
}

// We only need to ensure the TextBlockView for TextBlocks which are on the fast-path.
_Check_return_ HRESULT CTextAdapter::EnsureTextBlockView()
{
    if (m_pTextOwner->GetTypeIndex() == KnownTypeIndex::TextBlock)
    {
        CTextBlock* pTextBlock = static_cast<CTextBlock*>(m_pTextOwner);
        IFC_RETURN(pTextBlock->TextRangeAdapterAssociated());
    }
    return S_OK;
}
