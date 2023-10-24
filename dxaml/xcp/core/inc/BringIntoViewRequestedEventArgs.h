// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <DoubleUtil.h>

class CBringIntoViewRequestedEventArgs : public CRoutedEventArgs
{
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBringIntoViewRequestedEventArgs>::Index;
    }

    _Check_return_ HRESULT get_AnimationDesired(_Out_ BOOLEAN* pValue);
    _Check_return_ HRESULT put_AnimationDesired(_In_ BOOLEAN value);
    _Check_return_ HRESULT get_ForceIntoView(_Out_ BOOLEAN* pValue);
    _Check_return_ HRESULT put_ForceIntoView(_In_ BOOLEAN value);
    _Check_return_ HRESULT get_InterruptDuringManipulation(_Out_ BOOLEAN* pValue);
    _Check_return_ HRESULT put_InterruptDuringManipulation(_In_ BOOLEAN value);
    _Check_return_ HRESULT get_TargetElement(_Outptr_result_maybenull_ CUIElement** ppValue);
    _Check_return_ HRESULT put_TargetElement(_In_opt_ CUIElement* pValue);
    _Check_return_ HRESULT get_TargetRect(_Out_ wf::Rect* pValue);
    _Check_return_ HRESULT put_TargetRect(_In_ wf::Rect value);
    _Check_return_ HRESULT get_HorizontalAlignmentRatio(_Out_ DOUBLE* value);
    _Check_return_ HRESULT put_HorizontalAlignmentRatio(BOOLEAN value);
    _Check_return_ HRESULT get_VerticalAlignmentRatio(_Out_ DOUBLE* value);
    _Check_return_ HRESULT put_VerticalAlignmentRatio(BOOLEAN value);
    _Check_return_ HRESULT get_HorizontalOffset(_Out_ DOUBLE* value);
    _Check_return_ HRESULT put_HorizontalOffset(DOUBLE value);
    _Check_return_ HRESULT get_VerticalOffset(_Out_ DOUBLE* value);
    _Check_return_ HRESULT put_VerticalOffset(DOUBLE value);

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    bool m_animationDesired = false;
    bool m_forceIntoView = false;
    bool m_interruptDuringManipulation = false;
    xref_ptr<CUIElement> m_targetElement = NULL;
    XRECTF m_targetRect = {0,0,0,0};
    DOUBLE m_horizontalAlignmentRatio = DirectUI::DoubleUtil::NaN;
    DOUBLE m_verticalAlignmentRatio = DirectUI::DoubleUtil::NaN;
    DOUBLE m_horizontalOffset = 0.0;
    DOUBLE m_verticalOffset = 0.0;
};
