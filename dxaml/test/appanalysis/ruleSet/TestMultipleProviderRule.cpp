// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "TestEtwRuleImpl.h"

// The TestMultipleProvidersRule is a rule that detects if developers are not using correct programming
// techniques when rendering images on the screen. If the image control is not in the live tree
// before the source is set on the bitmap, then Decode to Render size feature can not be used.
using namespace Microsoft::Diagnostics::AppAnalysis;

class TestMultipleProvidersRule
    : public TestEtwRuleImpl<TestMultipleProvidersRule, appanalysis::RuleCategories_Performance>
{

public:

    TestMultipleProvidersRule()
    {
    }

    virtual ~TestMultipleProvidersRule()
    {
    }

    HRESULT TestMultipleProvidersRule::ParseBegin(
        _In_ appanalysis::IEtwEventRecord*
        )
    {
          
        appanalysis_impl::RuleTriggeredEventArgs::CreateParams params;
        wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> parseInfo;
        IFC_RETURN(CreateRuleTriggeredEventArgs(params, &parseInfo));

        FireNotification(parseInfo.Get());
     
        return S_OK;
    }

    HRESULT TestMultipleProvidersRule::ParseEnd(
        _In_ appanalysis::IEtwEventRecord*
        )
    {
        return S_OK;
    }

    HRESULT TestMultipleProvidersRule::PropertyChanged(
        _In_ appanalysis::IEtwEventRecord*
        )
    {
        return S_OK;
    }

};

BEGIN_PROVIDERS(TestMultipleProvidersRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(TestMultipleProvidersRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ParseXamlBegin_value, EventVersion_0, &TestMultipleProvidersRule::ParseBegin)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, PropertyChangedInfo_value, EventVersion_0, &TestMultipleProvidersRule::PropertyChanged)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ParseXamlEnd_value, EventVersion_0, &TestMultipleProvidersRule::ParseEnd)
END_CALLBACKS()

namespace AppAnalysis { namespace Test { 
////////////////////////////////////////////////////////////////////////////////
//
HRESULT TestMultipleProvidersRule_CreateInstance(
    _COM_Outptr_opt_ appanalysis::IEtwRule** ppInstance
    )
{
    // Initialize the output parameter to nullptr at the start
    if (ppInstance != nullptr) 
    {
        *ppInstance = nullptr;
    }    
    wrl::ComPtr<TestMultipleProvidersRule> rule;
    IFC_RETURN(TestMultipleProvidersRule::CreateInstance(
        L"Rule003", TEST_MULTIPLE_PROVIDER_TITLE,
        TEST_MULTIPLE_PROVIDER_IMPACT, TEST_MULTIPLE_PROVIDER_LINK_TITLE, L"http://testrule.com",
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterTestEvents(&watcher));

    IFC_RETURN(rule->CreateEtwRule(ppInstance, watcher.Get()));
    return S_OK;
}
} }