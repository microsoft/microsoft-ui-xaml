// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "KeySpline.h"


_Check_return_ HRESULT CKeySpline::InvokeImpl(
    _In_ const CDependencyProperty *pdp,
    _In_opt_ CDependencyObject *pNamescopeOwner
    )
{
    return E_NOTIMPL;
}
