// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"

// GenXbf links this stub in place of the real InputServices.cpp (see GenXbf.vcxproj), so it must also
// supply trivial definitions for ImeFocusPark's out-of-line ctor/dtor: CInputServices owns one by value
// (m_imeFocusPark), so the CInputServices ctor/dtor below construct and destroy it. A local empty Impl
// satisfies the std::unique_ptr<Impl> complete-type requirement in the destructor. The real behavior
// lives in InputServices.cpp and is never needed by the XBF generation tool.
struct ImeFocusPark::Impl {};

ImeFocusPark::ImeFocusPark() = default;

ImeFocusPark::~ImeFocusPark() = default;

CInputServices::CInputServices(_In_ CCoreServices*)
{
}

CInputServices::~CInputServices()
{
}

void CInputServices::DestroyPointerObjects(void)
{
}

XUINT32 CInputServices::Release(void)
{
    return 0;
}

ULONG CEventArgs::Release(void)
{
    return 0;
}

XUINT32 CEventManager::Release(void)
{
    return 0;
}

KeyTipManager::~KeyTipManager(void)
{
}

void CInteractionManager::DestroyAllInteractionEngine(void)
{
}

_Check_return_ HRESULT CDragDropState::ClearCache(bool)
{
    return S_OK;
}

KeyTip::~KeyTip(void)
{
}

_Check_return_ long KeyTipManager::Execute(void)
{
    return 0;
}

void ElementGestureTracker::Destroy()
{
}