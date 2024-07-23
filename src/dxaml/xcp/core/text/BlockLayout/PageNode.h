// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContainerNode.h"
#include "DynamicArray.h"
#include "BlockNodeBreak.h"

class ParagraphNode;

//---------------------------------------------------------------------------
//
//  PageNode
//
//  Represents one page of content.
//
//---------------------------------------------------------------------------
class PageNode final : public ContainerNode
{
public:
    PageNode(
        _In_ BlockLayoutEngine *pBlockLayoutEngine,
        _In_ CBlockCollection *pBlocks,
        _In_ CFrameworkElement *pPageOwner
        );
    ~PageNode() override;

    // Embedded element hosting helpers called from ParagraphNode to host elements on the page.
    _Check_return_ HRESULT AddElement(
        _In_ CInlineUIContainer *pContainer,
        _In_ ParagraphNode *pParagraphNode
        );
    _Check_return_ HRESULT RemoveElement(
        _In_ CInlineUIContainer *pContainer,
        _In_ ParagraphNode *pParagraphNode
        );
    _Check_return_ HRESULT RemoveElement(
        _In_ CInlineUIContainer *pContainer,
        _In_ XUINT32 elementIndex
        );
    _Check_return_ HRESULT UpdateElementPosition(
        _In_ CInlineUIContainer *pContainer,
        _In_ ParagraphNode *pParagraphNode,
        _In_ const XPOINTF &position
        );
    _Check_return_ HRESULT GetElementPosition(
        _In_ CInlineUIContainer *pContainer,
        _In_ ParagraphNode *pParagraphNode,
        _Out_ XPOINTF *pPosition
        );
    _Check_return_ HRESULT GetElementsWithinRange(
        _In_ XUINT32 position1,
        _In_ XUINT32 position2,
        _Outptr_result_buffer_all_(*pnCount) CInlineUIContainer ***pppChildren,
        _Out_ XINT32* pnCount
        );

    _Check_return_ HRESULT RemoveParagraphEmbeddedElements(_In_ ParagraphNode *pParagraphNode);
    void MarkParagraphEmbeddedElementsInvisible(_In_ ParagraphNode *pParagraphNode);

    XPOINTF GetContentRenderingOffset() const override;

    // Gets the absolute start position of this PageNode in the BlockCollection.
    XUINT32 GetStartPosition() const;

    // Gets FrameworkElement that hosts this page.
    CFrameworkElement* GetPageOwner() const;

    // Handle desired size changes in embedded elements.
    void OnChildDesiredSizeChanged(_In_ CUIElement* pChild);

    uint32_t FindInlineUIContainerOffset(_In_ CInlineUIContainer* iuc);

protected:
    _Check_return_ HRESULT MeasureCore(
        _In_ XSIZEF availableSize,
        _In_ XUINT32 blockMaxLines,
        _In_ bool allowEmptyContent,
        _In_ bool measureBottomless,
        _In_ bool suppressTopMargin,
        _In_opt_ BlockNodeBreak *pPreviousBreak
        ) override;
     _Check_return_ HRESULT ArrangeCore(
        _In_ XSIZEF finalSize
        ) override;

private:
    // CFrameworkElement that acts as the container for all embedded elements processed by this PageNode.
    // This need not be the same as the element that owns the BlockCollection that is PageNode's content,
    // but it is where any embedded elements encountered during formatting are attached.
    CFrameworkElement* m_pPageOwner;

    // Struct used by PageNode to store information about embedded elements obtained during different stages of formatting.
    struct EmbeddedElementInfo
    {
        CInlineUIContainer *pContainer;
        ParagraphNode *pParagraphNode;
        XPOINTF position;
        bool isVisible;
    };
    typedef xvector<EmbeddedElementInfo> EmbeddedElementInfoVector;
    EmbeddedElementInfoVector m_embeddedElements;

    // Cached index of this page's first child in the block collection. This is used to speed up calculations in
    // Measure when there's an incoming BreakRecord - we can compare the start index against the break record index
    // and delete children if necessary, etc. If content is invalidated all children will be deleted and PageNode will
    // ignore cached start index value in this case.
    // TODO: Consider setting it to some default value e.g. -1 in InvalidateContent override. Not necessary
    // now since can just check if first child == NULL, but may be necessary with incremental layout.
    uint32_t m_firstChildIndex;

    _Check_return_ HRESULT GetChildNode(
        _In_opt_ CBlockCollection *pBlocks,
        _In_ uint32_t childIndexInCollection,
        _Outptr_ BlockNode **ppChildNode
        );

    // Finds the EmbeddedElementInfo entry associated with the given embedded UIElement.
    _Check_return_ bool FindElement(
        _In_ CInlineUIContainer *pContainer,
        _In_ ParagraphNode *pParagraphNode,
        _Out_opt_ EmbeddedElementInfo *pElementInfo = nullptr,
        _Out_opt_ uint32_t *pIndex = nullptr
        );

    // Arranges embedded elements, if any.
    _Check_return_ HRESULT ArrangeEmbeddedElements();

    // Empties the embedded element list.
    void ClearEmbeddedElements();

    // Removes embedded elements that didn't fit on this page in formatting.
    _Check_return_ HRESULT RemoveClippedEmbeddedUIElements();
};

inline XUINT32 PageNode::GetStartPosition() const
{
    return ((m_pPreviousBreak == nullptr) ? 0 : m_pPreviousBreak->GetBreakIndex());
}

inline CFrameworkElement* PageNode::GetPageOwner() const
{
    return m_pPageOwner;
}

