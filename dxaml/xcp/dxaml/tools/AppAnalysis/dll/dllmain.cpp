// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wrl\module.h>

#include "resource.h"
#include "EtwEventRecord.h"
#include "EventProcessor.h"
#include "EtwRuleSet.h"
#include "RuleServiceProvider.h"
#include "RuleTriggeredEventArgs.h"
#include "EtwEventInfo.h"
#include "EtwProvider.h"
#include "EtwRule.h"
#include "EtwEventWatcher.h"
#include "ResourceString.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    ActivatableClass(EtwEventWatcher);
    ActivatableClass(EtwRuleSet);
    
    ActivatableClassWithFactory(RuleTriggeredEventArgs, RuleTriggeredEventArgsFactory);
    ActivatableClassWithFactory(ResourceString, ResourceStringFactory);
    ActivatableClassWithFactory(EtwEvent, EtwEventFactory);
    ActivatableClassWithFactory(EtwProvider, EtwProviderFactory);
    ActivatableClassWithFactory(EtwRule, EtwRuleFactory);

    ActivatableStaticOnlyFactory(EventProcessor);
    ActivatableStaticOnlyFactory(RuleServiceProvider);

} } }

STDAPI DllGetActivationFactory(
    _In_ HSTRING activatibleClassId,
    _COM_Outptr_ IActivationFactory** factory
    )
{
    auto &module = wrl::Module< wrl::InProc >::GetModule();
    return module.GetActivationFactory(activatibleClassId, factory);
}

_Check_return_ STDAPI ProcessEvent(
    _In_ PEVENT_RECORD eventRecord
    )
{
    IFC_RETURN(Microsoft::Diagnostics::AppAnalysis::EventProcessor::ProcessEventStatic(eventRecord));

    return S_OK;
}

