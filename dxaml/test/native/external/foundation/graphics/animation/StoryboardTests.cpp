// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StoryboardTests.h"
#include <AnimationTestHelper.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <DisableRenderingScopeGuard.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ StoryboardTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool StoryboardTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();

    return true;
}

bool StoryboardTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool StoryboardTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void StoryboardTests::ComplexStoryboardsWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ sb1;

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"[Storyboard AutoReverse + SpeedRatio]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);

        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->SpeedRatio = 2;
        sb1->AutoReverse = true;

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBAutoReverse");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[Storyboard repeats forever]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->RepeatBehavior = RepeatBehaviorHelper::Forever;

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBRepeatForever");
    sb1CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(sb1CompletedEvent->HasFired());     // It's looping forever. This shouldn't fire.
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[2 Nested Storyboards]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);

        Storyboard^ sb2 = AnimationTestHelper::MakeStoryboard(da1);
        sb2->AutoReverse = true;

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(sb2);
        sb1->RepeatBehavior = RepeatBehaviorHelper::FromCount(3);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBRepeat-SBReverse");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[Storyboard BeginTime]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        ::Windows::Foundation::TimeSpan span; span.Duration = 2000000L;
        sb1->BeginTime = span;
        sb1->RepeatBehavior = RepeatBehaviorHelper::FromCount(2);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBBeginTime");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[Storyboard repeat, DoubleAnimation BeginTime]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);
        ::Windows::Foundation::TimeSpan span; span.Duration = 2000000L;
        da1->BeginTime = span;

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->RepeatBehavior = RepeatBehaviorHelper::FromCount(2);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBRepeat-DABeginTime");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[Starting a Storyboard]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->RepeatBehavior = RepeatBehaviorHelper::FromCount(2);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBRestart");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedEvent->Reset();     // But keep the event attached
    VERIFY_IS_FALSE(sb1CompletedEvent->HasFired());

    LOG_OUTPUT(L"[Restarting a Storyboard]");
    RunOnUIThread([&]()
    {
        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBRestart");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedEvent->Reset();     // But keep the event attached
    VERIFY_IS_FALSE(sb1CompletedEvent->HasFired());

    LOG_OUTPUT(L"[Restarting a Storyboard after stopping]");
    RunOnUIThread([&]()
    {
        sb1->Stop();
        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SBRestart");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[Multiple animations inside a Storyboard]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);
        DoubleAnimation^ da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 2000000L);

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->RepeatBehavior = RepeatBehaviorHelper::FromCount(2);
        sb1->Children->Append(da2);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SB-2DA");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();

    LOG_OUTPUT(L"[Mixing simple animations with SetCustomTimeline ones]");
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 1000000L);
        DoubleAnimation^ da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 2000000L);

        Storyboard^ sb2 = AnimationTestHelper::MakeStoryboard(da1);
        sb2->AutoReverse = true;    // da1 will use SetCustomTimeline

        if (sb1 != nullptr) { sb1->Stop(); }
        sb1 = AnimationTestHelper::MakeStoryboard(sb2);
        sb1->Children->Append(da2); // da2 is all WUC

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SB-2DA-Mixed");
    sb1CompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    sb1CompletedRegistration.Detach();
    sb1CompletedEvent->Reset();
}

void StoryboardTests::CompletedEvent(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto sb2CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb2CompletedEvent = std::make_shared<Event>();
    auto sb3CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb3CompletedEvent = std::make_shared<Event>();
    auto sb4CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb4CompletedEvent = std::make_shared<Event>();
    auto sb5CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb5CompletedEvent = std::make_shared<Event>();
    auto sb6CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb6CompletedEvent = std::make_shared<Event>();
    auto sb7CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb7CompletedEvent = std::make_shared<Event>();
    auto sb8CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb8CompletedEvent = std::make_shared<Event>();
    auto sb9CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb9CompletedEvent = std::make_shared<Event>();
    auto sb10CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb10CompletedEvent = std::make_shared<Event>();

    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();
    auto da2CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da2CompletedEvent = std::make_shared<Event>();
    auto da3CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da3CompletedEvent = std::make_shared<Event>();
    auto da4CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da4CompletedEvent = std::make_shared<Event>();
    auto da5CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da5CompletedEvent = std::make_shared<Event>();
    auto da6CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da6CompletedEvent = std::make_shared<Event>();
    auto da7CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da7CompletedEvent = std::make_shared<Event>();
    auto da8CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da8CompletedEvent = std::make_shared<Event>();
    auto da9CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da9CompletedEvent = std::make_shared<Event>();
    auto da10CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da10CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L);
        DoubleAnimation^ da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 20000000L);
        DoubleAnimation^ da3 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr3", 10000000L);
        DoubleAnimation^ da4 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr4", 5000000L);
        DoubleAnimation^ da5 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr5", 2500000L);
        DoubleAnimation^ da6 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr6", 20000000L);
        DoubleAnimation^ da7 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr7", 10000000L);
        DoubleAnimation^ da8 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr8", 5000000L);
        DoubleAnimation^ da9 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr9", 2500000L);
        DoubleAnimation^ da10 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "canv", "Opacity", 2500000L, 0.25, 0.75);

        da2->SpeedRatio = 2;

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 5000000L;
        da3->BeginTime = span;

        da4->AutoReverse = true;

        da5->RepeatBehavior = RepeatBehaviorHelper::FromCount(4);

        Storyboard^ sb1 = AnimationTestHelper::MakeStoryboard(da1);
        Storyboard^ sb2 = AnimationTestHelper::MakeStoryboard(da2);
        Storyboard^ sb3 = AnimationTestHelper::MakeStoryboard(da3);
        Storyboard^ sb4 = AnimationTestHelper::MakeStoryboard(da4);
        Storyboard^ sb5 = AnimationTestHelper::MakeStoryboard(da5);
        Storyboard^ sb6 = AnimationTestHelper::MakeStoryboard(da6);
        Storyboard^ sb7 = AnimationTestHelper::MakeStoryboard(da7);
        Storyboard^ sb8 = AnimationTestHelper::MakeStoryboard(da8);
        Storyboard^ sb9 = AnimationTestHelper::MakeStoryboard(da9);
        Storyboard^ sb10 = AnimationTestHelper::MakeStoryboard(da10);

        sb6->SpeedRatio = 2;

        sb7->BeginTime = span;

        sb8->AutoReverse = true;

        sb9->RepeatBehavior = RepeatBehaviorHelper::FromCount(4);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        sb2CompletedRegistration.Attach(sb2, ref new wf::EventHandler<Object^>([sb2CompletedEvent](Object^ sender, Object^ e) { sb2CompletedEvent->Set(); }));
        sb3CompletedRegistration.Attach(sb3, ref new wf::EventHandler<Object^>([sb3CompletedEvent](Object^ sender, Object^ e) { sb3CompletedEvent->Set(); }));
        sb4CompletedRegistration.Attach(sb4, ref new wf::EventHandler<Object^>([sb4CompletedEvent](Object^ sender, Object^ e) { sb4CompletedEvent->Set(); }));
        sb5CompletedRegistration.Attach(sb5, ref new wf::EventHandler<Object^>([sb5CompletedEvent](Object^ sender, Object^ e) { sb5CompletedEvent->Set(); }));
        sb6CompletedRegistration.Attach(sb6, ref new wf::EventHandler<Object^>([sb6CompletedEvent](Object^ sender, Object^ e) { sb6CompletedEvent->Set(); }));
        sb7CompletedRegistration.Attach(sb7, ref new wf::EventHandler<Object^>([sb7CompletedEvent](Object^ sender, Object^ e) { sb7CompletedEvent->Set(); }));
        sb8CompletedRegistration.Attach(sb8, ref new wf::EventHandler<Object^>([sb8CompletedEvent](Object^ sender, Object^ e) { sb8CompletedEvent->Set(); }));
        sb9CompletedRegistration.Attach(sb9, ref new wf::EventHandler<Object^>([sb9CompletedEvent](Object^ sender, Object^ e) { sb9CompletedEvent->Set(); }));
        sb10CompletedRegistration.Attach(sb10, ref new wf::EventHandler<Object^>([sb10CompletedEvent](Object^ sender, Object^ e) { sb10CompletedEvent->Set(); }));

        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));
        da2CompletedRegistration.Attach(da2, ref new wf::EventHandler<Object^>([da2CompletedEvent](Object^ sender, Object^ e) { da2CompletedEvent->Set(); }));
        da3CompletedRegistration.Attach(da3, ref new wf::EventHandler<Object^>([da3CompletedEvent](Object^ sender, Object^ e) { da3CompletedEvent->Set(); }));
        da4CompletedRegistration.Attach(da4, ref new wf::EventHandler<Object^>([da4CompletedEvent](Object^ sender, Object^ e) { da4CompletedEvent->Set(); }));
        da5CompletedRegistration.Attach(da5, ref new wf::EventHandler<Object^>([da5CompletedEvent](Object^ sender, Object^ e) { da5CompletedEvent->Set(); }));
        da6CompletedRegistration.Attach(da6, ref new wf::EventHandler<Object^>([da6CompletedEvent](Object^ sender, Object^ e) { da6CompletedEvent->Set(); }));
        da7CompletedRegistration.Attach(da7, ref new wf::EventHandler<Object^>([da7CompletedEvent](Object^ sender, Object^ e) { da7CompletedEvent->Set(); }));
        da8CompletedRegistration.Attach(da8, ref new wf::EventHandler<Object^>([da8CompletedEvent](Object^ sender, Object^ e) { da8CompletedEvent->Set(); }));
        da9CompletedRegistration.Attach(da9, ref new wf::EventHandler<Object^>([da9CompletedEvent](Object^ sender, Object^ e) { da9CompletedEvent->Set(); }));
        da10CompletedRegistration.Attach(da10, ref new wf::EventHandler<Object^>([da10CompletedEvent](Object^ sender, Object^ e) { da10CompletedEvent->Set(); }));

        sb1->Begin();
        sb2->Begin();
        sb3->Begin();
        sb4->Begin();
        sb5->Begin();
        sb6->Begin();
        sb7->Begin();
        sb8->Begin();
        sb9->Begin();
        sb10->Begin();
    });

    sb1CompletedEvent->WaitForDefault();
    sb2CompletedEvent->WaitForDefault();
    sb3CompletedEvent->WaitForDefault();
    sb4CompletedEvent->WaitForDefault();
    sb5CompletedEvent->WaitForDefault();
    sb6CompletedEvent->WaitForDefault();
    sb7CompletedEvent->WaitForDefault();
    sb8CompletedEvent->WaitForDefault();
    sb9CompletedEvent->WaitForDefault();
    sb10CompletedEvent->WaitForDefault();

    da1CompletedEvent->WaitForDefault();
    da2CompletedEvent->WaitForDefault();
    da3CompletedEvent->WaitForDefault();
    da4CompletedEvent->WaitForDefault();
    da5CompletedEvent->WaitForDefault();
    da6CompletedEvent->WaitForDefault();
    da7CompletedEvent->WaitForDefault();
    da8CompletedEvent->WaitForDefault();
    da9CompletedEvent->WaitForDefault();
    da10CompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb2CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb3CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb4CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb5CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb6CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb7CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb8CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb9CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb10CompletedEvent->HasFired());

    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da2CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da3CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da4CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da5CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da6CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da7CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da8CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da9CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da10CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEventWUC()
{
    CompletedEvent(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::CompletedEventCollapsedTargetWUC()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();

    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();
    auto da2CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da2CompletedEvent = std::make_shared<Event>();
    auto da3CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da3CompletedEvent = std::make_shared<Event>();
    auto da4CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da4CompletedEvent = std::make_shared<Event>();
    auto da5CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da5CompletedEvent = std::make_shared<Event>();
    auto da6CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da6CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L);
        DoubleAnimation^ da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "trCollapsed", "X", 10000000L, 0, 10);
        DoubleAnimation^ da3 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr3dCollapsed", "TranslateX", 10000000L, 0, 10);
        DoubleAnimation^ da4 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "projCollapsed", "LocalOffsetX", 10000000L, 0, 10);
        DoubleAnimation^ da5 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "canv", "Opacity", 10000000L, 0, 1);
        DoubleAnimation^ da6 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "canv", "(Canvas.Left)", 10000000L, 0, 1);

        Storyboard^ sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->Children->Append(da2);
        sb1->Children->Append(da3);
        sb1->Children->Append(da4);
        sb1->Children->Append(da5);
        sb1->Children->Append(da6);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));
        da2CompletedRegistration.Attach(da2, ref new wf::EventHandler<Object^>([da2CompletedEvent](Object^ sender, Object^ e) { da2CompletedEvent->Set(); }));
        da3CompletedRegistration.Attach(da3, ref new wf::EventHandler<Object^>([da3CompletedEvent](Object^ sender, Object^ e) { da3CompletedEvent->Set(); }));
        da4CompletedRegistration.Attach(da4, ref new wf::EventHandler<Object^>([da4CompletedEvent](Object^ sender, Object^ e) { da4CompletedEvent->Set(); }));
        da5CompletedRegistration.Attach(da5, ref new wf::EventHandler<Object^>([da5CompletedEvent](Object^ sender, Object^ e) { da5CompletedEvent->Set(); }));
        da6CompletedRegistration.Attach(da6, ref new wf::EventHandler<Object^>([da6CompletedEvent](Object^ sender, Object^ e) { da6CompletedEvent->Set(); }));

        sb1->Begin();
    });

    sb1CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(1500));
    da1CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));    // It's 1.5 seconds later - sb1 waited already
    da2CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));
    da3CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));
    da4CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));
    da5CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));
    da6CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));

    // Even though the targets of da2-6 are collapsed, they should still complete, and so should sb1.
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da2CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da3CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da4CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da5CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da6CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEvent_ZeroDuration(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();

    DoubleAnimation^ da1;
    Storyboard^ sb1;

    RunOnUIThread([&]()
    {
        da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 0L);

        sb1 = AnimationTestHelper::MakeStoryboard(da1);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));

        sb1->Begin();
    });

    sb1CompletedEvent->WaitForDefault();
    da1CompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEventWUC_ZeroDuration()
{
    CompletedEvent_ZeroDuration(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::CompletedEvent_TargetDetached(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto sb2CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb2CompletedEvent = std::make_shared<Event>();
    auto sb3CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb3CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();
    auto da2CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da2CompletedEvent = std::make_shared<Event>();
    auto da3CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da3CompletedEvent = std::make_shared<Event>();

    // target detached, timelines released, but completed handlers still attached
    DoubleAnimation^ da1;
    Storyboard^ sb1;

    // target detached but timelines still held
    DoubleAnimation^ da2;
    Storyboard^ sb2;

    // target detached, timelines released, completed handlers detached = timelines should be deleted
    DoubleAnimation^ da3;
    Storyboard^ sb3;
    WeakReference da3w;
    WeakReference sb3w;

    RunOnUIThread([&]()
    {
        da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 5000000L);
        da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 5000000L);
        da3 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr3", 5000000L);

        sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb2 = AnimationTestHelper::MakeStoryboard(da2);
        sb3 = AnimationTestHelper::MakeStoryboard(da3);

        da3w = da3;
        sb3w = sb3;

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        sb2CompletedRegistration.Attach(sb2, ref new wf::EventHandler<Object^>([sb2CompletedEvent](Object^ sender, Object^ e) { sb2CompletedEvent->Set(); }));
        sb3CompletedRegistration.Attach(sb3, ref new wf::EventHandler<Object^>([sb3CompletedEvent](Object^ sender, Object^ e) { sb3CompletedEvent->Set(); }));
        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));
        da2CompletedRegistration.Attach(da2, ref new wf::EventHandler<Object^>([da2CompletedEvent](Object^ sender, Object^ e) { da2CompletedEvent->Set(); }));
        da3CompletedRegistration.Attach(da3, ref new wf::EventHandler<Object^>([da3CompletedEvent](Object^ sender, Object^ e) { da3CompletedEvent->Set(); }));

        sb1->Begin();
        sb2->Begin();
        sb3->Begin();
    });

    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"Detaching tree and releasing animations");

    RunOnUIThread([&]()
    {
        da1 = nullptr;
        sb1 = nullptr;

        da3 = nullptr;
        sb3 = nullptr;
        da3CompletedRegistration.Detach();
        sb3CompletedRegistration.Detach();

        Canvas^ animatingCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        animatingCanvas->RenderTransform = nullptr;
    });

    // The Storyboards are still expected to complete (except the one without a Completed handler), but the animations aren't.
    sb1CompletedEvent->WaitForDefault();
    sb2CompletedEvent->WaitForDefault();
    sb3CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    da1CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));    // It's a second later - sb3 waited already
    da2CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));
    da3CompletedEvent->WaitForNoThrow(std::chrono::milliseconds(0));

    sb3 = sb3w.Resolve<Storyboard>();
    da3 = da3w.Resolve<DoubleAnimation>();

    VERIFY_IS_NULL(sb3);
    VERIFY_IS_NULL(da3);

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb2CompletedEvent->HasFired());
    VERIFY_IS_FALSE(sb3CompletedEvent->HasFired());
    VERIFY_IS_FALSE(da1CompletedEvent->HasFired());
    VERIFY_IS_FALSE(da2CompletedEvent->HasFired());
    VERIFY_IS_FALSE(da1CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEventWUC_TargetDetached()
{
    CompletedEvent_TargetDetached(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::CompletedEvent_BlankStoryboard(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto sb2CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb2CompletedEvent = std::make_shared<Event>();

    Storyboard^ sb1;
    Storyboard^ sb2;

    RunOnUIThread([&]()
    {
        sb1 = AnimationTestHelper::MakeBlankStoryboard(10000000L);
        sb2 = AnimationTestHelper::MakeBlankStoryboard(0L);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        sb2CompletedRegistration.Attach(sb2, ref new wf::EventHandler<Object^>([sb2CompletedEvent](Object^ sender, Object^ e) { sb2CompletedEvent->Set(); }));

        sb1->Begin();
        sb2->Begin();
    });

    // The Storyboards are still expected to complete, but the animations aren't.
    sb1CompletedEvent->WaitForDefault();
    sb2CompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb2CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEventWUC_BlankStoryboard()
{
    CompletedEvent_BlankStoryboard(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::CompletedEvent_ClippedAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto sb2CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb2CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();
    auto da2CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da2CompletedEvent = std::make_shared<Event>();

    Storyboard^ sb1;
    Storyboard^ sb2;
    DoubleAnimation^ da1;
    DoubleAnimation^ da2;

    RunOnUIThread([&]()
    {
        da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L);
        da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 10000000L);

        sb1 = AnimationTestHelper::MakeStoryboard(da1, 2500000L);
        sb2 = AnimationTestHelper::MakeStoryboard(da2, 0);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        sb2CompletedRegistration.Attach(sb2, ref new wf::EventHandler<Object^>([sb2CompletedEvent](Object^ sender, Object^ e) { sb2CompletedEvent->Set(); }));
        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));
        da2CompletedRegistration.Attach(da2, ref new wf::EventHandler<Object^>([da2CompletedEvent](Object^ sender, Object^ e) { da2CompletedEvent->Set(); }));

        sb1->Begin();
        sb2->Begin();
    });

    // The Storyboards are still expected to complete, but the animations aren't.
    sb1CompletedEvent->WaitForDefault();
    sb2CompletedEvent->WaitForDefault();
    da1CompletedEvent->WaitForDefault();
    da2CompletedEvent->WaitForDefault();

    // There's no guarantee of timeliness of the Completed event, but we assume a 250ms animation fires Completed within 500ms.
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb2CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da2CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEventWUC_ClippedAnimation()
{
    CompletedEvent_ClippedAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::CompletedEvent_TargetNotRenderedWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    CompletedEvent_TargetNotRenderedCommon();
}

void StoryboardTests::CompletedEvent_TargetNotRendered_RenderDisabledWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    DisableRenderingScopeGuard disableRendering;
    CompletedEvent_TargetNotRenderedCommon();
}

void StoryboardTests::CompletedEvent_TargetNotRenderedCommon()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        Canvas^ animatingCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        animatingCanvas->Opacity = 0;   // Canvas no longer renders. The animated transform is inside.

        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L);
        Storyboard^ sb1 = AnimationTestHelper::MakeStoryboard(da1);

        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));
        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });

    da1CompletedEvent->WaitForDefault();
    sb1CompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
}

void StoryboardTests::ColorCompletedEventWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ColorCompletedEventCommon(false);
}

void StoryboardTests::ColorCompletedEvent_BrushNotRenderedWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ColorCompletedEventCommon(true);
}

void StoryboardTests::ColorCompletedEvent_BrushNotRendered_RenderDisabledWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    DisableRenderingScopeGuard disableRendering;
    ColorCompletedEventCommon(true);
}

void StoryboardTests::ColorCompletedEventCommon(bool hideCanvas)
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto ca1CompletedRegistration = CreateSafeEventRegistration(ColorAnimation, Completed); auto ca1CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        if (hideCanvas)
        {
            Canvas^ animatingCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
            animatingCanvas->Opacity = 0;   // Canvas no longer renders. The animated brush is inside.
        }

        ColorAnimation^ ca1 = AnimationTestHelper::MakeColorAnimation(rootCanvas, "brush", 10000000L, 0xffff0000, 0xff0000ff);
        Storyboard^ sb1 = AnimationTestHelper::MakeStoryboard(ca1);

        ca1CompletedRegistration.Attach(ca1, ref new wf::EventHandler<Object^>([ca1CompletedEvent](Object^ sender, Object^ e) { ca1CompletedEvent->Set(); }));
        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });

    ca1CompletedEvent->WaitForDefault();
    sb1CompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(ca1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEvent_DeviceLost(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L);
        Storyboard^ sb1 = AnimationTestHelper::MakeStoryboard(da1);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));

        sb1->Begin();
    });

    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->WaitForIdle();

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
}

void StoryboardTests::CompletedEvent_DeviceLost()
{
    CompletedEvent_DeviceLost(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::RestartStoryboardWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    DoubleAnimation^ da1;
    Storyboard^ sb1;
    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        // Use a long animation (10s). We don't want it to complete.
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 100000000L);

        sb1 = AnimationTestHelper::MakeStoryboard(da1);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        sb1->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Running");
    VERIFY_IS_FALSE(da1CompletedEvent->HasFired());

    RunOnUIThread([&]()
    {
        // Start the storyboard while it's already playing. This should not falsely trigger the completed event to fire.
        sb1->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Running");
    VERIFY_IS_FALSE(da1CompletedEvent->HasFired());

    RunOnUIThread([&]()
    {
        // Stop the storyboard and start it again. This should not falsely trigger the completed event to fire.
        sb1->Stop();
        sb1->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Running");
    VERIFY_IS_FALSE(da1CompletedEvent->HasFired());
}

void StoryboardTests::SuspendResume(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    TestCleanupWrapper cleanup([wh]() { wh->SetIsSuspended(false); });

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();
    auto sb2CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb2CompletedEvent = std::make_shared<Event>();
    auto sb3CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb3CompletedEvent = std::make_shared<Event>();
    auto da1CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da1CompletedEvent = std::make_shared<Event>();
    auto da2CompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); auto da2CompletedEvent = std::make_shared<Event>();
    auto ca1CompletedRegistration = CreateSafeEventRegistration(ColorAnimation, Completed); auto ca1CompletedEvent = std::make_shared<Event>();

    Storyboard^ sb1;
    Storyboard^ sb2;
    Storyboard^ sb3;
    DoubleAnimation^ da1;
    DoubleAnimation^ da2;
    ColorAnimation^ ca1;

    wh->SetTimeManagerClockOverrideConstant(0);

    RunOnUIThread([&]()
    {
        da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L, 10.0, 10.0);
        da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 10000000L, 10.0, 10.0);
        ca1 = AnimationTestHelper::MakeColorAnimation(rootCanvas, "brush", 10000000L, 0xffff0000, 0xff0000ff);

        sb1 = AnimationTestHelper::MakeStoryboard(da1, 20000000L);
        sb2 = AnimationTestHelper::MakeStoryboard(da2, 20000000L);
        sb3 = AnimationTestHelper::MakeStoryboard(ca1, 20000000L);

        sb1CompletedRegistration.Attach(sb1, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));
        sb2CompletedRegistration.Attach(sb2, ref new wf::EventHandler<Object^>([sb2CompletedEvent](Object^ sender, Object^ e) { sb2CompletedEvent->Set(); }));
        sb3CompletedRegistration.Attach(sb2, ref new wf::EventHandler<Object^>([sb3CompletedEvent](Object^ sender, Object^ e) { sb3CompletedEvent->Set(); }));
        da1CompletedRegistration.Attach(da1, ref new wf::EventHandler<Object^>([da1CompletedEvent](Object^ sender, Object^ e) { da1CompletedEvent->Set(); }));
        da2CompletedRegistration.Attach(da2, ref new wf::EventHandler<Object^>([da2CompletedEvent](Object^ sender, Object^ e) { da2CompletedEvent->Set(); }));
        ca1CompletedRegistration.Attach(ca1, ref new wf::EventHandler<Object^>([ca1CompletedEvent](Object^ sender, Object^ e) { ca1CompletedEvent->Set(); }));

        sb1->Begin();
        sb2->Begin();
        sb3->Begin();
    });

    LOG_OUTPUT(L"Suspend & resume. The storyboards should be paused on suspend and resumed on resume.");

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"1-A").GetString());

    wh->SimulateSuspendToPauseAnimations();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"1-S").GetString());

    // 0.25s elapsed while suspended. The animation should be seeked to 0.25 on resume.
    wh->SetTimeManagerClockOverrideConstant(0.25);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"1-R").GetString());

    LOG_OUTPUT(L"Pause one storyboard. Suspend & resume. The paused storyboard should stay paused after the resume.");

    RunOnUIThread([&]()
    {
        sb1->Pause();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2-P").GetString());

    wh->SimulateSuspendToPauseAnimations();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2-S").GetString());

    // Another 0.25s elapsed while suspended, but this animation was paused during that time. The animation should stay seeked to 0.25 on resume.
    wh->SetTimeManagerClockOverrideConstant(0.5);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2-R").GetString());

    LOG_OUTPUT(L"Resume the storyboard. Suspend & resume. The storyboards should be paused on suspend and resumed on resume.");

    RunOnUIThread([&]()
    {
        sb1->Resume();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"3-A").GetString());

    wh->SimulateSuspendToPauseAnimations();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"3-S").GetString());

    // Another 0.25s elapsed while suspended, with the animation running this time. The animation should seek to 0.5 on resume.
    wh->SetTimeManagerClockOverrideConstant(0.75);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"3-R").GetString());

    LOG_OUTPUT(L"Pause one storyboard, then immediately suspend & resume. The paused storyboard should stay paused after the resume.");

    RunOnUIThread([&]()
    {
        sb1->Pause();
        wh->SimulateSuspendToPauseAnimations();
        wh->SetIsSuspended(true);
    });

    // Suspends as if the pause didn't come in
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"4-S").GetString());

    // Resumes as if the animation was paused (after a tick)
    // Another 0.25s elapsed while suspended, but the animation was paused. It should stay seeked to 0.5.
    wh->SetTimeManagerClockOverrideConstant(1);
    wh->SetIsSuspended(false);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"4-R").GetString());

    LOG_OUTPUT(L"Resume the storyboard. Suspend & resume. The storyboards should be paused on suspend and resumed on resume.");

    RunOnUIThread([&]()
    {
        sb1->Resume();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"5-A").GetString());

    wh->SimulateSuspendToPauseAnimations();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"5-S").GetString());

    // Another 0.25s elapsed while suspended, with the animation running. It should seek to 0.75.
    wh->SetTimeManagerClockOverrideConstant(1.25);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"5-R").GetString());

    LOG_OUTPUT(L"Begin the storyboard then immediately suspend, before we get a chance to tick it.");

    RunOnUIThread([&]()
    {
        sb1->Stop();
        sb2->Stop();
        sb3->Stop();
    });
    wh->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        sb1->Begin();
        sb2->Begin();
        sb3->Begin();

        wh->SimulateSuspendToPauseAnimations();
        wh->SetIsSuspended(true);
    });

    // The animation started at 1.25 but we never ticked it, so it shouldn't be in the DComp output.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"6-S").GetString());

    // The animation restarted at 1.25 although we never ticked it, it's now 1.5. It should not seek on resuming because this is the first time it's ticked.
    wh->SetTimeManagerClockOverrideConstant(1.5);
    wh->SetIsSuspended(false);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"6-R").GetString());

    LOG_OUTPUT(L"Seek past the end of the animations but before the end of the storyboard. The animations should be paused on suspend and resumed on resume.");

    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 15000000L;
        sb1->Seek(span);
        sb2->Seek(span);
        sb3->Seek(span);
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"7-A").GetString());

    wh->SimulateSuspendToPauseAnimations();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"7-S").GetString());

    // The animation was seeked to 1.5s at 1.5s. It's now 1.75. It should seek to 1.75 on resuming.
    wh->SetTimeManagerClockOverrideConstant(1.75);
    wh->SimulateResumeToResumeAnimations();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"7-R").GetString());

    LOG_OUTPUT(L"Seek immediately after coming out of suspend. There should be no autoseek from the resume.");

    wh->SimulateSuspendToPauseAnimations();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"8-S").GetString());

    // The animation was explicitly seeked to 0.5s after the resume. That's the seek that should apply to DComp.
    wh->SetTimeManagerClockOverrideConstant(2);
    RunOnUIThread([&]()
    {
        wh->SimulateResumeToResumeAnimations();

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 5000000L;
        sb1->Seek(span);
        sb2->Seek(span);
        sb3->Seek(span);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"8-R").GetString());

    LOG_OUTPUT(L"The storyboards and animations should still complete.");

    wh->SetTimeManagerClockOverrideConstant(-1);

    // The Storyboards are still expected to complete.
    sb1CompletedEvent->WaitForDefault();
    sb2CompletedEvent->WaitForDefault();
    sb3CompletedEvent->WaitForDefault();
    da1CompletedEvent->WaitForDefault();
    da2CompletedEvent->WaitForDefault();
    ca1CompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(sb1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb2CompletedEvent->HasFired());
    VERIFY_IS_TRUE(sb3CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da1CompletedEvent->HasFired());
    VERIFY_IS_TRUE(da2CompletedEvent->HasFired());
    VERIFY_IS_TRUE(ca1CompletedEvent->HasFired());
}

void StoryboardTests::SuspendResumeWUC()
{
    SuspendResume(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::LongStoryboardsCommon()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Long continuous animation - works");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 80000000.0;
        ::Windows::Foundation::TimeSpan maxDuration;
        maxDuration.Duration = 20736000000000L;     // 24 days, in 10-ns ticks
        da->Duration = DurationHelper::FromTimeSpan(maxDuration);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"BigLinear").GetString());

    LOG_OUTPUT(L"Long key frame animation - works");
    RunOnUIThread([&]()
    {
        storyboard->Stop();

        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 0;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan maxDuration;
        maxDuration.Duration = 20736000000000L;     // 24 days, in 10-ns ticks
        KeyTime kt2; kt2.TimeSpan = maxDuration;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 40000000;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"BigKeyFrame").GetString());

    LOG_OUTPUT(L"Long repeat - works");
    RunOnUIThread([&]()
    {
        storyboard->Stop();

        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan maxDuration; 
        maxDuration.Duration = 20736000000000L;     // 24 days, in 10-ns ticks
        da->Duration = DurationHelper::FromTimeSpan(maxDuration);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        storyboard->RepeatBehavior = RepeatBehaviorHelper::FromCount(2);
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"LongRepeat").GetString());

    LOG_OUTPUT(L"Many repeats - works, but there's floating point error in the iteration count");
    RunOnUIThread([&]()
    {
        storyboard->Stop();

        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 50000000L;    // 5 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        storyboard->RepeatBehavior = RepeatBehaviorHelper::FromCount(2147483000);   // Slightly smaller than max Int32
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"BigRepeat").GetString());

    LOG_OUTPUT(L"Long reverse - works");
    RunOnUIThread([&]()
    {
        storyboard->Stop();

        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan maxDuration;
        maxDuration.Duration = 20736000000000L;     // 24 days, in 10-ns ticks
        da->Duration = DurationHelper::FromTimeSpan(maxDuration);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        storyboard->AutoReverse = true;
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"LongReverse").GetString());
}

void StoryboardTests::LongStoryboardsWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    LongStoryboardsCommon();
}

void StoryboardTests::StoryboardExpires_KeepExpression()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetTimeManagerClockOverrideConstant(0);

    Canvas^ rootCanvas;
    TransformGroup^ transformGroup;
    TranslateTransform^ translate;
    TranslateTransform^ translate2;
    RunOnUIThread([&]()
    {
        translate = ref new TranslateTransform();
        translate2 = ref new TranslateTransform();
        transformGroup = ref new TransformGroup();
        transformGroup->Children->Append(translate);
        transformGroup->Children->Append(translate2);

        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        rect->Width = 50;
        rect->Height = 50;
        rect->RenderTransform = transformGroup;

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(rect);

        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    Storyboard^ storyboard;     // Will run and expire
    Storyboard^ storyboard2;    // Will keep running
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L; // 1 second
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, translate);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);
        storyboard->Begin();

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 50.0;
        da2->To = 50.0;
        span.Duration = 100000000L; // 10 seconds
        da2->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da2, translate2);
        Storyboard::SetTargetProperty(da2, L"Y");

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da2);
        storyboard2->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    wh->SetTimeManagerClockOverrideConstant(0.5);
    wh->SynchronouslyTickUIThread(1);
    LOG_OUTPUT(L"[Storyboard is running.]");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    wh->SetTimeManagerClockOverrideConstant(1.5);
    wh->FireDCompAnimationCompleted(storyboard);
    wh->SynchronouslyTickUIThread(1);
    LOG_OUTPUT(L"[Storyboard expires.]");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
        storyboard2->Stop();
    });
}

void StoryboardTests::StoryboardExpires_RecreateExpression()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetTimeManagerClockOverrideConstant(0);

    Canvas^ rootCanvas;
    TransformGroup^ transformGroup;
    TranslateTransform^ translate;
    TranslateTransform^ translate2;
    RunOnUIThread([&]()
    {
        translate = ref new TranslateTransform();
        translate2 = ref new TranslateTransform();
        transformGroup = ref new TransformGroup();
        transformGroup->Children->Append(translate);
        transformGroup->Children->Append(translate2);

        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        rect->Width = 50;
        rect->Height = 50;
        rect->RenderTransform = transformGroup;
        rect->CompositeMode = ElementCompositeMode::SourceOver; // Force a comp node

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(rect);

        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    Storyboard^ storyboard;     // Will run and expire
    Storyboard^ storyboard2;    // Will keep running
    RunOnUIThread([&]()
    {
        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L; // 1 second
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, translate);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);
        storyboard->Begin();

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 50.0;
        da2->To = 50.0;
        span.Duration = 100000000L; // 10 seconds
        da2->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da2, translate2);
        Storyboard::SetTargetProperty(da2, L"Y");

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da2);
        storyboard2->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    wh->SetTimeManagerClockOverrideConstant(0.5);
    wh->SynchronouslyTickUIThread(1);
    LOG_OUTPUT(L"[Storyboard is running.]");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Running");

    wh->SetTimeManagerClockOverrideConstant(1.5);
    wh->FireDCompAnimationCompleted(storyboard);
    wh->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Storyboard expires. Releasing WUC expression.]");
        storyboard2->Stop();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "ExpressionReleased");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Restoring WUC expression. Don't crash.]");
        storyboard2->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "ExpressionRestored");

    RunOnUIThread([&]()
    {
        storyboard->Stop();
        storyboard2->Stop();
    });
}

void StoryboardTests::PauseSeekResume(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    TranslateTransform^ target;
    SolidColorBrush^ targetBrush;

    RunOnUIThread([&]()
    {
        target = ref new TranslateTransform();

        targetBrush = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(255, 255, 255, 0));

        const auto& canvas = ref new Canvas();
        canvas->Width = 10;
        canvas->Height = 10;
        canvas->RenderTransform = target;
        canvas->Background = targetBrush;

        const auto& rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(canvas);
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"> Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;
    Storyboard^ storyboard2 = nullptr;

    LOG_OUTPUT(L"> Running animation");
    RunOnUIThread([&]()
    {
        KeyTime kt;

        // Pause time is unreliable, so use a key frame animation that does nothing for the first little while.
        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 20000000L;  // Discrete jump to 70 at 2 seconds
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 35 * 2;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 40000000L;  // Discrete jump to 140 at 4 seconds
        KeyTime kt2; kt2.TimeSpan = s2;
        DiscreteDoubleKeyFrame^ kf2 = ref new DiscreteDoubleKeyFrame();
        kf2->Value = 35 * 4;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        ::Windows::Foundation::TimeSpan s3; s3.Duration = 100000000L; // Continues linearly to 350 at 10 seconds
        KeyTime kt3; kt3.TimeSpan = s3;
        LinearDoubleKeyFrame^ kf3 = ref new LinearDoubleKeyFrame();
        kf3->Value = 35 * 10;
        kf3->KeyTime = kt3;
        da->KeyFrames->Append(kf3);

        // Discrete jump to ffff0000 at 2 seconds
        const auto& ckf1 = ref new DiscreteColorKeyFrame();
        ckf1->Value = Microsoft::UI::ColorHelper::FromArgb(255, 255, 0, 0);
        span.Duration = 20000000L; kt.TimeSpan = span; ckf1->KeyTime = kt;

        // Discrete jump to ff00ff00 at 4 seconds
        const auto& ckf2 = ref new DiscreteColorKeyFrame();
        ckf2->Value = Microsoft::UI::ColorHelper::FromArgb(255, 0, 255, 0);
        span.Duration = 40000000L; kt.TimeSpan = span; ckf2->KeyTime = kt;

        // Continues linearly to ff0000ff at 10 seconds
        const auto& ckf3 = ref new LinearColorKeyFrame();
        ckf3->Value = Microsoft::UI::ColorHelper::FromArgb(255, 0, 0, 255);
        span.Duration = 100000000L; kt.TimeSpan = span; ckf3->KeyTime = kt;

        const auto& ca = ref new ColorAnimationUsingKeyFrames();
        span.Duration = 100000000L;    // 10 seconds
        ca->Duration = DurationHelper::FromTimeSpan(span);
        ca->KeyFrames->Append(ckf1);
        ca->KeyFrames->Append(ckf2);
        ca->KeyFrames->Append(ckf3);
        Storyboard::SetTarget(ca, targetBrush);
        Storyboard::SetTargetProperty(ca, L"Color");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);
        storyboard->Children->Append(ca);

        storyboard->Begin();

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        span.Duration = 50000000L; da2->BeginTime = span;   // 5 seconds
        span.Duration = 6000000000L; da2->Duration = DurationHelper::FromTimeSpan(span);    // 600 seconds = 10 minutes
        da2->From = 10.0;
        da2->To = 10.0;
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"Y");

        const auto& ca2 = ref new ColorAnimation();
        span.Duration = 50000000L; ca2->BeginTime = span;   // 5 seconds
        span.Duration = 6000000000L; ca2->Duration = DurationHelper::FromTimeSpan(span);    // 600 seconds = 10 minutes
        ca2->From = Microsoft::UI::ColorHelper::FromArgb(255, 255, 0, 255);
        ca2->To = Microsoft::UI::ColorHelper::FromArgb(255, 255, 0, 255);
        Storyboard::SetTarget(ca2, targetBrush);
        Storyboard::SetTargetProperty(ca2, L"Color");

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da2);
        storyboard2->Children->Append(ca2);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());

    LOG_OUTPUT(L"> Pausing animation");
    RunOnUIThread([&]()
    {
        storyboard->Pause();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Paused").GetString());

    LOG_OUTPUT(L"> Seeking paused animation");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 30000000L;    // 3 seconds
        storyboard->Seek(span);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"PausedSeeked").GetString());

    LOG_OUTPUT(L"> Resuming seeked paused animation");
    RunOnUIThread([&]()
    {
        storyboard->Resume();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Resumed").GetString());

    LOG_OUTPUT(L"> Pausing animation");
    RunOnUIThread([&]()
    {
        storyboard->Pause();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Paused2").GetString());

    LOG_OUTPUT(L"> Resumed paused animation");
    RunOnUIThread([&]()
    {
        storyboard->Resume();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Resumed").GetString());

    LOG_OUTPUT(L"> Seeking resumed animation");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 75000000L;    // 7.5 seconds
        storyboard->Seek(span);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ResumedSeeked").GetString());

    LOG_OUTPUT(L"> Seeking to negative time, past beginning of animation");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = -10000000L;    // -1 second
        storyboard->Seek(span);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"NegativeSeek").GetString());

    LOG_OUTPUT(L"> Seeking past beginning of animation");
    RunOnUIThread([&]()
    {
        // The color animation in storyboard2 targets the same brush as storyboard1. Stop the storyboard so we don't get into a
        // handoff scenario where the initial value is inconsistent.
        storyboard->Stop();

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 20000000L;    // 2 seconds
        storyboard2->Begin();
        storyboard2->Seek(span);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SeekBeforeBeginTime").GetString());

    LOG_OUTPUT(L"> Long seek");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 5400000000L;    // 540 seconds = 9 minutes
        storyboard2->Begin();
        storyboard2->Seek(span);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"LongSeek").GetString());
}

void StoryboardTests::PauseSeekResumeWUCFull()
{
    PauseSeekResume(DCompRendering::WUCCompleteSynchronousCompTree);
}

void StoryboardTests::CustomTimelineRepeat()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StoryboardTests-CanvasWithTargets.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    RunOnUIThread([&]()
    {
        DoubleAnimation^ da1 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr1", 10000000L);
        Storyboard^ sb1 = AnimationTestHelper::MakeStoryboard(da1);
        sb1->AutoReverse = true;    // This should have an EndSegment with a value of 0.

        DoubleAnimation^ da2 = AnimationTestHelper::MakeDoubleAnimation(rootCanvas, "tr2", 10000000L);
        Storyboard^ sb2 = AnimationTestHelper::MakeStoryboard(da2);
        sb2->AutoReverse = true;
        sb2->RepeatBehavior = RepeatBehaviorHelper::FromCount(4);    // This repeats, so it should still have an EndSegment with a value of 1.

        sb1->Begin();
        sb2->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void StoryboardTests::WUCAnimationTelemetry()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    // Animation telemetry will report storyboard names if the perf track event provider is enabled. Enable it here
    // to get consistent results for this test.

    // test is disabled due to removal of Perfanalyzer.
    //PerfAnalyzerSession session;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas ^root, ^l1, ^l2, ^l3, ^l4;
    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Name = L"root";
        wh->WindowContent = root;

        l1 = ref new Canvas();
        ApplyCanvasProperties(l1);
        root->Children->Append(l1);

        l2 = ref new Canvas();
        l2->Name = L"L2";
        ApplyCanvasProperties(l2);
        l1->Children->Append(l2);

        l3 = ref new Canvas();
        l3->Name = L"L3";
        ApplyCanvasProperties(l3);
        l2->Children->Append(l3);

        l4 = ref new Canvas();
        ApplyCanvasProperties(l4);
        l3->Children->Append(l4);
    });
    wh->WaitForIdle();

    Storyboard ^namedSB, ^unnamedSB;
    RunOnUIThread([&]()
    {
        // There's no API to set a name on Storyboard. Do it through markup.
        namedSB = safe_cast<Storyboard^>(Markup::XamlReader::Load(L"<Storyboard xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\" x:Name=\"NamedStoryboard\" />"));
        namedSB->Children->Append(AnimationTestHelper::MakeOpacityAnimation(l1));
        namedSB->Children->Append(AnimationTestHelper::MakeOpacityAnimation(l2));
        namedSB->Begin();

        unnamedSB = ref new Storyboard();
        unnamedSB->Children->Append(AnimationTestHelper::MakeOpacityAnimation(l3));
        unnamedSB->Children->Append(AnimationTestHelper::MakeOpacityAnimation(l4));
        unnamedSB->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, MockDComp::SaveToFileOptions::ShowComments);

    Canvas ^invisible;
    ProgressRing^ ring;
    RunOnUIThread([&]()
    {
        namedSB->Stop();
        unnamedSB->Stop();

        invisible = ref new Canvas();
        invisible->Name = L"invisible";
        root->Children->Append(invisible);

        ring = ref new ProgressRing();
        ring->Width = 50;
        ring->Height = 50;
        ring->IsActive = true;
        invisible->Children->Append(ring);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ProgressRing", MockDComp::SaveToFileOptions::ShowComments);

    RunOnUIThread([&]()
    {
        invisible->Opacity = 0.0;
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"HiddenProgressRing", MockDComp::SaveToFileOptions::ShowComments);
}

void StoryboardTests::SeekPastEnd_FillBehaviorStop()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ canvas;
    SolidColorBrush^ backgroundBrush;

    RunOnUIThread([&]()
    {
        backgroundBrush = ref new SolidColorBrush(Colors::Blue);

        canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 50;
        canvas->Background = backgroundBrush;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(canvas);
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    Storyboard^ storyboard = nullptr;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Animating Canvas with duration 3s animations.");

        ::Windows::Foundation::TimeSpan span; span.Duration = 30000000L;

        ColorAnimation^ colorAnimation = ref new ColorAnimation();
        colorAnimation->From = Colors::Red;
        colorAnimation->To = Colors::Lime;
        colorAnimation->Duration = DurationHelper::FromTimeSpan(span);
        colorAnimation->FillBehavior = FillBehavior::Stop;
        Storyboard::SetTarget(colorAnimation, backgroundBrush);
        Storyboard::SetTargetProperty(colorAnimation, L"Color");

        DoubleAnimation^ doubleAnimation = ref new DoubleAnimation();
        doubleAnimation->From = 50.0;
        doubleAnimation->To = 150.0;
        doubleAnimation->Duration = DurationHelper::FromTimeSpan(span);
        doubleAnimation->FillBehavior = FillBehavior::Stop;
        Storyboard::SetTarget(doubleAnimation, canvas);
        Storyboard::SetTargetProperty(doubleAnimation, L"Canvas.Left");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(colorAnimation);
        storyboard->Children->Append(doubleAnimation);
        storyboard->Begin();
        storyboard->Pause();
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Seeking to 4s.");
        ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 40000000L;
        storyboard->Seek(seekSpan);
    });
    wh->SynchronouslyTickUIThread(3);
    // This should send WUC animation completion but it doesn't. Maybe because it's paused?
    // Seeking WUC animations past their end does not raise animation completed notifications
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Seeked");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Resuming.");
        storyboard->Resume();
    });
    wh->SynchronouslyTickUIThread(3);
    // This is unpaused and definitely should send WUC animation completion but it doesn't.
    // Maybe because it never ticked in the DWM yet? But letting it tick before pausing doesn't fix this.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Resumed");

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void StoryboardTests::SeekPastEnd_RepeatDurationAndSpeedRatio()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ canvas;
    SolidColorBrush^ backgroundBrush;

    RunOnUIThread([&]()
    {
        backgroundBrush = ref new SolidColorBrush(Colors::Blue);

        canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 50;
        canvas->Background = backgroundBrush;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(canvas);
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    Storyboard^ storyboard = nullptr;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Animating Canvas with duration 3s ColorAnimation.");

        ::Windows::Foundation::TimeSpan span; span.Duration = 30000000L;

        ColorAnimation^ colorAnimation = ref new ColorAnimation();
        colorAnimation->SpeedRatio = 10;
        colorAnimation->From = Colors::Red;
        colorAnimation->To = Colors::Lime;
        colorAnimation->Duration = DurationHelper::FromTimeSpan(span);
        colorAnimation->AutoReverse = true;
        RepeatBehavior repeatBehavior = RepeatBehaviorHelper::FromDuration(span);
        colorAnimation->RepeatBehavior = repeatBehavior;
        Storyboard::SetTarget(colorAnimation, backgroundBrush);
        Storyboard::SetTargetProperty(colorAnimation, L"Color");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(colorAnimation);
        storyboard->Begin();
        storyboard->Pause();
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Seeking to 3s.");
        ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 30000000L;
        storyboard->Seek(seekSpan);
    });
    wh->SynchronouslyTickUIThread(3);
    // This should send WUC animation completion but it doesn't. Maybe because it's paused?
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Seeked");

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void StoryboardTests::MixedDependentAndIndependent()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ canvas;
    SolidColorBrush^ backgroundBrush;

    RunOnUIThread([&]()
    {
        backgroundBrush = ref new SolidColorBrush(Colors::Blue);

        canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 50;
        canvas->Background = backgroundBrush;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(canvas);
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    Storyboard^ storyboard = nullptr;
    RunOnUIThread([&]()
    {
        // A Storyboard containing some animations that can be converted to Composition animations and some that can't.
        // The ones that can should use a Composition animation, and the ones that can't should tick dependently.
        LOG_OUTPUT(L"> Creating mixed storyboard.");

        ::Windows::Foundation::TimeSpan span; span.Duration = 30000000L;
        ::Windows::Foundation::TimeSpan longerSpan; longerSpan.Duration = 40000000L;

        DoubleAnimation^ da1 = ref new DoubleAnimation();
        da1->From = 123.0;
        da1->To = 123.0;
        da1->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da1, canvas);
        Storyboard::SetTargetProperty(da1, L"(Canvas.Left)");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 321.0;
        da2->To = 321.0;
        da2->Duration = DurationHelper::FromTimeSpan(longerSpan);   // Clipped by Storyboard, so this falls back to dependent
        Storyboard::SetTarget(da2, canvas);
        Storyboard::SetTargetProperty(da2, L"(Canvas.Top)");

        storyboard = ref new Storyboard();
        storyboard->Duration = DurationHelper::FromTimeSpan(span);
        storyboard->Children->Append(da1);
        storyboard->Children->Append(da2);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void StoryboardTests::ApplyCanvasProperties(Canvas^ canvas)
{
    canvas->Width = 50;
    canvas->Height = 50;
    canvas->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(255, 0, 255, 0));
}

} } } } } }
