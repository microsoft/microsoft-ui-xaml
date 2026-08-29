// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RtblHasContentOverflow.h"
#include <XamlTailored.h>
#include "FileLoader.h"
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool RtblHasContentOverflow::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool RtblHasContentOverflow::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ RtblHasContentOverflow::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }

        //------------------------------------------------------------------------
        // Test case: Validates RTBl HasOverflowContent property
        //------------------------------------------------------------------------
        void RtblHasContentOverflow::ValidateOverflows()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(500, 800);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichTextBlock^ rtb2 = nullptr;
            xaml_controls::RichTextBlock^ rtb1 = nullptr;
            xaml_controls::RichTextBlock^ rtb3 = nullptr;
            xaml_controls::RichTextBlock^ rtb4 = nullptr;
            xaml_controls::RichTextBlock^ rtb5 = nullptr;
            xaml_controls::RichTextBlock^ rtb6 = nullptr;
            xaml_controls::RichTextBlock^ rtb7 = nullptr;
            xaml_controls::RichTextBlock^ rtb8 = nullptr;
            xaml_controls::RichTextBlock^ rtb9 = nullptr;
            xaml_controls::RichTextBlock^ rtb10 = nullptr;
            xaml_controls::RichTextBlock^ rtb11 = nullptr;
            xaml_controls::RichTextBlock^ rtb12 = nullptr;
            xaml_controls::RichTextBlock^ rtb13 = nullptr;

            xaml_controls::RichTextBlockOverflow^ rtblo1 = nullptr;
            xaml_controls::RichTextBlockOverflow^ rtblo2 = nullptr;
            xaml_controls::RichTextBlockOverflow^ rtblo3 = nullptr;
            xaml_controls::RichTextBlockOverflow^ rtblo4 = nullptr;
            xaml_controls::RichTextBlockOverflow^ rtblo5 = nullptr;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"RtblHasContentOverflow.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rtb1 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb1"));
                VERIFY_IS_NOT_NULL(rtb1);
                rtb2 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb2"));
                VERIFY_IS_NOT_NULL(rtb2);
                rtb3 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb3"));
                VERIFY_IS_NOT_NULL(rtb3);
                rtb4 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb4"));
                VERIFY_IS_NOT_NULL(rtb4);
                rtb5 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb5"));
                VERIFY_IS_NOT_NULL(rtb5);
                rtb6 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb6"));
                VERIFY_IS_NOT_NULL(rtb6);
                rtb7 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb7"));
                VERIFY_IS_NOT_NULL(rtb7);
                rtb8 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb8"));
                VERIFY_IS_NOT_NULL(rtb8);
                rtb9 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb9"));
                VERIFY_IS_NOT_NULL(rtb9);
                rtb10 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb10"));
                VERIFY_IS_NOT_NULL(rtb10);
                rtb11 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb11"));
                VERIFY_IS_NOT_NULL(rtb11);
                rtb12 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb12"));
                VERIFY_IS_NOT_NULL(rtb12);
                rtb13 = safe_cast<xaml_controls::RichTextBlock^>(rootStackPanel->FindName("rtb13"));
                VERIFY_IS_NOT_NULL(rtb13);

                rtblo1 = safe_cast<xaml_controls::RichTextBlockOverflow^>(rootStackPanel->FindName("rtblo1"));
                VERIFY_IS_NOT_NULL(rtblo1);
                rtblo2 = safe_cast<xaml_controls::RichTextBlockOverflow^>(rootStackPanel->FindName("rtblo2"));
                VERIFY_IS_NOT_NULL(rtblo2);
                rtblo3 = safe_cast<xaml_controls::RichTextBlockOverflow^>(rootStackPanel->FindName("rtblo3"));
                VERIFY_IS_NOT_NULL(rtblo3);
                rtblo4 = safe_cast<xaml_controls::RichTextBlockOverflow^>(rootStackPanel->FindName("rtblo4"));
                VERIFY_IS_NOT_NULL(rtblo4);
                rtblo5 = safe_cast<xaml_controls::RichTextBlockOverflow^>(rootStackPanel->FindName("rtblo5"));
                VERIFY_IS_NOT_NULL(rtblo5);

                VERIFY_IS_TRUE(!rtb1->HasOverflowContent, L"Expect rtb1->HasOverflowContent is false.");
                VERIFY_IS_TRUE(rtb2->HasOverflowContent, L"Expect rtb2->HasOverflowContent is true.");
                VERIFY_IS_TRUE(!rtb3->HasOverflowContent, L"Expect rtb3->HasOverflowContent is false.");
                VERIFY_IS_TRUE(rtb4->HasOverflowContent, L"Expect rtb4->HasOverflowContent is true.");

                // multiline test
                VERIFY_IS_TRUE(!rtb5->HasOverflowContent, L"Expect rtb5->HasOverflowContent is false.");
                VERIFY_IS_TRUE(!rtb6->HasOverflowContent, L"Expect rtb6 HasOverflowContent is false.");

                // multiparagraph test
                VERIFY_IS_TRUE(rtb7->HasOverflowContent, L"Expect rtb7->HasOverflowContent is true.");
                VERIFY_IS_TRUE(rtb8->HasOverflowContent, L"Expect rtb8->HasOverflowContent is true.");

                // richtextblockoverflow test
                VERIFY_IS_TRUE(rtb9->HasOverflowContent, L"Expect rtb9->HasOverflowContent is true.");
                VERIFY_IS_TRUE(!rtblo1->HasOverflowContent, L"Expect rtblo1->HasOverflowContent is false.");
                VERIFY_IS_TRUE(rtb10->HasOverflowContent, L"Expect rtb10->HasOverflowContent is true.");
                VERIFY_IS_TRUE(!rtblo2->HasOverflowContent, L"Expect rtblo2->HasOverflowContent is false.");

                // rtblo with multiparagaph test
                VERIFY_IS_TRUE(rtb11->HasOverflowContent, L"Expect rtb11->HasOverflowContent is true.");
                VERIFY_IS_TRUE(!rtblo3->HasOverflowContent, L"Expect rtblo3->HasOverflowContent is false.");
                VERIFY_IS_TRUE(rtb12->HasOverflowContent, L"Expect rtb12->HasOverflowContent is true.");
                VERIFY_IS_TRUE(!rtblo4->HasOverflowContent, L"Expect rtblo4->HasOverflowContent is false.");
                VERIFY_IS_TRUE(rtb13->HasOverflowContent, L"Expect rtb13->HasOverflowContent is true.");
                VERIFY_IS_TRUE(rtblo5->HasOverflowContent, L"Expect rtblo5->HasOverflowContent is true.");
            });

        }
    } }
} } } }