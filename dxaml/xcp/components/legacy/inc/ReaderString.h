// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// A miserable little class that holds on to pointers to strings returned from
// a XmlLite reader. Note that this class makes no promises w.r.t. lifetime of
// the returned strings.
class ReaderString
{
public:
    ReaderString() :
        m_pString(nullptr),
        m_cString(0)
    {}

    void SetString(_In_ unsigned int cString, 
        _In_reads_(cString+1) const wchar_t* pString)
    {
        m_cString = cString;
        m_pString = pString;
    }

    operator const wchar_t*() const
    {
        return Get();
    }

    const wchar_t* Get() const
    {
        return m_pString;
    }

    unsigned int length() const
    {
        return m_cString;
    }

private:
    const wchar_t* m_pString;
    unsigned int m_cString;
};

