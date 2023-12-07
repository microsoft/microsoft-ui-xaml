// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


//------------------------------------------------------------------------
//
//  Method:   FindHeaderValueInPackedString
//
//  Synopsis: Finds a header value in a packed header string. The packed
//            header string should be in the format passed to 
//            IPALDownloadResponseCallback::GotStream.
//
//            Limitation: does not support multi-line header values.
//
//            strPackedHeaders - Packed header string.
//
//            strHeaderName - Name of header to find, without any prefix or 
//            suffix ("Content-Type", not "\nContent-Type:").
//
//            ichStart - Index into strPackedHeaders to start searching.
//
//            pichEnd - On success, set to an index into strPackedHeaders
//            to start a subsequent search (useful for supporting multiple
//            headers with the same header name). Set to 
//            strPackedHeaders.GetCount() if no more searching should be
//            done on strPackedHeaders (i.e., end of string reached).
//
//            pwszHeaderValue - On success, if a valid header value is found,
//            points to the start of that value inside strPackedHeader's
//            buffer. If a valid header is not found set to NULL. Even if
//            a valid header is not found, there may be more of the string
//            to search, so pichEnd may != strPackedHeaders.GetCount()
//            even if pwszHeaderValue is NULL.
//
//            pcchHeaderValue - Works the same way as pwszHeaderValue,
//            and is the count of the valid header value.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT FindHeaderValueInPackedString(
    _In_ const xstring_ptr_view& strPackedHeaders,
    _In_ const xstring_ptr_view& strHeaderName,
    XUINT32 ichStart,
    _Out_ XUINT32* pichEnd,
    _Outptr_result_buffer_(*pcchHeaderValue + 1) const WCHAR** pwszHeaderValue,
    _Out_ XUINT32* pcchHeaderValue)
{
    XUINT32 cchHeaderName;
    const XUINT32 MAX_HEADER_NAME_LENGTH = 60;
    WCHAR wszHeaderNameSearch[MAX_HEADER_NAME_LENGTH + 3]; // "\n" + header + ":\0"
    XUINT32 cchHeaderNameSearch = 0;
    XUINT32 ichHeaderNameInPackedString = 0;
    XUINT32 ichHeaderValueInPackedString = 0;
    XUINT32 ichHeaderValueTerminatorInPackedString = 0;

    if (strHeaderName.IsNullOrEmpty())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    cchHeaderName = strHeaderName.GetCount();
    if (cchHeaderName > MAX_HEADER_NAME_LENGTH)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFCPTR_RETURN(pichEnd);
    *pichEnd = strPackedHeaders.GetCount();

    IFCPTR_RETURN(pwszHeaderValue);
    *pwszHeaderValue = NULL;

    IFCPTR_RETURN(pcchHeaderValue);
    *pcchHeaderValue = 0;    

    // packed string format:    
    // <status line>[<delimiter><field name>:<field value>]*\0,
    // where <delimiter> is "\n" or "\r\n",
    // and any leading or trailing whitespace for field values should be ignored

    // Build a search string to find the field name (e.g. "\nFoobar:")
    wszHeaderNameSearch[cchHeaderNameSearch++] = L'\n';
    xstrncpy(wszHeaderNameSearch + cchHeaderNameSearch, strHeaderName.GetBuffer(), cchHeaderName);
    cchHeaderNameSearch += cchHeaderName;
    wszHeaderNameSearch[cchHeaderNameSearch++] = L':';
    wszHeaderNameSearch[cchHeaderNameSearch] = NULL;

    // Search for the field name (e.g. "\nFoobar:") in the packed string, case insensitively.
    if (FAILED(strPackedHeaders.Find(
        XSTRING_PTR_EPHEMERAL2(wszHeaderNameSearch, cchHeaderNameSearch),
        ichStart, 
        xstrCompareCaseInsensitive, 
        &ichHeaderNameInPackedString)))
    {
        // header field name not found
        return S_OK;
    }

    ichHeaderValueInPackedString = ichHeaderNameInPackedString + cchHeaderNameSearch;

    if (ichHeaderValueInPackedString == strPackedHeaders.GetCount())
    {
        // malformed packed string, no value (e.g. "Foobar:\0")
        return S_OK;
    }

    // Find the delimiter at the end of the field value.
    ichHeaderValueTerminatorInPackedString = strPackedHeaders.FindChar(L'\n', ichHeaderValueInPackedString);
    if (ichHeaderValueTerminatorInPackedString == xstring_ptr_view::npos)
    {
        // malformed packed string, no value terminator (e.g. "Foobar: value\0")
        return S_OK;
    }

    // Set the end index - a subsequent search for the next instance of this header in the packed string should start here.
    *pichEnd = ichHeaderValueTerminatorInPackedString;

    // The delimiter can be just "\n" or "\r\n".
    if (strPackedHeaders.GetBuffer()[ichHeaderValueTerminatorInPackedString - 1] == L'\r')
    {
        --ichHeaderValueTerminatorInPackedString;
    }

    if (ichHeaderValueTerminatorInPackedString == ichHeaderValueInPackedString)
    {
        // malformed packed string, empty value (e.g. "Foobar:\r\n")
        return S_OK;
    }

    // trim leading and trailing whitespace
    
    while (ichHeaderValueInPackedString < ichHeaderValueTerminatorInPackedString && 
           xisspace(strPackedHeaders.GetBuffer()[ichHeaderValueInPackedString]))
    {
        ++ichHeaderValueInPackedString;
    }

    while (ichHeaderValueInPackedString < ichHeaderValueTerminatorInPackedString && 
           xisspace(strPackedHeaders.GetBuffer()[ichHeaderValueTerminatorInPackedString - 1]))
    {
        --ichHeaderValueTerminatorInPackedString;
    }

    if (ichHeaderValueTerminatorInPackedString == ichHeaderValueInPackedString)
    {
        // value is all whitespace (e.g. "Foobar:    \r\n")
        return S_OK;
    }

    *pwszHeaderValue = strPackedHeaders.GetBuffer() + ichHeaderValueInPackedString;
    *pcchHeaderValue = ichHeaderValueTerminatorInPackedString - ichHeaderValueInPackedString;

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Method:   ExtractFirstHeaderValueFromPackedString
//
//  Synopsis: Extracts the first header value for the specified header name
//            from a packed header string. The packed header string should 
//            be in the format passed to 
//            IPALDownloadResponseCallback::GotStream.
//
//            Limitation: does not support multi-line header values.
//
//            strPackedHeaders - Packed header string.
//
//            strHeaderName - Name of header to find, without any prefix or 
//            suffix ("Content-Type", not "\nContent-Type:").
//
//            strHeaderValue - on success, if a valid header value is 
//            found, will be set to the header value, excluding any leading
//            or trailing whitespace. If the specified header isn't found, 
//            the packed string is malformed, the header value is entirely 
//            whitespace, etc., strHeaderValue will be set to NULL (S_OK 
//            will still be returned).
//
//------------------------------------------------------------------------
extern "C"
_Check_return_ HRESULT ExtractFirstHeaderValueFromPackedString(
    _In_ const xstring_ptr_view& strPackedHeaders,
    _In_ const xstring_ptr_view& strHeaderName,
    _Out_ xstring_ptr* pstrHeaderValue)
{
    XUINT32 ulUnused;
    const WCHAR* wszHeaderValue;
    XUINT32 cchHeaderValue;

    pstrHeaderValue->Reset();

    if (strPackedHeaders.IsNullOrEmpty())
    {
        return S_OK;
    }

    IFC_RETURN(FindHeaderValueInPackedString(
        strPackedHeaders,
        strHeaderName,
        0 /* ichStart */,
        &ulUnused /* pichEnd */,
        &wszHeaderValue,
        &cchHeaderValue));

    if (wszHeaderValue)
    {
        IFC_RETURN(xstring_ptr::CloneBuffer(wszHeaderValue, cchHeaderValue, pstrHeaderValue));
    }

    return S_OK;
}


