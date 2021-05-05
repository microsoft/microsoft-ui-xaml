// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BackdropMaterialBrush.h"


thread_local int BackdropMaterialBrush::m_connectedBrushCount{};
thread_local winrt::com_ptr<MicaController> BackdropMaterialBrush::m_micaController{ nullptr };

void BackdropMaterialBrush::CreateOrDestroyMicaController()
{
    // If we are connecting the first BackdropMaterialBrush on this thread, create and configure the MicaController.
    // Or if we're disconnecting the last one, clean up the shared MicaController.
    if (m_connectedBrushCount > 0 && !m_micaController)
    {
        auto currentWindow = winrt::Window::Current();

        m_micaController = winrt::make_self<MicaController>();
        if (!m_micaController->SetTarget(currentWindow))
        {
            m_micaController = nullptr;
        }
    }
    else if (m_connectedBrushCount == 0 && m_micaController)
    {
        m_micaController = nullptr;
    }
}

void BackdropMaterialBrush::OnConnected()
{
    // Track whether we're connected and update the number of connected BackdropMaterialBrush on this thread.
    if (!m_connected)
    {
        m_connected = true;
        m_connectedBrushCount++;
        CreateOrDestroyMicaController();
    }

    m_fallbackColorChangedRevoker = RegisterPropertyChanged(*this, winrt::XamlCompositionBrushBase::FallbackColorProperty(), [this](auto&&...) {
        UpdateFallbackBrush();
        });

    UpdateFallbackBrush();
}

void BackdropMaterialBrush::OnDisconnected()
{
    if (m_connected)
    {
        m_connected = false;
        m_connectedBrushCount--;
        CreateOrDestroyMicaController();
    }

    m_fallbackColorChangedRevoker = {};
}

BackdropMaterialBrush::~BackdropMaterialBrush()
{
    OnDisconnected();
}

void BackdropMaterialBrush::UpdateFallbackBrush()
{
    if (!m_micaController)
    {
        if (!m_fallbackBrush)
        {
            auto compositor = winrt::Window::Current().Compositor();
            m_fallbackBrush = compositor.CreateColorBrush();
        }
        m_fallbackBrush.Color(FallbackColor());
    }
    else
    {
        m_fallbackBrush = nullptr;
    }

    CompositionBrush(m_fallbackBrush);
}
