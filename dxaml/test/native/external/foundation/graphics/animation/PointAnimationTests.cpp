// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PointAnimationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ PointAnimationTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool PointAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PointAnimationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool PointAnimationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void PointAnimationTests::PointAnimationBasic()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ myStoryboard;
    PointAnimation^ myPointAnimation;
    EllipseGeometry^ myEllipseGeometry;
    wf::Point ellipseCenterPosition = { 0.0f, 0.0f };

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"PointAnimationTests.xaml"));
    auto sbCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sbCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        myStoryboard = safe_cast<Storyboard^>(rootCanvas->FindName(L"MyStoryboard"));
        myPointAnimation = safe_cast<PointAnimation^>(rootCanvas->FindName(L"MyPointAnimation"));
        myEllipseGeometry = safe_cast<EllipseGeometry^>(rootCanvas->FindName(L"MyEllipseGeometry"));
        sbCompletedRegistration.Attach(myStoryboard, ref new wf::EventHandler<Object^>([sbCompletedEvent](Object^ sender, Object^ e) { sbCompletedEvent->Set(); }));
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"StartState").GetString());

    LOG_OUTPUT(L"Run PointAnimation Normally");
    RunOnUIThread([&]()
    {
        myStoryboard->Begin();
    });

    sbCompletedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    VERIFY_ARE_EQUAL(ellipseCenterPosition.X, 250.0);
    VERIFY_ARE_EQUAL(ellipseCenterPosition.Y, 150.0);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"CompleteNormal").GetString());

    LOG_OUTPUT(L"Run PointAnimation with AutoReverse");
    RunOnUIThread([&]()
    {
        myStoryboard->Stop();

        myStoryboard->AutoReverse = true;
        myStoryboard->Begin();
    });

    sbCompletedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
        myStoryboard->AutoReverse = false;
    });

    // TODO: Currently disabled due to : When AutoReverse = true, Timeline.Completed fires early when Storyboard is rerun
    //VERIFY_ARE_EQUAL(ellipseCenterPosition.X, 50.0);
    //VERIFY_ARE_EQUAL(ellipseCenterPosition.Y, 50.0);

    // TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"CompleteReverse").GetString());

    LOG_OUTPUT(L"Run PointAnimation with FillBehavior Stop");
    RunOnUIThread([&]()
    {
        myStoryboard->Stop();

        myStoryboard->FillBehavior = FillBehavior::Stop;
        myStoryboard->Begin();
    });

    sbCompletedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    VERIFY_ARE_EQUAL(ellipseCenterPosition.X, 50.0);
    VERIFY_ARE_EQUAL(ellipseCenterPosition.Y, 50.0);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"StartState").GetString());

    RunOnUIThread([&]()
    {
        myStoryboard->Stop();
    });
}

void PointAnimationTests::PauseSeekResume()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ myStoryboard;
    PointAnimation^ myPointAnimation;
    EllipseGeometry^ myEllipseGeometry;
    wf::Point ellipseCenterPosition = {0.0f, 0.0f};

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"PointAnimationTests.xaml"));
    auto sbCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sbCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        myStoryboard = safe_cast<Storyboard^>(rootCanvas->FindName(L"MyStoryboard"));
        myPointAnimation = safe_cast<PointAnimation^>(rootCanvas->FindName(L"MyPointAnimation"));
        myEllipseGeometry = safe_cast<EllipseGeometry^>(rootCanvas->FindName(L"MyEllipseGeometry"));
        sbCompletedRegistration.Attach(myStoryboard, ref new wf::EventHandler<Object^>([sbCompletedEvent](Object^ sender, Object^ e) { sbCompletedEvent->Set(); }));

        // For this test, use a longer duration of 10 sec
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;
        myPointAnimation->Duration = DurationHelper::FromTimeSpan(span);
    });

    LOG_OUTPUT(L"Playing storyboard");
    RunOnUIThread([&]()
    {
        myStoryboard->Begin();
    });

    // Let animation advance by a few frames
    TestServices::WindowHelper->SynchronouslyTickUIThread(10);

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    // Verify position increased during few frames of play
    VERIFY_IS_GREATER_THAN(ellipseCenterPosition.X, 50.0);
    VERIFY_IS_GREATER_THAN(ellipseCenterPosition.Y, 50.0);

    LOG_OUTPUT(L"Pausing storyboard");
    RunOnUIThread([&]()
    {
        myStoryboard->Pause();
    });

    // Stay paused for a few frames
    TestServices::WindowHelper->SynchronouslyTickUIThread(10);

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    wf::Point pausedCenterPosition = ellipseCenterPosition;

    // Verify position didn't change while paused
    VERIFY_ARE_EQUAL(pausedCenterPosition.X, ellipseCenterPosition.X);
    VERIFY_ARE_EQUAL(pausedCenterPosition.Y, ellipseCenterPosition.Y);

    LOG_OUTPUT(L"Seeking storyboard");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 95000000L;    // 9.5 seconds
        myStoryboard->Seek(span);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    VERIFY_ARE_EQUAL(ellipseCenterPosition.X, 240);
    VERIFY_ARE_EQUAL(ellipseCenterPosition.Y, 145);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Seeked").GetString());

    LOG_OUTPUT(L"Resuming storyboard");
    RunOnUIThread([&]()
    {
        myStoryboard->Resume();
    });

    // Let storyboard run to completion
    sbCompletedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    VERIFY_ARE_EQUAL(ellipseCenterPosition.X, 250);
    VERIFY_ARE_EQUAL(ellipseCenterPosition.Y, 150);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Resumed").GetString());

    RunOnUIThread([&]()
    {
        myStoryboard->Stop();
    });
}

void PointAnimationTests::NegativeTests()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ myStoryboard;
    PointAnimation^ myPointAnimation;
    EllipseGeometry^ myEllipseGeometry;
    wf::Point ellipseCenterPosition = { 0.0f, 0.0f };

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"PointAnimationTests.xaml"));
    auto animationCompletedRegistration = CreateSafeEventRegistration(PointAnimation, Completed); auto animationCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        myStoryboard = safe_cast<Storyboard^>(rootCanvas->FindName(L"MyStoryboard"));
        myPointAnimation = safe_cast<PointAnimation^>(rootCanvas->FindName(L"MyPointAnimation"));
        myEllipseGeometry = safe_cast<EllipseGeometry^>(rootCanvas->FindName(L"MyEllipseGeometry"));
        animationCompletedRegistration.Attach(myPointAnimation, ref new wf::EventHandler<Object^>([animationCompletedEvent](Object^ sender, Object^ e) { animationCompletedEvent->Set(); }));

        // For this test, use a shorter duration of 0.1 sec
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 1000000L;
        myPointAnimation->Duration = DurationHelper::FromTimeSpan(span);
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"StartState").GetString());

    LOG_OUTPUT(L"Try to run the test without EnableDependentAnimation set");
    RunOnUIThread([&]()
    {
        myPointAnimation->EnableDependentAnimation = false;
        myStoryboard->Begin();
    });

    bool gotCompleted = animationCompletedEvent->WaitForNoThrow(std::chrono::milliseconds(500) /* timeout */);
    VERIFY_IS_FALSE(gotCompleted, L"Completed event should not fire when EnableDependentAnimation is false");

    RunOnUIThread([&]()
    {
        ellipseCenterPosition = myEllipseGeometry->Center;
    });

    VERIFY_ARE_EQUAL(ellipseCenterPosition.X, 50.0);
    VERIFY_ARE_EQUAL(ellipseCenterPosition.Y, 50.0);

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"StartState").GetString());

    RunOnUIThread([&]()
    {
        myStoryboard->Stop();
    });
}

} } } } } }
