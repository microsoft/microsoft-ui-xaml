// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "BringIntoViewRequestedEventArgs.h"

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_TargetElement(_Outptr_ CUIElement** ppValue)
{
    m_targetElement.CopyTo(ppValue);
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_TargetElement(_In_ CUIElement* pValue)
{
    m_targetElement = pValue;
    // we need a targetElement to bring into view, so we fail if a null value is set.
    return m_targetElement == nullptr ? E_INVALIDARG : S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_TargetRect(_Out_ wf::Rect* pValue)
{
    pValue->X = m_targetRect.X;
    pValue->Y = m_targetRect.Y;
    pValue->Width = m_targetRect.Width;
    pValue->Height = m_targetRect.Height;
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_TargetRect(_In_ wf::Rect value)
{
    m_targetRect.X = value.X;
    m_targetRect.Y = value.Y;
    m_targetRect.Width = value.Width;
    m_targetRect.Height = value.Height;
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_AnimationDesired(_Out_ BOOLEAN* pValue)
{
    *pValue = (m_animationDesired == TRUE);
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_AnimationDesired(_In_ BOOLEAN value)
{
    m_animationDesired = value;
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_ForceIntoView(_Out_ BOOLEAN* pValue)
{
    *pValue = (m_forceIntoView == TRUE);
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_ForceIntoView(_In_ BOOLEAN value)
{
    m_forceIntoView = value;
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_InterruptDuringManipulation(_Out_ BOOLEAN* pValue)
{
    *pValue = (m_interruptDuringManipulation == TRUE);
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_InterruptDuringManipulation(_In_ BOOLEAN value)
{
    m_interruptDuringManipulation = value;
    return S_OK;
}

// HorizontalAlignmentRatio property.
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_HorizontalAlignmentRatio(_Out_ DOUBLE* value)
{
    *value = m_horizontalAlignmentRatio;
    return S_OK;
}
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_HorizontalAlignmentRatio(BOOLEAN value)
{
    m_horizontalAlignmentRatio = value;
    return S_OK;
}

// VerticalAlignmentRatio property.
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_VerticalAlignmentRatio(_Out_ DOUBLE* value)
{
    *value = m_verticalAlignmentRatio;
    return S_OK;
}
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_VerticalAlignmentRatio(BOOLEAN value)
{
    m_verticalAlignmentRatio = value;
    return S_OK;
}

// HorizontalOffset property.
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_HorizontalOffset(_Out_ DOUBLE* value)
{
    *value = m_horizontalOffset;
    return S_OK;
}
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_HorizontalOffset(DOUBLE value)
{
    if (DirectUI::DoubleUtil::IsNaN(value) || DirectUI::DoubleUtil::IsInfinity(value))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    m_horizontalOffset = value;
    return S_OK;
}

// VerticalOffset property.
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::get_VerticalOffset(_Out_ DOUBLE* value)
{
    *value = m_verticalOffset;
    return S_OK;
}
_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::put_VerticalOffset(DOUBLE value)
{
    if (DirectUI::DoubleUtil::IsNaN(value) || DirectUI::DoubleUtil::IsInfinity(value))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    m_verticalOffset = value;
    return S_OK;
}

_Check_return_ HRESULT CBringIntoViewRequestedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateBringIntoViewRequestedEventArgs(this, ppPeer));

    return S_OK;
}