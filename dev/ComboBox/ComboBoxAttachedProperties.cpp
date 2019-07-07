// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ComboBoxAttachedProperties.h"
#include "ComboBoxAttachedProperties.properties.h"

static constexpr auto c_popupName = L"Popup"sv;
static constexpr auto c_popupBorderName = L"PopupBorder"sv;
static constexpr auto c_editableTextName = L"EditableText"sv;

// Normal ComboBox and editable ComboBox have differnt CornerRadius behaviors.
// Xaml is not lifted yet when we implementing this feature so we don't have access to ComboBox code.
// Creating this attached property to help us plug in some extra logic without touching the actual ComboBox code.
void ComboBoxAttachedProperties::OnApplyDynamicCornerRadiusPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    bool shouldMonitorDropDownState = unbox_value<bool>(args.NewValue());
    if (shouldMonitorDropDownState)
    {
        if (auto comboBox = sender.try_as<winrt::ComboBox>())
        {
            comboBox.DropDownOpened(&ComboBoxAttachedProperties::OnDropDownOpened);
            comboBox.DropDownClosed(&ComboBoxAttachedProperties::OnDropDownClosed);
        }
    }
}

void ComboBoxAttachedProperties::OnDropDownOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        // We get dropDown open direction (above/below ComboBox TextBlock) by checking popup.VerticalOffset.
        // Sometimes VerticalOffset value is incorrect because popup is not fully opened when this function gets called.
        // Use dispatcher to make sure we get correct VerticalOffset.
        auto dispatcher = winrt::Window::Current().Dispatcher();
        dispatcher.RunAsync(
            winrt::CoreDispatcherPriority::Normal,
            winrt::DispatchedHandler([comboBox]()
                {
                    UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/true);
                }));
    }
}

void ComboBoxAttachedProperties::OnDropDownClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/false);
    }
}

void ComboBoxAttachedProperties::UpdateCornerRadius(const winrt::ComboBox& comboBox, bool isDropDownOpen)
{
    if (comboBox.IsEditable())
    {
        auto popupRadius = comboBox.CornerRadius();
        auto textBoxRadius = comboBox.CornerRadius();

        if (isDropDownOpen)
        {
            bool isOpenDown = IsPopupOpenDown(comboBox);
            winrt::CornerRadiusFilterConverter cornerRadiusConverter;

            auto popupRadiusFilterDirection = isOpenDown ? L"Bottom" : L"Top";
            popupRadius = cornerRadiusConverter.Convert(popupRadius, popupRadiusFilterDirection);

            auto textBoxRadiusFilterDirection = isOpenDown ? L"Top" : L"Bottom";
            textBoxRadius = cornerRadiusConverter.Convert(textBoxRadius, textBoxRadiusFilterDirection);
        }

        if (auto popupBorder = GetTemplateChildT<winrt::Border>(c_popupBorderName, comboBox))
        {
            popupBorder.CornerRadius(popupRadius);
        }
        if (auto textBox = GetTemplateChildT<winrt::TextBox>(c_editableTextName, comboBox))
        {
            textBox.CornerRadius(textBoxRadius);
        }
    }
}

bool ComboBoxAttachedProperties::IsPopupOpenDown(const winrt::ComboBox& comboBox)
{
    double verticalOffset = 0;
    if (auto popup = GetTemplateChildT<winrt::Popup>(c_popupName, comboBox))
    {
        verticalOffset = popup.VerticalOffset();
    }
    return verticalOffset > 0;
}
