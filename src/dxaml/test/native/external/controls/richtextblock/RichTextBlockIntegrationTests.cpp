// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichTextBlockIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TextControlHelper.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichTextBlock {

    bool RichTextBlockIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RichTextBlockIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RichTextBlockIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void RichTextBlockIntegrationTests::VerifySelectingTextWithTouchShowsSelectionFlyout()
    {
        TextControlHelper::VerifySelectingTextWithTouchShowsSelectionFlyout<xaml_controls::RichTextBlock>(
            [](xaml_controls::RichTextBlock^ richTextBlock)
            {
                auto run = ref new xaml_docs::Run();
                run->Text = "aaaaaaaaaa";

                auto paragraph = ref new xaml_docs::Paragraph();
                paragraph->Inlines->Append(run);

                richTextBlock->Blocks->Append(paragraph);
                richTextBlock->IsTextSelectionEnabled = true;
            });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::RichTextBlock
