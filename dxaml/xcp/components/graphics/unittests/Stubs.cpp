// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "encodedptr.h"
#include "D2d1.h"

//  Includes stubs for all the functions we need to get things to link.

IPALDebuggingServices * __stdcall GetPALDebuggingServices()
{
    ASSERT(false);
    return nullptr;
}

struct IPALThreadingServices;
IPALThreadingServices * __stdcall GetPALThreadingServices()
{
    ASSERT(false);
    return nullptr;
}

struct IPlatformServices;
EncodedPtr<IPlatformServices> gps;

#ifdef XcpMarkStrongPointer
#undef XcpMarkStrongPointer
#endif

extern "C" void XcpMarkStrongPointer(_In_ void* thisPtr, _In_ void* ptrToStrongPtr)
{
    ASSERT(false);
}

enum EReferenceTrackerWalkType
{
    RTW_Unpeg,
    RTW_Find,
    RTW_Peg,
    RTW_Reachable,
    RTW_None,
};

class CClassInfo;
class CDependencyProperty;
class CDependencyObject
{
public:
    const CClassInfo* GetClassInformation() const;
    const CDependencyProperty* GetContentProperty();

    void ReferenceTrackerWalk(
        _In_ EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer);

    bool OfTypeByIndex(_In_ KnownTypeIndex nIndex) const;

    void AddRef();
    void Release();
};

const CClassInfo* CDependencyObject::GetClassInformation() const
{
    return nullptr;
}

const CDependencyProperty* CDependencyObject::GetContentProperty()
{
    return nullptr;
}

bool CDependencyObject::OfTypeByIndex(_In_ KnownTypeIndex nIndex) const
{
    return false;
}

void CDependencyObject::ReferenceTrackerWalk(
    _In_ EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
}

void CDependencyObject::AddRef()
{
}

void CDependencyObject::Release()
{
}