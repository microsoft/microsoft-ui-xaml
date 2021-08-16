// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "TitleBarTemplateSettings.h"

namespace winrt::Microsoft::Experimental::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithDPFactory(TitleBarTemplateSettings)
}

#include "TitleBarTemplateSettings.g.cpp"

GlobalDependencyProperty TitleBarTemplateSettingsProperties::s_IconElementProperty{ nullptr };

TitleBarTemplateSettingsProperties::TitleBarTemplateSettingsProperties()
{
    EnsureProperties();
}

void TitleBarTemplateSettingsProperties::EnsureProperties()
{
    if (!s_IconElementProperty)
    {
        s_IconElementProperty =
            InitializeDependencyProperty(
                L"IconElement",
                winrt::name_of<winrt::IconElement>(),
                winrt::name_of<winrt::TitleBarTemplateSettings>(),
                false /* isAttached */,
                ValueHelper<winrt::IconElement>::BoxedDefaultValue(),
                nullptr);
    }
}

void TitleBarTemplateSettingsProperties::ClearProperties()
{
    s_IconElementProperty = nullptr;
}

void TitleBarTemplateSettingsProperties::IconElement(winrt::IconElement const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<TitleBarTemplateSettings*>(this)->SetValue(s_IconElementProperty, ValueHelper<winrt::IconElement>::BoxValueIfNecessary(value));
    }
}

winrt::IconElement TitleBarTemplateSettingsProperties::IconElement()
{
    return ValueHelper<winrt::IconElement>::CastOrUnbox(static_cast<TitleBarTemplateSettings*>(this)->GetValue(s_IconElementProperty));
}
