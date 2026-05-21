// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <LandmarksIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    struct TestData
    {
        bool fSupplyLT;
        xaml_automation_peers::AutomationLandmarkType lt;
        bool fSupplyLLT;
        Platform::String^ llt;
        bool fLandmarkOnContainer;
    };

    const std::map<xaml_automation_peers::AutomationLandmarkType, Platform::String^> make_landmarkTypeXAMLStringMapping()
    {
        std::map<xaml_automation_peers::AutomationLandmarkType, Platform::String^> m;
        m[xaml_automation_peers::AutomationLandmarkType::Custom]     = "Custom";
        m[xaml_automation_peers::AutomationLandmarkType::Form]       = "Form";
        m[xaml_automation_peers::AutomationLandmarkType::Main]       = "Main";
        m[xaml_automation_peers::AutomationLandmarkType::Navigation] = "Navigation";
        m[xaml_automation_peers::AutomationLandmarkType::Search]     = "Search";
        return m;
    };
    const std::map<xaml_automation_peers::AutomationLandmarkType, Platform::String^> c_landmarkTypeXAMLStringMapping = make_landmarkTypeXAMLStringMapping();

    const std::map<xaml_automation_peers::AutomationLandmarkType, LANDMARKTYPEID> make_landmarkTypeUIAIdMapping()
    {
        std::map<xaml_automation_peers::AutomationLandmarkType, LANDMARKTYPEID> m;
        m[xaml_automation_peers::AutomationLandmarkType::Custom]     = UIA_CustomLandmarkTypeId;
        m[xaml_automation_peers::AutomationLandmarkType::Form]       = UIA_FormLandmarkTypeId;
        m[xaml_automation_peers::AutomationLandmarkType::Main]       = UIA_MainLandmarkTypeId;
        m[xaml_automation_peers::AutomationLandmarkType::Navigation] = UIA_NavigationLandmarkTypeId;
        m[xaml_automation_peers::AutomationLandmarkType::Search]     = UIA_SearchLandmarkTypeId;
        return m;
    };
    const std::map<xaml_automation_peers::AutomationLandmarkType, LANDMARKTYPEID> c_landmarkTypeUIAIdMapping = make_landmarkTypeUIAIdMapping();

    const std::map<LANDMARKTYPEID, const wchar_t*> make_landmarkTypeUIAStringMapping()
    {
        std::map<LANDMARKTYPEID, const wchar_t*> m;
        m[UIA_CustomLandmarkTypeId]     = L"custom";
        m[UIA_FormLandmarkTypeId]       = L"form";
        m[UIA_MainLandmarkTypeId]       = L"main";
        m[UIA_NavigationLandmarkTypeId] = L"navigation";
        m[UIA_SearchLandmarkTypeId]     = L"search";
        return m;
    };
    const std::map<LANDMARKTYPEID, const wchar_t*> c_landmarkTypeUIAStringMapping = make_landmarkTypeUIAStringMapping();

    TestData g_TestData[] =
    {
        // TestCase                       SupplyLT  LandmarkType                                               SupplyLLT  LocalizedLandmarkType          fLandmarkOnContainer
        /* CustomBoth */                  { true,   xaml_automation_peers::AutomationLandmarkType::Custom,     true,      "CustomBoth",                  false },
        /* FormBoth */                    { true,   xaml_automation_peers::AutomationLandmarkType::Form,       true,      "FormBoth",                    false },
        /* MainBoth */                    { true,   xaml_automation_peers::AutomationLandmarkType::Main,       true,      "MainBoth",                    false },
        /* NavigationBoth */              { true,   xaml_automation_peers::AutomationLandmarkType::Navigation, true,      "NavigationBoth",              false },
        /* SearchBoth */                  { true,   xaml_automation_peers::AutomationLandmarkType::Search,     true,      "SearchBoth",                  false },
        /* CustomLTOnly */                { true,   xaml_automation_peers::AutomationLandmarkType::Custom,     false,     "",                            false },
        /* FormLTOnly */                  { true,   xaml_automation_peers::AutomationLandmarkType::Form,       false,     "",                            false },
        /* MainLTOnly */                  { true,   xaml_automation_peers::AutomationLandmarkType::Main,       false,     "",                            false },
        /* NavigationLTOnly */            { true,   xaml_automation_peers::AutomationLandmarkType::Navigation, false,     "",                            false },
        /* SearchLTOnly */                { true,   xaml_automation_peers::AutomationLandmarkType::Search,     false,     "",                            false },
        /* LLTOnly */                     { false,  (xaml_automation_peers::AutomationLandmarkType)0,          true,      "LLTOnly",                     false },
        /* NoLandmarks */                 { false,  (xaml_automation_peers::AutomationLandmarkType)0,          false,     "",                            false },
        /* LandmarksOnContainerBoth */    { true,   xaml_automation_peers::AutomationLandmarkType::Main,       true,      "LandmarksOnContainerBoth",    true },
        /* LandmarksOnContainerLTOnly */  { true,   xaml_automation_peers::AutomationLandmarkType::Main,       false,     "",                            true },
        /* LandmarksOnContainerLLTOnly */ { false,  (xaml_automation_peers::AutomationLandmarkType)0,          true,      "LandmarksOnContainerLLTOnly", true },
        /* LTOutOfRange */                { true,   (xaml_automation_peers::AutomationLandmarkType)99,        false,     "",                            false },
        /* EmptyLLT */                    { false,  (xaml_automation_peers::AutomationLandmarkType)0,          true,      "",                            false },
    };

    bool LandmarksIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool LandmarksIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool LandmarksIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void LandmarksIntegrationTests::VerifyCustomBoth()
    {
        _VerifyAutomationPropertiesMarkup(CustomBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(CustomBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(CustomBoth);
    }

    void LandmarksIntegrationTests::VerifyFormBoth()
    {
        _VerifyAutomationPropertiesMarkup(FormBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(FormBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(FormBoth);
    }

    void LandmarksIntegrationTests::VerifyMainBoth()
    {
        _VerifyAutomationPropertiesMarkup(MainBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(MainBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(MainBoth);
    }

    void LandmarksIntegrationTests::VerifyNavigationBoth()
    {
        _VerifyAutomationPropertiesMarkup(NavigationBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(NavigationBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(NavigationBoth);
    }

    void LandmarksIntegrationTests::VerifySearchBoth()
    {
        _VerifyAutomationPropertiesMarkup(SearchBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(SearchBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(SearchBoth);
    }

    void LandmarksIntegrationTests::VerifyCustomLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(CustomLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(CustomLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(CustomLTOnly);
    }

    void LandmarksIntegrationTests::VerifyFormLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(FormLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(FormLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(FormLTOnly);
    }

    void LandmarksIntegrationTests::VerifyMainLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(MainLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(MainLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(MainLTOnly);
    }

    void LandmarksIntegrationTests::VerifyNavigationLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(NavigationLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(NavigationLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(NavigationLTOnly);
    }

    void LandmarksIntegrationTests::VerifySearchLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(SearchLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(SearchLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(SearchLTOnly);
    }

    void LandmarksIntegrationTests::VerifyLLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(LLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(LLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(LLTOnly);
    }

    void LandmarksIntegrationTests::VerifyNoLandmarks()
    {
        _VerifyAutomationPropertiesMarkup(NoLandmarks);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(NoLandmarks);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(NoLandmarks);
    }

    void LandmarksIntegrationTests::VerifyLandmarksOnContainerBoth()
    {
        _VerifyAutomationPropertiesMarkup(LandmarksOnContainerBoth);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(LandmarksOnContainerBoth);
        LOG_OUTPUT(L"\r\n");

        // Disabling this check for now, as XAML crashes in FrameworkElementAutomationPeer::CreateInstanceWithOwner
        //_VerifyAutomationPeer(LandmarksOnContainerBoth);
    }

    void LandmarksIntegrationTests::VerifyLandmarksOnContainerLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(LandmarksOnContainerLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(LandmarksOnContainerLTOnly);
        LOG_OUTPUT(L"\r\n");

        // Disabling this check for now, as XAML crashes in FrameworkElementAutomationPeer::CreateInstanceWithOwner
        //_VerifyAutomationPeer(LandmarksOnContainerLTOnly);
    }

    void LandmarksIntegrationTests::VerifyLandmarksOnContainerLLTOnly()
    {
        _VerifyAutomationPropertiesMarkup(LandmarksOnContainerLLTOnly);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(LandmarksOnContainerLLTOnly);
        LOG_OUTPUT(L"\r\n");

        // Disabling this check for now, as XAML crashes in FrameworkElementAutomationPeer::CreateInstanceWithOwner
        //_VerifyAutomationPeer(LandmarksOnContainerLLTOnly);
    }

    void LandmarksIntegrationTests::VerifyLTOutOfRange()
    {
        _VerifyAutomationPropertiesCodebehind(LTOutOfRange);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(LTOutOfRange);
    }

    void LandmarksIntegrationTests::VerifyEmptyLLT()
    {
        _VerifyAutomationPropertiesMarkup(EmptyLLT);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPropertiesCodebehind(EmptyLLT);
        LOG_OUTPUT(L"\r\n");
        _VerifyAutomationPeer(EmptyLLT);
    }

    void LandmarksIntegrationTests::_VerifyAutomationPropertiesMarkup(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying landmarks added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = g_TestData[test].fLandmarkOnContainer ? L"LandmarkStackPanel" : L"LandmarkButton";
        uiaInfoTarget.m_AutomationID = g_TestData[test].fLandmarkOnContainer ? L"LandmarkStackPanel" : L"LandmarkButton";
        uiaInfoTarget.m_cType = g_TestData[test].fLandmarkOnContainer ? UIA_GroupControlTypeId : UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml;
            if (g_TestData[test].fLandmarkOnContainer)
            {
                xaml =
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' AutomationProperties.Name='LandmarkStackPanel' x:Name='LandmarkStackPanel'";

                if (g_TestData[test].fSupplyLT)
                {
                    xaml += " AutomationProperties.LandmarkType='";
                    if (test == LTOutOfRange)
                    {
                        xaml += "bad";
                    }
                    else
                    {
                        xaml += c_landmarkTypeXAMLStringMapping.at(g_TestData[test].lt);
                    }
                    xaml += "'";
                }

                if (g_TestData[test].fSupplyLLT)
                {
                    xaml += " AutomationProperties.LocalizedLandmarkType='" + g_TestData[test].llt + "'";
                }

                xaml += " >"
                    "    <Button />"
                    "</StackPanel>";
            }
            else
            {
                xaml =
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <Button AutomationProperties.Name='LandmarkButton' x:Name='LandmarkButton'";

                if (g_TestData[test].fSupplyLT)
                {
                    xaml += " AutomationProperties.LandmarkType='" + c_landmarkTypeXAMLStringMapping.at(g_TestData[test].lt) + "'";
                }

                if (g_TestData[test].fSupplyLLT)
                {
                    xaml += " AutomationProperties.LocalizedLandmarkType='" + g_TestData[test].llt + "'";
                }

                xaml += " />"
                    "</StackPanel>";
            }

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(test, uiaInfoTarget);
        });
    }

    void LandmarksIntegrationTests::_VerifyAutomationPropertiesCodebehind(TestCase test)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying landmarks added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = g_TestData[test].fLandmarkOnContainer ? L"LandmarkStackPanel" : L"LandmarkButton";
        uiaInfoTarget.m_AutomationID = g_TestData[test].fLandmarkOnContainer ? L"LandmarkStackPanel" : L"LandmarkButton";
        uiaInfoTarget.m_cType = g_TestData[test].fLandmarkOnContainer ? UIA_GroupControlTypeId : UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            if (g_TestData[test].fLandmarkOnContainer)
            {
                xaml_automation::AutomationProperties::SetName(stackPanel, ref new Platform::String(uiaInfoTarget.m_Name));
            }
            else
            {
                xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            }

            if (g_TestData[test].fSupplyLT)
            {
                if (g_TestData[test].fLandmarkOnContainer)
                {
                    xaml_automation::AutomationProperties::SetLandmarkType(stackPanel, g_TestData[test].lt);
                }
                else
                {
                    xaml_automation::AutomationProperties::SetLandmarkType(button, g_TestData[test].lt);
                }
            }

            if (g_TestData[test].fSupplyLLT)
            {
                if (g_TestData[test].fLandmarkOnContainer)
                {
                    xaml_automation::AutomationProperties::SetLocalizedLandmarkType(stackPanel, g_TestData[test].llt);
                }
                else
                {
                    xaml_automation::AutomationProperties::SetLocalizedLandmarkType(button, g_TestData[test].llt);
                }
            }

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(test, uiaInfoTarget);
        });
    }

    void LandmarksIntegrationTests::_VerifyAutomationPeer(TestCase test)
    {

        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying landmarks added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = g_TestData[test].fLandmarkOnContainer ? L"LandmarkStackPanel" : L"LandmarkButton";
        uiaInfoTarget.m_AutomationID = g_TestData[test].fLandmarkOnContainer ? L"LandmarkStackPanel" : L"LandmarkButton";
        uiaInfoTarget.m_cType = g_TestData[test].fLandmarkOnContainer ? UIA_PaneControlTypeId : UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = g_TestData[test].fLandmarkOnContainer ? ref new CustomStackPanel(test) : ref new xaml_controls::StackPanel;
            button = g_TestData[test].fLandmarkOnContainer ? ref new xaml_controls::Button : ref new CustomButton(test);

            if (g_TestData[test].fLandmarkOnContainer)
            {
                xaml_automation::AutomationProperties::SetName(stackPanel, ref new Platform::String(uiaInfoTarget.m_Name));
            }
            else
            {
                xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            }

            stackPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            _VerifyClient(test, uiaInfoTarget);
        });
    }

    void LandmarksIntegrationTests::_VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget)
    {
        auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
        spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
        WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

        wrl::ComPtr<IUIAutomationElement5> spUIAutomationElement5Target;
        VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement5), &spUIAutomationElement5Target));

        LANDMARKTYPEID landmarkType;
        AutoBSTR localizedLandmarkType;
        CONTROLTYPEID controlType;
        VERIFY_SUCCEEDED(spUIAutomationElement5Target->get_CurrentLandmarkType(&landmarkType));
        VERIFY_SUCCEEDED(spUIAutomationElement5Target->get_CurrentLocalizedLandmarkType(localizedLandmarkType.ReleaseAndGetAddressOf()));
        VERIFY_SUCCEEDED(spUIAutomationElement5Target->get_CurrentControlType(&controlType));

        if (g_TestData[test].fSupplyLT && test != LTOutOfRange)
        {
            VERIFY_ARE_EQUAL(landmarkType, c_landmarkTypeUIAIdMapping.at(g_TestData[test].lt));
        }
        else if (test == LLTOnly || test == LandmarksOnContainerLLTOnly)
        {
            VERIFY_ARE_EQUAL(landmarkType, UIA_CustomLandmarkTypeId);
        }
        else
        {
            VERIFY_ARE_EQUAL(landmarkType, 0);
        }

        if (g_TestData[test].fSupplyLLT)
        {
            LOG_OUTPUT(L"localizedLandmarkType: %ws", localizedLandmarkType.Get());
            VERIFY_ARE_EQUAL(0, wcscmp(localizedLandmarkType, g_TestData[test].llt->Data()));
        }
        else if (landmarkType != 0)
        {
            LOG_OUTPUT(L"localizedLandmarkType: %ws", localizedLandmarkType.Get());
            VERIFY_ARE_EQUAL(0, wcscmp(localizedLandmarkType, c_landmarkTypeUIAStringMapping.at(landmarkType)));
        }
        else
        {
            LOG_OUTPUT(L"localizedLandmarkType: %ws", localizedLandmarkType.Get());
            VERIFY_ARE_EQUAL(0, wcscmp(localizedLandmarkType, L""));
        }

        VERIFY_ARE_EQUAL(controlType, uiaInfoTarget.m_cType);
    }

    /*static*/ xaml_automation_peers::AutomationLandmarkType LandmarksIntegrationTests::_GetLandmarkTypeCore(TestCase test)
    {
        if (g_TestData[test].fSupplyLT)
        {
            return g_TestData[test].lt;
        }
        return xaml_automation_peers::AutomationLandmarkType::None;
    }

    /*static*/ Platform::String^ LandmarksIntegrationTests::_GetLocalizedLandmarkTypeCore(TestCase test)
    {
        if (g_TestData[test].fSupplyLLT)
        {
            return g_TestData[test].llt;
        }
        return nullptr;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
