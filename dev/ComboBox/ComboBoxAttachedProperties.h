// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ComboBoxAttachedProperties.g.h"
#include "ComboBoxAttachedProperties.properties.h"

class ComboBoxAttachedProperties
    : public winrt::implementation::ComboBoxAttachedPropertiesT<ComboBoxAttachedProperties>
    , public ComboBoxAttachedPropertiesProperties
{
public:
    static void OnApplyDynamicCornerRadiusPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

private:
    static void OnDropDownOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    static void OnDropDownClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    static void UpdateCornerRadius(const winrt::ComboBox& comboBox, bool isDropDownOpen);
    static bool IsPopupOpenDown(const winrt::ComboBox& comboBox);
};
