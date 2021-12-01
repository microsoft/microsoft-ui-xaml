#pragma once

#include <winrt/Windows.Graphics.Effects.h>

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Private.Composition.Effects_impl.h"
#pragma warning(pop)

namespace SystemBackdropComponentInternal
{
    winrt::Windows::UI::Composition::CompositionBrush BuildMicaEffectBrush(
        const winrt::Windows::UI::Composition::Compositor& compositor,
        winrt::Windows::UI::Color tintColor,
        float tintOpacity,
        float luminosityOpacity);

    winrt::Windows::UI::Composition::CompositionBrush CreateCrossFadeEffectBrush(
        const winrt::Windows::UI::Composition::Compositor& compositor,
        const winrt::Windows::UI::Composition::CompositionBrush& from,
        const winrt::Windows::UI::Composition::CompositionBrush& to);

    winrt::Windows::UI::Composition::ScalarKeyFrameAnimation CreateCrossFadeAnimation(
        const winrt::Windows::UI::Composition::Compositor& compositor);
}
