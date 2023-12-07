// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <strsafe.h>
#include <XStringUtils.h>
#include <XStringBuilder.h>
#include <TextSurrogates.h>
#include <wil\Result.h>

// Trims characters from the end of the string
// Trimmed characters are replaced with ellipsis
// maxLength parameter specifies the length of the output string (including ellipsis)
//
//  Returns: S_OK if trimming was successful
//           Returned string (*ppResult) must be deleted by the caller
_Check_return_ HRESULT TrimStringEndWithEllipsis(
    _In_ const xstring_ptr_view& strInput, _In_ uint32_t maxLength, _Out_ xstring_ptr* pstrResult)
{
    HRESULT hr = S_OK;

    if (strInput.GetCount() <= maxLength)
    {
        IFC(strInput.Promote(pstrResult));
    }
    else
    {
        // Since ellipsis takes up 3 chars, we wouldn't get meaningful string of 4 chars or less
        // so if maxLength is too short, we will just trim the string, but won't add ellipsis
        if (maxLength < 5)
        {
            const WCHAR* pszLastChar = strInput.GetBuffer() + maxLength - 1;
            if (maxLength > 0 && IS_LEADING_SURROGATE(*pszLastChar))
            {
                --maxLength;
            }
            IFC(xstring_ptr::CloneBuffer(strInput.GetBuffer(), maxLength, pstrResult));
        }
        else
        {
            XStringBuilder resultBuilder;

            IFC(resultBuilder.Initialize(maxLength));

            const WCHAR* pszLastChar = strInput.GetBuffer() + (maxLength - 3) - 1;
            if (IS_LEADING_SURROGATE(*pszLastChar))
            {
                --maxLength;
            }

            IFC(resultBuilder.Append(strInput.GetBuffer(), maxLength - 3));
            IFC(resultBuilder.Append(L"...", 3));

            IFC(resultBuilder.DetachString(pstrResult));
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        pstrResult->Reset();
    }
    return hr;
}

// Trims characters from the start of the string
// Trimmed characters are replaced with ellipsis
// maxLength parameter specifies the length of the output string (including ellipsis)
//
//  Returns: S_OK if trimming was successful
//           Returned string (*ppResult) must be deleted by the caller
_Check_return_ HRESULT TrimStringStartWithEllipsis(
    _In_ const xstring_ptr_view& strInput, _In_ uint32_t maxLength, _Out_ xstring_ptr* pstrResult)
{
    HRESULT hr = S_OK;

    if (strInput.GetCount() <= maxLength)
    {
        IFC(strInput.Promote(pstrResult));
    }
    else
    {
        // Since ellipsis takes up 3 chars, we wouldn't get meaningful string of 4 chars or less
        // so if maxLength is too short, we will just trim the string, but won't add ellipsis
        if (maxLength < 5)
        {
            const WCHAR* pszSubstr = strInput.GetBuffer() + strInput.GetCount() - maxLength;
            if (maxLength > 0 && IS_TRAILING_SURROGATE(*pszSubstr))
            {
                ++pszSubstr;
                --maxLength;
            }
            IFC(xstring_ptr::CloneBuffer(pszSubstr, maxLength, pstrResult));
        }
        else
        {
            XStringBuilder resultBuilder;

            IFC(resultBuilder.Initialize(maxLength));

            IFC(resultBuilder.Append(L"...", 3));

            const WCHAR* pszSubstr = strInput.GetBuffer() + strInput.GetCount() - (maxLength - 3);

            if (IS_TRAILING_SURROGATE(*pszSubstr))
            {
                ++pszSubstr;
                --maxLength;
            }

            IFC(resultBuilder.Append(pszSubstr, maxLength - 3));

            IFC(resultBuilder.DetachString(pstrResult));
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        pstrResult->Reset();
    }
    return hr;
}


char* WideCharToChar(
    _In_ uint32_t cstr,
    _In_reads_(cstr) const WCHAR* pstr,
    _Inout_opt_ uint32_t *pcstr,
    _In_reads_opt_(*pcstr) char* pstrDest
)
{
    char *pRet = nullptr;
    unsigned int nLen;

    // pstrDest nullptr means we need to calculate the destination 
    // string length
    if (pstrDest == nullptr)
    {

        nLen = ::WideCharToMultiByte(CP_UTF8, 0,
            pstr, cstr,
            nullptr, 0,
            nullptr, nullptr);

        // if we have pcstr, we need to populate it with
        // desired destination length and return.
        if (nullptr != pcstr)
        {
            *pcstr = nLen;
            goto Cleanup;
        }

        pRet = new char[nLen + 1];
    }
    else
    {
        pRet = pstrDest;
        nLen = (*pcstr) - 1;
    }

    if (pRet)
    {

        // WideCharToMultiByte is called twice, once to get the length that need to be allocated and a second time to do the work
#pragma warning(push)
#pragma warning (disable : 26000)
#pragma warning (disable : 26014)
        nLen = ::WideCharToMultiByte(CP_UTF8, 0,
            pstr, cstr,
            pRet, nLen + 1,
            nullptr, nullptr);
        // Ensure we always nullptr terminate
        pRet[nLen] = '\0';
        // Return the length of the converted string
        if (pcstr)
        {
            *pcstr = nLen;
        }
#pragma warning(pop)
    }

Cleanup:
    return pRet;
}

WCHAR* CharToWideChar(
    _Inout_ uint32_t* cstr,
    _In_reads_(*cstr) const char* pstr
)
{
    int nLen = ::MultiByteToWideChar(CP_UTF8, 0, pstr, *cstr, NULL, NULL);

    // use the platform to allocate because we will
    // Release it by the same way in CWinPlatformUtilities::XStrFree
    WCHAR* wstr = nullptr;
    if (nLen)
    {
        wstr = new WCHAR[nLen + 1];

        if (wstr != nullptr)
        {
            ::MultiByteToWideChar(CP_UTF8, 0, pstr, *cstr, wstr, nLen);
            wstr[nLen] = L'\0';
            *cstr = nLen;
        }
    }

    return wstr;
}

// Wrapper around StringCchPrintfW that returns an xstring_ptr containing the specified
// formatted data (or throws E_UNEXPECTED if the desired string is too long)
xstring_ptr StringCchPrintfWWrapper(_In_z_ const wchar_t* format, ...)
{
    xstring_ptr retval;
    va_list args;
    va_start(args, format);

    // Value returned by _vscwprintf() does not include terminating null
    auto count = _vscwprintf(format, args);

    if (count <= 0 || count > STRSAFE_MAX_CCH)
    {
        VERIFYHR(E_UNEXPECTED);
    }

    wchar_t* pBuffer = nullptr;
    XStringBuilder stringBuilder;
    // XStringBuilder::InitializeAndGetFixedBuffer() adds '1' to 'count' to account for terminating null
    VERIFYHR(stringBuilder.InitializeAndGetFixedBuffer(count, &pBuffer));
    VERIFYHR(StringCchVPrintf(pBuffer, static_cast<size_t>(count + 1), format, args));
    VERIFYHR(stringBuilder.DetachString(&retval));

    va_end(args);

    return retval;
}