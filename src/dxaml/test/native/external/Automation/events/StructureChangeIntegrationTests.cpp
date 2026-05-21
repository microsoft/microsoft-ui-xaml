// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StructureChangeIntegrationTests.h"
#include <AutomationClient\AutomationEventHandler.h>

#include <XamlTailored.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Events {

    bool StructureChangeIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool StructureChangeIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool StructureChangeIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void StructureChangeIntegrationTests::VerifyStructureChangeEvents()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestStructureChangeEvents";
        uiaInfo.m_AutomationID = L"TestStructureChangeEvents";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            xaml_controls::Button^ childButton = ref new xaml_controls::Button();
            button = ref new xaml_controls::Button();
            button->Height = 150;
            button->Width = 150;
            childButton->Height = 100;
            childButton->Width = 100;
            childButton->Content = L"Hello";
            button->Content = childButton;

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();
        WEX::Logging::Log::Comment(L"Added Button to Visual Tree");
        
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<AutomationClient::AutomationStructureChangeHandler> spAutomationStructureChangeHandler;

            xaml_automation_peers::AutomationPeer^ buttonAP;
            RunOnUIThread([&]()
            {
                buttonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button);
            });
            
            auto structureChangeEvent = std::make_shared<Event>();
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationStructureChangeHandler.Attach(new AutomationClient::AutomationStructureChangeHandler(spAutomationClientManager, structureChangeEvent, TreeScope_Subtree));

            WEX::Logging::Log::Comment(L"Add Event Handler on UI thread and Raise Structure Change event for ChildAdded");
            spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildAdded);
            spAutomationStructureChangeHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                buttonAP->RaiseStructureChangedEvent(xaml_automation_peers::AutomationStructureChangeType::ChildAdded, nullptr);
            });
            spAutomationStructureChangeHandler->ConfirmAndUnregister();

            WEX::Logging::Log::Comment(L"Add Event Handler on UI thread and Raise Structure Change event for ChildRemoved");
            spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildRemoved);
            spAutomationStructureChangeHandler->AttachEventHandler();

            xaml_automation_peers::AutomationPeer^ childButtonAP;
            RunOnUIThread([&]()
            {
                childButtonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(static_cast<xaml_controls::Button^>(button->Content));
                button->Content = nullptr;
                buttonAP->RaiseStructureChangedEvent(xaml_automation_peers::AutomationStructureChangeType::ChildRemoved, childButtonAP);
            });
            
            spAutomationStructureChangeHandler->ConfirmAndUnregister();

            WEX::Logging::Log::Comment(L"Add Event Handler on UI thread and Raise Structure Change event for ChildernInvalidated");
            spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenInvalidated);
            spAutomationStructureChangeHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                buttonAP->RaiseStructureChangedEvent(xaml_automation_peers::AutomationStructureChangeType::ChildrenInvalidated, nullptr);
            });
            spAutomationStructureChangeHandler->ConfirmAndUnregister();

            WEX::Logging::Log::Comment(L"Add Event Handler on UI thread and Raise Structure Change event for ChildernBulkAdded");
            spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenBulkAdded);
            spAutomationStructureChangeHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                buttonAP->RaiseStructureChangedEvent(xaml_automation_peers::AutomationStructureChangeType::ChildrenBulkAdded, nullptr);
            });
            spAutomationStructureChangeHandler->ConfirmAndUnregister();

            WEX::Logging::Log::Comment(L"Add Event Handler on UI thread and Raise Structure Change event for ChildernBulkRemoved");
            spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenBulkRemoved);
            spAutomationStructureChangeHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                buttonAP->RaiseStructureChangedEvent(xaml_automation_peers::AutomationStructureChangeType::ChildrenBulkRemoved, nullptr);
            });
            spAutomationStructureChangeHandler->ConfirmAndUnregister();

            WEX::Logging::Log::Comment(L"Add Event Handler on UI thread and Raise Structure Change event for ChildernReordered");
            spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenReordered);
            spAutomationStructureChangeHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                buttonAP->RaiseStructureChangedEvent(xaml_automation_peers::AutomationStructureChangeType::ChildrenReordered, nullptr);
            });
            spAutomationStructureChangeHandler->ConfirmAndUnregister();
        });
    }

    void StructureChangeIntegrationTests::VerifyAutomaticStructureChangedEvents()
    {
        TestCleanupWrapper cleanup([]()
        {
            RunOnUIThread([&]()
            {
                TestServices::Utilities->ClearMockUIAClientsListening();
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::StackPanel^ stackPanel1;
        xaml_controls::StackPanel^ stackPanel2;
        wrl::ComPtr<AutomationClient::AutomationStructureChangeHandler> spAutomationStructureChangeHandler;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestStructureChangeEvents";
        uiaInfo.m_AutomationID = L"TestStructureChangeEvents";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        RunOnUIThread([&]()
        {
            xaml_automation_peers::AutomationPeer^ automationPeer;
            xaml_controls::Button^ uiAutomationElement = ref new xaml_controls::Button();
            xaml_controls::Button^ subTree1 = ref new xaml_controls::Button();
            xaml_controls::Button^ subTree2 = ref new xaml_controls::Button();
            xaml_controls::StackPanel^ body = ref new xaml_controls::StackPanel();
            stackPanel1 = ref new xaml_controls::StackPanel();
            stackPanel2 = ref new xaml_controls::StackPanel();

            xaml_automation::AutomationProperties::SetName(uiAutomationElement, ref new Platform::String(uiaInfo.m_Name));
            automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(uiAutomationElement);

            subTree1->Content = stackPanel1;
            subTree2->Content = stackPanel2;
            body->Orientation = xaml_controls::Orientation::Horizontal;
            body->Children->Append(subTree1);
            body->Children->Append(subTree2);
            uiAutomationElement->Content = body;
            TestServices::WindowHelper->WindowContent = uiAutomationElement;
            TestServices::Utilities->SetMockUIAClientsListening();
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto structureChangeEvent = std::make_shared<Event>();
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationStructureChangeHandler.Attach(new AutomationClient::AutomationStructureChangeHandler(spAutomationClientManager, structureChangeEvent, TreeScope_Subtree));
        });

        WEX::Logging::Log::Comment(L"Verifying ChildAdded when entering.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildAdded);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Children->Append(child);
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildRemoved when toggling Visibility.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildRemoved);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Visibility = xaml::Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildAdded when toggling Visibility.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildAdded);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildAdded when reparenting.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildAdded);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                auto child = stackPanel1->Children->GetAt(0);
                stackPanel1->Children->RemoveAt(0);
                stackPanel2->Children->Append(child);
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildRemoved when reparenting.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildRemoved);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                auto child = stackPanel2->Children->GetAt(0);
                stackPanel2->Children->RemoveAt(0);
                stackPanel1->Children->Append(child);
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildAdded does not fire when toggling Visibility twice during the same tick.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildAdded);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Visibility = xaml::Visibility::Collapsed;
                stackPanel1->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmNegativeAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildRemoved does not fire when toggling Visibility twice during the same tick.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildRemoved);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Visibility = xaml::Visibility::Collapsed;
                stackPanel1->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmNegativeAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildRemoved when leaving.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildRemoved);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                stackPanel1->Children->Clear();
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildAdded does not fire when entering and leaving during the same tick.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildAdded);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Children->Append(child);
                stackPanel1->Children->Clear();
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmNegativeAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildRemoved does not fire when entering and leaving during the same tick.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildRemoved);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                child->Text = "hello";
                stackPanel1->Children->Append(child);
                stackPanel1->Children->Clear();
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmNegativeAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildBulkAdded.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenBulkAdded);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                for (int i = 0; i < 25; i++)
                {
                    xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                    child->Text = "hello";
                    stackPanel1->Children->Append(child);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildBulkRemoved.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenBulkRemoved);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                stackPanel1->Children->Clear();
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }

        WEX::Logging::Log::Comment(L"Verifying ChildrenInvalidated.");
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->Init(StructureChangeType::StructureChangeType_ChildrenInvalidated);
                spAutomationStructureChangeHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                for (int i = 0; i < 15; i++)
                {
                    xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                    child->Text = "hello";
                    stackPanel1->Children->Append(child);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel1->Children->Clear();

                for (int i = 0; i < 15; i++)
                {
                    xaml_controls::TextBlock^ child = ref new xaml_controls::TextBlock();
                    child->Text = "hello";
                    stackPanel1->Children->Append(child);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationStructureChangeHandler->ConfirmAndUnregister();
            });
        }
    }

} } } } } }
