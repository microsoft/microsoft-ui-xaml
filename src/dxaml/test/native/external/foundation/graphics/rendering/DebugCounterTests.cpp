// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DebugCounterTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>

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

bool DebugCounterTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool DebugCounterTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool DebugCounterTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void DebugCounterTests::DebugCounters()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_shapes::Rectangle^ rectangle;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = canvas;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        canvas->Children->Append(rectangle);

        Microsoft::UI::Xaml::Application::Current->DebugSettings->EnableFrameRateCounter = true;
    });
    wh->WaitForIdle();

    // We will turn this off and then back on to ensure that we properly cleanup and/or reuse the visuals as appropriate
    RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Application::Current->DebugSettings->EnableFrameRateCounter = false;
        });
    wh->WaitForIdle();

    RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Application::Current->DebugSettings->EnableFrameRateCounter = true;
        });
    wh->WaitForIdle();

    // Don't do surface comparison - there's no guarantee what the actual frame rates are. We'll just verify that the frame rate primitive is there.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

} } } } } }
