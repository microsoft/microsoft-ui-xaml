// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "TransformGroup.h"


CTransformGroup::CTransformGroup(_In_ const CTransformGroup& original, _Out_ HRESULT& hr)
    : CTransform(original, hr)
{
}

CTransformGroup::~CTransformGroup()
{
}

void CTransformGroup::UpdateCachedTransform()
{
}
