// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

class CString final
    : public CDependencyObject
{
public:
    CString(
        _In_ CCoreServices *pCore,
        _In_ const xstring_ptr& strString
        )
        : CDependencyObject(pCore)
        , m_strString(strString)
    {
    }

    CString(
        _In_ CCoreServices *pCore,
        _Inout_ xstring_ptr&& strString
        )
        : CDependencyObject(pCore)
        , m_strString(std::forward<xstring_ptr>(strString))
    {
    }

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
                          _In_ CREATEPARAMETERS *pCreate);

    static _Check_return_ HRESULT CreateFromXStringPtr(_In_ CCoreServices *pCore, _Inout_ xstring_ptr&& strString, _Outptr_ CDependencyObject **ppObject);

    KnownTypeIndex GetTypeIndex() const override;

    bool DoesAllowMultipleAssociation() const override { return true; }
   _Check_return_ HRESULT GetRawStringClone(_Outptr_result_maybenull_z_ WCHAR** ppString);

    xstring_ptr m_strString;
};
