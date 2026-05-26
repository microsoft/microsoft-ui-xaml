// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <HeadingLevelIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct HeadingLevelTestData
    {
        bool supplyHeadingLevel;
        Platform::String^ headingLevel;
        xaml_automation_peers::AutomationHeadingLevel typedHeadingLevel;
    };

    HeadingLevelTestData g_TestData[] =
    {
        // TestCase                             SupplyFD    headingLevel    typedHeadingLevel
        /* HeadingLevelNone */                  { true,     "0",            xaml_automation_peers::AutomationHeadingLevel::None},
        /* HeadingLevel_1 */                    { true,     "1",            xaml_automation_peers::AutomationHeadingLevel::Level1},
        /* HeadingLevel_2 */                    { true,     "2",            xaml_automation_peers::AutomationHeadingLevel::Level2 },
        /* HeadingLevel_3 */                    { true,     "3",            xaml_automation_peers::AutomationHeadingLevel::Level3 },
        /* HeadingLevel_4 */                    { true,     "4",            xaml_automation_peers::AutomationHeadingLevel::Level4 },
        /* HeadingLevel_5 */                    { true,     "5",            xaml_automation_peers::AutomationHeadingLevel::Level5 },
        /* HeadingLevel_6 */                    { true,     "6",            xaml_automation_peers::AutomationHeadingLevel::Level6 },
        /* HeadingLevel_7 */                    { true,     "7",            xaml_automation_peers::AutomationHeadingLevel::Level7 },
        /* HeadingLevel_8 */                    { true,     "8",            xaml_automation_peers::AutomationHeadingLevel::Level8 },
        /* HeadingLevel_9 */                    { true,     "9",            xaml_automation_peers::AutomationHeadingLevel::Level9 },
        /* HeadingLevelNotSet */                { false,    nullptr,        xaml_automation_peers::AutomationHeadingLevel::None},
    };

    bool HeadingLevelIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool HeadingLevelIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool HeadingLevelIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void HeadingLevelIntegrationTests::VerifyHeadingLevelNone()
    {
        ExecuteScenarios(HeadingLevelNone);
    }

    void HeadingLevelIntegrationTests::VerifyAllHeadingLevels()
    {
        for (int i = 0; i < 9; i++)
        {
            // HeadingLevel_1...HeadingLevel_9.
            TestCase tc = static_cast<TestCase>(static_cast<int>(HeadingLevel_1)+i);
            ExecuteScenarios(tc);
        }
    }

    void HeadingLevelIntegrationTests::VerifyUnsetHeadingLevel()
    {
        ExecuteScenarios(HeadingLevelNotSet);
    }

    void HeadingLevelIntegrationTests::ExecuteScenarios(TestCase tc)
    {
        VerifyAutomationPropertiesMarkup(tc);
        LOG_OUTPUT(L"\r\n");
        VerifyAutomationPropertiesCodebehind(tc);
        LOG_OUTPUT(L"\r\n");
        VerifyAutomationPeer(tc);
    }

    void HeadingLevelIntegrationTests::VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying HeadingLevel added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"HeadingLevelButton";
        uiaInfoTarget.m_AutomationID = L"HeadingLevelButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='HeadingLevelButton' x:Name='HeadingLevelButton'";

            if (g_TestData[test].supplyHeadingLevel)
            {
                xaml += " AutomationProperties.HeadingLevel='" + g_TestData[test].headingLevel + "'";
            }

            xaml += " />"
                "</StackPanel>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifyClient(test, uiaInfoTarget);
        });
    }

    void HeadingLevelIntegrationTests::VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying HeadingLevel added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"HeadingLevelButton";
        uiaInfoTarget.m_AutomationID = L"HeadingLevelButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            if (g_TestData[test].supplyHeadingLevel)
            {
                xaml_automation::AutomationProperties::SetHeadingLevel(button, g_TestData[test].typedHeadingLevel);
            }

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifyClient(test, uiaInfoTarget);
        });
    }

    void HeadingLevelIntegrationTests::VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying HeadingLevel added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"HeadingLevelButton";
        uiaInfoTarget.m_AutomationID = L"HeadingLevelButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel =  ref new xaml_controls::StackPanel;
            button = ref new CustomButton(test);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifyClient(test, uiaInfoTarget);
        });
    }

    void HeadingLevelIntegrationTests::VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto automationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> uiAutomationElementTarget;
        automationClientManagerTarget->GetCurrentUIAutomationElement(&uiAutomationElementTarget);
        WEX::Common::Throw::IfNull(uiAutomationElementTarget.Get());

        wrl::ComPtr<IUIAutomationElement8> uiAutomationElement8Target;
        VERIFY_SUCCEEDED(uiAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement8), &uiAutomationElement8Target));

        HEADINGLEVELID headingLevel = 0;
        VERIFY_SUCCEEDED(uiAutomationElement8Target->get_CurrentHeadingLevel(&headingLevel));

        if (g_TestData[test].supplyHeadingLevel)
        {
            LOG_OUTPUT(L"HeadingLevel: %d", headingLevel);
            VERIFY_ARE_EQUAL(headingLevel, GetHeadingLevelIDFromAutomationHeading(g_TestData[test].typedHeadingLevel));
        }
        else
        {
            LOG_OUTPUT(L"HeadingLevel: %d", headingLevel);
            VERIFY_ARE_EQUAL(headingLevel, GetHeadingLevelIDFromAutomationHeading(g_TestData[test].typedHeadingLevel));
        }
    }

    /*static*/ xaml_automation_peers::AutomationHeadingLevel HeadingLevelIntegrationTests::GetHeadingLevelCoreImpl(TestCase test)
    {
        if (g_TestData[test].supplyHeadingLevel)
        {
            return g_TestData[test].typedHeadingLevel;
        }

        return xaml_automation_peers::AutomationHeadingLevel::None;
    }

    /* static */ HEADINGLEVELID HeadingLevelIntegrationTests::GetHeadingLevelIDFromAutomationHeading(xaml_automation_peers::AutomationHeadingLevel level)
    {
        switch (level)
        {
        case xaml_automation_peers::AutomationHeadingLevel::None:
            return HeadingLevel_None;
        case xaml_automation_peers::AutomationHeadingLevel::Level1:
            return HeadingLevel1;
        case xaml_automation_peers::AutomationHeadingLevel::Level2:
            return HeadingLevel2;
        case xaml_automation_peers::AutomationHeadingLevel::Level3:
            return HeadingLevel3;
        case xaml_automation_peers::AutomationHeadingLevel::Level4:
            return HeadingLevel4;
        case xaml_automation_peers::AutomationHeadingLevel::Level5:
            return HeadingLevel5;
        case xaml_automation_peers::AutomationHeadingLevel::Level6:
            return HeadingLevel6;
        case xaml_automation_peers::AutomationHeadingLevel::Level7:
            return HeadingLevel7;
        case xaml_automation_peers::AutomationHeadingLevel::Level8:
            return HeadingLevel8;
        case xaml_automation_peers::AutomationHeadingLevel::Level9:
            return HeadingLevel9;
        }

        VERIFY_IS_TRUE(false, L"Unexpected heading level returned from provider");
        return  HeadingLevel_None;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
