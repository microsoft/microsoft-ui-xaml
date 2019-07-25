// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AcrylicTestApi.h"
#include "AcrylicBrush.h"

winrt::AcrylicBrush AcrylicTestApi::AcrylicBrush()
{
    return m_acrylicBrush;
}

void AcrylicTestApi::AcrylicBrush(winrt::AcrylicBrush const& value)
{
    m_acrylicBrush = value;
}

bool AcrylicTestApi::IsUsingAcrylicBrush()
{
    return winrt::get_self<::AcrylicBrush>(m_acrylicBrush)->m_isUsingAcrylicBrush;
}

winrt::CompositionBrush AcrylicTestApi::CompositionBrush()
{
    return (winrt::get_self<::AcrylicBrush>(m_acrylicBrush))->m_brush;
}

winrt::CompositionBrush AcrylicTestApi::NoiseBrush()
{
    return (winrt::get_self<::AcrylicBrush>(m_acrylicBrush))->m_noiseBrush;
}

void AcrylicTestApi::ForceCreateAcrylicBrush(bool useCrossFadeEffect)
{
    auto acrylicBrush = winrt::get_self<::AcrylicBrush>(m_acrylicBrush);

    acrylicBrush->CreateAcrylicBrush(useCrossFadeEffect, true);
}