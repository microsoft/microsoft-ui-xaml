// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {
    
    class AppAnalysisClientHelperStatics : public wrl::AgileActivationFactory<test_infra::IAppAnalysisClientHelperStatics>
        {
            InspectableClassStatic(RuntimeClass_Private_Infrastructure_AppAnalysisClientHelper, TrustLevel::BaseTrust);

        public:

            IFACEMETHOD(EnableRule)(
                HSTRING ruleId,
                HSTRING testIdentifier,
                BOOLEAN shouldHaveSourceInfo
                );

            IFACEMETHOD(VerifyRuleTriggered)(
                _In_ unsigned int timesTriggered
                );

            IFACEMETHOD(VerifyMeasurement)(
                _In_ unsigned int triggeredRuleIndex,
                _In_ appanalysis::Measurement measurement
                );

            IFACEMETHOD(VerifySourceInfo)(
                _In_ unsigned int triggeredRuleIndex,
                _In_ appanalysis::SourceInfo sourceInfo
                );

            IFACEMETHOD(VerifyCanLinkToLVT)(
                _In_ unsigned int triggeredRuleIndex,
                _In_ unsigned long long lvtHandle
                );

            IFACEMETHOD(VerifyDescription)(
                _In_ unsigned int triggeredRuleIndex,
                _In_ appanalysis::IResourceString* description
                );

            IFACEMETHOD(DisableCurrentRule());
            IFACEMETHOD(VerifyRuleNotTriggered());
        };
    }
}
