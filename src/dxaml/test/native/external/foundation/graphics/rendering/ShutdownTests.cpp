// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ShutdownTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Controls::Primitives;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool ShutdownTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

// These tests are special and should not be copied for their use of WindowHelper::Initialize/ShutdownXaml. Other tests should
// put these calls in the TestSetup and TestCleanup methods, respectively. These tests exercise scenarios that the framework
// would not be able to recover from and since we don't want to force them to run in their own process (takes a lot of time),
// we'll put these test together.
void ShutdownTests::ShutdownWithOutstandingDOs()
{
    auto wh = TestServices::WindowHelper;
    wh->InitializeXaml();
    auto u = TestServices::Utilities;

    TestCleanupWrapper cleanup([wh]()
    {
        // Don't do anything, we've already shutdown.
    });


    bool unused;
    u->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableVisualDebugTags), true, &unused);

    wh->InjectMockDComp();
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = canvas;

        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        rectangle->CompositeMode = ElementCompositeMode::SourceOver;
        Canvas::SetLeft(rectangle, 50);
        Canvas::SetTop(rectangle, 50);
        canvas->Children->Append(rectangle);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Forcing Xaml shutdown");
    wh->ShutdownXaml(); // This should tear down all peers, so "canvas" should not survive past this.

    // Expect no crash.
}


void ShutdownTests::ClosePopupWithTreeReset()
{
    auto windowHelper = TestServices::WindowHelper;
    windowHelper->InitializeXaml();
    auto shutdownGuard = wil::scope_exit([windowHelper]{
       windowHelper->ShutdownXaml();
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto filePath = GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\EmptyGrid.xaml";
    Grid^ emptyGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(filePath));
    RunOnUIThread([&]()
    {
        windowHelper->WindowContent = emptyGrid;
    });

    windowHelper->WaitForIdle();

    Popup^ popup = nullptr;
    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;

    LOG_OUTPUT(L"Make & open popup.");
    RunOnUIThread([&]()
    {
        rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
        rect->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
        rect->Width = 30;
        rect->Height = 30;

        popup = ref new Popup();
        popup->Child = rect;

        // Use Canvas offsets to create a DComp TranslateTransform, which adds the popup to the DComp object registry and
        // switches it to weak ref mode. Weak ref's xref::details::control_block will assert that the ref count doesn't drop
        // below 0 on a release.
        Canvas::SetLeft(popup, 10);
        Canvas::SetTop(popup, 10);
        popup->IsOpen = true;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Null out popup while keeping it open.");
    RunOnUIThread([&]()
    {
        rect = nullptr;
        popup = nullptr;
    });

    windowHelper->WaitForIdle();
    // The popup should remain open after we null out the pointers.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Reset the visual tree like we do during shutdown. Don't crash on a popup release.");
    windowHelper->ResetVisualTree();
}

} } } } } }
