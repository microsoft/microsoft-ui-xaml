// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <StringConversions.h>
#include <xstrutil.h>
#include <EnumDefs.g.h>
#include <chrono>
#include "stack_vector.h"
#include "clock.h"

#define INFINITY_STRING L"Infinity"
#define NAN_STRING L"NaN"

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strAutomaticStorage, L"Automatic");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strForeverStorage, L"Forever");

//------------------------------------------------------------------------
//
//  Function:   NullTerminateString
//
//  Synopsis:
//      Several NT functions expect NULL terminated strings but we often have
//  a length and an array of characters.  This handy function converts the one
//  we're given into the one we want.
//
//      Since this function is used in the processing of debug trace messages,
//  it cannot use 'new' which would recursively generate further debug trace
//  messages.
//
//  Implementation notes:
//      The caller needs to call HeapFree to release the string.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NullTerminateString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Outptr_result_z_ wchar_t **ppwsz
)
{
    HRESULT hr = S_OK;

    *ppwsz = new wchar_t[cString + 1];

    memcpy(*ppwsz, pString, sizeof(wchar_t) * cString);
    (*ppwsz)[cString] = L'\0';

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   ConvertString
//
//  Synopsis:
//      Calls the run time to convert a string to a double. Before passing the string to wcstod we make sure
//      that the string is NULL terminated.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
ConvertString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ DOUBLE *peValue
)
{
    double eValue;
    wchar_t *pEnd;
    HRESULT hr = S_OK;

    ASSERT(pString);

    WCHAR *pwsz = NULL;

    if (cString <= 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    // We need not check for limit less strings as NullTerminateString
    // does for us.
    errno = 0;

    //Should not pass non null terminated string to the function
    // expecting null terminated string. so if the string is not
    // null terminated, create one.
    if (pString[cString - 1] != '\0')
    {
        IFC(NullTerminateString(cString, pString, &pwsz));
        eValue = wcstod((const wchar_t *)pwsz, &pEnd);

        if ((pEnd == pwsz) || (ERANGE == errno))
        {
            hr = E_UNEXPECTED;

            goto Cleanup;
        }

        UINT64 len = pEnd - pwsz;
        pEnd = const_cast<WCHAR*>(pString)+len;
    }
    else
    {
        eValue = wcstod((const wchar_t *)pString, &pEnd);

        if ((pEnd == pString) || (ERANGE == errno))
        {
            IFC(E_UNEXPECTED);
        }

    }

    *peValue = eValue;
    *ppSuffix = (WCHAR *)pEnd;
    *pcSuffix = static_cast<UINT32>(cString - (pEnd - pString));

    hr = S_OK;

Cleanup:

    if (NULL != pwsz)
    {
        // Because the memory was returned from NullTerminateString
        // which created it using HeapAlloc, we use HeapFree to free it.
        delete[] pwsz;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Function:   UnsignedFromHexString
//
//  Synopsis:
//      Converts a hexadecimal value string to an unsigned 32 bit integer.
//  For example the string fe becomes 254 and the string 1234 becomes 4660.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
UnsignedFromHexString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ UINT32 *pnValue
)
{
    UINT32 nValue = 0;

// Deal with erroneous input

    if (!cString)
        return E_UNEXPECTED;

// Consume all the hexadecimal digits

    while (cString && xisxdigit(*pString))
    {
    // Return failure on overflow

        if (nValue & 0xF0000000)
            return E_UNEXPECTED;

    // Adjust the value for the next digit

        nValue *= 16;

        if (xisdigit(*pString))
            nValue += INT32(*pString) - L'0';
        else if (xisupper(*pString))
            nValue += INT32(*pString) - L'A' + 10;
        else
            nValue += INT32(*pString) - L'a' + 10;

        pString++;
        cString--;
    }

// Return the values to the caller

    *pnValue = nValue;
    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Function:   SignedFromDecimalString
//
//  Synopsis:
//      Converts a decimal value string to a signed 32 bit integer.
//
//  Notes:
//
//    o  Input must start with a digit or a sign. No leading space is allowed.
//    o  Leading sign (if present) must be followed immediately by digits.
//    o  Only digits are allowed. There is no support for thousand separators.
//------------------------------------------------------------------------

_Check_return_ HRESULT
SignedFromDecimalString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ INT32* pnValue
)
{
    UINT32  nValue            = 0;
    UINT32  bNegative         = FALSE;
    UINT32  nMaxPositiveValue = 0x7FFFFFFF;  // precomputed for testing overflow

    // Deal with erroneous input

    if (!cString)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Handle leading sign, if any

    if (*pString == L'+'  ||  *pString == L'-')
    {
        if (*pString == L'-')
        {
            bNegative = TRUE;
            nMaxPositiveValue = 0x80000000;
        }
        pString++;
        cString--;
    }

    // Get unsigned decimal. Don't trace because some callers use this function to
    // check if the string contains a decimal, so this failure may be expected.
    IFC_NOTRACE_RETURN(UnsignedFromDecimalString(cString, pString, &cString, &pString, &nValue));

    if ((UINT32)nValue > (UINT32)nMaxPositiveValue)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    // Return the values to the caller

    if (bNegative)
    {
        *pnValue = -INT32(nValue);
    }
    else
    {
        *pnValue = INT32(nValue);
    }

    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Function:   UnsignedFromDecimalString
//
//  Synopsis:
//      Converts a decimal value string to an unsigned 32 bit integer.
//
//  Notes:
//
//    o  Input must start with a digit. No leading space is allowed.
//    o  Only digits are allowed. There is no support for thousand separators.
//------------------------------------------------------------------------

_Check_return_ HRESULT
UnsignedFromDecimalString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ UINT32* pnValue
    )
{
    UINT32  nValue            = 0;

    // Deal with erroneous input

    if (!cString)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // We require at least one digit

    if (!xisdigit(*pString))
    {
        // Don't trace because some callers use this function to check if the
        // string contains a decimal, so this failure may be expected.
        IFC_NOTRACE_RETURN(E_INVALIDARG);
    }

    // Consume all the decimal digits

    while (cString && xisdigit(*pString))
    {
    // Adjust the value for the next digit

        if (nValue > 0xffffffff / 10)
        {
            // Multiplying by 10 would cause overflow
            return E_UNEXPECTED;
        }

        nValue *= 10;
        if ((nValue + UINT32(*pString) - L'0') < nValue)
        {
            // adding in the last digit would cause overflow
            // this could happen if nValue is 4294967290 and next digit is >5
            return E_UNEXPECTED;
        }

        nValue += UINT32(*pString) - L'0';

        pString++;
        cString--;
    }

    // Return the values to the caller
    *pnValue = nValue;
    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}


const XDOUBLE s_pow10[] =
{
    1E-30,
    1E-29,
    1E-28,
    1E-27,
    1E-26,
    1E-25,
    1E-24,
    1E-23,
    1E-22,
    1E-21,
    1E-20,
    1E-19,
    1E-18,
    1E-17,
    1E-16,
    1E-15,
    1E-14,
    1E-13,
    1E-12,
    1E-11,
    1E-10,
    1E-9,
    1E-8,
    1E-7,
    1E-6,
    1E-5,
    1E-4,
    1E-3,
    1E-2,
    1E-1,
    1E+0,
    1E+1,
    1E+2,
    1E+3,
    1E+4,
    1E+5,
    1E+6,
    1E+7,
    1E+8,
    1E+9,
    1E+10,
    1E+11,
    1E+12,
    1E+13,
    1E+14,
    1E+15,
    1E+16,
    1E+17,
    1E+18,
    1E+19,
    1E+20,
    1E+21,
    1E+22,
    1E+23,
    1E+24,
    1E+25,
    1E+26,
    1E+27,
    1E+28,
    1E+29,
    1E+30
};

//------------------------------------------------------------------------
//
//  Function:   Power10
//
//  Synopsis:
//      Returns ten raised to the integer value.
//
//------------------------------------------------------------------------

XDOUBLE Power10(_In_ INT32 nPower)
{
    return nPower < -30 ? 0.0 : (nPower > 30 ? 1E+30 : s_pow10[nPower + 30]);
}

//------------------------------------------------------------------------
//
//  Function:   FloatFromString
//
//  Synopsis:
//      Converts a string to a float.  Allows for exponential notation.
//
//  Implementation details:
//      To improve performance of this routine on ARM based processors the
//  values before and after the decimal point as well as the exponent are kept
//  as integers and only converted at the end.  This also keeps any error that
//  may accumulate as small as possible.  If this fast path code thinks it may
//  fail to produce a valid result it will fallback to the PAL provided format
//  conversion service.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
FloatFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ FLOAT *peValue,
    _In_ BOOL bAllowSpecialValues
)
{
    INT32  bSigned;
    INT32  nLeading = 0;
    INT32  nTrailing = 0;
    UINT32 cTrailing = 0;
    INT32  nExponent = 0;
    UINT32 cStringSave = cString;
    const WCHAR  *pStringSave = pString;
    XDOUBLE eValue;
    UINT32 cPostSpaceSave = cString;
    HRESULT hr;

// Consume any leading whitespace

    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }

// The value can be unsigned, negative, or positive.

    bSigned = 1;

    if (cString && ((L'-' == *pString) || (L'+' == *pString)))
    {
        if (L'-' == *pString)
            bSigned = -1;

        pString++;
        cString--;
    }

    cPostSpaceSave = cString;

// Consume digits for the leading portion

    while (cString && xisdigit(*pString))
    {
    // If we'd cause an overflow bail out to the PAL.  If we'd need more than
    // 30 bits after multiplying by 10 and adding 9 we'll bail.

        if (nLeading > 107374181)
        {
#pragma warning (push)
#pragma warning (disable : 26014)
            hr = ConvertString(cStringSave, pStringSave, pcSuffix, ppSuffix, &eValue);
#pragma warning (pop)
           *peValue = FLOAT(eValue);
            return hr;
        }

        nLeading *= 10;
        nLeading += INT32(*pString) - L'0';

        pString++;
        cString--;
    }

// Compute the signed value without branching.  bSigned is either 1 or -1.  So
// we get n * 1 which is n or n * -1 which is -n.  This should only take one
// instructions on most processors.
//
// For example on the x86 if we assume the value we want signed is in eax and
// the value of the sign is in ecx then we'd get the following instructions:
//
//      imul    eax,ecx
//
// Compare that to the traditional instruction flow:
//
//      test    ecx,0
//      jz      $L1000
//      neg     eax
// $L1000:

    nLeading *= bSigned;

// If the next character is a decimal point calculate the trailing portion

    if (cString && (L'.' == *pString))
    {
        pString++;
        cString--;

        while (cString && xisdigit(*pString))
        {
        // If we'd cause an overflow bail out to the PAL.  If we'd need more
        // than 30 bits after multiplying by 10 and adding 9 we'll bail.

            if (nTrailing > 107374181)
            {
#pragma warning (push)
#pragma warning (disable : 26014)
                hr = ConvertString(cStringSave, pStringSave, pcSuffix, ppSuffix, &eValue);
#pragma warning (pop)
               *peValue = FLOAT(eValue);
                return hr;
            }

            nTrailing *= 10;
            nTrailing += UINT32(*pString) - L'0';

            pString++;
            cString--;
            cTrailing++;
        }
    }

    nTrailing *= bSigned;

// If the next character is an exponent marker calculate the exponent

    if (cString && ((L'd' == *pString) || (L'D' == *pString) || (L'e' == *pString) || (L'E' == *pString)))
    {
        pString++;
        cString--;

    // The exponent can be unsigned, negative, or positive.

        bSigned = 1;

        if (cString && ((L'-' == *pString) || (L'+' == *pString)))
        {
            if (L'-' == *pString)
                bSigned = -1;

            pString++;
            cString--;
        }

    // Now consume the digits in the exponent.  We limit the exponent to the
    // range -29 to +29.

        while (cString && xisdigit(*pString))
        {
        // For exponents outside of our range call the PAL.

            if (nExponent > 2)
            {
                hr = ConvertString(cStringSave, pStringSave, pcSuffix, ppSuffix, &eValue);
               *peValue = FLOAT(eValue);
                return hr;
            }

            nExponent *= 10;
            nExponent += INT32(*pString) - L'0';

            pString++;
            cString--;
        }

    // Compute the signed value without branching.  See comment above.

        nExponent *= bSigned;
    }

    // We let the common numeric case try first, and only if we didn't
    // see anything we try to match the strings.
    //
    // Allow Infinity, -Infinity and NaN (case sensitive)
    // and also allows trailing whitespace.
    //
    // Expects the whole string to be consumed. If this would
    // be required in future as part of parsing multi-value
    // strings this would need to be changed. But since most of
    // conversion/creation functions currently have bugs with not checking
    // if the whole string is consumed no sense adding to the
    // future quirk burden by expecting the caller to do the
    // right thing.
    if (bAllowSpecialValues && (cString >= SZ_COUNT(NAN_STRING)) && (cPostSpaceSave == cString))
    {
        if ((cString >= SZ_COUNT(INFINITY_STRING)) && !xstrncmp(pString, STR_LEN_PAIR(INFINITY_STRING)))
        {
            eValue = bSigned ? XDOUBLE_INF : -XDOUBLE_INF;

            pString += SZ_COUNT(INFINITY_STRING);
            cString -= SZ_COUNT(INFINITY_STRING);
        }
        else if ((cString >= SZ_COUNT(NAN_STRING)) && !xstrncmp(pString, STR_LEN_PAIR(NAN_STRING)))
        {
            if (bSigned)
            {
                eValue = XDOUBLE_NAN;

                pString += SZ_COUNT(NAN_STRING);
                cString -= SZ_COUNT(NAN_STRING);
            }
            else
            {
                return E_FAIL;
            }
        }
        else
        {
            return E_FAIL;
        }

        // Verify all remaining characters are whitespace
        while (cString && xisspace(*pString))
        {
            pString++;
            cString--;
        }

        if (cString != 0)
        {
            return E_FAIL;
        }
    }
    else
    {

// At this point we can build the return value.

        eValue = XDOUBLE(nLeading);

        if (cTrailing)
            eValue += (XDOUBLE(nTrailing) / Power10(cTrailing));

        if (nExponent)
            eValue *= Power10(nExponent);
    }

// Return the values to the caller

   *peValue = FLOAT(eValue);

   *pcSuffix = cString;
   *ppSuffix = pString;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   NonSignedFromString
//
//  Synopsis:
//      Converts a string to a float.  However, the number must not only be
//  positive it can't have any sign in front of it.  This is used rarely and
//  since the extra cost of parametrizing the above function was deemed too
//  high this function is a duplicate with the initial sign test modified.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NonSignedFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ FLOAT *peValue
)
{
    INT32  bSigned;
    INT32  nLeading = 0;
    INT32  nTrailing = 0;
    UINT32 cTrailing = 0;
    INT32  nExponent = 0;
    UINT32 cStringSave = cString;
    const WCHAR  *pStringSave = pString;
    XDOUBLE eValue;
    HRESULT hr;

// Consume any leading whitespace

    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }

// Consume digits for the leading portion

    while (cString && xisdigit(*pString))
    {
    // If we'd cause an overflow bail out to the PAL.  If we'd need more than
    // 30 bits after multiplying by 10 and adding 9 we'll bail.

        if (nLeading > 107374181)
        {
#pragma warning (push)
#pragma warning (disable : 26014)
            hr = ConvertString(cStringSave, pStringSave, pcSuffix, ppSuffix, &eValue);
#pragma warning (pop)
           *peValue = FLOAT(eValue);
            return hr;
        }

        nLeading *= 10;
        nLeading += INT32(*pString) - L'0';

        pString++;
        cString--;
    }

// If the next character is a decimal point calculate the trailing portion

    if (cString && (L'.' == *pString))
    {
        pString++;
        cString--;

        while (cString && xisdigit(*pString))
        {
        // If we'd cause an overflow bail out to the PAL.  If we'd need more
        // than 30 bits after multiplying by 10 and adding 9 we'll bail.

            if (nTrailing > 107374181)
            {
#pragma warning (push)
#pragma warning (disable : 26014)
                hr = ConvertString(cStringSave, pStringSave, pcSuffix, ppSuffix, &eValue);
#pragma warning (pop)

               *peValue = FLOAT(eValue);
                return hr;
            }

            nTrailing *= 10;
            nTrailing += UINT32(*pString) - L'0';

            pString++;
            cString--;
            cTrailing++;
        }
    }

// If the next character is an exponent marker calculate the exponent

    if (cString && ((L'd' == *pString) || (L'D' == *pString) || (L'e' == *pString) || (L'E' == *pString)))
    {
        pString++;
        cString--;

    // The exponent can be unsigned, negative, or positive.

        bSigned = 1;

        if (cString && ((L'-' == *pString) || (L'+' == *pString)))
        {
            if (L'-' == *pString)
                bSigned = -1;

            pString++;
            cString--;
        }

    // Now consume the digits in the exponent.  We limit the exponent to the
    // range -29 to +29.

        while (cString && xisdigit(*pString))
        {
        // For exponents outside of our range call the PAL.

            if (nExponent > 2)
            {
#pragma warning (push)
#pragma warning (disable : 26014)
                hr = ConvertString(cStringSave, pStringSave, pcSuffix, ppSuffix, &eValue);
#pragma warning (pop)
               *peValue = FLOAT(eValue);
                return hr;
            }

            nExponent *= 10;
            nExponent += INT32(*pString) - L'0';

            pString++;
            cString--;
        }

    // Compute the signed value without branching.  See comment in the previous
    // function.

        nExponent *= bSigned;
    }

// At this point we can build the return value.

    eValue = XDOUBLE(nLeading);

    if (cTrailing)
        eValue += (XDOUBLE(nTrailing) / Power10(cTrailing));

    if (nExponent)
        eValue *= Power10(nExponent);

// Return the values to the caller

    *peValue = FLOAT(eValue);
    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   ArrayFromString
//
//  Synopsis:
//      Returns an array of floats of a specified length from a parse string.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
ArrayFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _In_ UINT32 cElements,
    _Out_writes_(cElements) FLOAT *peValue,
    _In_opt_ UINT32 bConsumeString
)
{
    INT32   bComma = FALSE;

    while (cString && cElements)
    {
        TrimWhitespace(cString, pString, &cString, &pString);

    // We might have a comma

        if (bComma && (cString && (L',' == *pString)))
        {
            cString--;
            pString++;

            TrimWhitespace(cString, pString, &cString, &pString);
        }

        const UINT32 cPrevious = cString;

        IFC_RETURN(FloatFromString(cString, pString, &cString, &pString, peValue++));

    // If we failed to parse a value then an error has occurred.

        if (cString == cPrevious)
            return E_UNEXPECTED;

    // Adjust the remaining count of characters

        cElements--;

    // Allow comma after the first value

        bComma = TRUE;
    }

    // Ensure that we read all the requested values
    if (cElements)
    {
        // Don't trace because some callers use this function to
        // determine the number of elements in the string, so this
        // failure may be expected.
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    // Ensure that we consumed the string (if bConsumeString is TRUE).
    if (bConsumeString)
    {
        // Size "10, 20" is a valid size, but "10,20," is not a valid size
        // Before doing that, trim the string.

        if (cString)
        {
            TrimWhitespace(cString, pString, &cString, &pString);
        }

        // Any non white-space characters is garbage, and we should fail.
        if (cString)
        {
            IFC_RETURN(E_FAIL);
        }
    }

    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   EnumerateFromString
//
//  Synopsis:
//      Walks a table of strings and if it finds an exact match returns the
//  value associated with it.  Used for enumerated attributes like FillRule,
//  MappingMode, and IsClosed.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
EnumerateFromString(
    _In_ UINT32 cTable,
    _In_reads_(cTable) const XTABLE *pTable,
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ INT32 *pnResult
)
{
    HRESULT hr;
    UINT32 iTable = 0;
    INT32 nValue = 0;
    const WCHAR *pSuffix;
    UINT32 cSuffix;

    TrimWhitespace(cString, pString, &cString, &pString);

    if (cString > 0 && xisdigit(*pString))
    {
        hr = SignedFromDecimalString(cString, pString, &cSuffix, &pSuffix, &nValue);
        if (SUCCEEDED(hr) && (0 == cSuffix))
        {
            while (iTable != cTable)
            {
                if (pTable->m_nValue == nValue)
                {
                    *pnResult = nValue;
                    return S_OK;
                }

                pTable++;
                iTable++;
            }
        }

    }

    while (iTable != cTable)
    {
        if ((cString == pTable->m_strStringStorage.Count) && !xstrncmpi(pString, pTable->m_strStringStorage.Buffer, cString))
        {
           *pnResult = pTable->m_nValue;
            return S_OK;
        }

        pTable++;
        iTable++;
    }

    return E_FAIL;
}

//------------------------------------------------------------------------
//
//  Function:   FlagsEnumerateFromString
//
//  Synopsis:
//      Walks a table of strings and combines all the enumerations in the
//      string with a bitwise OR operation to get the value.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
FlagsEnumerateFromString(
                    _In_ UINT32 cTable,
                    _In_reads_(cTable) const XTABLE *pTable,
                    _In_ UINT32 cString,
                    _In_reads_(cString) const WCHAR *pString,
                    _Out_ INT32 *pnResult
                    )
{
    HRESULT hr = S_OK;

    if(pString != NULL && cString > 0)
    {
        UINT32 nStringLength = cString;
        const WCHAR* pSearchString = pString;
        BOOL bConsumedComma = FALSE;

        // Initialize the output result.
        *pnResult = 0;

        // Trim initial whitespace and validate string.
        while(nStringLength > 0 && SUCCEEDED(hr))
        {
            // Consume white space.
            if(*pSearchString == L' ')
            {
                --nStringLength;
                ++pSearchString;
            }
            // Check for a comma, it's not valid to have one before the first value.
            else if(*pSearchString == L',')
            {
                hr = E_FAIL;
            }
            else
            {
                break;
            }
        }

        // Search for enumeration values within the string.
        while(nStringLength > 0 && SUCCEEDED(hr))
        {
            // Set the result to fail as it will be set to success if a value is matched.
            hr = E_FAIL;

            const XTABLE* pEnumTable = pTable;

            for(UINT32 iTable = 0; iTable < cTable; iTable++)
            {
                // Cache the length of the enumeration string.
                const UINT32 nEnumStringLength = pEnumTable->m_strStringStorage.Count;

                // Check the string fits within the search string.
                if(nEnumStringLength <= nStringLength)
                {
                    // Check for the sub string.
                    if (xstrncmp(pEnumTable->m_strStringStorage.Buffer, pSearchString, nEnumStringLength) == 0)
                    {
                        // Check the string is terminated and not just a partial match.
                        // The string must either align with the end of the input string or be followed by a comma or a space.
                        if(nStringLength == nEnumStringLength ||
                            (nStringLength > nEnumStringLength &&
                            (pSearchString[nEnumStringLength] == L' ' || pSearchString[nEnumStringLength] == L',')))
                        {
                            // OR the values together
                            *pnResult |= pEnumTable->m_nValue;

                            // Consume the parsed segment of the string.
                            nStringLength -= nEnumStringLength;
                            pSearchString += nEnumStringLength;

                            // Mark that a value was found
                            hr = S_OK;

                            break;
                        }
                    }
                }

                // Check next enumeration.
                ++pEnumTable;
            }

            // Reset comma consumption flag.
            bConsumedComma = FALSE;

            // Search to the next potential value.
            while(nStringLength > 0 && SUCCEEDED(hr))
            {
                // Consume white space.
                if(*pSearchString == L' ')
                {
                    --nStringLength;
                    ++pSearchString;
                }
                // Consume the first comma.
                else if(*pSearchString == L',')
                {
                    // Check a comma has not already been found.
                    if(bConsumedComma == FALSE)
                    {
                        bConsumedComma = TRUE;
                        --nStringLength;
                        ++pSearchString;
                    }
                    else
                    {
                        // Second comma, this string is invalid.
                        hr = E_FAIL;

                        break;
                    }
                }
                // Reached the next search string or end of string.
                else
                {
                    break;
                }
            }
        }

        // Check that the entire string has been consumed.
        if(nStringLength != 0)
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Function:   NameFromString
//
//  Synopsis:
//      Parses an XML name with optional namespace from the string.
//
//  Implementation notes:
//      The string has already been parsed by XML so entity and character swaps
//  have already occurred.  However, XML didn't know this property was actually
//  an XML property so we have to parse it ourselves.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
NameFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ XNAME *pName
    )
{
    HRESULT hr;

    TrimWhitespace(cString, pString, &cString, &pString);

// We won't know whether or not there is a namespace until we parse the string.
// Assume we don't have one for now.

    pName->pName = pString;

    while (cString && xisname(*pString))
    {
        pString++;
        cString--;
    }

// A zero length name is invalid
    pName->cName = UINT32(pString - pName->pName);
    if (!pName->cName)
    {
        IFC(E_FAIL);
    }

// If the current character is a colon then what we guessed was the name was
// actually the namespace instead.  Deal with it.

    if (cString && (L':' == *pString))
    {
        pString++;
        cString--;
        pName->pNamespace = pName->pName;
        pName->cNamespace = pName->cName;

        pName->pName = pString;

        while (cString && xisname(*pString))
        {
            pString++;
            cString--;
        }

    // A zero length name is invalid
        pName->cName = UINT32(pString - pName->pName);
        if (!pName->cName)
        {
            IFC(E_FAIL);
        }
    }
    else
    {
        pName->cNamespace = 0;
    }

    hr = S_OK;
    *pcSuffix = cString;
    *ppSuffix = pString;

Cleanup:
    return hr;
}


// Parse a sequence of unsigned digits that form part of a time segment.
_Check_return_ HRESULT ParseTimeSegment(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _In_ INT32 nLimit,
    _Out_ INT32 *pnSegmentValue
    )
{
    const UINT32 MAX_DIGITS = 7;
    UINT32 iDigitCount = 0;

    *pnSegmentValue = 0;

    // Parse segment
    while (cString && xisdigit(*pString))
    {
        // Return failure on overflow
        if ((*pnSegmentValue) >= nLimit)
        {
            return E_UNEXPECTED;
        }

        // Adjust the value for the next digit
        (*pnSegmentValue) = 10 * (*pnSegmentValue) + (XINT32(*pString) - L'0');

        pString = pString + 1;
        cString = cString - 1;
        iDigitCount++;

        if (iDigitCount > MAX_DIGITS)
        {
            return E_UNEXPECTED;
        }

    }

    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}

_Check_return_ HRESULT TimeSpanFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DOUBLE *peValue
    )
{
    UINT32 cString;
    auto pString = inString.GetBufferAndCount(&cString);

    // The formats for a TimeSpan are
    //
    //     [-]d.hh:mm:ss[.ff]
    //     [-]d.hh:mm
    //     [-]d.hh
    //     [-]d
    //
    //    hh - hours
    //    mm - minutes
    // ss.ff - seconds
    //
    // with whitespace allowed at the start and end of the string.
    const INT32 MAX_HOURS = 24;
    const INT32 MAX_MINUTES = 60;
    const INT32 MAX_SECONDS = 60;
    XINT32 nSign = 1;
    XINT32 nDays = 0;
    XINT32 nHours = 0;
    XINT32 nMinutes = 0;
    XINT32 nSeconds = 0;
    XINT32 bHoursParsed = FALSE;
    XINT32 nFractionNum = 0;
    XINT32 nFractionDen = 1;

    IFCPTR_RETURN(peValue);

    // Skip whitespace at the start of the string
    TrimWhitespace(cString, pString, &cString, &pString);

    // Parse sign bit
    if (cString && (*pString) == '-')
    {
        nSign = -1;
        pString++;
        cString--;
    }

    // Parse days
    if (cString && xisdigit(*pString))
    {
        IFC_RETURN(ParseTimeSegment(
            cString,
            pString,
            &cString,
            &pString,
            XINT32_MAX / 864000, // going to multiply days by 10*24*3600 to get seconds
            &nDays
            ));

        if (cString && (*pString) == ':')
        {
            // If we see a colon before a dot, than days actually meant hours
            nHours = nDays;
            nDays = 0;
            bHoursParsed = TRUE;
        }
        else if (cString && (*pString) == '.')
        {
            pString++;
            cString--;

            // If we see a day, than parse hours as well as days

            IFC_RETURN(ParseTimeSegment(
                cString,
                pString,
                &cString,
                &pString,
                XINT32_MAX / 36000, // going to multiply hours by 10*3600 to get seconds
                &nHours
                ));

            bHoursParsed = TRUE;
        }

        // If we see a hours, than we need minutes too.
        if (bHoursParsed)
        {
            if (cString && (*pString) == ':')
            {
                pString++;
                cString--;

                // Parse minutes
                IFC_RETURN(ParseTimeSegment(
                    cString,
                    pString,
                    &cString,
                    &pString,
                    XINT32_MAX / 600, // going to multiply minutes by 10*60 to get seconds
                    &nMinutes
                    ));
            }
            else
            {
                IFC_RETURN(E_FAIL);
            }
        }
    }


    // Consume colon and parse seconds

    nFractionNum = 0;
    nFractionDen = 1;

    if (cString && (*pString) == ':')
    {
        pString++;
        cString--;

        // Parse seconds

        IFC_RETURN(ParseTimeSegment(
            cString,
            pString,
            &cString,
            &pString,
            XINT32_MAX / 10, // going to multiply by 10 to advance digits
            &nSeconds
            ));

        // Parse second fractions

        if (cString && (*pString) == '.')
        {
            pString++;
            cString--;

            while (cString && xisdigit(*pString))
            {
                // Return failure on overflow

                if (nFractionDen >= XINT32_MAX / 10)
                    return E_UNEXPECTED;

                // Adjust the value for the next digit

                nFractionNum = 10 * nFractionNum + (XINT32(*pString) - L'0');
                nFractionDen *= 10;

                pString++;
                cString--;
            }
        }

    }

    // Skip whitespace
    TrimWhitespace(cString, pString, &cString, &pString);

    // If we have any characters left, it's an error

    if (cString)
    {
        IFC_RETURN(E_FAIL);
    }

    // Set value

    if (nHours >= MAX_HOURS || nMinutes >= MAX_MINUTES || nSeconds >= MAX_SECONDS)
    {
        IFC_RETURN(E_FAIL);
    }

    *peValue =
        XDOUBLE(nDays)*24.0f*3600.0
        + XDOUBLE(nHours)*3600.0
        + XDOUBLE(nMinutes)*60.0
        + XDOUBLE(nSeconds)
        + (XDOUBLE(nFractionNum) / XDOUBLE(nFractionDen));

    *peValue *= XDOUBLE(nSign);

    return S_OK;
}

_Check_return_ HRESULT TimeSpanToString(
    _In_ DOUBLE timeSpanInSec,
    _Out_ xstring_ptr& outString)
{
    using days = std::chrono::duration<int, std::ratio<24 * std::chrono::hours::period::num, std::chrono::hours::period::den>>;
    using ticks = std::chrono::duration<uint64_t, std::ratio<100 * std::nano::num, std::nano::den>>;

    bool isNegative = false;

    if (timeSpanInSec < 0.0)
    {
        isNegative = true;
        timeSpanInSec = -timeSpanInSec;
    }

    ticks timeSpanInTicks(static_cast<uint64_t>(timeSpanInSec * ticks::period::den / ticks::period::num));

    days timeSpanDays = std::chrono::duration_cast<days>(timeSpanInTicks);
    timeSpanInTicks -= timeSpanDays;

    std::chrono::hours timeSpanHours = std::chrono::duration_cast<std::chrono::hours>(timeSpanInTicks);
    timeSpanInTicks -= timeSpanHours;

    std::chrono::minutes timeSpanMinutes = std::chrono::duration_cast<std::chrono::minutes>(timeSpanInTicks);
    timeSpanInTicks -= timeSpanMinutes;

    std::chrono::seconds timeSpanSeconds = std::chrono::duration_cast<std::chrono::seconds>(timeSpanInTicks);
    timeSpanInTicks -= timeSpanSeconds;

    constexpr size_t bufferSize = 64;
    wchar_t buffer[bufferSize];

    int usedBufferLength = swprintf_s(
        buffer,
        bufferSize,
        L"%s%u.%02u:%02u:%02I64u.%07I64u",
        (isNegative) ? L"-" : L"",
        timeSpanDays.count(),
        timeSpanHours.count(),
        timeSpanMinutes.count(),
        timeSpanSeconds.count(),
        timeSpanInTicks.count());

    IFC_RETURN(xstring_ptr::CloneBuffer(
        buffer,
        usedBufferLength,
        &outString));

    return S_OK;
}

_Check_return_ HRESULT KeyTimeFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DOUBLE *peValue)
{
    UINT32 cString;
    auto pString = inString.GetBufferAndCount(&cString);

    if (cString == 0)
    {
        return S_OK;
    }

    // Ignore leading space
    TrimWhitespace(cString, pString, &cString, &pString);

    // Ignore trailing space
    TrimTrailingWhitespace(cString, pString, &cString, &pString);

    IFC_RETURN(TimeSpanFromString(xephemeral_string_ptr(pString, cString), peValue));
    return S_OK;
}

_Check_return_ HRESULT KeyTimeToString(
    _In_ DOUBLE timeSpanInSec,
    _Out_ xstring_ptr& outString)
{
    return TimeSpanToString(timeSpanInSec, outString);
}

_Check_return_ HRESULT DurationFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DirectUI::DurationType *pDurationType,
    _Out_ DOUBLE *peValue)
{
    UINT32 cString;
    auto pString = inString.GetBufferAndCount(&cString);

    if (cString == 0)
    {
        return S_OK;
    }

    // Ignore leading space
    TrimWhitespace(cString, pString, &cString, &pString);

    // Ignore trailing space
    TrimTrailingWhitespace(cString, pString, &cString, &pString);

    xstring_ptr strAutomatic(c_strAutomaticStorage);
    xstring_ptr strForever(c_strForeverStorage);

    if (cString == strAutomatic.GetCount() && xstrncmpi(strAutomatic.GetBuffer(), pString, cString) == 0)
    {
        *pDurationType = DirectUI::DurationType::Automatic;
    }
    else if (cString == strForever.GetCount() && xstrncmpi(strForever.GetBuffer(), pString, cString) == 0)
    {
        *pDurationType = DirectUI::DurationType::Forever;
    }
    else
    {
        *pDurationType = DirectUI::DurationType::TimeSpan;
        IFC_RETURN(TimeSpanFromString(xephemeral_string_ptr(pString, cString), peValue));
    }

    return S_OK;
}

_Check_return_ HRESULT DurationToString(
    _In_ DirectUI::DurationType durationType,
    _In_ DOUBLE timeSpanInSec,
    _Out_ xstring_ptr& outString)
{
    switch (durationType)
    {
        case DirectUI::DurationType::Automatic:
            outString = xstring_ptr(c_strAutomaticStorage);
            break;

        case DirectUI::DurationType::Forever:
            outString = xstring_ptr(c_strForeverStorage);
            break;

        case DirectUI::DurationType::TimeSpan:
            IFC_RETURN(TimeSpanToString(timeSpanInSec, outString));
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT RepeatBehaviorFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DirectUI::RepeatBehaviorType *repeatBehaviorType,
    _Out_ DOUBLE *durationInSec,
    _Out_ FLOAT *count)
{
    UINT32 cString;
    auto pString = inString.GetBufferAndCount(&cString);

    if (cString == 0)
    {
        return S_OK;
    }

    // Ignore leading space
    TrimWhitespace(cString, pString, &cString, &pString);

    // Ignore trailing space
    TrimTrailingWhitespace(cString, pString, &cString, &pString);

    // look for an "X" from the end
    UINT32 cXString = cString;
    auto pXString = pString + cString - 1;

    // Get the repeat
    if (cXString && (*pXString == 'X' || *pXString == 'x'))
    {
        cXString--;

        // Ignore trailing space
        TrimTrailingWhitespace(cXString, pString, &cXString, &pString);

        IFC_RETURN(FloatFromString(cXString, pString, &cXString, &pString, count));

        if (*count < 0.0f)
        {
            // we need to be in range, all repeats must be >= 0
            IFC_RETURN(E_FAIL);
        }
    }
    else
    {
        // there was no X, treat it as a Duration
        IFC_RETURN(DurationFromString(
            inString,
            reinterpret_cast<DirectUI::DurationType*>(repeatBehaviorType),
            durationInSec));
    }

    return S_OK;
}

_Check_return_ HRESULT RepeatBehaviorToString(
    _In_ DirectUI::RepeatBehaviorType repeatBehaviorType,
    _In_ DOUBLE durationInSec,
    _In_ FLOAT count,
    _Out_ xstring_ptr& outString)
{
    switch (repeatBehaviorType)
    {
        case DirectUI::RepeatBehaviorType::Count:
            {
                WCHAR* buffer = nullptr;
                HSTRING_BUFFER bufferHandle = nullptr;
                int length = _scwprintf(L"%fx", count);

                IFC_RETURN(::WindowsPreallocateStringBuffer(
                    length,
                    &buffer,
                    &bufferHandle));

                auto bufferGuard = wil::scope_exit([bufferHandle]()
                {
                    VERIFYHR(::WindowsDeleteStringBuffer(
                        bufferHandle));
                });

                IFCEXPECT_RETURN(swprintf_s(
                    buffer,
                    length + 1,
                    L"%fx",
                    count) == length);

                wrl_wrappers::HString strValue;

                IFC_RETURN(::WindowsPromoteStringBuffer(
                    bufferHandle,
                    strValue.ReleaseAndGetAddressOf()));

                IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(
                    strValue.Get(),
                    &outString));

                bufferGuard.release();
            }
            break;

        case DirectUI::RepeatBehaviorType::Duration:
            IFC_RETURN(TimeSpanToString(durationInSec, outString));
            break;

        case DirectUI::RepeatBehaviorType::Forever:
            outString = xstring_ptr(c_strForeverStorage);
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT GridLengthFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ XGRIDLENGTH *peValue
    )
{
    UINT32 cString;
    auto pString = inString.GetBufferAndCount(&cString);
    UINT32 cValueString = 0;

    TrimTrailingWhitespace(cString, pString, &cString, &pString);
    TrimWhitespace(cString, pString, &cString, &pString);

    if ((cString == 4) && !xstrncmpi(pString, L"Auto", 4))
    {
        //Auto sizing implies the element is measured at infinite dimensions
        peValue->type = DirectUI::GridUnitType::Auto;
        peValue->value = XGRIDLENGTH::Default();
    }
    else if ((cString > 0) && !xstrncmp(pString + cString - 1, L"*", 1))
    {
        // value is the weight w.r.t to other star definitions. Default value is defined in const XFLOAT XGRIDLENGTH::Default
        peValue->type = DirectUI::GridUnitType::Star;
        peValue->value = XGRIDLENGTH::Default();
        cValueString = cString - 1;
    }
    else
    {
        peValue->type = DirectUI::GridUnitType::Pixel;
        cValueString = cString;
    }

    // retrieve the Pixel value or the weight for star definition.
    if (cValueString > 0)
    {
        const UINT32 cPrevious = cValueString;

        IFC_RETURN(FloatFromString(cValueString, pString, &cValueString, &pString, &(peValue->value)));

        // If we failed to parse a value then an error has occurred.

        IFCEXPECT_RETURN(cValueString != cPrevious);
    }

    return S_OK;
}

_Check_return_ HRESULT ParseGridDefinitionCollectionInitializationString(
    _In_ const xstring_ptr& inString,
    _In_ Jupiter::stack_vector<xstring_ptr, 8>& defCollection)
{
    if (inString.IsNullOrEmpty())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    auto toRead = inString;
    while (!toRead.IsNullOrEmpty())
    {
        auto commaIndex = toRead.FindChar(L',');
        xstring_ptr element;
        if (commaIndex == xstring_ptr_view::npos)
        {
            element = toRead;
        }
        else
        {
            xephemeral_string_ptr untrimmedElement;
            toRead.SubString(0, commaIndex, &untrimmedElement);
            IFC_RETURN(xstring_ptr::CloneBufferTrimWhitespace(untrimmedElement.GetBuffer(), untrimmedElement.GetCount(), &element));
        }

        if (element.IsNullOrEmpty())
        {
            defCollection.m_vector.clear();
            IFC_RETURN(E_INVALIDARG);
        }
        else
        {
            defCollection.m_vector.push_back(element);
        }

        if (commaIndex == (toRead.GetCount()-1))
        {
            // Comma as the last character in the string is an error
            defCollection.m_vector.clear();
            IFC_RETURN(E_INVALIDARG);
        }
        else if (commaIndex == xstring_ptr_view::npos)
        {
            // We've processed the final element; we're done
            break;
        }
        else
        {
            xstring_ptr remainder;
            IFC_RETURN(toRead.SubString(commaIndex + 1, toRead.GetCount(), &remainder));
            toRead = remainder;
        }
    }

    return S_OK;
}

// exists for parser, do not use in regular code passes
_Check_return_ HRESULT ThicknessFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ XTHICKNESS *peValue
    )
{
    UINT32 cString;
    auto pString = inString.GetBufferAndCount(&cString);

    UINT32 cScratch;
    const WCHAR* pScratch;

    // The thickness can be specified as left, top, right, bottom...
    if (FAILED(ArrayFromString(cString, pString, &cScratch, &pScratch, 4, reinterpret_cast<FLOAT *>(peValue))))
    {
        // ...or two numbers, which are left/right and top/bottom...
        if (SUCCEEDED(ArrayFromString(cString, pString, &cScratch, &pScratch, 2, reinterpret_cast<FLOAT *>(peValue))))
        {
            peValue->right = peValue->left;
            peValue->bottom = peValue->top;
        }
        else
        {
            // ...or just one number, which is used all around.
            IFC_RETURN(ArrayFromString(cString, pString, &cScratch, &pScratch, 1, reinterpret_cast<FLOAT *>(peValue)));
            peValue->top = peValue->right = peValue->bottom = peValue->left;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT MatrixFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ CMILMatrix *pmatValue)
{
    XFLOAT  *pNumeric = (XFLOAT *) pmatValue;
    UINT32  cNumeric = 6;

// Consume initial white space

    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }

// Check for the Identity value

    if ((cString == SZ_COUNT(L"Identity")) && !xstrncmp(pString, STR_LEN_PAIR(L"Identity")))
    {
        pmatValue->SetToIdentity();
        pString += SZ_COUNT(L"Identity");
        cString -= SZ_COUNT(L"Identity");
        goto parsed;
    }

    while (cString && cNumeric)
    {
    // Consume white space and optional comma

        while (cString && xisspace(*pString))
        {
            pString++;
            cString--;
        }

    // We might be have a comma

        if (cString && (L',' == *pString))
        {
            cString--;
            pString++;

        // After the comma there can be more white space

            while (cString && xisspace(*pString))
            {
                pString++;
                cString--;
            }
        }

        const UINT32 cPrevious = cString;

        IFC_RETURN(FloatFromString(cString, pString, &cString, &pString, pNumeric++));

    // If we failed to parse a value then an error has occurred.

        if (cString == cPrevious)
        {
            IFC_RETURN(E_UNEXPECTED);
        }

    // Adjust the remaining count of characters

        cNumeric--;
    }

// Ensure that we read a full matrix

    if (cNumeric)
    {
        IFC_RETURN(E_FAIL);
    }

parsed:
    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}

_Check_return_ HRESULT Matrix4x4FromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ CMILMatrix4x4 *pmatValue)
{
    XFLOAT  *pNumeric = (XFLOAT *) pmatValue;
    UINT32  cNumeric = 16;

    // Consume initial white space
    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }

    // TODO:
    // The current arrangement might not be robust enough.
    // What if the user use "Identity123"?  And also what if the user start the matrix with a
    // comma?
    // Check for the Identity value
    if ((cString == SZ_COUNT(L"Identity")) && !xstrncmp(pString, STR_LEN_PAIR(L"Identity")))
    {
        pmatValue->SetToIdentity();             // defined in xcp\common\matrix.cpp
        pString += SZ_COUNT(L"Identity");
        cString -= SZ_COUNT(L"Identity");
        goto parsed;
    }

    while (cString && cNumeric)
    {
    // Consume white space and optional comma

        while (cString && xisspace(*pString))
        {
            pString++;
            cString--;
        }

        // We might be have a comma
        if (cString && (L',' == *pString))
        {
            pString++;
            cString--;

            // After the comma there can be more white space
            while (cString && xisspace(*pString))
            {
                pString++;
                cString--;
            }
        }

        const XUINT32 cPrevious = cString;

        IFC_RETURN(FloatFromString(cString, pString, &cString, &pString, pNumeric++));

        // If we failed to parse a value then an error has occurred.
        if (cString == cPrevious)
        {
            IFC_RETURN(E_UNEXPECTED);
        }

        // Adjust the remaining count of characters
        cNumeric--;
    }

    // Ensure that we read a full matrix
    if (cNumeric)
    {
        IFC_RETURN(E_FAIL);
    }

parsed:
    *pcSuffix = cString;
    *ppSuffix = pString;

    return S_OK;
}

_Check_return_ HRESULT Vector2FromString(
    const xstring_ptr_view& value,
    wfn::Vector2& convertedValue)
{
    std::array<float, 2> floatArray;
    IFC_RETURN(ArrayFromString(value, floatArray));
    convertedValue = { floatArray[0], floatArray[1]};
    return S_OK;
}

_Check_return_ HRESULT Vector3FromString(
    const xstring_ptr_view& value,
    wfn::Vector3& convertedValue)
{
    std::array<float, 3> floatArray;
    IFC_RETURN(ArrayFromString(value, floatArray));
    convertedValue = { floatArray[0], floatArray[1], floatArray[2] };
    return S_OK;
}

_Check_return_ HRESULT QuaternionFromString(
    const xstring_ptr_view& value,
    wfn::Quaternion& convertedValue)
{
    std::array<float, 4> floatArray;
    IFC_RETURN(ArrayFromString(value,  floatArray));
    convertedValue = { floatArray[0], floatArray[1], floatArray[2], floatArray[3] };
    return S_OK;
}

_Check_return_ HRESULT Matrix3x2FromString(
    const xstring_ptr_view& value,
    wfn::Matrix3x2& convertedValue)
{
    std::array<float, 6> floatArray;
    IFC_RETURN(ArrayFromString(value, floatArray));
    convertedValue =
    {
        floatArray[0], floatArray[1],
        floatArray[2], floatArray[3],
        floatArray[4], floatArray[5]
    };
    return S_OK;
}

_Check_return_ HRESULT Matrix4x4FromString(
    const xstring_ptr_view& value,
    wfn::Matrix4x4& convertedValue)
{
    std::array<float, 16> floatArray;
    IFC_RETURN(ArrayFromString(value, floatArray));
    convertedValue =
    {
        floatArray[0], floatArray[1], floatArray[2], floatArray[3],
        floatArray[4], floatArray[5], floatArray[6], floatArray[7],
        floatArray[8], floatArray[9], floatArray[10], floatArray[11],
        floatArray[12], floatArray[13], floatArray[14], floatArray[15]
    };
    return S_OK;
}

_Check_return_ HRESULT TimeSpanFromString(
    const xstring_ptr_view& value,
    wf::TimeSpan& convertedValue)
{
    double timeSpanInSec = 0.0;
    IFC_RETURN(TimeSpanFromString(value, &timeSpanInSec));
    convertedValue = TimeSpanUtil::FromSeconds(timeSpanInSec);
    return S_OK;
}
