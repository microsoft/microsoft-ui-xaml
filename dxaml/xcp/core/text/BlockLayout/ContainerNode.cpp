// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContainerNode.h"
#include "ContainerDrawingContext.h"

using namespace RichTextServices;

ContainerNode::ContainerNode(
    _In_ BlockLayoutEngine *pBlockLayoutEngine,
    _In_ CDependencyObject *pElement,
    _In_opt_ ContainerNode *pParentNode
    ) : BlockNode(pBlockLayoutEngine, pElement, pParentNode),
    m_pFirstChild(NULL)
{
}

ContainerNode::~ContainerNode()
{
    ClearChildren(m_pFirstChild);
    delete m_pDrawingContext;
}

//---------------------------------------------------------------------------
//
// ContainerNode::GetBaselineAlignmentOffset
//
// Synopsis:
//      Gets baseline alignment offset of first child.
//
//---------------------------------------------------------------------------
XFLOAT ContainerNode::GetBaselineAlignmentOffset() const
{
    XFLOAT baselineAlignmentOffset = GetContentRenderingOffset().y;
    if (m_pFirstChild != NULL)
    {
        baselineAlignmentOffset += m_pFirstChild->GetBaselineAlignmentOffset();
    }
    return baselineAlignmentOffset;
}

//---------------------------------------------------------------------------
//
// ContainerNode::GetChildOffset
//
// Synopsis:
//      Gets the offset of a child relative to the container's coordinate
//      system.
//
//---------------------------------------------------------------------------
XPOINTF ContainerNode::GetChildOffset(_In_ BlockNode *pChild)
{
    XPOINTF childOffset = GetContentRenderingOffset();
    BlockNode *pNode = m_pFirstChild;

    // Use RenderSize for offsetting, this type of calculation is only really valid post-Arrange.
    while (pNode != pChild)
    {
        childOffset.y += (pNode->GetRenderSize()).height;
        pNode = pNode->GetNext();
    }

    return childOffset;
}

_Check_return_ HRESULT ContainerNode::IsAtInsertionPosition(
    _In_ XUINT32 position,
    _Out_ bool *pIsAtInsertionPosition
    )
{
    BlockNode *pChild = NULL;
    XUINT32 childStartPosition;

    // ContainerNode may not be the root of block layout, so it should assume
    // that offsets will be adjusted by callers/parents to be within its scope.
    IFCEXPECT_ASSERT_RETURN(position < m_length);

    IFC_RETURN(GetChildContainingPosition(
        position,
        &pChild,
        &childStartPosition,
        NULL));

    if (pChild != NULL)
    {
        position -= childStartPosition;
        IFC_RETURN(pChild->IsAtInsertionPosition(position, pIsAtInsertionPosition));
    }
    else
    {
        *pIsAtInsertionPosition = FALSE;
    }

    return S_OK;
}

_Check_return_ HRESULT ContainerNode::PixelPositionToTextPosition(
    _In_ const XPOINTF& pixel,
    _Out_ XUINT32 *pPosition,
    _Out_opt_ TextGravity *pGravity
    )
{
    BlockNode *pChild = NULL;
    XUINT32 childStartPosition;
    XPOINTF childStartPoint;
    XUINT32 position = 0;
    TextGravity gravity = LineForwardCharacterForward;
    XPOINTF adjustedPoint = pixel;

    IFC_RETURN(GetChildContainingPoint(
        adjustedPoint,
        &pChild,
        &childStartPoint,
        &childStartPosition));

    if (pChild != NULL)
    {
        adjustedPoint.x -= childStartPoint.x;
        adjustedPoint.y -= childStartPoint.y;
        IFC_RETURN(pChild->PixelPositionToTextPosition(
            adjustedPoint,
            &position,
            &gravity));

        position += childStartPosition;
    }

    *pPosition = position;
    if (pGravity)
    {
        *pGravity = gravity;
    }

    return S_OK;
}

_Check_return_ HRESULT ContainerNode::GetTextBounds(
    _In_ XUINT32 start,
    _In_ XUINT32 length,
    _Inout_ xvector<RichTextServices::TextBounds> *pBounds
    )
{
    BlockNode *pBoundsStartChild = NULL;
    BlockNode *pBoundsEndChild = NULL;
    XPOINTF childOffset;
    XUINT32 childStartPosition;
    XUINT32 remainingLength = length;
    XUINT32 index = 0;

    ASSERT(length > 0);
    IFCEXPECT_ASSERT_RETURN(start < m_length);
    IFCEXPECT_ASSERT_RETURN((start + length) <= m_length);

    // Start+Length is inclusive, so end index is start + length - 1.
    IFC_RETURN(GetChildContainingPosition(start, &pBoundsStartChild, &childStartPosition, &childOffset));
    IFC_RETURN(GetChildContainingPosition(start + length - 1, &pBoundsEndChild, NULL, NULL));

    if (pBoundsStartChild != NULL)
    {
        start -= childStartPosition;
        do
        {
            ASSERT(remainingLength > 0);
            ASSERT(pBoundsStartChild != NULL);
            length = MIN(remainingLength, (pBoundsStartChild->GetContentLength() - start));
            index = pBounds->size();

            // Get bounds from the child, then translate back into container's space.
            IFC_RETURN(pBoundsStartChild->GetTextBounds(start, length, pBounds));
            for (XUINT32 i = index; i < pBounds->size(); i++)
            {
                (*pBounds)[i].rect.X += childOffset.x;
                (*pBounds)[i].rect.Y += childOffset.y;
            }

            // Adjust start, length and child offset.
            // Subsequent children will start at 0.
            remainingLength -= length;
            childOffset.y += pBoundsStartChild->GetRenderSize().height;
            start = 0;

            if (pBoundsStartChild == pBoundsEndChild)
            {
                break;
            }
            else
            {
                pBoundsStartChild = pBoundsStartChild->GetNext();
            }
        }
        while (true);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// ContainerNode::ArrangeCore
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ContainerNode::ArrangeCore(
    _In_ XSIZEF finalSize
    )
{
    XSIZEF remainingSize(finalSize);

    m_renderSize.width = finalSize.width;
    m_renderSize.height = 0.0f;

    // Drawing context is not needed at this stage by container node, but may be used by the owner post-Arrange to generate edges.
    // For ContainerNode it is a proxy to children's DrawingContexts, which may be created by children during their
    // Arrange phase and then will be required to generate rendering instructions.
    if (m_pDrawingContext == NULL)
    {
        m_pDrawingContext = new ContainerDrawingContext(this);
    }

    if (m_pFirstChild != NULL)
    {
        BlockNode *pChildNode = m_pFirstChild;

        do
        {
            IFC_RETURN(pChildNode->Arrange(remainingSize));

            XSIZEF childRenderSize = pChildNode->GetRenderSize();
            remainingSize.height = MAX(0.0f, remainingSize.height - childRenderSize.height);
            m_renderSize.height += childRenderSize.height;
            m_renderSize.width = MAX(m_renderSize.width, childRenderSize.width);

            pChildNode = pChildNode->GetNext();
        }
        while (pChildNode != NULL);
    }

    // RenderSize is either the sum of children's sizes or the final size, whichever is smaller.
    // If all children won't fit in the available slot, the node won't render out of bounds.
    m_renderSize.height = MIN(m_renderSize.height, finalSize.height);

    return S_OK;
}

//---------------------------------------------------------------------------
//
// ContainerNode::RenderCore
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ContainerNode::DrawCore(
    _In_ bool forceDraw
    )
{
    BlockNode *pChildNode = m_pFirstChild;

    while (pChildNode != NULL)
    {
        IFC_RETURN(pChildNode->Draw(forceDraw));
        pChildNode = pChildNode->GetNext();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// ContainerNode::ClearChildren
//
// Synopsis:
//      Deletes all child nodes starting at the specified child.
//
//---------------------------------------------------------------------------
void ContainerNode::ClearChildren(
    _In_opt_ BlockNode *pStart)
{
    BlockNode *pChildNode;

    // Make sure the start's previous node is not pointing to deleted memory.
    if (pStart != NULL &&
        pStart->GetPrevious() != NULL)
    {
        pStart->GetPrevious()->SetNext(NULL);
    }

    while (pStart != NULL)
    {
        pChildNode = pStart->GetNext();
        delete pStart;
        pStart = pChildNode;
    }
}

_Check_return_ HRESULT ContainerNode::GetChildContainingPosition(
    _In_ XUINT32 position,
    _Outptr_ BlockNode **ppChild,
    _Out_opt_ XUINT32 *pChildStartPosition,
    _Out_opt_ XPOINTF *pChildStartPoint
    ) const
{
    HRESULT hr = S_OK;
    BlockNode *pChild = m_pFirstChild;
    XPOINTF childStartPoint = GetContentRenderingOffset();
    XUINT32 childStartPosition = 0;

    ASSERT(position < m_length);

    while (pChild != NULL)
    {
        if (position < pChild->GetContentLength())
        {
            break;
        }
        else
        {
            childStartPoint.y += pChild->GetRenderSize().height;
            childStartPosition += pChild->GetContentLength();
            position -= pChild->GetContentLength();
            pChild = pChild->GetNext();
        }
    }

    *ppChild = pChild;
    if (pChildStartPosition)
    {
        *pChildStartPosition = childStartPosition;
    }
    if (pChildStartPoint)
    {
        *pChildStartPoint = childStartPoint;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT ContainerNode::GetChildContainingPoint(
    _In_ const XPOINTF& point,
    _Outptr_ BlockNode **ppChild,
    _Out_opt_ XPOINTF *pChildStartPoint,
    _Out_opt_ XUINT32 *pChildStartPosition
    ) const
{
    HRESULT hr = S_OK;
    BlockNode *pChild = m_pFirstChild;
    XPOINTF childStartPoint = GetContentRenderingOffset();
    XUINT32 childStartPosition = 0;
    XPOINTF localPoint = { point.x - childStartPoint.x, point.y - childStartPoint.y };

    // Pixel hit testing must always snap to some child.
    // The the point is outside the children collection, snap to the first
    // or last child based on y-coordinate.
    while (pChild != NULL)
    {
        XFLOAT childRenderHeight = pChild->GetRenderSize().height;

        if (localPoint.y < childRenderHeight ||
            pChild->GetNext() == NULL)
        {
            break;
        }
        else
        {
            childStartPoint.y += childRenderHeight;
            childStartPosition += pChild->GetContentLength();
            localPoint.y -= childRenderHeight;
            pChild = pChild->GetNext();
        }
    }

    *ppChild = pChild;
    if (pChildStartPosition)
    {
        *pChildStartPosition = childStartPosition;
    }
    if (pChildStartPoint)
    {
        *pChildStartPoint = childStartPoint;
    }

    RRETURN(hr);
}
