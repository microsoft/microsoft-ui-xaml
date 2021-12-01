// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AcrylicBrushFactory.h"

AcrylicBrushFactory::AcrylicBrushFactory()
{
    AcrylicBrush::EnsureProperties();
}

winrt::CompositionEffectBrush AcrylicBrushFactory::CreateBackdropAcrylicEffectBrush(
    winrt::Compositor const& compositor,
    winrt::Color const& initialTintColor,
    winrt::Color const& initialFallbackColor,
    bool willTintColorAlwaysBeOpaque)
{
    return CreateBackdropAcrylicEffectBrushWithLuminosity(
        compositor,
        initialTintColor,
        nullptr, // luminosityOpacity. Pass nullptr to let it auto-calculate luminosity opacity.
        initialFallbackColor,
        willTintColorAlwaysBeOpaque);
}

winrt::CompositionEffectBrush AcrylicBrushFactory::CreateBackdropAcrylicEffectBrushWithLuminosity(
    winrt::Compositor const& compositor,
    winrt::Color const& initialTintColor,
    winrt::IReference<double> const& luminosityOpacity,
    winrt::Color const& initialFallbackColor,
    bool willTintColorAlwaysBeOpaque)
{
    const winrt::Color initialLuminosityColor = GetLuminosityColor(initialTintColor, luminosityOpacity);
    
    // Update tintColor's alpha with the modifier
    winrt::Color tintColor = initialTintColor;
    const double tintOpacityModifier = GetTintOpacityModifier(tintColor);
    tintColor.A = static_cast<uint8_t>(round(tintOpacityModifier * tintColor.A));

    return AcrylicBrush::CreateAcrylicBrushWorker(
        compositor,
        false, // useWindowAcrylic
        false, // useCrossFadeEffect
        tintColor,
        initialLuminosityColor,
        initialFallbackColor,
        willTintColorAlwaysBeOpaque);
}
