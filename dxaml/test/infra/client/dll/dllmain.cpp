// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TestServices.h>
#include <WindowHelperStatics.h>
#include <TraceConsumer.h>
#include <ETWWaiterClientHelper.h>
#include <StoryboardMonitor.h>
#include <AppAnalysisClientHelper.h>
#include <PrivateModule.h>
#include <TestHostSettings.h>
#include "IslandHelper.h"
#include "VisualTreeVerifier.h"

namespace Private { namespace Infrastructure {

ActivatableStaticOnlyFactory(TestServicesStatics);
ActivatableStaticOnlyFactory(WindowHelperStatics);
ActivatableStaticOnlyFactory(TraceConsumerStatics);
ActivatableStaticOnlyFactory(ETWWaiterHelperStatics);
ActivatableStaticOnlyFactory(StoryboardMonitorStatics);
ActivatableStaticOnlyFactory(AppAnalysisClientHelperStatics);
ActivatableStaticOnlyFactory(TestHostSettingsStatics);
ActivatableClassWithFactory(IslandHelper, IslandHelperStatics);
ActivatableClassWithFactory(VisualTreeVerifier, VisualTreeVerifierStatics);

bool PrivateInfraModule::s_bFinalRelease = false;

PrivateInfraModule::~PrivateInfraModule()
{
    SetFinalRelease();
}

} }

extern "C" BOOL WINAPI DllMain(_In_ HINSTANCE, _In_ unsigned int dwReason, _In_opt_ void *)
{
    if (dwReason == DLL_PROCESS_DETACH)
    {
        auto &moduleRef = Private::Infrastructure::PrivateInfraModule::GetModule();
        moduleRef.SetFinalRelease();
    }
    return TRUE;
}

STDAPI DllGetActivationFactory(_In_ HSTRING activatibleClassId, _COM_Outptr_ IActivationFactory** factory)
{
    auto &moduleRef = Private::Infrastructure::PrivateInfraModule::GetModule();
    return moduleRef.GetActivationFactory(activatibleClassId, factory);
}

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID *ppv)
{
    auto &moduleRef = Private::Infrastructure::PrivateInfraModule::GetModule();
    return moduleRef.GetClassObject(rclsid, riid, ppv);
}

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow()
{
    auto &moduleRef = Private::Infrastructure::PrivateInfraModule::GetModule();
    return moduleRef.Terminate() ? S_OK : S_FALSE;
}
