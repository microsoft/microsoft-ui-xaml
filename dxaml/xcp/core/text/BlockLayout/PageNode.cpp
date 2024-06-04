// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PageNode.h"
#include "PageNodeBreak.h"
#include "BlockLayoutEngine.h"
#include "DrawingContext.h"
#include "ParagraphNode.h"
#include "BlockLayoutHelpers.h"
#include "TextBlockViewHelpers.h"

PageNode::PageNode(
    _In_ BlockLayoutEngine *pBlockLayoutEngine,
    _In_ CBlockCollection *pBlocks,
    _In_ CFrameworkElement *pPageOwner
    ) : ContainerNode(pBlockLayoutEngine, pBlocks, nullptr),
    m_pPageOwner(pPageOwner),
    m_firstChildIndex(0)
{
}

PageNode::~PageNode()
{
    ClearEmbeddedElements();
}

//---------------------------------------------------------------------------
//
// PageNode::AddElement
//
//  Synopsis:
//      Adds the given embedded element to the host.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::AddElement(
    _In_ CInlineUIContainer *pContainer,
    _In_ ParagraphNode *pParagraphNode
    )
{
    EmbeddedElementInfo info = { pContainer, pParagraphNode, { 0.0f, 0.0f }, false /* isVisible */ };
    xref_ptr<CUIElement> pElement;

    // If the element already exists, fail.
    if (FindElement(pContainer, pParagraphNode))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(pContainer->GetChild(pElement.ReleaseAndGetAddressOf()));

    // Reparent the element if it is not parented by this page's container.
    if (pElement->GetParentInternal() != m_pPageOwner)
    {
        IFC_RETURN(m_pPageOwner->AddChild(pElement));
    }

    IFC_RETURN(m_embeddedElements.push_back(info));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  PageNode::RemoveElement
//
//  Synopsis:
//      Removes the given embedded element from the host.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::RemoveElement(
    _In_ CInlineUIContainer *pContainer,
    _In_ ParagraphNode *pParagraphNode
    )
{
    uint32_t elementIndex = 0;
    EmbeddedElementInfo elementInfo;

    if (!FindElement(pContainer, pParagraphNode, &elementInfo, &elementIndex))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(RemoveElement(pContainer, elementIndex));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  PageNode::RemoveElement
//
//  Synopsis:
//      Removes the given embedded element from the host.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::RemoveElement(
    _In_ CInlineUIContainer *pContainer,
    _In_ XUINT32 elementIndex
    )
{
    xref_ptr<CUIElement> pElement;

    IFC_RETURN(pContainer->GetChild(pElement.ReleaseAndGetAddressOf()));
    IFC_RETURN(m_pPageOwner->RemoveChild(pElement));
    IFC_RETURN(m_embeddedElements.erase(elementIndex));

    // Reset the container's cached host, since it has been removed from this page's hosted elements
    // collection it should not remember the page.
    pContainer->ClearCachedHost();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  PageNode::UpdateElementPosition
//
//  Synopsis:
//      Updates the given embedded element position, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::UpdateElementPosition(
    _In_ CInlineUIContainer *pContainer,
    _In_ ParagraphNode *pParagraphNode,
    _In_ const XPOINTF    &position
    )
{
    uint32_t elementIndex = 0;
    EmbeddedElementInfo elementInfo;

    if (!FindElement(pContainer, pParagraphNode, &elementInfo, &elementIndex))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    elementInfo.position = position;
    elementInfo.isVisible = true;

    IFC_RETURN(m_embeddedElements.set_item(elementIndex, elementInfo));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  PageNode::GetElementPosition
//
//  Synopsis:
//      Retrieves the embedded element position in host space, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::GetElementPosition(
    _In_ CInlineUIContainer *pContainer,
    _In_ ParagraphNode *pParagraphNode,
    _Out_ XPOINTF    *pPosition
    )
{
    EmbeddedElementInfo elementInfo;

    if (!FindElement(pContainer, pParagraphNode, &elementInfo))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *pPosition = elementInfo.position;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  PageNode::GetElementsWithinRange
//
//  Synopsis:
//      Retrieves the array of embedded elements within two text positions.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::GetElementsWithinRange(
    _In_ XUINT32 position1,
    _In_ XUINT32 position2,
    _Outptr_result_buffer_all_(*pnCount) CInlineUIContainer ***pppChildren,
    _Out_ XINT32* pnCount
    )
{
    HRESULT hr = S_OK;
    CInlineUIContainer **ppChildren = nullptr;
    const uint32_t size = m_embeddedElements.size();
    uint32_t textPosition;
    EmbeddedElementInfoVector embeddedElementsInRange;
    EmbeddedElementInfo elementInfo;
    uint32_t count = 0;

    for (uint32_t i = 0; i < size; ++i)
    {
        IFC(m_embeddedElements.get_item(i, elementInfo));
        textPosition = FindInlineUIContainerOffset(elementInfo.pContainer);
        if (textPosition >= position1 && textPosition <= position2)
        {
            IFC(embeddedElementsInRange.push_back(elementInfo));
        }
    }

    count = embeddedElementsInRange.size();
    ppChildren  = new CInlineUIContainer *[count];
    for (uint32_t i = 0; i < count; i++)
    {
        IFC(embeddedElementsInRange.get_item(i, elementInfo));
        ppChildren[i] = elementInfo.pContainer;
    }
    *pppChildren = ppChildren;
    ppChildren = nullptr;
    *pnCount = count;

Cleanup:
    delete [] ppChildren;
    return hr;
}

uint32_t PageNode::FindInlineUIContainerOffset(_In_ CInlineUIContainer* iuc)
{
    uint32_t positionOfIUC = 0;
    bool found = false;
    CBlockCollection *pBlocks = static_cast<CBlockCollection*>(m_pElement);

    CDependencyObject *previousBlock = nullptr;
    auto blockCollection = pBlocks->GetCollection();
    for (auto block : blockCollection)
    {
        // Account for the offset at the end of each paragraph before searching the next one
        if (previousBlock)
        {
            positionOfIUC += 2;
        }
        // Go through each collection of inlines and try to find the InlineUIContainer we're looking for
        auto inlines = static_cast<CParagraph*>(block)->GetInlineCollection();
        bool iucFound = TextBlockViewHelpers::FindIUCPositionInInlines(inlines, iuc, /* Inout */ positionOfIUC);
        if (iucFound)
        {
            found = true;
            // Don't need to search through any other paragraphs
            break;
        }
        previousBlock = block;
    }

    ASSERT(found);
    return positionOfIUC;
}

//------------------------------------------------------------------------
//
//  PageNode::RemoveParagraphEmbeddedElements
//
//  Synopsis:
//      Removes the embedded elements associated with the specified
//      ParagraphNode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::RemoveParagraphEmbeddedElements(
    _In_ ParagraphNode *pParagraphNode
    )
{
    HRESULT hr = S_OK;
    CInlineUIContainer *pContainer = nullptr;
    CUIElement *pElement = nullptr;
    uint32_t size = m_embeddedElements.size();
    uint32_t index = 0;
    EmbeddedElementInfo elementInfo;

    while (index < size)
    {
        IFC(m_embeddedElements.get_item(index, elementInfo));
        if (elementInfo.pParagraphNode == pParagraphNode)
        {
            pContainer = elementInfo.pContainer;
            IFC(pContainer->GetChild(&pElement));
            IFC(m_pPageOwner->RemoveChild(pElement));
            pContainer->ClearCachedHost();
            ReleaseInterface(pElement);
            pElement = nullptr;
            IFC(m_embeddedElements.erase(index));
            size = m_embeddedElements.size();
        }
        else
        {
            index++;
        }
    }

Cleanup:
    ReleaseInterface(pElement);
    return hr;
}

//------------------------------------------------------------------------
//
//  PageNode::MarkParagraphEmbeddedElementsInvisible
//
//  Synopsis:
//      Marks the embedded elements associated with the specified
//      ParagraphNode as not visible.
//
//------------------------------------------------------------------------
void  PageNode::MarkParagraphEmbeddedElementsInvisible(
    _In_ ParagraphNode *pParagraphNode
    )
{
    // Reset visible state on all embedded elements since any visible ones will be arranged.
    for (EmbeddedElementInfoVector::iterator it = m_embeddedElements.begin();
        it != m_embeddedElements.end();
        ++it)
    {
        if (it->pParagraphNode == pParagraphNode)
        {
            it->isVisible = false;
        }
    }
}

XPOINTF PageNode::GetContentRenderingOffset() const
{
    XPOINTF offset = {0.0f, 0.0f};
    XTHICKNESS padding;

    BlockLayoutHelpers::GetPagePadding(m_pPageOwner, &padding);
    offset.x = padding.left;
    offset.y = padding.top;

    float layoutRoundingHeightAdjustment = 0.0f;
    BlockLayoutHelpers::GetLayoutRoundingHeightAdjustment(m_pPageOwner, &layoutRoundingHeightAdjustment);
    offset.y += layoutRoundingHeightAdjustment;

    return offset;
}

//---------------------------------------------------------------------------
//
//  PageNode::OnChildDesiredSizeChanged
//
//  Synopsis:
//      Handle desired size changes in embedded elements..
//
//---------------------------------------------------------------------------
void PageNode::OnChildDesiredSizeChanged(_In_ CUIElement* pChild)
{
    // If an embedded element's desired size changed, we need to invalidate measure entirely
    // since the position/wrapping of other content may also have changed.
    InvalidateMeasure();
}

_Check_return_ HRESULT PageNode::MeasureCore(
    _In_ XSIZEF availableSize,
    _In_ XUINT32 blockMaxLines,
    _In_ bool allowEmptyContent,
    _In_ bool measureBottomless,
    _In_ bool suppressTopMargin,
    _In_opt_ BlockNodeBreak *pPreviousBreak
    )
{
    HRESULT hr = S_OK;
    BlockNode *pChildNode = nullptr;
    BlockNode *pLastChildNode = nullptr;
    BlockNode *pNewFirstChild = nullptr;
    BlockNode *pFirstChildToDelete = nullptr;
    uint32_t childIndexInCollection = 0;
    CBlockCollection *pBlocks = do_pointer_cast<CBlockCollection>(m_pElement);
    XSIZEF remainingSize(availableSize);
    uint32_t availableLinesCount(blockMaxLines);
    PageNodeBreak *pPreviousPageBreak = nullptr;
    BlockNodeBreak *pChildBreak = nullptr;
    PageNodeBreak *pBreak = nullptr;
    bool measuringExistingChildren = false;
    uint32_t firstChildIndex = 0;
    uint32_t previousBreakIndex = 0;
    XTHICKNESS padding;
    MarginCollapsingState mcs = 0.0f;
    bool finishMeasureWithoutBreak = false;

    if (IsContentDirty())
    {
        // ContentDirty always implies MeasureDirty.
        ASSERT(IsMeasureDirty());
        // If content is dirty, all children should be deleted and re-created, since the block collection
        // itself may have changed, so child nodes are not valid. Also clear all embedded elements.
        ClearChildren(m_pFirstChild);
        m_pFirstChild = nullptr;
        m_firstChildIndex = 0;
        ClearEmbeddedElements();
    }
    else if (IsMeasureDirty())
    {
        ClearEmbeddedElements();
    }

    // Adjust the size to account for padding.
    BlockLayoutHelpers::GetPagePadding(m_pPageOwner, &padding);
    remainingSize.width  = MAX(0.0f, remainingSize.width  - (padding.left + padding.right));
    remainingSize.height = MAX(0.0f, remainingSize.height - (padding.top + padding.bottom));

    m_desiredSize.width = 0.0f;
    m_desiredSize.height = 0.0f;
    m_length = 0;
    m_cachedMaxLines = blockMaxLines;
    m_measuredLines = 0;
    firstChildIndex = m_firstChildIndex;
    ReleaseInterface(m_pBreak);

    if (pPreviousBreak != nullptr)
    {
        pPreviousPageBreak = static_cast<PageNodeBreak *>(pPreviousBreak);
        if (pPreviousPageBreak != nullptr)
        {
            // Store the character offset of the previous break, it needs to be aggregated with the length
            // of this node if this node breaks.
            childIndexInCollection = pPreviousPageBreak->GetBlockIndexInCollection();
            pChildBreak = pPreviousPageBreak->GetBlockBreak();
            previousBreakIndex = pPreviousPageBreak->GetBreakIndex();
        }
    }

    // Main Measure loop has the following cases, with or without break. The break simply determines the index of the first child we'll measure.
    // 1. New set of children to be measured has no overlap with existing collection and falls entirely after it, i.e:
    //    (m_firstChildIndex + child count from previous measure) < childIndexInCollection.
    //    Delete all the old children, then measure new starting at childIndexInCollection.
    // 2. New set of children to be measured has some overlap with the existing collection, but starts after it, i.e.:
    //    (m_firstChildIndex  < childIndexInCollection) but (m_firstChildIndex + child count from previous measure) > childIndexInCollection.
    //    Delete existing children up to the point of overlap, then measure existing children, then measure new if we run out of existing children to measure.
    // 3. New set of children to be measured has some overlap with the existing collection, but starts before it, i.e.:
    //    (childIndexInCollection < m_firstChildIndex) but (childIndexInCollection + new post-measure child count) > m_firstChildIndex.
    //    Measure new children until we hit m_firstChildIndex, then measure from the existing collection, then measure any additional new content if we run out of existing children to measure.
    // 4. New set of children to be measured has no overlap with the existing collection, and falls entirely before it, i.e.:
    //    (childIndexInCollection + new post-measure child count) <= m_firstChildIndex.
    //    Measure the new collection, then delete the entire old collection and replace it with the new one when we don't detect overlap.
    if (m_pFirstChild != nullptr)
    {
        if (firstChildIndex <= childIndexInCollection)
        {
            // Cases 1 and 2. Clear everything up to the new start index and see if there are any children left.
            BlockNode *pStart = m_pFirstChild;
            while (pStart != nullptr &&
                    firstChildIndex < childIndexInCollection)
            {
                pChildNode = pStart;
                pStart = pStart->GetNext();
                delete pChildNode;
                firstChildIndex++;
            }

            // The first child should now match the index specified through a break, if any.
            m_pFirstChild = pStart;
            firstChildIndex = childIndexInCollection;

            if (m_pFirstChild != nullptr)
            {
                m_pFirstChild->SetPrevious(nullptr);

                // If m_pFirstChild != NULL, we're in case 2 - some overlap. Otherwise, we're in case 1 and will just measure new content.
                measuringExistingChildren = true;
            }

            // We've already intersected our overlapping region and deleted any existing children that are excluded.
            // Additional children to delete will be determined by breaking, assume we won't delete any.
            pFirstChildToDelete = nullptr;
        }
        else
        {
            // So far we've detected no overlap. We'll start measuring new content, and assume we should delete
            // the old content unless something changes.
            pFirstChildToDelete = m_pFirstChild;
        }
    }

    pChildNode = nullptr;
    do
    {
        // Since remainingSize.height is adjusted during the Measure loop, ensure that it never goes below 0 unless the loop is broken.
        ASSERT(remainingSize.height >= 0.0f);

        if (measuringExistingChildren)
        {
            // Overlapping children - fetch from existing collection.
            ASSERT(childIndexInCollection >= firstChildIndex);

            if (childIndexInCollection == firstChildIndex)
            {
                pChildNode = m_pFirstChild;
            }
            else
            {
                // We should have measured at least one child.
                ASSERT(pChildNode != nullptr);
                pChildNode = pChildNode->GetNext();
            }
        }
        else
        {
            // Nodes that were not covered in a previous measure. Create new.
            IFC(GetChildNode(pBlocks, childIndexInCollection, &pChildNode));
        }

        if (pChildNode != nullptr)
        {
            auto rtsErr = pChildNode->Measure(remainingSize, availableLinesCount, mcs, allowEmptyContent, measureBottomless, suppressTopMargin, pChildBreak, &mcs);
            if (rtsErr == RichTextServices::INTERNAL_ERROR)
            {
                // If LineServices couldn't format the paragraph due to too many diacritics,
                // treat the paragraph as empty and swallow the error.
                allowEmptyContent = TRUE;
                hr = S_OK;
            }
            else
            {
                IFC(rtsErr);
            }

            // In ForceContent mode, a child will have non-0 content length if it is the only node on the page.
            // But a child may have 0 content length if remaining size does not allow any content and there is already content on the page.
            // In that case, we don't want to add the child to our children collection.
            if (pChildNode->GetContentLength() > 0)
            {
                XSIZEF childDesiredSize = pChildNode->GetDesiredSize();
                uint32_t paragraphMeasuredLinesCount = pChildNode->GetMeasuredLinesCount();
                m_length += pChildNode->GetContentLength();
                m_desiredSize.width = MAX(m_desiredSize.width, childDesiredSize.width);
                m_desiredSize.height += childDesiredSize.height;
                remainingSize.height -= childDesiredSize.height;
                m_measuredLines += paragraphMeasuredLinesCount;
                pChildBreak = pChildNode->GetBreak();

                if (blockMaxLines != 0)
                {
                    ASSERT(availableLinesCount >= paragraphMeasuredLinesCount);
                    availableLinesCount -= paragraphMeasuredLinesCount;
                }

                if (!measuringExistingChildren)
                {
                    // If not measuring existing children, this is a brand new node that needs to be added to our children collection.
                    if (pLastChildNode != nullptr)
                    {
                        pLastChildNode->SetNext(pChildNode);
                        pChildNode->SetPrevious(pLastChildNode);
                    }
                }
                else
                {
                    if (pChildNode == m_pFirstChild &&
                        pLastChildNode != nullptr)
                    {
                        // When we transition from measuring new to existing we need to connect the first
                        // child to the end of the chain. At this point, set pFirstChildToDelete to NULL
                        // since it means we don't want to delete the first child.
                        pLastChildNode->SetNext(pChildNode);
                        pChildNode->SetPrevious(pLastChildNode);
                        pFirstChildToDelete = nullptr;
                    }
                }

                // Track the first node we measure as the new first node.
                if (pLastChildNode == nullptr)
                {
                    pNewFirstChild = pChildNode;
                }
                pLastChildNode = pChildNode;

                // Check if PageNode meets any criteria to end measure loop. PageNode may
                // or may not create PageNodeBreak to end measure since PageNodeBreak
                // indicates that HasContentOverflow == true. We need to do an additional check
                // when ChildNode does not create a break because there maybe additional
                // CParagarph nodes we need to measure.
                //
                // 1) Child created a break, create PageNodeBreak
                // 2) Child did not create a break and there is more content to measure
                //     a) PageNode reached maxLine or maxHeight, create PageNodeBreak
                //     b) PageNode did not reach the restrictions, continue measuring
                // 3) Child did not create a break and measured all content, exit measure
                //    loop without setting a break.
                bool createBreak = false;
                if (pChildBreak != nullptr)
                {
                    createBreak = true;
                }
                else if (pBlocks != nullptr && pBlocks->GetCount() > (childIndexInCollection + 1))
                {
                    childIndexInCollection++;

                    if ( remainingSize.height <= 0.0f ||
                        (blockMaxLines != 0 && availableLinesCount <= 0))
                    {
                        createBreak = true;
                    }
                }
                else
                {
                    finishMeasureWithoutBreak = true;
                }

                if (createBreak)
                {
                    pBreak = new PageNodeBreak(m_length + previousBreakIndex,
                                                        childIndexInCollection,
                                                        pChildBreak);
                }

                // If we are exiting measure loop and we have measured existing children,
                // reset pFirstChildToDelete to the first child after this one for cleanup.
                if ((pBreak != nullptr || finishMeasureWithoutBreak) &&
                    measuringExistingChildren &&
                    pChildNode->GetNext() != nullptr)
                {
                    pFirstChildToDelete = pChildNode->GetNext();
                }

                // If we measured a child with any content at all we can allow subsequent children to have empty content.
                // Also, we don't need to suppress the top margin anymore.
                allowEmptyContent = TRUE;
                suppressTopMargin = FALSE;
            }
            else
            {
                // If we get to the point where we measured a child, and it had no content, we should be allowing empty
                // content. The only way a page can have empty content if allowEmptyContent was initially FALSE is if we're
                // at the end of content.
                ASSERT(allowEmptyContent);

                // No content fit in this child, break the page.
                pBreak = new PageNodeBreak(m_length + previousBreakIndex,
                                                  childIndexInCollection,
                                                  pChildBreak);

                if (measuringExistingChildren)
                {
                    // Delete list starts at this child.
                    pFirstChildToDelete = pChildNode;
                }
                else
                {
                    delete pChildNode;
                    pChildNode = nullptr;
                }
            }

            // After this child is measured, evaluate whether our state of measuring new vs. existing children changed.
            if (measuringExistingChildren)
            {
                if (pChildNode == nullptr ||
                    pChildNode->GetNext() == nullptr)
                {
                    // We were measuring existing children, in case 2 or 3, but ran out of content.
                    // Switch to creating and measuring new nodes.
                    measuringExistingChildren = false;
                }
            }
            else
            {
                if (childIndexInCollection == firstChildIndex)
                {
                    // We were measuring new children, in case 3, and hit the overlap point. Switch to
                    // the existing collection.
                    measuringExistingChildren = true;
                }
            }
        }

    } while (pChildNode != nullptr &&
             pBreak == nullptr &&
             !finishMeasureWithoutBreak);

    ClearChildren(pFirstChildToDelete);
    m_pFirstChild = pNewFirstChild;
    m_firstChildIndex = (pPreviousPageBreak == nullptr) ? 0 : pPreviousPageBreak->GetBlockIndexInCollection();

    // Add padding to desired size.
    m_desiredSize.width  += (padding.left + padding.right);
    m_desiredSize.height += (padding.top  + padding.bottom);

    m_pBreak = pBreak;
    pBreak = nullptr;

    // Remove any embedded elements that were added during formatting but then found to not fit on the page.
    IFC(RemoveClippedEmbeddedUIElements());

Cleanup:
    ReleaseInterface(pBreak);
    return hr;
}

_Check_return_ HRESULT PageNode::ArrangeCore(_In_ XSIZEF finalSize)
{
    XTHICKNESS padding;
    XSIZEF contentFinalSize;

    // Adjust the final size and viewport to account for padding.
    BlockLayoutHelpers::GetPagePadding(m_pPageOwner, &padding);
    contentFinalSize.width  = MAX(0.0f, finalSize.width  - (padding.left + padding.right));
    contentFinalSize.height = MAX(0.0f, finalSize.height - (padding.top + padding.bottom));

    // Since PageNode calls ContainerNode::ArrangeCore, that will invalidate any children nodes
    // if Arrange is not valid. PageNode-specific logic just resets embedded element visibility.
    // The Visibility is actually set in Render, when lines are drawn, but since Arrange guarantees that Render will
    // follow, this is okay.
    if (IsArrangeDirty())
    {
        // Reset visible state on all embedded elements since any visible ones will be re-arranged/re-rendered.
        for (EmbeddedElementInfoVector::iterator it = m_embeddedElements.begin();
            it != m_embeddedElements.end();
            ++it)
        {
            it->isVisible = false;
        }
    }

    // Arrange all children, then embedded elements. Embedded elements must be arranged after children because
    // offsets are determined when lines are drawn.
    IFC_RETURN(ContainerNode::ArrangeCore(contentFinalSize));

    IFC_RETURN(ArrangeEmbeddedElements());

    // Add padding back to render size, but never exceed final height.  We bubble up the
    // real render width (which might exceed the final width) to get FrameworkElement to
    // apply a layout clip in scenarios where we were unable to fit within the constraint.
    // See comments in ParagraphNode::ArrangeCore for more details.
    m_renderSize.width  = (m_renderSize.width + padding.left + padding.right);
    m_renderSize.height = MIN(finalSize.height, (m_renderSize.height + padding.top + padding.bottom));

    return S_OK;
}

_Check_return_  HRESULT PageNode::GetChildNode(
    _In_opt_ CBlockCollection *pBlocks,
    _In_ uint32_t childIndexInCollection,
    _Outptr_ BlockNode **ppChildNode
    )
{
    HRESULT hr = S_OK;
    BlockNode *pChildNode = nullptr;
    CBlock *pBlock = nullptr;

    if (pBlocks != nullptr &&
        childIndexInCollection < pBlocks->GetCount())
    {
        // DOCollection will AddRef pBlock when we access it. We don't want to hold a reference
        // to the block node here - BLE lifetime should be limited to a block's life time. Release the block
        // in Cleanup.
        pBlock = static_cast<CBlock *>(pBlocks->GetItemWithAddRef(childIndexInCollection));
        if (pBlock->OfTypeByIndex<KnownTypeIndex::Paragraph>())
        {
            pChildNode = new ParagraphNode(m_pBlockLayoutEngine, (static_cast<CParagraph *>(pBlock)), this);
        }
        else
        {
            ASSERT(FALSE);
        }
    }
    else if (childIndexInCollection == 0 &&
             (pBlocks == nullptr || pBlocks->GetCount() == 0))
    {
        // Block collection may be NULL (TextBlock) or empty (contentless RichTextBlock).
        // For an empty block collection, one line of content should still be measured at layout owner's properties.
        pChildNode = new ParagraphNode(m_pBlockLayoutEngine, nullptr, this);
    }

    *ppChildNode = pChildNode;
    ReleaseInterface(pBlock);
    return hr; //RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  PageNode::FindElement
//
//  Synopsis:
//      Finds the EmbeddedElementInfo entry associated with the given embedded UIElement.
//
//------------------------------------------------------------------------
_Check_return_ bool PageNode::FindElement(
    _In_ CInlineUIContainer *pContainer,
    _In_ ParagraphNode *pParagraphNode,
    _Out_opt_ EmbeddedElementInfo *pElementInfo,
    _Out_opt_ uint32_t            *pIndex)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    const uint32_t size = m_embeddedElements.size();

    for (uint32_t i = 0; i < size; ++i)
    {
        EmbeddedElementInfo elementInfo;
        IFC(m_embeddedElements.get_item(i, elementInfo));

        if (elementInfo.pContainer == pContainer)
        {
            // If we matched the element, assert that the paragraph container is the same.
            ASSERT(elementInfo.pParagraphNode ==  pParagraphNode);

            if (pElementInfo)
            {
                *pElementInfo = elementInfo;
            }

            if (pIndex)
            {
                *pIndex = i;
            }

            return true;
        }
    }

Cleanup:
    return false;
}

//------------------------------------------------------------------------
//
//  PageNode::ArrangeEmbeddedElements
//
//  Synopsis:
//      Arranges embedded elements, if any.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::ArrangeEmbeddedElements()
{
    HRESULT hr = S_OK;
    CInlineUIContainer *pContainer = nullptr;
    CUIElement *pElement = nullptr;

    for (EmbeddedElementInfoVector::const_iterator it = m_embeddedElements.begin();
         it != m_embeddedElements.end();
         ++it)
    {
        const EmbeddedElementInfo &info = *it;
        pContainer = info.pContainer;

        IFC(pContainer->GetChild(&pElement));
        XRECTF arrangeRect = { info.position.x, info.position.y, 0, 0 };

        // Elements that don't overlap with the viewport must be explicitly pushed outside.
        // Since, for performance reasons, we don't measure lines outside the viewport, we don't
        // know precisely where they render.  But any position that gets properly clipped is fine.
        if (!info.isVisible)
        {
            arrangeRect.Y = m_renderSize.height;
        }

        if (pElement->HasLayoutStorage())
        {
            const CLayoutStorage *pLayoutStorage = pElement->GetLayoutStorage();
            arrangeRect.Width = pLayoutStorage->m_desiredSize.width;
            arrangeRect.Height = pLayoutStorage->m_desiredSize.height;
        }

        IFC(pElement->Arrange(arrangeRect));
        ReleaseInterface(pElement);
        pElement = nullptr;
    }

Cleanup:
    ReleaseInterface(pElement);
    return hr;
}

//------------------------------------------------------------------------
//
//  PageNode::ClearEmbeddedElements
//
//  Synopsis:
//      Empties the embedded element list.
//
//------------------------------------------------------------------------
void PageNode::ClearEmbeddedElements()
{
    HRESULT hr = S_OK;
    CInlineUIContainer *pContainer = nullptr;
    CUIElement *pElement = nullptr;

    for (EmbeddedElementInfoVector::iterator it = m_embeddedElements.begin();
         it != m_embeddedElements.end();
         ++it)
    {
        pContainer = it->pContainer;

        hr = pContainer->GetChild(&pElement);

        if (SUCCEEDED(hr))
        {
            VERIFYHR(m_pPageOwner->RemoveChild(pElement));
        }
        pContainer->ClearCachedHost();
        ReleaseInterface(pElement);
        pElement = nullptr;
    }

    m_embeddedElements.clear();
}

//------------------------------------------------------------------------
//
//  PageNode::RemoveClippedEmbeddedUIElements
//
//  Synopsis:
//      Removes embedded elements, if any, positioned past the end of this
//      node.
//
//  Notes:
//      Embedded elements are added to the page during line formatting, but
//      after a line is formatted it may be determined that the line (or part of
//      it) doesn't fit on the page. Embedded elements added in content that
//      doesn't fit need to be removed.
//      This method is called at the *end* of measure, so content length and
//      previous break (for start position) are guaranteed to be set to accurately
//      determine which elements are outside the page bounds. It should not be
//      called before Measure or when Measure is invalid.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT PageNode::RemoveClippedEmbeddedUIElements()
{
    bool found = false;
    uint32_t offset = 0;
    CBlockCollection *pBlocks = do_pointer_cast<CBlockCollection>(m_pElement);

    while (m_embeddedElements.size() > 0)
    {
        EmbeddedElementInfo info = m_embeddedElements[m_embeddedElements.size() - 1];

        // Get the offset for the element's InlineUIContainer in the block collection. This can be used to determine
        // whether it lies on this page, since PageNode operates at block collection-level indices.
        IFC_RETURN(pBlocks->GetElementEdgeOffset(
            info.pContainer,
            ElementStart,
            &offset,
            &found));

        // If the element lies on this page, all previous elements will too, so stop removal loop.
        if (offset < GetStartPosition() + GetContentLength())
        {
            break;
        }

        IFC_RETURN(RemoveElement(info.pContainer, m_embeddedElements.size() - 1));
    }

    return S_OK;
}
