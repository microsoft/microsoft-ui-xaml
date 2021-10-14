// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBadge.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

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
    auto const defaultDesiredSize = __super::MeasureOverride(availableSize);
    if (defaultDesiredSize.Width < defaultDesiredSize.Height)
    {
        return { defaultDesiredSize.Height, defaultDesiredSize.Height };
    }
    return defaultDesiredSize;
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
    ValidateValueProperty();
    GoToAppropriateDisplayKindState();
}

void InfoBadge::ValidateValueProperty()
{
    if (Value() < -1)
    {
        throw winrt::hresult_out_of_bounds(L"Value must be equal to or greater than -1");
    }
}
#pragma endregion 

#pragma region GoToAppropriateDisplayKindState
void InfoBadge::GoToAppropriateDisplayKindState()
{
    winrt::Control const thisAsControl = *this;
    InfoBadgeTemplateHelpers::GoToState(thisAsControl, CalculateAppropriateDisplayKindState());
}

InfoBadgeDisplayKindStates InfoBadge::CalculateAppropriateDisplayKindState()
{
    if (Value() >= 0)
    {
        return InfoBadgeDisplayKindStates::Value;
    }

    return CalculateIconOrDotDisplayKindState();
}

InfoBadgeDisplayKindStates InfoBadge::CalculateIconOrDotDisplayKindState()
{
    if (auto const iconElement = TemplateSettings().IconElement())
    {
        return CalculateIconDisplayKindState(iconElement);
    }

    return InfoBadgeDisplayKindStates::Dot;
}

InfoBadgeDisplayKindStates InfoBadge::CalculateIconDisplayKindState(const winrt::Windows::UI::Xaml::Controls::IconElement& iconElement)
{
    if (iconElement.try_as<winrt::FontIcon>())
    {
        return InfoBadgeDisplayKindStates::FontIcon;
    }
    else
    {
        return InfoBadgeDisplayKindStates::Icon;
    }
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
    auto const cornerRadiusValue = ActualHeight() / 2;
    return winrt::CornerRadius{ cornerRadiusValue, cornerRadiusValue, cornerRadiusValue, cornerRadiusValue };
}

bool InfoBadge::IsCornerRadiusAvailableAndSet()
{
    // CornerRadius was added to Control in RS5
    return SharedHelpers::IsRS5OrHigher() && !(ReadLocalValue(winrt::Control::CornerRadiusProperty()) == winrt::DependencyProperty::UnsetValue());
}
#pragma endregion
