// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

class CCornerRadius : public CDependencyObject
{
private:
    CCornerRadius(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    static _Check_return_ HRESULT Validate(
        _In_ XCORNERRADIUS * pCornerRadius
        );

    static _Check_return_ HRESULT CornerRadiusFromString(
        _In_ UINT32 cString,
        _In_reads_(cString) const WCHAR *pString,
        _Out_ XCORNERRADIUS *peValue
    );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;
    bool DoesAllowMultipleAssociation() const override { return true; }

    XCORNERRADIUS m_cornerRadius = {};
};
