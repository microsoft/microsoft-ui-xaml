// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "InkToolbarStrokeWidthSlider.h"
#include "InkToolbarStrokeWidthSliderAutomationPeer.h"

InkToolbarStrokeWidthSlider::InkToolbarStrokeWidthSlider()
{
    // Pick up the stock Slider's default style (template, thumb, visuals); this subclass only
    // changes the automation peer, not the presentation.
    winrt::IControlProtected controlProtected = *this;
    controlProtected.DefaultStyleKey(box_value(L"Microsoft.UI.Xaml.Controls.Slider"));

    ValueChanged({ this, &InkToolbarStrokeWidthSlider::OnValueChangedEvent });
}

winrt::AutomationPeer InkToolbarStrokeWidthSlider::OnCreateAutomationPeer()
{
    return winrt::make<InkToolbarStrokeWidthSliderAutomationPeer>(*this);
}

void InkToolbarStrokeWidthSlider::OnValueChangedEvent(winrt::IInspectable const& /*sender*/, winrt::RangeBaseValueChangedEventArgs const& args)
{
    // Narrator caches the Value pattern string; raise the change so dragging/arrowing re-announces
    // the new absolute size rather than the stale one.
    if (auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::InkToolbarStrokeWidthSliderAutomationPeer>())
    {
        winrt::get_self<InkToolbarStrokeWidthSliderAutomationPeer>(peer)->RaiseValueChanged(args.OldValue(), args.NewValue());
    }
}
