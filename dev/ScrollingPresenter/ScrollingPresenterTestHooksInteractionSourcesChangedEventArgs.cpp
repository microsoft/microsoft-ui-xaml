// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollPresenterTestHooksInteractionSourcesChangedEventArgs.h"

ScrollPresenterTestHooksInteractionSourcesChangedEventArgs::ScrollPresenterTestHooksInteractionSourcesChangedEventArgs(
    const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources)
{
    m_interactionSources = interactionSources;
}

#pragma region IScrollPresenterTestHooksInteractionSourcesChangedEventArgs

winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection ScrollPresenterTestHooksInteractionSourcesChangedEventArgs::InteractionSources()
{
    return m_interactionSources;
}

#pragma endregion
