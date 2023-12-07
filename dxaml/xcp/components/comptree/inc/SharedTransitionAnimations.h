// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NamespaceAliases.h"
#include <optional>
#include <fwd/windows.ui.composition.h>

// Implicit property change transitions create and start WUC animations under the covers. Since WUC animations are templates,
// we'll pool them here. Each time an implicit property change transition starts, we'll get the animation of the appropriate
// type from this pool, configure it, and start it. Updating the template will not affect currently running animation instances.
class SharedTransitionAnimations
{
public:
    WUComp::ICompositionAnimationBase* GetScalarAnimationNoRef(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ WUComp::ICompositor* compositor,
        const wrl::Wrappers::HStringReference& propertyName,
        float scalar,
        const wf::TimeSpan& duration);

    WUComp::ICompositionAnimationBase* GetVector3AnimationNoRef(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ WUComp::ICompositor* compositor,
        const wrl::Wrappers::HStringReference& propertyName,
        const wfn::Vector3& vector3,
        const wrl::Wrappers::HStringReference& initialFrame,
        const wf::TimeSpan& duration);

    WUComp::ICompositionAnimation* GetBrushAnimationNoRef(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ WUComp::ICompositor* compositor,
        const std::optional<wu::Color>& fromColor,
        const wu::Color& toColor,
        const wf::TimeSpan& duration);

    void ReleaseDCompResources()
    {
        m_scalarAnimation.Reset();
        m_vector3Animation.Reset();
        m_brushAnimation.Reset();

        m_scalarEasingFunction.Reset();
        m_vector3EasingFunction.Reset();
        m_brushEasingFunction.Reset();
    }

private:
    void EnsureEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ WUComp::ICompositor* compositor);

    void SetAnimationDuration(
        const wrl::ComPtr<WUComp::IKeyFrameAnimation>& kfa,
        const wf::TimeSpan& duration);

private:
    wrl::ComPtr<WUComp::ICompositionAnimationBase> m_scalarAnimation;
    wrl::ComPtr<WUComp::ICompositionAnimationBase> m_vector3Animation;
    wrl::ComPtr<WUComp::ICompositionAnimation> m_brushAnimation;

    wrl::ComPtr<WUComp::ICompositionEasingFunction> m_scalarEasingFunction;
    wrl::ComPtr<WUComp::ICompositionEasingFunction> m_vector3EasingFunction;
    wrl::ComPtr<WUComp::ICompositionEasingFunction> m_brushEasingFunction;
};
