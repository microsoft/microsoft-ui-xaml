// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProvideValueTargetProperty.g.h"

class XamlProperty;

namespace DirectUI
{
    // It is not recommended that instances of this class be instantiated without use of ctl::make
    PARTIAL_CLASS(ProvideValueTargetProperty)
    {
    public:
        _Check_return_ HRESULT get_DeclaringTypeImpl(_Out_ wxaml_interop::TypeName* pValue);
        _Check_return_ HRESULT get_NameImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT get_TypeImpl(_Out_ wxaml_interop::TypeName* pValue);

        // Used to support instantiation and initialization via ctl::make
        _Check_return_ HRESULT Initialize(std::shared_ptr<XamlProperty> xamlProperty);

    private:
        std::shared_ptr<XamlProperty> m_xamlProperty;
    };
}