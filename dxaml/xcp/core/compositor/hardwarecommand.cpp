// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "hardwarecommand.h"
#include "compositorscheduler.h"

//------------------------------------------------------------------------------
//
//  Synopsis:
//     Ctor
//
//------------------------------------------------------------------------------
CNotifyRenderStateChangedCommand::CNotifyRenderStateChangedCommand(
    _In_ CompositorScheduler *pCompositorScheduler,
    bool isRenderEnabled)
    : CSchedulerCommandBase(pCompositorScheduler)
    , m_isRenderEnabled(isRenderEnabled)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//     Dtor
//
//------------------------------------------------------------------------------
CNotifyRenderStateChangedCommand::~CNotifyRenderStateChangedCommand()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on render thread. Propagates render state information
//      to the CompositorScheduler.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CNotifyRenderStateChangedCommand::Execute()
{
    m_pCompositorSchedulerNoRef->EnableRender(m_isRenderEnabled);
    RRETURN(S_OK);
}
