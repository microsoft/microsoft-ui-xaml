// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <RegressionTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

#undef GetClassName // Conflicts with automation peer method of the same name

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Tests::Native::External::Automation::Regression;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Regression {

    bool RegressionTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        RunOnUIThread([]()
        {
            auto runtime = GetModuleHandleW(L"Microsoft.UI.Xaml.dll");
            VERIFY_IS_NOT_NULL(runtime);
            std::wstring runtimePath(MAX_PATH, L'\0');
            for (;;)
            {
                auto length = GetModuleFileNameW(runtime, runtimePath.data(), static_cast<DWORD>(runtimePath.size()));
                VERIFY_IS_TRUE(length > 0);
                if (length < runtimePath.size())
                {
                    break;
                }
                runtimePath.resize(runtimePath.size() * 2);
            }
            LOG_OUTPUT(L"Runtime under test: %s", runtimePath.c_str());
        });
        return true;
    }

    bool RegressionTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RegressionTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void RegressionTests::ValidateEmptyItemInContainer()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Platform::String^>^ itemsList;
        xaml_controls::ListView^ listView;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestList";
        uiaInfo.m_AutomationID = L"TestList";
        uiaInfo.m_cType = UIA_ListControlTypeId;

        RunOnUIThread([&]()
        {
            itemsList = ref new Platform::Collections::Vector<Platform::String^>();
            itemsList->Append(nullptr);
            listView = ref new xaml_controls::ListView();
            xaml_automation::AutomationProperties::SetName(listView, ref new Platform::String(uiaInfo.m_Name));
            listView->ItemsSource = itemsList;
            TestServices::WindowHelper->WindowContent = listView;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
        });

        RunOnUIThread([&]()
        {
            xaml_automation_peers::AutomationPeer^ listviewAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(listView);

            listView->SelectedIndex = 0;

            VERIFY_IS_NULL(listView->SelectedItem);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(listView);
    }

    void RegressionTests::ValidateAutomationPeerSetup()
    {
        TestCleanupWrapper cleanup;
        xaml_automation_peers::ButtonAutomationPeer^ buttonAP = nullptr;
        RunOnUIThread([&]()
        {
            xaml_controls::Button^ button;
            button = ref new xaml_controls::Button();

            // This shouldn't be utilized to obtain AP directly instead use CreatePeerFromElement
            // As a lot of people already doing this, we want to add a regression test for a related
            // crash fix.
            buttonAP = ref new xaml_automation_peers::ButtonAutomationPeer(button);
        });

        RunOnUIThread([&]()
        {
            try
            {
                buttonAP->IsControlElement();
                VERIFY_IS_TRUE(false, L"UIA_E_ELEMENTNOTAVAILABLE exception was expected.");
            }
            catch (Platform::Exception^ e)
            {
                VERIFY_ARE_EQUAL(e->HResult, int(UIA_E_ELEMENTNOTAVAILABLE));
            }
        });
    }

    void RegressionTests::ValidateListViewDuplicateItemPeers()
    {
        ValidateDuplicateItemPeers(ItemPeerControl::ListView, false, false);
        ValidateDuplicateItemPeers(ItemPeerControl::ListView, true, false);
    }

    void RegressionTests::ValidateGridViewDuplicateItemPeers()
    {
        ValidateDuplicateItemPeers(ItemPeerControl::GridView, false, false);
        ValidateDuplicateItemPeers(ItemPeerControl::GridView, true, false);
    }

    void RegressionTests::ValidateItemsControlDuplicateItemPeers()
    {
        ValidateDuplicateItemPeers(ItemPeerControl::ItemsControl, true, false);
    }

    void RegressionTests::ValidateGroupedDuplicateItemPeers()
    {
        ValidateDuplicateItemPeers(ItemPeerControl::ListView, false, true);
        ValidateDuplicateItemPeers(ItemPeerControl::ListView, true, true);
        ValidateDuplicateItemPeers(ItemPeerControl::GridView, false, true);
        ValidateDuplicateItemPeers(ItemPeerControl::GridView, true, true);
    }

    bool RegressionTests::AreSamePeer(Platform::Object^ first, Platform::Object^ second)
    {
        if (!first || !second)
        {
            return first == second;
        }

        // Navigate returns IInspectable; compare canonical identities, not projected interface addresses.
        wrl::ComPtr<IUnknown> firstIdentity;
        wrl::ComPtr<IUnknown> secondIdentity;
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(first)->QueryInterface(IID_PPV_ARGS(&firstIdentity)));
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(second)->QueryInterface(IID_PPV_ARGS(&secondIdentity)));
        return firstIdentity.Get() == secondIdentity.Get();
    }

    void RegressionTests::ValidateItemPeerChildren(xaml_automation_peers::AutomationPeer^ peer, bool validateNavigation)
    {
        auto children = peer->GetChildren();
        VERIFY_IS_NOT_NULL(children);
        if (!children)
        {
            return;
        }

        for (unsigned int i = 0; i < children->Size; ++i)
        {
            auto child = children->GetAt(i);
            VERIFY_IS_NOT_NULL(child);
            for (unsigned int j = 0; j < i; ++j)
            {
                VERIFY_IS_FALSE(AreSamePeer(child, children->GetAt(j)), L"A logical peer must appear only once among siblings.");
            }
            if (child && validateNavigation)
            {
                auto previous = i == 0 ? nullptr : children->GetAt(i - 1);
                auto next = i + 1 == children->Size ? nullptr : children->GetAt(i + 1);
                VERIFY_IS_TRUE(AreSamePeer(child->GetParent(), peer));
                VERIFY_IS_TRUE(AreSamePeer(child->Navigate(xaml_automation_peers::AutomationNavigationDirection::PreviousSibling), previous));
                VERIFY_IS_TRUE(AreSamePeer(child->Navigate(xaml_automation_peers::AutomationNavigationDirection::NextSibling), next));
            }
        }
        if (validateNavigation && children->Size > 0)
        {
            VERIFY_IS_TRUE(AreSamePeer(peer->Navigate(xaml_automation_peers::AutomationNavigationDirection::FirstChild), children->GetAt(0)));
            VERIFY_IS_TRUE(AreSamePeer(peer->Navigate(xaml_automation_peers::AutomationNavigationDirection::LastChild), children->GetAt(children->Size - 1)));
        }
    }

    void RegressionTests::ValidateDuplicateItemPeers(ItemPeerControl controlType, bool useStackPanel, bool grouped)
    {
        TestCleanupWrapper cleanup;
        for (int dataSet = 0; dataSet < 3; ++dataSet)
        {
            xaml_controls::ItemsControl^ control = nullptr;
            xaml::Data::CollectionViewSource^ view = nullptr;
            Platform::Collections::Vector<Platform::Object^>^ items = nullptr;
            xaml_automation_peers::AutomationPeer^ peer = nullptr;
            xaml_automation_peers::AutomationPeer^ representative = nullptr;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Item peer case: control=%d, stackPanel=%d, grouped=%d, dataSet=%d", static_cast<int>(controlType), useStackPanel, grouped, dataSet);
                switch (controlType)
                {
                case ItemPeerControl::ListView:
                    control = ref new xaml_controls::ListView();
                    break;
                case ItemPeerControl::GridView:
                    control = ref new xaml_controls::GridView();
                    break;
                case ItemPeerControl::ItemsControl:
                    control = ref new RegressionItemsControl();
                    break;
                }
                control->Width = 500;
                control->Height = 600;
                if (useStackPanel)
                {
                    control->ItemsPanel = safe_cast<xaml_controls::ItemsPanelTemplate^>(xaml::Markup::XamlReader::Load(
                        L"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><StackPanel /></ItemsPanelTemplate>"));
                }

                auto createItems = [&](const wchar_t* prefix)
                {
                    auto result = ref new Platform::Collections::Vector<Platform::Object^>();
                    const wchar_t* interleaved[] = { L"repeat", L"middle", L"repeat", L"last", L"middle", L"tail" };
                    for (int i = 0; i < 6; ++i)
                    {
                        std::wstring name(prefix);
                        name += dataSet == 0 ? std::to_wstring(i) : dataSet == 1 ? L"duplicate" : interleaved[i];
                        result->Append(ref new Platform::String(name.c_str()));
                    }
                    return result;
                };
                items = createItems(L"");
                if (grouped)
                {
                    auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
                    groups->Append(createItems(L"first-"));
                    groups->Append(createItems(L"second-"));
                    view = ref new xaml::Data::CollectionViewSource();
                    view->IsSourceGrouped = true;
                    view->Source = groups;
                    control->ItemsSource = view->View;
                    auto style = ref new xaml_controls::GroupStyle();
                    style->HeaderTemplate = safe_cast<xaml::DataTemplate^>(xaml::Markup::XamlReader::Load(
                        L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='Group header' /></DataTemplate>"));
                    control->GroupStyle->Append(style);
                }
                else
                {
                    control->ItemsSource = items;
                }
                auto root = ref new xaml_controls::Grid();
                root->Children->Append(control);
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(control);
                VERIFY_IS_NOT_NULL(peer);
                if (!peer)
                {
                    return;
                }
                if (dataSet == 1)
                {
                    auto provider = dynamic_cast<xaml_automation::Provider::IItemContainerProvider^>(
                        peer->GetPattern(xaml_automation_peers::PatternInterface::ItemContainer));
                    VERIFY_IS_NOT_NULL(provider);
                    if (provider)
                    {
                        VERIFY_IS_NOT_NULL(provider->FindItemByProperty(nullptr, nullptr, nullptr));
                    }
                }

                ValidateItemPeerChildren(peer, false);
                // Cache reuse must not reintroduce a repeated peer on subsequent navigation queries.
                for (int pass = 0; pass < 2; ++pass)
                {
                    ValidateItemPeerChildren(peer, true);
                }
                auto children = peer->GetChildren();
                const unsigned int expectedItemCount = dataSet == 0 ? 6u : dataSet == 1 ? 1u : 4u;
                if (grouped)
                {
                    VERIFY_ARE_EQUAL(2u, children->Size);
                    for (auto child : children)
                    {
                        VERIFY_ARE_EQUAL(xaml_automation_peers::AutomationControlType::Group, child->GetAutomationControlType());
                        ValidateItemPeerChildren(child, false);
                        ValidateItemPeerChildren(child, true);
                        auto groupChildren = child->GetChildren();
                        VERIFY_ARE_EQUAL(expectedItemCount + 1, groupChildren->Size);
                        VERIFY_ARE_EQUAL(ref new Platform::String(L"Group header"), groupChildren->GetAt(0)->GetName());
                    }
                }
                else
                {
                    VERIFY_ARE_EQUAL(expectedItemCount, children->Size);
                    if (dataSet == 2 && children->Size == 4)
                    {
                        const wchar_t* expected[] = { L"repeat", L"middle", L"last", L"tail" };
                        for (unsigned int i = 0; i < 4; ++i)
                        {
                            VERIFY_ARE_EQUAL(ref new Platform::String(expected[i]), children->GetAt(i)->GetName());
                        }
                    }
                    if (children->Size > 0)
                    {
                        representative = children->GetAt(0);
                    }
                    auto list = dynamic_cast<xaml_controls::ListViewBase^>(control);
                    if (list)
                    {
                        list->SelectedIndex = 0;
                        auto selection = dynamic_cast<xaml_automation::Provider::ISelectionProvider^>(
                            peer->GetPattern(xaml_automation_peers::PatternInterface::Selection));
                        VERIFY_IS_NOT_NULL(selection);
                        if (selection)
                        {
                            auto selected = selection->GetSelection();
                            VERIFY_IS_NOT_NULL(selected);
                            if (selected)
                            {
                                VERIFY_ARE_EQUAL(1u, selected->Length);
                            }
                        }
                    }
                    items->RemoveAt(0);
                    items->Append(L"added tail");
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                if (!grouped && peer)
                {
                    ValidateItemPeerChildren(peer, false);
                    ValidateItemPeerChildren(peer, true);
                    if (dataSet != 0 && representative)
                    {
                        unsigned int index = 0;
                        VERIFY_IS_TRUE(peer->GetChildren()->IndexOf(representative, &index), L"A surviving logical peer retains its identity.");
                    }
                }
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void RegressionTests::VerifyAutomationFocusAfterSuspend()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Button^ button2 = nullptr;

        auto focusChangeEvent = std::make_shared<Event>();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestButton1Name";
        uiaInfo.m_AutomationID = L"TestButton1Id";
        uiaInfo.m_ItemStatus = L"TestButton1";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfo2;
        uiaInfo2.m_Name = L"TestButton2Name";
        uiaInfo2.m_AutomationID = L"TestButton2Id";
        uiaInfo2.m_ItemStatus = L"TestButton2";
        uiaInfo2.m_cType = UIA_ButtonControlTypeId;

        xaml_controls::Page^ page1;

        wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> spAutomationFocusChangeHandler;

        auto pageLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto pageLoadedEvent = std::make_shared<Event>();

        LOG_OUTPUT(L"Creating page and adding two buttons with automation properties.");
        RunOnUIThread([&]()
        {
            page1 = TestServices::WindowHelper->SetupSimulatedAppPage();

            button1 = ref new xaml_controls::Button();
            button1->Content = "Button1";
            xaml_automation::AutomationProperties::SetName(button1, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(button1, ref new Platform::String(uiaInfo.m_AutomationID));

            button2 = ref new xaml_controls::Button();
            button2->Content = "Button2";
            xaml_automation::AutomationProperties::SetName(button2, ref new Platform::String(uiaInfo2.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(button2, ref new Platform::String(uiaInfo2.m_AutomationID));

            xaml_controls::StackPanel^ stackPanel = ref new xaml_controls::StackPanel();

            stackPanel->Children->Append(button1);
            stackPanel->Children->Append(button2);

            page1->Content = stackPanel;

            pageLoadedRegistration.Attach(page1, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                pageLoadedEvent->Set();
            }));
        });
        pageLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Reading automation properties for button1 to attach AutomationFocusChangeHandler.");
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
        wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
        wrl::ComPtr<IUIAutomation> spUIAutomation;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

        LOG_OUTPUT(L"Creating and attaching AutomationFocusChangeHandler.");
        // Attach an AutomationFocusChangeHandler to listen for focus changes while navigating between pages.
        RunOnUIThread([&]()
        {
            spAutomationFocusChangeHandler.Attach(new AutomationClient::AutomationFocusChangeHandler(spAutomationClientManager, focusChangeEvent, TreeScope_Subtree));
            spAutomationFocusChangeHandler->Init();
            spAutomationFocusChangeHandler->AttachEventHandler();

            // We move focus to another element first to make sure button1 is not already focused.
            LOG_OUTPUT(L"Moving focus to button2.");
            button2->Focus(xaml::FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto removeAutomationFocusChangeEventHandler = wil::scope_exit([&] {
            RunOnUIThread([&]()
            {
                spAutomationFocusChangeHandler->RemoveEventHandler();
            });
        });

        RunOnUIThread([&]()
        {
            button1->Focus(xaml::FocusState::Pointer);
        });

        LOG_OUTPUT(L"Validating automation focus moved to button.");
        // Validate AutomationFocusChangeHandler follows focus for normal cases.
        ValidateAutomationFocus(uiaInfo, spAutomationFocusChangeHandler, true);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to button2.");
            button2->Focus(xaml::FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate AutomationFocusChangeHandler follows focus for different controls in same page.
        ValidateAutomationFocus(uiaInfo2, spAutomationFocusChangeHandler, true);

        // Simulate Suspend and Resume cycle.
        TestServices::WindowHelper->ForceDisconnectRootOnSuspend(true);
        LOG_OUTPUT(L"Triggering Suspension.");
        TestServices::WindowHelper->TriggerSuspend(false /* isTriggeredByResourceTimer */, true /* allowOfferResources */);
        LOG_OUTPUT(L"Triggering Resume.");
        TestServices::WindowHelper->TriggerResume();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validating automation focus is still with button.");
        // Validate focus is still on last element after suspension.
        ValidateAutomationFocus(uiaInfo2, spAutomationFocusChangeHandler, false);

        xaml_controls::Button^ button3 = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo3;
        uiaInfo3.m_Name = L"TestButton3Name";
        uiaInfo3.m_AutomationID = L"TestButton3Id";
        uiaInfo3.m_ItemStatus = L"TestButton3";
        uiaInfo3.m_cType = UIA_ButtonControlTypeId;

        xaml_controls::Page^ page2;

        LOG_OUTPUT(L"Navigating to new page and adding new button with automation properties.");
        RunOnUIThread([&]()
        {
            page2 = TestServices::WindowHelper->SetupSimulatedAppPage();
            button3 = ref new xaml_controls::Button();
            button3->Content = "Button3";
            xaml_automation::AutomationProperties::SetName(button3, ref new Platform::String(uiaInfo3.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(button3, ref new Platform::String(uiaInfo3.m_AutomationID));
            page2->Content = button3;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate automation focus moved to element in new page.");
        // Validate focus changes to element in new page after a suspension.
        ValidateAutomationFocus(uiaInfo3, spAutomationFocusChangeHandler, true);
    }

    void RegressionTests::ValidateAutomationPeerHasFocusInAutomationFocusChangedEvent()
    {
        // We want to verify the case where a control has a fully custom AutomationPeer (i.e. not derrived from FrameworkElementAutomationPeer).
        // When that AutomationPeer raises its AutomationFocusChanged in response to a call to SetFocus, it should report that it has focus
        // when the event is raised.
        // Regression coverage for bug:
        // - Narrator not announcing autosuggestions in universal mail app
        TestCleanupWrapper cleanup;

         // Leak: CUIAWrapper reports as being leaked
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        ControlWithCustomAutomationPeer^ customControl;
        auto focusChangeEvent = std::make_shared<Event>();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CustomControlName";
        uiaInfo.m_AutomationID = L"CustomControlId";
        wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> spAutomationFocusChangeHandler;
        bool eventFired = false;
        bool hasKeyboardFocus = false;

        RunOnUIThread([&]()
        {
            customControl = ref new ControlWithCustomAutomationPeer();
            xaml_automation::AutomationProperties::SetName(customControl, ref new Platform::String(uiaInfo.m_Name));
            xaml_automation::AutomationProperties::SetAutomationId(customControl, ref new Platform::String(uiaInfo.m_AutomationID));

            auto rootPanel = ref new xaml_controls::StackPanel();
            rootPanel->Children->Append(customControl);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationFocusChangeHandler.Attach(new AutomationClient::AutomationFocusChangeHandler(spAutomationClientManager, focusChangeEvent, TreeScope_Subtree));
            spAutomationFocusChangeHandler->Init();
            spAutomationFocusChangeHandler->AttachEventHandler();
        });

        xaml_automation_peers::AutomationPeer^ ap; // need the peer to live longer than the RunOnUIThread lambda
        RunOnUIThread([&]()
        {
            ap = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(customControl);

            spAutomationFocusChangeHandler->focusChangedCallback = [ap, &eventFired, &hasKeyboardFocus]()
            {
                eventFired = true;
                hasKeyboardFocus = ap->HasKeyboardFocus();
                LOG_OUTPUT(L"AutomationFocusChanged event raised. AutomationPeer HasKeyboardFocus = %d", hasKeyboardFocus);
            };

            ap->SetFocus();
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            if (!eventFired)
            {
                spAutomationFocusChangeHandler->Confirm();
            }

            VERIFY_IS_TRUE(hasKeyboardFocus, L"The AutomationPeer should have focus when its focus changed event is raised.");
        });

        RunOnUIThread([&]()
        {
            // There can be an issue with the order of event as we shut down.  As we shut down the test, we end up firing another
            // another focus event and if that gets processed after we have destructed the tree (and the corresponding automation
            // peer), the handler will attempt to access the now destructed peer.  So remove the event handler before we exit.
            spAutomationFocusChangeHandler->focusChangedCallback = nullptr;
        });
    }

    void RegressionTests::ValidateAutomationFocus(Automation::AutomationClient::UIAElementInfo uiaInfo, wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> spAutomationFocusChangeHandler, bool waitingFocusChange)
    {
        // Gets automation properties from the UIAElemenInfo and the AutomationFocusChangeHandler to validate focus is on the provided UIAElement
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spUIAutomation;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

        if (waitingFocusChange)
        {
            spAutomationFocusChangeHandler->Confirm();
        }

        Common::AutoVariant autoVar;
        Platform::String^ buttonName = nullptr;
        Platform::String^ focusedElementName = nullptr;

        spUIAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
        buttonName = UIAutomationHelper::StringFromVariant(autoVar.Storage());

        spAutomationFocusChangeHandler->GetLastFocusedElement()->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
        focusedElementName = UIAutomationHelper::StringFromVariant(autoVar.Storage());

        VERIFY_ARE_EQUAL(buttonName, focusedElementName);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::Regression
