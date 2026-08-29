// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBlockTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <CustomSystemFontCollectionOverride.h>
#include <SafeEventRegistration.h>
#include <UIAutomationCore.h>
#include <WUCRenderingScopeGuard.h>
#include <ChangeDPI.h>
#include <TestComparisonGuards.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Automation::Peers;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

#ifndef E_NOT_SUPPORTED
#define E_NOT_SUPPORTED                  _HRESULT_TYPEDEF_(0x80131515L)
#endif

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {


        Platform::String^ TextBlockTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }
        bool TextBlockTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBlockTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextBlockTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Renders a few TextBlocks that are using DWriteTextLayout.
        //------------------------------------------------------------------------
        void TextBlockTests::RenderFastPathTextBlocksWUCFull()
        {
            DCompValidationHelper(L"TextBlockFastPathTests.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        void TextBlockTests::RenderFastPathTextBlocksWUCFullWithDebugDevice()
        {
            RuntimeEnabledFeatureOverride featureUseDebugD3DDevice(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableDebugD3DDevice, true);
            DCompValidationHelper(L"TextBlockFastPathTests.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        //------------------------------------------------------------------------
        // Test case: test deferred InlineCollection creation.
        //------------------------------------------------------------------------
        void TextBlockTests::TestTextBlockFlyWeightInlineCollection()
        {
            TestCleanupWrapper cleanup;

            Platform::String ^textString = "Hello world this is a TextBlock!";
            Grid^ rootGrid = nullptr;
            TextBlock^ tb = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
            RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RunOnUIThread([&]()
            {
                StackPanel^ stackPanel = ref new StackPanel();
                tb = ref new TextBlock();
                tb->FontFamily = ref new FontFamily("Segoe UI");
                tb->Text = textString;
                stackPanel->Children->Append(tb);
                rootGrid->Children->Append(stackPanel);
            });

            TestServices::WindowHelper->WaitForIdle();

            // TextBlocks should have no inlines so far.
            TestServices::Utilities->VerifyUIElementTree(L"Initial");

            RunOnUIThread([&]()
            {
                VERIFY_IS_NOT_NULL(tb->Inlines, L"The InlineCollection should be created on demand");
                VERIFY_ARE_EQUAL(tb->Inlines->Size, (unsigned int)1, L"One Run should have been added to the InlineCollection");
            });
            TestServices::WindowHelper->WaitForIdle();

            // TextBlocks should have inlines.
            TestServices::Utilities->VerifyUIElementTree(L"Created");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(tb->Text == textString);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: test  LineStackingStrategies/LineHeight/TextLineBounds
        //------------------------------------------------------------------------
        void TextBlockTests::TestTextBlockLineStackingStrategies()
        {
            DCompValidationHelper(L"TextBlockLineStackingStrategies.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison);
        }

        //------------------------------------------------------------------------
        // Test case: test TextLineBounds on the fast path.
        //------------------------------------------------------------------------
        void TextBlockTests::TestTextLineBounds()
        {
            DCompValidationHelper(L"TextBlockTextLineBounds.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison);
        }


        void TextBlockTests::MaxLineLongWords()
        {
            DCompValidationHelper(L"MaxLineCompat.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly);
        }

        void TextBlockTests::TextDecorations()
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
            RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TextDecorations.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                TextBlock^ tb = safe_cast<TextBlock^>(root->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(tb);
                RichTextBlock^rtb = safe_cast<RichTextBlock^>(root->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(rtb);
                tb->TextDecorations = wut::TextDecorations::Strikethrough;
                rtb->TextDecorations = wut::TextDecorations::Strikethrough | wut::TextDecorations::Underline;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::ChangeTextDecorations()
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
            RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            TextBlock^ tb;
            TextBlock^ tb2;
            RichTextBlock^ rtb;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' Text='12TextBlock' FontSize='30'/>"
                    L"  <TextBlock x:Name='mytbSlowPath' Text='12TextBlock' FontSize='30' Language='ar-sa'/>" // Language='ar-sa' will put it on the slow path
                    L"  <RichTextBlock x:Name='myrtb'><Paragraph><Run Text='13Some text in RTB'/><Run Text='Text in a Run' FontSize='30' /><Run Text='Some text in RTB'/></Paragraph></RichTextBlock>"
                    L"</StackPanel>"));

                tb = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                tb2 = safe_cast<TextBlock^>(rootPanel->FindName(L"mytbSlowPath"));
                rtb = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(tb);
                VERIFY_IS_NOT_NULL(rtb);
                tb->TextDecorations = wut::TextDecorations::Strikethrough;
                tb2->TextDecorations = wut::TextDecorations::Strikethrough;
                rtb->TextDecorations = wut::TextDecorations::Strikethrough;

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"st");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Removing Strikethrough TextDecorations and ensuring the textblocks re-rendered.");
            RunOnUIThread([&]()
            {
                tb->TextDecorations = wut::TextDecorations::None;
                tb2->TextDecorations = wut::TextDecorations::None;
                rtb->TextDecorations = wut::TextDecorations::None;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"none");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Adding Underline TextDecorations and ensuring the textblocks re-rendered.");
            RunOnUIThread([&]()
            {
                tb->TextDecorations = wut::TextDecorations::Underline;
                tb2->TextDecorations = wut::TextDecorations::Underline;
                rtb->TextDecorations = wut::TextDecorations::Underline;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ul");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Removing Underline TextDecorations and ensuring the textblocks re-rendered.");
            RunOnUIThread([&]()
            {
                tb->TextDecorations = wut::TextDecorations::None;
                tb2->TextDecorations = wut::TextDecorations::None;
                rtb->TextDecorations = wut::TextDecorations::None;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"none");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Adding Underline TextDecorations and ensuring the textblocks re-rendered.");
            RunOnUIThread([&]()
            {
                tb->TextDecorations = wut::TextDecorations::Underline;
                tb2->TextDecorations = wut::TextDecorations::Underline;
                rtb->TextDecorations = wut::TextDecorations::Underline;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ul");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Adding Strikethrough TextDecorations as well and ensuring the textblocks re-rendered.");
            RunOnUIThread([&]()
            {
                tb->TextDecorations = wut::TextDecorations::Strikethrough | wut::TextDecorations::Underline;
                tb2->TextDecorations = wut::TextDecorations::Strikethrough | wut::TextDecorations::Underline;
                rtb->TextDecorations = wut::TextDecorations::Strikethrough | wut::TextDecorations::Underline;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"stul");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Removing Underline TextDecorations and ensuring the textblocks re-rendered back at original Strikethrough state.");
            RunOnUIThread([&]()
            {
                tb->TextDecorations = wut::TextDecorations::Strikethrough;
                tb2->TextDecorations = wut::TextDecorations::Strikethrough;
                rtb->TextDecorations = wut::TextDecorations::Strikethrough;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"st");
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: test fast path Padding.
        //------------------------------------------------------------------------
        void TextBlockTests::FastPathTextBlockPadding()
        {
            DCompValidationHelper(L"TextBlockPaddingTests.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly);
        }

        void TextBlockTests::FastPathTextBlockPaddingWUCFull()
        {
            DCompValidationHelper(L"TextBlockPaddingTests.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        //------------------------------------------------------------------------
        // Test case: test languages that could require number substitution is on the slow path.
        //------------------------------------------------------------------------
        void TextBlockTests::NumberSubstitutionTest()
        {
            DCompValidationHelper(L"NumberSubstitutionTests.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly);
        }

        //------------------------------------------------------------------------
        // Test case: test fast path with high dpi.
        //------------------------------------------------------------------------
        void TextBlockTests::PlateauScaleTestWUCFull()
        {
            DCompValidationHelper(L"SimpleTextBlock.xaml", 1.8f, MockDComp::SurfaceComparison::ReferencedOnly, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        //------------------------------------------------------------------------
        // Test case: test fast path with high dpi.
        //------------------------------------------------------------------------
        void TextBlockTests::GridLayoutTest()
        {
            DCompValidationHelper(L"TextBlockInGrid.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly);
        }

        //------------------------------------------------------------------------
        // Test case: basic TextPointer test.
        //------------------------------------------------------------------------
        void TextBlockTests::TextPointerTest()
        {
            TestCleanupWrapper cleanup;
            TextBlock^ tb = nullptr;
            TextPointer^ tp = nullptr;
            RunOnUIThread([&]()
             {
                 tb = ref new TextBlock();
                 tb->Text = "Hello world this is a TextBlock!";
                 tp = tb->ContentStart;
                 // Deferred creates an inline collection, and it should have the text.
                 tp = tp->GetPositionAtOffset(1,LogicalDirection::Forward);
                 VERIFY_IS_NOT_NULL(tp, L"TextPointer should be able to move forward without problem");
             });
        }

        //------------------------------------------------------------------------
        // Test case: verify the AcutalWidth and AcutalHeight are including Padding
        //------------------------------------------------------------------------
        void TextBlockTests::ActualSizeIncludePadding()
        {
            TestCleanupWrapper cleanup;

            Grid^ rootGrid = nullptr;
            TextBlock^ tb = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
            RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RunOnUIThread([&]()
            {
                StackPanel^ stackPanel = ref new StackPanel();
                tb = ref new TextBlock();
                tb->FontSize = 80;
                tb->Padding =  xaml::Thickness({0, 20, 0, 20});
                tb->Text = "Hello";
                stackPanel->Children->Append(tb);
                rootGrid->Children->Append(stackPanel);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(floor(tb->ActualHeight), 146.0f);
            });
        }

        //------------------------------------------------------------------------
        // Test case: verify empty TextBlock layout.
        //------------------------------------------------------------------------
        void TextBlockTests::EmptyTextBlockMeasure()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ fastPathTB = nullptr;
            TextBlock^ slowPathTB = nullptr;

            RunOnUIThread([&]()
            {
                fastPathTB = ref new TextBlock(); //Create an empty fast path TextBlock here.
                slowPathTB = ref new TextBlock(); //Create an empty slow path TextBlock here.
                slowPathTB->CharacterSpacing = 1; // Trigger the slow path.
                fastPathTB->UseLayoutRounding = false;
                slowPathTB->UseLayoutRounding = false;

                fastPathTB->FontSize = 80;
                slowPathTB->FontSize = 80;

                fastPathTB->Padding =  xaml::Thickness({0, 20, 0, 20});
                slowPathTB->Padding =  xaml::Thickness({0, 20, 0, 20});

                VERIFY_ARE_EQUAL(fastPathTB->ActualHeight, 0.0f); // Before this TextBlock is measured, it should return 0 height..
                VERIFY_ARE_EQUAL(slowPathTB->ActualHeight, 0.0f); // Before this TextBlock is measured, it should return 0 height..

                VERIFY_ARE_EQUAL(fastPathTB->ActualWidth, 0.0f); // Before this TextBlock is measured, it should return 0 width..
                VERIFY_ARE_EQUAL(slowPathTB->ActualWidth, 0.0f); // Before this TextBlock is measured, it should return 0 width..

                fastPathTB->Measure(wf::Size(500.0f, 500.0f)); // Now measure the empty TextBlock.
                slowPathTB->Measure(wf::Size(500.0f, 500.0f)); // Now measure the empty TextBlock.

                VERIFY_ARE_EQUAL(fastPathTB->DesiredSize.Width, 0.0f); // Desired width after measure is still 0.
                VERIFY_ARE_EQUAL(slowPathTB->DesiredSize.Width, 0.0f); // Desired width after measure is still 0.

                VERIFY_ARE_EQUAL(fastPathTB->ActualWidth, fastPathTB->DesiredSize.Width); // ActualWidth == DesiredSize.Width
                VERIFY_ARE_EQUAL(slowPathTB->ActualWidth, slowPathTB->DesiredSize.Width); // ActualWidth == DesiredSize.Width

                VERIFY_ARE_EQUAL(floor(fastPathTB->DesiredSize.Height), 146.0f); // Desired height is the line height + padding.
                VERIFY_ARE_EQUAL(floor(slowPathTB->DesiredSize.Height), 146.0f); // Desired height is the line height + padding.

                VERIFY_ARE_EQUAL(floor(fastPathTB->ActualHeight), floor(fastPathTB->DesiredSize.Height)); //ActualHeight == DesiredSize.Height(Rounded).
                VERIFY_ARE_EQUAL(floor(slowPathTB->ActualHeight), floor(slowPathTB->DesiredSize.Height)); //ActualHeight == DesiredSize.Height(Rounded).
                // Set Text and re-measure.
                fastPathTB->Text = "Hello";
                slowPathTB->Text = "Hello";

                fastPathTB->Measure(wf::Size(500.0f, 500.0f)); // Now measure the empty TextBlock.
                slowPathTB->Measure(wf::Size(500.0f, 500.0f)); // Now measure the empty TextBlock.

                VERIFY_ARE_EQUAL(floor(fastPathTB->ActualHeight), 146.0f);
                VERIFY_ARE_EQUAL(floor(slowPathTB->ActualHeight), 146.0f);

                VERIFY_ARE_EQUAL(floor(fastPathTB->ActualWidth), 184.0f);
                VERIFY_ARE_EQUAL(floor(slowPathTB->ActualWidth), 184.0f);
            });
        }

        void TextBlockTests::HitTestEmptyTextBlock()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='emptyTextBlock' IsTextSelectionEnabled='True' Width='100'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"emptyTextBlock"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            wf::Point point;

            RunOnUIThread([&]() {
                auto height = textBlock->ActualHeight;

                 // Start with the point at the specified fraction into the element, and then
                // transform that to global coordinates.
                point.X = 0;
                point.Y = static_cast<float>(height * 0.5);

                auto transform = textBlock->TransformToVisual(nullptr);
                point = transform->TransformPoint(point);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(point); // Hit-test, it should not crash.
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::HitTestTightBoundTextBlock()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"mytb";
            uiaInfo.m_AutomationID = L"mytb";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' Language='ar-sa' LineStackingStrategy='BaselineToBaseline'  TextLineBounds='Tight' FontSize='15' LineHeight='20' Text='A botched promotional campaign for Rhode Island that included a video that showed Reykjavik,' TextWrapping='Wrap' Width='200' Margin='16' FontFamily='Segoe UI'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
                wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
                AutoBSTR textFromTextRange;

                POINT point = { 0, 0 };
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), L"TextIntegrationTests::VerifyRangeFromPoint: Failed in retreiving Text Pattern.");
                WEX::Common::Throw::IfNull(spTextPattern.Get(), L"TextIntegrationTests::VerifyRangeFromPoint: This TextBlock doesn't support Text Pattern which is required.");

                // Expansion test a line.
                int moved;
                VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Line, 1, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Line, 1, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"that included a video that ", textFromTextRange)); // The third line is expanded.
            });

            TestServices::WindowHelper->WaitForIdle();
        }


        void TextBlockTests::GetUIAAttributesFromEmptyTextRange()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"mytb";
            uiaInfo.m_AutomationID = L"mytb";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            POINT point = { 0, 0 };
            AutoVariant varData;
            Platform::Object^ prop;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' Text='' TextDecorations='Strikethrough'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), L"TextIntegrationTests::VerifyRangeFromPoint: Failed in retreiving Text Pattern.");
                WEX::Common::Throw::IfNull(spTextPattern.Get(), L"TextIntegrationTests::VerifyRangeFromPoint: This TextBlock doesn't support Text Pattern which is required.");

                VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsReadOnlyAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_TRUE);

                RunOnUIThread([&]()
                {
                    textBlock->Text = "Hello"; // set the text to non-null.
                });
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsReadOnlyAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_TRUE);

                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_StrikethroughStyleAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_I4);
                VERIFY_ARE_EQUAL(varData.Storage()->lVal, TextDecorationLineStyle_Single);

                RunOnUIThread([&]()
                {
                    textBlock->Language = "ar-sa"; // put the textblock on the slow path.
                });
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsReadOnlyAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_TRUE);

                RunOnUIThread([&]()
                {
                    textBlock->Language = "en-us"; // put the textblock on the fast path.
                });
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_FontNameAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BSTR);
                VERIFY_IS_TRUE(!wcscmp(L"Segoe UI", varData.Storage()->bstrVal));
            });

            TestServices::WindowHelper->WaitForIdle();

            // Verify for empty text ranges, getting attributes still return the correct values.
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                point = { 30, 10 };
                VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange)); // get the degenerate (empty) text range nearest the specified location
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsItalicAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_FALSE);
                RunOnUIThread([&]()
                {
                    textBlock->Language = "ar-sa"; // put the textblock on the slow.path.
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                point = { 30, 10 };
                VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange)); // get the degenerate (empty) text range nearest the specified location
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsItalicAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_FALSE);
            });

            // Insert an empty Run at the beginning of the InlineCollection. Set it to be Italic, then try to get the UIA_IsItalicAttributeId.
            RunOnUIThread([&]()
            {
                Run^ run = ref new Run();
                run->FontStyle = ::Windows::UI::Text::FontStyle::Italic;
                textBlock->Inlines->InsertAt(0,run);
            });
            TestServices::WindowHelper->WaitForIdle();
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                IUnknown* punkMixedAttributeValue = nullptr;
                VERIFY_SUCCEEDED(UiaGetReservedMixedAttributeValue(&punkMixedAttributeValue));
                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&spUIAutomationTextRange)); // get the document range.
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsItalicAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_UNKNOWN); // The range has mixed formats. MixedAttribute IUnknown is returned from CTextRangeAdapter::GetAttributeValue.
                VERIFY_ARE_EQUAL(varData.Storage()->punkVal, punkMixedAttributeValue);
            });

            // Set Italic on the second Run, Then try to get the UIA_IsItalicAttributeId.
            RunOnUIThread([&]()
            {
                auto run = textBlock->Inlines->GetAt(1);
                run->FontStyle = ::Windows::UI::Text::FontStyle::Italic;
            });
            TestServices::WindowHelper->WaitForIdle();
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&spUIAutomationTextRange)); // get the document range.
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsItalicAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_TRUE); // Now both runs are Italic it should return true.
            });
        }

        void TextBlockTests::FindAttributeInTextRange()
        {
            // Enable failfast on stowed exception. FindAttribute is not implemented and therefore returns E_NOTIMPL,
            // causing a crash on some SKUs where failfast is enabled. We want to avoid the crash, not the error HRESULT.
            DebugSettings^ debugSettings;
            bool origFailFastOnErrors = false;
            RunOnUIThread([&]
            {
                debugSettings = Application::Current->DebugSettings;
                origFailFastOnErrors = debugSettings->FailFastOnErrors;
                debugSettings->FailFastOnErrors = true;
            });

            TestCleanupWrapper cleanup([&]()
            {
                if (debugSettings != nullptr)
                {
                    RunOnUIThread([&]
                    {
                        debugSettings->FailFastOnErrors = origFailFastOnErrors;
                    });
                }
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"mytb";
            uiaInfo.m_AutomationID = L"mytb";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            wrl::ComPtr<IUIAutomationTextRange> spResultUIAutomationTextRange;
            AutoVariant varData;
            varData.Storage()->vt = VT_BOOL;
            varData.Storage()->boolVal = VARIANT_FALSE;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' Text='' TextDecorations='Strikethrough'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                WEX::Common::Throw::IfFailed(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), 
                    L"TextIntegrationTests::FindAttributeInTextRange: Failed in retreiving Text Pattern.");
                WEX::Common::Throw::IfNull(spTextPattern.Get(), 
                    L"TextIntegrationTests::FindAttributeInTextRange: This TextBlock doesn't support Text Pattern which is required.");

                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&spUIAutomationTextRange));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_ARE_EQUAL(spUIAutomationTextRange->FindAttribute(UIA_IsReadOnlyAttributeId, varData.Get(), TRUE, &spResultUIAutomationTextRange), E_NOTIMPL);
            });
        }

        void TextBlockTests::FindTextInTextRange()
        {
            // Enable failfast on stowed exception. FindText is not implemented and therefore returns E_NOT_SUPPORTED,
            // causing a crash on some SKUs where failfast is enabled. We want to avoid the crash, not the error HRESULT.
            DebugSettings^ debugSettings;
            bool origFailFastOnErrors = false;
            RunOnUIThread([&]
            {
                debugSettings = Application::Current->DebugSettings;
                origFailFastOnErrors = debugSettings->FailFastOnErrors;
                debugSettings->FailFastOnErrors = true;
            });

            TestCleanupWrapper cleanup([&]()
            {
                if (debugSettings != nullptr)
                {
                    RunOnUIThread([&]
                    {
                        debugSettings->FailFastOnErrors = origFailFastOnErrors;
                    });
                }
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"mytb";
            uiaInfo.m_AutomationID = L"mytb";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            wrl::ComPtr<IUIAutomationTextRange> spResultUIAutomationTextRange;
            AutoBSTR textToFind = L"FindThis";

            RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' "
                        L"            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                        L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <TextBlock x:Name='mytb' Text='' TextDecorations='Strikethrough'/>"
                        L"</StackPanel>"));
                    textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                    VERIFY_IS_NOT_NULL(textBlock);
                    xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                {
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                    auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                    spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                    VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                    WEX::Common::Throw::IfFailed(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern),
                        L"TextIntegrationTests::FindTextInTextRange: Failed in retreiving Text Pattern.");
                    WEX::Common::Throw::IfNull(spTextPattern.Get(),
                        L"TextIntegrationTests::FindTextInTextRange: This TextBlock doesn't support Text Pattern which is required.");

                    VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&spUIAutomationTextRange));
                    VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                    VERIFY_ARE_EQUAL(spUIAutomationTextRange->FindText(textToFind, FALSE, TRUE, &spResultUIAutomationTextRange), E_NOT_SUPPORTED);
                });
        }

        void TextBlockTests::UIATextChangedEvent()
        {
            TestCleanupWrapper cleanup;
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            RichTextBlock^ richTextBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"mytb";
            uiaInfo.m_AutomationID = L"mytb";
            uiaInfo.m_cType = UIA_TextControlTypeId;

            Automation::AutomationClient::UIAElementInfo uiaInfo2;
            uiaInfo2.m_Name = L"myrtb";
            uiaInfo2.m_AutomationID = L"myrtb";
            uiaInfo2.m_cType = UIA_TextControlTypeId;
            auto spEvent = std::make_shared<Event>();
            wrl::ComPtr<AutomationClient::AutomationEventHandler> spTextPatternTextChangedHandler;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' Text='' FontSize='15'/>"
                    L"  <RichTextBlock x:Name='myrtb' FontSize='15'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
                xaml_automation::AutomationProperties::SetName(richTextBlock, ref new Platform::String(uiaInfo2.m_Name));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            // Test TextBlock
            TestServices::WindowHelper->WaitForIdle();
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spTextPatternTextChangedHandler.Attach(new AutomationClient::AutomationEventHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, UIA_Text_TextChangedEventId));
                spTextPatternTextChangedHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                textBlock->Text = "Hello"; // change text
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(1, spEvent->TimesFired()); // Fired only once.
            spEvent->Reset();

            RunOnUIThread([&]()
            {
                textBlock->Text = "Hello Again"; // change text again
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(1, spEvent->TimesFired()); // Fired only once.
            spEvent->Reset();

            // Insert a Run into the Inlines collection.
            Run^ run = nullptr;
            RunOnUIThread([&]()
            {
                run = ref new Run();
                run->Text = "Some text in run";
                textBlock->Inlines->InsertAt(0,run);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(1, spEvent->TimesFired()); // Fired only once.
            spEvent->Reset();

            RunOnUIThread([&]()
            {
                run->Text = "Some other text in run"; // change text in the run.
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(1, spEvent->TimesFired()); // Fired only once.
            spEvent->Reset();

            // Test RichTextBlock
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spTextPatternTextChangedHandler->RemoveEventHandler();
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo2);
                spTextPatternTextChangedHandler.Attach(new AutomationClient::AutomationEventHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, UIA_Text_TextChangedEventId));
                spTextPatternTextChangedHandler->AttachEventHandler();
            });

            RunOnUIThread([&]()
            {
                 Paragraph^ p = ref new Paragraph();
                 richTextBlock->Blocks->Append(p); // Paragraph added to the Blocks, fire once.
                 Run^ run = ref new Run();
                 run->Text = "Some text in run";
                 p->Inlines->InsertAt(0,run); // Inlines collection created in Paragraph, fire once; Run added to Inlines collection, fire once.
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(3, spEvent->TimesFired()); // Fired 3 times, total.
            spEvent->Reset();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                spTextPatternTextChangedHandler->RemoveEventHandler();
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: test the DebugSetting IsTextPerformanceVisualizationEnabled
        //------------------------------------------------------------------------
        void TextBlockTests::IsTextPerformanceVisualizationEnabled()
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
                Application::Current->DebugSettings->IsTextPerformanceVisualizationEnabled = true;
            });
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "SimpleTextBlock.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

            RunOnUIThread([&]()
            {
                Application::Current->DebugSettings->IsTextPerformanceVisualizationEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
        }

        void TextBlockTests::SelectionChangedEvent()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;

            auto textBlockSelectionChangedEvent = std::make_shared<Event>();
            auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, SelectionChanged);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                TestServices::WindowHelper->WindowContent = root;

                textSelectionChangedRegistration.Attach(
                    textBlock,
                    ref new xaml::RoutedEventHandler(
                    [textBlockSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    textBlockSelectionChangedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Select some text by click and dragging
            TestServices::InputHelper->DragToCenter(textBlock, -100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);

            // Verify some text was selected
            textBlockSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(textBlockSelectionChangedEvent->HasFired());

            RunOnUIThread([&]()
            {
                // take richTextBlock of the tree
                UINT index;
                root->Children->IndexOf(textBlock, &index);
                root->Children->RemoveAt(index);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                root->Children->Append(textBlock);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBlock->SelectAll();
            });
            // Verify some text was selected
            textBlockSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(textBlockSelectionChangedEvent->HasFired());
        }

        void TextBlockTests::SelectionTextUpdate()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            Platform::String ^strToChange = "Simple Text";

            auto textBlockGotFocusEvent = std::make_shared<Event>();
            auto textBlockLeftGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, GotFocus);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                TestServices::WindowHelper->WindowContent = root;

                textBlockLeftGotFocusRegistration.Attach(
                    textBlock,
                    ref new xaml::RoutedEventHandler(
                        [textBlockGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    textBlockGotFocusEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                textBlock->Focus(FocusState::Pointer);
            });

            textBlockGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBlock->Text = strToChange;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->Text == strToChange);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::DisableTextSelection()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            Platform::String ^strToChange = "Simple Text";

            auto textBlockGotFocusEvent = std::make_shared<Event>();
            auto textBlockLeftGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, GotFocus);
            auto textBlockSelectionChangedEvent = std::make_shared<Event>();
            auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, SelectionChanged);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                TestServices::WindowHelper->WindowContent = root;

                textBlockLeftGotFocusRegistration.Attach(
                    textBlock,
                    ref new xaml::RoutedEventHandler(
                        [textBlockGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    textBlockGotFocusEvent->Set();
                }));

                textSelectionChangedRegistration.Attach(
                    textBlock,
                    ref new xaml::RoutedEventHandler(
                        [textBlockSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    textBlockSelectionChangedEvent->Set();
                }));

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(textBlock);

            textBlockGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            textBlockSelectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                textBlock->IsTextSelectionEnabled = false;
                textBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            // Verify no text was selected
            textBlockSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(200));
            VERIFY_IS_TRUE(!textBlockSelectionChangedEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() // enable test selection
            {
                textBlock->IsTextSelectionEnabled = true;
                textBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            // Verify text is selected
            textBlockSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            textBlockSelectionChangedEvent->Reset();
            RunOnUIThread([&]() // disable selection again, this time without grippers active
            {
                textBlock->IsTextSelectionEnabled = false;
                textBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            // Verify no text was selected
            textBlockSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(200));
            VERIFY_IS_TRUE(!textBlockSelectionChangedEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::TextUpdatesWithFocus()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            TextBlock^ textBlock = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <TextBlock x:Name='mytb' Text='Test Text' IsTextSelectionEnabled='True'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus to text block and selecting text");
            RunOnUIThread([&]()
            {
                TextPointer^ start = textBlock->ContentStart;
                TextPointer^ end = textBlock->ContentEnd;
                textBlock->Focus(FocusState::Pointer);
                textBlock->Select(start, end);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Change the text formatting to get a new arrange
            LOG_OUTPUT(L"Changing Character Spacing to 2");
            RunOnUIThread([&]()
            {
                textBlock->CharacterSpacing = 2;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FormatChange");

            // First addition of text will verify that we render correctly if the selection changes
            // since adding text will collapse the selection to the start.
            LOG_OUTPUT(L"Changing text to: Test Text 2");
            RunOnUIThread([&]()
            {
                textBlock->Text = L"Test Text 2";
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SelectionChange");

            // Second addition will verify that we render correctly if the selection doesn't change
            // since it will still be collapsed at the start from the previous add.
            LOG_OUTPUT(L"Changing text to: Test Text 3");
            RunOnUIThread([&]()
            {
                textBlock->Text = L"Test Text 3";
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NoSelectionChange");
        }

        void TextBlockTests::UpdateFontScale()
        {
            DCompValidationHelper(L"TextBlockWithPopup.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison, 3.5f);
        }

        void TextBlockTests::LineHeightScaleWithFontScale()
        {
            DCompValidationHelper(L"TextBlockLineScale.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison, 3.5f);
        }

        void TextBlockTests::MixedFontFamily()
        {
            DCompValidationHelper(L"MixedFontFamily.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison);
        }

        void TextBlockTests::Typography()
        {
            DCompValidationHelper(L"TextBlockTypography.xaml", 1.0f, MockDComp::SurfaceComparison::NoComparison);
        }

        void TextBlockTests::TextBlockSelectionWUCFull()
        {
            TextBlockSelection(DCompRendering::WUCCompleteSynchronousCompTree);
        }

        void TextBlockTests::TextBlockSelection(DCompRendering dcompRendering)
        {
            TextBlock^ textBlock = nullptr;
            TextPointer^ selectionStartPointer = nullptr;
            TextPointer^ selectionEndPointer = nullptr;
            Platform::String ^selectedText = "abc";

            WUCRenderingScopeGuard guard(dcompRendering, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockSelectionTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            // Scenario 1: Tap a deafult Textblock and select a word.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb1"));
                VERIFY_IS_NOT_NULL(textBlock);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(textBlock);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->SelectedText->Equals(selectedText));
            });

            // Scenario 2: Tap a BitmapCache Textblock and select a word.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb2"));
                VERIFY_IS_NOT_NULL(textBlock);
                textBlock->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(textBlock);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->SelectedText->Equals(selectedText));
            });

            // Scenario 3: Tap a RTL TextBlock and select a word.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb3"));
                VERIFY_IS_NOT_NULL(textBlock);
                textBlock->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(textBlock);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->SelectedText->Equals(selectedText));
            });

            // Scenario 4: Select all text in an Arabic TextBlock.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb4"));
                VERIFY_IS_NOT_NULL(textBlock);
                textBlock->Focus(FocusState::Pointer);
                textBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                selectedText = L"\u0634\u0644\u0624\u064a\u062b\u0628\u0644\u062a\u0647\u062a\u0646\u0645\u0629\u0649";
                VERIFY_IS_TRUE(textBlock->SelectedText->Equals(selectedText));
            });

            // Scenario 5: Drag select a RTL TextBlock with Bidi content.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb5"));
                VERIFY_IS_NOT_NULL(textBlock);
                textBlock->Focus(FocusState::Pointer);
            });

            // Select some text by click and dragging
            TestServices::InputHelper->DragFromCenter(textBlock, 100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
             {
                 selectedText = L"\u0634\u0644\u0624\u064a\u062b";
                 VERIFY_IS_TRUE(textBlock->SelectedText->Equals(selectedText));
             });

            // Scenario 6: Use TextPointer to select a TextBlock that contains clusters.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb6"));
                VERIFY_IS_NOT_NULL(textBlock);
                textBlock->Focus(FocusState::Pointer);
                selectionStartPointer = textBlock->ContentStart;
                selectionEndPointer = textBlock->ContentStart;
                selectionEndPointer = selectionEndPointer->GetPositionAtOffset(2,LogicalDirection::Forward);
                VERIFY_IS_NOT_NULL(selectionEndPointer, L"TextPointer should be able to move forward without problem");
                textBlock->Select(selectionStartPointer, selectionEndPointer); // Select the first cluster.
            });
            TestServices::WindowHelper->WaitForIdle();
            // Verify the correct cluster is selected.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "cluster");
            TestServices::WindowHelper->WaitForIdle();

            // Scenario 7: Select all text in a multi-line wrapping TextBlock.
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"tb7"));
                VERIFY_IS_NOT_NULL(textBlock);
                textBlock->Focus(FocusState::Pointer);
                textBlock->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();
            // Verify the selection rendering is correct.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "wrap");
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::HyperlinkSelection()
        {
            TestCleanupWrapper cleanup;
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Margin='0,25,0,0' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock IsTextSelectionEnabled='True' x:Name='mytb'><Hyperlink>This is a Hyperlink</Hyperlink></TextBlock>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBlock->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();

            // Select some text by click and dragging
            TestServices::InputHelper->DragFromCenter(textBlock, 100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
             {
                 Platform::String ^selectedText = "Hyperlink";
                 VERIFY_IS_TRUE(textBlock->SelectedText->Equals(selectedText));
             });
        }

        void TextBlockTests::ImageFontsWUCFull()
        {
            // Emoji / color-font glyphs rasterize a hair differently between OS builds (no JPEG here),
            // so allow the small per-channel tolerance.
            ImageCompareToleranceGuard tolerance(RENDER_COMPARE_TOLERANCE_SMALL);
            DCompValidationHelper(L"ImageFonts.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        void TextBlockTests::TilingWUCFull()
        {
            DCompValidationHelper(L"TextBlockTiling.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        void TextBlockTests::BatchingWUCFull()
        {
            DCompValidationHelper(L"TextBlockBatching.xaml", 1.0f, MockDComp::SurfaceComparison::ReferencedOnly, 1.0f, DCompRendering::WUCCompleteSynchronousCompTree);
        }

        //------------------------------------------------------------------------
        // Test case: test invalid WSS settings.
        //------------------------------------------------------------------------
        void TextBlockTests::InvalidWSS()
        {
            TestCleanupWrapper cleanup;

            Platform::String ^textString = "Hello";
            Grid^ rootGrid = nullptr;
            TextBlock^ tb = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
            RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RunOnUIThread([&]()
            {
                StackPanel^ stackPanel = ref new StackPanel();
                tb = ref new TextBlock();
                tb->Text = textString;
                tb->FontStretch = static_cast<::Windows::UI::Text::FontStretch>(10);
                ::Windows::UI::Text::FontWeight weight;
                weight.Weight = 2000;
                tb->FontWeight = weight;
                tb->FontStyle = static_cast<::Windows::UI::Text::FontStyle>(3);
                stackPanel->Children->Append(tb);
                rootGrid->Children->Append(stackPanel);
            });

            // App should not crash.
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::StartEndHorizontalTextAlignment()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;

                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                // HorizontalTextAlignment::Start should be the same as TextAlignment::Left
                textBlock->HorizontalTextAlignment = TextAlignment::Start;
                VERIFY_IS_TRUE(textBlock->TextAlignment == TextAlignment::Left);

                // HorizontalTextAlignment::End should be the same as TextAlignment::Right
                textBlock->HorizontalTextAlignment = TextAlignment::End;
                VERIFY_IS_TRUE(textBlock->TextAlignment == TextAlignment::Right);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::NoContextFlyoutWhenSelectionIsDisabled()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            xaml_primitives::FlyoutBase^ flyoutBase = nullptr;
            auto contextFlyoutOpenedEvent = std::make_shared<Event>();
            auto contextFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='textBlock' FontSize='32' IsTextSelectionEnabled='False'><Hyperlink FontSize='15'>This is a hyperlink</Hyperlink></TextBlock>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);
                flyoutBase = textBlock->ContextFlyout;
                VERIFY_IS_NOT_NULL(flyoutBase);

                contextFlyoutOpenedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout opened...");
                    contextFlyoutOpenedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Make sure TextBlock has the focus...");
            RunOnUIThread([&]()
            {
                textBlock->Focus(FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press menu key and check if ContextFlyout can be opened...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_apps#$u$_apps");
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());

            LOG_OUTPUT(L"Hold and check if ContextFlyout can be opened...");
            TestServices::InputHelper->Hold(textBlock);
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::WordBreakerSelection()
        {
            TestCleanupWrapper cleanup;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            TextBlock^ textBlock = nullptr;

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "WordBreakerTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
                textBlock = safe_cast<TextBlock^>(root->FindName("textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(textBlock, 0.3f, 0.5f);
            TestServices::InputHelper->Tap(textBlock, 0.3f, 0.5f);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->SelectedText->Equals("!!!"));
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(textBlock, 0.25f, 0.5f);
            TestServices::InputHelper->Tap(textBlock, 0.25f, 0.5f);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->SelectedText->Equals("text"));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::TextBlockWordBreakerNavigation()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TextBlock";

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "WordBreakerTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
                textBlock = safe_cast<TextBlock^>(root->FindName("textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
                AutoBSTR textFromTextRange;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                // Get TextPattern
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern));
                VERIFY_IS_NOT_NULL(spTextPattern.Get());

                // Get DocumentRange
                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"This is some text. !!! In a text block?\r\nMr. Smith-Jones uses a lot of punctuation -- in his to-do lists.", textFromTextRange));

                // Move the end point to match the start
                VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByRange(TextPatternRangeEndpoint_End, spUIAutomationTextRange.Get(), TextPatternRangeEndpoint_Start));
                // Move the end point to the end of the word
                BOOL moved = FALSE;
                VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Word, 1, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"This ", textFromTextRange));

                // Move both
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 5, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"In ", textFromTextRange));

                // Keep moving around, making sure we are where we expect
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 4, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"\r\n", textFromTextRange));

                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 2, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"Smith-Jones ", textFromTextRange));

                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 9, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"to-do ", textFromTextRange));

                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"lists.", textFromTextRange));

                // Go backwards almost all the way
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, -21, &moved));
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"is ", textFromTextRange));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::IsTextTrimmedFastPath()
        {
            IsTextTrimmedPropertyAndEventHelper(L"textBlock");
            IsTextTrimmedBindingHelper(L"textBlock");
        }

        void TextBlockTests::IsTextTrimmedSlowPath()
        {
            IsTextTrimmedPropertyAndEventHelper(L"textBlockSlowPath");
            IsTextTrimmedBindingHelper(L"textBlockSlowPath");
        }

        void TextBlockTests::RasterizationScale()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            RunOnUIThread([&]()
            {
                TextBlock^ textBlock = ref new TextBlock();
                textBlock->FontSize = 40;
                textBlock->Text = "Hello";

                TextBlock^ complexTextBlock = ref new TextBlock();
                complexTextBlock->Text = "1234567890-1234567890-1234567890-1234567890-1234567890";
                complexTextBlock->FontSize = 30;
                complexTextBlock->Width = 200;
                complexTextBlock->TextWrapping = TextWrapping::Wrap;
                complexTextBlock->LineHeight = 10;

                StackPanel^ root = ref new StackPanel();
                root->RasterizationScale = 2;
                root->Children->Append(textBlock);
                root->Children->Append(complexTextBlock);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        void TextBlockTests::IsTextTrimmedPropertyAndEventHelper(Platform::String^ textBlockName)
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            Platform::String^ originalText = nullptr;
            Platform::String^ longerText = "This is text This is text This is text";
            double originalWidth = 0;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            auto textBlockIsTextTrimmedChangedEvent = std::make_shared<Event>();
            auto textBlockIsTextTrimmedChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, IsTextTrimmedChanged);

            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(textBlockName));
                VERIFY_IS_NOT_NULL(textBlock);

                // Turn on text trimming
                textBlock->TextTrimming = TextTrimming::CharacterEllipsis;
                TestServices::WindowHelper->WindowContent = root;

                textBlockIsTextTrimmedChangedRegistration.Attach(
                    textBlock,
                    ref new ::Windows::Foundation::TypedEventHandler<TextBlock^, IsTextTrimmedChangedEventArgs^>(
                        [textBlockIsTextTrimmedChangedEvent](Platform::Object^ sender, xaml_controls::IsTextTrimmedChangedEventArgs^ args)
                {
                    // Event has been triggered
                    textBlockIsTextTrimmedChangedEvent->Set();
                }));

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Original text should not be trimmed
                VERIFY_IS_TRUE(!(textBlock->IsTextTrimmed));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming state to true
                originalWidth = textBlock->Width;
                textBlock->Width = 100.0;
                originalText = textBlock->Text;
                if (textBlockName->Equals(L"textBlock"))
                {
                    textBlock->Text = longerText;
                }
                else
                {
                    textBlock->Inlines->Clear();
                    Run^ runText = ref new Run();
                    runText->Text = longerText;
                    textBlock->Inlines->Append(runText);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            // Verify the event fired the first time
            textBlockIsTextTrimmedChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(textBlockIsTextTrimmedChangedEvent->HasFired());
            textBlockIsTextTrimmedChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                // Text now should be trimmed
                VERIFY_IS_TRUE(textBlock->IsTextTrimmed);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming back to false
                if (textBlockName->Equals(L"textBlock"))
                {
                    textBlock->Text = originalText;
                }
                else
                {
                    textBlock->Inlines->Clear();
                    Run^ runText = ref new Run();
                    runText->Text = originalText;
                    textBlock->Inlines->Append(runText);
                }
                textBlock->Width = originalWidth;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Text should not be trimmed again
                VERIFY_IS_TRUE(!(textBlock->IsTextTrimmed));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Verify the event fired again
            textBlockIsTextTrimmedChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(textBlockIsTextTrimmedChangedEvent->HasFired());
        }

        void TextBlockTests::IsTextTrimmedBindingHelper(Platform::String^ textBlockName)
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;

            Platform::String^ originalText = nullptr;
            Platform::String^ longerText = "This is text This is text This is text";
            double originalWidth = 0;
            Microsoft::UI::Xaml::Data::Binding^ binding = nullptr;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(textBlockName));
                VERIFY_IS_NOT_NULL(textBlock);

                // Turn on text trimming
                textBlock->TextTrimming = TextTrimming::CharacterEllipsis;

                // Bind IsTextSelectionEnabled to IsTextTrimmed
                binding = ref new Microsoft::UI::Xaml::Data::Binding();
                binding->Path = ref new PropertyPath(L"IsTextTrimmed");
                binding->ElementName = textBlockName;
                textBlock->SetBinding(TextBlock::IsTextSelectionEnabledProperty, binding);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Original text should not be trimmed
                VERIFY_IS_TRUE(!(textBlock->IsTextTrimmed));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming state to true
                originalWidth = textBlock->Width;
                textBlock->Width = 100.0;
                originalText = textBlock->Text;
                if (textBlockName->Equals(L"textBlock"))
                {
                    textBlock->Text = longerText;
                }
                else
                {
                    textBlock->Inlines->Clear();
                    Run^ runText = ref new Run();
                    runText->Text = longerText;
                    textBlock->Inlines->Append(runText);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Text now should be trimmed
                VERIFY_IS_TRUE(textBlock->IsTextTrimmed);
                // IsTextSelectionEnabled should have changed via binding
                VERIFY_IS_TRUE(textBlock->IsTextSelectionEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming back to false
                if (textBlockName->Equals(L"textBlock"))
                {
                    textBlock->Text = originalText;
                }
                else
                {
                    textBlock->Inlines->Clear();
                    Run^ runText = ref new Run();
                    runText->Text = originalText;
                    textBlock->Inlines->Append(runText);
                }
                textBlock->Width = originalWidth;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Text should not be trimmed again
                VERIFY_IS_TRUE(!(textBlock->IsTextTrimmed));
                // IsTextSelectionEnabled should have changed via binding
                VERIFY_IS_TRUE(!(textBlock->IsTextSelectionEnabled));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBlockTests::DCompValidationHelper(
            Platform::String^ filename,
            float scale,
            MockDComp::SurfaceComparison comparisonMode,
            float fontScale,
            DCompRendering dcompRendering)
        {
            // Clear out the current window content before injecting MockDComp, to
            // MockDComp doesn't capture an image for anything currently in the content,
            // since that will interfere with the expected surface counts.
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
            RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

            WUCRenderingScopeGuard guard(dcompRendering, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(800, 600), scale);

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + filename));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            FontScaleOverride fontScaleOverride(fontScale);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Popup^ popup = safe_cast<Popup^>(root->FindName(L"myPopup"));
                if (popup)
                {
                    popup->IsOpen = true;
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(comparisonMode);

            TestServices::WindowHelper->WaitForIdle();
        }
    } }
} } } }

