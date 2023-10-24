// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <NoParentShareableDependencyObject.h>

_Check_return_ HRESULT CNoParentShareableDependencyObject::OnSkippedLiveEnter()
{
    return S_OK;
}

_Check_return_ HRESULT CNoParentShareableDependencyObject::OnSkippedLiveLeave()
{
    return S_OK;
}
