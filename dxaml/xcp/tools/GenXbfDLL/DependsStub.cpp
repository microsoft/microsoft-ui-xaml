// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// CDependencyObject Stub
//   There are no actual CDO's created but XamlQualifiedObject
//   in its destructor calls UnpegManagedPeerNoRef, so we
//   need a stub implementation to keep the linker happy.

#include "precomp.h"

void CDependencyObject::UnpegManagedPeerNoRef()
{
    ASSERT(FALSE);
}

void __thiscall CDependencyObject::UnpegManagedPeer(bool) { }

long __thiscall CDependencyObject::PegManagedPeer(bool,bool*) { return S_OK; }

void CDependencyObject::AddRefImpl(UINT32 cRef)
{
}

void CDependencyObject::ReleaseImpl(UINT32 cRef)
{
    if (cRef == 0)
    {
        delete this;
    }
}
