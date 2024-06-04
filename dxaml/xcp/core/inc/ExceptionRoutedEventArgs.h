// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CExceptionRoutedEventArgs : public CRoutedEventArgs
{
public:
    CExceptionRoutedEventArgs()
    {
    }

    // Destructor
    ~CExceptionRoutedEventArgs() override
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_ErrorMessage(_Out_ xstring_ptr* pValue)
    {
        *pValue = m_strErrorMessage;
        RRETURN(S_OK);
    }

    xstring_ptr m_strErrorMessage;
};
