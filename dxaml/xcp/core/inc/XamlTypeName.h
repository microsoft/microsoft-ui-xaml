// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlName.h"

class XamlTypeName final : public XamlName
{
public:
    // TODO: Make these private?
    XamlTypeName(_In_ const xstring_ptr& inName)
        : XamlName(inName)
    {
    }

    XamlTypeName(
                _In_ const xstring_ptr& inPrefix,
                _In_ const xstring_ptr& inName)
        : XamlName(inPrefix, inName)
    {
    }

    ~XamlTypeName() override
    {
    }

    static HRESULT Parse(
                _In_ const xstring_ptr_view& inLongName,
                std::shared_ptr<XamlTypeName>& outTypeName);

    static HRESULT Parse(
                _In_ const xstring_ptr_view& inPrefix,
                _In_ const xstring_ptr_view& inName,
                std::shared_ptr<XamlTypeName>& outTypeName);

    HRESULT get_ScopedName(_Out_ xstring_ptr* pstrOut) override;

private:
    xstring_ptr m_ssScopedName;
};

class XamlTypeNameParser : public XamlNameParser
{
public:

    XamlTypeNameParser(_In_ const xstring_ptr_view& input)
        : XamlNameParser(input)
    {
    }

    HRESULT ParseXamlTypeName(
            _In_ const xstring_ptr_view& inPrefix,
            std::shared_ptr<XamlTypeName>& outTypeName);

    HRESULT ParseXamlTypeName(
            std::shared_ptr<XamlTypeName>& outTypeName)
    {
        RRETURN(ParseXamlTypeName(xstring_ptr::NullString(), outTypeName));
    }
};



