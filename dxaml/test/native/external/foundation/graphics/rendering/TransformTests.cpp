// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TransformTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

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

Platform::String^ TransformTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
}

bool TransformTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool TransformTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool TransformTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void TransformTests::TransformGroupDirtyFlags()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    auto root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"EmptyGrid.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = root;
    });

    TestServices::WindowHelper->WaitForIdle();

    TransformGroup^ tg;
    Point out;

    RunOnUIThread([&]()
    {
        tg = ref new TransformGroup();
        auto tt = ref new TranslateTransform();
        tt->X = 10;
        tg->Children->Append(tt);
        out = tg->TransformPoint(Point(0, 0));
    });
    VERIFY_ARE_EQUAL(10, out.X);

    RunOnUIThread([&]()
    {
        auto tt = ref new TranslateTransform();
        tt->X = 20;
        tg->Children->Append(tt);
        out = tg->TransformPoint(Point(0, 0));
    });
    VERIFY_ARE_EQUAL(30, out.X);

    RunOnUIThread([&]()
    {
        tg->Children->RemoveAt(0);
        out = tg->TransformPoint(Point(0, 0));
    });
    VERIFY_ARE_EQUAL(20, out.X);

    RunOnUIThread([&]()
    {
        tg->Children->Clear();
        out = tg->TransformPoint(Point(0, 0));
    });
    VERIFY_ARE_EQUAL(0, out.X);
}

void TransformTests::CompositeTransformDirtyFlagsWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CanvasWithTransforms.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();

    Storyboard^ storyboard = nullptr;
    Storyboard^ storyboard2 = nullptr;

    // <B-XAML: Edge crashes if you try to do anything on RS1_ONECORE_DEP_UXP\14291.1001.160316-1731>
    //
    // An animation targets CompositeTransform, and goes into Filling. Normally this unmarks the element for transform animations,
    // and we drop out of expression mode and back into a static matrix. But, if something else keeps it in expression mode, then
    // the WUC animation stays set on the expression, even though the Xaml animation has expired.
    //
    // If something else dirties that CompositeTransform, we'll go update its expression. We'll find the WUC animation that's set
    // on it and try to set it again. This is no good - that will restart the animation, which we don't want. In this case it also
    // crashes the app, because we'll create a scoped batch for that WUC animation and look for a Xaml timeline to store the scoped
    // batch. We find no such timeline, since the timeline responsible has already completed and is no longer active, and then we
    // AV.
    //
    // CompositeTransform has individual dirty flags for whether its components have dirty animations. It should use and clean those
    // flags.
    LOG_OUTPUT(L"[Update CompositeTransform component after an animation on it goes into Filling]");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;

        // Long animation on tr1 to put the transforms in expression mode
        auto target1 = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da1 = ref new DoubleAnimation();
        da1->From = 50.0; da1->To = 50.0;
        span.Duration = 50000000L; da1->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da1, target1);
        Storyboard::SetTargetProperty(da1, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);

        // 1-frame animation on tr2 to put a WUC animation on it.
        auto target2 = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"tr2"));

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 50.0; da2->To = 100.0;
        span.Duration = 160000L; da2->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da2, target2);
        Storyboard::SetTargetProperty(da2, L"TranslateX");

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da2);

        sb1CompletedRegistration.Attach(storyboard2, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        storyboard->Begin();
        storyboard2->Begin();
    });

    // Wait for the short animation to end
    sb1CompletedEvent->WaitForDefault();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"AnimComplete1");

    RunOnUIThread([&]()
    {
        // Dirty the CompositeTransform. It shouldn't be reattaching the expired animation. If it does, we crash.
        auto target2 = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"tr2"));
        target2->ScaleX = 2;
    });

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"AnimComplete2");

    RunOnUIThread([&]()
    {
        storyboard->Stop();
        storyboard2->Stop();
    });
}

} } } } } }
