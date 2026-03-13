// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RuntimeProfiler.h"
#include "SystemBackdropElement.h"

SystemBackdropElement::SystemBackdropElement()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SystemBackdropElement);
    Loaded({
        [this](auto const&, auto const&)
        {
            UpdatePlacementVisual();
        }
    });
    Unloaded({
        [this](auto const&, auto const&)
        {
            // Async Loaded / Unloaded events can result in Unloaded after an element is
            // back in Loaded state.
            if (!IsLoaded())
            {
                ReleaseCompositionResources();
            }
        }
    });
}

SystemBackdropElement::~SystemBackdropElement()
{
    ReleaseCompositionResources();
}

void SystemBackdropElement::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (property == s_SystemBackdropProperty)
    {
        if (args.NewValue() != args.OldValue())
        {
            const auto& oldSystemBackdrop = args.OldValue().try_as<winrt::SystemBackdrop>();
            const auto& newSystemBackdrop = args.NewValue().try_as<winrt::SystemBackdrop>();

            if (oldSystemBackdrop)
            {
                if (m_backdropLink)
                {
                    oldSystemBackdrop.OnTargetDisconnected(m_backdropLink);
                }
                m_registeredWithSystemBackdrop = false;
            }

            m_systemBackdrop = newSystemBackdrop;

            if (newSystemBackdrop)
            {
                UpdatePlacementVisual();
            }
            else
            {
                // Clean up composition resources when backdrop is set to null
                if (m_backdropLink)
                {
                    winrt::Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::SetElementChildVisual(*this, nullptr);
                }
                m_backdropLink = nullptr;
                m_clip = nullptr;
            }
        }
    }
    else if (property == s_CornerRadiusProperty)
    {
        if (args.NewValue() == args.OldValue())
        {
            return;
        }

        if (auto newCornerRadius = args.NewValue().try_as<winrt::IReference<winrt::CornerRadius>>())
        {
            m_cornerRadius = newCornerRadius.Value();
        }
        else
        {
            m_cornerRadius = {};
        }

        UpdatePlacementVisualClip();
    }
}

winrt::Size SystemBackdropElement::ArrangeOverride(winrt::Size const& finalSize)
{
    winrt::Size arrangedSize = __super::ArrangeOverride(finalSize);
    
    m_lastArrangedSize = arrangedSize;
    
    UpdatePlacementVisual();
    
    return arrangedSize;
}

void SystemBackdropElement::UpdatePlacementVisual()
{
    if (!IsLoaded())
    {
        return;
    }
    TryConnectSystemBackdrop();
   
    if (m_backdropLink)
    {
        UpdatePlacementVisualSize();
        UpdatePlacementVisualClip();
    }
}

void SystemBackdropElement::UpdatePlacementVisualSize()
{
    if (!m_backdropLink)
    {
        return;
    }
    
    // Set the size of the placement visual to match our arranged size
    m_backdropLink.PlacementVisual().Size(winrt::float2{ m_lastArrangedSize.Width, m_lastArrangedSize.Height });
    m_backdropLink.PlacementVisual().Offset(winrt::float3{ 0.0f, 0.0f, 0.0f });
}

void SystemBackdropElement::UpdatePlacementVisualClip()
{
    if (!m_backdropLink || !m_compositor)
    {
        return;
    }
    
    // Create or update the clip to restrict the visual to the parent container bounds
    if (!m_clip)
    {
        m_clip = m_compositor.CreateRectangleClip();
    }

    // Set clip bounds to match our arranged size
    m_clip.Top(0.0f);
    m_clip.Left(0.0f);
    m_clip.Right(m_lastArrangedSize.Width);
    m_clip.Bottom(m_lastArrangedSize.Height);

    //Applying corner radius
    m_clip.TopLeftRadius({static_cast<float>(m_cornerRadius.TopLeft), static_cast<float>(m_cornerRadius.TopLeft)});
    m_clip.TopRightRadius({static_cast<float>(m_cornerRadius.TopRight), static_cast<float>(m_cornerRadius.TopRight)});
    m_clip.BottomLeftRadius({static_cast<float>(m_cornerRadius.BottomLeft), static_cast<float>(m_cornerRadius.BottomLeft)});
    m_clip.BottomRightRadius({static_cast<float>(m_cornerRadius.BottomRight), static_cast<float>(m_cornerRadius.BottomRight)});
    
    // Apply the clip to the placement visual
    m_backdropLink.PlacementVisual().Clip(m_clip);
}

void SystemBackdropElement::EnsureCompositionResources()
{
    if (!m_compositor)
    {
        // Get the compositor from the visual tree
        auto xamlRoot = this->XamlRoot();
        if (!xamlRoot)
        {
            return;
        }
        
        // Get the compositor
        m_compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
    }

    if (!m_backdropLink)
    {
        m_backdropLink = winrt::ContentExternalBackdropLink::Create(m_compositor);
        winrt::Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::SetElementChildVisual(*this, m_backdropLink.PlacementVisual());
    }
}

void SystemBackdropElement::TryConnectSystemBackdrop()
{
    if (!m_registeredWithSystemBackdrop)
    {
        EnsureCompositionResources();
        if (!m_backdropLink)
        {
            return;
        }

        auto xamlRoot = XamlRoot();

        if (xamlRoot && m_systemBackdrop)
        {
            m_systemBackdrop.OnTargetConnected(m_backdropLink, xamlRoot);
            m_registeredWithSystemBackdrop = true;
        }
    }
}

void SystemBackdropElement::ReleaseCompositionResources()
{
    if (m_systemBackdrop && m_backdropLink)
    {
        m_systemBackdrop.OnTargetDisconnected(m_backdropLink);
    }
    m_backdropLink = nullptr;
    m_compositor = nullptr;
    m_clip = nullptr;
    m_registeredWithSystemBackdrop = false;
}