// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ComboBoxHelper.h"
#include "DispatcherHelper.h"
#include "CornerRadiusFilterConverter.h"
#include "ResourceAccessor.h"

static constexpr auto c_popupBorderName = L"PopupBorder"sv;
static constexpr auto c_editableTextName = L"EditableText"sv;
static constexpr auto c_editableTextBorderName = L"BorderElement"sv;
static constexpr auto c_controlCornerRadiusKey = L"ControlCornerRadius"sv;
static constexpr auto c_overlayCornerRadiusKey = L"OverlayCornerRadius"sv;
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
    ComboBoxHelperProperties::ClearProperties();
}


// Normal ComboBox and editable ComboBox have different CornerRadius behaviors.
// Xaml is not lifted yet when we implementing this feature so we don't have access to ComboBox code.
// Creating this attached property to help us plug in some extra logic without touching the actual ComboBox code.
void ComboBoxHelper::OnKeepInteriorCornersSquarePropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto comboBox = sender.try_as<winrt::ComboBox>())
    {
        bool shouldMonitorDropDownState = unbox_value<bool>(args.NewValue());
        if (shouldMonitorDropDownState)
        {
            auto revokersInspectable = winrt::make<ComboBoxDropDownEventRevokers>();
            auto revokers = revokersInspectable.as<ComboBoxDropDownEventRevokers>();

            revokers->m_dropDownOpenedRevoker = comboBox.DropDownOpened(winrt::auto_revoke, ComboBoxHelper::OnDropDownOpened);
            revokers->m_dropDownClosedRevoker = comboBox.DropDownClosed(winrt::auto_revoke, ComboBoxHelper::OnDropDownClosed);

            comboBox.SetValue(DropDownEventRevokersProperty(), revokersInspectable);
        }
        else
        {
            comboBox.SetValue(DropDownEventRevokersProperty(), nullptr);
        }
    }
}

void ComboBoxHelper::OnDropDownOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    auto comboBox = sender.as<winrt::ComboBox>();
    // We need to know whether the dropDown opens above or below the ComboBox in order to update corner radius correctly.
        // Sometimes TransformToPoint value is incorrect because popup is not fully opened when this function gets called.
        // Use dispatcher to make sure we get correct VerticalOffset.
    DispatcherHelper dispatcherHelper;
    dispatcherHelper.RunAsync([comboBox]()
        {
            UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/true);
        });
}

void ComboBoxHelper::OnDropDownClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    auto comboBox = sender.as<winrt::ComboBox>();
    UpdateCornerRadius(comboBox, /*IsDropDownOpen=*/false);
}

void ComboBoxHelper::UpdateCornerRadius(const winrt::ComboBox& comboBox, bool isDropDownOpen)
{
    if (comboBox.IsEditable())
    {
        auto textBoxRadius = unbox_value<winrt::CornerRadius>(ResourceAccessor::ResourceLookup(comboBox, box_value(c_controlCornerRadiusKey)));
        auto popupRadius = unbox_value<winrt::CornerRadius>(ResourceAccessor::ResourceLookup(comboBox, box_value(c_overlayCornerRadiusKey)));

        if (winrt::IControl7 comboBoxControl7 = comboBox)
        {
            textBoxRadius = comboBoxControl7.CornerRadius();
        }

        if (isDropDownOpen)
        {
            const bool isOpenDown = IsPopupOpenDown(comboBox);
            auto cornerRadiusConverter = winrt::make_self<CornerRadiusFilterConverter>();

            const auto popupRadiusFilter = isOpenDown ? winrt::CornerRadiusFilterKind::Bottom : winrt::CornerRadiusFilterKind::Top;
            popupRadius = cornerRadiusConverter->Convert(popupRadius, popupRadiusFilter);

            const auto textBoxRadiusFilter = isOpenDown ? winrt::CornerRadiusFilterKind::Top : winrt::CornerRadiusFilterKind::Bottom;
            textBoxRadius = cornerRadiusConverter->Convert(textBoxRadius, textBoxRadiusFilter);
        }

        if (auto popupBorder = GetTemplateChildT<winrt::Border>(c_popupBorderName, comboBox))
        {
            popupBorder.CornerRadius(popupRadius);
        }

        if (auto textBox = GetTemplateChildT<winrt::TextBox>(c_editableTextName, comboBox))
        {
            if (winrt::IControl7 textBoxControl7 = textBox)
            {
                textBoxControl7.CornerRadius(textBoxRadius);
            }
            else
            {
                if (auto textBorder = GetTemplateChildT<winrt::Border>(c_editableTextBorderName, textBox))
                {
                    textBorder.CornerRadius(textBoxRadius);
                }
            }
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
            const auto transform = popupBorder.TransformToVisual(textBox);
            const auto popupTop = transform.TransformPoint(winrt::Point(0, 0));
            verticalOffset = popupTop.Y;
        }
    }
    return verticalOffset > 0;
}
