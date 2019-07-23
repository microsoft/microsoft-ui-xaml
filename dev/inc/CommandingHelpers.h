// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace CommandingHelpers
{
#ifdef ICONSOURCE_INCLUDED
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

    // For downlevel reasons, IconSource is defined both in WUX and in MUX, because SwipeControl relies on it
    // and SwipeControl needs to work downlevel.  As a result, however, we can run into problems when we have
    // types like UICommand that reference IconSource but only exist in WUX - in those circumstances,
    // that type will always reference the WUX IconSource even if the rest of our MUX code is using the MUX IconSource.
    // For that reason, we need to be able to convert from the WUX IconSource to the MUX IconSource to ensure
    // that we're always using the correct type.
    class WUXIconSourceToMUXIconSourceConverter : public winrt::implements<WUXIconSourceToMUXIconSourceConverter, winrt::IValueConverter>
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

#endif

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
}  // namespace CommandingHelpers
