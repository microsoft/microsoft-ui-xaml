// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EasingFunctionBase_partial.h"
#include <EasingFunctions.h>

using namespace DirectUI;
using namespace xaml;

_Check_return_ HRESULT DirectUI::EasingFunctionBase::EaseImpl(_In_ DOUBLE normalizedTime, _Out_ DOUBLE* returnValue)
{
    HRESULT hr = S_OK;
    XFLOAT fAlpha = 0.0f;
    XFLOAT fNormalizedTime = static_cast<XFLOAT>(normalizedTime);

    IFCPTR(returnValue);

    IFC(CoreImports::EasingFunctionBase_Ease(
        static_cast<CEasingFunctionBase*>(GetHandle()),
        fNormalizedTime,
        &fAlpha));

    *returnValue = static_cast<DOUBLE>(fAlpha);

Cleanup:
    RRETURN(hr);
}

