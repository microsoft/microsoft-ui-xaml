﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenterTestHooksInteractionSourcesChangedEventArgs.g.h"

class ScrollPresenterTestHooksInteractionSourcesChangedEventArgs :
    public winrt::implementation::ScrollPresenterTestHooksInteractionSourcesChangedEventArgsT<ScrollPresenterTestHooksInteractionSourcesChangedEventArgs>
{
public:
    ScrollPresenterTestHooksInteractionSourcesChangedEventArgs(const winrt::Microsoft::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources);

    // IScrollPresenterTestHooksInteractionSourcesChangedEventArgs overrides
    winrt::Microsoft::UI::Composition::Interactions::CompositionInteractionSourceCollection InteractionSources();
        
private:
    winrt::Microsoft::UI::Composition::Interactions::CompositionInteractionSourceCollection m_interactionSources{ nullptr };
};
