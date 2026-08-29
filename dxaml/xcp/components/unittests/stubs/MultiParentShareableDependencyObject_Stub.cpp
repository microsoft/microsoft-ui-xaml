// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "MultiParentShareableDependencyObject.h"


_Check_return_ HRESULT CMultiParentShareableDependencyObject::AddParent(
    _In_ CDependencyObject *pNewParent,
    bool fPublic /*= true */,
    _In_opt_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler /*= NULL */
    )
{
    return S_OK;
}

_Check_return_ HRESULT CMultiParentShareableDependencyObject::RemoveParent(_In_ CDependencyObject *pParentToRemove)
{
    return S_OK;
}

void CMultiParentShareableDependencyObject::NWPropagateDirtyFlag(DirtyFlags flags)
{
}
