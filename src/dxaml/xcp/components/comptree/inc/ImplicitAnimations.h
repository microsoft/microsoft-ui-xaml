// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <NamespaceAliases.h>
#include <fwd/windows.ui.composition.h>

enum class ImplicitAnimationType
{
    Show,
    Hide
};

enum class EffectiveVisibility
{
    Visible,
    Hidden,
};

struct EffectiveVisibilityInfo
{
    // The effective visibility, independent of isExempt
    EffectiveVisibility vis = EffectiveVisibility::Visible;

    // "Exemption" flag used to override the application of effective visibility changes
    bool isExempt = false;

    bool operator == (const EffectiveVisibilityInfo& rhs) const
    {
        return (vis == rhs.vis) && (isExempt == rhs.isExempt);
    }

    bool operator != (const EffectiveVisibilityInfo& rhs) const
    {
        return (vis != rhs.vis) || (isExempt != rhs.isExempt);
    }
};

struct EffectiveVisibilityTracker
{
    EffectiveVisibilityInfo previous;
    EffectiveVisibilityInfo current;
};

struct ImplicitAnimationInfo
{
    wrl::ComPtr<WUComp::ICompositionAnimationBase> implicitAnimation;   // The implicit animation.  Always non-null.
    wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;           // Tracks animation completion.  Will be non-null only if the animation is playing.

    // The target of the implicit animation.  Will be non-null only if the animation is playing.
    // This is typically a WUC visual inside the comp node of the element being animated, but that comp node can be removed
    // after the animation starts if an ancestor element is culled from the scene with something like Opacity=0, so we save
    // the animation target explicitly.
    wrl::ComPtr<WUComp::ICompositionObject2> animationTarget;

    EventRegistrationToken token{};                                     // Registration token for scopedBatch.  Only valid while the animation is playing.

    bool pendingChange = false;                                         // true iff implicit animation is set to a new value while animation is playing.
    wrl::ComPtr<WUComp::ICompositionAnimationBase> pendingAnimation;    // If pendingChange == true, the animation to update to when current animation finishes.
};

