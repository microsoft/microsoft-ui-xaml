// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BlockNode.h"

//---------------------------------------------------------------------------
//
//  ContainerNode
//
//  Base class for block nodes whose content is a collection of other nodes.
//
//---------------------------------------------------------------------------
class ContainerNode : public BlockNode
{
public:
    ContainerNode(
        _In_ BlockLayoutEngine *pBlockLayoutEngine,
        _In_ CDependencyObject *pElement,
        _In_opt_ ContainerNode *pParentNode
        );

    ~ContainerNode() override;

    XFLOAT GetBaselineAlignmentOffset() const override;

    BlockNode *GetFirstChild() const override;
    XPOINTF GetChildOffset(_In_ BlockNode *pChild);

    // Query method overrides.
    _Check_return_ HRESULT IsAtInsertionPosition(
        _In_ XUINT32 position,
        _Out_ bool *pIsAtInsertionPosition) override;

    _Check_return_ HRESULT PixelPositionToTextPosition(
        _In_ const XPOINTF& pixel,
        _Out_ XUINT32 *pPosition,
        _Out_opt_ TextGravity *pGravity) override;
    _Check_return_ HRESULT GetTextBounds(
        _In_ XUINT32 start,
        _In_ XUINT32 length,
        _Inout_ xvector<RichTextServices::TextBounds> *pBounds) override;

protected:
    BlockNode *m_pFirstChild;

    _Check_return_ HRESULT ArrangeCore(
        _In_ XSIZEF finalSize
        ) override;
    _Check_return_ HRESULT DrawCore(
        _In_ bool forceDraw
        ) override;

    // Deletes all child nodes of this node starting at pStart.
    void ClearChildren(
        _In_opt_ BlockNode *pStart
        );

private:
    // Helpers to match child to position and point for hit testing.
    _Check_return_ HRESULT GetChildContainingPosition(
        _In_ XUINT32 position,
        _Outptr_ BlockNode **ppChild,
        _Out_opt_ XUINT32 *pChildStartPosition,
        _Out_opt_ XPOINTF *pChildStartPoint
        ) const;
    _Check_return_ HRESULT GetChildContainingPoint(
        _In_ const XPOINTF& point,
        _Outptr_ BlockNode **ppChild,
        _Out_opt_ XPOINTF *pChildStartPoint,
        _Out_opt_ XUINT32 *pChildStartPosition
        ) const;
};

inline BlockNode *ContainerNode::GetFirstChild() const
{
    return m_pFirstChild;
}
