// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ComboBoxHelper.h"
#include "ComboBoxHelper.properties.h"
#include "DispatcherHelper.h"
#include "Converters.h"

using CornerRadiusFilterType = CornerRadiusFilterConverter::FilterType;

static constexpr auto c_popupName = L"Popup"sv;
static constexpr auto c_popupBorderName = L"PopupBorder"sv;
static constexpr auto c_editableTextName = L"EditableText"sv;
GlobalDependencyProperty ComboBoxHelper::s_DropDownEventRevokersProperty{ nullptr };

ComboBoxHelper::ComboBoxHelper()
{
    EnsureProperties();
}

void ComboBoxHelper::EnsureProperties()
{
    if (!s_DropDownEventRevokersProperty)
    {
        s_DropDownEventRevokersProperty =
            InitializeDependencyProperty(
                L"DropDownEventRevokers",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::ComboBoxHelper>(),
                true /* isAttached */,
                ValueHelper<winrt::IInspectable>::BoxedDefaultValue(),
                nullptr);
    }

    ComboBoxHelperProperties::EnsureProperties();
}

void ComboBoxHelper::ClearProperties()
{
    s_DropDownEventRevokersProperty = nullptr;
}


// Normal ComboBox and editable ComboBox have different CornerRadius behaviors.
// Xaml is not lifted yet when we implementing this feature so we don't have access to ComboBox code.
// Creating this attached property to help us plug in some extra logic without touching the actual ComboBox code.
void ComboBoxHelper::OnApplyDynamicCornerRadiusPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        if (auto revokersInspectable = comboBox.GetValue(DropDownEventRevokersProperty()))
        {
            if (auto revokers = revokersInspectable.try_as<ComboBoxDropDownEventRevokers>())
            {
                revokers->RevokeAll();
            }
        }
        
        bool shouldMonitorDropDownState = unbox_value<bool>(args.NewValue());
        if (shouldMonitorDropDownState)
        {
            auto revokersInspectable = winrt::make<ComboBoxDropDownEventRevokers>();
            auto revokers = revokersInspectable.try_as<ComboBoxDropDownEventRevokers>();

            revokers->m_dropDownOpenedRevoker = comboBox.DropDownOpened(winrt::auto_revoke, ComboBoxHelper::OnDropDownOpened);
            revokers->m_dropDownClosedRevoker = comboBox.DropDownClosed(winrt::auto_revoke, ComboBoxHelper::OnDropDownClosed);

            comboBox.SetValue(DropDownEventRevokersProperty(), revokersInspectable);
        }
    }
}

void ComboBoxHelper::SetDropDownEventRevokers(winrt::UIElement const& target, winrt::IInspectable const& value)
{
    target.SetValue(s_DropDownEventRevokersProperty, ValueHelper<winrt::IInspectable>::BoxValueIfNecessary(value));
}

winrt::IInspectable ComboBoxHelper::GetDropDownEventRevokers(winrt::UIElement const& target)
{
    return ValueHelper<winrt::IInspectable>::CastOrUnbox(target.GetValue(s_DropDownEventRevokersProperty));
}

void ComboBoxHelper::OnDropDownOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        // We need to know whether the dropDown opens above or below the ComboBox in order to update corner radius correctly.
        // Sometimes TransformToPoint value is incorrect because popup is not fully opened when this function gets called.
        // Use dispatcher to make sure we get correct VerticalOffset.
        DispatcherHelper dispatcherHelper;
        dispatcherHelper.RunAsync([comboBox]()
            {
                UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/true);
            });
    }
}

void ComboBoxHelper::OnDropDownClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/false);
    }
}

void ComboBoxHelper::UpdateCornerRadius(const winrt::ComboBox& comboBox, bool isDropDownOpen)
{
    if (comboBox.IsEditable())
    {
        // TODO, read default values from theme resource once we have them
        winrt::CornerRadius popupRadius { 0 };
        winrt::CornerRadius textBoxRadius{ 0 };

        if (auto comboBoxControl7 = comboBox.try_as<winrt::IControl7>())
        {
            popupRadius = comboBoxControl7.CornerRadius();
            textBoxRadius = comboBoxControl7.CornerRadius();
        }

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

bool ComboBoxHelper::IsPopupOpenDown(const winrt::ComboBox& comboBox)
{
    double verticalOffset = 0;
    if (auto popupBorder = GetTemplateChildT<winrt::Border>(c_popupBorderName, comboBox))
    {
        if (auto textBox = GetTemplateChildT<winrt::TextBox>(c_editableTextName, comboBox))
        {
            auto transform = popupBorder.TransformToVisual(textBox);
            auto popupTop = transform.TransformPoint(winrt::Point(0, 0));
            verticalOffset = popupTop.Y;
        }
    }
    return verticalOffset > 0;
}
