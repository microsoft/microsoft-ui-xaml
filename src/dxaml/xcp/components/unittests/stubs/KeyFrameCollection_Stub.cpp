// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xstring_ptr.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "KeyFrameCollection.h"


_Check_return_ HRESULT CKeyFrameCollection::InitializeKeyFrames(_In_ XFLOAT rOneIterationDuration)
{
    return E_NOTIMPL;
}

void CKeyFrameCollection::OriginateInvalidKeyFrameError()
{
}
