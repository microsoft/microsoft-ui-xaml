// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SpectrumBrush.h"
#include "Vector.h"

void SpectrumBrush::OnConnected()
{
    m_isConnected = true;
    UpdateSpectrumBrush();
}

void SpectrumBrush::OnDisconnected()
{
    m_isConnected = false;

    if (m_brush)
    {
        m_brush.Close();
        m_brush = nullptr;
        CompositionBrush(nullptr);
    }
}

void SpectrumBrush::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    winrt::Compositor compositor = winrt::Window::Current().Compositor();

    if (property == s_MinSurfaceProperty)
    {
        m_minSurfaceBrush = compositor.CreateSurfaceBrush(MinSurface());
        UpdateSpectrumBrush();
    }
    else if (property == s_MaxSurfaceProperty)
    {
        m_maxSurfaceBrush = compositor.CreateSurfaceBrush(MaxSurface());
        UpdateSpectrumBrush();
    }
    else if (property == s_MaxSurfaceOpacityProperty)
    {
        if (m_brushEffect)
        {
            m_brushEffect->Weight(static_cast<float>(MaxSurfaceOpacity()));
        }
    }
}

void SpectrumBrush::CreateSpectrumBrush()
{
    winrt::Compositor compositor = winrt::Window::Current().Compositor();

    m_brushEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CrossFadeEffect>();
    m_brushEffect->Source1(winrt::CompositionEffectSourceParameter{ L"MinSurface" });
    m_brushEffect->Source2(winrt::CompositionEffectSourceParameter{ L"MaxSurface" });
    m_brushEffect->Weight(static_cast<float>(MaxSurfaceOpacity()));

    winrt::CompositionEffectFactory effectFactory = compositor.CreateEffectFactory(*m_brushEffect);
    m_brush = effectFactory.CreateBrush();

    CompositionBrush(m_brush);
}

void SpectrumBrush::UpdateSpectrumBrush()
{
    if (m_isConnected)
    {
        if (!m_brush)
        {
            CreateSpectrumBrush();
        }

        m_brush.SetSourceParameter(L"MinSurface", m_minSurfaceBrush);
        m_brush.SetSourceParameter(L"MaxSurface", m_maxSurfaceBrush);
    }
}
