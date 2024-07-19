// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SystemBackdrop.h"

class CCoreServices;

CSystemBackdrop::CSystemBackdrop(_In_ CCoreServices *pCore)
    : CMultiParentShareableDependencyObject(pCore)
{
}
