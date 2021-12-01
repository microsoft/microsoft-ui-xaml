// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

class CustomProperty :
    public winrt::implements<CustomProperty, winrt::ICustomProperty>
{
public:
    CustomProperty(
        winrt::hstring const& name,
        winrt::TypeName const& typeName,
        std::function<winrt::IInspectable(winrt::IInspectable const&)>,
        std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)> m_setter);

    bool CanRead();
    bool CanWrite();
    winrt::hstring Name();
    winrt::TypeName Type();

    winrt::IInspectable GetIndexedValue(winrt::IInspectable const& target, winrt::IInspectable const& index);
    void SetIndexedValue(winrt::IInspectable const& target, winrt::IInspectable const& value, winrt::IInspectable const& index);

    winrt::IInspectable GetValue(winrt::IInspectable const& target);
    void SetValue(winrt::IInspectable const& target, winrt::IInspectable const& value);

private:
    winrt::hstring m_name{};
    winrt::TypeName m_typeName{};
    std::function<winrt::IInspectable(winrt::IInspectable const&)> m_getter;
    std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)> m_setter;
};