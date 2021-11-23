// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBadge.h"
#include "InfoBadgeImpl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

using namespace InfoBadgeImpl;

#pragma region Constructor
InfoBadge::InfoBadge()
{
    InitializeControl();
    AttachEventHandlers();
}

void InfoBadge::InitializeControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBadge);

    SetDefaultStyleKey(this);

    SetValue(s_TemplateSettingsProperty, winrt::make<::InfoBadgeTemplateSettings>());
}

void InfoBadge::AttachEventHandlers()
{
    SizeChanged({ this, &InfoBadge::OnSizeChanged });
}
#pragma endregion

#pragma region ControlOverrides
void InfoBadge::OnApplyTemplate()
{
    GoToAppropriateDisplayKindState();
}

winrt::Size InfoBadge::MeasureOverride(winrt::Size const& availableSize)
{
    return MeasureOverrideImpl(__super::MeasureOverride(availableSize));
}
#pragma endregion 

#pragma region OnIconSourcePropertyChanged
void InfoBadge::OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    TemplateSettings().IconElement(GetIconElementFromSource());
    GoToAppropriateDisplayKindState();
}

winrt::IconElement InfoBadge::GetIconElementFromSource()
{
    if (auto const iconSource = IconSource())
    {
        return iconSource.CreateIconElement();
    }
    return nullptr;
}
#pragma endregion 

#pragma region OnValuePropertyChanged
void InfoBadge::OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ValidateValuePropertyImpl(Value());
    GoToAppropriateDisplayKindState();
}
#pragma endregion 

#pragma region GoToAppropriateDisplayKindState
void InfoBadge::GoToAppropriateDisplayKindState()
{
    auto const [hasIcon, hasFontIcon] = HasIconThenHasFontIcon();
    GoToState(CalculateAppropriateDisplayKindStateImpl(Value(), hasIcon, hasFontIcon));
}

std::tuple<bool, bool> InfoBadge::HasIconThenHasFontIcon()
{
    auto const IconElement = TemplateSettings().IconElement();
    auto const hasIcon = IconElement != nullptr;
    auto const hasFontIcon = hasIcon ? IconElement.try_as<winrt::FontIcon>() != nullptr : false;
    return std::make_tuple(hasIcon, hasFontIcon);
}
#pragma endregion

#pragma region OnSizeChanged
void InfoBadge::OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs& args)
{
    TemplateSettings().InfoBadgeCornerRadius(GetFullyRoundedCornerRadiusValueIfUnset());
}

winrt::CornerRadius InfoBadge::GetFullyRoundedCornerRadiusValueIfUnset()
{
    if (IsCornerRadiusAvailableAndSet())
    {
        return CornerRadius();
    }
    return SharedHelpers::CornerRadiusFromTuple(GetFullyRoundedCornerRadiusValueImpl(ActualHeight()));
}

bool InfoBadge::IsCornerRadiusAvailableAndSet()
{
    // CornerRadius was added to Control in RS5
    return SharedHelpers::IsRS5OrHigher() && !(ReadLocalValue(winrt::Control::CornerRadiusProperty()) == winrt::DependencyProperty::UnsetValue());
}
#pragma endregion
