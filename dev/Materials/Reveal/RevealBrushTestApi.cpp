// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RevealBrushTestApi.h"
#include "RevealBrush.h"

#include "RevealBrushTestApi.properties.cpp"

winrt::RevealBrush RevealBrushTestApi::RevealBrush()
{
    return m_revealBrush;
}

void RevealBrushTestApi::RevealBrush(winrt::RevealBrush const& value)
{
    m_revealBrush = value;
}

bool RevealBrushTestApi::IsInFallbackMode()
{
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_isInFallbackMode;
}

bool RevealBrushTestApi::IsAmbientLightSet()
{
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_isAmbientLightSet;
}

bool RevealBrushTestApi::IsBorderLightSet()
{
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_isBorderLightSet;
}

bool RevealBrushTestApi::IsHoverLightSet()
{
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_isHoverLightSet;
}

winrt::CompositionBrush RevealBrushTestApi::CompositionBrush()
{
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_brush;
}

winrt::CompositionBrush RevealBrushTestApi::NoiseBrush()
{
#if BUILD_WINDOWS
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_dpiScaledNoiseBrush;
#else
    return (winrt::get_self<::RevealBrush>(m_revealBrush))->m_noiseBrush;
#endif
}
