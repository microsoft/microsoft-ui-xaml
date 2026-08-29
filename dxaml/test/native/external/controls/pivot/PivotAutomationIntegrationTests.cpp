// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "PivotAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include "TestCleanupWrapper.h"
#include "ChangeDPI.h"

#include <ControlHelper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Controls { namespace Pivot {

        bool PivotAutomationIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

    bool PivotAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

        bool PivotAutomationIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // Test Cases

        //  Loads simple markup with Pivot and verifies AutomationProperties are correct.
        void PivotAutomationIntegrationTests::VerifyAutomationProperties()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Pivot^ pivot = SetupPivotAutomationTest();
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestPivotName";
            uiaInfo.m_AutomationID = L"TestPivotId";
            uiaInfo.m_ItemStatus = L"TestPivot";
            uiaInfo.m_cType = UIA_PaneControlTypeId;

            const WCHAR title[] = L"Test Pivot";
            const WCHAR leftHeader[] = L"Left Header Content";
            const WCHAR rightHeader[] = L"Right Header Content";
            unsigned int pivotItemCount = 3;

            RunOnUIThread([&]()
            {
                pivot->Title = ref new Platform::String(title);
                pivot->LeftHeader = ref new Platform::String(leftHeader);
                pivot->RightHeader = ref new Platform::String(rightHeader);
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                Common::AutoVariant autoVar;
                Common::AutoBSTR autoBstr;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                LOG_OUTPUT(L"Verifying UIA localized control property for Pivot.");
                spUIAutomationElement->get_CurrentLocalizedControlType(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(L"tab", autoBstr);

                wrl::ComPtr<IUIAutomation> spAutomation;
                spAutomationClientManager->GetAutomation(&spAutomation);

                wrl::ComPtr<IUIAutomation> spUIAutomation;
                spAutomationClientManager->GetAutomation(&spUIAutomation);

                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"Failed to create true PropertyCondition.");

                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"Failed to create TreeWalker.");

                // The first child should be the Title:
                wrl::ComPtr<IUIAutomationElement> spChildElement;
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spChildElement), L"Failed to get first child element.");
                VERIFY_IS_NOT_NULL(spChildElement);

                spChildElement->get_CurrentClassName(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(L"TextBlock", autoBstr);

                spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(title, autoBstr);

                // The next child should be the Left Header:
                wrl::ComPtr<IUIAutomationElement> spNextChildElement;
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spChildElement.Get(), &spNextChildElement), L"Failed to get next PivotItem");
                VERIFY_IS_NOT_NULL(spNextChildElement);
                spChildElement = spNextChildElement;

                spChildElement->get_CurrentClassName(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(L"TextBlock", autoBstr);

                spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(leftHeader, autoBstr);

                // The next three children should be the PivotItems:
                for (unsigned int i = 1; i <= pivotItemCount; i++)
                {
                    LOG_OUTPUT(L"Navigate to the next PivotItem and verify properties.");
                    LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spChildElement.Get(), &spNextChildElement), L"Failed to get next PivotItem");
                    VERIFY_IS_NOT_NULL(spNextChildElement);
                    spChildElement = spNextChildElement;

                    spChildElement->get_CurrentLocalizedControlType(autoBstr.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(L"tab item", autoBstr);

                    spChildElement->get_CurrentClassName(autoBstr.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(L"PivotItem", autoBstr);

                    spChildElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                    VERIFY_ARE_EQUAL(static_cast<LONG>(i), autoVar.Storage()->lVal, L"Verify PositionInSet property of PivotItem");

                    spChildElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                    VERIFY_ARE_EQUAL(static_cast<LONG>(pivotItemCount), autoVar.Storage()->lVal, L"Verify SizeOfSet property of PivotItem");

                    spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                    auto expectedName = ref new Platform::String(L"Item ") + i;
                    auto actualName = ref new Platform::String(autoBstr.Get());
                    VERIFY_ARE_EQUAL(expectedName, actualName, L"Verify UIA Name of PivotItem");
                }

                // The next child should be the Right Header Content:
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spChildElement.Get(), &spNextChildElement), L"Failed to get next PivotItem");
                VERIFY_IS_NOT_NULL(spNextChildElement);
                spChildElement = spNextChildElement;

                spChildElement->get_CurrentClassName(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(L"TextBlock", autoBstr);

                spChildElement->get_CurrentName(autoBstr.ReleaseAndGetAddressOf());
                Common::AutoBSTR::VerifyAreEqual(rightHeader, autoBstr);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Pivot^ pivot = SetupPivotAutomationTest();

            wf::Rect windowBounds;
            wf::Rect controlBounds1;
            wf::Rect controlBounds2;

            RunOnUIThread([&]()
            {
                windowBounds = TestServices::WindowHelper->WindowBounds;
                controlBounds1 = ControlHelper::GetBounds(GetPivotHeader(pivot, 0));
                controlBounds2 = ControlHelper::GetBounds(GetPivotHeader(pivot, 1));
            });

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestPivotName";
            uiaInfo.m_AutomationID = L"TestPivotId";
            uiaInfo.m_ItemStatus = L"TestPivot";
            uiaInfo.m_cType = UIA_PaneControlTypeId;

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElementParent;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement1;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement2;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement3;
                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                wrl::ComPtr<IUIAutomation> spUIAutomation;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                spAutomationClientManager->GetAutomation(&spUIAutomation);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed in creating True PropertyCondition.");
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed in creating TreeWalker.");

                LOG_OUTPUT(L"Verifying UIA Client side node for Pivot exist.");
                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                Common::AutoVariant autoVar;

                LOG_OUTPUT(L"Get the Window UIA Element.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetParentElement(spUIAutomationElement.Get(), &spUIPivotItemAutomationElementParent), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get window.");
                VERIFY_IS_NOT_NULL(spUIPivotItemAutomationElementParent);

                spUIPivotItemAutomationElementParent->GetCurrentPropertyValue(UIA_BoundingRectanglePropertyId, autoVar.ReleaseAndGetAddressOf());
                wf::Rect windowBoundingRectangle = UIAutomationHelper::RectFromVariant(autoVar.Storage());

                LOG_OUTPUT(L"Navigate to first item in PivotItem and verify properties.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIPivotItemAutomationElement1), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get first PivotItem.");
                VERIFY_IS_NOT_NULL(spUIPivotItemAutomationElement1);

                spUIPivotItemAutomationElement1->GetCurrentPropertyValue(UIA_BoundingRectanglePropertyId, autoVar.ReleaseAndGetAddressOf());
                wf::Rect boundingRectangle1 = UIAutomationHelper::RectFromVariant(autoVar.Storage());
                // UIA reports bounding rectangles in screen coordinates, where xaml reports app coordinates. On windows core, the test app is not opened in full screen
                // which caused discrepancies because neither Control bounds or Window bounds accounted for the screen coordinate position when we did the below comparison.
                // instead if we use the UIA bounding rect for the window we now account for this potentiality.
                LOG_OUTPUT(L"Bounding rectangle = (%.0f, %.0f, %.0f, %.0f)", boundingRectangle1.X, boundingRectangle1.Y, boundingRectangle1.Width, boundingRectangle1.Height);
                LOG_OUTPUT(L"Control bounds     = (%.0f, %.0f, %.0f, %.0f)", controlBounds1.X, controlBounds1.Y, controlBounds1.Width, controlBounds1.Height);
                LOG_OUTPUT(L"Window bounds     = (%.0f, %.0f, %.0f, %.0f)", windowBounds.X, windowBounds.Y, windowBounds.Width, windowBounds.Height);
                LOG_OUTPUT(L"Window bounding rectangle = (%.0f, %.0f, %.0f, %.0f)", windowBoundingRectangle.X, windowBoundingRectangle.Y, windowBoundingRectangle.Width, windowBoundingRectangle.Height);
                spUIPivotItemAutomationElement1->GetCurrentPropertyValue(UIA_ClickablePointPropertyId, autoVar.ReleaseAndGetAddressOf());
                wf::Point clickablePoint1 = UIAutomationHelper::PointFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Clickable point = (%.0f, %.0f)", clickablePoint1.X, clickablePoint1.Y);
                spUIPivotItemAutomationElement1->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                bool isOffscreen1 = UIAutomationHelper::BoolFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Is offscreen = %s", isOffscreen1 ? L"true" : L"false");

                VERIFY_ARE_EQUAL(controlBounds1.X + windowBoundingRectangle.X, boundingRectangle1.X);
                VERIFY_ARE_EQUAL(controlBounds1.Y + windowBoundingRectangle.Y, boundingRectangle1.Y);
                VERIFY_ARE_EQUAL(controlBounds1.Width, boundingRectangle1.Width);
                VERIFY_ARE_EQUAL(controlBounds1.Height, boundingRectangle1.Height);
                // ClickablePoint values are truncated to the nearest integer in transit to the client,
                // so we need to account for that.
                VERIFY_ARE_EQUAL((int)(boundingRectangle1.X + boundingRectangle1.Width / 2), (int)clickablePoint1.X);
                VERIFY_ARE_EQUAL((int)(boundingRectangle1.Y + boundingRectangle1.Height / 2), (int)clickablePoint1.Y);
                VERIFY_IS_FALSE(isOffscreen1);

                LOG_OUTPUT(L"Navigate to second item in PivotItem and verify properties.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIPivotItemAutomationElement1.Get(), &spUIPivotItemAutomationElement2), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get second PivotItem.");
                VERIFY_IS_NOT_NULL(spUIPivotItemAutomationElement2);

                spUIPivotItemAutomationElement2->GetCurrentPropertyValue(UIA_BoundingRectanglePropertyId, autoVar.ReleaseAndGetAddressOf());
                wf::Rect boundingRectangle2 = UIAutomationHelper::RectFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Bounding rectangle = (%.0f, %.0f, %.0f, %.0f)", boundingRectangle2.X, boundingRectangle2.Y, boundingRectangle2.Width, boundingRectangle2.Height);
                LOG_OUTPUT(L"Control bounds     = (%.0f, %.0f, %.0f, %.0f)", controlBounds2.X, controlBounds2.Y, controlBounds2.Width, controlBounds2.Height);
                LOG_OUTPUT(L"Window bounds     = (%.0f, %.0f, %.0f, %.0f)", windowBounds.X, windowBounds.Y, windowBounds.Width, windowBounds.Height);
                LOG_OUTPUT(L"Window bounding rectangle = (%.0f, %.0f, %.0f, %.0f)", windowBoundingRectangle.X, windowBoundingRectangle.Y, windowBoundingRectangle.Width, windowBoundingRectangle.Height);
                spUIPivotItemAutomationElement2->GetCurrentPropertyValue(UIA_ClickablePointPropertyId, autoVar.ReleaseAndGetAddressOf());
                wf::Point clickablePoint2 = UIAutomationHelper::PointFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Clickable point = (%.0f, %.0f)", clickablePoint2.X, clickablePoint2.Y);
                spUIPivotItemAutomationElement2->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                bool isOffscreen2 = UIAutomationHelper::BoolFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Is offscreen = %s", isOffscreen2 ? L"true" : L"false");

                VERIFY_ARE_EQUAL(controlBounds2.X + windowBoundingRectangle.X, boundingRectangle2.X);
                VERIFY_ARE_EQUAL(controlBounds2.Y + windowBoundingRectangle.Y, boundingRectangle2.Y);
                // The second header is half-way offscreen, so its bounding rectangle will be reported
                // as less than the control bounds.
                VERIFY_IS_GREATER_THAN(controlBounds2.Width, boundingRectangle2.Width);
                VERIFY_ARE_EQUAL(controlBounds2.Height, boundingRectangle2.Height);
                // ClickablePoint values are truncated to the nearest integer in transit to the client,
                // so we need to account for that.
                VERIFY_ARE_EQUAL((int)(boundingRectangle2.X + boundingRectangle2.Width / 2), (int)clickablePoint2.X);
                VERIFY_ARE_EQUAL((int)(boundingRectangle2.Y + boundingRectangle2.Height / 2), (int)clickablePoint2.Y);
                VERIFY_IS_FALSE(isOffscreen2);

                LOG_OUTPUT(L"Navigate to third item in PivotItem and verify properties.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIPivotItemAutomationElement2.Get(), &spUIPivotItemAutomationElement3), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get third PivotItem.");
                VERIFY_IS_NOT_NULL(spUIPivotItemAutomationElement3);

                spUIPivotItemAutomationElement3->GetCurrentPropertyValue(UIA_BoundingRectanglePropertyId, autoVar.ReleaseAndGetAddressOf());
                wf::Rect boundingRectangle3 = UIAutomationHelper::RectFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Bounding rectangle = (%.0f, %.0f, %.0f, %.0f)", boundingRectangle3.X, boundingRectangle3.Y, boundingRectangle3.Width, boundingRectangle3.Height);
                spUIPivotItemAutomationElement3->GetCurrentPropertyValue(UIA_ClickablePointPropertyId, autoVar.ReleaseAndGetAddressOf());
                bool clickablePoint3WasEmpty = autoVar.Storage()->vt == VT_EMPTY;
                spUIPivotItemAutomationElement3->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                bool isOffscreen3 = UIAutomationHelper::BoolFromVariant(autoVar.Storage());
                LOG_OUTPUT(L"Is offscreen = %s", isOffscreen3 ? L"true" : L"false");

                // The third header is completely offscreen, so its bounding rectangle will be reported
                // as all zeroes, and it won't return a clickable point.
                VERIFY_ARE_EQUAL(0, boundingRectangle3.X);
                VERIFY_ARE_EQUAL(0, boundingRectangle3.Y);
                VERIFY_ARE_EQUAL(0, boundingRectangle3.Width);
                VERIFY_ARE_EQUAL(0, boundingRectangle3.Height);
                VERIFY_IS_TRUE(clickablePoint3WasEmpty);
                VERIFY_IS_TRUE(isOffscreen3);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void PivotAutomationIntegrationTests::VerifyFocusChangesOnSelectedItemChange()
        {
            // Leak: CUIAWrapper reports as being leaked
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            TestCleanupWrapper cleanup;

            xaml_controls::Pivot^ pivot = SetupPivotAutomationTest();
            auto focusChangeEvent = std::make_shared<Event>();
            wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> spAutomationFocusChangeHandler;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestPivotName";
            uiaInfo.m_AutomationID = L"TestPivotId";
            uiaInfo.m_ItemStatus = L"TestPivot";
            uiaInfo.m_cType = UIA_PaneControlTypeId;

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                wrl::ComPtr<IUIAutomation> spUIAutomation;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationFocusChangeHandler.Attach(new AutomationClient::AutomationFocusChangeHandler(spAutomationClientManager, focusChangeEvent, TreeScope_Subtree));
                spAutomationFocusChangeHandler->Init();
            });

            RunOnUIThread([&]()
            {
                pivot->Focus(xaml::FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spAutomationFocusChangeHandler->AttachEventHandler();
            });

            LOG_OUTPUT(L"Move to the next PivotItem with the right arrow on the keyboard.  A UI automation FocusChanged event should be raised.");
            TestServices::KeyboardHelper->Right();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement1;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement2;
                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                wrl::ComPtr<IUIAutomation> spUIAutomation;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                spAutomationClientManager->GetAutomation(&spUIAutomation);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed in creating True PropertyCondition.");
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed in creating TreeWalker.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIPivotItemAutomationElement1), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get first PivotItem.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIPivotItemAutomationElement1.Get(), &spUIPivotItemAutomationElement2), L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get second PivotItem.");

                spAutomationFocusChangeHandler->Confirm();

                Common::AutoBSTR pivotItem2Name;
                Common::AutoBSTR focusedElementName;

                spUIPivotItemAutomationElement2->get_CurrentName(pivotItem2Name.ReleaseAndGetAddressOf());
                spAutomationFocusChangeHandler->GetLastFocusedElement()->get_CurrentName(focusedElementName.ReleaseAndGetAddressOf());

                LOG_OUTPUT(L"The currently focused element should be the second PivotItem now.");
                Common::AutoBSTR::VerifyAreEqual(pivotItem2Name, focusedElementName);

                spAutomationFocusChangeHandler->RemoveEventHandler();
            });
        }

        //  Loads simple markup with Pivot and verifies hit-testing on PivotItems.
        void PivotAutomationIntegrationTests::VerifyHitTestOnPivotItems()
        {
            // Leak: CUIAWrapper reports as being leaked
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            TestCleanupWrapper cleanup;
            xaml_controls::Pivot^ pivot = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestPivotName";
            uiaInfo.m_AutomationID = L"TestPivotId";
            uiaInfo.m_ItemStatus = L"TestPivot";
            uiaInfo.m_cType = UIA_PaneControlTypeId;

            int countPivotItems;

            RunOnUIThread([&]()
            {
                // Important: Make the pivot wider than all the pivot items contained in it. Each of the 3 pivot items
                // in this test have a width of around 100, so given the pivot a width of 500.

                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Margin='50' Width='500'>"
                    L"  <PivotItem Header='PItem1' AutomationProperties.Name='TestPivotItem1'>"
                    L"      <StackPanel Margin='5'>"
                    L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />"
                    L"      </StackPanel>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem2' AutomationProperties.Name='TestPivotItem2'>"
                    L"      <StackPanel Margin='5'>"
                    L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />"
                    L"      </StackPanel>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem3' AutomationProperties.Name='TestPivotItem3'>"
                    L"      <StackPanel Margin='5'>"
                    L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />"
                    L"      </StackPanel>"
                    L"  </PivotItem>"
                    L"</Pivot>"));

                VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);
                VERIFY_ARE_EQUAL(pivot->Items->Size, 3u);
                xaml_automation::AutomationProperties::SetName(pivot, ref new Platform::String(uiaInfo.m_Name));
                xaml_automation::AutomationProperties::SetAutomationId(pivot, ref new Platform::String(uiaInfo.m_AutomationID));

                // Attach Pivot to Window content
                TestServices::WindowHelper->WindowContent = pivot;

                countPivotItems = pivot->Items->Size;
            });

            // Wait for all async activities to be done
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                LOG_OUTPUT(L"Verifying pivot element was created.");
                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                wrl::ComPtr<IUIAutomation> spUIAutomation;
                spAutomationClientManager->GetAutomation(&spUIAutomation);

                POINT pt = { 0, 0 };
                Common::AutoVariant autoVar;
                Common::AutoBSTR autoBSTR;
                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement;
                wrl::ComPtr<IUIAutomationElement> spUINextPivotItemAutomationElement;
                wrl::ComPtr<IUIAutomationElement> spUIAutomationHitTestElement;

                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition),
                    L"PivotAutomationIntegrationTests::VerifyHitTestOnPivotItems: Failed in creating True PropertyCondition.");

                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker),
                    L"PivotAutomationIntegrationTests::VerifyHitTestOnPivotItems: Failed in creating TreeWalker.");

                LOG_OUTPUT(L"Navigate to first PivotItem.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIPivotItemAutomationElement),
                    L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get first PivotItem.");
                VERIFY_IS_NOT_NULL(spUIPivotItemAutomationElement);

                // Now loop through all the pivot items, hit-testing them in a variety of places.
                const wchar_t* pszExpectedPivotItemNames[] = {L"TestPivotItem1", L"TestPivotItem2", L"TestPivotItem3"};

                for (int i = 0; i < countPivotItems; ++i)
                {
                    LOG_OUTPUT(L"Interacting with next pivot item.");

                    spUIPivotItemAutomationElement->get_CurrentName(autoBSTR.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(pszExpectedPivotItemNames[i], autoBSTR);

                    // First hit-test at where the pivot item claims is a clickable point.
                    LOG_OUTPUT(L"Get pivot item's ClickablePoint.");
                    spUIPivotItemAutomationElement->GetCurrentPropertyValue(UIA_ClickablePointPropertyId, autoVar.ReleaseAndGetAddressOf());
                    wf::Point clickablePoint = UIAutomationHelper::PointFromVariant(autoVar.Storage());
                    pt.x = (LONG)clickablePoint.X;
                    pt.y = (LONG)clickablePoint.Y;

                    LOG_OUTPUT(L"Hit-test at the pivot item's ClickablePoint.");
                    spUIAutomation->ElementFromPoint(pt, &spUIAutomationHitTestElement);
                    VERIFY_IS_NOT_NULL(spUIAutomationHitTestElement);
                    spUIAutomationHitTestElement->get_CurrentName(autoBSTR.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(pszExpectedPivotItemNames[i], autoBSTR);

                    // Next hit-test at the center of the pivot items BoundingRect.
                    LOG_OUTPUT(L"Get pivot item's BoundingRectangle.");
                    spUIPivotItemAutomationElement->GetCurrentPropertyValue(UIA_BoundingRectanglePropertyId, autoVar.ReleaseAndGetAddressOf());
                    wf::Rect pivotItemBoundingRectangle = UIAutomationHelper::RectFromVariant(autoVar.Storage());
                    pt.x = (LONG)(pivotItemBoundingRectangle.X + (pivotItemBoundingRectangle.Width / 2));
                    pt.y = (LONG)(pivotItemBoundingRectangle.Y + (pivotItemBoundingRectangle.Height / 2));

                    LOG_OUTPUT(L"Hit-test at center of pivot item's bounding rect.");
                    spUIAutomation->ElementFromPoint(pt, &spUIAutomationHitTestElement);
                    VERIFY_IS_NOT_NULL(spUIAutomationHitTestElement);
                    spUIAutomationHitTestElement->get_CurrentName(autoBSTR.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(pszExpectedPivotItemNames[i], autoBSTR);

                    // Next hit-test near the corners of the pivot item.
                    pt.x = (LONG)pivotItemBoundingRectangle.X + 1;
                    pt.y = (LONG)pivotItemBoundingRectangle.Y + 1;

                    LOG_OUTPUT(L"Hit-test near the top-left corner of the pivot item.");
                    spUIAutomation->ElementFromPoint(pt, &spUIAutomationHitTestElement);
                    VERIFY_IS_NOT_NULL(spUIAutomationHitTestElement);
                    spUIAutomationHitTestElement->get_CurrentName(autoBSTR.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(pszExpectedPivotItemNames[i], autoBSTR);

                    pt.x = (LONG)(pivotItemBoundingRectangle.X + pivotItemBoundingRectangle.Width) - 1;
                    pt.y = (LONG)(pivotItemBoundingRectangle.Y + pivotItemBoundingRectangle.Height) - 1;

                    LOG_OUTPUT(L"Hit-test near the bottom-right corner of the first pivot item.");
                    spUIAutomation->ElementFromPoint(pt, &spUIAutomationHitTestElement);
                    VERIFY_IS_NOT_NULL(spUIAutomationHitTestElement);
                    spUIAutomationHitTestElement->get_CurrentName(autoBSTR.ReleaseAndGetAddressOf());
                    Common::AutoBSTR::VerifyAreEqual(pszExpectedPivotItemNames[i], autoBSTR);

                    // Is this the last pivot item?
                    if (i < countPivotItems - 1)
                    {
                        LOG_OUTPUT(L"Navigate to next PivotItem.");
                        LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIPivotItemAutomationElement.Get(), &spUINextPivotItemAutomationElement),
                            L"PivotAutomationIntegrationTests::VerifyOverriddenValuesForPivotItems: Failed to get next PivotItem.");
                        VERIFY_IS_NOT_NULL(spUINextPivotItemAutomationElement);

                        spUIPivotItemAutomationElement = spUINextPivotItemAutomationElement.Detach();
                    }
                    else
                    {
                        // Now verify that a hit-test beyond the last pivot item hits the pivot.
                        pt.x += 5;

                        LOG_OUTPUT(L"Hit-test beyond the last pivot item.");
                        spUIAutomation->ElementFromPoint(pt, &spUIAutomationHitTestElement);
                        spUIAutomationHitTestElement->get_CurrentName(autoBSTR.ReleaseAndGetAddressOf());
                        Common::AutoBSTR::VerifyAreEqual(uiaInfo.m_Name, autoBSTR);
                    }
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void PivotAutomationIntegrationTests::VerifyPivotItemsReportCorrectPositionAndItemCount()
        {
            // Leak: CUIAWrapper reports as being leaked
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            TestCleanupWrapper cleanup;
            xaml_controls::Pivot^ pivot = nullptr;
            xaml_controls::Button^ focusButton = nullptr;
            auto focusChangeEvent = std::make_shared<Event>();
            wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> spAutomationFocusChangeHandler;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestPivotName";
            uiaInfo.m_AutomationID = L"TestPivotId";
            uiaInfo.m_ItemStatus = L"TestPivot";
            uiaInfo.m_cType = UIA_PaneControlTypeId;

            auto pivotItemLoadedEvent = std::make_shared<Event>();
            auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);
            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                <Button Content='Button 1' />
                                <Button x:Name='FocusButton' Content='Button 1' />
                            </StackPanel>
                            <Pivot Name='Pivot'
                                   Width='150'>
                                <PivotItem Header='Item 1'>
                                    <StackPanel Margin='5'>
                                        <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />
                                    </StackPanel>
                                </PivotItem>
                                <PivotItem Header='Item 2'>
                                    <StackPanel Margin='5'>
                                        <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />
                                    </StackPanel>
                                </PivotItem>
                                <PivotItem Header='Item 3'>
                                    <StackPanel Margin='5'>
                                        <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />
                                    </StackPanel>
                                </PivotItem>
                            </Pivot>
                        </StackPanel>)"));

                pivot = safe_cast<xaml_controls::Pivot^>(rootPanel->FindName(L"Pivot"));
                pivotItemLoadedRegistration.Attach(pivot, [pivotItemLoadedEvent](){ pivotItemLoadedEvent->Set(); });

                xaml_automation::AutomationProperties::SetName(pivot, ref new Platform::String(uiaInfo.m_Name));
                xaml_automation::AutomationProperties::SetAutomationId(pivot, ref new Platform::String(uiaInfo.m_AutomationID));

                focusButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"FocusButton"));
                buttonGotFocusRegistration.Attach(focusButton, [&]() { buttonGotFocusEvent->Set(); });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            pivotItemLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"We'll initialize focus on the button immediately before the Pivot, so we can deterministically give the Pivot focus via tab.");
                focusButton->Focus(xaml::FocusState::Keyboard);
            });

            buttonGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationFocusChangeHandler.Attach(new AutomationClient::AutomationFocusChangeHandler(spAutomationClientManager, focusChangeEvent, TreeScope_Subtree));
                spAutomationFocusChangeHandler->Init();
                spAutomationFocusChangeHandler->AttachEventHandler();
            });

            LOG_OUTPUT(L"Give the Pivot focus with the tab key.  A UI automation FocusChanged event should be raised.");
            TestServices::KeyboardHelper->Tab();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                Common::AutoVariant autoVar;

                spAutomationFocusChangeHandler->Confirm();

                spAutomationFocusChangeHandler->GetLastFocusedElement()->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_ARE_EQUAL(1, autoVar.Storage()->lVal, L"Verify PositionInSet property of PivotItem");

                spAutomationFocusChangeHandler->GetLastFocusedElement()->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_ARE_EQUAL(3, autoVar.Storage()->lVal, L"Verify SizeOfSet property of PivotItem");

                spAutomationFocusChangeHandler->RemoveEventHandler();
            });
        }

        interface class IDataItem
        {
            property Platform::String^ Name { Platform::String^ get(); };
        };

        ref class DataItem sealed: public IDataItem
        {
        public:
            DataItem(Platform::String^ name) : m_name(name) { }
            virtual property Platform::String^ Name { Platform::String^ get() { return m_name; } }
        private:
            Platform::String^ m_name;
        };

        void PivotAutomationIntegrationTests::VerifyBindingPivotSelectedItemToInterfacePreservesSelectionAutomationProperty()
        {
            // Leak: CUIAWrapper reports as being leaked
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            TestCleanupWrapper cleanup;
            xaml_controls::Pivot^ pivot = nullptr;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestPivotName";
            uiaInfo.m_AutomationID = L"TestPivotId";
            uiaInfo.m_ItemStatus = L"TestPivot";
            uiaInfo.m_cType = UIA_PaneControlTypeId;

            auto pivotItemLoadedEvent = std::make_shared<Event>();
            auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);

            RunOnUIThread([&]()
            {
                auto pivot = safe_cast<xaml_controls::Pivot^>(xaml_markup::XamlReader::Load(
                    LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <Pivot.HeaderTemplate>
                                <DataTemplate>
                                    <TextBlock Text='{Binding Name}' />
                                </DataTemplate>
                            </Pivot.HeaderTemplate>
                            <Pivot.ItemTemplate>
                                <DataTemplate>
                                    <TextBlock Text='{Binding Name}' />
                                </DataTemplate>
                            </Pivot.ItemTemplate>
                        </Pivot>)"));

                wfc::IVector<IDataItem^>^ dataItems = ref new Platform::Collections::Vector<IDataItem^>();
                dataItems->Append(ref new DataItem("James"));
                dataItems->Append(ref new DataItem("Joel"));
                dataItems->Append(ref new DataItem("Steph"));

                pivot->ItemsSource = dataItems;
                pivot->SelectedItem = dataItems->GetAt(0);

                pivotItemLoadedRegistration.Attach(pivot, [pivotItemLoadedEvent](){ pivotItemLoadedEvent->Set(); });

                xaml_automation::AutomationProperties::SetName(pivot, ref new Platform::String(uiaInfo.m_Name));
                xaml_automation::AutomationProperties::SetAutomationId(pivot, ref new Platform::String(uiaInfo.m_AutomationID));

                TestServices::WindowHelper->WindowContent = pivot;
            });

            pivotItemLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationElement> spUIPivotItemAutomationElement;
                wrl::ComPtr<IUIAutomationSelectionItemPattern> spUIPivotItemSelectionItemPattern;
                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                wrl::ComPtr<IUIAutomation> spUIAutomation;
                BOOL isPivotItemSelected = FALSE;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                spAutomationClientManager->GetAutomation(&spUIAutomation);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"PivotAutomationIntegrationTests::VerifyBindingPivotSelectedItemToInterfacePreservesSelectionAutomationProperty: Failed in creating True PropertyCondition.");
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"PivotAutomationIntegrationTests::VerifyBindingPivotSelectedItemToInterfacePreservesSelectionAutomationProperty: Failed in creating TreeWalker.");

                LOG_OUTPUT(L"Navigate to first item in PivotItem and verify that it reports itself to UIA clients as selected.");
                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIPivotItemAutomationElement),
                         L"PivotAutomationIntegrationTests::VerifyBindingPivotSelectedItemToInterfacePreservesSelectionAutomationProperty: Failed to get first PivotItem.");
                LogThrow_IfFailedWithMessage(spUIPivotItemAutomationElement->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spUIPivotItemSelectionItemPattern),
                         L"PivotAutomationIntegrationTests::VerifyBindingPivotSelectedItemToInterfacePreservesSelectionAutomationProperty: Failed to get first PivotItem as SelectionItem.");
                LogThrow_IfFailed(spUIPivotItemSelectionItemPattern->get_CurrentIsSelected(&isPivotItemSelected));
                VERIFY_IS_TRUE(isPivotItemSelected);
            });
        }

        void PivotAutomationIntegrationTests::VerifyScanModeNavigation()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Pivot^ pivot = SetupPivotAutomationTest();

            RunOnUIThread([&]()
            {
                pivot->Focus(xaml::FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Verify first item is selected");
                VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);
            });

            Automation::AutomationClient::UIAElementInfo item1ContentUIAInfo;
            item1ContentUIAInfo.m_Name = L"Item1StackPanel";

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                LOG_OUTPUT(L"Verify pivot item 1 content is in UIA tree");

                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(item1ContentUIAInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                VERIFY_IS_NOT_NULL(spUIAutomationElement);
            });

            LOG_OUTPUT(L"Move to the next PivotItem with the right arrow on keyboard.");
            TestServices::KeyboardHelper->Right();

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Verify second item is selected");
                VERIFY_ARE_EQUAL(pivot->SelectedIndex, 1);
            });

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                LOG_OUTPUT(L"Verify pivot item 1 content is not in UIA tree");

                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(item1ContentUIAInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                VERIFY_IS_NULL(spUIAutomationElement);
            });

        }

        xaml_controls::Pivot^ PivotAutomationIntegrationTests::SetupPivotAutomationTest()
        {
            xaml_controls::Pivot^ pivot = nullptr;

            auto pivotItemLoadedEvent = std::make_shared<Event>();
            auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);

            RunOnUIThread([&]()
            {
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                Width='150'>
                            <PivotItem Header='Item 1'>
                                <StackPanel Margin='5' AutomationProperties.Name="Item1StackPanel">
                                    <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />
                                </StackPanel>
                            </PivotItem>
                            <PivotItem Header='Item 2' >
                                <StackPanel Margin='5' AutomationProperties.Name="Item2StackPanel">
                                    <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />
                                </StackPanel>
                            </PivotItem>
                            <PivotItem Header='Item 3' >
                                <StackPanel Margin='5' AutomationProperties.Name="Item3StackPanel">
                                    <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />
                                </StackPanel>
                            </PivotItem>
                        </Pivot>)"));

                VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);
                VERIFY_ARE_EQUAL(pivot->Items->Size, 3u);
                xaml_automation::AutomationProperties::SetName(pivot, ref new Platform::String(L"TestPivotName"));
                xaml_automation::AutomationProperties::SetAutomationId(pivot, ref new Platform::String(L"TestPivotId"));

                pivotItemLoadedRegistration.Attach(pivot, [pivotItemLoadedEvent](){ pivotItemLoadedEvent->Set(); });
                TestServices::WindowHelper->WindowContent = pivot;
            });

            pivotItemLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            return pivot;
        }

        xaml::FrameworkElement^ PivotAutomationIntegrationTests::GetPivotHeader(xaml_controls::Pivot^ pivot, int index)
        {
            xaml_controls::Panel^ headerPanel = safe_cast<xaml_controls::Panel^>(TreeHelper::GetVisualChildByName(pivot, "Header"));
            xaml_controls::Panel^ staticHeaderPanel = safe_cast<xaml_controls::Panel^>(TreeHelper::GetVisualChildByName(pivot, "StaticHeader"));

            if (headerPanel->Children->Size > 0)
            {
                return safe_cast<xaml::FrameworkElement^>(headerPanel->Children->GetAt(index));
            }
            else
            {
                return safe_cast<xaml::FrameworkElement^>(staticHeaderPanel->Children->GetAt(index));
            }
        }
    } }
} } } }
