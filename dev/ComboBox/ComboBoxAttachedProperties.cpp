// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ComboBoxAttachedProperties.h"
#include "ComboBoxAttachedProperties.properties.h"
#include "DispatcherHelper.h"
#include "Converters.h"
#include "HashMap.h"

static constexpr auto c_popupName = L"Popup"sv;
static constexpr auto c_popupBorderName = L"PopupBorder"sv;
static constexpr auto c_editableTextName = L"EditableText"sv;

using CornerRadiusFilterType = CornerRadiusFilterConverter::FilterType;

// Normal ComboBox and editable ComboBox have different CornerRadius behaviors.
// Xaml is not lifted yet when we implementing this feature so we don't have access to ComboBox code.
// Creating this attached property to help us plug in some extra logic without touching the actual ComboBox code.
void ComboBoxAttachedProperties::OnApplyDynamicCornerRadiusPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    static std::map<winrt::ComboBox*, std::vector<winrt::event_token>> comboBoxEventTokenMap;
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        if (comboBoxEventTokenMap.find(&comboBox) != comboBoxEventTokenMap.end())
        {
            auto eventTokens = comboBoxEventTokenMap[&comboBox];
            comboBox.DropDownOpened(eventTokens[0]);
            comboBox.DropDownClosed(eventTokens[1]);
        }

        bool shouldMonitorDropDownState = unbox_value<bool>(args.NewValue());
        if (shouldMonitorDropDownState)
        {
            auto dropDownOpenToken = comboBox.DropDownOpened(ComboBoxAttachedProperties::OnDropDownOpened);
            auto dropDownClosedToken = comboBox.DropDownClosed(ComboBoxAttachedProperties::OnDropDownClosed);

            std::vector<winrt::event_token> tokenVector;
            tokenVector.push_back(dropDownOpenToken);
            tokenVector.push_back(dropDownClosedToken);
            comboBoxEventTokenMap[&comboBox] = tokenVector;
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
        DispatcherHelper dispatcherHelper;
        dispatcherHelper.RunAsync([comboBox]()
            {
                UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/true);
            });
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
            auto cornerRadiusConverter = winrt::make_self<CornerRadiusFilterConverter>();

            auto popupRadiusFilter = isOpenDown ? CornerRadiusFilterType::Bottom : CornerRadiusFilterType::Top;
            popupRadius = cornerRadiusConverter->Convert(popupRadius, popupRadiusFilter);

            auto textBoxRadiusFilter = isOpenDown ? CornerRadiusFilterType::Top : CornerRadiusFilterType::Bottom;
            textBoxRadius = cornerRadiusConverter->Convert(textBoxRadius, textBoxRadiusFilter);
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
    return verticalOffset >= 0;
}
