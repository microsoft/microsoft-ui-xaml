// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

//  CColor is created by the XAML parser for <Color> Objects
class CColor : public CDependencyObject
{
private:
    CColor(_In_ CCoreServices *pCore);

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;
    bool DoesAllowMultipleAssociation() const override { return true; }

    static _Check_return_ HRESULT ColorFromString(
        _In_ const xstring_ptr_view& str,
        _Out_ UINT32 *prgbResult);

    // CColor fields
    UINT32 m_rgb;
};

