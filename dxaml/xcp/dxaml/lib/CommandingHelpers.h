// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides helpers for interop between UICommand and its consumers.

#pragma once

namespace DirectUI
{
    namespace CommandingHelpers
    {
        _Check_return_ HRESULT BindToLabelPropertyIfUnset(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ DependencyObject* target,
            _In_ KnownPropertyIndex labelPropertyIndex);

        _Check_return_ HRESULT BindToIconPropertyIfUnset(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ DependencyObject* target,
            _In_ KnownPropertyIndex iconPropertyIndex);

        _Check_return_ HRESULT BindToIconSourcePropertyIfUnset(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ DependencyObject* target,
            _In_ KnownPropertyIndex iconSourcePropertyIndex);

        _Check_return_ HRESULT BindToKeyboardAcceleratorsIfUnset(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ UIElement* target);

        _Check_return_ HRESULT BindToAccessKeyIfUnset(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ UIElement* target);

        _Check_return_ HRESULT BindToDescriptionPropertiesIfUnset(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ DependencyObject* target);
            
        _Check_return_ HRESULT ClearBindingIfSet(
            _In_ xaml_input::IXamlUICommand* uiCommand,
            _In_ FrameworkElement* target,
            _In_ KnownPropertyIndex labelPropertyIndex);
    };
}

