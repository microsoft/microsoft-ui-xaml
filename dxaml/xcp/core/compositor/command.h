// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      Composition command interfaces

#pragma once

class UIThreadScheduler;
class CompositorScheduler;
class HWTextureManager;

//
// Interface for one-shot commands to be executed by the compositor scheduler prior to frame
// processing. These commands are independent of a compositor.
//
struct ISchedulerCommand
{
    virtual ~ISchedulerCommand() { }

    virtual _Check_return_ HRESULT Execute() = 0;
};

class CSchedulerCommandBase : public ISchedulerCommand
{
public:
    CSchedulerCommandBase(_In_ CompositorScheduler *pCompositorScheduler)
        : m_pCompositorSchedulerNoRef(pCompositorScheduler)
    {
        XCP_WEAK(&m_pCompositorSchedulerNoRef);
    }
protected:
    _Notnull_ CompositorScheduler *m_pCompositorSchedulerNoRef;
};
