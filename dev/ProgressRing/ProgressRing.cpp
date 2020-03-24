// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "math.h"

float GetCircleRadius(double thickness, double actualWidth);
winrt::float4 Color4(winrt::Color color);

static constexpr wstring_view s_LayoutRootName{ L"LayoutRoot"sv };
static constexpr wstring_view s_OutlinePathName{ L"OutlinePathPart"sv };
static constexpr wstring_view s_OutlineFigureName{ L"OutlineFigurePart"sv };
static constexpr wstring_view s_OutlineArcName{ L"OutlineArcPart"sv };
static constexpr wstring_view s_RingPathName{ L"RingPathPart"sv };
static constexpr wstring_view s_RingFigureName{ L"RingFigurePart"sv };
static constexpr wstring_view s_RingArcName{ L"RingArcPart"sv };
static constexpr wstring_view s_LottiePlayerName{ L"LottiePlayer"sv };
static constexpr wstring_view s_DeterminateStateName{ L"Determinate"sv };
static constexpr wstring_view s_IndeterminateStateName{ L"Indeterminate"sv };
static constexpr wstring_view s_DefaultForegroundThemeResourceName{ L"SystemControlHighlightAccentBrush"sv };
static constexpr wstring_view s_DefaultBackgroundThemeResourceName{ L"SystemControlBackgroundBaseLowBrush"sv };
static constexpr wstring_view s_ForegroundName{ L"Foreground"sv };
static constexpr wstring_view s_BackgroundName{ L"Background"sv };

ProgressRing::ProgressRing()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressRing);

    SetDefaultStyleKey(this);

    RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), { this, &ProgressRing::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::Control::ForegroundProperty(), { this, &ProgressRing::OnForegroundPropertyChanged });
    RegisterPropertyChangedCallback(winrt::Control::BackgroundProperty(), { this, &ProgressRing::OnBackgroundPropertyChanged });

    SizeChanged({ this, &ProgressRing::OnSizeChanged });
}

winrt::AutomationPeer ProgressRing::OnCreateAutomationPeer()
{
    return winrt::make<ProgressRingAutomationPeer>(*this);
}

void ProgressRing::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_outlinePath.set(GetTemplateChildT<winrt::Path>(s_OutlinePathName, controlProtected));
    m_outlineFigure.set(GetTemplateChildT<winrt::PathFigure>(s_OutlineFigureName, controlProtected));
    m_outlineArc.set(GetTemplateChildT<winrt::ArcSegment>(s_OutlineArcName, controlProtected));
    m_ringPath.set(GetTemplateChildT<winrt::Path>(s_RingPathName, controlProtected));
    m_ringFigure.set(GetTemplateChildT<winrt::PathFigure>(s_RingFigureName, controlProtected));
    m_ringArc.set(GetTemplateChildT<winrt::ArcSegment>(s_RingArcName, controlProtected));

    m_player.set([this, controlProtected]()
    {
        auto const player = GetTemplateChildT<winrt::AnimatedVisualPlayer>(s_LottiePlayerName, controlProtected);
        if (player)
        {
            player.RegisterPropertyChangedCallback(winrt::UIElement::OpacityProperty(), { this, &ProgressRing::OnOpacityPropertyChanged });
        }
        return player;
    }());

    GetStrokeThickness();
    ApplyLottieAnimation();
    UpdateRing();
    UpdateStates();
}

void ProgressRing::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    UpdateRing();
}

void ProgressRing::OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateSegment();
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
        if (auto const progressRingIndeterminate = player.Source().try_as<AnimatedVisuals::ProgressRingIndeterminate>())
        {
            SetLottieForegroundColor(progressRingIndeterminate);
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
        if (auto const progressRingIndeterminate = player.Source().try_as<AnimatedVisuals::ProgressRingIndeterminate>())
        {
            SetLottieBackgroundColor(progressRingIndeterminate);
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

void ProgressRing::OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressRing::ApplyLottieAnimation()
{
    if (auto&& player = m_player.get())
    {
        player.Source(winrt::make<AnimatedVisuals::ProgressRingIndeterminate>());

        if (const auto progressRingIndeterminate = player.Source().try_as<AnimatedVisuals::ProgressRingIndeterminate>())
        {
            SetLottieForegroundColor(progressRingIndeterminate);
            SetLottieBackgroundColor(progressRingIndeterminate);
        }
    }
}

void ProgressRing::SetLottieForegroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate)
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

    const auto color = Color4(foregroundColor);

    progressRingIndeterminate->GetThemeProperties(compositor).InsertVector4(s_ForegroundName, color);
}

void ProgressRing::SetLottieBackgroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate)
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
            // Default color fallback if Background() Brush does not contain SolidColorBrush with Color property.
            return SharedHelpers::FindInApplicationResources(s_DefaultBackgroundThemeResourceName).as<winrt::SolidColorBrush>().Color();
        }
    }();

    const auto color = Color4(backgroundColor);

    progressRingIndeterminate->GetThemeProperties(compositor).InsertVector4(s_BackgroundName, color);
}

void ProgressRing::UpdateStates()
{
    if (IsIndeterminate())
    {
        winrt::VisualStateManager::GoToState(*this, s_IndeterminateStateName, true);

        if (auto&& player = m_player.get())
        {
            const auto _ = player.PlayAsync(0, 1, true);
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_DeterminateStateName, true);
    }
}

void ProgressRing::UpdateSegment()
{
    if (auto&& ringArc = m_ringArc.get())
    {
        auto const angle = [this]()
        {
            auto const normalizedRange = [this]()
            {
                const double minimum = Minimum();
                const double range = Maximum() - minimum;
                const double delta = Value() - minimum;

                const double normalizedRange = (range == 0.0) ? 0.0 : (delta / range);
                // normalizedRange offsets calculation to display a full ring when value = 100%
                // std::nextafter is set as a float as winrt::Point takes floats.
                return std::min(normalizedRange, static_cast<double>(std::nextafterf(1.0, 0.0)));
            }();
            return 2 * M_PI * normalizedRange;
        }();

        const double thickness = GetStrokeThickness();

        const auto size = GetCircleRadius(thickness, ActualWidth());
        const double translationFactor = std::max(thickness / 2.0, 0.0);

        const double x = (std::sin(angle) * size) + size + translationFactor;
        const double y = (((std::cos(angle) * size) - size) * -1) + translationFactor;

        ringArc.IsLargeArc(angle >= M_PI);
        ringArc.Point(winrt::Point(static_cast<float>(x), static_cast<float>(y)));
    }
}

void ProgressRing::UpdateRing()
{
    const double thickness = GetStrokeThickness();
    const auto radius = GetCircleRadius(thickness, ActualWidth());
    const float translationFactor = static_cast<float>(std::max(thickness / 2.0, 0.0));

    if (auto&& outlineFigure = m_outlineFigure.get())
    {
        outlineFigure.StartPoint(winrt::Point(radius + translationFactor, translationFactor));
    }

    if (auto&& ringFigure = m_ringFigure.get())
    {
        ringFigure.StartPoint(winrt::Point(radius + translationFactor, translationFactor));
    }

    if (auto&& outlineArc = m_outlineArc.get())
    {
        outlineArc.Size(winrt::Size(radius, radius));
        outlineArc.Point(winrt::Point(radius + translationFactor - 0.05f, translationFactor));
    }

    if (auto&& ringArc = m_ringArc.get())
    {
        ringArc.Size(winrt::Size(radius, radius));
    }

    UpdateSegment();
}

double ProgressRing::GetStrokeThickness()
{
    const auto ringPathStrokeThickness = [ringPath = m_ringPath.get()]()
    {
        if (ringPath)
        {
            return ringPath.StrokeThickness();
        }
        return 0.0;
    }();
    
    const auto outlinePathStrokeThickness = [outlinePath = m_outlinePath.get()]()
    {
        if (outlinePath)
        {
            return outlinePath.StrokeThickness();
        }
        return 0.0;
    }();

    return std::max(ringPathStrokeThickness, outlinePathStrokeThickness);
}

float GetCircleRadius(double thickness, double actualWidth)
{
    const double safeThickness = std::max(thickness, static_cast<double>(0.0));

    // ProgressRing only accounts for Width (rather than Width + Height) to ensure that it is always a circle and not an ellipse.
    const double radius = std::max((actualWidth - safeThickness) / 2.0, 0.0);

    return static_cast<float>(radius);
}

winrt::float4 Color4(winrt::Color color)
{
    return { static_cast<float>(color.R), static_cast<float>(color.G), static_cast<float>(color.B), static_cast<float>(color.A) };
}
