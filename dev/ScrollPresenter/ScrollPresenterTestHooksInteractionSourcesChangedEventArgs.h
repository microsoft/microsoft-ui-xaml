// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerTestHooksInteractionSourcesChangedEventArgs.g.h"

class ScrollerTestHooksInteractionSourcesChangedEventArgs :
    public winrt::implementation::ScrollerTestHooksInteractionSourcesChangedEventArgsT<ScrollerTestHooksInteractionSourcesChangedEventArgs>
{
public:
    ScrollerTestHooksInteractionSourcesChangedEventArgs(const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);

    // IScrollerTestHooksInteractionSourcesChangedEventArgs overrides
    winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection InteractionSources();
        
private:
    winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection m_interactionSources{ nullptr };
};
