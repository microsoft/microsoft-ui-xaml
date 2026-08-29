// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBounds.h"
#include "FlowDirection.h"

class BlockNodeBreak;
class BlockLayoutEngine;
class DrawingContext;
class ContainerNode;

typedef XFLOAT MarginCollapsingState;

//---------------------------------------------------------------------------
//
//  BlockNode
//
//---------------------------------------------------------------------------
class BlockNode
{
public:
    BlockNode(
        _In_ BlockLayoutEngine *pBlockLayoutEngine,
        _In_opt_ CDependencyObject *pElement,
        _In_opt_ ContainerNode *pParentNode
        );
    virtual ~BlockNode();

    _Check_return_ HRESULT Measure(
        _In_ XSIZEF availableSize,
        _In_ XUINT32 maxLines,
        _In_ MarginCollapsingState mcsIn,
        _In_ bool allowEmptyContent,
        _In_ bool measureBottomless,
        _In_ bool suppressTopMargin,
        _In_opt_ BlockNodeBreak *pPreviousBreak,
        _Out_opt_ MarginCollapsingState *pMcsOut
        );
    _Check_return_ HRESULT Arrange(
        _In_ XSIZEF finalSize
        );
    _Check_return_ HRESULT Draw(
        _In_ bool forceRender
        );

    virtual XFLOAT GetBaselineAlignmentOffset() const = 0;

    void InvalidateContent();
    void InvalidateMeasure();
    void InvalidateArrange();

    _Check_return_ HRESULT TransformOffsetFromRoot(
        _In_ const XPOINTF& offset, 
        _Out_ XPOINTF *pTransformedOffset
        );
    _Check_return_ HRESULT TransformOffsetToRoot(
        _In_ const XPOINTF& offset, 
        _Out_ XPOINTF *pTransformedOffset
        );

    bool IsMeasureDirty() const;
    bool IsArrangeDirty() const;

    virtual BlockNode *GetFirstChild() const;
    BlockNode *GetNext() const;
    BlockNode *GetPrevious() const;
    void SetNext(BlockNode *pNext);
    void SetPrevious(BlockNode *pPrevious);

    XSIZEF GetDesiredSize() const;
    XSIZEF GetRenderSize() const;
    XUINT32 GetContentLength() const;
    BlockNodeBreak *GetBreak() const;
    DrawingContext *GetDrawingContext() const;
    virtual XPOINTF GetContentRenderingOffset() const;

    // Query methods used by TextView.
    virtual _Check_return_ HRESULT IsAtInsertionPosition(
        _In_ XUINT32 position,
        _Out_ bool *pIsAtInsertionPosition) = 0;
    virtual _Check_return_ HRESULT PixelPositionToTextPosition(
        _In_ const XPOINTF& pixel,
        _Out_ XUINT32 *pPosition,
        _Out_opt_ TextGravity *pGravity) = 0;
    virtual _Check_return_ HRESULT GetTextBounds(
        _In_ XUINT32 start,
        _In_ XUINT32 length,
        _Inout_ xvector<RichTextServices::TextBounds> *pBounds) = 0;

    RichTextServices::FlowDirection::Enum GetFlowDirection() const;

    void CleanupRealizations();

    XUINT32 GetMeasuredLinesCount() const;

protected:
    BlockLayoutEngine *m_pBlockLayoutEngine;
    CDependencyObject *m_pElement;
    ContainerNode *m_pParentNode;

    // NOTE: desiredSize, renderSize and prevAvailableSize do not contain margin value. 
    // Internally all computation is done without adding margin, however public getters 
    // for those properties add collapsed margin when returning values to the caller.
    XSIZEF m_desiredSize;
    XSIZEF m_renderSize;
    XSIZEF m_prevAvailableSize;
    XUINT32 m_cachedMaxLines; // used to check CanBypassMeasure()
    XUINT32 m_measuredLines;

    BlockNodeBreak *m_pBreak;
    BlockNodeBreak *m_pPreviousBreak;
    XUINT32 m_length;
    DrawingContext *m_pDrawingContext;
    XTHICKNESS m_margin;

    // Measure/Arrange Core are virtual overrides that derived classes must implement to customize layout logic.
    // Public Measure/Arrange methods wrap Core methods in layout bypass checks so when Measure/ArrangeCore is called
    // on a derived class, it knows that bypass is not a possibility and can implement layout logic directly.
    virtual _Check_return_ HRESULT MeasureCore(
        _In_ XSIZEF availableSize,
        _In_ XUINT32 maxLines,
        _In_ bool allowEmptyContent,
        _In_ bool measureBottomless,
        _In_ bool suppressTopMargin,
        _In_opt_ BlockNodeBreak *pPreviousBreak
        ) = 0;
    virtual _Check_return_ HRESULT ArrangeCore(
        _In_ XSIZEF finalSize
        ) = 0;
    virtual _Check_return_ HRESULT DrawCore(
        _In_ bool forceDraw
        ) = 0;

    // Virtual layout bypass checks to allow derived classes to implement their own layout bypass policies.
    virtual _Check_return_ HRESULT CanBypassMeasure(
        _In_ XSIZEF availableSize,
        _In_ XUINT32 maxLines,
        _In_ bool allowEmptyContent,
        _In_ bool measureBottomless,
        _In_opt_ BlockNodeBreak *pPreviousBreak,
        _Out_ bool *pCanBypass
        );
    bool CanBypassArrange(
        _In_ XSIZEF finalSize
        );

    // Protected data about the state of layout that can be used by overrides to get layout information.
    bool IsContentDirty() const;
    bool IsMeasureInProgress() const;
    bool IsArrangeInProgress() const;
    bool IsDrawInProgress() const;
    bool IsEmptyContentAllowed() const;
    bool IsMeasureBottomless() const;

private:    
    BlockNode *m_pNext;
    BlockNode *m_pPrevious;

    // Layout state "flags".
    // The following bools serve as layout state "flags". All variables which display flag-like behavior should
    // be declared in this block to ensure that they are grouped together to reduce memory footprint.
    bool m_isContentDirty : 1;
    bool m_isMeasureDirty : 1;
    bool m_isArrangeDirty : 1;
    bool m_isMeasureInProgress : 1;
    bool m_isArrangeInProgress : 1;
    bool m_isDrawInProgress : 1;
    bool m_isEmptyContentAllowed : 1;
    bool m_measureBottomless : 1;
    bool m_isMeasureBypassed : 1;
    bool m_isArrangeBypassed : 1;
};

inline bool BlockNode::IsMeasureDirty() const
{
    return m_isMeasureDirty;
}

inline bool BlockNode::IsArrangeDirty() const
{
    return m_isArrangeDirty;
}

inline BlockNode *BlockNode::GetFirstChild() const
{
    // Base BlockNode has no children, return NULL here.
    return NULL;
}

inline BlockNode *BlockNode::GetNext() const
{
    return m_pNext;
}

inline BlockNode *BlockNode::GetPrevious() const
{
    return m_pPrevious;
}

inline void BlockNode::SetNext(BlockNode *pNext) 
{
    m_pNext = pNext;
}

inline void BlockNode::SetPrevious(BlockNode *pPrevious) 
{
    m_pPrevious = pPrevious;
}

inline XSIZEF BlockNode::GetDesiredSize() const
{
    XSIZEF desiredSize = {
        m_desiredSize.width  + (m_margin.left + m_margin.right),
        m_desiredSize.height + (m_margin.top + m_margin.bottom)
    };
    return desiredSize;
}

inline XSIZEF BlockNode::GetRenderSize() const
{
    XSIZEF renderSize = {
        m_renderSize.width  + (m_margin.left + m_margin.right),
        m_renderSize.height + (m_margin.top + m_margin.bottom)
    };
    return renderSize;
}

inline XUINT32 BlockNode::GetContentLength() const
{
    return m_length;
}

inline BlockNodeBreak *BlockNode::GetBreak() const
{
    return m_pBreak;
}

inline DrawingContext *BlockNode::GetDrawingContext() const
{
    return m_pDrawingContext;
}

inline bool BlockNode::IsContentDirty() const
{
    return m_isContentDirty;
}

inline bool BlockNode::IsMeasureInProgress() const
{
    return m_isMeasureInProgress;
}

inline bool BlockNode::IsArrangeInProgress() const
{
    return m_isArrangeInProgress;
}

inline bool BlockNode::IsDrawInProgress() const
{
    return m_isDrawInProgress;
}

inline bool BlockNode::IsEmptyContentAllowed() const
{
    return m_isEmptyContentAllowed;
}

inline bool BlockNode::IsMeasureBottomless() const
{
    return m_measureBottomless;
}

inline XUINT32 BlockNode::GetMeasuredLinesCount() const
{
    return m_measuredLines;
}
