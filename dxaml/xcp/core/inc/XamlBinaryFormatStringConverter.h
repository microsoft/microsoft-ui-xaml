// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlBinaryFormatStringConverter final
{
public:
    XamlBinaryFormatStringConverter(_Const_ _In_ const CCoreServices* core)
        : m_core(core)
    {}

    bool TryConvertValue(_Const_ _In_ const XamlTypeToken typeConverterToken, _Inout_ CValue& valueContainer);

private:
    const CCoreServices* m_core;
};

