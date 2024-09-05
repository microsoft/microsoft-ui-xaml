// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "MicaBackdrop.h"

namespace winrt::Microsoft::UI::Xaml::Media
{
    CppWinRTActivatableClassWithDPFactory(MicaBackdrop)
}

#include "MicaBackdrop.g.cpp"

GlobalDependencyProperty MicaBackdropProperties::s_KindProperty{ nullptr };

MicaBackdropProperties::MicaBackdropProperties()
{
    EnsureProperties();
}

void MicaBackdropProperties::EnsureProperties()
{
    if (!s_KindProperty)
    {
        s_KindProperty =
            InitializeDependencyProperty(
                L"Kind",
                winrt::name_of<winrt::MicaKind>(),
                winrt::name_of<winrt::MicaBackdrop>(),
                false /* isAttached */,
                ValueHelper<winrt::MicaKind>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnKindPropertyChanged));
    }
}

void MicaBackdropProperties::ClearProperties()
{
    s_KindProperty = nullptr;
}

void MicaBackdropProperties::OnKindPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::MicaBackdrop>();
    winrt::get_self<MicaBackdrop>(owner)->OnPropertyChanged(args);
}

void MicaBackdropProperties::Kind(winrt::MicaKind const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<MicaBackdrop*>(this)->SetValue(s_KindProperty, ValueHelper<winrt::MicaKind>::BoxValueIfNecessary(value));
    }
}

winrt::MicaKind MicaBackdropProperties::Kind()
{
    return ValueHelper<winrt::MicaKind>::CastOrUnbox(static_cast<MicaBackdrop*>(this)->GetValue(s_KindProperty));
}