// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Description:
//      Functions to prevent integer overflow bugs

#ifndef _INTSAFE_H_INCLUDED_
#define _INTSAFE_H_INCLUDED_

#define E_ARITHMETIC_OVERFLOW 0x80070216

//-----------------------------------------------------------------------------
//  Type convertors
//-----------------------------------------------------------------------------

XCP_FORCEINLINE
_Check_return_ HRESULT
Int8ToUInt8(
_In_ XINT8 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int8ToUInt16(
_In_ XINT8 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int8ToUInt32(
_In_ XINT8 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int8ToUInt64(
_In_ XINT8 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT64(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToInt8(
_In_ XUINT8 Source,
_Out_ XINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XINT8(0x7f))
    {
       *pTarget = XINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToInt16(
_In_ XUINT8 Source,
_Out_ XINT16 *pTarget)
{
    HRESULT hr;

   *pTarget = XINT16(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToUInt16(
_In_ XUINT8 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

   *pTarget = XUINT16(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToInt32(
_In_ XUINT8 Source,
_Out_ XINT32 *pTarget)
{
    HRESULT hr;

   *pTarget = XINT32(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToUInt32(
_In_ XUINT8 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

   *pTarget = XUINT32(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToInt64(
_In_ XUINT8 Source,
_Out_ XINT64 *pTarget)
{
    HRESULT hr;

   *pTarget = XINT64(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8ToUInt64(
_In_ XUINT8 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

   *pTarget = XUINT64(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int16ToUInt8(
_In_ XINT16 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if ((Source < 0) || (Source > XUINT8(~0)))
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int16ToUInt16(
_In_ XINT16 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int16ToUInt32(
_In_ XINT16 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int16ToUInt64(
_In_ XINT16 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT64(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToInt8(
_In_ XUINT16 Source,
_Out_ XINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XINT8(0x7f))
    {
       *pTarget = XINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToUInt8(
_In_ XUINT16 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XUINT8(~0))
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToInt16(
_In_ XUINT16 Source,
_Out_ XINT16 *pTarget)
{
    HRESULT hr;

    if (Source > XINT16(0x7fff))
    {
       *pTarget = XINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToInt32(
_In_ XUINT16 Source,
_Out_ XINT32 *pTarget)
{
    HRESULT hr;

   *pTarget = XINT32(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToUInt32(
_In_ XUINT16 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

   *pTarget = XUINT32(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToInt64(
_In_ XUINT16 Source,
_Out_ XINT64 *pTarget)
{
    HRESULT hr;

   *pTarget = XINT64(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16ToUInt64(
_In_ XUINT16 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

   *pTarget = XUINT64(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int32ToUInt8(
_In_ XINT32 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if ((Source < 0) || (Source > XUINT8(~0)))
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int32ToUInt16(
_In_ XINT32 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

    if ((Source < 0) || (Source > XUINT16(~0)))
    {
       *pTarget = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int32ToUInt32(
_In_ XINT32 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int32ToUInt64(
_In_ XINT32 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT64(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToInt8(
_In_ XUINT32 Source,
_Out_ XINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XINT8(0x7f))
    {
       *pTarget = XINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToUInt8(
_In_ XUINT32 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XUINT8(~0))
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToInt16(
_In_ XUINT32 Source,
_Out_ XINT16 *pTarget)
{
    HRESULT hr;

    if (Source > XINT16(0x7fff))
    {
       *pTarget = XINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToUInt16(
_In_ XUINT32 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

    if (Source > XUINT16(~0))
    {
       *pTarget = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToInt32(
_In_ XUINT32 Source,
_Out_ XINT32 *pTarget)
{
    HRESULT hr;

    if (Source > XINT32(0x7fffffff))
    {
       *pTarget = XINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToInt64(
_In_ XUINT32 Source,
_Out_ XINT64 *pTarget)
{
    HRESULT hr;

   *pTarget = XINT64(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32ToUInt64(
_In_ XUINT32 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

   *pTarget = XUINT64(Source);
    hr = S_OK;

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int64ToUInt8(
_In_ XINT64 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if ((Source < 0) || (Source > XUINT8(~0)))
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int64ToUInt16(
_In_ XINT64 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

    if ((Source < 0) || (Source > XUINT16(~0)))
    {
       *pTarget = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int64ToUInt32(
_In_ XINT64 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

    if ((Source < 0) || (Source > XUINT32(~0)))
    {
       *pTarget = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
Int64ToUInt64(
_In_ XINT64 Source,
_Out_ XUINT64 *pTarget)
{
    HRESULT hr;

    if (Source < 0)
    {
       *pTarget = XUINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT64(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToInt8(
_In_ XUINT64 Source,
_Out_ XINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XINT8(0x7f))
    {
       *pTarget = XINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToUInt8(
_In_ XUINT64 Source,
_Out_ XUINT8 *pTarget)
{
    HRESULT hr;

    if (Source > XUINT8(~0))
    {
       *pTarget = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT8(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToInt16(
_In_ XUINT64 Source,
_Out_ XINT16 *pTarget)
{
    HRESULT hr;

    if (Source > XINT16(0x7fff))
    {
       *pTarget = XINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToUInt16(
_In_ XUINT64 Source,
_Out_ XUINT16 *pTarget)
{
    HRESULT hr;

    if (Source > XUINT16(~0))
    {
       *pTarget = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT16(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToInt32(
_In_ XUINT64 Source,
_Out_ XINT32 *pTarget)
{
    HRESULT hr;

    if (Source > XINT32(0x7fffffff))
    {
       *pTarget = XINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToUInt32(
_In_ XUINT64 Source,
_Out_ XUINT32 *pTarget)
{
    HRESULT hr;

    if (Source > XUINT32(~0))
    {
       *pTarget = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XUINT32(Source);
        hr = S_OK;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64ToInt64(
_In_ XUINT64 Source,
_Out_ XINT64 *pTarget)
{
    HRESULT hr;

    if (Source > XINT64(0x7fffffffffffffffLL))
    {
       *pTarget = XINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }
    else
    {
       *pTarget = XINT64(Source);
        hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//  Addition functions
//-----------------------------------------------------------------------------

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8Add(
_In_ XUINT8 Augend,
_In_ XUINT8 Addend,
_Out_ XUINT8 *pResult)
{
    HRESULT hr;

    if (XUINT8(Augend + Addend) >= Augend)
    {
       *pResult = XUINT8(Augend + Addend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16Add(
_In_ XUINT16 Augend,
_In_ XUINT16 Addend,
_Out_ XUINT16 *pResult)
{
    HRESULT hr;

    if (XUINT16(Augend + Addend) >= Augend)
    {
       *pResult = XUINT16(Augend + Addend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32Add(
_In_ XUINT32 Augend,
_In_ XUINT32 Addend,
_Out_ XUINT32 *pResult)
{
    HRESULT hr;

    if (XUINT32(Augend + Addend) >= Augend)
    {
       *pResult = XUINT32(Augend + Addend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64Add(
_In_ XUINT64 Augend,
_In_ XUINT64 Addend,
_Out_ XUINT64 *pResult)
{
    HRESULT hr;

    if (XUINT64(Augend + Addend) >= Augend)
    {
       *pResult = XUINT64(Augend + Addend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

template<class XTemp, class YTemp>
XCP_FORCEINLINE
_Check_return_ HRESULT
templateAdd(
_In_ XTemp Augend,
_In_ YTemp Addend,
_Out_ XTemp *pResult)
{
    HRESULT hr;

    if (XTemp(Augend + Addend) >= Augend)
    {
       *pResult = XTemp(Augend + Addend);
        hr = S_OK;
    }
    else
    {
       *pResult = XTemp(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//  Subtraction functions
//-----------------------------------------------------------------------------

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8Sub(
_In_ XUINT8 Minuend,
_In_ XUINT8 Subtrahend,
_Out_ XUINT8 *pResult)
{
    HRESULT hr;

    if (Minuend >= Subtrahend)
    {
       *pResult = XUINT8(Minuend - Subtrahend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT8(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16Sub(
_In_ XUINT16 Minuend,
_In_ XUINT16 Subtrahend,
_Out_ XUINT16 *pResult)
{
    HRESULT hr;

    if (Minuend >= Subtrahend)
    {
       *pResult = XUINT16(Minuend - Subtrahend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT16(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32Sub(
_In_ XUINT32 Minuend,
_In_ XUINT32 Subtrahend,
_Out_ XUINT32 *pResult)
{
    HRESULT hr;

    if (Minuend >= Subtrahend)
    {
       *pResult = XUINT32(Minuend - Subtrahend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT32(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt64Sub(
_In_ XUINT64 Minuend,
_In_ XUINT64 Subtrahend,
_Out_ XUINT64 *pResult)
{
    HRESULT hr;

    if (Minuend >= Subtrahend)
    {
       *pResult = XUINT64(Minuend - Subtrahend);
        hr = S_OK;
    }
    else
    {
       *pResult = XUINT64(~0);
        hr = E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//  Multiplication functions
//-----------------------------------------------------------------------------

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt8Mul(
_In_ XUINT8 Multiplicand,
_In_ XUINT8 Multiplier,
_Out_ XUINT8 *pResult)
{
    XUINT16 temp;

    temp = Multiplicand * Multiplier;


    return UInt16ToUInt8(temp, pResult);
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt16Mul(
_In_ XUINT16 Multiplicand,
_In_ XUINT16 Multiplier,
_Out_ XUINT16 *pResult)
{
    XUINT32 temp;

    temp = Multiplicand * Multiplier;


    return UInt32ToUInt16(temp, pResult);
}

XCP_FORCEINLINE
_Check_return_ HRESULT
UInt32Mul(
_In_ XUINT32 Multiplicand,
_In_ XUINT32 Multiplier,
_Out_ XUINT32 *pResult)
{
    XUINT64 temp;

    temp = (XUINT64) Multiplicand * (XUINT64) Multiplier;


    return UInt64ToUInt32(temp, pResult);
}

// Make up for a minor difference between xcpsafe.h and intsafe.h:
#define UInt32Mult UInt32Mul

template<class XTemp, class YTemp>
XCP_FORCEINLINE
_Check_return_ HRESULT
templateMul(
_In_ XTemp Multiplicand,
_In_ YTemp Multiplier,
_Out_ XTemp *pResult)
{
    XUINT64 temp;

    temp = Multiplicand * Multiplier;

    if ( sizeof(XTemp) == 4 )
    {
        return UInt64ToUInt32(temp, (XUINT32*)pResult);
    }
    else if ( sizeof(XTemp) == 2 )
    {
        return UInt64ToUInt16(temp, (XUINT16*)pResult);
    }
    *pResult = temp;
    return S_OK;
}

#endif // _INTSAFE_H_INCLUDED_

