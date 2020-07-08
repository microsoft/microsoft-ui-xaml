// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "math.h"

static constexpr wstring_view s_LayoutRootName{ L"LayoutRoot"sv };
static constexpr wstring_view s_LottiePlayerName{ L"LottiePlayer"sv };
static constexpr wstring_view s_DefaultForegroundThemeResourceName{ L"SystemControlHighlightAccentBrush"sv };
static constexpr wstring_view s_DefaultBackgroundThemeResourceName{ L"SystemControlBackgroundBaseLowBrush"sv };
static constexpr wstring_view s_ForegroundName{ L"Foreground"sv };
static constexpr wstring_view s_BackgroundName{ L"Background"sv };
static constexpr wstring_view s_ActiveStateName{ L"Active"sv };
static constexpr wstring_view s_DeterminateActiveStateName{ L"DeterminateActive"sv };
static constexpr wstring_view s_InactiveStateName{ L"Inactive"sv };

ProgressRing::ProgressRing()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressRing);

    SetDefaultStyleKey(this);

    RegisterPropertyChangedCallback(winrt::Control::ForegroundProperty(), { this, &ProgressRing::OnForegroundPropertyChanged });
    RegisterPropertyChangedCallback(winrt::Control::BackgroundProperty(), { this, &ProgressRing::OnBackgroundPropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), { this, &ProgressRing::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::MinimumProperty(), { this, &ProgressRing::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::MaximumProperty(), { this, &ProgressRing::OnRangeBasePropertyChanged });
    
    SetValue(s_TemplateSettingsProperty, winrt::make<::ProgressRingTemplateSettings>());

    SizeChanged({ this, &ProgressRing::OnSizeChanged });
}

winrt::AutomationPeer ProgressRing::OnCreateAutomationPeer()
{
    return winrt::make<ProgressRingAutomationPeer>(*this);
}

void ProgressRing::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(s_LayoutRootName, controlProtected));

    m_player.set([this, controlProtected]()
        {
            auto const indeterminateplayer = GetTemplateChildT<winrt::AnimatedVisualPlayer>(s_LottiePlayerName, controlProtected);
            if (indeterminateplayer)
            {
                indeterminateplayer.RegisterPropertyChangedCallback(winrt::UIElement::OpacityProperty(), { this, &ProgressRing::OnOpacityPropertyChanged });
            }
            return indeterminateplayer;
        }());

    SetAnimatedVisualPlayerSource();
    UpdateLottieProgress();
    UpdateStates();
}

void ProgressRing::OnDeterminateSourcePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    SetAnimatedVisualPlayerSource();
}

void ProgressRing::OnIndeterminateSourcePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    SetAnimatedVisualPlayerSource();
}

void ProgressRing::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    ApplyTemplateSettings();
}

void ProgressRing::OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (!IsIndeterminate())
    {
        UpdateLottieProgress();
    }
}

void ProgressRing::OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (const auto foreground = Foreground().try_as<winrt::SolidColorBrush>())
    {
        foreground.RegisterPropertyChangedCallback(winrt::SolidColorBrush::ColorProperty(), { this, &ProgressRing::OnForegroundColorPropertyChanged });
    }

    OnForegroundColorPropertyChanged(nullptr, nullptr);
}

void ProgressRing::OnForegroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (auto&& player = m_player.get())
    {
        if (auto const progressRingAnimation = player.Source())
        {
            if (IsIndeterminate())
            {
                if (!IndeterminateSource())
                {
                    SetLottieForegroundColor(progressRingAnimation);
                }
            }
            else
            {
                if (!DeterminateSource())
                {
                    SetLottieForegroundColor(progressRingAnimation);
                }
            }
        }
    }
}

void ProgressRing::OnBackgroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (const auto background = Background().try_as<winrt::SolidColorBrush>())
    {
        background.RegisterPropertyChangedCallback(winrt::SolidColorBrush::ColorProperty(), { this, &ProgressRing::OnBackgroundColorPropertyChanged });
    }

    OnBackgroundColorPropertyChanged(nullptr, nullptr);
}

void ProgressRing::OnBackgroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (auto&& player = m_player.get())
    {
        if (auto const progressRingAnimation = player.Source())
        {
            if (IsIndeterminate())
            {
                if (!IndeterminateSource())
                {
                    SetLottieBackgroundColor(progressRingAnimation);
                }
            }
            else
            {
                if (!DeterminateSource())
                {
                    SetLottieBackgroundColor(progressRingAnimation);
                }
            }
        }
    }
}

void ProgressRing::OnOpacityPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (auto&& player = m_player.get())
    {
        if (player.Opacity() == 0)
        {
            player.Stop();
        }
    }
}

void ProgressRing::OnIsActivePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressRing::OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressRing::SetAnimatedVisualPlayerSource()
{
    if (auto&& player = m_player.get())
    {
        if (IsIndeterminate())
        {
            if (!IndeterminateSource())
            {
                player.Source(winrt::make<AnimatedVisuals::ProgressRingIndeterminate>());

                if (const auto progressRingAnimation = player.Source())
                {
                    SetLottieForegroundColor(progressRingAnimation);
                    SetLottieBackgroundColor(progressRingAnimation);
                }
            }
            else
            {
                player.Source(IndeterminateSource());
            }
        }
        else
        {
            if (!DeterminateSource())
            {
                player.Source(winrt::make<AnimatedVisuals::ProgressRingDeterminate>());

                if (const auto progressRingAnimation = player.Source())
                {
                    SetLottieForegroundColor(progressRingAnimation);
                    SetLottieBackgroundColor(progressRingAnimation);
                }
            }
            else
            {
                player.Source(DeterminateSource());
            }
        }      
    }
}

void ProgressRing::SetLottieForegroundColor(const winrt::IAnimatedVisualSource animatedVisualSource)
{
    const auto compositor = winrt::Window::Current().Compositor();

    const auto foregroundColor = [foreground = Foreground().try_as<winrt::SolidColorBrush>()]()
    {
        if (foreground)
        {
            return foreground.Color();
        }
        else
        {
            // Default color fallback if Foreground() Brush does not contain SolidColorBrush with Color property.
            return SharedHelpers::FindInApplicationResources(s_DefaultForegroundThemeResourceName).as<winrt::SolidColorBrush>().Color();
        }
    }();

    if (const auto progressRingAnimation = animatedVisualSource.try_as<AnimatedVisuals::ProgressRingIndeterminate>())
    {
        progressRingAnimation->GetThemeProperties(compositor).InsertVector4(s_ForegroundName, SharedHelpers::RgbaColor(foregroundColor));
    }
}

void ProgressRing::SetLottieBackgroundColor(const winrt::IAnimatedVisualSource animatedVisualSource)
{
    const auto compositor = winrt::Window::Current().Compositor();

    const auto backgroundColor = [background = Background().try_as<winrt::SolidColorBrush>()]()
    {
        if (background)
        {
            return background.Color();
        }
        else
        {
             //Default color fallback if Background() Brush does not contain SolidColorBrush with Color property.
            return SharedHelpers::FindInApplicationResources(s_DefaultBackgroundThemeResourceName).as<winrt::SolidColorBrush>().Color();
        }
    }();

    if (const auto progressRingAnimation = animatedVisualSource.try_as<AnimatedVisuals::ProgressRingIndeterminate>())
    {
        progressRingAnimation->GetThemeProperties(compositor).InsertVector4(s_BackgroundName, SharedHelpers::RgbaColor(backgroundColor));
    }
}

void ProgressRing::UpdateLottieProgress()
{
    if (auto&& player = m_player.get())
    {
        const double value = Value();
        const double range = Maximum() - Minimum();
        const double fromProgress = m_oldValue / range;
        const double toProgress = value / range;

        const auto _ = player.PlayAsync(fromProgress, toProgress, false);
        m_oldValue = value;
    }
}

void ProgressRing::UpdateStates()
{
    if (IsActive())
    {
        if (IsIndeterminate())
        {
            winrt::VisualStateManager::GoToState(*this, s_ActiveStateName, true);

            SetAnimatedVisualPlayerSource();

            if (auto&& player = m_player.get())
            {
                const auto _ = player.PlayAsync(0, 1, true);
            }
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, s_DeterminateActiveStateName, true);

            SetAnimatedVisualPlayerSource();
            UpdateLottieProgress();
        }   
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_InactiveStateName, true);

        if (auto&& player = m_player.get())
        {
            player.Stop();
        }
    }
}

void ProgressRing::ApplyTemplateSettings()
{
    // TemplateSetting properties from WUXC for backwards compatibility.
    const auto templateSettings = winrt::get_self<::ProgressRingTemplateSettings>(TemplateSettings());

    const auto [width, diameterValue, anchorPoint] = [this]()
    {
        if (this->ActualWidth())
        {
            const float width = static_cast<float>(this->ActualWidth());

            const auto diameterAdditive = [width]()
            {
                if (width <= 40.0f)
                {
                    return 1.0f;
                }
                return 0.0f;
            }();

            const float diamaterValue = (width * 0.1f) + diameterAdditive;
            const float anchorPoint = (width * 0.5f) - diamaterValue;
            return std::make_tuple(width, diamaterValue, anchorPoint);
        }

        return std::make_tuple(0.0f, 0.0f, 0.0f);
    }();
  
    templateSettings->EllipseDiameter(diameterValue);

    const winrt::Thickness thicknessEllipseOffset = { 0, anchorPoint, 0, 0 };

    templateSettings->EllipseOffset(thicknessEllipseOffset);
    templateSettings->MaxSideLength(width);
}
