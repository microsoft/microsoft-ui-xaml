// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "CommandingHelpers.h"

winrt::IInspectable CommandingHelpers::IconSourceToIconSourceElementConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    if (value)
    {
        winrt::IconSource iconSource{ value.as<winrt::IconSource>() };
        winrt::IconSourceElement iconSourceElement;
        iconSourceElement.IconSource(iconSource);
        return iconSourceElement;
    }

    return nullptr;
}

winrt::IInspectable CommandingHelpers::IconSourceToIconSourceElementConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language) noexcept
{
    winrt::throw_hresult(E_NOTIMPL);
}

void CommandingHelpers::BindToIconSourcePropertyIfUnset(
    winrt::XamlUICommand const& uiCommand,
    winrt::DependencyObject const& target,
    winrt::DependencyProperty const& iconSourceProperty)
{
    winrt::IconSource localIconSource = target.ReadLocalValue(iconSourceProperty).try_as<winrt::IconSource>();

    if (!localIconSource)
    {
        SharedHelpers::SetBinding(uiCommand, L"IconSource", target, iconSourceProperty);
    }
}

void CommandingHelpers::BindToIconPropertyIfUnset(
    winrt::XamlUICommand const& uiCommand,
    winrt::DependencyObject const& target,
    winrt::DependencyProperty const& iconProperty)
{
    if (!target.ReadLocalValue(iconProperty).try_as<winrt::IconElement>())
    {
        SharedHelpers::SetBinding(uiCommand, L"Icon", target, iconProperty, winrt::make<IconSourceToIconSourceElementConverter>());
    }
}

void CommandingHelpers::BindToLabelPropertyIfUnset(
    winrt::XamlUICommand const& uiCommand,
    winrt::DependencyObject const& target,
    winrt::DependencyProperty const& labelProperty)
{
    auto labelReference = target.ReadLocalValue(labelProperty).try_as<winrt::IReference<winrt::hstring>>();

    if (!labelReference || labelReference.Value().empty())
    {
        SharedHelpers::SetBinding(uiCommand, L"Label", target, labelProperty);
    }
}



void CommandingHelpers::BindToKeyboardAcceleratorsIfUnset(
    winrt::XamlUICommand const& uiCommand,
    winrt::UIElement const& target)
{
    if (target.KeyboardAccelerators().Size() == 0)
    {
        // Keyboard accelerators can't have two parents, so we'll need to copy them
        // and bind to the original properties instead of assigning them.
        // We set up bindings so that modifications to the app-defined accelerators
        // will propagate to the accelerators that are used by the framework.
        for (winrt::KeyboardAccelerator keyboardAccelerator : uiCommand.KeyboardAccelerators())
        {
            winrt::KeyboardAccelerator keyboardAcceleratorCopy;

            SharedHelpers::SetBinding(keyboardAccelerator, L"IsEnabled", keyboardAcceleratorCopy, winrt::KeyboardAccelerator::IsEnabledProperty());
            SharedHelpers::SetBinding(keyboardAccelerator, L"Key", keyboardAcceleratorCopy, winrt::KeyboardAccelerator::KeyProperty());
            SharedHelpers::SetBinding(keyboardAccelerator, L"Modifiers", keyboardAcceleratorCopy, winrt::KeyboardAccelerator::ModifiersProperty());
            SharedHelpers::SetBinding(keyboardAccelerator, L"ScopeOwner", keyboardAcceleratorCopy, winrt::KeyboardAccelerator::ScopeOwnerProperty());
            target.KeyboardAccelerators().Append(keyboardAcceleratorCopy);
        }
    }
}

void CommandingHelpers::BindToAccessKeyIfUnset(
    winrt::XamlUICommand const& uiCommand,
    winrt::UIElement const& target)
{
    if (target.AccessKey().empty())
    {
        SharedHelpers::SetBinding(uiCommand, L"AccessKey", target, winrt::UIElement::AccessKeyProperty());
    }
}

void CommandingHelpers::BindToDescriptionPropertiesIfUnset(
    winrt::XamlUICommand const& uiCommand,
    winrt::DependencyObject const& target)
{
    if (winrt::AutomationProperties::GetHelpText(target).empty())
    {
        SharedHelpers::SetBinding(uiCommand, L"Description", target, winrt::AutomationProperties::HelpTextProperty());
    }

    winrt::IInspectable localToolTipAsI = winrt::ToolTipService::GetToolTip(target);
    auto localToolTipAsString = localToolTipAsI.try_as<winrt::IReference<winrt::hstring>>();

    if ((!localToolTipAsString || localToolTipAsString.Value().empty()) && !localToolTipAsI.try_as<winrt::ToolTip>())
    {
        SharedHelpers::SetBinding(uiCommand, L"Description", target, winrt::ToolTipService::ToolTipProperty());
    }
}

void CommandingHelpers::ClearBindingIfSet(
    winrt::XamlUICommand const& uiCommand,
    winrt::FrameworkElement const& target,
    winrt::DependencyProperty const& targetProperty)
{
    if (auto bindingExpression = target.GetBindingExpression(targetProperty))
    {
        if (auto parentBinding = bindingExpression.ParentBinding())
        {
            if (auto source = parentBinding.Source())
            {
                if (source == uiCommand)
                {
                    target.ClearValue(targetProperty);
                }
            }
        }
    }
}
