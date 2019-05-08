// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTestHooksInteractionSourcesChangedEventArgs.h"

ScrollerTestHooksInteractionSourcesChangedEventArgs::ScrollerTestHooksInteractionSourcesChangedEventArgs(
    const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources)
{
    m_interactionSources = interactionSources;
}

#pragma region IScrollerTestHooksInteractionSourcesChangedEventArgs

winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection ScrollerTestHooksInteractionSourcesChangedEventArgs::InteractionSources()
{
    return m_interactionSources;
}

#pragma endregion
