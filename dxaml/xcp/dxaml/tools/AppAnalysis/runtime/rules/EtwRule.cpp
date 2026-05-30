// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ETWRule.h"
#include "helpers.h"
#include "RuleServiceProvider.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    IFACEMETHODIMP
    EtwRuleFactory::CreateInstance(
        _In_ appanalysis::IRule* backingRule,
        _In_ appanalysis::IEtwEventWatcher* eventWatcher,
        _COM_Outptr_ appanalysis::IEtwRule** instance
        )
    {
        ARG_VALIDRETURNPOINTER(instance);
        *instance = nullptr;

        wrl::ComPtr<EtwRule> rule;
        IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(&rule, backingRule, eventWatcher));

        *instance = rule.Detach();

        return S_OK;
    }

    HRESULT EtwRule::RuntimeClassInitialize(
        _In_ appanalysis::IRule* backingRule,
        _In_ appanalysis::IEtwEventWatcher* eventWatcher
        )
    {
        IFCPTR_RETURN(backingRule);
        IFCPTR_RETURN(eventWatcher);

        m_backingRule = backingRule;
        IFC_RETURN(AppAnalysisHelpers::As(eventWatcher, &m_watcher));

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwRule::get_ID
    //
    IFACEMETHODIMP EtwRule::get_BackingRule(
        _COM_Outptr_ appanalysis::IRule** rule
        )
    {
        ARG_VALIDRETURNPOINTER(rule);
        *rule = nullptr;

        IFC_RETURN(m_backingRule.CopyTo(rule));

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwRule::get_RegisteredEvents
    //
    IFACEMETHODIMP EtwRule::get_RegisteredEvents(
        _COM_Outptr_ wfc::IVectorView<appanalysis::EtwEvent*>** registeredEvents
        )
    {
        ARG_VALIDRETURNPOINTER(registeredEvents);
        *registeredEvents = nullptr;

        IFC_RETURN(m_watcher->get_RegisteredEvents(registeredEvents));

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwRule::Start()
    //
    IFACEMETHODIMP EtwRule::Start()
    {
        // always register for the source info service
        wrl::ComPtr<appanalysis::IRuleServiceProviderStatics> ruleServiceProvider;
        IFC_RETURN(RuleServiceProvider::GetProvider(&ruleServiceProvider));
        IFC_RETURN(ruleServiceProvider->RegisterService(__uuidof(appanalysis::ISourceInfoRuleService)));

        IFC_RETURN(m_watcher->Start());

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwRule::Stop()
    //
    IFACEMETHODIMP EtwRule::Stop()
    {
        IFC_RETURN(m_watcher->Stop());

        wrl::ComPtr<appanalysis::IRuleServiceProviderStatics> ruleServiceProvider;
        IFC_RETURN(RuleServiceProvider::GetProvider(&ruleServiceProvider));
        IFC_RETURN(ruleServiceProvider->UnregisterService(__uuidof(appanalysis::ISourceInfoRuleService)));

        return S_OK;
    }

} } }
