// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "LoopingSelectorAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include "TestCleanupWrapper.h"

#include <ControlHelper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace LoopingSelector {

    WCHAR LoopingSelectorAutomationIntegrationTests::s_automationName[] = L"TestLoopingSelectorName";
    WCHAR LoopingSelectorAutomationIntegrationTests::s_automationId[] = L"TestLoopingSelectorId";
    WCHAR LoopingSelectorAutomationIntegrationTests::s_automationItemStatus[] = L"TestLoopingSelector";
    CONTROLTYPEID LoopingSelectorAutomationIntegrationTests::s_automationTypeId = UIA_ListControlTypeId;

    bool LoopingSelectorAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool LoopingSelectorAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool LoopingSelectorAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    // Test Cases

    //  Loads simple markup with LoopingSelector and verifies AutomationProperties are correct.
    void LoopingSelectorAutomationIntegrationTests::VerifyAutomationProperties()
    {
        TestCleanupWrapper cleanup;
        auto loopingSelector = SetupLoopingSelectorAutomationTest(50);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_automationName;
        uiaInfo.m_AutomationID = s_automationId;
        uiaInfo.m_ItemStatus = s_automationItemStatus;
        uiaInfo.m_cType = s_automationTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem;
            wrl::ComPtr<IUIAutomationElement> spUINextLoopingSelectorItem;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            AutoBSTR propertyValue;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying that UIA client-side node for LoopingSelector exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            LOG_OUTPUT(L"Verifying the UIA name property from client-side node for LoopingSelector.");
            spUIAutomationElement->get_CurrentName(propertyValue.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Name = '%s', Expected = '%s'.", propertyValue.Get(), s_automationName);
            VERIFY_IS_TRUE(!wcscmp(s_automationName, propertyValue));

            LOG_OUTPUT(L"Verifying the UIA class name property from client-side node for LoopingSelector.");
            spUIAutomationElement->get_CurrentClassName(propertyValue.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"ClassName = '%s', Expected = '%s'.", propertyValue.Get(), L"LoopingSelector");
            VERIFY_IS_TRUE(!wcscmp(L"LoopingSelector", propertyValue));

            LOG_OUTPUT(L"Navigate to the first LoopingSelectorItem and verify properties.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed to create true PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed to create TreeWalker.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUILoopingSelectorItem), L"Failed to get first LoopingSelector.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem);

            spUILoopingSelectorItem->get_CurrentClassName(propertyValue.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"ClassName = '%s', Expected = '%s'.", propertyValue.Get(), L"LoopingSelectorItem");
            VERIFY_IS_TRUE(!wcscmp(L"LoopingSelectorItem", propertyValue));

            spUILoopingSelectorItem->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"PositionInSet = %d, Expected = %d.", autoVar.Storage()->lVal, 1);
            VERIFY_ARE_EQUAL(1, autoVar.Storage()->lVal);
            spUILoopingSelectorItem->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"SizeOfSet = %d, Expected = %d.", autoVar.Storage()->lVal, 50);
            VERIFY_ARE_EQUAL(50, autoVar.Storage()->lVal);

            for (int i = 0; i < 49; i++)
            {
                LOG_OUTPUT(L"Navigate to the next LoopingSelectorItem and verify properties.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUILoopingSelectorItem.Get(), &spUINextLoopingSelectorItem), L"Failed to get next LoopingSelectorItem");
                VERIFY_IS_NOT_NULL(spUINextLoopingSelectorItem);

                spUILoopingSelectorItem = spUINextLoopingSelectorItem;

                spUILoopingSelectorItem->get_CurrentClassName(propertyValue.ReleaseAndGetAddressOf());
                LOG_OUTPUT(L"ClassName = '%s', Expected = '%s'.", propertyValue.Get(), L"LoopingSelectorItem");
                VERIFY_IS_TRUE(!wcscmp(L"LoopingSelectorItem", propertyValue));

                spUILoopingSelectorItem->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                LOG_OUTPUT(L"PositionInSet = %d, Expected = %d.", autoVar.Storage()->lVal, i + 2);
                VERIFY_ARE_EQUAL(i + 2, autoVar.Storage()->lVal);
                spUILoopingSelectorItem->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                LOG_OUTPUT(L"SizeOfSet = %d, Expected = %d.", autoVar.Storage()->lVal, 50);
                VERIFY_ARE_EQUAL(50, autoVar.Storage()->lVal);
            }
        });
    }

    void LoopingSelectorAutomationIntegrationTests::VerifyPatternsRS1()
    {
        VerifyPatterns(false /* expectExpandCollapsePattern */);
    }

    void LoopingSelectorAutomationIntegrationTests::VerifyPatterns(bool expectExpandCollapsePattern)
    {
        TestCleanupWrapper cleanup;
        auto loopingSelector = SetupLoopingSelectorAutomationTest(50);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_automationName;
        uiaInfo.m_AutomationID = s_automationId;
        uiaInfo.m_ItemStatus = s_automationItemStatus;
        uiaInfo.m_cType = s_automationTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUILoopingSelector;
            wrl::ComPtr<IUIAutomationExpandCollapsePattern> spUILoopingSelectorAsExpandCollapsePattern;
            wrl::ComPtr<IUIAutomationScrollPattern> spUILoopingSelectorAsScrollPattern;
            wrl::ComPtr<IUIAutomationSelectionPattern> spUILoopingSelectorAsSelectionPattern;
            wrl::ComPtr<IUIAutomationItemContainerPattern> spUILoopingSelectorAsItemContainerPattern;

            wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem;
            wrl::ComPtr<IUIAutomationScrollItemPattern> spUILoopingSelectorItemAsScrollItemPattern;
            wrl::ComPtr<IUIAutomationSelectionItemPattern> spUILoopingSelectorItemAsSelectionItemPattern;

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomation> spUIAutomation;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            LOG_OUTPUT(L"Verify that the LoopingSelector's peer supports the Scroll, Selection, and ItemContainer patterns, and does not support the ExpandCollapse pattern.");
            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUILoopingSelector);

            LogThrow_IfFailedWithMessage(spUILoopingSelector->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, __uuidof(IUIAutomationExpandCollapsePattern), &spUILoopingSelectorAsExpandCollapsePattern), L"Failed in fetching LoopingSelector's ExpandCollapse pattern.");

            if (expectExpandCollapsePattern)
            {
                VERIFY_IS_NOT_NULL(spUILoopingSelectorAsExpandCollapsePattern);
            }
            else
            {
                VERIFY_IS_NULL(spUILoopingSelectorAsExpandCollapsePattern);
            }

            LogThrow_IfFailedWithMessage(spUILoopingSelector->GetCurrentPatternAs(UIA_ScrollPatternId, __uuidof(IUIAutomationScrollPattern), &spUILoopingSelectorAsScrollPattern), L"Failed in fetching LoopingSelector's Scroll pattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorAsScrollPattern);
            LogThrow_IfFailedWithMessage(spUILoopingSelector->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &spUILoopingSelectorAsSelectionPattern), L"Failed in fetching LoopingSelector's Selection pattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorAsSelectionPattern);
            LogThrow_IfFailedWithMessage(spUILoopingSelector->GetCurrentPatternAs(UIA_ItemContainerPatternId, __uuidof(IUIAutomationItemContainerPattern), &spUILoopingSelectorAsItemContainerPattern), L"Failed in fetching LoopingSelector's ItemContainer pattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorAsItemContainerPattern);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed to create true PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed to create TreeWalker.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUILoopingSelector.Get(), &spUILoopingSelectorItem), L"Failed to get first LoopingSelector.");

            LOG_OUTPUT(L"Verify that the LoopingSelectorItem's peer supports the ScrollItem and SelectionItem patterns.");
            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spUILoopingSelectorItemAsScrollItemPattern), L"Failed in fetching LoopingSelectorItem's ScrollItem pattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItemAsScrollItemPattern);
            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spUILoopingSelectorItemAsSelectionItemPattern), L"Failed in fetching LoopingSelectorItem's SelectionItem pattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItemAsSelectionItemPattern);
        });
    }

    void LoopingSelectorAutomationIntegrationTests::VerifyUIAClientCanScroll()
    {
        TestCleanupWrapper cleanup;
        auto loopingSelector = SetupLoopingSelectorAutomationTest(50);
        xaml_controls::ScrollViewer^ loopingSelectorScrollViewer = nullptr;

        std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            loopingSelectorScrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(loopingSelector);

            viewChangedRegistration.Attach(
                loopingSelectorScrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_automationName;
        uiaInfo.m_AutomationID = s_automationId;
        uiaInfo.m_ItemStatus = s_automationItemStatus;
        uiaInfo.m_cType = s_automationTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem1;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem2;
        wrl::ComPtr<IUIAutomationScrollItemPattern> spUILoopingSelectorItem2AsScrollItemPattern;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem3;
        wrl::ComPtr<IUIAutomationScrollItemPattern> spUILoopingSelectorItem3AsScrollItemPattern;
        wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
        wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        RECT initialBoundingRectangle;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed to create true PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed to create TreeWalker.");

            LOG_OUTPUT(L"Verifying that UIA client-side node for LoopingSelector exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUILoopingSelectorItem1), L"Failed to get first LoopingSelector.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem1);

            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem1->get_CurrentBoundingRectangle(&initialBoundingRectangle), L"Failed to get first LoopingSelector's BoundingRectangle.");

            LOG_OUTPUT(L"Navigate to the second LoopingSelectorItem and scroll it into view.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUILoopingSelectorItem1.Get(), &spUILoopingSelectorItem2), L"Failed to get second LoopingSelectorItem");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem2);

            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem2->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spUILoopingSelectorItem2AsScrollItemPattern), L"Failed in fetching second LoopingSelectorItem's ScrollItemPattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem2AsScrollItemPattern);

            VERIFY_SUCCEEDED(spUILoopingSelectorItem2AsScrollItemPattern->ScrollIntoView());
        });

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            RECT currentBoundingRectangle;
            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem2->get_CurrentBoundingRectangle(&currentBoundingRectangle), L"Failed to get second LoopingSelector's BoundingRectangle.");

            VERIFY_ARE_EQUAL(initialBoundingRectangle.left, currentBoundingRectangle.left);
            VERIFY_ARE_EQUAL(initialBoundingRectangle.right, currentBoundingRectangle.right);
            VERIFY_ARE_EQUAL(initialBoundingRectangle.top, currentBoundingRectangle.top);
            VERIFY_ARE_EQUAL(initialBoundingRectangle.bottom, currentBoundingRectangle.bottom);

            LOG_OUTPUT(L"Navigate to the third LoopingSelectorItem and scroll it into view.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUILoopingSelectorItem2.Get(), &spUILoopingSelectorItem3), L"Failed to get third LoopingSelectorItem.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem3);

            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem3->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spUILoopingSelectorItem3AsScrollItemPattern), L"Failed in fetching third LoopingSelectorItem's ScrollItemPattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem3AsScrollItemPattern);

            VERIFY_SUCCEEDED(spUILoopingSelectorItem3AsScrollItemPattern->ScrollIntoView());
        });

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            RECT currentBoundingRectangle;
            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem3->get_CurrentBoundingRectangle(&currentBoundingRectangle), L"Failed to get third LoopingSelector's BoundingRectangle.");

            VERIFY_ARE_EQUAL(initialBoundingRectangle.left, currentBoundingRectangle.left);
            VERIFY_ARE_EQUAL(initialBoundingRectangle.right, currentBoundingRectangle.right);
            VERIFY_ARE_EQUAL(initialBoundingRectangle.top, currentBoundingRectangle.top);
            VERIFY_ARE_EQUAL(initialBoundingRectangle.bottom, currentBoundingRectangle.bottom);
        });
    }

    void LoopingSelectorAutomationIntegrationTests::VerifySelectionIsSeparateFromScrolling()
    {
        TestCleanupWrapper cleanup;
        auto loopingSelector = SetupLoopingSelectorAutomationTest(50);
        xaml_controls::ScrollViewer^ loopingSelectorScrollViewer = nullptr;

        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(loopingSelector, [&](){ selectionChangedEvent->Set(); });

            loopingSelectorScrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(loopingSelector);

            viewChangedRegistration.Attach(
                loopingSelectorScrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_automationName;
        uiaInfo.m_AutomationID = s_automationId;
        uiaInfo.m_ItemStatus = s_automationItemStatus;
        uiaInfo.m_cType = s_automationTypeId;

        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
        wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem1;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem2;
        wrl::ComPtr<IUIAutomationScrollItemPattern> spUILoopingSelectorItem2AsScrollItemPattern;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem3;
        wrl::ComPtr<IUIAutomationSelectionItemPattern> spUILoopingSelectorItem3AsSelectionItemPattern;
        wrl::ComPtr<IUIAutomationElement> spUILoopingSelectorItem4;
        wrl::ComPtr<IUIAutomationScrollItemPattern> spUILoopingSelectorItem4AsScrollItemPattern;
        wrl::ComPtr<IUIAutomationSelectionItemPattern> spUILoopingSelectorItem4AsSelectionItemPattern;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying that UIA client-side node for LoopingSelector exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed to create true PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed to create TreeWalker.");

            LOG_OUTPUT(L"Navigate to the first LoopingSelectorItem.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUILoopingSelectorItem1), L"Failed to get first LoopingSelectorItem.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem1);

            LOG_OUTPUT(L"Navigate to the second LoopingSelectorItem and scroll it into view.  It should not be selected.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUILoopingSelectorItem1.Get(), &spUILoopingSelectorItem2), L"Failed to get second LoopingSelectorItem.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem2);

            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem2->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spUILoopingSelectorItem2AsScrollItemPattern), L"Failed in fetching second LoopingSelectorItem's ScrollItemPattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem2AsScrollItemPattern);

            VERIFY_SUCCEEDED(spUILoopingSelectorItem2AsScrollItemPattern->ScrollIntoView());
        });

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, loopingSelector->SelectedIndex);

            LOG_OUTPUT(L"It should, however, be put in the selected visual state.");
            VERIFY_IS_FALSE(ControlHelper::IsInVisualState(GetLoopingSelectorItemWithContent(loopingSelector, 0), L"CommonStates", L"Selected"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(GetLoopingSelectorItemWithContent(loopingSelector, 1), L"CommonStates", L"Selected"));
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            UIAutomationHelper::VerifyElementAtIndexIsSelected(0, spUIAutomation.Get(), spUIAutomationElement.Get());

            LOG_OUTPUT(L"Navigate to the third LoopingSelectorItem and select it.  It should be scrolled into view and put in the visual state.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUILoopingSelectorItem2.Get(), &spUILoopingSelectorItem3), L"Failed to get third LoopingSelectorItem.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem3);

            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem3->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spUILoopingSelectorItem3AsSelectionItemPattern), L"Failed in fetching third LoopingSelectorItem's SelectionItemPattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem3AsSelectionItemPattern);

            VERIFY_SUCCEEDED(spUILoopingSelectorItem3AsSelectionItemPattern->Select());
        });

        viewChangedEvent->WaitForDefault();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, loopingSelector->SelectedIndex);

            VERIFY_IS_FALSE(ControlHelper::IsInVisualState(GetLoopingSelectorItemWithContent(loopingSelector, 1), L"CommonStates", L"Selected"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(GetLoopingSelectorItemWithContent(loopingSelector, 2), L"CommonStates", L"Selected"));
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            UIAutomationHelper::VerifyElementAtIndexIsSelected(2, spUIAutomation.Get(), spUIAutomationElement.Get());

            LOG_OUTPUT(L"Navigate to the fourth LoopingSelectorItem.  Scroll it into view, and then select it.  We should still be able to.");
            LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUILoopingSelectorItem3.Get(), &spUILoopingSelectorItem4), L"Failed to get fourth LoopingSelectorItem.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem4);

            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem4->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spUILoopingSelectorItem4AsScrollItemPattern), L"Failed in fetching fourth LoopingSelectorItem's ScrollItemPattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem4AsScrollItemPattern);

            VERIFY_SUCCEEDED(spUILoopingSelectorItem4AsScrollItemPattern->ScrollIntoView());
        });

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            LogThrow_IfFailedWithMessage(spUILoopingSelectorItem4->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spUILoopingSelectorItem4AsSelectionItemPattern), L"Failed in fetching fourth LoopingSelectorItem's SelectionItemPattern.");
            VERIFY_IS_NOT_NULL(spUILoopingSelectorItem4AsSelectionItemPattern);

            VERIFY_SUCCEEDED(spUILoopingSelectorItem4AsSelectionItemPattern->Select());
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(3, loopingSelector->SelectedIndex);

            VERIFY_IS_FALSE(ControlHelper::IsInVisualState(GetLoopingSelectorItemWithContent(loopingSelector, 2), L"CommonStates", L"Selected"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(GetLoopingSelectorItemWithContent(loopingSelector, 3), L"CommonStates", L"Selected"));
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            UIAutomationHelper::VerifyElementAtIndexIsSelected(3, spUIAutomation.Get(), spUIAutomationElement.Get());
        });
    }

    void LoopingSelectorAutomationIntegrationTests::VerifySelectionAfterLooping()
    {
        TestCleanupWrapper cleanup;
        auto loopingSelector = SetupLoopingSelectorAutomationTest(2);
        std::shared_ptr<AutomationClient::AutomationClientManager> automationClientManager;
        wrl::ComPtr<IUIAutomation> automation;
        std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(loopingSelector, [&]() { selectionChangedEvent->Set(); });
            loopingSelector->Focus(xaml::FocusState::Keyboard);
            loopingSelector->ShouldLoop = TRUE;
            VERIFY_ARE_EQUAL(0, loopingSelector->SelectedIndex);
        });
        
        TestServices::WindowHelper->WaitForIdle();

        // Scroll down to loop over all the items
        TestServices::KeyboardHelper->Down();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            loopingSelector->Focus(xaml::FocusState::Keyboard);
            VERIFY_ARE_EQUAL(1, loopingSelector->SelectedIndex);
        });

        TestServices::KeyboardHelper->Down();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, loopingSelector->SelectedIndex);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            automationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
            automationClientManager->GetAutomation(&automation);

            LOG_OUTPUT(L"Get the focused element from UIA client");
            wrl::ComPtr<IUIAutomationElement> focusedElement;
            automation->GetFocusedElement(&focusedElement);
            VERIFY_IS_NOT_NULL(focusedElement);

            wrl::ComPtr<IUIAutomationSelectionPattern> selectionPattern;
            LogThrow_IfFailedWithMessage(focusedElement->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &selectionPattern), L"Failed getting Selection Pattern.");

            wrl::ComPtr<IUIAutomationElementArray> currentUIASelection;
            LogThrow_IfFailedWithMessage(selectionPattern->GetCurrentSelection(&currentUIASelection), L"Failed while getting selected Items.");

            int count = 0;
            LogThrow_IfFailedWithMessage(currentUIASelection->get_Length(&count), L"Failed while getting selected count.");
            LOG_OUTPUT(L"Verifying that selected count is 1.");
            VERIFY_IS_TRUE(count == 1);

            wrl::ComPtr<IUIAutomationElement> selectedItem;
            LOG_OUTPUT(L"Get the selected element");
            LogThrow_IfFailedWithMessage(currentUIASelection->GetElement(0, &selectedItem), L"Failed while getting selected item");
            VERIFY_IS_NOT_NULL(selectedItem);

            wrl::ComPtr<IUIAutomationSelectionItemPattern> selectionItemPattern;
            LogThrow_IfFailedWithMessage(selectedItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &selectionItemPattern), L"Failed in fetching SelectionItemPattern.");
            VERIFY_IS_NOT_NULL(selectionItemPattern);

            BOOL isSelected = FALSE;
            VERIFY_SUCCEEDED(selectionItemPattern->get_CurrentIsSelected(&isSelected));
            VERIFY_IS_TRUE(!!isSelected);
        });

        // Navigate up past data index 0 to loop in the other direction.
        TestServices::KeyboardHelper->Up();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, loopingSelector->SelectedIndex);
        });

        TestServices::KeyboardHelper->Up();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, loopingSelector->SelectedIndex);
        });

        TestServices::KeyboardHelper->Up();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, loopingSelector->SelectedIndex);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            // Validate the selected element
            {
                LOG_OUTPUT(L"Get the focused element from UIA client");
                wrl::ComPtr<IUIAutomationElement> focusedElement;
                automation->GetFocusedElement(&focusedElement);
                VERIFY_IS_NOT_NULL(focusedElement);

                wrl::ComPtr<IUIAutomationSelectionPattern> selectionPattern;
                LogThrow_IfFailedWithMessage(focusedElement->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &selectionPattern), L"Failed getting Selection Pattern.");

                wrl::ComPtr<IUIAutomationElementArray> currentUIASelection;
                LogThrow_IfFailedWithMessage(selectionPattern->GetCurrentSelection(&currentUIASelection), L"Failed while getting selected Items.");

                int count = 0;
                LogThrow_IfFailedWithMessage(currentUIASelection->get_Length(&count), L"Failed while getting selected count.");
                LOG_OUTPUT(L"Verifying that selected count is 1.");
                VERIFY_IS_TRUE(count == 1);

                wrl::ComPtr<IUIAutomationElement> selectedItem;
                LOG_OUTPUT(L"Get the selected element");
                LogThrow_IfFailedWithMessage(currentUIASelection->GetElement(0, &selectedItem), L"Failed while getting selected item");
                VERIFY_IS_NOT_NULL(selectedItem);

                wrl::ComPtr<IUIAutomationSelectionItemPattern> selectionItemPattern;
                LogThrow_IfFailedWithMessage(selectedItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &selectionItemPattern), L"Failed in fetching SelectionItemPattern.");
                VERIFY_IS_NOT_NULL(selectionItemPattern);

                BOOL isSelected = FALSE;
                VERIFY_SUCCEEDED(selectionItemPattern->get_CurrentIsSelected(&isSelected));
                VERIFY_IS_TRUE(!!isSelected);
            }

            LOG_OUTPUT(L"Walk through all children and make sure only one is selected"); 
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = s_automationName;
            uiaInfo.m_AutomationID = s_automationId;
            uiaInfo.m_ItemStatus = s_automationItemStatus;
            uiaInfo.m_cType = s_automationTypeId;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            automationClientManager->GetAutomation(&automation);
            wrl::ComPtr<IUIAutomationElement> automationElement;
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            LOG_OUTPUT(L"Verifying that UIA client-side node for LoopingSelector exists.");
            VERIFY_IS_NOT_NULL(automationElement);

            wrl::ComPtr<IUIAutomationCondition> trueCondition;
            wrl::ComPtr<IUIAutomationTreeWalker> walker;
            LogThrow_IfFailedWithMessage(automation->CreateTrueCondition(&trueCondition), L"Failed to create true PropertyCondition.");
            LogThrow_IfFailedWithMessage(automation->CreateTreeWalker(trueCondition.Get(), &walker), L"Failed to create TreeWalker.");

            LOG_OUTPUT(L"Navigate to the first LoopingSelectorItem.");
            wrl::ComPtr<IUIAutomationElement> item1;
            LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(automationElement.Get(), &item1), L"Failed to get first LoopingSelectorItem.");
            VERIFY_IS_NOT_NULL(item1);

            auto currentItem = item1;
            int selectedCount = 0;
            while (currentItem)
            {
                wrl::ComPtr<IUIAutomationSelectionItemPattern> selectionItemPattern;
                LogThrow_IfFailedWithMessage(currentItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &selectionItemPattern), L"Failed in fetching SelectionItemPattern.");
                VERIFY_IS_NOT_NULL(selectionItemPattern);

                BOOL isSelected = FALSE;
                VERIFY_SUCCEEDED(selectionItemPattern->get_CurrentIsSelected(&isSelected));
                if (isSelected)
                {
                    LOG_OUTPUT(L"Found Selected item.");
                    selectedCount++;
                }

                LOG_OUTPUT(L"Navigate to the next item");
                wrl::ComPtr<IUIAutomationElement> nextItem;
                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(currentItem.Get(), &nextItem), L"Failed to get next LoopingSelectorItem.");
                currentItem = nextItem;
            }

            VERIFY_ARE_EQUAL(1, selectedCount);
        });
    }

    // Once containers are recycled, they used to get a new peer everytime. We just verify here
    // that when there are enough containers and we walk through them to cause recycling, the index and counts
    // are still accurate. 
    void LoopingSelectorAutomationIntegrationTests::VerifyIndexInGroupAfterRecycling()
    {
        TestCleanupWrapper cleanup;
        wrl::ComPtr<IUIAutomationTreeWalker> automationTreeWalker;
        wrl::ComPtr<IUIAutomationElement> loopingSelectorItem;
        Common::AutoVariant autoVar;
        int itemCount = 100; // About 10 items in viewport, so 100 is plenty to cause a recycle
        auto loopingSelector = SetupLoopingSelectorAutomationTest(itemCount);

        RunOnUIThread([&]()
        {
            loopingSelector->ShouldLoop = TRUE;
            loopingSelector->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> automationElement;
            wrl::ComPtr<IUIAutomation> automation;
            wrl::ComPtr<IUIAutomationCondition> trueCondition;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = s_automationName;
            uiaInfo.m_AutomationID = s_automationId;
            uiaInfo.m_ItemStatus = s_automationItemStatus;
            uiaInfo.m_cType = s_automationTypeId;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetAutomation(&automation);
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            LOG_OUTPUT(L"Verifying that UIA client-side node for LoopingSelector exists.");
            VERIFY_IS_NOT_NULL(automationElement);

            LOG_OUTPUT(L"Navigate to the first LoopingSelectorItem and verify properties.");
            LogThrow_IfFailedWithMessage(automation->CreateTrueCondition(&trueCondition), L"Failed to create true PropertyCondition.");
            LogThrow_IfFailedWithMessage(automation->CreateTreeWalker(trueCondition.Get(), &automationTreeWalker), L"Failed to create TreeWalker.");
            LogThrow_IfFailedWithMessage(automationTreeWalker->GetFirstChildElement(automationElement.Get(), &loopingSelectorItem), L"Failed to get first LoopingSelector.");
            VERIFY_IS_NOT_NULL(loopingSelectorItem);

            loopingSelectorItem->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"PositionInSet = %d, Expected = %d.", autoVar.Storage()->lVal, 1);
            VERIFY_ARE_EQUAL(1, autoVar.Storage()->lVal);
            loopingSelectorItem->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"SizeOfSet = %d, Expected = %d.", autoVar.Storage()->lVal, itemCount);
            VERIFY_ARE_EQUAL(itemCount, autoVar.Storage()->lVal);
        });


        for (int i = 0; i < itemCount -1; i++)
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                AutoBSTR propertyValue;
                wrl::ComPtr<IUIAutomationElement> nextLoopingSelectorItem;
                wrl::ComPtr<IUIAutomationScrollItemPattern> itemScrollPattern;

                LOG_OUTPUT(L"Navigate to the next LoopingSelectorItem and verify properties.");
                LogThrow_IfFailedWithMessage(automationTreeWalker->GetNextSiblingElement(loopingSelectorItem.Get(), &nextLoopingSelectorItem), L"Failed to get next LoopingSelectorItem");
                VERIFY_IS_NOT_NULL(nextLoopingSelectorItem);

                loopingSelectorItem = nextLoopingSelectorItem;

                loopingSelectorItem->get_CurrentClassName(propertyValue.ReleaseAndGetAddressOf());
                LOG_OUTPUT(L"ClassName = '%s', Expected = '%s'.", propertyValue.Get(), L"LoopingSelectorItem");
                VERIFY_IS_TRUE(!wcscmp(L"LoopingSelectorItem", propertyValue));

                loopingSelectorItem->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                LOG_OUTPUT(L"PositionInSet = %d, Expected = %d.", autoVar.Storage()->lVal, (i + 2) % (itemCount + 1));
                VERIFY_ARE_EQUAL(i + 2, autoVar.Storage()->lVal);
                loopingSelectorItem->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                LOG_OUTPUT(L"SizeOfSet = %d, Expected = %d.", autoVar.Storage()->lVal, itemCount);
                VERIFY_ARE_EQUAL(itemCount, autoVar.Storage()->lVal);

                LogThrow_IfFailedWithMessage(loopingSelectorItem->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &itemScrollPattern), L"Failed in fetching LoopingSelectorItem's ScrollItemPattern.");
                VERIFY_IS_NOT_NULL(itemScrollPattern);

                VERIFY_SUCCEEDED(itemScrollPattern->ScrollIntoView());
            });

            TestServices::WindowHelper->WaitForIdle();
        }
       
    }
    // LoopingSelector does not export a constructor via WinRT. This is because it is not designed to be used by 3rd party apps.
    // Rather it is for use exclusively by DatePickerFlyout and TimePickerFlyout.
    // In order to test this control in isolation we rely on a test hook to create the control.
    xaml_primitives::LoopingSelector^ LoopingSelectorAutomationIntegrationTests::SetupLoopingSelectorAutomationTest(int itemCount)
    {
        xaml_primitives::LoopingSelector^ loopingSelector;

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, Loaded);

        RunOnUIThread([&]()
        {
            Platform::Object^ loopingSelectorAsObj;
            TestServices::Utilities->CreateLoopingSelector(&loopingSelectorAsObj);
            loopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(loopingSelectorAsObj);
            VERIFY_IS_NOT_NULL(loopingSelector);

            loopingSelector->Width = 100;
            loopingSelector->Height = 300;
            loopingSelector->ItemHeight = 50;
            loopingSelector->ItemWidth = 100;
            loopingSelector->ItemTemplate = nullptr;

            wfc::IVector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>();
            for (int i = 0; i < itemCount; i++)
            {
                Platform::Object^ o = ::Windows::Foundation::PropertyValue::CreateInt32(i);
                items->Append(o);
            }

            loopingSelector->Items = items;

            xaml_automation::AutomationProperties::SetName(loopingSelector, ref new Platform::String(s_automationName));
            xaml_automation::AutomationProperties::SetAutomationId(loopingSelector, ref new Platform::String(s_automationId));

            loadedRegistration.Attach(loopingSelector, [&](){ loadedEvent->Set(); });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(loopingSelector);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return loopingSelector;
    }

    xaml_primitives::LoopingSelectorItem^ LoopingSelectorAutomationIntegrationTests::GetLoopingSelectorItemWithContent(xaml_primitives::LoopingSelector^ loopingSelector, int content)
    {
        xaml_primitives::LoopingSelectorItem^ loopingSelectorItemToReturn = nullptr;

        RunOnUIThread([&]()
        {
            wfc::IVector<xaml_primitives::LoopingSelectorItem^>^ loopingSelectorItems = ref new Platform::Collections::Vector<xaml_primitives::LoopingSelectorItem^>();
            Platform::Object^ expectedContent = ::Windows::Foundation::PropertyValue::CreateInt32(content);

            TreeHelper::GetVisualChildrenByType(loopingSelector, loopingSelectorItems);

            for (unsigned int i = 0; i < loopingSelectorItems->Size; i++)
            {
                xaml_primitives::LoopingSelectorItem^ loopingSelectorItem = loopingSelectorItems->GetAt(i);

                if (loopingSelectorItem->Content->Equals(expectedContent))
                {
                    loopingSelectorItemToReturn = loopingSelectorItem;
                }
            }
        });

        return loopingSelectorItemToReturn;
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::LoopingSelector
