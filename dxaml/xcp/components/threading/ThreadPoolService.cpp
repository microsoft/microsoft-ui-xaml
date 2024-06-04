// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <windows.system.threading.h>
#include "ThreadPoolService.h"

bool ThreadPoolService::s_hasInstance = false;

ThreadPoolService::ThreadPoolService()
{
    IFCFAILFAST(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_System_Threading_ThreadPool).Get(),
        &m_spThreadPoolFactory));

    IFCFAILFAST(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_System_Threading_ThreadPoolTimer).Get(),
        &m_spThreadPoolTimerFactory));

    s_hasInstance = true;
}