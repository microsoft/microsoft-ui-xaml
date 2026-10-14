// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <Patterns\InvokePatternHandler.h>
#include <HyperlinkAutomationPeerIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

#include <ChangeDPI.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool HyperlinkAutomationPeerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool HyperlinkAutomationPeerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool HyperlinkAutomationPeerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_docs::Hyperlink^ HyperlinkAutomationPeerIntegrationTests::SetupHyperlinkTest()
    {
        auto gridRoot = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
            L"<Grid \r\n"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
            L"  x:Name='root' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
            L"      <TextBlock x:Name='tb' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink' AutomationProperties.Name='hyperlink'> \r\n"
            L"              Hello world \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"      <TextBlock x:Name='tbN' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink1Name' AutomationProperties.Name='hyperlink1Name' NavigateUri=\"http://www.bing.com\"> \r\n"
            L"              Hello world \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"      <TextBlock x:Name='tbNN' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink2Name' NavigateUri=\"http://www.bing.com\"> \r\n"
            L"              HyperlinkNoName \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"      <TextBlock x:Name='tbNC' Width='100' Height='400'> \r\n"
            L"          <Hyperlink x:Name='hyperlink3Name' NavigateUri=\"http://www.bing.com\"> \r\n"
            L"          </Hyperlink> \r\n"
            L"      </TextBlock> \r\n"
            L"</Grid>"));
        TestServices::WindowHelper->WindowContent = gridRoot;

        xaml_docs::Hyperlink^ hyperlink = safe_cast<xaml_docs::Hyperlink^>(gridRoot->FindName(L"hyperlink"));

        return hyperlink;
    }

    //
    // Test Cases
    //
    void HyperlinkAutomationPeerIntegrationTests::DoesSupportEssentialPatterns()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"hyperlink";
        uiaInfo.m_AutomationID = L"hyperlink";
        uiaInfo.m_ItemStatus = L"hyperlink";
        uiaInfo.m_cType = UIA_HyperlinkControlTypeId;

        RunOnUIThread([&]()
        {
            SetupHyperlinkTest();
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            AutomationClient::AutomationGenericTests automationGenericTests(spAutomationClientManager);
            automationGenericTests.DoesSupportEssentialPatternsForControlType(uiaInfo.m_cType);
        });

    }

    void HyperlinkAutomationPeerIntegrationTests::CanBeInvokedByUIA()
    {
        TestCleanupWrapper cleanup;
        xaml_docs::Hyperlink^ hyperlink = nullptr;
        Automation::AutomationClient::UIAElementInfo uiaInfo;

        auto spClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_docs::Hyperlink, Click);


        RunOnUIThread([&]()
        {
            uiaInfo.m_Name = L"hyperlink";
            uiaInfo.m_AutomationID = L"hyperlink";
            uiaInfo.m_ItemStatus = L"hyperlink";
            uiaInfo.m_cType = UIA_HyperlinkControlTypeId;

            hyperlink = SetupHyperlinkTest();

            clickRegistration.Attach(hyperlink,
                ref new wf::TypedEventHandler<xaml_docs::Hyperlink^, xaml_docs::HyperlinkClickEventArgs^>([spClickEvent](xaml_docs::Hyperlink^ sender, xaml_docs::HyperlinkClickEventArgs^ e)
                {
                    spClickEvent->Set();
                }));
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<Automation::Patterns::InvokePatternHandler> spAutomationInvokePatternHandler;

            auto spAutomationClientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationInvokePatternHandler.Attach(new Automation::Patterns::InvokePatternHandler(spAutomationClientManager));
            spAutomationInvokePatternHandler->Invoke();
        });
        TestServices::WindowHelper->WaitForIdle();
        spClickEvent->WaitForDefault();
    }

    void HyperlinkAutomationPeerIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"hyperlink1Name";

        // Setup
        RunOnUIThread([&]()
        {
            SetupHyperlinkTest();
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spHyperlinkWithName;
            wrl::ComPtr<IUIAutomationElement> spHyperlinkWithContent;
            wrl::ComPtr<IUIAutomationElement> spHyperlinkWithNeither;
            wrl::ComPtr<IUIAutomationElement> spTextBlockOverHyperlinkWithName;
            wrl::ComPtr<IUIAutomationElement> spTextBlockOverHyperlinkWithContent;
            wrl::ComPtr<IUIAutomationElement> spTextBlockOverHyperlinkWithNeither;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spHyperlinkWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"HyperlinkAutomationPeerIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"HyperlinkAutomationPeerIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for HyperlinkWithName exists.");
            VERIFY_IS_NOT_NULL(spHyperlinkWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spHyperlinkWithName.");
            spHyperlinkWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            spUIAutomationTreeWalker->GetParentElement(spHyperlinkWithName.Get(), &spTextBlockOverHyperlinkWithName);
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBlockOverHyperlinkWithName.Get(), &spTextBlockOverHyperlinkWithContent);

            LOG_OUTPUT(L"Navigate to the second link.");
            spUIAutomationTreeWalker->GetFirstChildElement(spTextBlockOverHyperlinkWithContent.Get(), &spHyperlinkWithContent);
            VERIFY_IS_NOT_NULL(spHyperlinkWithContent);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spHyperlinkWithContent.");
            spHyperlinkWithContent->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"HyperlinkNoName", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third link.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBlockOverHyperlinkWithContent.Get(), &spTextBlockOverHyperlinkWithNeither);
            spUIAutomationTreeWalker->GetFirstChildElement(spTextBlockOverHyperlinkWithNeither.Get(), &spHyperlinkWithNeither);
            VERIFY_IS_NOT_NULL(spHyperlinkWithNeither);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spHyperlinkWithNeither.");
            spHyperlinkWithNeither->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"http://www.bing.com/", (autoVar.Storage())->bstrVal));
        });
    }

    void HyperlinkAutomationPeerIntegrationTests::VerifyIsOffscreenWhenScrolledOutOfView()
    {
        TestCleanupWrapper cleanup;

        // Setup: a RichTextBlock inside a small ScrollViewer, with a visible hyperlink at the top of the
        // paragraph and another hyperlink placed far below the viewport so that it is clipped out of view.
        // Before the fix, IsOffscreen was delegated to the containing RichTextBlock (which is on-screen), so
        // the scrolled-out hyperlink reported IsOffscreen=false while its BoundingRectangle collapsed to empty
        // - an inconsistent state that fails accessibility checks ("An on-screen element must not have a null
        // BoundingRectangle property.").
        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
                L"  x:Name='root' Background='#000000' Width='480' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"      <ScrollViewer x:Name='scrollViewer' Height='120' VerticalAlignment='Top'> \r\n"
                L"          <RichTextBlock x:Name='rtb' Width='400' FontSize='20' LineHeight='40'> \r\n"
                L"              <Paragraph> \r\n"
                L"                  <Hyperlink x:Name='visibleHyperlink' AutomationProperties.Name='visibleHyperlink' NavigateUri='http://www.bing.com'>visible link</Hyperlink> \r\n"
                L"                  <LineBreak/> Line two. <LineBreak/> Line three. <LineBreak/> Line four. \r\n"
                L"                  <LineBreak/> Line five. <LineBreak/> Line six. <LineBreak/> Line seven. \r\n"
                L"                  <LineBreak/> Line eight. <LineBreak/> Line nine. <LineBreak/> Line ten. \r\n"
                L"                  <LineBreak/> \r\n"
                L"                  <Hyperlink x:Name='offscreenHyperlink' AutomationProperties.Name='offscreenHyperlink' NavigateUri='http://www.bing.com'>some link</Hyperlink> \r\n"
                L"              </Paragraph> \r\n"
                L"          </RichTextBlock> \r\n"
                L"      </ScrollViewer> \r\n"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = root;
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        // The scrolled-out hyperlink must report IsOffscreen=true and an empty BoundingRectangle so the two
        // are consistent (this is the AI-Insights accessibility rule this fix restores).
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"offscreenHyperlink";

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spHyperlink;
                Common::AutoVariant autoVar;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spHyperlink);

                LOG_OUTPUT(L"Verifying UIA Client side node for offscreen hyperlink exists.");
                VERIFY_IS_NOT_NULL(spHyperlink);

                LOG_OUTPUT(L"Verifying IsOffscreen is true for the hyperlink scrolled out of view.");
                spHyperlink->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_ARE_EQUAL(VARIANT_TRUE, (autoVar.Storage())->boolVal);

                LOG_OUTPUT(L"Verifying BoundingRectangle is empty for the hyperlink scrolled out of view.");
                RECT boundingRect{};
                VERIFY_SUCCEEDED(spHyperlink->get_CurrentBoundingRectangle(&boundingRect));
                LOG_OUTPUT(L"(Left, Top, Right, Bottom) = (%d, %d, %d, %d).", boundingRect.left, boundingRect.top, boundingRect.right, boundingRect.bottom);
                VERIFY_IS_TRUE((boundingRect.right - boundingRect.left) <= 0);
                VERIFY_IS_TRUE((boundingRect.bottom - boundingRect.top) <= 0);
            });
        }

        // Regression guard for the new branch: a hyperlink that is actually within the viewport must remain
        // IsOffscreen=false with a non-empty BoundingRectangle.
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"visibleHyperlink";

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spHyperlink;
                Common::AutoVariant autoVar;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spHyperlink);

                LOG_OUTPUT(L"Verifying UIA Client side node for visible hyperlink exists.");
                VERIFY_IS_NOT_NULL(spHyperlink);

                LOG_OUTPUT(L"Verifying IsOffscreen is false for the visible hyperlink.");
                spHyperlink->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_ARE_EQUAL(VARIANT_FALSE, (autoVar.Storage())->boolVal);

                LOG_OUTPUT(L"Verifying BoundingRectangle is non-empty for the visible hyperlink.");
                RECT boundingRect{};
                VERIFY_SUCCEEDED(spHyperlink->get_CurrentBoundingRectangle(&boundingRect));
                LOG_OUTPUT(L"(Left, Top, Right, Bottom) = (%d, %d, %d, %d).", boundingRect.left, boundingRect.top, boundingRect.right, boundingRect.bottom);
                VERIFY_IS_TRUE((boundingRect.right - boundingRect.left) > 0);
                VERIFY_IS_TRUE((boundingRect.bottom - boundingRect.top) > 0);
            });
        }
    }

    void HyperlinkAutomationPeerIntegrationTests::VerifyIsOffscreenForMultiLineHyperlink()
    {
        TestCleanupWrapper cleanup;

        // Setup: a hyperlink whose own caption is long enough to wrap across multiple lines, placed far below
        // a small ScrollViewer's viewport so the entire multi-line range is clipped out of view. This
        // exercises the multi-rect union path in GetTextElementBoundingRect: the wrapped hyperlink produces
        // several line rects, and every one must be clipped away for their union to collapse to empty. The
        // hyperlink must then report IsOffscreen=true so it stays consistent with that empty BoundingRectangle.
        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
                L"  x:Name='root' Background='#000000' Width='480' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"      <ScrollViewer x:Name='scrollViewer' Height='120' VerticalAlignment='Top'> \r\n"
                L"          <RichTextBlock x:Name='rtb' Width='400' FontSize='20' LineHeight='40'> \r\n"
                L"              <Paragraph> \r\n"
                L"                  <Hyperlink x:Name='visibleHyperlink' AutomationProperties.Name='visibleHyperlink' NavigateUri='http://www.bing.com'>visible link</Hyperlink> \r\n"
                L"                  <LineBreak/> Line two. <LineBreak/> Line three. <LineBreak/> Line four. \r\n"
                L"                  <LineBreak/> Line five. <LineBreak/> Line six. <LineBreak/> Line seven. \r\n"
                L"                  <LineBreak/> Line eight. <LineBreak/> Line nine. <LineBreak/> Line ten. \r\n"
                L"                  <LineBreak/> \r\n"
                L"                  <Hyperlink x:Name='multiLineHyperlink' AutomationProperties.Name='multiLineHyperlink' NavigateUri='http://www.bing.com'>This is a deliberately long hyperlink caption that wraps across multiple lines within the four hundred pixel wide RichTextBlock so that its bounding rectangle spans several rows.</Hyperlink> \r\n"
                L"              </Paragraph> \r\n"
                L"          </RichTextBlock> \r\n"
                L"      </ScrollViewer> \r\n"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = root;
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        // The multi-line hyperlink scrolled entirely out of view must report IsOffscreen=true with an empty
        // BoundingRectangle (all of its line rects are clipped, so their union is empty).
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"multiLineHyperlink";

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spHyperlink;
                Common::AutoVariant autoVar;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spHyperlink);

                LOG_OUTPUT(L"Verifying UIA Client side node for multi-line hyperlink exists.");
                VERIFY_IS_NOT_NULL(spHyperlink);

                LOG_OUTPUT(L"Verifying IsOffscreen is true for the multi-line hyperlink scrolled out of view.");
                spHyperlink->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_ARE_EQUAL(VARIANT_TRUE, (autoVar.Storage())->boolVal);

                LOG_OUTPUT(L"Verifying BoundingRectangle is empty for the multi-line hyperlink scrolled out of view.");
                RECT boundingRect{};
                VERIFY_SUCCEEDED(spHyperlink->get_CurrentBoundingRectangle(&boundingRect));
                LOG_OUTPUT(L"(Left, Top, Right, Bottom) = (%d, %d, %d, %d).", boundingRect.left, boundingRect.top, boundingRect.right, boundingRect.bottom);
                VERIFY_IS_TRUE((boundingRect.right - boundingRect.left) <= 0);
                VERIFY_IS_TRUE((boundingRect.bottom - boundingRect.top) <= 0);
            });
        }
    }

    void HyperlinkAutomationPeerIntegrationTests::VerifyIsOffscreenForHyperlinkInRichTextBlockOverflow()
    {
        TestCleanupWrapper cleanup;

        // Setup: a RichTextBlock whose content overflows into a linked RichTextBlockOverflow. The master is
        // given Height='0' so its entire paragraph - including the trailing hyperlink - is laid out inside the
        // RichTextBlockOverflow, a different container than the one the hyperlink is declared in. The overflow
        // sits in a small ScrollViewer with the hyperlink positioned below the viewport, so it is clipped out
        // of view. The WI calls out RichTextBlockOverflow as a case to preserve: the hyperlink rendered in the
        // overflow must still report IsOffscreen=true with an empty BoundingRectangle.
        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
                L"  x:Name='root' Background='#000000' Width='480' Height='600' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                L"      <StackPanel Orientation='Vertical'> \r\n"
                L"          <RichTextBlock x:Name='master' Width='400' Height='0' FontSize='20' LineHeight='40' OverflowContentTarget='{Binding ElementName=overflow}'> \r\n"
                L"              <Paragraph> \r\n"
                L"                  Flowed line one. <LineBreak/> Flowed line two. <LineBreak/> Flowed line three. \r\n"
                L"                  <LineBreak/> Flowed line four. <LineBreak/> Flowed line five. <LineBreak/> Flowed line six. \r\n"
                L"                  <LineBreak/> Flowed line seven. <LineBreak/> Flowed line eight. <LineBreak/> Flowed line nine. \r\n"
                L"                  <LineBreak/> \r\n"
                L"                  <Hyperlink x:Name='overflowHyperlink' AutomationProperties.Name='overflowHyperlink' NavigateUri='http://www.bing.com'>overflow link</Hyperlink> \r\n"
                L"              </Paragraph> \r\n"
                L"          </RichTextBlock> \r\n"
                L"          <ScrollViewer x:Name='overflowScrollViewer' Height='120' VerticalAlignment='Top'> \r\n"
                L"              <RichTextBlockOverflow x:Name='overflow' Width='400'/> \r\n"
                L"          </ScrollViewer> \r\n"
                L"      </StackPanel> \r\n"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = root;
        });

        // Wait for all async activities to be done
        TestServices::WindowHelper->WaitForIdle();

        // The hyperlink laid out inside the RichTextBlockOverflow but clipped below its ScrollViewer must
        // report IsOffscreen=true with an empty BoundingRectangle.
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"overflowHyperlink";

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spHyperlink;
                Common::AutoVariant autoVar;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spHyperlink);

                LOG_OUTPUT(L"Verifying UIA Client side node for overflow hyperlink exists.");
                VERIFY_IS_NOT_NULL(spHyperlink);

                LOG_OUTPUT(L"Verifying IsOffscreen is true for the hyperlink clipped inside RichTextBlockOverflow.");
                spHyperlink->GetCurrentPropertyValue(UIA_IsOffscreenPropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_ARE_EQUAL(VARIANT_TRUE, (autoVar.Storage())->boolVal);

                LOG_OUTPUT(L"Verifying BoundingRectangle is empty for the hyperlink clipped inside RichTextBlockOverflow.");
                RECT boundingRect{};
                VERIFY_SUCCEEDED(spHyperlink->get_CurrentBoundingRectangle(&boundingRect));
                LOG_OUTPUT(L"(Left, Top, Right, Bottom) = (%d, %d, %d, %d).", boundingRect.left, boundingRect.top, boundingRect.right, boundingRect.bottom);
                VERIFY_IS_TRUE((boundingRect.right - boundingRect.left) <= 0);
                VERIFY_IS_TRUE((boundingRect.bottom - boundingRect.top) <= 0);
            });
        }
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
