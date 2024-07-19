// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RPNode.h"

#include <cvalue.h>
#include <DeferredElement.h>
#include <DeferredElementCustomRuntimeData.h>
#include <uielement.h>
#include <XamlProperty.h>

CDependencyObject* RPNode::GetElement() const
{
    return m_element;
}

xstring_ptr RPNode::GetName() const
{
    return m_element->m_strName;
}

float RPNode::GetDesiredWidth() const
{
    if (m_element->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement* element = static_cast<CUIElement*>(m_element);
        return element->GetLayoutStorage()->m_desiredSize.width;
    }
    else
    {
        return 0.0f;
    }
}

float RPNode::GetDesiredHeight() const
{
    if (m_element->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement* element = static_cast<CUIElement*>(m_element);
        return element->GetLayoutStorage()->m_desiredSize.height;
    }
    else
    {
        return 0.0f;
    }
}

_Check_return_ HRESULT RPNode::Measure(_In_ XSIZEF constrainedAvailableSize)
{
    if (m_element->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement* element = static_cast<CUIElement*>(m_element);
        IFC_RETURN(element->Measure(constrainedAvailableSize));
        IFC_RETURN(element->EnsureLayoutStorage());
    }

    return S_OK;
}

_Check_return_ HRESULT RPNode::Arrange(_In_ XRECTF finalRect)
{
    if (m_element->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement* element = static_cast<CUIElement*>(m_element);
        IFC_RETURN(element->Arrange(finalRect));
    }

    return S_OK;
}

void RPNode::GetLeftOfValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_LeftOf, pValue);
}

void RPNode::GetAboveValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_Above, pValue);
}

void RPNode::GetRightOfValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_RightOf, pValue);
}

void RPNode::GetBelowValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_Below, pValue);
}

void RPNode::GetAlignHorizontalCenterWithValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWith, pValue);
}

void RPNode::GetAlignVerticalCenterWithValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignVerticalCenterWith, pValue);
}

void RPNode::GetAlignLeftWithValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignLeftWith, pValue);
}

void RPNode::GetAlignTopWithValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignTopWith, pValue);
}

void RPNode::GetAlignRightWithValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignRightWith, pValue);
}

void RPNode::GetAlignBottomWithValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignBottomWith, pValue);
}

void RPNode::GetAlignLeftWithPanelValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignLeftWithPanel, pValue);
}

void RPNode::GetAlignTopWithPanelValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignTopWithPanel, pValue);
}

void RPNode::GetAlignRightWithPanelValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignRightWithPanel, pValue);
}

void RPNode::GetAlignBottomWithPanelValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignBottomWithPanel, pValue);
}

void RPNode::GetAlignHorizontalCenterWithPanelValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWithPanel, pValue);
}

void RPNode::GetAlignVerticalCenterWithPanelValue(_Out_ CValue* pValue) const
{
    GetPropertyValue(KnownPropertyIndex::RelativePanel_AlignVerticalCenterWithPanel, pValue);
}

bool RPNode::IsLeftAnchored()
{
    return (IsAlignLeftWithPanel() || IsAlignLeftWith() || (IsRightOf() && !IsAlignHorizontalCenterWith()));
}

bool RPNode::IsTopAnchored()
{
    return (IsAlignTopWithPanel() || IsAlignTopWith() || (IsBelow() && !IsAlignVerticalCenterWith()));
}

bool RPNode::IsRightAnchored()
{
    return (IsAlignRightWithPanel() || IsAlignRightWith() || (IsLeftOf() && !IsAlignHorizontalCenterWith()));
}

bool RPNode::IsBottomAnchored()
{
    return (IsAlignBottomWithPanel() || IsAlignBottomWith() || (IsAbove() && !IsAlignVerticalCenterWith()));
}

bool RPNode::IsHorizontalCenterAnchored()
{
    return ((IsAlignHorizontalCenterWithPanel() && !IsAlignLeftWithPanel() && !IsAlignRightWithPanel() && !IsAlignLeftWith() && !IsAlignRightWith() && !IsLeftOf() && !IsRightOf())
        || (IsAlignHorizontalCenterWith() && !IsAlignLeftWithPanel() && !IsAlignRightWithPanel() && !IsAlignLeftWith() && !IsAlignRightWith()));
}

bool RPNode::IsVerticalCenterAnchored()
{
    return ((IsAlignVerticalCenterWithPanel() && !IsAlignTopWithPanel() && !IsAlignBottomWithPanel() && !IsAlignTopWith() && !IsAlignBottomWith() && !IsAbove() && !IsBelow())
        || (IsAlignVerticalCenterWith() && !IsAlignTopWithPanel() && !IsAlignBottomWithPanel() && !IsAlignTopWith() && !IsAlignBottomWith()));
}

void RPNode::SetPending(bool value)
{
    if (value)
    {
        m_state |= RPState::Pending;
    }
    else
    {
        m_state &= ~RPState::Pending;
    }
}

void RPNode::SetMeasured(bool value)
{
    if (value)
    {
        m_state |= RPState::Measured;
    }
    else
    {
        m_state &= ~RPState::Measured;
    }
}

void RPNode::SetArrangedHorizontally(bool value)
{
    if (value)
    {
        m_state |= RPState::ArrangedHorizontally;
    }
    else
    {
        m_state &= ~RPState::ArrangedHorizontally;
    }
}

void RPNode::SetArrangedVertically(bool value)
{
    if (value)
    {
        m_state |= RPState::ArrangedVertically;
    }
    else
    {
        m_state &= ~RPState::ArrangedVertically;
    }
}

void RPNode::SetLeftOfConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_leftOfNode = neighbor;
        m_constraints |= RPConstraints::LeftOf;
    }
    else
    {
        m_leftOfNode = nullptr;
        m_constraints &= ~RPConstraints::LeftOf;
    }
}

void RPNode::SetAboveConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_aboveNode = neighbor;
        m_constraints |= RPConstraints::Above;
    }
    else
    {
        m_aboveNode = nullptr;
        m_constraints &= ~RPConstraints::Above;
    }
}

void RPNode::SetRightOfConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_rightOfNode = neighbor;
        m_constraints |= RPConstraints::RightOf;
    }
    else
    {
        m_rightOfNode = nullptr;
        m_constraints &= ~RPConstraints::RightOf;
    }
}

void RPNode::SetBelowConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_belowNode = neighbor;
        m_constraints |= RPConstraints::Below;
    }
    else
    {
        m_belowNode = nullptr;
        m_constraints &= ~RPConstraints::Below;
    }
}

void RPNode::SetAlignHorizontalCenterWithConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_alignHorizontalCenterWithNode = neighbor;
        m_constraints |= RPConstraints::AlignHorizontalCenterWith;
    }
    else
    {
        m_alignHorizontalCenterWithNode = nullptr;
        m_constraints &= ~RPConstraints::AlignHorizontalCenterWith;
    }
}

void RPNode::SetAlignVerticalCenterWithConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_alignVerticalCenterWithNode = neighbor;
        m_constraints |= RPConstraints::AlignVerticalCenterWith;
    }
    else
    {
        m_alignVerticalCenterWithNode = nullptr;
        m_constraints &= ~RPConstraints::AlignVerticalCenterWith;
    }
}

void RPNode::SetAlignLeftWithConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_alignLeftWithNode = neighbor;
        m_constraints |= RPConstraints::AlignLeftWith;
    }
    else
    {
        m_alignLeftWithNode = nullptr;
        m_constraints &= ~RPConstraints::AlignLeftWith;
    }
}

void RPNode::SetAlignTopWithConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_alignTopWithNode = neighbor;
        m_constraints |= RPConstraints::AlignTopWith;
    }
    else
    {
        m_alignTopWithNode = nullptr;
        m_constraints &= ~RPConstraints::AlignTopWith;
    }
}

void RPNode::SetAlignRightWithConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_alignRightWithNode = neighbor;
        m_constraints |= RPConstraints::AlignRightWith;
    }
    else
    {
        m_alignRightWithNode = nullptr;
        m_constraints &= ~RPConstraints::AlignRightWith;
    }
}

void RPNode::SetAlignBottomWithConstraint(_In_opt_ RPNode* neighbor)
{
    if (neighbor)
    {
        m_alignBottomWithNode = neighbor;
        m_constraints |= RPConstraints::AlignBottomWith;
    }
    else
    {
        m_alignBottomWithNode = nullptr;
        m_constraints &= ~RPConstraints::AlignBottomWith;
    }
}

void RPNode::SetAlignLeftWithPanelConstraint(_In_ bool value)
{
    if (value)
    {
        m_constraints |= RPConstraints::AlignLeftWithPanel;
    }
    else
    {
        m_constraints &= ~RPConstraints::AlignLeftWithPanel;
    }
}

void RPNode::SetAlignTopWithPanelConstraint(_In_ bool value)
{
    if (value)
    {
        m_constraints |= RPConstraints::AlignTopWithPanel;
    }
    else
    {
        m_constraints &= ~RPConstraints::AlignTopWithPanel;
    }
}

void RPNode::SetAlignRightWithPanelConstraint(_In_ bool value)
{
    if (value)
    {
        m_constraints |= RPConstraints::AlignRightWithPanel;
    }
    else
    {
        m_constraints &= ~RPConstraints::AlignRightWithPanel;
    }
}

void RPNode::SetAlignBottomWithPanelConstraint(_In_ bool value)
{
    if (value)
    {
        m_constraints |= RPConstraints::AlignBottomWithPanel;
    }
    else
    {
        m_constraints &= ~RPConstraints::AlignBottomWithPanel;
    }
}

void RPNode::SetAlignHorizontalCenterWithPanelConstraint(_In_ bool value)
{
    if (value)
    {
        m_constraints |= RPConstraints::AlignHorizontalCenterWithPanel;
    }
    else
    {
        m_constraints &= ~RPConstraints::AlignHorizontalCenterWithPanel;
    }
}

void RPNode::SetAlignVerticalCenterWithPanelConstraint(_In_ bool value)
{
    if (value)
    {
        m_constraints |= RPConstraints::AlignVerticalCenterWithPanel;
    }
    else
    {
        m_constraints &= ~RPConstraints::AlignVerticalCenterWithPanel;
    }
}

void RPNode::UnmarkNeighborsAsHorizontalOrVerticalLeaves()
{
    bool isHorizontallyCenteredFromLeft = false;
    bool isHorizontallyCenteredFromRight = false;
    bool isVerticallyCenteredFromTop = false;
    bool isVerticallyCenteredFromBottom = false;

    if (!IsAlignLeftWithPanel())
    {
        if (IsAlignLeftWith())
        {
            m_alignLeftWithNode->m_isHorizontalLeaf = false;
        }
        else if (IsAlignHorizontalCenterWith())
        {
            isHorizontallyCenteredFromLeft = true;
        }
        else if (IsRightOf())
        {
            m_rightOfNode->m_isHorizontalLeaf = false;
        }
    }

    if (!IsAlignTopWithPanel())
    {
        if (IsAlignTopWith())
        {
            m_alignTopWithNode->m_isVerticalLeaf = false;
        }
        else if (IsAlignVerticalCenterWith())
        {
            isVerticallyCenteredFromTop = true;
        }
        else if (IsBelow())
        {
            m_belowNode->m_isVerticalLeaf = false;
        }
    }

    if (!IsAlignRightWithPanel())
    {
        if (IsAlignRightWith())
        {
            m_alignRightWithNode->m_isHorizontalLeaf = false;
        }
        else if (IsAlignHorizontalCenterWith())
        {
            isHorizontallyCenteredFromRight = true;
        }
        else if (IsLeftOf())
        {
            m_leftOfNode->m_isHorizontalLeaf = false;
        }
    }

    if (!IsAlignBottomWithPanel())
    {
        if (IsAlignBottomWith())
        {
            m_alignBottomWithNode->m_isVerticalLeaf = false;
        }
        else if (IsAlignVerticalCenterWith())
        {
            isVerticallyCenteredFromBottom = true;
        }
        else if (IsAbove())
        {
            m_aboveNode->m_isVerticalLeaf = false;
        }
    }

    if (isHorizontallyCenteredFromLeft && isHorizontallyCenteredFromRight)
    {
        m_alignHorizontalCenterWithNode->m_isHorizontalLeaf = false;
    }

    if (isVerticallyCenteredFromTop && isVerticallyCenteredFromBottom)
    {
        m_alignVerticalCenterWithNode->m_isVerticalLeaf = false;
    }
}

void RPNode::GetPropertyValue(
    _In_ KnownPropertyIndex propertyIndex,
    _Out_ CValue* value) const
{
    ASSERT(m_element->OfTypeByIndex<KnownTypeIndex::UIElement>() || m_element->OfTypeByIndex<KnownTypeIndex::DeferredElement>());

    if (m_element->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        IFCFAILFAST(m_element->GetValueByIndex(propertyIndex, value));
    }
    else if (m_element->OfTypeByIndex<KnownTypeIndex::DeferredElement>())
    {
        bool valueSet = false;
        CDeferredElement* element = static_cast<CDeferredElement*>(m_element);
        DeferredElementCustomRuntimeData* runtimeData = element->GetCustomRuntimeData();

        if (runtimeData)
        {
            for (const auto& prop : runtimeData->GetNonDeferredProperties())
            {
                KnownPropertyIndex index = prop.first.get()->get_PropertyToken().GetHandle();

                if (index == propertyIndex)
                {
                    value->CopyConverted(prop.second);
                    valueSet = true;
                    break;
                }
            }
        }

        // If the value was not found, this means that the property
        // was not set in the first place.
        if (!valueSet)
        {
            value->Unset();
        }
    }
}
