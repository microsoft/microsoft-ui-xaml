// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <stdint.h>

template <uint32_t Ti>
class TStringBuilder
{
public:
    TStringBuilder();
    ~TStringBuilder();

    static const uint32_t npos = static_cast<uint32_t>(-1);

private:
    WCHAR* m_pBuffer;
    HSTRING_BUFFER m_hPreallocatedBuffer;
    uint32_t m_cBuffer;
    uint32_t m_cActual;
    WCHAR m_inlineBuffer[Ti]{};

public:
    _Check_return_ HRESULT Initialize(uint32_t uInitialSize = Ti);
    _Check_return_ HRESULT Initialize(_In_ const xstring_ptr_view& strString);
    _Check_return_ HRESULT Initialize(_In_reads_(cBuffer) const WCHAR* pBuffer, uint32_t cBuffer);
    _Check_return_ HRESULT Append(_In_ const xstring_ptr_view& spstrToAppend);
    _Check_return_ HRESULT Append(_In_reads_(cBuffer) const WCHAR* pBuffer, uint32_t cBuffer);
    _Check_return_ HRESULT AppendChar(const WCHAR ch);
    _Check_return_ HRESULT TrimWhitespace();
    _Check_return_ HRESULT ShiftLeft(uint32_t uAmount);
    _Check_return_ HRESULT DetachString(_Out_ xstring_ptr* pstrDetachedString);
    _Check_return_ HRESULT Reset(uint32_t uInitialSize = Ti, bool bForceShrink = false);
    WCHAR GetChar(uint32_t index) const
    {
        return m_pBuffer[index];
    }
    const WCHAR* GetBuffer() const { return m_pBuffer; }
    WCHAR* GetMutableBuffer() { return m_pBuffer; }
    uint32_t GetCount() const { return m_cActual; }

    _Check_return_ HRESULT InitializeAndGetFixedBuffer(
        uint32_t count,
        _Outptr_result_buffer_(count) WCHAR** ppBuffer
        );

    void SetFixedBufferCount(
        uint32_t count
        );

    bool IsNullTerminated() const
    {
        return m_cActual == 0 || m_pBuffer[m_cActual] == L'\0';
    }

    // Looks for the first occurrence of the specified character in this
    // string view.
    // Follows the same pattern as std::basic_string::rfind()
    // If found, returns an offset from the start of the string.
    // If startingIndex >= size(), returns npos.
    uint32_t FindChar(
        WCHAR ch,
        uint32_t startingIndex = 0) const;

    _Check_return_ HRESULT Remove(uint32_t ichStart, uint32_t cchRemove);

    void Replace(
        _In_ const WCHAR oldChar,
        _In_ const WCHAR newChar
        );

private:
    _Check_return_ HRESULT EnsureBufferForAdding(uint32_t uNumberOfCharsToBeAdded);
    _Check_return_ HRESULT ExpandBuffer(uint32_t uNewBufferSize);

    static bool IsWhitespaceChar(const WCHAR& ch)
    {
        // TODO: This is duplicated in the xamlscanner.
        // TODO: Also we have a constant xiswhite lookup.
        // TODO: Also in string builder.
        return (ch == L' ') || (ch == L'\n') || (ch == L'\r') || (ch == L'\t');
    }
};

typedef TStringBuilder<96> XStringBuilder;
typedef TStringBuilder<16> XSmallStringBuilder;

