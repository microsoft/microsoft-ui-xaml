// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RPGraph.h"
#include "RPNode.h"

#include <corep.h>
#include <DeferredElement.h>
#include <depends.h>
#include <uielement.h>

_Check_return_ HRESULT RPGraph::ResolveConstraints(
    _In_ CDependencyObject* parent,
    _In_ CCoreServices* core,
    _In_ CDependencyObject* namescopeOwner,
    _In_ Jupiter::NameScoping::NameScopeType nameScopeType)
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        CValue value;
        auto& node = *it;

        node.GetLeftOfValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* leftOfNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &leftOfNode));
            node.SetLeftOfConstraint(leftOfNode);
        }

        node.GetAboveValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* aboveNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &aboveNode));
            node.SetAboveConstraint(aboveNode);
        }

        node.GetRightOfValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* rightOfNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &rightOfNode));
            node.SetRightOfConstraint(rightOfNode);
        }

        node.GetBelowValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* belowNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &belowNode));
            node.SetBelowConstraint(belowNode);
        }

        node.GetAlignHorizontalCenterWithValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* alignHorizontalCenterWithNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &alignHorizontalCenterWithNode));
            node.SetAlignHorizontalCenterWithConstraint(alignHorizontalCenterWithNode);
        }

        node.GetAlignVerticalCenterWithValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* alignVerticalCenterWithNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &alignVerticalCenterWithNode));
            node.SetAlignVerticalCenterWithConstraint(alignVerticalCenterWithNode);
        }

        node.GetAlignLeftWithValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* alignLeftWithNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &alignLeftWithNode));
            node.SetAlignLeftWithConstraint(alignLeftWithNode);
        }

        node.GetAlignTopWithValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* alignTopWithNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &alignTopWithNode));
            node.SetAlignTopWithConstraint(alignTopWithNode);
        }

        node.GetAlignRightWithValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* alignRightWithNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &alignRightWithNode));
            node.SetAlignRightWithConstraint(alignRightWithNode);
        }

        node.GetAlignBottomWithValue(&value);
        if (!value.IsNullOrUnset())
        {
            RPNode* alignBottomWithNode = nullptr;
            IFC_RETURN(GetNodeByValue(value, parent, core, namescopeOwner, nameScopeType, it, &alignBottomWithNode));
            node.SetAlignBottomWithConstraint(alignBottomWithNode);
        }

        node.GetAlignLeftWithPanelValue(&value);
        if (!value.IsNullOrUnset())
        {
            node.SetAlignLeftWithPanelConstraint(value.AsBool());
        }

        node.GetAlignTopWithPanelValue(&value);
        if (!value.IsNullOrUnset())
        {
            node.SetAlignTopWithPanelConstraint(value.AsBool());
        }

        node.GetAlignRightWithPanelValue(&value);
        if (!value.IsNullOrUnset())
        {
            node.SetAlignRightWithPanelConstraint(value.AsBool());
        }

        node.GetAlignBottomWithPanelValue(&value);
        if (!value.IsNullOrUnset())
        {
            node.SetAlignBottomWithPanelConstraint(value.AsBool());
        }

        node.GetAlignHorizontalCenterWithPanelValue(&value);
        if (!value.IsNullOrUnset())
        {
            node.SetAlignHorizontalCenterWithPanelConstraint(value.AsBool());
        }

        node.GetAlignVerticalCenterWithPanelValue(&value);
        if (!value.IsNullOrUnset())
        {
            node.SetAlignVerticalCenterWithPanelConstraint(value.AsBool());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT RPGraph::MeasureNodes(XSIZEF availableSize)
{
    for (RPNode &node : m_nodes)
    {
        IFC_RETURN(MeasureNode(&node, availableSize));
    }

    m_availableSizeForNodeResolution = availableSize;

    return S_OK;
}

_Check_return_ HRESULT RPGraph::ArrangeNodes(XRECTF finalRect)
{
    XSIZEF finalSize;
    finalSize.height = finalRect.Height;
    finalSize.width = finalRect.Width;

    // If the final size is the same as the available size that we used
    // to measure the nodes, this means that the pseudo-arrange pass  
    // that we did during the measure pass is, in fact, valid and the 
    // ArrangeRects that were calculated for each node are correct. In 
    // other words, we can just go ahead and call arrange on each
    // element. However, if the width and/or height of the final size
    // differs (e.g. when the element's HorizontalAlignment and/or
    // VerticalAlignment is something other than Stretch and thus the final
    // size corresponds to the desired size of the panel), we must first
    // recalculate the horizontal and/or vertical values of the ArrangeRects,
    // respectively.
    if (m_availableSizeForNodeResolution.width != finalSize.width)
    {
        for (RPNode &node : m_nodes)
        {
            node.SetArrangedHorizontally(false);
        }

        for (RPNode &node : m_nodes)
        {
            IFC_RETURN(ArrangeNodeHorizontally(&node, finalSize));
        }
    }

    if (m_availableSizeForNodeResolution.height != finalSize.height)
    {
        for (RPNode &node : m_nodes)
        {
            node.SetArrangedVertically(false);
        }

        for (RPNode &node : m_nodes)
        {
            IFC_RETURN(ArrangeNodeVertically(&node, finalSize));
        }
    }

    m_availableSizeForNodeResolution = finalSize;

    for (RPNode &node : m_nodes)
    {
        ASSERT(node.IsArranged());

        XRECTF layoutSlot;
        layoutSlot.X = std::max(node.m_arrangeRect.X + finalRect.X, 0.0f);
        layoutSlot.Y = std::max(node.m_arrangeRect.Y + finalRect.Y, 0.0f);
        layoutSlot.Width = std::max(node.m_arrangeRect.Width, 0.0f);
        layoutSlot.Height = std::max(node.m_arrangeRect.Height, 0.0f);

        IFC_RETURN(node.Arrange(layoutSlot));
    }

    return S_OK;
}

XSIZEF RPGraph::CalculateDesiredSize()
{
    XSIZEF maxDesiredSize = { 0.0f, 0.0f };

    MarkHorizontalAndVerticalLeaves();

    for (RPNode &node : m_nodes)
    {
        if (node.m_isHorizontalLeaf)
        {
            m_minX = 0.0f;
            m_maxX = 0.0f;
            m_isMinCapped = false;
            m_isMaxCapped = false;

            AccumulatePositiveDesiredWidth(&node, 0.0f);
            maxDesiredSize.width = std::max(maxDesiredSize.width, m_maxX - m_minX);
        }

        if (node.m_isVerticalLeaf)
        {
            m_minY = 0.0f;
            m_maxY = 0.0f;
            m_isMinCapped = false;
            m_isMaxCapped = false;

            AccumulatePositiveDesiredHeight(&node, 0.0f);
            maxDesiredSize.height = std::max(maxDesiredSize.height, m_maxY - m_minY);
        }
    }

    return maxDesiredSize;
}

_Check_return_ HRESULT RPGraph::GetNodeByValue(
    _In_ CValue& value,
    _In_ CDependencyObject* parent,
    _In_ CCoreServices* core,
    _In_ CDependencyObject* namescopeOwner,
    _In_ Jupiter::NameScoping::NameScopeType nameScopeType,
    _In_ std::forward_list<RPNode>::iterator it,
    _Outptr_opt_ RPNode** ppNode)
{
    // Here we will have either a valueString which corresponds to the name
    // of the element we are looking for, or a valueObject of type UIElement
    // which is a direct reference to said element.
    if (value.GetType() == valueString)
    {
        const xstring_ptr name = value.AsString();

        if (!name.IsNullOrEmpty())
        {
            for (RPNode& node : m_nodes)
            {
                if (name.Equals(node.GetName()))
                {
                    *ppNode = &node;
                    return S_OK;
                }
            }

            // If there is no match within the children, the target might
            // actually be a deferred element. If that's the case, we will
            // create a node for this deferred element and inject it into 
            // the graph.
            auto deferredElement = core->GetDeferredElementIfExists(name, namescopeOwner, nameScopeType);

            if (deferredElement && deferredElement->GetParent() == parent)
            {
                *ppNode  = &(*m_nodes.emplace_after(it, deferredElement));
                return S_OK;
            }

            // If there is truly no matching node in the end, then we must 
            // throw an InvalidOperationException. We will fail fast here 
            // and let the CRelativePanel handle the rest.
            m_knownErrorPending = true;
            m_agErrorCode = AG_E_RELATIVEPANEL_NAME_NOT_FOUND;
            m_errorParameter = name;
            IFC_RETURN(E_FAIL);
        }
    }
    else
    {
        CUIElement* valueAsUIElement = checked_cast<CUIElement>(value);

        if (valueAsUIElement)
        {
            for (RPNode& node : m_nodes)
            {
                if (node.GetElement() == valueAsUIElement)
                {
                    *ppNode = &node;
                    return S_OK;
                }
            }

            // If there is no match, we must throw an InvalidOperationException.
            // We will fail fast here and let the CRelativePanel handle the rest.
            m_knownErrorPending = true;
            m_agErrorCode = AG_E_RELATIVEPANEL_REF_NOT_FOUND;
            IFC_RETURN(E_FAIL);
        }
    }

    *ppNode = nullptr;
    return S_OK;
}

void RPGraph::CalculateMeasureRectHorizontally(_In_ RPNode* node, _In_ XSIZEF availableSize, _Out_ float& x, _Out_ float& width)
{
    bool isHorizontallyCenteredFromLeft = false;
    bool isHorizontallyCenteredFromRight = false;

    // The initial values correspond to the entire available space. In
    // other words, the edges of the element are aligned to the edges
    // of the panel by default. We will now constrain each side of this
    // space as necessary.
    x = 0.0f;
    width = availableSize.width;

    // If we have infinite available width, then the Width of the
    // MeasureRect is also infinite; we do not have to constrain it.
    if (availableSize.width != std::numeric_limits<float>::infinity())
    {
        // Constrain the left side of the available space, i.e.
        // a) The child has its left edge aligned with the panel (default),
        // b) The child has its left edge aligned with the left edge of a sibling,
        // or c) The child is positioned to the right of a sibling.
        //
        //  |;;                 |               |                                                   
        //  |;;                 |               |                
        //  |;;                 |:::::::::::::::|                       ;;:::::::::::::;; 
        //  |;;                 |;             ;|       .               ;;             ;;
        //  |;;                 |;             ;|     .;;............   ;;             ;;
        //  |;;                 |;             ;|   .;;;;::::::::::::   ;;             ;;
        //  |;;                 |;             ;|    ':;;::::::::::::   ;;             ;;
        //  |;;                 |;             ;|      ':               ;;             ;;       
        //  |;;                 |:::::::::::::::|                       :::::::::::::::::
        //  |;;                 |               |               
        //  |;;                 |               |
        //  AlignLeftWithPanel  AlignLeftWith   RightOf
        //
        if (!node->IsAlignLeftWithPanel())
        {
            if (node->IsAlignLeftWith())
            {
                RPNode* alignLeftWithNeighbor = node->m_alignLeftWithNode;
                float restrictedHorizontalSpace = alignLeftWithNeighbor->m_arrangeRect.X;

                x = restrictedHorizontalSpace; 
                width -= restrictedHorizontalSpace;                
            }
            else if (node->IsAlignHorizontalCenterWith())
            {
                isHorizontallyCenteredFromLeft = true;
            }
            else if (node->IsRightOf())
            {
                RPNode* rightOfNeighbor = node->m_rightOfNode;
                float restrictedHorizontalSpace = rightOfNeighbor->m_arrangeRect.X + rightOfNeighbor->m_arrangeRect.Width;

                x = restrictedHorizontalSpace;
                width -= restrictedHorizontalSpace;                
            }
        }

        // Constrain the right side of the available space, i.e.
        // a) The child has its right edge aligned with the panel (default),
        // b) The child has its right edge aligned with the right edge of a sibling,
        // or c) The child is positioned to the left of a sibling.
        //  
        //                                          |               |                   ;;|
        //                                          |               |                   ;;|
        //  ;;:::::::::::::;;                       |;:::::::::::::;|                   ;;|
        //  ;;             ;;               .       |;             ;|                   ;;|
        //  ;;             ;;   ............;;.     |;             ;|                   ;;|
        //  ;;             ;;   ::::::::::::;;;;.   |;             ;|                   ;;|
        //  ;;             ;;   ::::::::::::;;:'    |;             ;|                   ;;|
        //  ;;             ;;               :'      |;             ;|                   ;;|
        //  :::::::::::::::::                       |:::::::::::::::|                   ;;|
        //                                          |               |                   ;;|
        //                                          |               |                   ;;|
        //                                          LeftOf          AlignRightWith      AlignRightWithPanel
        //
        if (!node->IsAlignRightWithPanel())
        {
            if (node->IsAlignRightWith())
            {
                RPNode* alignRightWithNeighbor = node->m_alignRightWithNode;

                width -= availableSize.width - (alignRightWithNeighbor->m_arrangeRect.X + alignRightWithNeighbor->m_arrangeRect.Width);
            }
            else if (node->IsAlignHorizontalCenterWith())
            {
                isHorizontallyCenteredFromRight = true;
            }
            else if (node->IsLeftOf())
            {
                RPNode* leftOfNeighbor = node->m_leftOfNode;

                width -= availableSize.width - leftOfNeighbor->m_arrangeRect.X;
            }
        }

        if (isHorizontallyCenteredFromLeft && isHorizontallyCenteredFromRight)
        {
            RPNode* alignHorizontalCenterWithNeighbor = node->m_alignHorizontalCenterWithNode;
            float centerOfNeighbor = alignHorizontalCenterWithNeighbor->m_arrangeRect.X + (alignHorizontalCenterWithNeighbor->m_arrangeRect.Width / 2.0f);
            width = std::min(centerOfNeighbor, availableSize.width - centerOfNeighbor) * 2.0f;
            x = centerOfNeighbor - (width / 2.0f);
        }
    }
}

void RPGraph::CalculateMeasureRectVertically(_In_ RPNode* node, _In_ XSIZEF availableSize, _Out_ float& y, _Out_ float& height)
{
    bool isVerticallyCenteredFromTop = false;
    bool isVerticallyCenteredFromBottom = false;

    // The initial values correspond to the entire available space. In
    // other words, the edges of the element are aligned to the edges
    // of the panel by default. We will now constrain each side of this
    // space as necessary.
    y = 0.0f;
    height = availableSize.height;

    // If we have infinite available height, then the Height of the
    // MeasureRect is also infinite; we do not have to constrain it.
    if (availableSize.height != std::numeric_limits<float>::infinity())
    {
        // Constrain the top of the available space, i.e.
        // a) The child has its top edge aligned with the panel (default),
        // b) The child has its top edge aligned with the top edge of a sibling,
        // or c) The child is positioned to the below a sibling.
        //
        //  ================================== AlignTopWithPanel
        //  ::::::::::::::::::::::::::::::::::
        //
        //
        //
        //  --------;;=============;;--------- AlignTopWith
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //  --------::=============::--------- Below 
        //                  .
        //                .:;:.
        //              .:;;;;;:.
        //                ;;;;;
        //                ;;;;;
        //                ;;;;;
        //                ;;;;;
        //                ;;;;;
        //
        //          ;;:::::::::::::;; 
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          :::::::::::::::::
        //
        if (!node->IsAlignTopWithPanel())
        {
            if (node->IsAlignTopWith())
            {
                RPNode* alignTopWithNeighbor = node->m_alignTopWithNode;
                float restrictedVerticalSpace = alignTopWithNeighbor->m_arrangeRect.Y;

                y = restrictedVerticalSpace;
                height -= restrictedVerticalSpace;                
            }
            else if (node->IsAlignVerticalCenterWith())
            {
                isVerticallyCenteredFromTop = true;
            }
            else if (node->IsBelow())
            {
                RPNode* belowNeighbor = node->m_belowNode;
                float restrictedVerticalSpace = belowNeighbor->m_arrangeRect.Y + belowNeighbor->m_arrangeRect.Height;

                y = restrictedVerticalSpace;
                height -= restrictedVerticalSpace;                
            }
        }

        // Constrain the bottom of the available space, i.e.
        // a) The child has its bottom edge aligned with the panel (default),
        // b) The child has its bottom edge aligned with the bottom edge of a sibling,
        // or c) The child is positioned to the above a sibling.
        //
        //          ;;:::::::::::::;; 
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          :::::::::::::::::
        //
        //                ;;;;;
        //                ;;;;;
        //                ;;;;;
        //                ;;;;;
        //                ;;;;;
        //              ..;;;;;..
        //               ':::::'
        //                 ':`
        //                  
        //  --------;;=============;;--------- Above 
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //          ;;             ;;
        //  --------::=============::--------- AlignBottomWith
        //
        // 
        //
        //  ::::::::::::::::::::::::::::::::::
        //  ================================== AlignBottomWithPanel
        //
        if (!node->IsAlignBottomWithPanel())
        {
            if (node->IsAlignBottomWith())
            {
                RPNode* alignBottomWithNeighbor = node->m_alignBottomWithNode;

                height -= availableSize.height - (alignBottomWithNeighbor->m_arrangeRect.Y + alignBottomWithNeighbor->m_arrangeRect.Height);
            }
            else if (node->IsAlignVerticalCenterWith())
            {
                isVerticallyCenteredFromBottom = true;
            }
            else if (node->IsAbove())
            {
                RPNode* aboveNeighbor = node->m_aboveNode;

                height -= availableSize.height - aboveNeighbor->m_arrangeRect.Y;
            }
        }

        if (isVerticallyCenteredFromTop && isVerticallyCenteredFromBottom)
        {
            RPNode* alignVerticalCenterWithNeighbor = node->m_alignVerticalCenterWithNode;
            float centerOfNeighbor = alignVerticalCenterWithNeighbor->m_arrangeRect.Y + (alignVerticalCenterWithNeighbor->m_arrangeRect.Height / 2.0f);
            height = std::min(centerOfNeighbor, availableSize.height - centerOfNeighbor) * 2.0f;
            y = centerOfNeighbor - (height / 2.0f);
        }
    }
}

void RPGraph::CalculateArrangeRectHorizontally(_In_ RPNode* node, _Out_ float& x, _Out_ float& width)
{
    XRECTF measureRect = node->m_measureRect;
    float desiredWidth = std::min(measureRect.Width, node->GetDesiredWidth());

    ASSERT(node->IsMeasured() && (measureRect.Width != std::numeric_limits<float>::infinity()));

    // The initial values correspond to the left corner, using the 
    // desired size of element. If no attached properties were set, 
    // this means that the element will default to the left corner of
    // the panel.
    x = measureRect.X;
    width = desiredWidth;

    if (node->IsLeftAnchored())
    {
        if (node->IsRightAnchored())
        {
            x = measureRect.X;
            width = measureRect.Width;
        }
        else
        {
            x = measureRect.X;
            width = desiredWidth;
        }
    }
    else if (node->IsRightAnchored())
    {
        x = measureRect.X + measureRect.Width - desiredWidth;
        width = desiredWidth;
    }
    else if (node->IsHorizontalCenterAnchored())
    {
        x = measureRect.X + (measureRect.Width / 2.0f) - (desiredWidth / 2.0f);
        width = desiredWidth;
    }
}

void RPGraph::CalculateArrangeRectVertically(_In_ RPNode* node, _Out_ float& y, _Out_ float& height)
{
    XRECTF measureRect = node->m_measureRect;
    float desiredHeight = std::min(measureRect.Height, node->GetDesiredHeight());

    ASSERT(node->IsMeasured() && (measureRect.Height != std::numeric_limits<float>::infinity()));

    // The initial values correspond to the top corner, using the 
    // desired size of element. If no attached properties were set, 
    // this means that the element will default to the top corner of
    // the panel.
    y = measureRect.Y;
    height = desiredHeight;

    if (node->IsTopAnchored())
    {
        if (node->IsBottomAnchored())
        {
            y = measureRect.Y;
            height = measureRect.Height;
        }
        else
        {
            y = measureRect.Y;
            height = desiredHeight;
        }
    }
    else if (node->IsBottomAnchored())
    {
        y = measureRect.Y + measureRect.Height - desiredHeight;
        height = desiredHeight;
    }
    else if (node->IsVerticalCenterAnchored())
    {
        y = measureRect.Y + (measureRect.Height / 2.0f) - (desiredHeight / 2.0f);
        height = desiredHeight;
    }
}

void RPGraph::MarkHorizontalAndVerticalLeaves()
{
    for (RPNode &node : m_nodes)
    {
        node.m_isHorizontalLeaf = true;
        node.m_isVerticalLeaf = true;
    }

    for (RPNode &node : m_nodes)
    {
        node.UnmarkNeighborsAsHorizontalOrVerticalLeaves();
    }
}

void RPGraph::AccumulatePositiveDesiredWidth(_In_ RPNode* node, _In_ float x)
{
    float initialX = x;
    bool isHorizontallyCenteredFromLeft = false;
    bool isHorizontallyCenteredFromRight = false;

    ASSERT(node->IsMeasured());

    // If we are going in the positive direction, move the cursor
    // right by the desired width of the node with which we are 
    // currently working and refresh the maximum positive value.
    x += node->GetDesiredWidth();
    m_maxX = std::max(m_maxX, x);

    if (node->IsAlignLeftWithPanel())
    {
        if (!m_isMaxCapped)
        {
            m_maxX = x;
            m_isMaxCapped = true;
        }
    }
    else if (node->IsAlignLeftWith())
    {
        // If the AlignLeftWithNode and AlignRightWithNode are the
        // same element, we can skip the former, since we will move 
        // through the latter later.
        if (node->m_alignLeftWithNode != node->m_alignRightWithNode)
        {
            AccumulateNegativeDesiredWidth(node->m_alignLeftWithNode, x);
        }
    }
    else if (node->IsAlignHorizontalCenterWith())
    {
        isHorizontallyCenteredFromLeft = true;
    }
    else if (node->IsRightOf())
    {
        AccumulatePositiveDesiredWidth(node->m_rightOfNode, x);
    }

    if (node->IsAlignRightWithPanel())
    {
        if (m_isMinCapped)
        {
            m_minX = std::min(m_minX, initialX);
        }
        else
        {
            m_minX = initialX;
            m_isMinCapped = true;
        }
    }
    else if (node->IsAlignRightWith())
    {
        // If this element's right is aligned to some other 
        // element's right, now we will be going in the positive
        // direction to that other element in order to continue the
        // traversal of the dependency chain. But first, since we 
        // arrived to the node where we currently are by going in
        // the positive direction, that means that we have already 
        // moved the cursor right to calculate the maximum positive 
        // value, so we will use the initial value of Y.
        AccumulatePositiveDesiredWidth(node->m_alignRightWithNode, initialX);
    }
    else if (node->IsAlignHorizontalCenterWith())
    {
        isHorizontallyCenteredFromRight = true;
    }
    else if (node->IsLeftOf())
    {
        // If this element is to the left of some other element,
        // now we will be going in the negative direction to that
        // other element in order to continue the traversal of the
        // dependency chain. But first, since we arrived to the
        // node where we currently are by going in the positive
        // direction, that means that we have already moved the 
        // cursor right to calculate the maximum positive value, so
        // we will use the initial value of X.
        AccumulateNegativeDesiredWidth(node->m_leftOfNode, initialX);
    }

    if (isHorizontallyCenteredFromLeft && isHorizontallyCenteredFromRight)
    {
        float centerX = x - (node->GetDesiredWidth() / 2.0f);
        float edgeX = centerX - (node->m_alignHorizontalCenterWithNode->GetDesiredWidth() / 2.0f);
        m_minX = std::min(m_minX, edgeX);
        AccumulatePositiveDesiredWidth(node->m_alignHorizontalCenterWithNode, edgeX);
    }
    else if (node->IsHorizontalCenterAnchored())
    {
        // If this node is horizontally anchored to the center, then it
        // means that it is the root of this dependency chain based on
        // the current definition of precedence for constraints: 
        // e.g. AlignLeftWithPanel 
        // > AlignLeftWith 
        // > RightOf
        // > AlignHorizontalCenterWithPanel    
        // Thus, we can report its width as twice the width of 
        // either the difference from center to left or the difference
        // from center to right, whichever is the greatest.
        float centerX = x - (node->GetDesiredWidth() / 2.0f);
        float upper = m_maxX - centerX;
        float lower = centerX - m_minX;
        m_maxX = std::max(upper, lower) * 2.0f;
        m_minX = 0.0f;
    }
}

void RPGraph::AccumulateNegativeDesiredWidth(_In_ RPNode* node, _In_ float x)
{
    float initialX = x;
    bool isHorizontallyCenteredFromLeft = false;
    bool isHorizontallyCenteredFromRight = false;

    ASSERT(node->IsMeasured());

    // If we are going in the negative direction, move the cursor
    // left by the desired width of the node with which we are 
    // currently working and refresh the minimum negative value.
    x -= node->GetDesiredWidth();
    m_minX = std::min(m_minX, x);

    if (node->IsAlignRightWithPanel())
    {
        if (!m_isMinCapped)
        {
            m_minX = x;
            m_isMinCapped = true;
        }
    }
    else if (node->IsAlignRightWith())
    {
        // If the AlignRightWithNode and AlignLeftWithNode are the
        // same element, we can skip the former, since we will move 
        // through the latter later.
        if (node->m_alignRightWithNode != node->m_alignLeftWithNode)
        {
            AccumulatePositiveDesiredWidth(node->m_alignRightWithNode, x);
        }
    }
    else if (node->IsAlignHorizontalCenterWith())
    {
        isHorizontallyCenteredFromRight = true;
    }
    else if (node->IsLeftOf())
    {
        AccumulateNegativeDesiredWidth(node->m_leftOfNode, x);
    }

    if (node->IsAlignLeftWithPanel())
    {
        if (m_isMaxCapped)
        {
            m_maxX = std::max(m_maxX, initialX);
        }
        else
        {
            m_maxX = initialX;
            m_isMaxCapped = true;
        }
    }
    else if (node->IsAlignLeftWith())
    {
        // If this element's left is aligned to some other element's
        // left, now we will be going in the negative direction to 
        // that other element in order to continue the traversal of
        // the dependency chain. But first, since we arrived to the
        // node where we currently are by going in the negative 
        // direction, that means that we have already moved the 
        // cursor left to calculate the minimum negative value,
        // so we will use the initial value of X.
        AccumulateNegativeDesiredWidth(node->m_alignLeftWithNode, initialX);
    }
    else if (node->IsAlignHorizontalCenterWith())
    {
        isHorizontallyCenteredFromLeft = true;
    }
    else if (node->IsRightOf())
    {
        // If this element is to the right of some other element,
        // now we will be going in the positive direction to that
        // other element in order to continue the traversal of the
        // dependency chain. But first, since we arrived to the
        // node where we currently are by going in the negative
        // direction, that means that we have already moved the 
        // cursor left to calculate the minimum negative value, so
        // we will use the initial value of X.
        AccumulatePositiveDesiredWidth(node->m_rightOfNode, initialX);
    }

    if (isHorizontallyCenteredFromLeft && isHorizontallyCenteredFromRight)
    {
        float centerX = x + (node->GetDesiredWidth() / 2.0f);
        float edgeX = centerX + (node->m_alignHorizontalCenterWithNode->GetDesiredWidth() / 2.0f);
        m_maxX = std::max(m_maxX, edgeX);
        AccumulateNegativeDesiredWidth(node->m_alignHorizontalCenterWithNode, edgeX);
    }
    else if(node->IsHorizontalCenterAnchored())
    {
        // If this node is horizontally anchored to the center, then it
        // means that it is the root of this dependency chain based on
        // the current definition of precedence for constraints: 
        // e.g. AlignLeftWithPanel 
        // > AlignLeftWith 
        // > RightOf
        // > AlignHorizontalCenterWithPanel    
        // Thus, we can report its width as twice the width of 
        // either the difference from center to left or the difference
        // from center to right, whichever is the greatest.
        float centerX = x + (node->GetDesiredWidth() / 2.0f);
        float upper = m_maxX - centerX;
        float lower = centerX - m_minX;
        m_maxX = std::max(upper, lower) * 2.0f;
        m_minX = 0.0f;
    }
}

void RPGraph::AccumulatePositiveDesiredHeight(_In_ RPNode* node, _In_ float y)
{
    float initialY = y;
    bool isVerticallyCenteredFromTop = false;
    bool isVerticallyCenteredFromBottom = false;

    ASSERT(node->IsMeasured());

    // If we are going in the positive direction, move the cursor
    // up by the desired height of the node with which we are 
    // currently working and refresh the maximum positive value.
    y += node->GetDesiredHeight();
    m_maxY = std::max(m_maxY, y);

    if (node->IsAlignTopWithPanel())
    {
        if (!m_isMaxCapped)
        {
            m_maxY = y;
            m_isMaxCapped = true;
        }
    }
    else if (node->IsAlignTopWith())
    {
        // If the AlignTopWithNode and AlignBottomWithNode are the
        // same element, we can skip the former, since we will move 
        // through the latter later.
        if (node->m_alignTopWithNode != node->m_alignBottomWithNode)
        {
            AccumulateNegativeDesiredHeight(node->m_alignTopWithNode, y);
        }
    }
    else if (node->IsAlignVerticalCenterWith())
    {
        isVerticallyCenteredFromTop = true;
    }
    else if (node->IsBelow())
    {
        AccumulatePositiveDesiredHeight(node->m_belowNode, y);
    }

    if (node->IsAlignBottomWithPanel())
    {
        if (m_isMinCapped)
        {
            m_minY = std::min(m_minY, initialY);
        }
        else
        {
            m_minY = initialY;
            m_isMinCapped = true;
        }
    }
    else if (node->IsAlignBottomWith())
    {
        // If this element's bottom is aligned to some other 
        // element's bottom, now we will be going in the positive
        // direction to that other element in order to continue the
        // traversal of the dependency chain. But first, since we 
        // arrived to the node where we currently are by going in
        // the positive direction, that means that we have already 
        // moved the cursor up to calculate the maximum positive 
        // value, so we will use the initial value of Y.
        AccumulatePositiveDesiredHeight(node->m_alignBottomWithNode, initialY);
    }
    else if (node->IsAlignVerticalCenterWith())
    {
        isVerticallyCenteredFromBottom = true;
    }
    else if (node->IsAbove())
    {
        // If this element is above some other element, now we will 
        // be going in the negative direction to that other element
        // in order to continue the traversal of the dependency  
        // chain. But first, since we arrived to the node where we 
        // currently are by going in the positive direction, that
        // means that we have already moved the cursor up to 
        // calculate the maximum positive value, so we will use
        // the initial value of Y.
        AccumulateNegativeDesiredHeight(node->m_aboveNode, initialY);
    }

    if (isVerticallyCenteredFromTop && isVerticallyCenteredFromBottom)
    {
        float centerY = y - (node->GetDesiredHeight() / 2.0f);
        float edgeY = centerY - (node->m_alignVerticalCenterWithNode->GetDesiredHeight() / 2.0f);
        m_minY = std::min(m_minY, edgeY);
        AccumulatePositiveDesiredHeight(node->m_alignVerticalCenterWithNode, edgeY);
    }
    else if (node->IsVerticalCenterAnchored())
    {
        // If this node is vertically anchored to the center, then it
        // means that it is the root of this dependency chain based on
        // the current definition of precedence for constraints: 
        // e.g. AlignTopWithPanel 
        // > AlignTopWith
        // > Below
        // > AlignVerticalCenterWithPanel 
        // Thus, we can report its height as twice the height of 
        // either the difference from center to top or the difference
        // from center to bottom, whichever is the greatest.
        float centerY = y - (node->GetDesiredHeight() / 2.0f);
        float upper = m_maxY - centerY;
        float lower = centerY - m_minY;
        m_maxY = std::max(upper, lower) * 2.0f;
        m_minY = 0.0f;
    }
}

void RPGraph::AccumulateNegativeDesiredHeight(_In_ RPNode* node, _In_ float y)
{
    float initialY = y;
    bool isVerticallyCenteredFromTop = false;
    bool isVerticallyCenteredFromBottom = false;

    ASSERT(node->IsMeasured());

    // If we are going in the negative direction, move the cursor
    // down by the desired height of the node with which we are 
    // currently working and refresh the minimum negative value.
    y -= node->GetDesiredHeight();
    m_minY = std::min(m_minY, y);

    if (node->IsAlignBottomWithPanel())
    {
        if (!m_isMinCapped)
        {
            m_minY = y;
            m_isMinCapped = true;
        }
    }
    else if (node->IsAlignBottomWith())
    {
        // If the AlignBottomWithNode and AlignTopWithNode are the
        // same element, we can skip the former, since we will move 
        // through the latter later.
        if (node->m_alignBottomWithNode != node->m_alignTopWithNode)
        {
            AccumulatePositiveDesiredHeight(node->m_alignBottomWithNode, y);
        }
    }
    else if (node->IsAlignVerticalCenterWith())
    {
        isVerticallyCenteredFromBottom = true;
    }
    else if (node->IsAbove())
    {
        AccumulateNegativeDesiredHeight(node->m_aboveNode, y);
    }

    if (node->IsAlignTopWithPanel())
    {
        if (m_isMaxCapped)
        {
            m_maxY = std::max(m_maxY, initialY);
        }
        else
        {
            m_maxY = initialY;
            m_isMaxCapped = true;
        }
    }
    else if (node->IsAlignTopWith())
    {
        // If this element's top is aligned to some other element's
        // top, now we will be going in the negative direction to 
        // that other element in order to continue the traversal of
        // the dependency chain. But first, since we arrived to the
        // node where we currently are by going in the negative 
        // direction, that means that we have already moved the 
        // cursor down to calculate the minimum negative value,
        // so we will use the initial value of Y.
        AccumulateNegativeDesiredHeight(node->m_alignTopWithNode, initialY);
    }
    else if (node->IsAlignVerticalCenterWith())
    {
        isVerticallyCenteredFromTop = true;
    }
    else if (node->IsBelow())
    {
        // If this element is below some other element, now we'll
        // be going in the positive direction to that other element  
        // in order to continue the traversal of the dependency
        // chain. But first, since we arrived to the node where we
        // currently are by going in the negative direction, that
        // means that we have already moved the cursor down to
        // calculate the minimum negative value, so we will use
        // the initial value of Y.
        AccumulatePositiveDesiredHeight(node->m_belowNode, initialY);
    }

    if (isVerticallyCenteredFromTop && isVerticallyCenteredFromBottom)
    {
        float centerY = y + (node->GetDesiredHeight() / 2.0f);
        float edgeY = centerY + (node->m_alignVerticalCenterWithNode->GetDesiredHeight() / 2.0f);
        m_maxY = std::max(m_maxY, edgeY);
        AccumulateNegativeDesiredHeight(node->m_alignVerticalCenterWithNode, edgeY);
    }
    else if(node->IsVerticalCenterAnchored())
    {
        // If this node is vertically anchored to the center, then it
        // means that it is the root of this dependency chain based on
        // the current definition of precedence for constraints: 
        // e.g. AlignTopWithPanel 
        // > AlignTopWith
        // > Below
        // > AlignVerticalCenterWithPanel 
        // Thus, we can report its height as twice the height of 
        // either the difference from center to top or the difference
        // from center to bottom, whichever is the greatest.
        float centerY = y + (node->GetDesiredHeight() / 2.0f);
        float upper = m_maxY - centerY;
        float lower = centerY - m_minY;
        m_maxY = std::max(upper, lower) * 2.0f;
        m_minY = 0.0f;
    }
}

_Check_return_ HRESULT RPGraph::MeasureNode(_In_opt_ RPNode* node, _In_ XSIZEF availableSize)
{
    if (node == nullptr)
    {
        return S_OK;
    }

    if (node->IsPending())
    {
        // If the node is already in the process of being resolved
        // but we tried to resolve it again, that means we are in the
        // middle of circular dependency and we must throw an 
        // InvalidOperationException. We will fail fast here and let
        // the CRelativePanel handle the rest.
        m_knownErrorPending = true;
        m_agErrorCode = AG_E_RELATIVEPANEL_CIRCULAR_DEP;
        IFC_RETURN(E_FAIL);
    }
    else if (node->IsUnresolved())
    {
        XSIZEF constrainedAvailableSize;

        // We must resolve the dependencies of this node first.
        // In the meantime, we will mark the state as pending.
        node->SetPending(true);

        IFC_RETURN(MeasureNode(node->m_leftOfNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_aboveNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_rightOfNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_belowNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_alignLeftWithNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_alignTopWithNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_alignRightWithNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_alignBottomWithNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_alignHorizontalCenterWithNode, availableSize));
        IFC_RETURN(MeasureNode(node->m_alignVerticalCenterWithNode, availableSize));

        node->SetPending(false);

        CalculateMeasureRectHorizontally(node, availableSize, node->m_measureRect.X, node->m_measureRect.Width);
        CalculateMeasureRectVertically(node, availableSize, node->m_measureRect.Y, node->m_measureRect.Height);

        constrainedAvailableSize.width = std::max(node->m_measureRect.Width, 0.0f);
        constrainedAvailableSize.height = std::max(node->m_measureRect.Height, 0.0f);
        IFC_RETURN(node->Measure(constrainedAvailableSize));
        node->SetMeasured(true);

        // (Pseudo-) Arranging against infinity does not make sense, so 
        // we will skip the calculations of the ArrangeRects if 
        // necessary. During the true arrange pass, we will be given a
        // non-infinite final size; we will do the necessary
        // calculations until then.
        if (availableSize.width != std::numeric_limits<float>::infinity())
        {
            CalculateArrangeRectHorizontally(node, node->m_arrangeRect.X, node->m_arrangeRect.Width);
            node->SetArrangedHorizontally(true);
        }

        if (availableSize.height != std::numeric_limits<float>::infinity())
        {
            CalculateArrangeRectVertically(node, node->m_arrangeRect.Y, node->m_arrangeRect.Height);
            node->SetArrangedVertically(true);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT RPGraph::ArrangeNodeHorizontally(_In_opt_ RPNode* node, _In_ XSIZEF finalSize)
{
    if (node == nullptr)
    {
        return S_OK;
    }

    if (!node->IsArrangedHorizontally())
    {
        // We must resolve dependencies first.
        IFC_RETURN(ArrangeNodeHorizontally(node->m_leftOfNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_aboveNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_rightOfNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_belowNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_alignLeftWithNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_alignTopWithNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_alignRightWithNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_alignBottomWithNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_alignHorizontalCenterWithNode, finalSize));
        IFC_RETURN(ArrangeNodeHorizontally(node->m_alignVerticalCenterWithNode, finalSize));

        CalculateMeasureRectHorizontally(node, finalSize, node->m_measureRect.X, node->m_measureRect.Width);
        CalculateArrangeRectHorizontally(node, node->m_arrangeRect.X, node->m_arrangeRect.Width);

        node->SetArrangedHorizontally(true);
    }

    return S_OK;
}

_Check_return_ HRESULT RPGraph::ArrangeNodeVertically(_In_opt_ RPNode* node, _In_ XSIZEF finalSize)
{
    if (node == nullptr)
    {
        return S_OK;
    }

    if (!node->IsArrangedVertically())
    {
        // We must resolve dependencies first.
        IFC_RETURN(ArrangeNodeVertically(node->m_leftOfNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_aboveNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_rightOfNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_belowNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_alignLeftWithNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_alignTopWithNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_alignRightWithNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_alignBottomWithNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_alignHorizontalCenterWithNode, finalSize));
        IFC_RETURN(ArrangeNodeVertically(node->m_alignVerticalCenterWithNode, finalSize));

        CalculateMeasureRectVertically(node, finalSize, node->m_measureRect.Y, node->m_measureRect.Height);
        CalculateArrangeRectVertically(node, node->m_arrangeRect.Y, node->m_arrangeRect.Height);

        node->SetArrangedVertically(true);
    }

    return S_OK;
}
