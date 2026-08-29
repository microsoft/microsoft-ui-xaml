// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PopupThemeTransition.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(PopupThemeTransition)
    {

    public:
        _Check_return_ HRESULT CreateStoryboardImpl(_In_ xaml::IUIElement* element, _In_ wf::Rect start, _In_ wf::Rect destination, _In_ xaml::TransitionTrigger transitionTrigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards, _Out_ xaml::TransitionParent* parentForTransition) override;
        _Check_return_ HRESULT ParticipatesInTransitionImpl(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _Out_ BOOLEAN* returnValue) override;

        void SetOverlayElement(_In_ xaml::IFrameworkElement* const overlayElement)
        {
            SetPtrValue(m_tpOverlayElement, overlayElement);
        }

    private:
        _Check_return_ HRESULT CreateReaderboardStoryboards(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards);

        TrackerPtr<xaml::IFrameworkElement> m_tpOverlayElement;
    };
}
