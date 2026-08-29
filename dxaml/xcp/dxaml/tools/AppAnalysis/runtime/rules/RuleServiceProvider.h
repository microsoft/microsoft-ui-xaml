// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <map>
#include "helpers.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

////////////////////////////////////////////////////////////////////////////////
//
// RuleServiceProvider
//
class RuleServiceProvider
    : public wrl::ActivationFactory<appanalysis::IRuleServiceProviderStatics>
{
    InspectableClassStatic(
        RuntimeClass_Microsoft_Diagnostics_AppAnalysis_RuleServiceProvider, 
        TrustLevel::BaseTrust
        );

public:
    RuleServiceProvider();
    virtual ~RuleServiceProvider();

    HRESULT RuntimeClassInitialize();
    static HRESULT GetProvider(_COM_Outptr_ appanalysis::IRuleServiceProviderStatics** singleton);

    //
    // IRuleServiceProviderStatics
    //
    IFACEMETHOD(RegisterService)(
        _In_ GUID serviceId
        ) override;

    IFACEMETHOD(UnregisterService)(
        _In_ GUID serviceId
        ) override;

    IFACEMETHOD(GetService)(
        _In_ GUID serviceId,
        _COM_Outptr_ appanalysis::IRuleService** instance
        ) override;

private:

    HRESULT CreateRuleService(
        _In_ const GUID& iid,
        _COM_Outptr_ appanalysis::IRuleService** service
        );
    
    struct RuleService
    {
        wrl::ComPtr<appanalysis::IRuleService>   Service;
        unsigned int                UseCount = 0;
    };
    std::map<GUID, RuleService, AppAnalysisHelpers::GuidMapComparer> m_services;
   
};
} } }
