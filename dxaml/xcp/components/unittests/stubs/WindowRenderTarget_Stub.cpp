// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif
#include <WindowsPresentTarget.h>
#include <WindowRenderTarget.h>


_Check_return_ HRESULT CWindowRenderTarget::RequestMainDCompDeviceCommit()
{
    return E_NOTIMPL;
}

XUINT32 CWindowRenderTarget::GetWidth() const
{
    ASSERT(FALSE);
    return 0;
}

XUINT32 CWindowRenderTarget::GetHeight() const
{
    ASSERT(FALSE);
    return 0;
}

WindowsGraphicsDeviceManager* CWindowRenderTarget::GetGraphicsDeviceManager() const
{
    return nullptr;
}

xref_ptr<WindowsPresentTarget> CWindowRenderTarget::GetPresentTarget()
{
    return nullptr;
}

DCompTreeHost * CWindowRenderTarget::GetDCompTreeHost(void)
{
    return nullptr;
}

