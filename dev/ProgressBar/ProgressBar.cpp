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

    // TODO: things
    SetProgressBarIndicatorWidth();

}

void ProgressBar::OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // NOTE: This hits when IsIndeterminate changes because we set MUX_PROPERTY_CHANGED_CALLBACK to true in the idl.

    // TODO: things
}

void ProgressBar::SetProgressBarIndicatorWidth()
{
    if (auto progressBarIndicator = m_progressBarIndicator.get())
    {
        progressBarIndicator.Width(Value());
    }
}
