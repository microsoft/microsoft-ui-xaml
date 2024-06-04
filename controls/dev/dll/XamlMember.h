// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlMember :
    public winrt::implements<XamlMember, winrt::IXamlMember>
{
public:
    XamlMember(
        wstring_view const& memberName,
        const winrt::IXamlType& type,
        std::function<winrt::IInspectable(const winrt::IInspectable&)> getter,
        std::function<void(const winrt::IInspectable&, const winrt::IInspectable&)> setter,
        bool isDependencyProperty,
        bool isAttachable);

    // IXamlMember
    bool IsAttachable();
    bool IsDependencyProperty();
    bool IsReadOnly();
    winrt::hstring Name();
    winrt::IXamlType TargetType();
    winrt::IXamlType Type();
    winrt::IInspectable GetValue(winrt::IInspectable const& instance);

    void SetValue(winrt::IInspectable const& instance, winrt::IInspectable const& value);

private:

    hstring m_memberName{};
    winrt::IXamlType m_type{};
    bool m_isDependencyProperty{};
    bool m_isAttachable{};
    std::function<winrt::IInspectable(winrt::IInspectable)> m_getter{};
    std::function<void(winrt::IInspectable, winrt::IInspectable)> m_setter{};
};
