// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InfoBadgeTemplateSettings.h"

#include "InfoBadge.g.h"
#include "InfoBadge.properties.h"
#include "InfoBadgeTemplatePartHelpers.h"

class InfoBadge :
    public ReferenceTracker<InfoBadge, winrt::implementation::InfoBadgeT>,
    public InfoBadgeProperties
{
#pragma region Constructor
public:
    InfoBadge();
    ~InfoBadge() {}
private:
    void InitializeControl();
    void AttachEventHandlers();
#pragma endregion

#pragma region ControlOverrides
public:
    void OnApplyTemplate();
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
#pragma endregion 

#pragma region OnIconSourcePropertyChanged
public:
    void OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
private:
    winrt::IconElement GetIconElementFromSource();
#pragma endregion 

#pragma region OnValuePropertyChanged
public:
    void OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
private:
    void ValidateValueProperty();
#pragma endregion

#pragma region GoToAppropriateDisplayKindState
private:
    void GoToAppropriateDisplayKindState();
    InfoBadgeDisplayKindStates CalculateAppropriateDisplayKindState();
    InfoBadgeDisplayKindStates CalculateIconOrDotDisplayKindState();
    InfoBadgeDisplayKindStates CalculateIconDisplayKindState(const winrt::Windows::UI::Xaml::Controls::IconElement& iconElement);
#pragma endregion

#pragma region OnSizeChanged
private:
    void OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs& args);
    winrt::CornerRadius GetFullyRoundedCornerRadiusValueIfUnset();
    bool IsCornerRadiusAvailableAndSet();
#pragma endregion
};
