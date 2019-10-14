// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressBar.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ProgressBar::ProgressBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressBar);

    SetDefaultStyleKey(this);

    // NOTE: This is necessary only because Value isn't one of OUR properties, it's implemented in RangeBase.
    // If it was one of ProgressBar's properties, defined in the IDL, you'd do it differently (see IsIndeterminate).
    RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), { this, &ProgressBar::OnRangeBaseValueChanged });

    // NOTE: You can also hook up to events on your control from here.
    Loaded({ this, &ProgressBar::OnLoaded });

    SetValue(s_TemplateSettingsProperty, winrt::make<::ProgressBar2TemplateSettings>());

}

void ProgressBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // NOTE: Example of how named parts are loaded from the template. Important to remember that it's possible for
    // any of them not to be found, since devs can replace the template with their own.
    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(L"LayoutRoot", controlProtected));
    if (auto layoutRoot = m_layoutRoot.get())
    {
        // NOTE: You can hook up to events or property changes on your template parts here, but you have to hook up revokers also
        // because the template can change or the control could be removed and readded to the tree, etc.
        // (I can think of no reason you would want to handle grid loading, this is just an example of how it works.)
        m_layoutRootLoadedRevoker = layoutRoot.Loaded(winrt::auto_revoke, { this, &ProgressBar::OnLayoutRootLoaded });
    }

    m_progressBarIndicator.set(GetTemplateChildT<winrt::Rectangle>(L"ProgressBarIndicator", controlProtected));
    if (auto progressBarIndicator = m_progressBarIndicator.get())
    {
        m_progressBarIndicatorRevoker = progressBarIndicator.Loaded(winrt::auto_revoke, { this, &ProgressBar::OnLayoutRootLoaded });
    }

    if (IsIndeterminate())
    {
        UpdateStates();
    }
}

void ProgressBar::OnLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    // TODO: things
}

void ProgressBar::OnLayoutRootLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    // TODO: things
}

void ProgressBar::OnRangeBaseValueChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    // NOTE: This hits when the Value property changes, because we called RegisterPropertyChangedCallback.

    SetProgressBarIndicatorWidth();

}

void ProgressBar::OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // NOTE: This hits when IsIndeterminate changes because we set MUX_PROPERTY_CHANGED_CALLBACK to true in the idl.

    UpdateStates();
}

void ProgressBar::OnShowPausedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{

    UpdateStates();
}

void ProgressBar::UpdateStates()
{
    if (IsIndeterminate())
    {
        SetProgressBarIndicatorWidth();
        UpdateWidthBasedTemplateSettings();
        winrt::VisualStateManager::GoToState(*this, L"Indeterminate", true);
    }
    else if (ShowPaused())
    {
        winrt::VisualStateManager::GoToState(*this, L"Paused", true);
    }
    else if (!ShowPaused())
    {
        winrt::VisualStateManager::GoToState(*this, L"Determinate", true);
    }
}

void ProgressBar::SetProgressBarIndicatorWidth()
{
    if (auto progressBarIndicator = m_progressBarIndicator.get())
    {
        double progressBarWidth = m_layoutRoot.get().ActualWidth();

        if (IsIndeterminate())
        {
            progressBarIndicator.Width(progressBarWidth / 3);
            return;
        }

        double maximum = Maximum();
        double minimum = Minimum();
        double increment = 0;
        
        increment = progressBarWidth / (maximum - minimum);
        progressBarIndicator.Width(increment * (Value() - minimum));

    }
}

void ProgressBar::UpdateWidthBasedTemplateSettings()
{
    auto const templateSettings = winrt::get_self<::ProgressBar2TemplateSettings>(TemplateSettings());
    auto progressBar = m_layoutRoot.get();

    auto const [width, height] = [progressBar]()
    {
        if (progressBar)
        {
            float const width = static_cast<float>(progressBar.ActualWidth());
            float const height = static_cast<float>(progressBar.ActualHeight());
            return std::make_tuple(width, height);
        }
        return std::make_tuple(0.0f, 0.0f);
    }();

    templateSettings->ContainerAnimationEndPosition(width);
    templateSettings->ContainerAnimationStartPosition(0);

    winrt::Windows::UI::Xaml::Media::RectangleGeometry rectangle = [width, height]()
    {
        auto const returnValue = winrt::RectangleGeometry();
        winrt::Rect rect{ 0, 0, width, height };
        returnValue.Rect(rect);
        return returnValue;
    }();

    templateSettings->ClipRect(rectangle);
}
