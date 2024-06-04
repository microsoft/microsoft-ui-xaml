// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <forward_list>
#include <namescope\inc\NameScopeRoot.h>

class CDependencyObject;
class CUIElement;
class CValue;
class CCoreServices;
class RPNode;

class RPGraph
{
public:
    RPGraph()
        : m_minX(0.0f)
        , m_maxX(0.0f)
        , m_minY(0.0f)
        , m_maxY(0.0f)
        , m_isMinCapped(false)
        , m_isMaxCapped(false)
        , m_knownErrorPending(false)
        , m_agErrorCode(0)
    { }

    std::forward_list<RPNode>& GetNodes() { return m_nodes; }

    _Check_return_ HRESULT ResolveConstraints(
        _In_ CDependencyObject* parent,
        _In_ CCoreServices* core, 
        _In_ CDependencyObject* namescopeOwner, 
        _In_ Jupiter::NameScoping::NameScopeType nameScopeType);

    _Check_return_ HRESULT MeasureNodes(XSIZEF availableSize);

    _Check_return_ HRESULT ArrangeNodes(XRECTF finalRect);

    // Starting at zero, we will traverse all the horizontal and vertical 
    // chains in the graph one by one while moving a "cursor" up and down 
    // as we go in jumps that are equivalent to the size of the element 
    // associated to the node that we are currently visiting. Everytime
    // that the cursor hits a new max or new min value, we cache it. At the
    // end, the difference between the max and the min values corresponds
    // to the desired size of that chain. The size of the biggest chain
    // also corresponds to the desired size of the panel.
    XSIZEF CalculateDesiredSize();

    bool m_knownErrorPending;
    int m_agErrorCode;
    xstring_ptr m_errorParameter;

private:
    _Check_return_ HRESULT GetNodeByValue(
        _In_ CValue& value, 
        _In_ CDependencyObject* parent,
        _In_ CCoreServices* core,
        _In_ CDependencyObject* namescopeOwner, 
        _In_ Jupiter::NameScoping::NameScopeType nameScopeType,
        _In_ std::forward_list<RPNode>::iterator it,
        _Outptr_opt_ RPNode** ppNode);

    // Starting off with the space that is available to the entire panel
    // (a.k.a. available size), we will constrain this space little by 
    // little based on the ArrangeRects of the dependencies that the
    // node has. The end result corresponds to the MeasureRect of this node. 
    // Consider the following example: if an element is to the left of a 
    // sibling, that means that the space that is available to this element
    // in particular is now the available size minus the Width of the 
    // ArrangeRect associated with this sibling.
    void CalculateMeasureRectHorizontally(_In_ RPNode* node, _In_ XSIZEF availableSize, _Out_ float& x, _Out_ float& width);
    void CalculateMeasureRectVertically(_In_ RPNode* node, _In_ XSIZEF availableSize, _Out_ float& y, _Out_ float& height);

    // The ArrageRect (a.k.a. layout slot) corresponds to the specific rect 
    // within the MeasureRect that will be given to an element for it to
    // position itself. The exact rect is calculated based on anchoring
    // and, unless anchoring dictates otherwise, the size of the
    // ArrangeRect is equal to the desired size of the element itself. 
    // Consider the following example: if the node is right-anchored, the 
    // right side of the ArrangeRect should overlap with the right side
    // of the MeasureRect; this same logic is applied to the other three
    // sides of the rect.
    void CalculateArrangeRectHorizontally(_In_ RPNode* node, _Out_ float& x, _Out_ float& width);
    void CalculateArrangeRectVertically(_In_ RPNode* node, _Out_ float& y, _Out_ float& height);

    void MarkHorizontalAndVerticalLeaves();

    void AccumulatePositiveDesiredWidth(_In_ RPNode* node, _In_ float x);
    void AccumulateNegativeDesiredWidth(_In_ RPNode* node, _In_ float x);
    void AccumulatePositiveDesiredHeight(_In_ RPNode* node, _In_ float y);
    void AccumulateNegativeDesiredHeight(_In_ RPNode* node, _In_ float y);

    // Calculates the MeasureRect of a node and then calls Measure on the
    // corresponding element by passing the Width and Height of this rect.
    // Given that the calculation of the MeasureRect requires the 
    // ArrangeRects of the dependencies, we call this method recursively on
    // said dependencies first and calculate both rects as we go. In other
    // words, this method is figuratively a combination of a measure pass 
    // plus a pseudo-arrange pass.
    _Check_return_ HRESULT MeasureNode(_In_opt_ RPNode* node, _In_ XSIZEF availableSize);

    // Calculates the X and Width properties of the ArrangeRect of a node
    // as well as the X and Width properties of the MeasureRect (which is
    // necessary in order to calculate the former correctly). Given that 
    // the calculation of the MeasureRect requires the ArrangeRects of the
    // dependencies, we call this method recursively on said dependencies
    // first.
    _Check_return_ HRESULT ArrangeNodeHorizontally(_In_opt_ RPNode* node, _In_ XSIZEF finalSize);

    // Calculates the Y and Height properties of the ArrangeRect of a node
    // as well as the Y and Height properties of the MeasureRect (which is
    // necessary in order to calculate the former correctly).Given that 
    // the calculation of the MeasureRect requires the ArrangeRects of the
    // dependencies, we call this method recursively on said dependencies
    // first.
    _Check_return_ HRESULT ArrangeNodeVertically(_In_opt_ RPNode* node, _In_ XSIZEF finalSize);

    std::forward_list<RPNode> m_nodes;

    XSIZEF m_availableSizeForNodeResolution{};

    float m_minX;
    float m_maxX;
    float m_minY;
    float m_maxY;
    bool m_isMinCapped;
    bool m_isMaxCapped; 
};
