// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EdgeUIThemeTransition.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(EdgeUIThemeTransition)
    {

    public:
        EdgeUIThemeTransition()
        {
            // a default of 0 means that we will react to all layout triggers
            // and use the default Animation timing
            m_iNextTickToReactTo = 0;
        }

        _Check_return_ HRESULT CreateStoryboardImpl(_In_ xaml::IUIElement* element, _In_ wf::Rect start, _In_ wf::Rect destination, _In_ xaml::TransitionTrigger transitionTrigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards, _Out_ xaml::TransitionParent* parentForTransition) override;
        _Check_return_ HRESULT ParticipatesInTransitionImpl(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _Out_ BOOLEAN* returnValue) override;

        void SetToOnlyReactToTickAndUseIHMTiming(_In_ INT16 tick) { m_iNextTickToReactTo = tick; }

    private:
        // Appbar uses the SetToOnlyReactToTickAndUseIHMTiming method. This is also used here to indicate the appropriate timing.
        _Check_return_ BOOLEAN UsePanelAnimationInsteadOfEdgeAnimation() { return m_iNextTickToReactTo != 0; }

    private:
        // Allows us to use the EdgeUIThemeTransition in a mode where it only reacts to certain ticks
        // Example: ApplicationBar only wants the layout transition part to react if the bar is moved
        // because of a layout change - not when the height is change for instance.
        // This value is being mis-used to also indicate other timings.
        INT16 m_iNextTickToReactTo;
    };
}


