// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TransformToVisualTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>
#include <ChangeDpi.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Foundation;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ TransformToVisualTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
}

bool TransformToVisualTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool TransformToVisualTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool TransformToVisualTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void TransformToVisualTests::TransformToVisual_Siblings()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ child1;
    Canvas^ child2;
    RunOnUIThread([&]()
    {
        child1 = ref new Canvas();
        Canvas::SetLeft(child1, 50);

        child2 = ref new Canvas();
        Canvas::SetTop(child2, 50);

        Canvas^ root = ref new Canvas();
        root->Children->Append(child1);
        root->Children->Append(child2);

        wh->WindowContent = root;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        const auto& transform = child1->TransformToVisual(child2);

        wf::Point origin(0,0);
        wf::Point transformed = transform->TransformPoint(origin);

        VERIFY_ARE_EQUAL(50, transformed.X);
        VERIFY_ARE_EQUAL(-50, transformed.Y);
    });
    wh->WaitForIdle();
}

void TransformToVisualTests::BasicTestOnHighDPI()
{
    TestCleanupWrapper cleanup;
    const auto& wh = TestServices::WindowHelper;

    // In some environments, we don't register for all the system events (e.g. window size changes) until we
    // we render our first frame, so make sure we render one before we change the DPI which will wait for
    // the dpi change notifications.
    RunOnUIThread([&]()
    {
        wh->WindowContent = ref new Grid();
    });
    wh->WaitForIdle();

    ChangeDPI changeDPI;    // Runs the test with DPI scale factor of 125%
    wh->WaitForIdle();

    Canvas^ child;
    RunOnUIThread([&]()
    {
        child = ref new Canvas();
        Canvas::SetLeft(child, 50);
        Canvas::SetTop(child, 50);

        Canvas^ root = ref new Canvas();
        root->Children->Append(child);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        const auto& transform = child->TransformToVisual(nullptr);

        wf::Point origin(0,0);
        wf::Point transformed = transform->TransformPoint(origin);

        VERIFY_ARE_EQUAL(50, transformed.X);
        VERIFY_ARE_EQUAL(50, transformed.Y);
    });
}

} } } } } }
