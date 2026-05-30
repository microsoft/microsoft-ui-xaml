// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "EtwEventWatcher.h"
#include "EtwConsumer.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { 

    ////////////////////////////////////////////////////////////////////////////////
    //
    // The ETWRuleService templated class is to be used as base class for rule services. It implements
    // common functionality that all rule services can benefit from. This is an internal class.
    //

    template<class ServiceClass, class ServiceInterface>
    class ETWRuleService
        : public ETWConsumer<ServiceClass, appanalysis::IRuleService, ServiceInterface>
    {
    public:

        ETWRuleService()
        {
        }

        virtual ~ETWRuleService()
        {
        }

        HRESULT RuntimeClassInitialize()
        {
            IFC_RETURN(ETWConsumer::RegisterEvents(&m_watcher));

            // Rule Service will be on by default
            wrl::ComPtr<appanalysis::IEtwEventWatcherPrivate> watcherPrivate;
            IFC_RETURN(m_watcher.As(&watcherPrivate));
            IFC_RETURN(watcherPrivate->Start());

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        static HRESULT CreateInstance(
            _COM_Outptr_ appanalysis::IRuleService** ppService
            )
        {
            ARG_VALIDRETURNPOINTER(ppService);
            *ppService = nullptr;

            wrl::ComPtr<ServiceClass> service;
            IFC_RETURN(wrl::MakeAndInitialize<ServiceClass>(&service));

            IFC_RETURN(service.CopyTo(ppService));

            return S_OK;

        }

    private:
        wrl::ComPtr<appanalysis::IEtwEventWatcher> m_watcher;

    };
} } }
