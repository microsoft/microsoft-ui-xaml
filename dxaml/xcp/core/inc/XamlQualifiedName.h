// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlName.h"

class XamlQualifiedName
    final : public XamlName
{
public:
    XamlQualifiedName(
                _In_ const xstring_ptr& inPrefix,
                _In_ const xstring_ptr& inName)
        : XamlName(
                inPrefix,
                inName)
    {
    }

    HRESULT get_ScopedName(_Out_ xstring_ptr* pstrOutPrefix) override;

private:
    xstring_ptr m_ssScopedName;

};
