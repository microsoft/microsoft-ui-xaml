// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlName.h"

class XamlPropertyName final : public XamlName
{
    friend class XamlPropertyNameParser;
public:
    static HRESULT Parse(
        _In_ const xstring_ptr_view& spPrefix,
        _In_ const xstring_ptr_view& spLongName,
        _Out_ std::shared_ptr<XamlPropertyName>& spPropertyName);

    HRESULT get_ScopedName(_Out_ xstring_ptr* pstrOutScopedName) override;
    HRESULT get_ScopedOwnerName(_Out_ xstring_ptr* pstrOutScopedOwnerName);
    HRESULT get_FullName(_Out_ xstring_ptr* pstrOutFullName);
    HRESULT get_OwnerName(_Out_ xstring_ptr* pstrOutOwnerName);
    HRESULT get_OwnerPrefix(_Out_ xstring_ptr* pstrOutOwnerPrefix);
    HRESULT get_Owner(std::shared_ptr<XamlName>& outOwner);
    HRESULT get_IsDotted(bool& isDotted);

    XamlPropertyName(_In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName);
    XamlPropertyName(const std::shared_ptr<XamlName>& inOwner, _In_ const xstring_ptr& inName);
    XamlPropertyName(_In_ const xstring_ptr& inName);
    XamlPropertyName(const std::shared_ptr<XamlName>& inOwner, _In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName);

private:
    void Init(const std::shared_ptr<XamlName>& inOwner, _In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName);

    bool PrivateIsDotted()         {   return !!m_Owner; }

    std::shared_ptr<XamlName> m_Owner;
    xstring_ptr m_ssFullName;
    xstring_ptr m_ssScopedName;

};


class XamlPropertyNameParser : public XamlNameParser
{
public:
    XamlPropertyNameParser(_In_ const xstring_ptr_view& input)
        : XamlNameParser(input)
    {
    }

    HRESULT ParseXamlPropertyName(_In_ const xstring_ptr_view& inPrefix, std::shared_ptr<XamlPropertyName>& outPropertyName);
    HRESULT ParseXamlPropertyName(std::shared_ptr<XamlPropertyName>& outPropertyName)
    {
        xstring_ptr nullPrefix;
        return ParseXamlPropertyName(nullPrefix, outPropertyName);
    }

private:
    bool ParseProperty(_Out_ xephemeral_string_ptr* pstrOutssOwner, _Out_ xephemeral_string_ptr* pstrOutssProperty);

};

