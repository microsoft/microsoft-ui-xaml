// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// F10 (branch "transitions"): XAML-owned animations in immediate-rendered windows. UIElement Scalar/Vector3 transitions
// and BrushTransition are evaluated by XAML (components\base\ImmediateAnimations.cpp) instead of being started as
// Composition facade animations, so the window doesn't fall back. The walk reads the animated values here. See
// docs/design-notes/immediate-rendering-backends-proposal.md, "Full XAML coverage: transitions".

#include "precomp.h"
#include "ImmediateRenderer.h"
#include <ImmediateAnimations.h>
#include "XamlIslandRoot.h"
#include "VisualTree.h"
#include <UIThreadScheduler.h>

using namespace RenderingBackend;
using namespace ImmediateAnimations;

bool ImmediateRenderer::IsElementImmediateCallback(_In_ const void* element) noexcept
{
    auto uiElement = static_cast<CUIElement*>(const_cast<void*>(element));
    ImmediateRenderer* self = uiElement->GetContext()->m_immediateRenderer;
    if (self == nullptr)
    {
        return true;
    }
    CXamlIslandRoot* island = VisualTree::GetXamlIslandRootForElement(uiElement);
    return island == nullptr || !self->IsFallenBack(island);
}

bool ImmediateRenderer::HasTransitions(_In_ CUIElement* element) noexcept
{
    const TransitionStore& store = GetThreadStore();
    return !store.Empty() && store.HasOwner(element);
}

bool ImmediateRenderer::ApplyTransitions(_In_ CUIElement* element, _Inout_ float* opacity) noexcept
{
    TransitionStore& store = GetThreadStore();
    const int64_t now = NowMicroseconds();
    bool running = false;
    float value[4];
    for (uint32_t channel = 0; channel < static_cast<uint32_t>(Channel::Count); ++channel)
    {
        if (store.TryGetValue(element, static_cast<Channel>(channel), now, value))
        {
            running = true;
            if (static_cast<Channel>(channel) == Channel::Opacity)
            {
                *opacity = value[0];
            }
        }
    }

    if (!running)
    {
        // Finished: the static values apply from now on.
        store.RemoveOwner(element);
        return false;
    }

    float hold;
    if (!TryGetHoldProgress(&hold))
    {
        element->GetContext()->RequestAdditionalFrame(RequestFrameReason::AnimationTick);
    }
    return true;
}

bool ImmediateRenderer::TryGetTransitionColor(_In_ CUIElement* element, _Inout_ ColorF* color) noexcept
{
    const TransitionStore& store = GetThreadStore();
    float argb[4];
    if (store.Empty() || !store.TryGetValue(element, Channel::BackgroundColor, NowMicroseconds(), argb))
    {
        return false;
    }
    color->a = argb[0] / 255.0f;
    color->r = argb[1] / 255.0f;
    color->g = argb[2] / 255.0f;
    color->b = argb[3] / 255.0f;
    return true;
}
