// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CErrorEventArgs : public CEventArgs
{
public:

    ::ErrorType  m_eType;                      // Error Type.
    XUINT32    m_iErrorCode;                 // Error ID
    HRESULT    m_hResult;                    // Error HRESULT
    CCoreServices* m_pCore;

    xstring_ptr m_strErrorMessage;

    CErrorEventArgs(_In_ CCoreServices* pCore)
    {
        m_pCore = pCore;
        m_eType = UnknownError;
        m_iErrorCode = 0;
        m_hResult = S_OK;
    }

    // Destructor
    ~CErrorEventArgs() override
    {
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::ErrorEventArgs;
    }

    bool IsErrorEventArgs() const override
    {
        return true;
    }

    _Check_return_ HRESULT UpdateErrorMessage(XINT32 bReplaceExistingMessage = FALSE);
};
