// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>

class CDependencyObject;
class CValue;

enum class RPConstraints
{
    None                            = 0x00000,
    LeftOf                          = 0x00001,
    Above                           = 0x00002,
    RightOf                         = 0x00004,
    Below                           = 0x00008,
    AlignHorizontalCenterWith       = 0x00010,
    AlignVerticalCenterWith         = 0x00020,
    AlignLeftWith                   = 0x00040,
    AlignTopWith                    = 0x00080,
    AlignRightWith                  = 0x00100,
    AlignBottomWith                 = 0x00200,
    AlignLeftWithPanel              = 0x00400,
    AlignTopWithPanel               = 0x00800,
    AlignRightWithPanel             = 0x01000,
    AlignBottomWithPanel            = 0x02000,
    AlignHorizontalCenterWithPanel  = 0x04000,
    AlignVerticalCenterWithPanel    = 0x08000
};
DEFINE_ENUM_FLAG_OPERATORS(RPConstraints);

enum class RPState
{
    Unresolved              = 0x00,
    Pending                 = 0x01,
    Measured                = 0x02,
    ArrangedHorizontally    = 0x04,
    ArrangedVertically      = 0x08,
    Arranged                = 0x0C
};
DEFINE_ENUM_FLAG_OPERATORS(RPState);

class RPNode
{
public:  
    RPNode(CDependencyObject* element)
        : m_element(element)
        , m_state(RPState::Unresolved)
        , m_isHorizontalLeaf(true)
        , m_isVerticalLeaf(true)
        , m_constraints(RPConstraints::None)
        , m_leftOfNode(nullptr)
        , m_aboveNode(nullptr)
        , m_rightOfNode(nullptr)
        , m_belowNode(nullptr)
        , m_alignHorizontalCenterWithNode(nullptr)
        , m_alignVerticalCenterWithNode(nullptr)
        , m_alignLeftWithNode(nullptr)
        , m_alignTopWithNode(nullptr)
        , m_alignRightWithNode(nullptr)
        , m_alignBottomWithNode(nullptr)
    { }

    CDependencyObject* GetElement() const;
    xstring_ptr GetName() const;
    float GetDesiredWidth() const;
    float GetDesiredHeight() const;

    _Check_return_ HRESULT Measure(_In_ XSIZEF constrainedAvailableSize);
    _Check_return_ HRESULT Arrange(_In_ XRECTF finalRect);

    void GetLeftOfValue(_Out_ CValue* pValue) const;
    void GetAboveValue(_Out_ CValue* pValue) const;
    void GetRightOfValue(_Out_ CValue* pValue) const;
    void GetBelowValue(_Out_ CValue* pValue) const;
    void GetAlignHorizontalCenterWithValue(_Out_ CValue* pValue) const;
    void GetAlignVerticalCenterWithValue(_Out_ CValue* pValue) const;
    void GetAlignLeftWithValue(_Out_ CValue* pValue) const;
    void GetAlignTopWithValue(_Out_ CValue* pValue) const;
    void GetAlignRightWithValue(_Out_ CValue* pValue) const;
    void GetAlignBottomWithValue(_Out_ CValue* pValue) const;
    void GetAlignLeftWithPanelValue(_Out_ CValue* pValue) const;
    void GetAlignTopWithPanelValue(_Out_ CValue* pValue) const;
    void GetAlignRightWithPanelValue(_Out_ CValue* pValue) const;
    void GetAlignBottomWithPanelValue(_Out_ CValue* pValue) const;
    void GetAlignHorizontalCenterWithPanelValue(_Out_ CValue* pValue) const;
    void GetAlignVerticalCenterWithPanelValue(_Out_ CValue* pValue) const;

    // The node is said to be anchored when its ArrangeRect is expected to
    // align with its MeasureRect on one or more sides. For example, if the 
    // node is right-anchored, the right side of the ArrangeRect should overlap
    // with the right side of the MeasureRect. Anchoring is determined by
    // specific combinations of dependencies.
    bool IsLeftAnchored();
    bool IsTopAnchored();
    bool IsRightAnchored();
    bool IsBottomAnchored();
    bool IsHorizontalCenterAnchored();
    bool IsVerticalCenterAnchored();

    // RPState flag checks.
    bool IsUnresolved() const           { return m_state == RPState::Unresolved; }
    bool IsPending() const              { return (m_state & RPState::Pending) == RPState::Pending; }
    bool IsMeasured() const             { return (m_state & RPState::Measured) == RPState::Measured; }
    bool IsArrangedHorizontally() const { return (m_state & RPState::ArrangedHorizontally) == RPState::ArrangedHorizontally; }
    bool IsArrangedVertically() const   { return (m_state & RPState::ArrangedVertically) == RPState::ArrangedVertically; }
    bool IsArranged() const             { return (m_state & RPState::Arranged) == RPState::Arranged; }

    void SetPending(bool value);
    void SetMeasured(bool value);
    void SetArrangedHorizontally(bool value);
    void SetArrangedVertically(bool value);

    // RPEdge flag checks.
    bool IsLeftOf() const                       { return (m_constraints & RPConstraints::LeftOf) == RPConstraints::LeftOf; }
    bool IsAbove() const                        { return (m_constraints & RPConstraints::Above) == RPConstraints::Above; }
    bool IsRightOf() const                      { return (m_constraints & RPConstraints::RightOf) == RPConstraints::RightOf; }
    bool IsBelow() const                        { return (m_constraints & RPConstraints::Below) == RPConstraints::Below; }
    bool IsAlignHorizontalCenterWith() const    { return (m_constraints & RPConstraints::AlignHorizontalCenterWith) == RPConstraints::AlignHorizontalCenterWith; }
    bool IsAlignVerticalCenterWith() const      { return (m_constraints & RPConstraints::AlignVerticalCenterWith) == RPConstraints::AlignVerticalCenterWith; }
    bool IsAlignLeftWith() const                { return (m_constraints & RPConstraints::AlignLeftWith) == RPConstraints::AlignLeftWith; }
    bool IsAlignTopWith() const                 { return (m_constraints & RPConstraints::AlignTopWith) == RPConstraints::AlignTopWith; }
    bool IsAlignRightWith() const               { return (m_constraints & RPConstraints::AlignRightWith) == RPConstraints::AlignRightWith; }
    bool IsAlignBottomWith() const              { return (m_constraints & RPConstraints::AlignBottomWith) == RPConstraints::AlignBottomWith; }
    bool IsAlignLeftWithPanel() const               { return (m_constraints & RPConstraints::AlignLeftWithPanel) == RPConstraints::AlignLeftWithPanel; }
    bool IsAlignTopWithPanel() const                { return (m_constraints & RPConstraints::AlignTopWithPanel) == RPConstraints::AlignTopWithPanel; }
    bool IsAlignRightWithPanel() const              { return (m_constraints & RPConstraints::AlignRightWithPanel) == RPConstraints::AlignRightWithPanel; }
    bool IsAlignBottomWithPanel() const             { return (m_constraints & RPConstraints::AlignBottomWithPanel) == RPConstraints::AlignBottomWithPanel; }
    bool IsAlignHorizontalCenterWithPanel() const    { return (m_constraints & RPConstraints::AlignHorizontalCenterWithPanel) == RPConstraints::AlignHorizontalCenterWithPanel; }
    bool IsAlignVerticalCenterWithPanel() const      { return (m_constraints & RPConstraints::AlignVerticalCenterWithPanel) == RPConstraints::AlignVerticalCenterWithPanel; }

    void SetLeftOfConstraint(_In_opt_ RPNode* neighbor);
    void SetAboveConstraint(_In_opt_ RPNode* neighbor);
    void SetRightOfConstraint(_In_opt_ RPNode* neighbor);
    void SetBelowConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignHorizontalCenterWithConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignVerticalCenterWithConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignLeftWithConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignTopWithConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignRightWithConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignBottomWithConstraint(_In_opt_ RPNode* neighbor);
    void SetAlignLeftWithPanelConstraint(_In_ bool value);
    void SetAlignTopWithPanelConstraint(_In_ bool value);
    void SetAlignRightWithPanelConstraint(_In_ bool value);
    void SetAlignBottomWithPanelConstraint(_In_ bool value);
    void SetAlignHorizontalCenterWithPanelConstraint(_In_ bool value);
    void SetAlignVerticalCenterWithPanelConstraint(_In_ bool value);

    void UnmarkNeighborsAsHorizontalOrVerticalLeaves();

    // Represents the space that's available to an element given its set of
    // constraints. The width and height of this rect is used to measure
    // a given element.
    XRECTF m_measureRect;

    // Represents the exact space within the MeasureRect that will be used 
    // to arrange a given element.
    XRECTF m_arrangeRect;

    RPState m_state;

    // Indicates if this is the last element in a dependency chain that is
    // formed by only connecting nodes horizontally.
    bool m_isHorizontalLeaf;

    // Indicates if this is the last element in a dependency chain that is
    // formed by only connecting nodes vertically.
    bool m_isVerticalLeaf;

    RPConstraints m_constraints;
    RPNode* m_leftOfNode;
    RPNode* m_aboveNode;
    RPNode* m_rightOfNode;
    RPNode* m_belowNode;
    RPNode* m_alignHorizontalCenterWithNode;
    RPNode* m_alignVerticalCenterWithNode;
    RPNode* m_alignLeftWithNode;
    RPNode* m_alignTopWithNode;
    RPNode* m_alignRightWithNode;
    RPNode* m_alignBottomWithNode;

private:
    void GetPropertyValue(_In_ KnownPropertyIndex propertyIndex, _Out_ CValue* value) const;

    CDependencyObject* m_element;
};
