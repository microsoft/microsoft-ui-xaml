// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenterTestHooksInteractionSourcesChangedEventArgs.g.h"

class ScrollPresenterTestHooksInteractionSourcesChangedEventArgs :
    public winrt::implementation::ScrollPresenterTestHooksInteractionSourcesChangedEventArgsT<ScrollPresenterTestHooksInteractionSourcesChangedEventArgs>
{
public:
    ScrollPresenterTestHooksInteractionSourcesChangedEventArgs(const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);

    // IScrollPresenterTestHooksInteractionSourcesChangedEventArgs overrides
    winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection InteractionSources();
        
private:
    winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection m_interactionSources{ nullptr };
};
