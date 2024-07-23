// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlOneCoreTransforms.h>
#include "TextNavigationHelper.h"
#include "RootScale.h"

using namespace DirectUI;

CTextRangeAdapter::~CTextRangeAdapter()
{
    ReleaseInterface(m_pStartTextPointer);
    ReleaseInterface(m_pEndTextPointer);
    ReleaseInterface(m_pTextAdapter);
    delete[] m_attributeValueInfoTable;
}

_Check_return_ HRESULT CTextRangeAdapter::Create(
    _In_ CCoreServices *pCore,
    _In_ CTextAdapter *pTextAdapter,
    _In_ CTextPointerWrapper *pStartTextPointer,
    _In_ CTextPointerWrapper *pEndTextPointer,
    _In_ CDependencyObject *pTextOwner,
    _Outptr_ CTextRangeAdapter **ppTextRangeAdapter
)
{
    CREATEPARAMETERS createParameters(pCore);
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    *ppTextRangeAdapter = nullptr;

    IFC_RETURN(CreateDO(textRangeAdapter.ReleaseAndGetAddressOf(), &createParameters));
    IFC_RETURN(textRangeAdapter->Initialize(pTextAdapter, pStartTextPointer, pEndTextPointer, pTextOwner));
    IFC_RETURN(pTextAdapter->EnsureTextBlockView()); // Currently only normal TextBlock will support proper TextPattern features.

    *ppTextRangeAdapter = textRangeAdapter.detach();

    return S_OK;
}


_Check_return_ HRESULT CTextRangeAdapter::Clone(_Outptr_ CTextRangeAdapter** ppCloneObject)
{
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    xref_ptr<CTextPointerWrapper> startPointer;
    xref_ptr<CTextPointerWrapper> endPointer;
    XINT32 startOffset;
    XINT32 endOffset;

    *ppCloneObject = nullptr;

    IFC_RETURN(m_pStartTextPointer->GetOffset(&startOffset));
    IFC_RETURN(m_pEndTextPointer->GetOffset(&endOffset));

    IFC_RETURN(m_pStartTextPointer->Clone(m_pTextOwner, startPointer.ReleaseAndGetAddressOf()));
    IFC_RETURN(m_pEndTextPointer->Clone(m_pTextOwner, endPointer.ReleaseAndGetAddressOf()));

    IFC_RETURN(CTextRangeAdapter::Create(GetContext(), m_pTextAdapter, startPointer, endPointer, m_pTextOwner, textRangeAdapter.ReleaseAndGetAddressOf()));

    *ppCloneObject = textRangeAdapter.detach();

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::Compare(_In_ CTextRangeAdapter* pTargetRange, _Out_ bool* bRetVal)
{
    XINT32 startOffset;
    XINT32 endOffset;
    XINT32 targetStartOffset;
    XINT32 targetEndOffset;

    *bRetVal = FALSE;
    if (m_pTextOwner == pTargetRange->m_pTextOwner)
    {
        IFC_RETURN(m_pStartTextPointer->GetOffset(&startOffset));
        IFC_RETURN(m_pEndTextPointer->GetOffset(&endOffset));
        IFC_RETURN(pTargetRange->m_pStartTextPointer->GetOffset(&targetStartOffset));
        IFC_RETURN(pTargetRange->m_pEndTextPointer->GetOffset(&targetEndOffset));

        if (startOffset == targetStartOffset &&
            endOffset == targetEndOffset)
        {
            *bRetVal = TRUE;
        }
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::CompareEndpoints(
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ CTextRangeAdapter* pTargetTextRangeProvider,
    _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint,
    _Out_ XINT32* pReturnValue)
{
    CTextPointerWrapper *pCallerEndPoint = nullptr;
    CTextPointerWrapper *pTargetEndPoint = nullptr;
    XINT32 callerEndPointPosition = 0;
    XINT32 targetEndPointPosition = 0;

    *pReturnValue = 0;
    if (m_pTextOwner == pTargetTextRangeProvider->m_pTextOwner)
    {
        pCallerEndPoint = endPoint == UIAXcp::TextPatternRangeEndpoint_Start ? m_pStartTextPointer : m_pEndTextPointer;
        pTargetEndPoint = targetEndPoint == UIAXcp::TextPatternRangeEndpoint_Start ? pTargetTextRangeProvider->m_pStartTextPointer : pTargetTextRangeProvider->m_pEndTextPointer;

        IFC_RETURN(pCallerEndPoint->GetOffset(&callerEndPointPosition));
        IFC_RETURN(pTargetEndPoint->GetOffset(&targetEndPointPosition));

        *pReturnValue = callerEndPointPosition - targetEndPointPosition;
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetEnclosingElement
//
//  Synopsis:
//      Returns innermost element that encloses the textrange.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextRangeAdapter::GetEnclosingElement(_Outptr_ CAutomationPeer** pRetVal)
{
    CAutomationPeer *pAP = nullptr;

    IFCPTR_RETURN(pRetVal);

    if (m_pTextOwner)
    {
        CInline* link = nullptr;
        // If the range is in a link, that link should be the enclosing element
        if (RangeIsInLink(&link))
        {
            pAP = link->OnCreateAutomationPeer();
        }
        else
        {
            pAP = m_pTextOwner->OnCreateAutomationPeer();
        }

    }

    *pRetVal = pAP;

    return S_OK;
}

bool CTextRangeAdapter::RangeIsInLink(_Outptr_result_maybenull_ CInline **ppLink)
{
    CTextElement *run = nullptr;
    const WCHAR *pRunCharacters = nullptr;
    CHyperlink *hyperlink = nullptr;
    CHyperlink *hyperlink2 = nullptr;
    uint32_t runTextLength = 0;
    int startOffset = 0;
    int endOffset = 0;

    *ppLink = nullptr;

    IFCFAILFAST(m_pStartTextPointer->GetOffset(&startOffset));
    IFCFAILFAST(m_pEndTextPointer->GetOffset(&endOffset));

    ITextContainer* pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);
    if (pTextContainer == nullptr)
    {
        return false;
    }
    // Don't let UIA get stuck right at the end of a line
    uint32_t containerPositions = 0;
    pTextContainer->GetPositionCount(&containerPositions);
    if ((uint32_t)startOffset >= containerPositions ||
        (uint32_t)endOffset >= containerPositions)
    {
        return false;
    }

    IFCFAILFAST(pTextContainer->GetRun(startOffset, nullptr, nullptr, nullptr, &run, &pRunCharacters, &runTextLength));
    if (run != nullptr
        && run->GetTypeIndex() == KnownTypeIndex::Run
        && static_cast<CRun*>(run)->IsInsideHyperlink(&hyperlink))
    {
        IFCFAILFAST(pTextContainer->GetRun(endOffset, nullptr, nullptr, nullptr, &run, &pRunCharacters, &runTextLength));
        if (run != nullptr
            && run->GetTypeIndex() == KnownTypeIndex::Run
            && static_cast<CRun*>(run)->IsInsideHyperlink(&hyperlink2))
        {
            if (hyperlink == hyperlink2)
            {
                *ppLink = hyperlink;
                return true;
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// Returns an array of APs for children UIEs within this range
//--------------------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextRangeAdapter::GetChildren(_Outptr_result_buffer_all_(*pnCount) CAutomationPeer ***pppChildren, _Out_ XINT32* pnCount)
{
    HRESULT hr = S_OK;
    PageNode* pPageNode = nullptr;
    CAutomationPeer *pAP = nullptr;
    CInlineUIContainer** ppContainerElements = nullptr;
    xref_ptr<CUIElement> uiElement;
    xvector<CAutomationPeer*> pAPCollection;
    CAutomationPeer **ppAPSubChildren = nullptr;
    int32_t subChildrenCount = 0;
    int32_t containerCount = 0;
    uint32_t startOffset;
    uint32_t endOffset;

    *pnCount = 0;
    *pppChildren = nullptr;

    IFC(m_pStartTextPointer->GetPlainTextPosition().GetOffset(&startOffset));
    IFC(m_pEndTextPointer->GetPlainTextPosition().GetOffset(&endOffset));
    pPageNode = CTextAdapter::GetPageNode(m_pTextOwner);
    if (pPageNode)
    {
        IFC(pPageNode->GetElementsWithinRange(startOffset, endOffset, &ppContainerElements, &containerCount));
        for (int32_t current = 0; current < containerCount; current++)
        {
            IFC(ppContainerElements[current]->GetChild(uiElement.ReleaseAndGetAddressOf()));
            if (uiElement)
            {
                pAP = uiElement->OnCreateAutomationPeer();
                if (pAP)
                {
                    IFC(pAPCollection.push_back(pAP));
                    AddRefInterface(pAP);
                }
                else
                {
                    subChildrenCount = uiElement->GetAPChildren(&ppAPSubChildren);
                    for (int32_t i = 0; i < subChildrenCount; i++)
                    {
                        IFC(pAPCollection.push_back(ppAPSubChildren[i]));
                        AddRefInterface(ppAPSubChildren[i]);
                    }
                }
                delete[] ppAPSubChildren;
                ppAPSubChildren = nullptr;
            }
        }
    }

    IFC(GetLinkChildrenWithinRange(&pAPCollection, startOffset, endOffset));

    containerCount = pAPCollection.size();
    if (containerCount > 0)
    {
        *pppChildren  = new CAutomationPeer *[containerCount];
        for (XINT32 i =0; i < containerCount; i++)
        {
            pAPCollection.get_item(i, ((*pppChildren)[i]));
        }
        *pnCount = containerCount;
    }

Cleanup:
    delete[] ppContainerElements;
    delete[] ppAPSubChildren;
    if (hr != S_OK)
    {
        HRESULT hr2 = S_OK;
        for(XUINT32 i =0; i < pAPCollection.size(); i++)
        {
            hr2 = pAPCollection.get_item(i, pAP);
            if (hr2 != S_OK)
            {
                hr = hr2;
            }
            ReleaseInterface(pAP);
        }
    }
    return hr;
}

_Check_return_ HRESULT CTextRangeAdapter::GetLinkChildrenWithinRange(
    _Inout_ xvector<CAutomationPeer*> *pAPCollection,
    _In_ uint32_t startOffset,
    _In_ uint32_t endOffset)
{
    // Focusable children are always Hyperlinks
    CDOCollection *focusableChildren = nullptr;

    // If the range is already the hyperlink, don't burrow further
    CAutomationPeer *pAP = nullptr;
    IFC_RETURN(this->GetEnclosingElement(&pAP));
    if (pAP != nullptr &&
         (pAP->OfTypeByIndex<KnownTypeIndex::HyperlinkAutomationPeer>()))
    {
        return S_OK;
    }

    if (m_pTextOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        IFC_RETURN((static_cast<CTextBlock*>(m_pTextOwner))->GetFocusableChildren(&focusableChildren));
    }
    else
    {
        CDependencyObject* owner = m_pTextOwner;
        if (owner->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
        {
            owner = static_cast<CRichTextBlockOverflow*>(owner)->GetMaster();
        }
        if (owner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            IFC_RETURN((static_cast<CRichTextBlock*>(owner))->GetFocusableChildren(&focusableChildren));
        }
    }
    IFC_RETURN(GetFocusableChildrenInRange(pAPCollection, focusableChildren, startOffset, endOffset));

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::GetFocusableChildrenInRange(
    _Inout_ xvector<CAutomationPeer*> *pAPCollection,
    _In_opt_ CDOCollection *focusableChildren,
    _In_ uint32_t startOffset,
    _In_ uint32_t endOffset)
{
    if (focusableChildren == nullptr || focusableChildren->GetCount() == 0)
        return S_OK;

    CAutomationPeer *pAP = nullptr;
    auto& children = focusableChildren->GetCollection();

    for (const auto& child : children)
    {
        CInline* link = static_cast<CInline*>(child);
        if (IsLinkWithinRange(link, startOffset, endOffset))
        {
            pAP = link->OnCreateAutomationPeer();
            if (pAP)
            {
                IFC_RETURN(pAPCollection->push_back(pAP));
                AddRefInterface(pAP);
            }
        }
    }

    return S_OK;
}

bool CTextRangeAdapter::IsLinkWithinRange(
    _In_ CInline* link,
    _In_ uint32_t startOffset,
    _In_ uint32_t endOffset)
{
    xref_ptr<CTextPointerWrapper> start;
    xref_ptr<CTextPointerWrapper> end;
    XINT32 linkStartOffset = 0;
    XINT32 linkEndOffset = 0;

    if (link->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
    {
        CHyperlink* hyperlink = static_cast<CHyperlink*>(link);
        IFCFAILFAST(hyperlink->GetTextContentStart(start.ReleaseAndGetAddressOf()));
        IFCFAILFAST(hyperlink->GetTextContentEnd(end.ReleaseAndGetAddressOf()));
    }
    else
    {
        // Could be an InlineUIContainer
        return false;
    }

    if (start == nullptr || end == nullptr)
    {
        return false;
    }
    IFCFAILFAST(start->GetOffset(&linkStartOffset));
    IFCFAILFAST(end->GetOffset(&linkEndOffset));

    //  The definition of being a text range child is "any object that is partially or fully contained by the range but does not contain the range".
    bool isStartWithinRange = (XUINT32)linkStartOffset >= startOffset && (XUINT32)linkStartOffset < endOffset;
    bool isEndWithinRange = (XUINT32)linkEndOffset > startOffset && (XUINT32)linkEndOffset <= endOffset;

    return isStartWithinRange || isEndWithinRange;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToEnclosingUnit(_In_ UIAXcp::TextUnit unit)
{
    uint32_t moveCount = 0;
    bool doExecuteForNextUnit = false;
    bool bIsEmptyLine = false;

    // first make it a degenerated Range, then Expand to specified unit
    IFC_RETURN(Normalize(this));

    // Expanding Start to unit boundry after normalizing will move end to the unit boundary and then we can move back the start by a unit
    // This saves us a separate check for if the pointer is at unit boundary itself, which otherwise needed to be handeled differently than
    // default Expand.
    switch(unit)
    {
    case UIAXcp::TextUnit_Character :
        IFC_RETURN(ExpandToCharacter(UIAXcp::TextPatternRangeEndpoint_End, this, 1, &moveCount));
        break;
    case UIAXcp::TextUnit_Format :
        IFC_RETURN(ExpandToFormat(UIAXcp::TextPatternRangeEndpoint_Start, this, 1, &moveCount));
        if (moveCount > 0)
        {
            IFC_RETURN(ExpandToFormat(UIAXcp::TextPatternRangeEndpoint_Start, this, -1, &moveCount));
        }
        break;
    case UIAXcp::TextUnit_Word :
        IFC_RETURN(ExpandToWord(UIAXcp::TextPatternRangeEndpoint_Start, this, 1, &moveCount));
        if (moveCount > 0)
        {
            IFC_RETURN(ExpandToWord(UIAXcp::TextPatternRangeEndpoint_Start, this, -1, &moveCount));
        }
        break;
    case UIAXcp::TextUnit_Line :
        IFC_RETURN(IsAtEmptyLine(&bIsEmptyLine)); /*Empty line is already an enclosing unit*/
        if (!bIsEmptyLine)
        {
            IFC_RETURN(ExpandToLine(UIAXcp::TextPatternRangeEndpoint_Start, this, 1, &moveCount));
            if (moveCount > 0)
            {
                IFC_RETURN(ExpandToLine(UIAXcp::TextPatternRangeEndpoint_Start, this, -1, &moveCount));
            }
        }
        break;
    case UIAXcp::TextUnit_Paragraph :
        IFC_RETURN(ExpandToParagraph(UIAXcp::TextPatternRangeEndpoint_Start, this, 1, &moveCount, &doExecuteForNextUnit));
        if (!doExecuteForNextUnit)
        {
            if (moveCount > 0)
            {
                IFC_RETURN(ExpandToParagraph(UIAXcp::TextPatternRangeEndpoint_Start, this, -1, &moveCount, &doExecuteForNextUnit));
            }
            break;
        }
    case UIAXcp::TextUnit_Page :
        IFC_RETURN(ExpandToPage(this, 1));
    case UIAXcp::TextUnit_Document :
        IFC_RETURN(ExpandToDocument(this, 1));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::GetAttributeValue(DirectUI::AutomationTextAttributesEnum attributeID, _Out_ CValue* pRetVal)
{
    ITextContainer *pTextContainer = nullptr;
    CTextElement *pContainingElement = nullptr;
    const InheritedProperties* pInheritedProperties = nullptr;
    TextNestingType textNestingType;
    const TextFormatting *pFormatting = nullptr;
    xref_ptr<CTextPointerWrapper> elementEndPositionPoint;
    CParagraph* pParagraph = nullptr;
    const WCHAR* pCharacters = nullptr;
    XUINT32 cCharacters = 0;
    XUINT32 rangeEndPosition = 0;
    XUINT32 elementEndPosition = 0;
    CValue value;
    bool isValueSame = false;
    XUINT32 positionCount = 0;
    XUINT32 characterPosition = 0;

    IFCPTR_RETURN(pRetVal);
    pRetVal->SetNull();
    value.SetNull();

    IFC_RETURN(m_pStartTextPointer->GetPlainTextPosition().GetOffset(&characterPosition));
    IFC_RETURN(m_pEndTextPointer->GetPlainTextPosition().GetOffset(&rangeEndPosition));
    pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);
    pTextContainer->GetPositionCount(&positionCount);
    rangeEndPosition = rangeEndPosition < positionCount ? rangeEndPosition : positionCount;

    // loop through next chunk utilizing number of characters in previous chunk.
    // we have to loop through different spot within range to make sure attribute value is same
    // across whole range. In case we find a difference, we always return MixedAttribute.
    // TODO:: use same approach as in Move APIs instead of depending upon GetRun.
    do
    {
        do
        {
            // This stub eliminates the hidden runs and also goes to next chunk
            characterPosition += cCharacters;
            // If rangeEndPosition is more than character Position we want to bail out for sure,
            if (characterPosition > rangeEndPosition || characterPosition >= positionCount)
            {
                // The cases where we really fail, we must return Not supported.
                if (characterPosition >= positionCount && value.IsNull())
                {
                    IFC_RETURN(E_NOT_SUPPORTED);
                }
                else
                {
                    return S_OK;
                }
            }
            IFC_RETURN(pTextContainer->GetRun(characterPosition, &pFormatting, &pInheritedProperties, &textNestingType, &pContainingElement,  &pCharacters, &cCharacters));

            // For degen range (begin = end), do not bail out.
            if (characterPosition == rangeEndPosition)
            {
                break;
            }
        } while (pContainingElement == nullptr && cCharacters > 0); // If we don't have a valid text element, try to move forward.

        if (pContainingElement == nullptr && characterPosition == rangeEndPosition)
        {
            uint32_t adjustedPosition = characterPosition;
            // GetRun can return containing Element as null. When the character position is at the reserved/hidden positions.
            // We don't want to bail out yet for empty TextBlock. We move forward/backward by 1 position accordingly and try to get run again.
            // There should be at least one Run in the container so we shouldn't hit infinite loop.
            while (textNestingType == OpenNesting)
            {
                adjustedPosition++;
                ASSERT(adjustedPosition < positionCount);
                IFC_RETURN(pTextContainer->GetRun(adjustedPosition, &pFormatting, &pInheritedProperties, &textNestingType, &pContainingElement,  &pCharacters, &cCharacters));
            }
            while (textNestingType == CloseNesting)
            {
                ASSERT(adjustedPosition > 0);
                --adjustedPosition;
                IFC_RETURN(pTextContainer->GetRun(adjustedPosition, &pFormatting, &pInheritedProperties, &textNestingType, &pContainingElement,  &pCharacters, &cCharacters));
            }
        }

        if (pContainingElement == nullptr)
        {
            break; // We really need to bail out now.
        }
        pParagraph = CTextRangeAdapter::GetParagraphFromTextElement(pContainingElement);
        IFC_RETURN(GetAttributeValueFromTextElement(attributeID, pContainingElement, &value, pParagraph));

        // we don't need the comparison very first time.
        if (pRetVal->IsNull() != TRUE)
        {
            IFC_RETURN(AttributeValueComparer(*pRetVal, value, &isValueSame));

            // If we find mixed values for the attribute we want to return MixedAttribute IUnknown to UIACore.
            // As IInspectable doesn't support wrapping IUnknown publicly and as we have to return to DXAML layer where
            // expected return value is IInspectable we use a hack of EmptyInspectable in case of type being IUnknown.
            // We fill-in the right MixedAttribute in CValue once we return to CManagedTextRangeProvider in core.
            if (isValueSame == FALSE)
            {
                pRetVal->SetIUnknownNoRef(nullptr);
                break;
            }
        }
        IFC_RETURN(pContainingElement->GetContentEnd(elementEndPositionPoint.ReleaseAndGetAddressOf()));
        IFC_RETURN(elementEndPositionPoint->GetPlainTextPosition().GetOffset(&elementEndPosition));
        IFC_RETURN(pRetVal->CopyConverted(value));
    } while (rangeEndPosition > elementEndPosition);

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::GetText(_In_ XUINT32 maxLength, _Out_ xstring_ptr* pstrRetVal)
{
    HRESULT hr = S_OK;
    XINT32 startOffset = 0;
    XINT32 endOffset = 0;
    XUINT32 cCharacters = 0;
    const WCHAR *pCharacters = nullptr;
    bool isTakeOwnership = false;

    pstrRetVal->Reset();

    ITextContainer *pTextContainer = (m_pStartTextPointer->GetPlainTextPosition()).GetTextContainer();
    IFC(m_pStartTextPointer->GetOffset(&startOffset));
    IFC(m_pEndTextPointer->GetOffset(&endOffset));
    IFC(pTextContainer->GetText(startOffset, endOffset, TRUE, &cCharacters, &pCharacters, &isTakeOwnership));

    if (pCharacters && cCharacters > 0)
    {
        // PREfast believes that the above call to pTextContainer->GetText overran the buffer. It does not since the buffer is
        // allocated in GetText.
        _Analysis_assume_(cCharacters == 0);
        IFC(xstring_ptr::CloneBuffer(pCharacters, cCharacters, pstrRetVal));
    }

Cleanup:
    if (isTakeOwnership)
    {
        delete [] pCharacters;
    }
    return hr;
}

_Check_return_ HRESULT CTextRangeAdapter::GetBoundingRectangles(_Outptr_result_buffer_all_(*pnCount) XDOUBLE** ppRectangles, _Out_ XINT32* pnCount)
{
    HRESULT hr = S_OK;
    ITextView* pTextView = nullptr;
    int32_t startOffset;
    int32_t endOffset;
    uint32_t cRectangles = 0;
    XRECTF *pRectangles = nullptr;
    double* pFlatRectangles = nullptr;

    *ppRectangles = nullptr;
    *pnCount = 0;

    IFC(m_pStartTextPointer->GetOffset(&startOffset));
    IFC(m_pEndTextPointer->GetOffset(&endOffset));
    pTextView = CTextAdapter::GetTextView(m_pTextOwner);

    if (pTextView)
    {
        const float scale = RootScale::GetRasterizationScaleForElement(m_pTextOwner);
        IFC(pTextView->TextRangeToTextBounds(startOffset, endOffset, &cRectangles, &pRectangles));
        if (cRectangles > 0)
        {
            pFlatRectangles  = new XDOUBLE [4 * cRectangles];
            // Flatten the Rect list to double Array as Variant do not support array of Rects.
            for (XUINT32 current = 0; current < cRectangles; current++)
            {
                XRECTF_RB pBounds = { };
                IFC(static_cast<CUIElement*>(m_pTextOwner)->TransformToWorldSpace(&(ToXRectFRB(pRectangles[current])), &pBounds, false /* ignoreClipping */, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));
                pRectangles[current] = ToXRectF(pBounds);
                if (XamlOneCoreTransforms::IsEnabled())
                {
                    // In OneCoreTransforms mode, TransformToWorldSpace returns logical pixels so we must convert to RasterizedClient
                    const auto logicalRect = pRectangles[current];
                    const auto physicalRect = logicalRect * scale;
                    pRectangles[current] = physicalRect;
                }

                pFlatRectangles[4 * current] = pRectangles[current].X;
                pFlatRectangles[4 * current + 1] = pRectangles[current].Y;
                pFlatRectangles[4 * current + 2] = pRectangles[current].Width;
                pFlatRectangles[4 * current + 3] = pRectangles[current].Height;
            }
        }
    }
    *ppRectangles = pFlatRectangles;
    pFlatRectangles =nullptr;
    *pnCount = 4 * cRectangles;

Cleanup:
    delete [] pFlatRectangles;
    delete [] pRectangles;
    return hr;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
//   For a non-degenerate (non-empty) text range, ITextRangeProvider::Move should normalize and move the text range by performing the following
//   steps.
//     1.Collapse the text range to a degenerate (empty) range at the starting endpoint.
//     2.If necessary, move the resulting text range backward in the document to the beginning of the requested unit boundary.
//     3.Move the text range forward or backward in the document by the requested number of text unit boundaries.
//     4.Expand the text range from the degenerate state by moving the ending endpoint forward by one requested text unit boundary.
//
//    If any of the preceding steps fail, the text range should be left unchanged. If the text range cannot be moved as far as the requested
//    number of text units, but can be moved by a smaller number of text units, the text range should be moved by the smaller number of text
//    units and pRetVal should be set to the number of text units moved successfully.
//----------------------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextRangeAdapter::Move(_In_ UIAXcp::TextUnit unit, _In_ XINT32 count, _Out_ XINT32* pReturnValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CTextRangeAdapter> clonedRange;
    xref_ptr<CTextRangeAdapter> documentRangeAdapter;
    XINT32 moveCount = 0;
    XUINT32  cCharacters = 0;
    const WCHAR *pCharacters = nullptr;
    XUINT32 startOffset;
    XUINT32 endOffset;
    bool isTakeOwnership = false;
    bool bIsEmptyLine = false;


    *pReturnValue = 0;

    // If we can't move to Next TextUnit as it doesn't exist we discard the whole operation. Hence this API in our
     // design becomes no-op for Page and Document.
    if (count != 0 && (unit != UIAXcp::TextUnit_Page) && (unit != UIAXcp::TextUnit_Document))
    {
        IFC(m_pTextAdapter->GetDocumentRange(documentRangeAdapter.ReleaseAndGetAddressOf()));
        IFC(Clone(clonedRange.ReleaseAndGetAddressOf()));
        IFCEXPECT(clonedRange);

        IFC(ExpandToEnclosingUnit(unit));
        ITextContainer *pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);

        // we don't want to move backwards at all in case we are at the start of document.
        if (count < 0)
        {
            IFC(m_pStartTextPointer->GetPlainTextPosition().GetOffset(&endOffset));
            IFC(documentRangeAdapter->m_pStartTextPointer->GetPlainTextPosition().GetOffset(&startOffset));
            ASSERT(startOffset <= endOffset);
            IFC(pTextContainer->GetText(startOffset, endOffset, TRUE, &cCharacters, &pCharacters, &isTakeOwnership));

            // handles reserved runs in begining. Start pointer is already at begining so no need to move here
            if (cCharacters == 0)
            {
                moveCount = 0;
            }
            else
            {
                IFC(MoveEndpointByUnit(UIAXcp::TextPatternRangeEndpoint_Start, unit, count, &moveCount));
            }
        }
        else
        {
            IFC(MoveEndpointByUnit(UIAXcp::TextPatternRangeEndpoint_Start, unit, count, &moveCount, &bIsEmptyLine));
        }

        // If we are in last word/line/paragraph (TextUnit) when we move to forward direction MoveEndpointByUnit will move StartPointer
        // to End which is valid for that API. but here we want to discard that move as it's not valid move for this API.
        // If we can't move to Next TextUnit as it doesn't exist we discard the whole operation.
        if (count > 0 && moveCount > 0)
        {
            IFC(m_pStartTextPointer->GetPlainTextPosition().GetOffset(&startOffset));
            IFC(documentRangeAdapter->m_pEndTextPointer->GetPlainTextPosition().GetOffset(&endOffset));
            IFC(pTextContainer->GetText(startOffset, endOffset, TRUE, &cCharacters, &pCharacters, &isTakeOwnership));
            if (cCharacters <= 2)
            {
                // in case of line, last line from Visible range perspective ends right before new line at end.
                if (cCharacters == 0 || IsXamlNewline(pCharacters[0]))
                {
                    // This means we were already on last unit and now reached degenerate range so just discard the move right away.
                    if (moveCount == 1)
                    {
                        // If we are here and  unit is Line and it's empty then we have actually moved a line which happens to be
                        // degenerate and in that case moveCount == 1 is correct and needn't be changed.
                        if (unit != UIAXcp::TextUnit_Line || !bIsEmptyLine)
                        {
                            moveCount = 0;
                        }
                    }
                    else
                    {
                        // in case move Count is more than 1, we have moved 1 too far hence reached degenerate range,
                        // we must move back one unit to remain on last unit in Document.
                        XINT32 adjustedMoveCount = 0;
                        // If we are moving by Line units and are on empty line (caused by LineBreaks, there's no need to go back as degenerate range is a valid range)
                        if (unit == UIAXcp::TextUnit_Line && bIsEmptyLine)
                        {
                            moveCount -= 1;
                        }
                        else
                        {
                            IFC(MoveEndpointByUnit(UIAXcp::TextPatternRangeEndpoint_Start, unit, -1, &adjustedMoveCount));
                            if (adjustedMoveCount != 0)
                            {
                                moveCount -= 1;
                            }
                        }
                    }

                }
            }
        }

        if (moveCount != 0)
        {
            IFC(ExpandToEnclosingUnit(unit));
        }
        else
        {
            ReplaceInterface(m_pStartTextPointer, clonedRange->m_pStartTextPointer);
            ReplaceInterface(m_pEndTextPointer, clonedRange->m_pEndTextPointer);
        }
    }

    *pReturnValue = moveCount;
Cleanup:
    if (isTakeOwnership)
    {
        delete [] pCharacters;
    }
    return hr;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
//   The endpoint is moved forward or backward, as specified, to the next available unit boundary. If the original endpoint was at the boundary
//   of the specified text unit, the endpoint is moved to the next available text unit boundary.
//   If the endpoint being moved crosses the other endpoint of the same text range, the other endpoint is also moved, resulting in a degenerate
//   range and ensuring the correct ordering of the endpoint (that is, that the start is always less than or equal to the end).
//------------------------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextRangeAdapter::MoveEndpointByUnit(
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ UIAXcp::TextUnit unit,
    _In_ XINT32 count,
    _Out_ XINT32* pReturnValue,
    _Out_opt_ bool* pbIsEmptyLine /*To be utilised only for Line Unit and by Move Method*/)
{
    XUINT32 moveCount = 0;
    bool doExecuteForNextUnit = false;
    xref_ptr<CTextPointerWrapper> contentStart;
    xref_ptr<CTextPointerWrapper> contentEnd;

    *pReturnValue = 0;

    switch(unit)
    {
    case UIAXcp::TextUnit_Character :
        IFC_RETURN(ExpandToCharacter(endPoint, this, count, &moveCount));
        break;
    case UIAXcp::TextUnit_Format :
        IFC_RETURN(ExpandToFormat(endPoint, this, count, &moveCount));
        break;
    case UIAXcp::TextUnit_Word :
        IFC_RETURN(ExpandToWord(endPoint, this, count, &moveCount));
        break;
    case UIAXcp::TextUnit_Line :
        IFC_RETURN(ExpandToLine(endPoint, this, count, &moveCount, pbIsEmptyLine));
        break;
    case UIAXcp::TextUnit_Paragraph :
        IFC_RETURN(ExpandToParagraph(endPoint, this, count, &moveCount, &doExecuteForNextUnit));
        if(!doExecuteForNextUnit)
        {
            break;
        }
    case UIAXcp::TextUnit_Page :
    case UIAXcp::TextUnit_Document :
        if(count !=0)
        {
            moveCount = 1;
            if(endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
            {
                if(count > 0)
                {
                    IFC_RETURN(ExpandToEnclosingUnit(unit));
                    IFC_RETURN(this->m_pEndTextPointer->Clone(m_pTextOwner, &(this->m_pStartTextPointer)));
                }
                if(count < 0)
                {
                    IFC_RETURN(CTextAdapter::GetContentEndPointers(m_pTextOwner, contentStart.ReleaseAndGetAddressOf(), contentEnd.ReleaseAndGetAddressOf()));
                    IFC_RETURN(contentStart->Clone(m_pTextOwner, &(this->m_pStartTextPointer)));
                }
            }
            else
            {
                if(count < 0)
                {
                    IFC_RETURN(this->m_pStartTextPointer->Clone(m_pTextOwner, &(this->m_pEndTextPointer)));
                    moveCount = 1;
                }
                if(count > 0)
                {
                    IFC_RETURN(CTextAdapter::GetContentEndPointers(m_pTextOwner, contentStart.ReleaseAndGetAddressOf(), contentEnd.ReleaseAndGetAddressOf()));
                    IFC_RETURN(contentEnd->Clone(m_pTextOwner, &(this->m_pEndTextPointer)));
                }
            }
        }
        break;
    }
    *pReturnValue = count < 0 ? 0- (XINT32)moveCount : (XINT32)moveCount;

    return S_OK;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
//   If the endpoint being moved crosses the other endpoint of the same text range, that other endpoint is moved also, resulting in a
//   degenerate (empty) range and ensuring the correct ordering of the endpoints (that is, the start is always less than or equal to the end).
//----------------------------------------------------------------------------------------------------------------------------------------------
_Check_return_ HRESULT CTextRangeAdapter::MoveEndpointByRange(_In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ CTextRangeAdapter* pTargetTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint)
{
    HRESULT hr = S_OK;
    CTextPointerWrapper *pTargetPointer = nullptr;
    XINT32 startOffset;
    XINT32 endOffset;

    if (m_pTextOwner == pTargetTextRangeProvider->m_pTextOwner)
    {
        pTargetPointer = UIAXcp::TextPatternRangeEndpoint_Start == targetEndPoint ? pTargetTextRangeProvider->m_pStartTextPointer : pTargetTextRangeProvider->m_pEndTextPointer;
        AddRefInterface(pTargetPointer);

        if (UIAXcp::TextPatternRangeEndpoint_Start == endPoint)
        {
            ReleaseInterface(m_pStartTextPointer);
            IFC(pTargetPointer->Clone(m_pTextOwner, &m_pStartTextPointer));

            IFC(m_pStartTextPointer->GetOffset(&startOffset));
            IFC(m_pEndTextPointer->GetOffset(&endOffset));
            if (startOffset > endOffset)
            {
                // it will be a de-generated range
                ReleaseInterface(m_pEndTextPointer);
                IFC(m_pStartTextPointer->Clone(m_pTextOwner, &m_pEndTextPointer));
            }
        }
        else
        {
            ReleaseInterface(m_pEndTextPointer);
            IFC(pTargetPointer->Clone(m_pTextOwner, &m_pEndTextPointer));

            IFC(m_pStartTextPointer->GetOffset(&startOffset));
            IFC(m_pEndTextPointer->GetOffset(&endOffset));
            if (startOffset > endOffset)
            {
                ReleaseInterface(m_pStartTextPointer);
                IFC(m_pEndTextPointer->Clone(m_pTextOwner, &m_pStartTextPointer));
            }
        }
    }
    else
    {
        IFC(E_INVALIDARG);
    }

Cleanup:
    ReleaseInterface(pTargetPointer);
    return hr;
}

_Check_return_ HRESULT CTextRangeAdapter::Select()
{
    TextSelectionManager* pTextSelectionManager = nullptr;

    pTextSelectionManager = CTextAdapter::GetSelectionManager(m_pTextOwner);
    if (pTextSelectionManager == nullptr)
    {
        IFC_RETURN(E_NOT_SUPPORTED);
    }

    if (m_pTextOwner->GetTypeIndex() == KnownTypeIndex::TextBlock)
    {
        CTextBlock* pTbl = do_pointer_cast<CTextBlock>(m_pTextOwner);
        IFC_RETURN(pTbl->Select(m_pStartTextPointer, m_pEndTextPointer));
    }
    else if (m_pTextOwner->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
    {
        CRichTextBlock* pRTbl = do_pointer_cast<CRichTextBlock>(m_pTextOwner);
        IFC_RETURN(pRTbl->Select(m_pStartTextPointer, m_pEndTextPointer));
    }
    else if (m_pTextOwner->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
    {
        CRichTextBlockOverflow* pRTblo = do_pointer_cast<CRichTextBlockOverflow>(m_pTextOwner);
        CRichTextBlock* pRtbl = pRTblo->GetMaster();
        IFC_RETURN(pRtbl->Select(m_pStartTextPointer, m_pEndTextPointer));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::AddToSelection()
{
    return E_NOT_SUPPORTED;
}

_Check_return_ HRESULT CTextRangeAdapter::RemoveFromSelection()
{
    return E_NOT_SUPPORTED;
}

_Check_return_ HRESULT CTextRangeAdapter::ScrollIntoView()
{
    HRESULT hr = S_OK;
    ITextView* pTextView = nullptr;
    XINT32 startOffset = 0;
    XINT32 endOffset = 0;
    XUINT32 cRectangles = 0;
    XRECTF *pRectangles = nullptr;
    XRECTF finalRect{};

    IFC(m_pStartTextPointer->GetOffset(&startOffset));
    IFC(m_pEndTextPointer->GetOffset(&endOffset));
    pTextView = CTextAdapter::GetTextView(m_pTextOwner);

    if (pTextView)
    {
        IFC(pTextView->TextRangeToTextBounds(startOffset, endOffset, &cRectangles, &pRectangles));
        if (cRectangles > 0)
        {
            finalRect.X = pRectangles[0].X;
            finalRect.Y = pRectangles[0].Y;
            finalRect.Height  = pRectangles[0].Y + pRectangles[cRectangles-1].Y + pRectangles[cRectangles-1].Height;
            finalRect.Width = pRectangles[0].Width;
        }

        static_cast<CUIElement*>(m_pTextOwner)->BringIntoView(finalRect, true /*forceIntoView*/, true /*useAnimation*/, true /*skipDuringManipulation*/);
    }

Cleanup:
    delete [] pRectangles;
    return hr;
}

_Check_return_ HRESULT CTextRangeAdapter::Normalize(_In_ CTextRangeAdapter* pTextRangeAdapter)
{
    IFCPTR_RETURN(pTextRangeAdapter);

    ReleaseInterface(pTextRangeAdapter->m_pEndTextPointer);
    IFC_RETURN((pTextRangeAdapter->m_pStartTextPointer)->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pEndTextPointer)));

    return S_OK;
}

//  Summary:
//      validation follows rules to make sure EndPoint is within DocumentRange and start pointer is always less than equal to end pointer for all different cases possible.
//
//  Parameters
//      endPoint                    --  EndPoint which we are looking to adjust for the Range. It could either be Start EndPoint or End EndPoint
//      nextPosition                --  The new position determined for the mentioned endPoint.
//      pTextRangeAdapter           --  The corresponding TextRange whose endPoint is getting updated.
//      pFoundEmptyAdjustedRange    --  Boolean value determining if the new TextPosition corresponds to empty text, if It is empty
//                                      then caller must look for next position in the range for the given TextUnit.
//
_Check_return_ HRESULT CTextRangeAdapter::ValidateAndAdjust(
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ const CPlainTextPosition& nextPosition,
    _In_ CTextRangeAdapter* pTextRangeAdapter,
    _Out_opt_ bool* pFoundEmptyAdjustedRange)
{
    HRESULT hr = S_OK;
    xref_ptr<CTextRangeAdapter> documentRangeAdapter;
    XUINT32  cCharacters = 0;
    const WCHAR *pCharacters = nullptr;
    XUINT32 contentStartOffset;
    XUINT32 contentEndOffset;
    XUINT32 startOffset;
    XUINT32 endOffset;
    XUINT32 oldOffset;
    XUINT32 newOffset;
    bool isEndPosition = true;
    bool isTakeOwnership = false;

    IFCPTR(pTextRangeAdapter->m_pTextAdapter);
    IFC((pTextRangeAdapter->m_pTextAdapter)->GetDocumentRange(documentRangeAdapter.ReleaseAndGetAddressOf()));
    IFCPTR(documentRangeAdapter);

    IFC((documentRangeAdapter->m_pStartTextPointer->GetPlainTextPosition()).GetOffset(&contentStartOffset));
    IFC((documentRangeAdapter->m_pEndTextPointer->GetPlainTextPosition()).GetOffset(&contentEndOffset));

    if (endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
    {
        IFC((pTextRangeAdapter->m_pStartTextPointer->GetPlainTextPosition()).GetOffset(&oldOffset));
        ReleaseInterface(pTextRangeAdapter->m_pStartTextPointer);
        IFC((pTextRangeAdapter->m_pEndTextPointer->GetPlainTextPosition()).GetOffset(&endOffset));
        IFC(nextPosition.GetOffset(&startOffset));

        // if Start is more than End, End will need to be brought to same position as Start.
        if (startOffset > endOffset)
        {
            // If start goes beyond Document Range's end that becomes it's cut-off point.
            if (startOffset >= contentEndOffset)
            {
                IFC(documentRangeAdapter->m_pEndTextPointer->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pStartTextPointer)));
                newOffset = contentEndOffset;
            }
            else
            {
                IFC(CTextPointerWrapper::Create(pTextRangeAdapter->GetContext(), nextPosition, &(pTextRangeAdapter->m_pStartTextPointer)));
                isEndPosition = FALSE;
                newOffset = startOffset;

            }
            ReleaseInterface(pTextRangeAdapter->m_pEndTextPointer);
            IFC(pTextRangeAdapter->m_pStartTextPointer->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pEndTextPointer)));
        }
        else
        {
            // If start goes beyond Document Range's start that becomes it's cut-off point.
            if (startOffset <= contentStartOffset)
            {
                IFC(documentRangeAdapter->m_pStartTextPointer->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pStartTextPointer)));
                newOffset = contentStartOffset;
            }
            else
            {
                IFC(CTextPointerWrapper::Create(pTextRangeAdapter->GetContext(), nextPosition, &(pTextRangeAdapter->m_pStartTextPointer)));
                isEndPosition = FALSE;
                newOffset = startOffset;
            }
        }
    }
    else
    {
        IFC((pTextRangeAdapter->m_pEndTextPointer->GetPlainTextPosition()).GetOffset(&oldOffset));
        ReleaseInterface(pTextRangeAdapter->m_pEndTextPointer);
        IFC((pTextRangeAdapter->m_pStartTextPointer->GetPlainTextPosition()).GetOffset(&startOffset));
        IFC(nextPosition.GetOffset(&endOffset));

        // if End is less than Start, Start will need to be brought to same position as End.
        if (startOffset > endOffset)
        {
            // If End goes beyond Document Range's Start that becomes it's cut-off point.
            if (endOffset <= contentStartOffset)
            {
                IFC(documentRangeAdapter->m_pStartTextPointer->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pEndTextPointer)));
                newOffset = contentStartOffset;
            }
            else
            {
                IFC(CTextPointerWrapper::Create(pTextRangeAdapter->GetContext(), nextPosition, &(pTextRangeAdapter->m_pEndTextPointer)));
                isEndPosition = FALSE;
                newOffset = endOffset;
            }
            ReleaseInterface(pTextRangeAdapter->m_pStartTextPointer);
            IFC(pTextRangeAdapter->m_pEndTextPointer->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pStartTextPointer)));
        }
        else
        {
            // If End goes beyond Document Range's End that becomes it's cut-off point.
            if (endOffset >= contentEndOffset)
            {
                IFC(documentRangeAdapter->m_pEndTextPointer->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pEndTextPointer)));
                newOffset = contentEndOffset;
            }
            else
            {
                IFC(CTextPointerWrapper::Create(pTextRangeAdapter->GetContext(), nextPosition, &(pTextRangeAdapter->m_pEndTextPointer)));
                isEndPosition = FALSE;
                newOffset = endOffset;
            }
        }
    }
    if (pFoundEmptyAdjustedRange != nullptr)
    {
        *pFoundEmptyAdjustedRange = FALSE;
        if ((newOffset - oldOffset != 0) && !isEndPosition)
        {
            startOffset = newOffset > oldOffset ? oldOffset : newOffset;
            endOffset = newOffset + oldOffset - startOffset;

            ITextContainer *pTextContainer = CTextAdapter::GetTextContainer(pTextRangeAdapter->m_pTextOwner);
            if (pTextContainer)
            {
                IFC(pTextContainer->GetText(startOffset, endOffset, TRUE, &cCharacters, &pCharacters, &isTakeOwnership));
                if (cCharacters == 0)
                {
                    *pFoundEmptyAdjustedRange = TRUE;
                }
            }
        }
    }

Cleanup:
    if (isTakeOwnership)
    {
        delete [] pCharacters;
    }
    return hr;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToCharacter(
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ CTextRangeAdapter* pTextRangeAdapter,
    _In_ XINT32 count,
    _Out_ XUINT32* pnCount)
{
    CPlainTextPosition plainTextPosition;
    XUINT32 moveCount = 0;
    bool foundEmptyAdjustedRange = false;

    *pnCount = 0;
    if (count != 0)
    {
        if (endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
        {
            plainTextPosition = pTextRangeAdapter->m_pStartTextPointer->GetPlainTextPosition();
        }
        else
        {
            plainTextPosition = pTextRangeAdapter->m_pEndTextPointer->GetPlainTextPosition();
        }
        IFC_RETURN(CTextRangeAdapter::MoveByCharacter(count, &moveCount, &plainTextPosition));
        IFC_RETURN(ValidateAndAdjust(endPoint, plainTextPosition, pTextRangeAdapter, &foundEmptyAdjustedRange));
        if (foundEmptyAdjustedRange)
        {
            IFC_RETURN(ExpandToCharacter(endPoint, pTextRangeAdapter, count, pnCount));
        }
    }
    *pnCount = moveCount;

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::MoveByCharacter(
    _In_ int count,
    _Out_ uint32_t* movedCount,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    CPlainTextPosition nextPosition = *plainTextPosition;
    uint32_t posCount = count >= 0 ? count : 0-count;
    uint32_t startOffset;
    uint32_t endOffset;
    uint32_t moveCount = 0;
    bool foundPos = true;

    for (uint32_t i = 0; (i < posCount) && foundPos; i++)
    {
        if (count >= 0)
        {
            IFCFAILFAST(nextPosition.GetOffset(&startOffset));
            IFCFAILFAST(nextPosition.GetNextInsertionPosition(&foundPos, &nextPosition));
            IFCFAILFAST(nextPosition.GetOffset(&endOffset));
        }
        else
        {
            IFCFAILFAST(nextPosition.GetOffset(&endOffset));
            IFCFAILFAST(nextPosition.GetPreviousInsertionPosition(&foundPos, &nextPosition));
            IFCFAILFAST(nextPosition.GetOffset(&startOffset));
        }

        if (foundPos)
        {
            uint32_t  cCharacters = 0;
            const WCHAR *pCharacters = nullptr;
            bool isTakeOwnership = false;
            ITextContainer *pTextContainer = plainTextPosition->GetTextContainer();

            if (pTextContainer)
            {
                IFCFAILFAST(pTextContainer->GetText(startOffset, endOffset, TRUE, &cCharacters, &pCharacters, &isTakeOwnership));
                // if it's an empty position, skip it.
                if (cCharacters == 0)
                {
                    // We are skipping this move but we still want to move Same number of real units.
                    posCount +=1;
                }
                else
                {
                    moveCount++;

                    // only update the position if we actually moved over a non-empty position
                    *plainTextPosition = nextPosition;
                }
            }

            if (isTakeOwnership)
            {
                delete [] pCharacters;
                pCharacters = nullptr;
                isTakeOwnership = FALSE;
            }
        }
    }
    *movedCount = moveCount;
    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToFormat(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount)
{
    ITextContainer *pTextContainer = nullptr;
    CTextElement *pContainingElement = nullptr;
    CTextElement *pAfterBoundaryElement = nullptr;
    CTextElement *pBeforeBoundaryElement = nullptr;
    xref_ptr<CTextPointerWrapper> textPointerWrapperFinal = nullptr;
    const InheritedProperties* pInheritedProperties = nullptr;
    TextNestingType textNestingType;
    const TextFormatting *pFormatting = nullptr;
    const WCHAR* pCharacters = nullptr;
    XUINT32 cCharacters = 0;
    XUINT32 posCount = XcpAbs(count);
    XUINT32 moveCount = 0;
    XUINT32 textPosition = 0;
    bool foundEmptyAdjustedRange = false;
    XUINT32 positionCount = 0;
    XINT32 direction = 1;
    xref_ptr<CTextRangeAdapter> documentRangeAdapter = nullptr;

    *pnCount = 0;
    // Retrieve the textPosition corresponding to referred edge.
    if (count != 0)
    {
        if (endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
        {
            IFC_RETURN(pTextRangeAdapter->m_pStartTextPointer->GetPlainTextPosition().GetOffset(&textPosition));
        }
        else
        {
            IFC_RETURN(pTextRangeAdapter->m_pEndTextPointer->GetPlainTextPosition().GetOffset(&textPosition));
        }

        pTextContainer = CTextAdapter::GetTextContainer(pTextRangeAdapter->m_pTextOwner);
        pTextContainer->GetPositionCount(&positionCount);
        // eliminate reserved runs or LineBreaks here at start or end of the range depending upon the direction.
        // As textPosition and character position are slightly different concepts which are often merged in these APIs
        // there is a posibility that textposition could be more than length of TextContainer (basically due to reserved runs)
        // and we have to make sure it never happens by adjusting the textposition accordingly.
        if (count > 0)
        {
            do
            {
                textPosition += cCharacters;
                if (textPosition >= positionCount)
                {
                    return S_OK;
                }
                IFC_RETURN(pTextContainer->GetRun(textPosition, &pFormatting, &pInheritedProperties, &textNestingType, &pContainingElement,  &pCharacters, &cCharacters));
            } while ((pCharacters == nullptr && cCharacters > 0) || (pContainingElement && pContainingElement->GetTypeIndex() == KnownTypeIndex::LineBreak));
        }
        else
        {
            direction = -1;
            do
            {
                if (textPosition == 0 && cCharacters > 0)
                {
                    return S_OK;
                }
                textPosition -= cCharacters;
                if (textPosition >= positionCount)
                {
                    textPosition =  positionCount-1;
                }
                IFC_RETURN(pTextContainer->GetRun(textPosition, &pFormatting, &pInheritedProperties, &textNestingType, &pContainingElement,  &pCharacters, &cCharacters));
            } while ((pCharacters == nullptr && cCharacters > 0) || (pContainingElement && pContainingElement->GetTypeIndex() == KnownTypeIndex::LineBreak));
        }

        pAfterBoundaryElement = pContainingElement;
        for (moveCount = 0; moveCount < posCount && pAfterBoundaryElement != nullptr; moveCount++)
        {
            pContainingElement = pAfterBoundaryElement;

            // traverse the tree in given direction (+ve forward, -ve backwards) to find the next element where format boundary exist.
            IFC_RETURN(pTextRangeAdapter->TraverseTextElementTreeForFormat(pContainingElement, direction, &pAfterBoundaryElement, &pBeforeBoundaryElement));
        }

        // Retrieve the pointer corresponding to the found Element depending upon the direction, if element is nullptr
        // that means we reached begining/end of the document and we should utilize DocumentRange's start/end pointers accordingly.
        if (direction > 0)
        {
            // when moving fwd as we are using element before boundary for last eleemnt we don't need to use DocumentRange's end pointer.
            // as that mixes total range with some reserved range at end and hence results in Mixed attributes.
            IFC_RETURN(pBeforeBoundaryElement->GetContentEnd(textPointerWrapperFinal.ReleaseAndGetAddressOf()));
        }
        else
        {
            if (pAfterBoundaryElement != nullptr)
            {
                IFC_RETURN(pAfterBoundaryElement->GetContentEnd(textPointerWrapperFinal.ReleaseAndGetAddressOf()));
            }

            if (pAfterBoundaryElement == nullptr || textPointerWrapperFinal == nullptr)
            {
                ASSERT(pTextRangeAdapter->m_pTextAdapter != nullptr);
                IFC_RETURN((pTextRangeAdapter->m_pTextAdapter)->GetDocumentRange(documentRangeAdapter.ReleaseAndGetAddressOf()));
                textPointerWrapperFinal = documentRangeAdapter->m_pStartTextPointer;
            }
        }

        // validate the final position and update.
        IFC_RETURN(ValidateAndAdjust(endPoint, textPointerWrapperFinal->GetPlainTextPosition(), pTextRangeAdapter, &foundEmptyAdjustedRange));
        *pnCount = moveCount;
    }
    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToWord(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount)
{
    CPlainTextPosition plainTextPosition;
    XUINT32 moveCount = 0;
    bool foundEmptyAdjustedRange = false;

    *pnCount = 0;
    if (count != 0)
    {
        if (endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
        {
            plainTextPosition = pTextRangeAdapter->m_pStartTextPointer->GetPlainTextPosition();
        }
        else
        {
            plainTextPosition = pTextRangeAdapter->m_pEndTextPointer->GetPlainTextPosition();
        }
        IFC_RETURN(CTextRangeAdapter::MoveByWord(count, &moveCount, &plainTextPosition));
        IFC_RETURN(ValidateAndAdjust(endPoint, plainTextPosition, pTextRangeAdapter, &foundEmptyAdjustedRange));
        if (foundEmptyAdjustedRange)
        {
            IFC_RETURN(ExpandToWord(endPoint, pTextRangeAdapter, count, pnCount));
        }
    }
    *pnCount = moveCount;

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::MoveByWord(
    _In_ int count,
    _Out_ uint32_t* movedCount,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    uint32_t posCount = count >= 0 ? count : 0-count;
    TextGravity eHitGravity = LineForwardCharacterForward;
    CTextPosition nextPosition;
    CTextPosition curPosition = CTextPosition(*plainTextPosition);
    uint32_t moveCount = 0;

    for (uint32_t i = 0; i < posCount; i++)
    {
        if (count > 0)
        {
            IFC_RETURN(CTextBoxHelpers::GetAdjacentWordNavigationBoundaryPosition(
                plainTextPosition->GetTextContainer(),
                curPosition,
                FindBoundaryType::ForwardIncludeTrailingWhitespace,
                TagConversion::Default,
                &nextPosition,
                &eHitGravity));
        }
        else
        {
            IFC_RETURN(CTextBoxHelpers::GetAdjacentWordNavigationBoundaryPosition(
                plainTextPosition->GetTextContainer(),
                curPosition,
                FindBoundaryType::Backward,
                TagConversion::Default,
                &nextPosition,
                &eHitGravity));
        }
        if (curPosition != nextPosition)
        {
            curPosition = nextPosition;
            moveCount++;
        }
        else
        {
            break;
        }
    }
    *movedCount = moveCount;
    *plainTextPosition = curPosition.GetPlainPosition();
    return S_OK;
}

// pbIsEmptyLine is to be used only from Move methods, we could use a separate function to determine if we are on empty line, though
// that operation is costly as it requires to retreive visible ranges and we actually can get that info right here.
_Check_return_ HRESULT CTextRangeAdapter::ExpandToLine(
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ CTextRangeAdapter* pTextRangeAdapter,
    _In_ XINT32 count,
    _Out_ XUINT32* pnCount,
    _Out_opt_ bool* pbIsEmptyLine
    )
{
    HRESULT hr  = S_OK;
    CTextRangeAdapter** pVisibleRangesForAdapter = nullptr;
    XINT32 startoffset = 0;
    XINT32 endoffset = 0;
    XINT32 offset = 0;
    CPlainTextPosition textPosition;
    XINT32 nLines = 0;
    XINT32 lineIndex = -1;
    XINT32 newLineIndex = -1;
    XINT32 lineBreakerAdjuster = 0;
    XINT32 deltaOffset = 0;

    IFCPTR(pTextRangeAdapter);
    IFCPTR(pnCount);
    *pnCount = 0;
    if (pbIsEmptyLine)
    {
        *pbIsEmptyLine = FALSE;
    }

    if (count != 0)
    {
        IFCPTR(pTextRangeAdapter->m_pTextAdapter);
        if (endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
        {
            IFC(pTextRangeAdapter->m_pStartTextPointer->GetOffset(&offset));
        }
        else
        {
            IFC(pTextRangeAdapter->m_pEndTextPointer->GetOffset(&offset));
        }

        IFC((pTextRangeAdapter->m_pTextAdapter)->GetVisibleRanges(&pVisibleRangesForAdapter, &nLines));
        for (lineIndex = 0; lineIndex < nLines; lineIndex++)
        {
            IFC(pVisibleRangesForAdapter[lineIndex]->m_pStartTextPointer->GetOffset(&startoffset));
            deltaOffset = startoffset - endoffset;
            IFC(pVisibleRangesForAdapter[lineIndex]->m_pEndTextPointer->GetOffset(&endoffset));
            // this handles the break between two consecutive caused by reserved runs
            // handles empty line caused by Line breaks
            if (offset >= startoffset - deltaOffset && ((offset < endoffset) || (offset == startoffset)))
            {
                break;
            }
        }
        if (count > 0)
        {
            // If we are outside of line bounds or are at last line which is empty, we can't move forward.
            if ((lineIndex >= nLines) || (lineIndex == nLines-1 && startoffset == endoffset))
            {
                if (pbIsEmptyLine && startoffset == endoffset)
                {
                    *pbIsEmptyLine = TRUE;
                }
                *pnCount = 0;
                goto Cleanup;
            }
            newLineIndex = lineIndex + count;
            if (newLineIndex < nLines)
            {
                // newLineIndex will always point to the next line hence use the Start pointer.
                textPosition = pVisibleRangesForAdapter[newLineIndex]->m_pStartTextPointer->GetPlainTextPosition();
                IFC(pVisibleRangesForAdapter[newLineIndex]->m_pStartTextPointer->GetOffset(&startoffset));
                IFC(pVisibleRangesForAdapter[newLineIndex]->m_pEndTextPointer->GetOffset(&endoffset));
            }
            else
            {
                // In case we have exceeded the number of lines, we should be at end of last line
                textPosition = pVisibleRangesForAdapter[nLines-1]->m_pEndTextPointer->GetPlainTextPosition();
                IFC(pVisibleRangesForAdapter[nLines-1]->m_pStartTextPointer->GetOffset(&startoffset));
                IFC(pVisibleRangesForAdapter[nLines-1]->m_pEndTextPointer->GetOffset(&endoffset));

                // We sticking to current's End as Next start does not exist. So Hypothetical nextline is used to keep the move count right.
                newLineIndex = nLines;
            }
            if (startoffset == endoffset && pbIsEmptyLine)
            {
                 *pbIsEmptyLine = TRUE;
            }
            *pnCount = MAX(0, newLineIndex - lineIndex);
        }
        else
        {
            if(offset <= startoffset || offset >= endoffset)
            {
                lineBreakerAdjuster = -1;
            }
            if (lineIndex + lineBreakerAdjuster == -1)
            {
                *pnCount = 0;
                goto Cleanup;
            }
            newLineIndex = MIN(nLines - 1,MAX(0, lineIndex + count + 1 + lineBreakerAdjuster));
            textPosition = pVisibleRangesForAdapter[newLineIndex]->m_pStartTextPointer->GetPlainTextPosition();
            *pnCount = XcpAbs(newLineIndex - lineIndex);
        }
        IFC(ValidateAndAdjust(endPoint, textPosition, pTextRangeAdapter));
    }
Cleanup:
    for (XINT32 j = 0; j < nLines; j++)
    {
        ReleaseInterface(pVisibleRangesForAdapter[j]);
    }
    delete [] pVisibleRangesForAdapter;
    return hr;
}

_Check_return_ HRESULT CTextRangeAdapter::MoveToLine(
    _In_ bool isMovingToEnd,
    _In_ CTextRangeAdapter* textRangeAdapter,
    _Inout_ CPlainTextPosition* movingTextPosition
    )
{
    CTextRangeAdapter** visibleRangesForAdapter = nullptr;
    CPlainTextPosition textPosition;
    int offset = 0;
    uint32_t uintoffset = 0;
    int nLines = 0;
    int endoffset = 0;

    auto extraCleanup = wil::scope_exit([&]{
        for (int j = 0; j < nLines; j++)
        {
            ReleaseInterface(visibleRangesForAdapter[j]);
        }
        delete [] visibleRangesForAdapter;
    });

    IFCFAILFAST(movingTextPosition->GetOffset(&uintoffset));
    offset = uintoffset;
    IFCFAILFAST((textRangeAdapter->m_pTextAdapter)->GetVisibleRanges(&visibleRangesForAdapter, &nLines));

    for (int lineIndex = 0; lineIndex < nLines; lineIndex++)
    {
        int startoffset;
        int deltaOffset;
        IFCFAILFAST(visibleRangesForAdapter[lineIndex]->m_pStartTextPointer->GetOffset(&startoffset));
        deltaOffset = startoffset - endoffset;
        IFCFAILFAST(visibleRangesForAdapter[lineIndex]->m_pEndTextPointer->GetOffset(&endoffset));

        // If this matches the end, and we have more lines available, then we need to check if offset
        // should be considered the end of this line, or the beginning of the next
        if (offset == endoffset && lineIndex + 1 < nLines)
        {
            int nextLineStartOffset;
            IFCFAILFAST(visibleRangesForAdapter[lineIndex + 1]->m_pStartTextPointer->GetOffset(&nextLineStartOffset));
            if (offset == nextLineStartOffset)
            {
                // offset also matches the start of the next line, so now we need to determine which line to choose.
                // Use gravity/direction as the tie-breaker.
                TextGravity movingGravity;
                IFCFAILFAST(movingTextPosition->GetGravity(&movingGravity));

                uint32_t endDirection;
                IFCFAILFAST(visibleRangesForAdapter[lineIndex]->m_pEndTextPointer->GetLogicalDirection(&endDirection));
                if (movingGravity == LineForwardCharacterForward && endDirection == RichTextServices::LogicalDirection::Backward)
                {
                    // movingTextPosition is forward direction, but endOffset is backward direction, so
                    // this line does *not* contain movingTextPosition. Go to the next iteration of the loop
                    // to use the next line.
                    continue;
                }
                // else gravity/direction say we can use the current line
            }
            // else the next line is after offset, so let the code below go ahead and use the current line
        }
        if (offset >= startoffset - deltaOffset && offset <= endoffset || lineIndex == nLines - 1)   //within the range or last line
        {
            if (isMovingToEnd)
            {
                textPosition = visibleRangesForAdapter[lineIndex]->m_pEndTextPointer->GetPlainTextPosition();
            }
            else
            {
                textPosition = visibleRangesForAdapter[lineIndex]->m_pStartTextPointer->GetPlainTextPosition();
            }
            break;
        }
    }
    *movingTextPosition = textPosition;
    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToParagraph(
    _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
    _In_ CTextRangeAdapter* pTextRangeAdapter,
    _In_ XINT32 count,
    _Out_ XUINT32* pnCount,
    _Out_ bool* doExecuteForNextUnit)
{
    CBlockCollection* pBlockCollection = nullptr;
    xref_ptr<CTextPointerWrapper> startPointerWrapper;
    xref_ptr<CTextPointerWrapper> endPointerWrapper;
    CParagraph *pParagraphNoRef = nullptr;
    CPlainTextPosition textPosition;
    xref_ptr<CDependencyObject> block = nullptr;
    XINT32 startoffset = 0;
    XINT32 endoffset = 0;
    XINT32 offset;
    XINT32 paraIndex = -1;
    XINT32 newParaIndex = -1;
    XINT32 paraBreakerAdjuster = 0;
    XINT32 deltaOffset = 0;

    IFCPTR_RETURN(pTextRangeAdapter);
    IFCPTR_RETURN(pnCount);
    IFCPTR_RETURN(doExecuteForNextUnit);
    *pnCount = 0;
    *doExecuteForNextUnit = FALSE;
    if (count != 0)
    {
        IFCPTR_RETURN(pTextRangeAdapter->m_pTextAdapter);
        if (endPoint == UIAXcp::TextPatternRangeEndpoint_Start)
        {
            IFC_RETURN(pTextRangeAdapter->m_pStartTextPointer->GetOffset(&offset));
        }
        else
        {
            IFC_RETURN(pTextRangeAdapter->m_pEndTextPointer->GetOffset(&offset));
        }
        pBlockCollection = CTextAdapter::GetBlockCollection(pTextRangeAdapter->m_pTextOwner);
        if (pBlockCollection)
        {
            XINT32 numBlocks = pBlockCollection->GetCount();
            for (paraIndex = 0; paraIndex < numBlocks; paraIndex++)
            {
                block = static_cast<CDependencyObject*>(pBlockCollection->GetItemWithAddRef(paraIndex));
                pParagraphNoRef = static_cast<CParagraph*>(block.get());
                if (pParagraphNoRef)
                {
                    IFC_RETURN(pParagraphNoRef->GetContentStart(startPointerWrapper.ReleaseAndGetAddressOf()));
                    IFC_RETURN(pParagraphNoRef->GetContentEnd(endPointerWrapper.ReleaseAndGetAddressOf()));
                    IFC_RETURN(startPointerWrapper->GetOffset(&startoffset));
                    deltaOffset = startoffset - endoffset;
                    IFC_RETURN(endPointerWrapper->GetOffset(&endoffset));
                    // this handles the break between two consecutive lines where there is deltaoffset difference
                    // between and and new lines start generally introduced by reserved runs
                    if (offset >= startoffset - deltaOffset && offset < endoffset)
                    {
                        break;
                    }
                }
            }
            if (count >= 0)
            {
                if (paraIndex == numBlocks)
                {
                    *pnCount = 0;
                    return S_OK;
                }
                newParaIndex = MIN(numBlocks - 1, paraIndex + count - 1);
                if(newParaIndex >= 0)
                {
                    block = static_cast<CDependencyObject*>(pBlockCollection->GetItemWithAddRef(newParaIndex));
                    pParagraphNoRef = static_cast<CParagraph*>(block.get());
                    if (pParagraphNoRef)
                    {
                        IFC_RETURN(pParagraphNoRef->GetContentEnd(endPointerWrapper.ReleaseAndGetAddressOf()));
                        textPosition = endPointerWrapper->GetPlainTextPosition();
                    }
                }
                *pnCount = XcpAbs(newParaIndex - paraIndex) + 1;
            }
            else
            {
                if (offset <= startoffset || offset >= endoffset)
                {
                    paraBreakerAdjuster = -1;
                }
                if (paraIndex + paraBreakerAdjuster == -1)
                {
                    *pnCount = 0;
                    return S_OK;
                }
                newParaIndex = MAX(0, paraIndex + count + 1 + paraBreakerAdjuster);
                if (newParaIndex < numBlocks)
                {
                    block = static_cast<CDependencyObject*>(pBlockCollection->GetItemWithAddRef(newParaIndex));
                    pParagraphNoRef = static_cast<CParagraph*>(block.get());
                    if (pParagraphNoRef)
                    {
                        IFC_RETURN(pParagraphNoRef->GetContentStart(startPointerWrapper.ReleaseAndGetAddressOf()));
                        textPosition = startPointerWrapper->GetPlainTextPosition();
                    }
                }
                *pnCount = XcpAbs(newParaIndex - paraIndex);
            }
            IFC_RETURN(ValidateAndAdjust(endPoint, textPosition, pTextRangeAdapter));
        }
        else
        {
            *doExecuteForNextUnit = TRUE;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToPage(_In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count)
{
    // In XAML TextOM Document range is same as Page range.
    IFC_RETURN(ExpandToDocument(pTextRangeAdapter, count));

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::ExpandToDocument(_In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count)
{
    xref_ptr<CTextRangeAdapter> documentRangeAdapter;

    IFCPTR_RETURN(pTextRangeAdapter->m_pTextAdapter);
    IFC_RETURN((pTextRangeAdapter->m_pTextAdapter)->GetDocumentRange(documentRangeAdapter.ReleaseAndGetAddressOf()));
    ReleaseInterface(pTextRangeAdapter->m_pStartTextPointer);
    ReleaseInterface(pTextRangeAdapter->m_pEndTextPointer);
    IFC_RETURN((documentRangeAdapter->m_pStartTextPointer)->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pStartTextPointer)));
    IFC_RETURN((documentRangeAdapter->m_pEndTextPointer)->Clone(pTextRangeAdapter->m_pTextOwner, &(pTextRangeAdapter->m_pEndTextPointer)));

    return S_OK;
}

// This method traverses the n-ary TextElement tree in both forward/backward direction defined by "direction". The tree walk
// is essentially LefttoRight or RightToLeft while looking for FormatBreaker at node boundaries to break.
// The algorithm essentially have three blocks in it.
//      Block 1:
//          Finds out the NextElement when the current element is a collection.
//      Block 2:
//          In case current element is a leaf node, we fetch the parent/collection and look for index of the
//          next element in given direction and put parent on top of stack with that indices to be proccessed in Block 1.
//      Block 3:
//          Check the node for format breaker if it's a format breaker we break out of the loop otherwise we push the element
//          on top of stack to be processed.
_Check_return_ HRESULT CTextRangeAdapter::TraverseTextElementTreeForFormat(_In_ CTextElement *pTextElementStart, _In_ XINT32 direction, _Outptr_ CTextElement** pAfterBoundaryElement, _Outptr_ CTextElement** pBeforeBoundaryElement)
{
    CTextElementCollection* pCollection = nullptr;
    CTextElement* pCurrentElement = nullptr;
    CTextElement* pPreviousElement = pTextElementStart;
    CTextElement* pNextElement = nullptr;
    CTextElement* pParentElement = nullptr;
    TextElementNodeInfo node;
    xstack<TextElementNodeInfo> elementStack; // stack to keep track of latest node we are working on.
    bool isFormatBoundary = false;
    bool bisHiddenRunOrLineBreak = false;
    XINT32 index = 0;

    *pAfterBoundaryElement = nullptr;
    *pBeforeBoundaryElement = nullptr;

    // Ensure different property values for the start element to be compared against.
    IFC_RETURN(EnsureAttributeIdInfoForStartElement(pTextElementStart));

    // push the first element in stack with child index as 0, child index will always be 0 for leaf node.
    // first element will always be leaf node.
    IFC_RETURN(elementStack.push(TextElementNodeInfo(pTextElementStart,0)));
    while (!elementStack.empty() && !isFormatBoundary)
    {
        // initialize NextElement to nullptr for every new iteration.
        pNextElement = nullptr;
        IFC_RETURN(elementStack.top(node));
        pCurrentElement = node.pNode;
        // if node exist but has nullptr Element, that is a indication it represents the Text Control itself and
        // collection would correspond to TextBlock's InlineCollection or BlockCollection of Rich Text Controls.
        if (pCurrentElement == nullptr)
        {
            if (m_pTextOwner->GetTypeIndex() == KnownTypeIndex::TextBlock)
            {
                pCollection = static_cast<CTextBlock*>(m_pTextOwner)->GetInlineCollection();
            }
            else
            {
                pCollection = CTextAdapter::GetBlockCollection(m_pTextOwner);
            }
        }
        else
        {
            pCollection = pCurrentElement->GetInlineCollection();
        }
        index = node.currentChildindex;
        // Block 1
        if (pCollection != nullptr)
        {
            if (index >= 0)
            {
                pNextElement = do_pointer_cast<CTextElement>(pCollection->GetItemImpl(index));

                if (pNextElement)
                {
                    pCollection = pNextElement->GetInlineCollection();
                    // Non-leaf node need not be compared for Format boundary.
                    if (pCollection != nullptr)
                    {
                        if (direction > 0)
                        {
                            // In moving forward for non-leaf node we want it's first child to be pushed on
                            // top of stack to be processed
                            index = 0;
                        }
                        else
                        {
                            // In moving backwards for non-leaf node we want it's last child to be pushed on
                            // top of stack to be processed
                            index = pCollection->GetCount() - 1;
                        }
                        IFC_RETURN(elementStack.push(TextElementNodeInfo(pNextElement,index)));
                        continue;
                    }
                }

                // Verify if we need HiddenRun check here
                if (pNextElement != nullptr)
                {
                    //check for Hidden Run, LineBreak here
                    IFC_RETURN(IsHiddenRunOrLineBreak(pNextElement, &bisHiddenRunOrLineBreak));
                    if (bisHiddenRunOrLineBreak)
                    {
                        IFC_RETURN(elementStack.push(TextElementNodeInfo(pNextElement,0)));
                        continue;
                    }
                }
            }
        }

        // Block 2
        if (pNextElement == nullptr)
        {
            IFC_RETURN(elementStack.pop());
            if (pCurrentElement != nullptr)
            {
                pParentElement = nullptr;
                pCollection = do_pointer_cast<CTextElementCollection>(pCurrentElement->GetParent());
                if (pCollection != nullptr)
                {
                    pParentElement = do_pointer_cast<CTextElement>(pCollection->GetInheritanceParentInternal());
                }
                IFC_RETURN(pCollection->IndexOf(static_cast<CDependencyObject*>(pCurrentElement), &index));

                // Push the parent on top of stack with next element's index, this node will get processed in Block 1 to find out NextElement to go in Block 3
                if (elementStack.empty())
                {
                    IFC_RETURN(elementStack.push(TextElementNodeInfo(pParentElement,index+(1*direction))));
                }
                else
                {
                    IFC_RETURN(elementStack.top(node));
                    // if parentElement of collection is nullptr here, then this is top level block collection containing paragraphs
                    // and it's parent would be the m_pTextOwner itself, RTBl/RTBlo
                    if (node.pNode == pParentElement)
                    {
                        IFC_RETURN(elementStack.pop());
                        node.currentChildindex = index + (1*direction);
                        IFC_RETURN(elementStack.push(node));
                    }
                    else
                    {
                        IFC_RETURN(elementStack.push(TextElementNodeInfo(pParentElement,index+(1*direction))));
                    }
                }
            }
            continue;
        }

        // Block 3
        IFC_RETURN(IsFormatBreaker(pNextElement, &isFormatBoundary));
        if (!isFormatBoundary)
        {
            // This is always a Leaf-Node.
            pPreviousElement = pNextElement;
            IFC_RETURN(elementStack.push(TextElementNodeInfo(pNextElement,0)));
        }
    }

    *pAfterBoundaryElement = pNextElement;
    *pBeforeBoundaryElement = pPreviousElement;

    return S_OK;
}

// This method comapres two TextElements for different supported TextAttributes. if it finds difference in values
// it identifies that as a format breaker and returns TRUE.
_Check_return_ HRESULT CTextRangeAdapter::IsFormatBreaker(_In_ CTextElement *pContainingElementNext, _Out_ bool* returnValue)
{
    ITextContainer *pTextContainer = nullptr;
    CParagraph* pParagraph = nullptr;
    CValue nextElementResult;
    bool comparer = true;
    XINT32 size = ARRAY_SIZE(AttributeIdInfo);

    IFCPTR_RETURN(returnValue);
    ASSERT(pContainingElementNext != nullptr);
    *returnValue = FALSE;
    pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);
    pParagraph = CTextRangeAdapter::GetParagraphFromTextElement(pContainingElementNext);

    // Every InlineUIContainer is to be treated as format breaker.
    if (pContainingElementNext->GetTypeIndex() == KnownTypeIndex::InlineUIContainer)
    {
        // just use this boolean as it is used to set the return value.
        comparer = FALSE;
    }
    else
    {
        // go through all the supported attributes and check if we have a Format boundary.
        for (int i=0; i < size; i++)
        {
            IFC_RETURN(GetAttributeValueFromTextElement(AttributeIdInfo[i], pContainingElementNext, &nextElementResult, pParagraph));
            IFC_RETURN(AttributeValueComparer(m_attributeValueInfoTable[i], nextElementResult, &comparer));

            if (comparer == FALSE)
            {
                break;
            }
        }
    }

    *returnValue = !comparer;

    return S_OK;
}

// Retrieves the Attribute value corresponding to the attributeID for a given TextElement.
_Check_return_ HRESULT CTextRangeAdapter::GetAttributeValueFromTextElement(
    _In_ DirectUI::AutomationTextAttributesEnum attributeID,
    _In_ CTextElement* pContainingElement, 
    _Out_ CValue* pRetValue, 
    _In_opt_ CParagraph* pParagraph)
{
    CValue returnValue;
    CValue tempValue;
    CValue tempValue2;
    XUINT32 tempValueInt;
    XTHICKNESS* thickNess = nullptr;
    XTHICKNESS* paraThickNess = nullptr;

    pRetValue->SetNull();

    if (pContainingElement != nullptr)
    {
        // some properties are present at TextElement level but some other are determined by Containing Paragraph or the Text Control itself.
        switch(attributeID)
        {
        case DirectUI::AutomationTextAttributesEnum::CapStyleAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::Typography_Capitals, &returnValue));
            break;
        case DirectUI::AutomationTextAttributesEnum::CultureAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::TextElement_Language, &tempValue));
            returnValue.SetSigned(XStringPtrToLCID(tempValue.AsString()));
            break;
        case DirectUI::AutomationTextAttributesEnum::FontNameAttribute:
            {
                IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::TextElement_FontFamily, &tempValue));
                ASSERT(tempValue.GetType() == valueObject);
                CFontFamily* fontFamilyDO = do_pointer_cast<CFontFamily>(tempValue.AsObject());
                xstring_ptr strValue;
                IFC_RETURN(fontFamilyDO->get_Source(&strValue));
                returnValue.SetString(strValue);
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::FontSizeAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::TextElement_FontSize, &returnValue));
            break;
        case DirectUI::AutomationTextAttributesEnum::FontWeightAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::TextElement_FontWeight, &returnValue));
            break;
        case DirectUI::AutomationTextAttributesEnum::ForegroundColorAttribute:
        case DirectUI::AutomationTextAttributesEnum::UnderlineColorAttribute:
        case DirectUI::AutomationTextAttributesEnum::StrikethroughColorAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::TextElement_Foreground, &returnValue));
            break;
        case DirectUI::AutomationTextAttributesEnum::HorizontalTextAlignmentAttribute:
            IFC_RETURN(m_pTextOwner->GetValueByIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, &returnValue));
            break;
        case DirectUI::AutomationTextAttributesEnum::IndentationFirstLineAttribute:
            if (pParagraph)
            {
                tempValue.SetFloat(0);
                if (m_pTextOwner->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
                {
                    IFC_RETURN(m_pTextOwner->GetValueByIndex(KnownPropertyIndex::RichTextBlock_TextIndent, &tempValue));
                }
                IFC_RETURN(pParagraph->GetValueByIndex(KnownPropertyIndex::Paragraph_TextIndent, &tempValue2));
                returnValue.SetFloat(tempValue.AsFloat() + tempValue2.AsFloat());
            }
            else
            {
                returnValue.SetFloat(0);
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::IsHiddenAttribute:
            returnValue.SetBool(FALSE);
            break;
        case DirectUI::AutomationTextAttributesEnum::IsItalicAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::TextElement_FontStyle, &tempValue));
            IFC_RETURN(tempValue.GetEnum(tempValueInt));
            if (tempValueInt == static_cast<XUINT32>(DirectUI::FontStyle::Italic) || tempValueInt == static_cast<XUINT32>(DirectUI::FontStyle::Oblique))
            {
                returnValue.SetBool(TRUE);
            }
            else
            {
                returnValue.SetBool(FALSE);
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::IsReadOnlyAttribute:
            returnValue.SetBool(TRUE);
            break;
        case DirectUI::AutomationTextAttributesEnum::MarginBottomAttribute:
            IFC_RETURN(m_pTextOwner->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Margin, &tempValue));
            thickNess = tempValue.AsThickness();
            if (pParagraph)
            {
                IFC_RETURN(pParagraph->GetValueByIndex(KnownPropertyIndex::Block_Margin, &tempValue));
                paraThickNess = tempValue.AsThickness();
                if (thickNess != nullptr && paraThickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->bottom + paraThickNess->bottom);
                }
            }
            else
            {
                if (thickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->bottom);
                }
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::MarginLeadingAttribute:
            IFC_RETURN(m_pTextOwner->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Margin, &tempValue));
            thickNess = tempValue.AsThickness();
            if (pParagraph)
            {
                IFC_RETURN(pParagraph->GetValueByIndex(KnownPropertyIndex::Block_Margin, &tempValue));
                paraThickNess = tempValue.AsThickness();
                if (thickNess != nullptr && paraThickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->left + paraThickNess->left);
                }
            }
            else
            {
                if (thickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->left);
                }
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::MarginTopAttribute:
            IFC_RETURN(m_pTextOwner->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Margin, &tempValue));
            thickNess = tempValue.AsThickness();
            if (pParagraph)
            {
                IFC_RETURN(pParagraph->GetValueByIndex(KnownPropertyIndex::Block_Margin, &tempValue));
                paraThickNess = tempValue.AsThickness();
                if (thickNess != nullptr && paraThickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->top + paraThickNess->top);
                }
            }
            else
            {
                if (thickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->top);
                }
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::MarginTrailingAttribute:
            IFC_RETURN(m_pTextOwner->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Margin, &tempValue));
            thickNess = tempValue.AsThickness();
            if (pParagraph)
            {
                IFC_RETURN(pParagraph->GetValueByIndex(KnownPropertyIndex::Block_Margin, &tempValue));
                paraThickNess = tempValue.AsThickness();
                if (thickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->right + paraThickNess->right);
                }
            }
            else
            {
                if (thickNess != nullptr)
                {
                    returnValue.SetFloat(thickNess->right);
                }
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::IsSubscriptAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::Typography_Variants, &tempValue));
            IFC_RETURN(tempValue.GetEnum(tempValueInt));
            if (tempValueInt == static_cast<XUINT32>(DirectUI::FontVariants::Subscript))
            {
                returnValue.SetBool(TRUE);
            }
            else
            {
                returnValue.SetBool(FALSE);
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::IsSuperscriptAttribute:
            IFC_RETURN(pContainingElement->GetValueByIndex(KnownPropertyIndex::Typography_Variants, &tempValue));
            IFC_RETURN(tempValue.GetEnum(tempValueInt));
            if (tempValueInt == static_cast<XUINT32>(DirectUI::FontVariants::Superscript))
            {
                returnValue.SetBool(TRUE);
            }
            else
            {
                returnValue.SetBool(FALSE);
            }
            break;
        case DirectUI::AutomationTextAttributesEnum::UnderlineStyleAttribute:
        case DirectUI::AutomationTextAttributesEnum::StrikethroughStyleAttribute:
        {
            const TextFormatting* pTextFormatting = nullptr;
            IFC_RETURN(pContainingElement->GetTextFormatting(&pTextFormatting));

            if (((static_cast<XUINT32>(pTextFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Underline)) && attributeID == DirectUI::AutomationTextAttributesEnum::UnderlineStyleAttribute)
                || ((static_cast<XUINT32>(pTextFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Strikethrough)) && attributeID == DirectUI::AutomationTextAttributesEnum::StrikethroughStyleAttribute))
            {
                returnValue.SetEnum(TextDecorationLineStyle_Single);
            }
            else
            {
                returnValue.SetEnum(TextDecorationLineStyle_None);
            }
            break;
        }
        default:
            return E_NOT_SUPPORTED;
        }
    }

    IFC_RETURN(pRetValue->CopyConverted(returnValue));

    return S_OK;
}

// TextAttribute value specific comparer of CValue.
_Check_return_ HRESULT CTextRangeAdapter::AttributeValueComparer(_In_ CValue value1, _In_ CValue value2, _Out_ bool* pRetValue)
{
    CDependencyObject *pDO1 = nullptr;
    CSolidColorBrush *pBrush1 = nullptr;
    CSolidColorBrush *pBrush2 = nullptr;
    CValue value1AsString;
    CValue value2AsString;

    *pRetValue = FALSE;
    ASSERT(value1.GetType() == value2.GetType());
    if (value1.GetType() == valueObject)
    {
        pDO1 =  value1.AsObject();
        if (pDO1->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
        {
            pBrush1 = do_pointer_cast<CSolidColorBrush>(pDO1);
            pBrush2 = do_pointer_cast<CSolidColorBrush>(value2.AsObject());
            *pRetValue = pBrush1->m_rgb == pBrush2->m_rgb;
        }
        else if ((pDO1)->OfTypeByIndex<KnownTypeIndex::String>())
        {
            IFC_RETURN(GetCValueStringFromCValueObject(value1, &value1AsString));
            IFC_RETURN(GetCValueStringFromCValueObject(value2, &value2AsString));
            *pRetValue = value1AsString == value2AsString;
        }
    }
    else
    {
        *pRetValue = value1 == value2;
    }

    return S_OK;
}


_Check_return_ HRESULT CTextRangeAdapter::GetCValueStringFromCValueObject(_In_ CValue valueAsObject, _Out_ CValue* pValueAsString)
{
    CDependencyObject* pDO = nullptr;
    const CDependencyProperty *pdp = nullptr;
    CValue  internalValue;

    IFCPTR_RETURN(pValueAsString);
    pValueAsString->SetString(xstring_ptr::NullString());
    if (valueAsObject.GetType() == valueObject)
    {
        pDO = valueAsObject.AsObject();
        if (pDO)
        {
            if ((pDO)->OfTypeByIndex<KnownTypeIndex::String>())
            {
                pdp = (pDO)->GetContentProperty();
                IFCEXPECT_RETURN(pdp);
                IFC_RETURN((pDO)->GetValue(pdp, &internalValue));
                pValueAsString->SetString(internalValue.AsString());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::IsHiddenRunOrLineBreak(_In_ CTextElement* pElement, _Out_ bool* pisHiddenRunOrLineBreak)
{
    XUINT32 characterPosition;
    ITextContainer *pTextContainer = nullptr;
    CTextElement *pContainingElement = nullptr;
    const InheritedProperties* pInheritedProperties = nullptr;
    TextNestingType textNestingType;
    const TextFormatting *pFormatting = nullptr;
    const WCHAR* pCharacters = nullptr;
    XUINT32 cCharacters = 0;
    xref_ptr<CTextPointerWrapper> textPointer;

    *pisHiddenRunOrLineBreak = FALSE;
    if (pElement->GetTypeIndex() == KnownTypeIndex::LineBreak)
    {
        *pisHiddenRunOrLineBreak = TRUE;
    }

    if (*pisHiddenRunOrLineBreak == FALSE)
    {
        IFC_RETURN(pElement->GetContentStart(textPointer.ReleaseAndGetAddressOf()));
        IFC_RETURN(textPointer->GetPlainTextPosition().GetOffset(&characterPosition));
        pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);
        if (pElement->GetInlineCollection() == nullptr)
        {
            IFC_RETURN(pTextContainer->GetRun(characterPosition, &pFormatting, &pInheritedProperties, &textNestingType, &pContainingElement, &pCharacters, &cCharacters));
            if (pCharacters == nullptr && cCharacters > 0)
            {
                *pisHiddenRunOrLineBreak = TRUE;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextRangeAdapter::EnsureAttributeIdInfoForStartElement(_In_ CTextElement* pElement)
{
    ITextContainer *pTextContainer = nullptr;
    CParagraph *pParagraph = nullptr;
    XINT32 size = ARRAY_SIZE(AttributeIdInfo);

    pTextContainer = CTextAdapter::GetTextContainer(m_pTextOwner);
    pParagraph = CTextRangeAdapter::GetParagraphFromTextElement(pElement);
    for (XINT32 i = 0; i < size; i++)
    {
        IFC_RETURN(GetAttributeValueFromTextElement(AttributeIdInfo[i], pElement, &(m_attributeValueInfoTable[i]), pParagraph));
    }

    return S_OK;
}

_Check_return_ CParagraph* CTextRangeAdapter::GetParagraphFromTextElement(_In_ CTextElement* pElement)
{
    CDependencyObject *pTempAsDO = nullptr;
    CParagraph* pParagraph = nullptr;

    pTempAsDO = static_cast<CDependencyObject*>(pElement);
    while (pTempAsDO
        && pTempAsDO->GetTypeIndex() != KnownTypeIndex::Paragraph
        && pTempAsDO->GetTypeIndex() != KnownTypeIndex::TextBlock)
    {
        if (pTempAsDO->GetInheritanceParentInternal() != nullptr)
        {
            pTempAsDO = static_cast<CDependencyObject*>(pTempAsDO->GetInheritanceParentInternal());
        }
        else
        {
            pTempAsDO = static_cast<CDependencyObject*>(pTempAsDO->GetParent());
        }
    }
    pParagraph = do_pointer_cast<CParagraph>(pTempAsDO);

    return pParagraph;
}

// This Method determines if the Start pointer of this range is at EmptyLine/degenerate line
_Check_return_ HRESULT CTextRangeAdapter::IsAtEmptyLine(_Out_ bool* pbIsEmptyLine)
{
    HRESULT hr  = S_OK;
    CTextRangeAdapter** pVisibleRangesForAdapter = nullptr;
    XINT32 startoffset = 0;
    XINT32 endoffset = 0;
    XINT32 offset = 0;
    XINT32 nLines = 0;
    XINT32 lineIndex = -1;
    XINT32 deltaOffset = 0;

    *pbIsEmptyLine = FALSE;

    IFCPTR(m_pTextAdapter);
    IFC(m_pStartTextPointer->GetOffset(&offset));
    IFC((m_pTextAdapter)->GetVisibleRanges(&pVisibleRangesForAdapter, &nLines));
    for (lineIndex = 0; lineIndex < nLines; lineIndex++)
    {
        IFC(pVisibleRangesForAdapter[lineIndex]->m_pStartTextPointer->GetOffset(&startoffset));
        deltaOffset = startoffset - endoffset;
        IFC(pVisibleRangesForAdapter[lineIndex]->m_pEndTextPointer->GetOffset(&endoffset));
        // this handles the break between two consecutive caused by reserved runs
        // handles empty line caused by Line breaks when moving Start pointer.
        if (offset >= startoffset - deltaOffset && ((offset < endoffset) || (offset == startoffset)))
        {
            break;
        }
    }
    if (startoffset == endoffset)
    {
        *pbIsEmptyLine = TRUE;
    }
Cleanup:
    for (int j = 0; j < nLines; j++)
    {
        ReleaseInterface(pVisibleRangesForAdapter[j]);
    }
    delete [] pVisibleRangesForAdapter;
    return hr;
}

