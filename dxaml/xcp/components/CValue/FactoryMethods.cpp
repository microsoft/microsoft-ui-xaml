// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValue.h>
#include <primitives.h>
#include <Double.h>
#include <xstrutil.h>
#include <EnumValueTable.h>
#include <StringConversions.h>

// TODO: These methods don't have anything to do with CValue/CInt32/etc... As such, we should decouple them from those types.
_Check_return_ HRESULT CBoolean::CreateCValue(_In_ CREATEPARAMETERS* pCreate, _Inout_ CValue& value)
{
    INT32 result = 0;
    INT32 nValue = 0;
    const WCHAR *pStringValue = NULL;
    UINT32 cStringValue;

    IFCEXPECT_RETURN(pCreate->m_value.GetType() == valueString);

    UINT32 cString = 0;
    const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);

    // Booleans are a special enum, any value can represent true, so first
    // try to manually convert the string to an int. Otherwise we'll lookup the
    // string in the type table.
    if (FAILED(SignedFromDecimalString(cString, pString, &cStringValue, &pStringValue, &result)))
    {
        // Call the type converter
        IFC_RETURN(EnumerateFromString(2, satBoolean, cString, pString, &result));
        value.SetBool(!!result);
    }
    else
    {
        // SignedFromDecimalString() guarantees that pStringValue will be >= pCreate->m_value's count.
        ASSERT(pStringValue >= pString);
        if (cStringValue == 0)
        {
            value.SetBool(nValue != 0);
        }
        else
        {
            IFC_RETURN(E_UNEXPECTED);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CInt32::CreateCValue(_In_ CREATEPARAMETERS* pCreate, _Inout_ CValue& value)
{
    INT32 result = 0;

    IFCEXPECT_RETURN(pCreate->m_value.GetType() == valueString);

    // Call the type converter
    UINT32 cString = 0;
    const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);

    IFC_RETURN(SignedFromDecimalString(cString, pString, &cString, &pString, &result));

    value.SetSigned(result);

    return S_OK;
}

_Check_return_ HRESULT CDouble::CreateCValue(_In_ CREATEPARAMETERS* pCreate, _Inout_ CValue& value)
{
    FLOAT result = 0;

    IFCEXPECT_RETURN(pCreate->m_value.GetType() == valueString);

    // Trim the whitespace
    // Can't pass the address of the m_count bitfield, so...
    UINT32 cCount = 0;
    const WCHAR* bufferFromString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cCount);

    TrimWhitespace(cCount, bufferFromString, &cCount, &bufferFromString);

    if (cCount > 0)
    {
        // Call the type converter
        const UINT32 cPrevious = cCount;
        const WCHAR* extraInfoString = bufferFromString;

        HRESULT hrConversion = FloatFromString(cCount, bufferFromString, &cCount, &bufferFromString, &result, TRUE);

        if (FAILED(hrConversion))
        {
            std::vector<std::wstring> extraInfo;
            std::wstring extraInfoEntry = L"FloatFromString failure for: ";

            extraInfoEntry.append(extraInfoString);
            extraInfo.push_back(extraInfoEntry);

            IFC_RETURN_EXTRA_INFO(hrConversion, &extraInfo);
        }

        // If we failed to parse a value then we error out.
        IFCEXPECT_RETURN(cCount != cPrevious);
    }

    value.SetFloat(result);

    return S_OK;
}

_Check_return_ HRESULT CLengthConverter::CreateCValue(_In_ CREATEPARAMETERS* pCreate, _Inout_ CValue& value)
{
    IFCEXPECT_RETURN(pCreate->m_value.GetType() == valueString);

    UINT32 cString = 0;
    const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);

    if ((cString == 4) && !xstrncmpi(pString, L"auto", 4))
    {
        value.SetFloat(static_cast<FLOAT>(XDOUBLE_NAN));
    }
    else
    {
        IFC_RETURN(CDouble::CreateCValue(pCreate, value));
    }

    return S_OK;
}
