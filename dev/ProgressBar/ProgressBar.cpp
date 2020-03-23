// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressBar.h"
#include "ProgressBarAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ProgressBar::ProgressBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressBar);

    SetDefaultStyleKey(this);

    SizeChanged({ this, &ProgressBar::OnSizeChanged });

    // NOTE: This is necessary only because Value isn't one of OUR properties, it's implemented in RangeBase.
    // If it was one of ProgressBar's properties, defined in the IDL, you'd do it differently (see IsIndeterminate).
    RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), { this, &ProgressBar::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::MinimumProperty(), { this, &ProgressBar::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::MaximumProperty(), { this, &ProgressBar::OnRangeBasePropertyChanged });

    SetValue(s_TemplateSettingsProperty, winrt::make<::ProgressBarTemplateSettings>());
}

winrt::AutomationPeer ProgressBar::OnCreateAutomationPeer()
{
    return winrt::make<ProgressBarAutomationPeer>(*this);
}


void ProgressBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // NOTE: Example of how named parts are loaded from the template. Important to remember that it's possible for
    // any of them not to be found, since devs can replace the template with their own.

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(s_LayoutRootName, controlProtected));
    m_determinateProgressBarIndicator.set(GetTemplateChildT<winrt::Rectangle>(s_DeterminateProgressBarIndicatorName, controlProtected));
    m_indeterminateProgressBarIndicator.set(GetTemplateChildT<winrt::Rectangle>(s_IndeterminateProgressBarIndicatorName, controlProtected));
    m_indeterminateProgressBarIndicator2.set(GetTemplateChildT<winrt::Rectangle>(s_IndeterminateProgressBarIndicator2Name, controlProtected));

    UpdateStates();
}

void ProgressBar::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    SetProgressBarIndicatorWidth();
    UpdateWidthBasedTemplateSettings();
}

void ProgressBar::OnRangeBasePropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    // NOTE: This hits when the Value property changes, because we called RegisterPropertyChangedCallback.
    SetProgressBarIndicatorWidth();
}

void ProgressBar::OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // NOTE: This hits when IsIndeterminate changes because we set MUX_PROPERTY_CHANGED_CALLBACK to true in the idl.
    SetProgressBarIndicatorWidth();
    UpdateStates(); 
}

void ProgressBar::OnShowPausedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressBar::OnShowErrorPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressBar::UpdateStates()
{
    if (ShowError() && IsIndeterminate())
    {
        winrt::VisualStateManager::GoToState(*this, s_IndeterminateErrorStateName, true);
    }
    else if (ShowError())
    {
        winrt::VisualStateManager::GoToState(*this, s_ErrorStateName, true);
    }
    else if (ShowPaused() && IsIndeterminate())
    {
        winrt::VisualStateManager::GoToState(*this, s_IndeterminatePausedStateName, true);
    }
    else if (ShowPaused())
    {
        winrt::VisualStateManager::GoToState(*this, s_PausedStateName, true);
    }
    else if (IsIndeterminate())
    {
        UpdateWidthBasedTemplateSettings();
        winrt::VisualStateManager::GoToState(*this, s_IndeterminateStateName, true);
    }
    else if (!IsIndeterminate())
    {
        winrt::VisualStateManager::GoToState(*this, s_DeterminateStateName, true);
    }
}

void ProgressBar::SetProgressBarIndicatorWidth()
{
    const auto templateSettings = winrt::get_self<::ProgressBarTemplateSettings>(TemplateSettings());

    if (auto&& progressBar = m_layoutRoot.get())
    {
        if (auto&& determinateProgressBarIndicator = m_determinateProgressBarIndicator.get())
        {
            const double progressBarWidth = progressBar.ActualWidth();
            const double prevIndicatorWidth = determinateProgressBarIndicator.ActualWidth();
            const double maximum = Maximum();
            const double minimum = Minimum();
            const auto padding = Padding();

            // Adds "Updating" state in between to trigger RepositionThemeAnimation Visual Transition
            // in ProgressBar.xaml when reverting back to previous state
            winrt::VisualStateManager::GoToState(*this, s_UpdatingStateName, true); 

            if (IsIndeterminate())
            {
                determinateProgressBarIndicator.Width(0);

                if (auto&& indeterminateProgressBarIndicator = m_indeterminateProgressBarIndicator.get())
                {
                    indeterminateProgressBarIndicator.Width(progressBarWidth * 0.4); // 40% of ProgressBar Width
                }

                if (auto&& indeterminateProgressBarIndicator2 = m_indeterminateProgressBarIndicator2.get())
                {
                    indeterminateProgressBarIndicator2.Width(progressBarWidth * 0.6); // 60% of ProgressBar Width
                }
            }
            else if (std::abs(maximum - minimum) > DBL_EPSILON)
            {
                const double maxIndicatorWidth = progressBarWidth - (padding.Left + padding.Right);
                const double increment = maxIndicatorWidth / (maximum - minimum);
                const double indicatorWidth = increment * (Value() - minimum);
                const double widthDelta = indicatorWidth - prevIndicatorWidth;
                templateSettings->IndicatorLengthDelta(-widthDelta);
                determinateProgressBarIndicator.Width(indicatorWidth);
            }
            else
            {
                determinateProgressBarIndicator.Width(0); // Error
            }
           
            UpdateStates(); // Reverts back to previous state
        }
    }
}

void ProgressBar::UpdateWidthBasedTemplateSettings()
{
    const auto templateSettings = winrt::get_self<::ProgressBarTemplateSettings>(TemplateSettings());

    const auto [width, height] = [progressBar = m_layoutRoot.get()]()
    {
        if (progressBar)
        {
            const float width = static_cast<float>(progressBar.ActualWidth());
            const float height = static_cast<float>(progressBar.ActualHeight());
            return std::make_tuple(width, height);
        }
        return std::make_tuple(0.0f, 0.0f);
    }();

    const double indeterminateProgressBarIndicatorWidth = width * 0.4; // Indicator width at 40% of ProgressBar
    const double indeterminateProgressBarIndicatorWidth2 = width * 0.6; // Indicator width at 60% of ProgressBar

    templateSettings->ContainerAnimationStartPosition(indeterminateProgressBarIndicatorWidth * -1.0); // Position at -100%
    templateSettings->ContainerAnimationEndPosition(indeterminateProgressBarIndicatorWidth * 3.0); // Position at 300%

    templateSettings->ContainerAnimationStartPosition2(indeterminateProgressBarIndicatorWidth2 * -1.5); // Position at -150%
    templateSettings->ContainerAnimationEndPosition2(indeterminateProgressBarIndicatorWidth2 * 1.66); // Position at 166%

    templateSettings->ContainerAnimationMidPosition(width * 0.2);

    const auto rectangle = [width, height, padding = Padding()]()
    {
        const auto returnValue = winrt::RectangleGeometry();
        returnValue.Rect({
            static_cast<float>(padding.Left),
            static_cast<float>(padding.Top),
            width - static_cast<float>(padding.Right + padding.Left),
            height - static_cast<float>(padding.Bottom + padding.Top)
            });
        return returnValue;
    }();

    templateSettings->ClipRect(rectangle);
}
