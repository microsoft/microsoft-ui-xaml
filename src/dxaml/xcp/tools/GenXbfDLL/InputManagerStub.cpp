// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"

CInputServices::CInputServices(CCoreServices*)
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

HRESULT CDragDropState::ClearCache(bool)
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