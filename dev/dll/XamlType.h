// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlTypeBase :
    public winrt::implements<XamlTypeBase, winrt::IXamlType>
{
public:
    void AddDPMember(
        wstring_view const& name,
        wstring_view const& baseTypeName,
        winrt::DependencyProperty const& dp,
        bool isContent);

    void AddMember(
        wstring_view const& name,
        wstring_view const& baseTypeName,
        std::function<winrt::IInspectable(const winrt::IInspectable&)> getter,
        std::function<void(const winrt::IInspectable&, const winrt::IInspectable&)> setter,
        bool isContent,
        bool isDependencyProperty,
        bool isAttachable);

    // IXamlType
    winrt::IXamlType BaseType();
    winrt::IXamlMember ContentProperty();
    winrt::hstring FullName();
    bool IsArray();
    bool IsCollection();
    bool IsConstructible();
    bool IsDictionary();
    bool IsMarkupExtension();
    bool IsBindable();
    winrt::IXamlType ItemType();
    winrt::IXamlType KeyType();
    winrt::TypeName UnderlyingType();
    winrt::IInspectable ActivateInstance();
    winrt::IInspectable CreateFromString(winrt::hstring const& value);
    winrt::IXamlMember GetMember(winrt::hstring const& name);
    void AddToVector(winrt::IInspectable const& instance, winrt::IInspectable const& value);
    void AddToMap(winrt::IInspectable const& instance, winrt::IInspectable const& key, winrt::IInspectable const& value);
    void RunInitializer();

private:
    void EnsureProperties();

protected:

    hstring m_typeName;
    hstring m_baseTypeName;

    std::function<winrt::IInspectable()> m_activator;
    std::function<void(XamlTypeBase&)> m_populatePropertiesFunc;
    std::function<winrt::IInspectable(hstring)> m_createFromString;
    std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)> m_collectionAdd;
    std::function<void(winrt::IInspectable const&, winrt::IInspectable const&, winrt::IInspectable const&)> m_addToMap;

    winrt::IXamlMember m_contentProperty;
    std::vector<winrt::IXamlMember> m_members{};

    bool m_isSystemType = false;
};

class XamlType : public XamlTypeBase
{
public:
    XamlType(
        wstring_view const& typeName,
        wstring_view const& baseTypeName,
        std::function<winrt::IInspectable()> activator,
        std::function<void(XamlTypeBase&)> populatePropertiesFunc
    );

    void SetCollectionAddFunc(std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)> collectionAdd);
    void SetAddToMapFunc(std::function<void(winrt::IInspectable const&, winrt::IInspectable const&, winrt::IInspectable const&)> addToMap);
};

class EnumXamlType : public XamlTypeBase
{
public:
    EnumXamlType(
        wstring_view const& typeName,
        std::function<winrt::IInspectable(hstring)> createFromString
    );
};

class PrimitiveXamlType : public XamlTypeBase
{
public:
    explicit PrimitiveXamlType(wstring_view const& typeName)
    {
        m_typeName = typeName;
        m_isSystemType = true;
    }
};