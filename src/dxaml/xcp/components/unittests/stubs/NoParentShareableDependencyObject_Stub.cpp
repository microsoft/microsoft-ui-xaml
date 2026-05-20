// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "NoParentShareableDependencyObject.h"


_Check_return_ HRESULT CNoParentShareableDependencyObject::Enter(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    return CDependencyObject::Enter(pNamescopeOwner, params);
}

_Check_return_ HRESULT CNoParentShareableDependencyObject::Leave(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    return CDependencyObject::Leave(pNamescopeOwner, params);
}
