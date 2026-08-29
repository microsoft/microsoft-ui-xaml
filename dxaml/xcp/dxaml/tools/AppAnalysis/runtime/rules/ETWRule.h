// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EtwEventWatcher.h"

#define MAX_FORMAT_STRING 1024

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    //
    class EtwRuleFactory
        : public wrl::AgileActivationFactory<appanalysis::IEtwRuleFactory>
    {
    public:
        // IEtwRuleFactory
        IFACEMETHOD(CreateInstance)(
            _In_ appanalysis::IRule* elementId,
            _In_ appanalysis::IEtwEventWatcher* watcher,
            _COM_Outptr_ appanalysis::IEtwRule** ppInstance
            ) override;
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // The EtwRule templated class is to be used as base class for rules. It implements
    // common functionality that all rules can benefit from. This is an internal class.
    //

    class EtwRule
        : public wrl::RuntimeClass<appanalysis::IEtwRule, wrl::FtmBase>
    {
        InspectableClass(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwRule, BaseTrust);

    public:

        EtwRule()
        {
        }

        virtual ~EtwRule()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        HRESULT RuntimeClassInitialize(
            _In_ appanalysis::IRule* rule,
            _In_ appanalysis::IEtwEventWatcher* eventWatcher
            );

        ////////////////////////////////////////////////////////////////////////////////
        //
        IFACEMETHOD(get_BackingRule)(
            _COM_Outptr_ appanalysis::IRule** rule
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        //
        IFACEMETHOD(get_RegisteredEvents)(
            _COM_Outptr_ wfc::IVectorView<appanalysis::EtwEvent*>** registeredEvents
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        //
        IFACEMETHOD(Start)() override;

        ////////////////////////////////////////////////////////////////////////////////
        //
        IFACEMETHOD(Stop)() override;

    private:
        wrl::ComPtr<appanalysis::IRule> m_backingRule;
        wrl::ComPtr<appanalysis::IEtwEventWatcherPrivate> m_watcher;

    };


} } } 
