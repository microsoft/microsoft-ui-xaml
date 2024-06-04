// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "focusmgr.h"
#include "contentroot.h"

CFocusManager::CFocusManager(_In_ CCoreServices* pCoreService, _In_ CContentRoot& contentRoot)
    : m_contentRoot(contentRoot)
{
}

CFocusManager::~CFocusManager()
{
}

XUINT32 CFocusManager::AddRef()
{
    return 0;
}

XUINT32 CFocusManager::Release()
{
    return 0;
}

CFocusRectManager::~CFocusRectManager()
{
}

void CFocusManager::ReleaseFocusRectManagerResources(void)
{
}

FocusRect::RevealFocusAnimator::~RevealFocusAnimator()
{
}

FocusObserver::FocusObserver(_In_ CCoreServices *pCoreServices, _In_ CContentRoot* contentRoot)
{
}

FocusObserver::~FocusObserver()
{
}

_Check_return_ HRESULT FocusObserver::Init(_In_opt_ xaml_hosting::IFocusController* const pFocusController)
{
    return E_NOTIMPL;
}

wuc::CoreWindowActivationMode FocusObserver::GetActivationMode() const
{
    return wuc::CoreWindowActivationMode::CoreWindowActivationMode_None;
}