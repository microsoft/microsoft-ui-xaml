// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wrl\module.h>

#include "TestRuleSet.h"
#include "EventProcessor.h"
#include "RuleServiceProvider.h"

namespace AppAnalysis { namespace Test {
    ActivatableClass(TestRuleSet);

} }

extern "C" BOOL WINAPI DllMain(
    _In_ HINSTANCE hInst,
    _In_ ULONG ulReason,
    _In_ LPVOID /*lpReserved*/
    )
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

STDAPI DllGetActivationFactory(
    _In_ HSTRING activatibleClassId,
    _COM_Outptr_ IActivationFactory** factory
    )
{
    auto &module = wrl::Module< wrl::InProc >::GetModule();
    return module.GetActivationFactory(activatibleClassId, factory);
}