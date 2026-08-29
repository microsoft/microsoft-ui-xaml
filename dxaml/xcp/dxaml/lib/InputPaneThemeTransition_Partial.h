// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "InputPaneThemeTransition.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(InputPaneThemeTransition)
    {

    public:
        InputPaneThemeTransition()
            : m_isInputPaneTransitionEnabled(FALSE)
            , m_isInputPaneShow(FALSE)
        {
        }

        _Check_return_ HRESULT CreateStoryboardImpl(_In_ xaml::IUIElement* element, _In_ wf::Rect start, _In_ wf::Rect destination, _In_ xaml::TransitionTrigger transitionTrigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards, _Out_ xaml::TransitionParent* parentForTransition) override;
        _Check_return_ HRESULT ParticipatesInTransitionImpl(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _Out_ BOOLEAN* returnValue) override;

        BOOLEAN IsInputPaneShow()
        {
            return m_isInputPaneShow;
        }

        BOOLEAN IsInputPaneTransitionEnabled()
        {
            return m_isInputPaneTransitionEnabled;
        }

        void SetInputPaneState(BOOLEAN isInputPaneShow)
        {
            m_isInputPaneShow = isInputPaneShow;
        }

        void SetInputPaneTransitionState(BOOLEAN isInputPaneTransitionEnabled)
        {
            m_isInputPaneTransitionEnabled = isInputPaneTransitionEnabled;
        }

    private:
        BOOLEAN m_isInputPaneTransitionEnabled;
        BOOLEAN m_isInputPaneShow;
    };
}
