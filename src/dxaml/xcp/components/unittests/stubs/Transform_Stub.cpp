// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "Transform.h"


_Check_return_ HRESULT CTransform::TransformPoints(
    _In_reads_(cPoints) XPOINTF *pptOriginal,
    _Inout_updates_(cPoints) XPOINTF *pptTransformed,
    XUINT32 cPoints
    )
{
    return E_NOTIMPL;
}
