// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XStringBuilder.h>
#include <xstring_ptr.h>
#include <algorithm>

#undef max

// A reasonable maximum size for buffers that will make integer
// overflow prevention in TStringBuilder easier.
const uint32_t c_MaximumBufferSize = std::numeric_limits<uint32_t>::max() / sizeof(WCHAR);

// TStringBuilder is templated over the size of the initial inline
// buffer. Once the size of that buffer becomes insufficient,
// a Windows Runtime string gets preallocated and populated with
// the contents filled in so far.
template <uint32_t Ti>
TStringBuilder<Ti>::TStringBuilder()
    : m_pBuffer(m_inlineBuffer)
    , m_hPreallocatedBuffer(nullptr)
    , m_cBuffer(Ti)
    , m_cActual(0)
{
}

template <uint32_t Ti>
TStringBuilder<Ti>::~TStringBuilder()
{
    if (m_pBuffer != m_inlineBuffer)
    {
        ASSERT(nullptr != m_hPreallocatedBuffer);

        IGNOREHR(WindowsDeleteStringBuffer(m_hPreallocatedBuffer));

        m_hPreallocatedBuffer = nullptr;
    }

    ASSERT(nullptr == m_hPreallocatedBuffer);

    m_pBuffer = nullptr;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Initialize(
    uint32_t initialBufferSize
    )
{
    IFCEXPECT_RETURN(initialBufferSize < (c_MaximumBufferSize - Ti));

    ++initialBufferSize; // allow for the nullptr terminator

    if (m_pBuffer != m_inlineBuffer)
    {
        // Allow for re-initialization
        ASSERT(nullptr != m_hPreallocatedBuffer);

        IGNOREHR(WindowsDeleteStringBuffer(m_hPreallocatedBuffer));

        m_hPreallocatedBuffer = nullptr;

        m_pBuffer = m_inlineBuffer;
        m_cBuffer = Ti;
    }

    ASSERT(nullptr == m_hPreallocatedBuffer);

    if (initialBufferSize > m_cBuffer)
    {
        // Only allocate on the heap if stack storage is insufficient
        IFC_RETURN(WindowsPreallocateStringBuffer(
            initialBufferSize - 1, // minus one, as WPSB will take care of allocating space for the nullptr terminator
            &m_pBuffer,
            &m_hPreallocatedBuffer));

        m_cBuffer = initialBufferSize;
    }

    m_cActual = 0;

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Initialize(
    _In_ const xstring_ptr_view& strString
    )
{
    uint32_t count;
    const WCHAR* buffer = strString.GetBufferAndCount(&count);

    return Initialize(buffer, count);
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Initialize(
    _In_reads_(cBuffer) const WCHAR* initialBuffer,
    uint32_t initialBufferSize
    )
{
    IFC_RETURN(Initialize(initialBufferSize));

    memcpy(m_pBuffer, initialBuffer, initialBufferSize * sizeof(WCHAR));

    m_cActual = initialBufferSize;

    ASSERT(m_cActual < m_cBuffer);
    m_pBuffer[m_cActual] = L'\0';

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Append(
    const xstring_ptr_view& strToAppend
    )
{
    return Append(strToAppend.GetBuffer(), strToAppend.GetCount());
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Append(
    _In_reads_(cBuffer) const WCHAR* pBuffer,
    uint32_t cBuffer
    )
{
    // If this assert hits you didn't call initialize.
    ASSERT(m_pBuffer != nullptr);
    ASSERT(cBuffer == 0 || pBuffer != nullptr);

    if (cBuffer > 0)
    {
        IFC_RETURN(EnsureBufferForAdding(cBuffer));
        ASSERT(m_cBuffer > m_cActual + cBuffer);
        memcpy(m_pBuffer + m_cActual, pBuffer, cBuffer * sizeof(WCHAR));
        m_cActual += cBuffer;

        ASSERT(m_cBuffer > m_cActual);
        m_pBuffer[m_cActual] = '\0';
    }

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::AppendChar(
    const WCHAR ch
    )
{
    // If this assert hits you didn't call initialize.
    ASSERT(m_pBuffer != nullptr);

    IFC_RETURN(EnsureBufferForAdding(1));
    ASSERT(m_cBuffer > m_cActual);
    m_pBuffer[m_cActual] = ch;
    m_cActual++;

    // Always keep the buffer nullptr-terminated
    ASSERT(m_cBuffer > m_cActual);
    m_pBuffer[m_cActual] = L'\0';

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::DetachString(
    _Out_ xstring_ptr* pstrDetachedString
    )
{
    if (m_cActual == 0)
    {
        ASSERT(nullptr == m_hPreallocatedBuffer);

        pstrDetachedString->Reset();
    }
    else if (m_pBuffer != m_inlineBuffer)
    {
        ASSERT(nullptr != m_hPreallocatedBuffer);

        HSTRING promotedBufferHandle = nullptr;

        IFC_RETURN(WindowsPromoteStringBuffer(
            m_hPreallocatedBuffer,
            &promotedBufferHandle));

        m_hPreallocatedBuffer = nullptr; // ownership of the buffer is with the HSTRING now

        HRESULT hrClone;

        if (WindowsGetStringLen(promotedBufferHandle) == m_cActual)
        {
            hrClone = xstring_ptr::CloneRuntimeStringHandle(
                promotedBufferHandle,
                pstrDetachedString);
        }
        else
        {
            hrClone = xstring_ptr::CloneBuffer(
                m_pBuffer,
                m_cActual,
                pstrDetachedString);
        }

        IGNOREHR(WindowsDeleteString(promotedBufferHandle)); // ownership of the HSTRING is with the xstring_ptr now, or we have cloned the buffer

        IFC_RETURN(hrClone);
    }
    else
    {
        ASSERT(nullptr == m_hPreallocatedBuffer);

        IFC_RETURN(xstring_ptr::CloneBuffer(
            m_pBuffer,
            m_cActual,
            pstrDetachedString));
    }

    ASSERT(nullptr == m_hPreallocatedBuffer);

    m_pBuffer = m_inlineBuffer;
    m_cBuffer = Ti;
    m_cActual = 0;

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Reset(
    uint32_t uInitialSize,
    bool bForceShrink
    )
{
    if (bForceShrink && (m_cBuffer > uInitialSize))
    {
        // If we've already given away the buffer, we
        // have to create a new one regardless.
        //
        // Also if it's bigger than the initial size, and we want to forcibly
        // make the buffer the minimum size.
        IFC_RETURN(Initialize(uInitialSize));
    }
    else
    {
        m_cActual = 0;
    }

    return S_OK;

}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::InitializeAndGetFixedBuffer(
    uint32_t count,
    _Outptr_result_buffer_(count) WCHAR** ppBuffer
    )
{
    IFC_RETURN(Initialize(count + 1));

    ASSERT(m_cBuffer > m_cActual + count);

    m_pBuffer[count] = L'\0';
    m_cActual = count;
    *ppBuffer = m_pBuffer;

    return S_OK;
}

template <uint32_t Ti> void
TStringBuilder<Ti>::SetFixedBufferCount(
    uint32_t count
    )
{
    ASSERT(count < m_cBuffer);
    m_cActual = count;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::EnsureBufferForAdding(
    uint32_t uNumberOfCharsToBeAdded
    )
{
    // Always keep one character slot for the nullptr terminator
    const uint32_t actualRequested = uNumberOfCharsToBeAdded + m_cActual + 1;

    IFCEXPECT_RETURN(
           uNumberOfCharsToBeAdded + m_cActual >= m_cActual
        && actualRequested >= m_cActual); // protect from add overflow

    if (actualRequested >= m_cBuffer)
    {
        uint32_t bufferRequested;

        if (uNumberOfCharsToBeAdded >= m_cBuffer)
        {
            // Make it big enough to fit with m_cBuffer extra space.
            const uint32_t paddingSize = (m_cBuffer == 0) ? Ti : m_cBuffer;

            bufferRequested = actualRequested + paddingSize;
        }
        else
        {
            IFCEXPECT_RETURN(m_cBuffer < (std::numeric_limits<uint32_t>::max() / 2));

            bufferRequested = m_cBuffer * 2;
        }

        IFCEXPECT_RETURN(bufferRequested >= actualRequested);

        IFC_RETURN(ExpandBuffer(bufferRequested));
    }

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::ExpandBuffer(
    uint32_t uNewBufferSize
    )
{
    WCHAR* pNewBuffer = nullptr;
    HSTRING_BUFFER hNewPreallocatedBuffer = nullptr;

    ASSERT(uNewBufferSize > m_cBuffer
        && m_cBuffer > m_cActual);

    IFC_RETURN(WindowsPreallocateStringBuffer(
        uNewBufferSize - 1, // minus one, as WPSB will take care of allocating space for the nullptr terminator
        &pNewBuffer,
        &hNewPreallocatedBuffer));

    memcpy(pNewBuffer, m_pBuffer, m_cActual * sizeof(WCHAR));
    pNewBuffer[m_cActual] = L'\0';

    if (m_pBuffer != m_inlineBuffer)
    {
        ASSERT(nullptr != m_hPreallocatedBuffer);

        IGNOREHR(WindowsDeleteStringBuffer(m_hPreallocatedBuffer));
    }

    m_pBuffer = pNewBuffer;
    m_cBuffer = uNewBufferSize;
    m_hPreallocatedBuffer = hNewPreallocatedBuffer;

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::ShiftLeft(
    uint32_t uAmount
    )
{
    if (uAmount < m_cActual)
    {
        uint32_t uNumberOfCharsLeftToMoveIncludingnullptr = (m_cActual - uAmount) + 1;
        memmove(m_pBuffer, &(m_pBuffer[uAmount]), uNumberOfCharsLeftToMoveIncludingnullptr * sizeof(WCHAR));
        m_cActual -= uAmount;
    }
    else
    {
        m_cActual = 0;
    }

    return S_OK;
}

template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::TrimWhitespace()
{
    uint32_t uTrimFront = 0;
    uint32_t uTrimBack = 0;

    if (m_cActual > 0)
    {
        for (uint32_t i = 0; i < m_cActual; i++)
        {
            if (!IsWhitespaceChar(m_pBuffer[i]))
            {
                break;
            }

            uTrimFront++;
        }

        for (XINT32 i = m_cActual - 1; i >= 0; i--)
        {
            if (!IsWhitespaceChar(m_pBuffer[i]))
            {
                break;
            }

            uTrimBack++;
        }
    }

    // avoid potential underflow if string is entirely whitespace (whitespace gets counted twice)
    // also avoid overflow on add of uTrimFront + uTrimBack by subtracting.
    IFCEXPECT_RETURN(m_cActual - uTrimFront > uTrimBack);

    // TODO: Maybe use ShiftLeft.
    m_cActual -= (uTrimFront + uTrimBack);

    if (uTrimFront > 0)
    {
        memmove(m_pBuffer, &(m_pBuffer[uTrimFront]), m_cActual * sizeof(WCHAR));
    }

    return S_OK;
}

template <uint32_t Ti>
uint32_t TStringBuilder<Ti>::FindChar(
    WCHAR ch,
    uint32_t startingIndex /* = 0 */
    ) const
{
    const auto thisBuffer = GetBuffer();
    const auto thisCount = GetCount();

    if (startingIndex < thisCount)
    {
        const auto end = thisBuffer + thisCount;
        auto iter = std::find(thisBuffer + startingIndex, end, ch);
        if (iter != end)
        {
            return static_cast<uint32_t>(iter - thisBuffer);
        }
    }

    // No match found
    return TStringBuilder::npos;
}


// Removes some portion of a source string at a specific index
// in the current string starting from the specified character index
// of the source string.
//
// Parameters:
//     ichStart        --  index at which to start removing characters.
//     cchRemove       --  count of characters to remove.
template <uint32_t Ti> _Check_return_ HRESULT
TStringBuilder<Ti>::Remove(uint32_t ichStart, uint32_t cchRemove
    )
{
    // too many characters to remove given the remaining characters
    // in the buffer.
    if (ichStart + cchRemove > GetCount())
    {
        IFC_RETURN(E_FAIL);
    }

    memmove(&m_pBuffer[ichStart],
            &m_pBuffer[ichStart + cchRemove],
            (GetCount() - (ichStart + cchRemove)) * sizeof(WCHAR));

    m_cActual -= cchRemove;
    m_pBuffer[m_cActual] = '\0';

    return S_OK;
}

template <uint32_t Ti> void
TStringBuilder<Ti>::Replace(
    _In_ const WCHAR oldChar,
    _In_ const WCHAR newChar
    )
{
    for (uint32_t i = 0; i < m_cActual; i++)
    {
        if (m_pBuffer[i] == oldChar)
        {
            m_pBuffer[i] = newChar;
        }
    }
}

// Instantiate the templates for XStringBuilder and XSmallStringBuilder
template class TStringBuilder<16>;
template class TStringBuilder<96>;

