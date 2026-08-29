// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppAnalysisCommon.h"
#include "XamlLogging.h"
#include "MockTDH.h"
#include "EtwRuleSetTests.h"
#include "resource.h"

using namespace AppAnalysis::Test;
namespace Windows { namespace UI { namespace Xaml { namespace Tests {  namespace Tools { namespace AppAnalysis {

////////////////////////////////////////////////////////////////////////////////
// ValidateBasicProperties: Tests that properties on ETWRule work for the golden path.
//
void ETWRuleSetTests::ValidateBasicProperties()
{

    RuleTester ruleTester;
    wrl::ComPtr<wfc::IVectorView<appanalysis::EtwRule*>> rules = ruleTester.GetRules();

    UINT count = 0;
    LogThrow_IfFailed(rules->get_Size(&count));
    VERIFY_IS_GREATER_THAN(count, 0u); // verify we get some rules

    wrl::ComPtr<appanalysis::IEtwRule> etwRule;
    LogThrow_IfFailed(rules->GetAt(0, &etwRule));
    wrl::ComPtr<appanalysis::IRule> spRule;

    VERIFY_SUCCEEDED(etwRule->get_BackingRule(&spRule));
    VERIFY_IS_NOT_NULL(spRule);

    wrl::ComPtr<wfc::IVectorView<appanalysis::EtwEvent*>> eventInfo;
    // Test IRule interfaces
    {
        wil::unique_hstring ruleId;
        VERIFY_SUCCEEDED(spRule->get_Id(&ruleId));
        VERIFY_IS_TRUE(AppAnalysisHelpers::CompareStrings(ruleId.get(), StringRef(L"AA0001")) == 0);

        wil::unique_hstring ruleTitle;
        VERIFY_SUCCEEDED(spRule->get_Title(&ruleTitle));

        wil::unique_hstring linkTitle;
        wil::unique_hstring linkUri;
        VERIFY_SUCCEEDED(spRule->get_LinkTitle(&linkTitle));
        VERIFY_SUCCEEDED(spRule->get_LinkUri(&linkUri));
    }

    wrl::ComPtr<appanalysis::IEtwProvider> provider;
    // Get providers. This is a test for ETWRule
    {
        wrl::ComPtr<wfc::IVectorView<appanalysis::EtwEvent*>> registeredEvents;
        wrl::ComPtr<appanalysis::IEtwEventWatcher> eventWatcher;

        VERIFY_SUCCEEDED(etwRule->get_RegisteredEvents(&registeredEvents));

        UINT count = 0;
        LogThrow_IfFailed(registeredEvents->get_Size(&count));
        VERIFY_IS_GREATER_THAN(count, 0u);

        wrl::ComPtr<appanalysis::IEtwEvent> etwEvent;
        LogThrow_IfFailed(registeredEvents->GetAt(0, &etwEvent));

        VERIFY_SUCCEEDED(etwEvent->get_Provider(&provider));

        GUID providerId = GUID_NULL;
        LogThrow_IfFailed(provider->get_ID(&providerId));
        VERIFY_IS_TRUE(!!IsEqualGUID(providerId, WINDOWS_UI_XAML_ETW_PROVIDER));
    }

    // Get EventInfo. This is a test for ETWRule
    {
        appanalysis::ProviderType type;
        VERIFY_SUCCEEDED(provider->get_ProviderType(&type));
        VERIFY_ARE_EQUAL(type, appanalysis::ProviderType_Manifest);
    }

    // Get Manifest Path. This is a test for EtwProvider
    {
        wil::unique_hstring manifest;
        VERIFY_SUCCEEDED(provider->get_Manifest(&manifest));
        VERIFY_ARE_EQUAL(AppAnalysisHelpers::CompareStrings(manifest.get(), StringRef(L"Microsoft-Windows-XAML-ETW.man")), 0);
    }
}

} } } } } }
