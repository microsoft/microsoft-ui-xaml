// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "CommandingHelpers.h"

// IconSource is implemented in WUX in the OS repo, so we don't need to
// include IconSource.h on that side.
#ifdef ICONSOURCE_INCLUDED
#include "IconSource.h"

winrt::IInspectable CommandingHelpers::IconSourceToIconSourceElementConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    if (value)
    {
        winrt::IconSource iconSource{ value.as<winrt::IconSource>() };
        winrt::Windows::UI::Xaml::Controls::IconSource wuxIconSource = iconSource.try_as<winrt::Windows::UI::Xaml::Controls::IconSource>();

        if (SharedHelpers::IsIconSourceElementAvailable() && wuxIconSource)
        {
            winrt::IconSourceElement iconSourceElement;
            iconSourceElement.IconSource(wuxIconSource);
            return iconSourceElement;
        }
        else
        {
            iconSource.CreateIconElement();
        }
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

winrt::IInspectable CommandingHelpers::WUXIconSourceToMUXIconSourceConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    if (auto bitmapIconSource = value.try_as<winrt::Windows::UI::Xaml::Controls::BitmapIconSource>())
    {
        winrt::BitmapIconSource returnValue;

        returnValue.Foreground(bitmapIconSource.Foreground());
        returnValue.UriSource(bitmapIconSource.UriSource());
        returnValue.ShowAsMonochrome(bitmapIconSource.ShowAsMonochrome());

        return returnValue;
    }
    else if (auto fontIconSource = value.try_as<winrt::Windows::UI::Xaml::Controls::FontIconSource>())
    {
        winrt::FontIconSource returnValue;

        returnValue.Foreground(fontIconSource.Foreground());
        returnValue.FontFamily(fontIconSource.FontFamily());
        returnValue.FontSize(fontIconSource.FontSize());
        returnValue.FontStyle(fontIconSource.FontStyle());
        returnValue.FontWeight(fontIconSource.FontWeight());
        returnValue.Glyph(fontIconSource.Glyph());
        returnValue.IsTextScaleFactorEnabled(fontIconSource.IsTextScaleFactorEnabled());
        returnValue.MirroredWhenRightToLeft(fontIconSource.MirroredWhenRightToLeft());

        return returnValue;
    }
    else if (auto pathIconSource = value.try_as<winrt::Windows::UI::Xaml::Controls::PathIconSource>())
    {
        winrt::PathIconSource returnValue;

        returnValue.Foreground(pathIconSource.Foreground());
        returnValue.Data(pathIconSource.Data());

        return returnValue;
    }
    else if (auto symbolIconSource = value.try_as<winrt::Windows::UI::Xaml::Controls::SymbolIconSource>())
    {
        winrt::SymbolIconSource returnValue;

        returnValue.Foreground(symbolIconSource.Foreground());
        returnValue.Symbol(symbolIconSource.Symbol());

        return returnValue;
    }

    // We've been passed a MUX icon source if we got here, so just return it verbatim.
    return value;
}

winrt::IInspectable CommandingHelpers::WUXIconSourceToMUXIconSourceConverter::ConvertBack(
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
        SharedHelpers::SetBinding(uiCommand, L"IconSource", target, iconSourceProperty, winrt::make<WUXIconSourceToMUXIconSourceConverter>());
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

#endif

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
