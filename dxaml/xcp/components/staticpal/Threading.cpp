// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XINT32
PAL_InterlockedCompareExchange(
    _Inout_ XINT32 *pTarget,
    _In_ XINT32 Exchange,
    _In_ XINT32 Comperand
)
{
    return ::InterlockedCompareExchange((LONG*)pTarget, Exchange, Comperand);
}

XINT32
PAL_InterlockedDecrement(_Inout_ XINT32 *pnTarget)
{
    return ::InterlockedDecrement((LONG *) pnTarget);
}

XINT32
PAL_InterlockedExchange(
    _Inout_ XINT32 *pnTarget,
    _In_ XINT32 nValue
)
{
    return ::InterlockedExchange((LONG *) pnTarget, nValue);
}

XINT32
PAL_InterlockedIncrement(_Inout_ XINT32 *pnTarget)
{
    return ::InterlockedIncrement((LONG *) pnTarget);
}