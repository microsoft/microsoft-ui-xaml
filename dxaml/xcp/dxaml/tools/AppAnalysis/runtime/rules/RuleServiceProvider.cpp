// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuleServiceProvider.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { 
    HRESULT SourceInfoRuleService_CreateInstance(appanalysis::IRuleService**);

    RuleServiceProvider::RuleServiceProvider()
    {
    }

    RuleServiceProvider::~RuleServiceProvider()
    {
    }

    HRESULT RuleServiceProvider::RuntimeClassInitialize()
    {
        return S_OK;
    }

    HRESULT RuleServiceProvider::GetProvider(_COM_Outptr_ appanalysis::IRuleServiceProviderStatics** singleton)
    {
        wrl::ComPtr<IActivationFactory> factory;
        IFC_RETURN(wrl::Module< wrl::InProc >::GetModule().GetActivationFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_RuleServiceProvider), &factory));
        IFC_RETURN(factory.CopyTo(singleton));
    
        return S_OK;
    }

    IFACEMETHODIMP RuleServiceProvider::RegisterService(
        _In_ GUID serviceId)
    {
        auto iter = m_services.find(serviceId);
        if (iter == m_services.end())
        {
            // Insert a new node if this service doesn't exist
            RuleService newService;
            IFC_RETURN(CreateRuleService(serviceId, &newService.Service));
            IFCSTL_RETURN(
            {
                auto pair = m_services.emplace(serviceId, std::move(newService));
                iter = pair.first;
            });
        }
        // Increment the use count
        iter->second.UseCount++;

        return S_OK;
    }

    IFACEMETHODIMP RuleServiceProvider::UnregisterService(
        _In_ GUID serviceId)
    {
        auto iter = m_services.find(serviceId);
        if (iter != m_services.end())
        {
            // Remove service on last unregister
            if (--iter->second.UseCount == 0)
            {
                m_services.erase(iter);
            }
        }

        return S_OK;
    }


    IFACEMETHODIMP RuleServiceProvider::GetService(
        _In_ GUID serviceId,
        _COM_Outptr_ appanalysis::IRuleService** service)
    {

        auto iter = m_services.find(serviceId);
        if (iter == m_services.end())
        {
            return E_NOTFOUND;
        }

        IFC_RETURN(iter->second.Service.CopyTo(service));

        return S_OK;
    }

    HRESULT RuleServiceProvider::CreateRuleService(_In_ const GUID & iid, _COM_Outptr_ appanalysis::IRuleService ** service)
    {
        if (InlineIsEqualGUID(iid, appanalysis::IID_ISourceInfoRuleService))
        {
            return SourceInfoRuleService_CreateInstance(service);
        }

        return E_NOINTERFACE;
    }

} } }
