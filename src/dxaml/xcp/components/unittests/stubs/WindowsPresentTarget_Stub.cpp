// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif
#include <WindowsPresentTarget.h>


_Check_return_ long WindowsPresentTarget::CreateCompositedWindowlessPresentTarget(unsigned int,unsigned int, _In_ struct IViewObjectPresentNotifySite *, _Outptr_ class WindowsPresentTarget **ppPresentTarget)
{
    *ppPresentTarget = nullptr;
    return 0;
}

