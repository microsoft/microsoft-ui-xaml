// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

// Class that handles the InteractionTracker callbacks. Has a weak reference back to the ScrollPresenter so it can be garbage-collected, 
// since the InteractionTracker keeps a strong reference to this object.
class SwipeControlInteractionTrackerOwner
    : public winrt::implements<SwipeControlInteractionTrackerOwner, winrt::IInteractionTrackerOwner>
{
public:
    SwipeControlInteractionTrackerOwner(const winrt::SwipeControl& swipeControl);
    ~SwipeControlInteractionTrackerOwner();

#pragma region IInteractionTrackerOwner
    void ValuesChanged(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerValuesChangedArgs& args);
    void RequestIgnored(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerRequestIgnoredArgs& args);
    void InteractingStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerInteractingStateEnteredArgs& args);
    void InertiaStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerInertiaStateEnteredArgs& args);
    void IdleStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerIdleStateEnteredArgs& args);
    void CustomAnimationStateEntered(const winrt::InteractionTracker& sender, const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args);
#pragma endregion

private:
    weak_ref<winrt::SwipeControl> m_owner{ nullptr };
};
