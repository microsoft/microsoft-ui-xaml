// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BackingEtwRule.h"
#include "throw.h"
namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    ////////////////////////////////////////////////////////////////////////////////
    //
    // The Rule templated class is to be used as base class for Etw based rules. 
    // common functionality that all rules can benefit from. This is an internal class
    // that implements the IRule interface and also implements ETWConsumer, which is 
    // an internal class that is able to declare events and register them with an
    // EventWatcher.
    //
    typedef HRESULT (*pfnGetActivationFactory)(HSTRING, IActivationFactory**);
    template<class T, appanalysis::RuleCategories DeclaredRuleCategories = appanalysis::RuleCategories_None>
    class TestEtwRuleImpl
        : public EtwRuleImpl<T, DeclaredRuleCategories>
    {

    public:

        TestEtwRuleImpl()
            : GetFactory(nullptr)
        {
        }

        virtual ~TestEtwRuleImpl()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////
        // overriding register events to use the appanalysis class factory
        HRESULT RegisterTestEvents(
            _COM_Outptr_ appanalysis::IEtwEventWatcher** watcher
            )
        {
            ARG_VALIDRETURNPOINTER(watcher);
            *watcher = nullptr;

            wrl::ComPtr<appanalysis::IEtwEventWatcher> etwEventWatcher;
            IFC_RETURN(CreateWatcher(&etwEventWatcher));

            for (UINT i = 0; i < _countof(m_Providers); i++)
            {
                GUID currentProviderId = m_Providers[i].ProviderId;

                wrl::ComPtr<appanalysis::IEtwProvider> provider;
                IFC_RETURN(CreateProvider(&provider, m_Providers[i]));
                for (UINT j = 0; j < _countof(m_EventCallbacks); j++)
                {
                    if (IsEqualGUID(m_EventCallbacks[j].ProviderId, currentProviderId))
                    {
                        wrl::ComPtr<appanalysis::IEtwEvent> eventInfo;
                        IFC_RETURN(CreateEtwEvent(&eventInfo, provider.Get(), m_EventCallbacks[j].EventInfo));
                        auto callback = wrl::Callback<appanalysis::IEtwEventRecordCallback>(static_cast<T*>(this), m_EventCallbacks[j].Callback);
                        IFC_RETURN(etwEventWatcher->RegisterEvent(eventInfo.Get(), callback.Get()));
                    }
                }
            }

            *watcher = etwEventWatcher.Detach();
            return S_OK;
        }
        
        
        HRESULT CreateEtwRule(appanalysis::IEtwRule** rule, appanalysis::IEtwEventWatcher* watcher)
        {
            *rule = nullptr;
            IFC_RETURN(EnsureGetActivationFactory());

            wrl::ComPtr<IActivationFactory> factory;
            IFC_RETURN(GetFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwRule), &factory));

            wrl::ComPtr<appanalysis::IEtwRuleFactory> etwRuleFactory;
            IFC_RETURN(factory.As(&etwRuleFactory));

            IFC_RETURN(etwRuleFactory->CreateInstance(this, watcher, rule));

            return S_OK;
        }
        
        private:
        
        pfnGetActivationFactory GetFactory;
        
        HRESULT CreateWatcher(appanalysis::IEtwEventWatcher** watcher)
        {
            *watcher = nullptr;
            IFC_RETURN(EnsureGetActivationFactory());

            wrl::ComPtr<IActivationFactory> factory;
            IFC_RETURN(GetFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwEventWatcher), &factory));

            wrl::ComPtr<IInspectable> objAsInsp;
            IFC_RETURN(factory->ActivateInstance(&objAsInsp));
            IFC_RETURN(objAsInsp.CopyTo(watcher));
            
            return S_OK;
        }

        HRESULT CreateProvider(appanalysis::IEtwProvider** provider, const ProviderInfo& providerInfo)
        {
            *provider = nullptr;
            IFC_RETURN(EnsureGetActivationFactory());

            wrl::ComPtr<IActivationFactory> factory;
            IFC_RETURN(GetFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwProvider), &factory));

            wrl::ComPtr<appanalysis::IEtwProviderStatics> etwProviderFactory;
            IFC_RETURN(factory.As(&etwProviderFactory));

            if (providerInfo.ProviderType == appanalysis::ProviderType_Kernel)
            {
                IFC_RETURN(etwProviderFactory->CreateKernelProvider(providerInfo.ProviderId, providerInfo.EnableFlags, provider));
            }
            else if (providerInfo.ProviderType == appanalysis::ProviderType_Manifest)
            {
                IFC_RETURN(etwProviderFactory->Create(providerInfo.ProviderId, StringRef(providerInfo.ManifestPath), provider));
            }
            else
            {
                return E_INVALIDARG;
            }

            return S_OK;
        }

        HRESULT CreateEtwEvent(appanalysis::IEtwEvent** etwEvent, appanalysis::IEtwProvider* provider, const EventInfo& info)
        {
            *etwEvent = nullptr;
            IFC_RETURN(EnsureGetActivationFactory());

            wrl::ComPtr<IActivationFactory> factory;
            IFC_RETURN(GetFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwEvent), &factory));

            wrl::ComPtr<appanalysis::IEtwEventFactory> etwEventFactory;
            IFC_RETURN(factory.As(&etwEventFactory));

            IFC_RETURN(etwEventFactory->CreateInstance(info.EventId, info.EventVersion, provider, etwEvent));

            return S_OK;
        }

        HRESULT EnsureGetActivationFactory()
        {
            if (!GetFactory)
            {
                HMODULE dll = GetModuleHandle(L"Microsoft.Diagnostics.AppAnalysis.dll");
                IFCPTR_RETURN(dll);
                GetFactory = (pfnGetActivationFactory)(GetProcAddress(dll, "DllGetActivationFactory"));
                IFCPTR_RETURN(GetFactory);
            }

            return S_OK;
        }
   
    };

} } } 
