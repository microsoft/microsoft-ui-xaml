// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBadge.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

static constexpr wstring_view c_IconPresenterName{ L"IconPresenter"sv };

InfoBadge::InfoBadge()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBadge);

    SetDefaultStyleKey(this);

    SetValue(s_TemplateSettingsProperty, winrt::make<::InfoBadgeTemplateSettings>());
    SizeChanged({ this, &InfoBadge::OnSizeChanged });
}

void InfoBadge::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    OnDisplayKindPropertiesChanged();
    OnIsOpenChanged();
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

void InfoBadge::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto const property = args.Property();
    winrt::Control const thisAsControl = *this;

    if (property == winrt::InfoBadge::IsOpenProperty())
    {
        OnIsOpenChanged();
    }
    else if (property == winrt::InfoBadge::ValueProperty() ||
             property == winrt::InfoBadge::IconSourceProperty())
    {
        OnDisplayKindPropertiesChanged();
    }
}

void InfoBadge::OnIsOpenChanged()
{
    winrt::Control const thisAsControl = *this;
    winrt::VisualStateManager::GoToState(thisAsControl, IsOpen() ? L"Opened" : L"Closed", true);
}

void InfoBadge::OnDisplayKindPropertiesChanged()
{
    winrt::Control const thisAsControl = *this;
    if (auto const iconSource = IconSource())
    {
        TemplateSettings().IconElement(iconSource.CreateIconElement());
        if (auto const fontIconSource = iconSource.try_as<winrt::FontIconSource>())
        {
            winrt::VisualStateManager::GoToState(thisAsControl, L"FontIcon", true);
        }
        else
        {
            winrt::VisualStateManager::GoToState(thisAsControl, L"Icon", true);
        }
    }
    else if (Value() >= 0)
    {
        winrt::VisualStateManager::GoToState(thisAsControl, L"Value", true);
    }
    else
    {
        winrt::VisualStateManager::GoToState(thisAsControl, L"Dot", true);
    }
}


void InfoBadge::OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs& args)
{
    auto const value = [this]()
    {
        auto const cornerRadiusValue = ActualHeight() / 2;
        if (ReadLocalValue(winrt::Control::CornerRadiusProperty()) == winrt::DependencyProperty::UnsetValue())
        {
            return winrt::CornerRadius{ cornerRadiusValue, cornerRadiusValue, cornerRadiusValue, cornerRadiusValue };
        }
        else
        {
            return CornerRadius();
        }
    }();

    TemplateSettings().InfoBadgeCornerRadius(value);
}
