// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      Hardware composition commands

#pragma once

#include "command.h"

class CNotifyRenderStateChangedCommand : public CSchedulerCommandBase
{
public:
    CNotifyRenderStateChangedCommand(
        _In_ CompositorScheduler *pCompositorScheduler,
        bool isRenderEnabled
        );

    virtual ~CNotifyRenderStateChangedCommand();

    virtual _Check_return_ HRESULT Execute();
private:
    bool m_isRenderEnabled;
};
