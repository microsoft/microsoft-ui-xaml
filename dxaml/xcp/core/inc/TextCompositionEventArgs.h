// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTextCompositionEventArgsBase : public CEventArgs
{
public:
    CTextCompositionEventArgsBase()
    {
        m_startIndex = -1;
        m_length = 0;
    }

    ~CTextCompositionEventArgsBase() override
    {
    }

    virtual _Check_return_ HRESULT get_StartIndex (_Out_ INT* pValue)
    {
        *pValue = m_startIndex;
        return S_OK;
    }

    virtual _Check_return_ HRESULT put_StartIndex(_In_ INT value)
    {
        m_startIndex = value;
        return S_OK;
    }

    virtual _Check_return_ HRESULT get_Length (_Out_ INT* pValue)
    {
        *pValue = m_length;
        return S_OK;
    }

    virtual _Check_return_ HRESULT put_Length(_In_ INT value)
    {
        m_length = value;
        return S_OK;
    }

    // CTextCompositionEventArgsBase fields
    INT m_startIndex;
    INT m_length;
};

class CTextCompositionStartedEventArgs : public CTextCompositionEventArgsBase
{
public:
    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;
};

class CTextCompositionChangedEventArgs : public CTextCompositionEventArgsBase
{
public:
    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;
};

class CTextCompositionEndedEventArgs : public CTextCompositionEventArgsBase
{
public:
    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;
};


