// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CoreWindowFocusObserver.h"

#include <CoreP.h>
#include <TreeWalker.h>

#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <DOPointerCast.h>
#include <UIElement.h>
#include <CControl.h>
#include <FocusMgr.h>
#include <Host.h>
#include <chrono>
#include <Transforms.h>
#include "FocusableHelper.h"
#include <TextElement.h>
#include <CoreWindow.h>

using namespace DirectUI;
using namespace Focus;
using namespace Jupiter;

CoreWindowFocusObserver::CoreWindowFocusObserver(_In_ CCoreServices *pCoreServices, _In_ CContentRoot* contentRoot)
    : FocusObserver(pCoreServices, contentRoot)
{
}

CoreWindowFocusObserver::~CoreWindowFocusObserver()
{
    VERIFYHR(DeInit());
}

_Check_return_ HRESULT
CoreWindowFocusObserver::DeInit()
{
    if (m_windowActivatedEventCookie.value!=0)
    {
        IFC_RETURN(m_spCoreWindow->remove_Activated(m_windowActivatedEventCookie));
        m_windowActivatedEventCookie.value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT
CoreWindowFocusObserver::Init(_In_ wuc::ICoreWindow* const pCoreWindow)
{
    m_spCoreWindow = pCoreWindow;
    IFC_RETURN(m_spCoreWindow->add_Activated(
        Microsoft::WRL::Callback<wf::ITypedEventHandler <
            wuc::CoreWindow*,
            wuc::WindowActivatedEventArgs*>>(
            this, &CoreWindowFocusObserver::OnActivated).Get(),
        &m_windowActivatedEventCookie));

    wuc::CoreWindowActivationMode activationMode = GetActivationMode();
    const bool active = (activationMode == wuc::CoreWindowActivationMode::CoreWindowActivationMode_ActivatedInForeground);
    if (active)
    {
        m_activeState = wuc::CoreWindowActivationState_CodeActivated;
    }

    return S_OK;
}

bool CoreWindowFocusObserver::IsActivated() const
{
    return !(m_activeState == wuc::CoreWindowActivationState_Deactivated);
}

wuc::CoreWindowActivationMode CoreWindowFocusObserver::GetActivationMode() const
{
    ctl::ComPtr<wuc::ICoreWindow5> spCoreWindow5;
    IFCFAILFAST(m_spCoreWindow.As(&spCoreWindow5));
    auto activationMode = wuc::CoreWindowActivationMode::CoreWindowActivationMode_None;
    IFCFAILFAST(spCoreWindow5->get_ActivationMode(&activationMode));

    return activationMode;
}

_Check_return_ HRESULT
CoreWindowFocusObserver::OnActivated(
    _In_ wuc::ICoreWindow* pSender,
    _In_ wuc::IWindowActivatedEventArgs* pArgs)
{
    IFC_RETURN(pArgs->get_WindowActivationState(&m_activeState));
    return S_OK;
}