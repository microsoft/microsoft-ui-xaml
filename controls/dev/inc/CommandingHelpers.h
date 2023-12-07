// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace CommandingHelpers
{
    class IconSourceToIconSourceElementConverter : public winrt::implements<IconSourceToIconSourceElementConverter, winrt::IValueConverter>
    {
    public:
        winrt::IInspectable Convert(
            winrt::IInspectable const& value,
            winrt::TypeName const& targetType,
            winrt::IInspectable const& parameter,
            winrt::hstring const& language);

        winrt::IInspectable ConvertBack(
            winrt::IInspectable const& value,
            winrt::TypeName const& targetType,
            winrt::IInspectable const& parameter,
            winrt::hstring const& language) noexcept;
    };

    void BindToIconPropertyIfUnset(
        winrt::XamlUICommand const& uiCommand,
        winrt::DependencyObject const& target,
        winrt::DependencyProperty const& iconProperty);

    void BindToIconSourcePropertyIfUnset(
        winrt::XamlUICommand const& uiCommand,
        winrt::DependencyObject const& target,
        winrt::DependencyProperty const& iconSourceProperty);

    void BindToLabelPropertyIfUnset(
        winrt::XamlUICommand const& uiCommand,
        winrt::DependencyObject const& target,
        winrt::DependencyProperty const& labelProperty);

    void BindToKeyboardAcceleratorsIfUnset(
        winrt::XamlUICommand const& uiCommand,
        winrt::UIElement const& target);

    void BindToAccessKeyIfUnset(
        winrt::XamlUICommand const& uiCommand,
        winrt::UIElement const& target);

    void BindToDescriptionPropertiesIfUnset(
        winrt::XamlUICommand const& uiCommand,
        winrt::DependencyObject const& target);

    void ClearBindingIfSet(
        winrt::XamlUICommand const& uiCommand,
        winrt::FrameworkElement const& target,
        winrt::DependencyProperty const& targetProperty);
};
