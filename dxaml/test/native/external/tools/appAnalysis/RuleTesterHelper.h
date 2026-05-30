// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "assert.h"
#include "XamlOM.WinUI.h" // InstanceHandle definition
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class RuleTesterHelper
    {
    public:
        RuleTesterHelper(Platform::String^ ruleId, Platform::String^ testIdentifier, bool shouldHaveSourceInfo = true)
        {
            // if we don't have source info, enable visual diagnostics so that only care about notifications with live visual
            // trees.
            test_infra::AppAnalysisClientHelper::EnableRule(ruleId, testIdentifier, shouldHaveSourceInfo);
        }

        // Verify's the rule fired and that we got the expected count of notifications.
        void VerifyRuleTriggered(unsigned int expectedCount)
        {
            VERIFY_NO_THROW(test_infra::AppAnalysisClientHelper::VerifyRuleTriggered(expectedCount));
        }

        void VerifyRuleNotTriggered()
        {
            VERIFY_NO_THROW(test_infra::AppAnalysisClientHelper::VerifyRuleNotTriggered());
        }

        void VerifyMeasurement(unsigned int index, Microsoft::Diagnostics::AppAnalysis::MeasurementUnit unit, double value)
        {
           Microsoft::Diagnostics::AppAnalysis::Measurement measurement;
           measurement.Unit = unit;
           measurement.Value = value;
           VERIFY_NO_THROW(test_infra::AppAnalysisClientHelper::VerifyMeasurement(index, measurement));
        }

        void VerifySourceInfo(unsigned int index, Platform::String^ fileName, unsigned int line, unsigned int column)
        {
            Microsoft::Diagnostics::AppAnalysis::SourceInfo sourceInfo;
            sourceInfo.FileName = fileName;
            sourceInfo.LineNumber = line;
            sourceInfo.ColumnNumber = column;
            VERIFY_NO_THROW(test_infra::AppAnalysisClientHelper::VerifySourceInfo(index, sourceInfo));
        }
        
        void VerifyCanLinkToLVT(unsigned int index, InstanceHandle lvtElement)
        {
            VERIFY_NO_THROW(test_infra::AppAnalysisClientHelper::VerifyCanLinkToLVT(index, lvtElement));
        }

        template <typename... Args>
        void VerifyDescription(unsigned int index, unsigned int resourceId, Args... args)
        {

            Microsoft::Diagnostics::AppAnalysis::ResourceString^ description = MakeResourceString(resourceId, std::forward_as_tuple(args...), args...);
            VERIFY_NO_THROW(test_infra::AppAnalysisClientHelper::VerifyDescription(index, description));
        }

        ~RuleTesterHelper()
        {
            test_infra::AppAnalysisClientHelper::DisableCurrentRule();
        }

    private:

        template<typename T>
        Microsoft::Diagnostics::AppAnalysis::ResourceString^ MakeResourceString(unsigned int resourceId, T tuple, ...)
        {
            va_list vaList;
            va_start(vaList, tuple);

            Microsoft::Diagnostics::AppAnalysis::ResourceString^ description = ref new Microsoft::Diagnostics::AppAnalysis::ResourceString(resourceId);

            for (unsigned int i = 0; i < std::tuple_size<T>::value; ++i)
            {
                description->Append(ref new Platform::String(va_arg(vaList, LPCWSTR)));
            }
            va_end(vaList);

            return description;
        }
        RuleTesterHelper(const RuleTesterHelper&) = delete;
        RuleTesterHelper& operator=(const RuleTesterHelper&) = delete;
    };

}}}}}
