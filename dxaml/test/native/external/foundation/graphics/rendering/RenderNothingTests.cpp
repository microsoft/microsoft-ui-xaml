// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RenderNothingTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool RenderNothingTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool RenderNothingTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool RenderNothingTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void RenderNothingTests::RenderNoContent()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    RunOnUIThread([&]()
    {
        // We need WindowContentManager::SetContent to call Application_SetVisualRoot to fire the application startup complete
        // event, but SetContent will early exit if the content didn't change. Since the default content is null, set something
        // else and null it out again.
        Canvas^ canvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = canvas;

        TestServices::WindowHelper->WindowContent = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
}

void RenderNothingTests::RenderNoVisualTree()
{
    auto windowHelper = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([windowHelper]()
    {
        windowHelper->ResetWindowContentAndWaitForIdle();
    });

    windowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    RunOnUIThread([&]()
    {
        // We need WindowContentManager::SetContent to call Application_SetVisualRoot to fire the application startup complete
        // event, otherwise the test hangs at startup.
        Canvas^ canvas = ref new Canvas();
        windowHelper->WindowContent = canvas;
    });

    windowHelper->WaitForIdle();

    LOG_OUTPUT(L"Reset the visual tree. Don't crash.");
    windowHelper->ResetVisualTree();
}

} } } } } }
