// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "HubAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <UIAutomationHelper.h>
#include "TestCleanupWrapper.h"

#include <ControlHelper.h>
#include <TreeHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Controls { namespace Hub {

        bool HubAutomationIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }
         
    bool HubAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

        bool HubAutomationIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // Test Cases

        //  Loads simple markup with Hub and verifies AutomationProperties are correct.
        void HubAutomationIntegrationTests::VerifyAutomationProperties()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Hub^ hub = nullptr;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestHubName";
            uiaInfo.m_AutomationID = L"TestHubId";

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Hub, Loaded);

            RunOnUIThread([&]()
            {
                hub = safe_cast<xaml_controls::Hub^> (xaml_markup::XamlReader::Load(
                    LR"(<Hub xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Header='Hub Header'>
                            <HubSection Header='Section 1'>
                                <DataTemplate>
                                    <Button Content='Section 1 Button' />
                                </DataTemplate>
                            </HubSection>
                            <HubSection Header='Section 2' IsHeaderInteractive='True'>
                                <DataTemplate>
                                    <TextBlock Text='Section 2 TextBlock' />
                                </DataTemplate>
                            </HubSection>
                        </Hub>)"));

                xaml_automation::AutomationProperties::SetName(hub, ref new Platform::String(uiaInfo.m_Name));
                xaml_automation::AutomationProperties::SetAutomationId(hub, ref new Platform::String(uiaInfo.m_AutomationID));

                loadedRegistration.Attach(hub, [loadedEvent](){ loadedEvent->Set(); });

                TestServices::WindowHelper->WindowContent = hub;
            });

            // Wait for all async activities to be done
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                Common::AutoVariant autoVar;
                Common::AutoBSTR autoBstr;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                LOG_OUTPUT(L"Verifying UIA Localaized ControlType property from Client side node for Hub.");
                spUIAutomationElement->get_CurrentLocalizedControlType(autoBstr.ReleaseAndGetAddressOf());
                AutoBSTR::VerifyAreEqual(L"group", autoBstr);

                spUIAutomationElement->get_CurrentClassName(autoBstr.ReleaseAndGetAddressOf());
                AutoBSTR::VerifyAreEqual(L"Hub", autoBstr);

                spUIAutomationElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                AutoBSTR::VerifyAreEqual(L"TestHubName", autoBstr);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void HubAutomationIntegrationTests::VerifyItemOrder()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Hub^ hub = nullptr;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestHubName";
            uiaInfo.m_AutomationID = L"TestHubId";

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Hub, Loaded);

            RunOnUIThread([&]()
            {
                hub = safe_cast<xaml_controls::Hub^> (xaml_markup::XamlReader::Load(
                    LR"(<Hub xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Header='Hub Header'>
                            <HubSection Header='Section 1'>
                                <DataTemplate>
                                    <Button Content='Section 1 Button' />
                                </DataTemplate>
                            </HubSection>
                            <HubSection Header='Section 2' IsHeaderInteractive='True'>
                                <DataTemplate>
                                    <Button Content='Section 2 Button' />
                                </DataTemplate>
                            </HubSection>
                        </Hub>)"));

                xaml_automation::AutomationProperties::SetName(hub, ref new Platform::String(uiaInfo.m_Name));
                xaml_automation::AutomationProperties::SetAutomationId(hub, ref new Platform::String(uiaInfo.m_AutomationID));

                loadedRegistration.Attach(hub, [loadedEvent](){ loadedEvent->Set(); });

                TestServices::WindowHelper->WindowContent = hub;
            });

            // Wait for all async activities to be done
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                Common::AutoVariant autoVar;
                Common::AutoBSTR autoBstr;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                wrl::ComPtr<IUIAutomation> spAutomation;
                spAutomationClientManager->GetAutomation(&spAutomation);

                wrl::ComPtr<IUIAutomation> spUIAutomation;
                spAutomationClientManager->GetAutomation(&spUIAutomation);

                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed to create true PropertyCondition.");

                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed to create TreeWalker.");

                wrl::ComPtr<IUIAutomationElement> spChildElement;
                wrl::ComPtr<IUIAutomationElement> spNextChildElement;
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spChildElement), L"Failed to get first child element.");
                VERIFY_IS_NOT_NULL(spChildElement);

                // The first child should be the header.
                spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                AutoBSTR::VerifyAreEqual(L"Hub Header", autoBstr);

                // Next should be the first section.
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spChildElement.Get(), &spNextChildElement), L"Failed to get next child element");
                spChildElement = spNextChildElement;
                spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                AutoBSTR::VerifyAreEqual(L"Section 1", autoBstr);

                {
                    wrl::ComPtr<IUIAutomationElement> spSectionChildElement;
                    wrl::ComPtr<IUIAutomationElement> spNextSectionChildElement;
                    LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spChildElement.Get(), &spSectionChildElement), L"Failed to get first section child element.");
                    VERIFY_IS_NOT_NULL(spSectionChildElement);

                    // The first section header should be the first child of the first section.
                    spSectionChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                    AutoBSTR::VerifyAreEqual(L"Section 1", autoBstr);

                    // The first section content should be the next child of the first section.
                    LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spSectionChildElement.Get(), &spNextSectionChildElement), L"Failed to get next child element");
                    spSectionChildElement = spNextSectionChildElement;
                    spSectionChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                    AutoBSTR::VerifyAreEqual(L"Section 1 Button", autoBstr);
                }

                // Next should be the second section.
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spChildElement.Get(), &spNextChildElement), L"Failed to get next child element");
                spChildElement = spNextChildElement;
                spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                AutoBSTR::VerifyAreEqual(L"Section 2", autoBstr);

                {
                    wrl::ComPtr<IUIAutomationElement> spSectionChildElement;
                    wrl::ComPtr<IUIAutomationElement> spNextSectionChildElement;
                    LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spChildElement.Get(), &spSectionChildElement), L"Failed to get first section child element.");
                    VERIFY_IS_NOT_NULL(spSectionChildElement);

                    // The second section header should be the first child of the second section.
                    spSectionChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                    AutoBSTR::VerifyAreEqual(L"Section 2", autoBstr);

                    // The "see more" button should be the next child of the second section.
                    LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spSectionChildElement.Get(), &spNextSectionChildElement), L"Failed to get next child element");
                    spSectionChildElement = spNextSectionChildElement;
                    spSectionChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                    AutoBSTR::VerifyAreEqual(L"See more", autoBstr);

                    // Next should be the second section content.
                    LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spSectionChildElement.Get(), &spNextSectionChildElement), L"Failed to get next child element");
                    spSectionChildElement = spNextSectionChildElement;
                    spSectionChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                    AutoBSTR::VerifyAreEqual(L"Section 2 Button", autoBstr);
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }

    } }
} } } }

