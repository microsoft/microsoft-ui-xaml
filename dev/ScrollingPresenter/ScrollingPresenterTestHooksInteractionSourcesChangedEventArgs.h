// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs.g.h"

class ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs :
    public winrt::implementation::ScrollingPresenterTestHooksInteractionSourcesChangedEventArgsT<ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs>
{
public:
    ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs(const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);

    // IScrollingPresenterTestHooksInteractionSourcesChangedEventArgs overrides
    winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection InteractionSources();
        
private:
    winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection m_interactionSources{ nullptr };
};
