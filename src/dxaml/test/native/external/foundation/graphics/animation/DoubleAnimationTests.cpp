// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DoubleAnimationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <AnimationTestHelper.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ DoubleAnimationTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool DoubleAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();

    return true;
}

bool DoubleAnimationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool DoubleAnimationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void DoubleAnimationTests::RenderAnimatedCanvas(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-RenderAnimatedCanvas.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
}

void DoubleAnimationTests::RenderAnimatedCanvasWUCFull()
{
    RenderAnimatedCanvas(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::RetargetAnimation()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"idle").GetString());

    DoubleAnimation^ da = nullptr;
    Storyboard^ sb = nullptr;

    LOG_OUTPUT(L"Animate X on tr1");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        sb = ref new Storyboard();
        sb->Children->Append(da);

        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"1x").GetString());

    LOG_OUTPUT(L"Animate X on tr2");
    RunOnUIThread([&]()
    {
        sb->Stop();
        TranslateTransform^ target2 = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr2"));
        Storyboard::SetTarget(da, target2);
        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2x").GetString());

    LOG_OUTPUT(L"Animate Y on tr2");
    RunOnUIThread([&]()
    {
        sb->Stop();
        Storyboard::SetTargetProperty(da, L"Y");
        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2y").GetString());

    LOG_OUTPUT(L"Stop animations");
    RunOnUIThread([&]()
    {
        sb->Stop();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"idle").GetString());

    LOG_OUTPUT(L"Animate Y on tr2 again");
    RunOnUIThread([&]()
    {
        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2y").GetString());

    LOG_OUTPUT(L"Stop animations, but force a comp node on the target");
    RunOnUIThread([&]()
    {
        sb->Stop();
        Canvas^ canv = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        canv->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"idlecompnode").GetString());

    LOG_OUTPUT(L"Animate Y on tr2 again");
    RunOnUIThread([&]()
    {
        sb->Begin();
    });

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"2yb").GetString());

    LOG_OUTPUT(L"Stop animations; canvas should keep its comp node");
    RunOnUIThread([&]()
    {
        sb->Stop();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"idlecompnode").GetString());
}

void DoubleAnimationTests::AnimationHandoff(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Storyboard^ baseline = nullptr;
    Storyboard^ storyboard = nullptr;
    Storyboard^ storyboard2 = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    // The initial value of the handoff animation depends on the source of the handoff. In order to make this predictable,
    // use a constant animation.
    LOG_OUTPUT(L"Baseline animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 5.0;
        da->To = 5.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        baseline = ref new Storyboard();
        baseline->Children->Append(da);

        baseline->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Baseline").GetString());

    LOG_OUTPUT(L"Handoff to linear To animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ToHandoff").GetString());

    LOG_OUTPUT(L"Back to baseline animation");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
        baseline->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Baseline").GetString());

    LOG_OUTPUT(L"Handoff to linear By animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->By = 30.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ByHandoff").GetString());

    LOG_OUTPUT(L"Back to baseline animation");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
        baseline->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Baseline").GetString());

    LOG_OUTPUT(L"Handoff to key frames");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 30000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        // Only this first segment should have handoff.
        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 20000000L;
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 50;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        ::Windows::Foundation::TimeSpan s3; s3.Duration = 30000000L;
        KeyTime kt3; kt3.TimeSpan = s3;
        LinearDoubleKeyFrame^ kf3 = ref new LinearDoubleKeyFrame();
        kf3->Value = 150;
        kf3->KeyTime = kt3;
        da->KeyFrames->Append(kf3);

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da);

        storyboard2->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"KeyFrameHandoff").GetString());

    LOG_OUTPUT(L"Back to baseline animation");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
        baseline->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Baseline").GetString());

    LOG_OUTPUT(L"Explicit From value - no handoff");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 5.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"NoHandoff").GetString());

    LOG_OUTPUT(L"Stop all animations");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
        baseline->Stop();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    LOG_OUTPUT(L"Self handoff without a commit in between - should not actually hand off");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"NoSelfHandoff").GetString());

    LOG_OUTPUT(L"Back to baseline animation");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
        baseline->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Baseline").GetString());

    LOG_OUTPUT(L"Self handoff - should not crash");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SelfHandoff").GetString());

    LOG_OUTPUT(L"Self handoff - should not crash");
    RunOnUIThread([&]()
    {
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SelfHandoff").GetString());

    LOG_OUTPUT(L"Handoff to a zero-duration animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 0L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 1000;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ToZeroDuration").GetString());

    LOG_OUTPUT(L"Handoff from a zero-duration animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da);

        storyboard2->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"FromZeroDuration").GetString());

    LOG_OUTPUT(L"Handoff to a zero-duration animation (clipped by the storyboard)");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 1000;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        storyboard = ref new Storyboard();
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 0L;
        storyboard->Duration = DurationHelper::FromTimeSpan(span2);
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ToZeroDuration").GetString());

    LOG_OUTPUT(L"Handoff from a zero-duration animation (clipped by the storyboard)");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da);

        storyboard2->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"FromZeroDuration").GetString());
}

void DoubleAnimationTests::AnimationHandoffWUC()
{
    AnimationHandoff(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::AnimationDuration()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Duration 0.0001. Don't crash.");
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 5.0;
        da->To = 5.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 1000L;    // 0.1ms
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->WaitForIdle();
}

void DoubleAnimationTests::AnimationHandoff_DirtyFlags()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;
    Storyboard^ storyboard2 = nullptr;
    DoubleAnimation^ animation = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    LOG_OUTPUT(L"X/Y animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        storyboard = ref new Storyboard();
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        span.Duration = 100000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");
        storyboard->Children->Append(da);

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 25.0;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 100000000L;
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 25.0;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 25.0;
        da2->To = 25.0;
        span.Duration = 100000000L;    // 10 seconds
        da2->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"Y");
        storyboard->Children->Append(da2);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"XY").GetString());

    LOG_OUTPUT(L"Handoff X, Y should keep playing");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        storyboard2 = ref new Storyboard();

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 15.0;
        da->To = 15.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");
        storyboard2->Children->Append(da);

        storyboard2->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"NewX").GetString());

    LOG_OUTPUT(L"X/Y animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        storyboard = ref new Storyboard();
        storyboard->AutoReverse = true;
        ::Windows::Foundation::TimeSpan span;

        animation = ref new DoubleAnimation();
        animation->From = 25.0;
        animation->To = 25.0;
        span.Duration = 10000000L;
        animation->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(animation, target);
        Storyboard::SetTargetProperty(animation, L"X");
        storyboard->Children->Append(animation);

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 25.0;
        da->To = 25.0;
        span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Y");
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"Dur1").GetString());

    LOG_OUTPUT(L"Increase duration of one animation, which increases duration of Storyboard, other animation should remake DComp animation.");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        storyboard->Stop();
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 30000000L;
        animation->Duration = DurationHelper::FromTimeSpan(span);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"Dur3").GetString());

    LOG_OUTPUT(L"Adding another animation, all animations should remake DComp animations.");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr2"));
        ::Windows::Foundation::TimeSpan span;

        storyboard->Stop();

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 25.0;
        da->To = 25.0;
        span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"Dur3-2").GetString());
}

void DoubleAnimationTests::LoadedTrigger_BeginTime()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"LoadedTrigger-BeginTime.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"Initial").GetString());

    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"asdf"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000000L;    // 1 second
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Y");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        sb1CompletedRegistration.Attach(storyboard, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        storyboard->Begin();
    });

    sb1CompletedEvent->WaitForDefault();

    LOG_OUTPUT(L"1 second later. The animation instance should have the same ID.");
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"1s").GetString());
}

void DoubleAnimationTests::PauseResumeOpacityAnimationWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Running animation]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        target->CompositeMode = ElementCompositeMode::SourceOver;   // Force a comp node

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 50000000L;  // Discrete jump to 0.5 at 5 seconds
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 0.5;
        kf1->KeyTime = kt1;

        // Pause time is unreliable, so use a key frame animation that does nothing for the first little while.
        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        da->KeyFrames->Append(kf1);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Opacity");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());

    LOG_OUTPUT(L"[Pausing animation - this clears the \"has independent opacity animation\" flag but keeps the WUC animation set on the UIE]");
    RunOnUIThread([&]()
    {
        storyboard->Pause();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Paused").GetString());

    LOG_OUTPUT(L"[Resuming paused animation - this shouldn't AV on a null expression]");
    RunOnUIThread([&]()
    {
        storyboard->Resume();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::PauseResumeOffsetAnimationWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Running animation]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        target->CompositeMode = ElementCompositeMode::SourceOver;   // Force a comp node

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 50000000L;  // Discrete jump to 50 at 5 seconds
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 50;
        kf1->KeyTime = kt1;

        // Pause time is unreliable, so use a key frame animation that does nothing for the first little while.
        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        da->KeyFrames->Append(kf1);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());

    LOG_OUTPUT(L"[Pausing animation - this clears the \"has independent offset animation\" flag but keeps the WUC animation set on the UIE]");
    RunOnUIThread([&]()
    {
        storyboard->Pause();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Paused").GetString());

    LOG_OUTPUT(L"[Resuming paused animation - this shouldn't AV on a null expression]");
    RunOnUIThread([&]()
    {
        storyboard->Resume();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::PauseAndSeekBeforeFirstFrame(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Pause before first tick happens");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 40000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
        storyboard->Pause();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Paused").GetString());

    LOG_OUTPUT(L"Seek before first tick happens");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 40000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();

        ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 20000000L;
        storyboard->Seek(seekSpan);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Seeked").GetString());
}

void DoubleAnimationTests::PauseAndSeekBeforeFirstFrameWUC()
{
    PauseAndSeekBeforeFirstFrame(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::PauseAndOverwriteWithStaticValue(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"> Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;
    TranslateTransform^ target;
    double uiThreadValue = -1;
    ClockState currentState = ClockState::Stopped;

    LOG_OUTPUT(L"> Running animation");
    RunOnUIThread([&]()
    {
        target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 40000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());

    LOG_OUTPUT(L"> Paused animation");
    RunOnUIThread([&]()
    {
        storyboard->Pause();
    });
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        uiThreadValue = target->X;
    });
    VERIFY_ARE_EQUAL(100, uiThreadValue);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Paused").GetString());

    LOG_OUTPUT(L"> Overwriting with static value");
    RunOnUIThread([&]()
    {
        target->X = 400;
    });
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        uiThreadValue = target->X;
    });

    LOG_OUTPUT(L"> RS1+ - static overwrite not allowed while animation is paused");
    VERIFY_ARE_EQUAL(100, uiThreadValue);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"PausedOverwritten").GetString());

    LOG_OUTPUT(L"> Resuming paused animation");
    RunOnUIThread([&]()
    {
        storyboard->Resume();
    });
    wh->SynchronouslyTickUIThread(1);
    RunOnUIThread([&]()
    {
        uiThreadValue = target->X;
        currentState = storyboard->GetCurrentState();
    });
    VERIFY_ARE_EQUAL(ClockState::Active, currentState);
    VERIFY_ARE_EQUAL(100, uiThreadValue);
    // Should look the same as the running case
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Running").GetString());
}

void DoubleAnimationTests::PauseAndOverwriteWithStaticValueWUC()
{
    PauseAndOverwriteWithStaticValue(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::SeekAlignedToLastTick(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    wh->SetTimeManagerClockOverrideConstant(0);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;
    DoubleAnimationUsingKeyFrames^ da = nullptr;

    // Let a 100ms animation expire, wait 500ms more, SeekAlignedToLastTick back to 0. The animation should be running again.

    LOG_OUTPUT(L"Running animation");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 1000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 0;  // Discrete jump to 100 in the beginning
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 1000000L; // Discrete jump to 50 (and hold) at the end
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 50;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);   // Tick at clock 0s

    // Tick at clock 1s = storyboard expires
    wh->SetTimeManagerClockOverrideConstant(1);
    wh->FireDCompAnimationCompleted(storyboard);
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"End").GetString());

    // Tick at clock 1.5s = storyboard still expired
    wh->SetTimeManagerClockOverrideConstant(1.5);
    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"SeekAlignedToLastTick, verify still active");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span; span.Duration = 0;
        storyboard->SeekAlignedToLastTick(span);    // Seeked at clock 1.5s

        VERIFY_ARE_EQUAL(ClockState::Active, storyboard->GetCurrentState());
    });
    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"10 ms later, verify still active");
    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(ClockState::Active, storyboard->GetCurrentState());
    });
    // Tick at clock 1.51s
    wh->SetTimeManagerClockOverrideConstant(1.51);
    wh->SynchronouslyTickUIThread(1);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SeekedToZero").GetString());

    LOG_OUTPUT(L"Update duration, restart, followed by SeekAlignedToLastTick");
    RunOnUIThread([&]()
    {
        storyboard->Stop();

        ::Windows::Foundation::TimeSpan span; span.Duration = 1000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);

        storyboard->Begin();

        ::Windows::Foundation::TimeSpan span2; span2.Duration = 500000L;
        storyboard->SeekAlignedToLastTick(span2);
        wh->SetTimeManagerClockOverrideConstant(2);

        VERIFY_ARE_EQUAL(ClockState::Active, storyboard->GetCurrentState());
    });
    // Tick at clock 2s
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SeekedToHalf").GetString());

    LOG_OUTPUT(L"Update duration, restart, followed by SeekAlignedToLastTick");
    RunOnUIThread([&]()
    {
        storyboard->Stop();

        ::Windows::Foundation::TimeSpan span; span.Duration = 1000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);

        storyboard->Begin();

        ::Windows::Foundation::TimeSpan span2; span2.Duration = 500000L;
        storyboard->SeekAlignedToLastTick(span2);
        wh->SetTimeManagerClockOverrideConstant(2.5);

        VERIFY_ARE_EQUAL(ClockState::Active, storyboard->GetCurrentState());
    });
    // Tick at clock 2.5s
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SeekedToHalf").GetString());
}

void DoubleAnimationTests::SeekAlignedToLastTickWUC()
{
    SeekAlignedToLastTick(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::CanvasLeftTopAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    Storyboard^ baseline = nullptr;
    Storyboard^ storyboard2 = nullptr;
    Storyboard^ storyboard3 = nullptr;

    // The initial value of the handoff animation depends on the source of the handoff. In order to make this predictable,
    // use a constant animation.
    LOG_OUTPUT(L"Canvas left, constant animation");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        baseline = ref new Storyboard();
        baseline->Children->Append(da);

        baseline->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"CanvasLeft");

    LOG_OUTPUT(L"Canvas left handoff");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 20000000L;    // 2 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da);

        storyboard2->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"CanvasLeftHandoff");

    LOG_OUTPUT(L"+Canvas top, key frames");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Top)");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 30000000L;  // Discrete jump to 105 at 3 seconds
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 35 * 3;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 100000000L; // Continues linearly to 350 at 10 seconds
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 35 * 10;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        storyboard3 = ref new Storyboard();
        storyboard3->Children->Append(da);

        storyboard3->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"CanvasLeftTop");
}

void DoubleAnimationTests::CanvasLeftTopAnimationWUC()
{
    CanvasLeftTopAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::ClipAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTargetClip.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ clipStoryboard = nullptr;
    Storyboard^ transformStoryboard = nullptr;
    Canvas^ animatedCanvas = nullptr;

    LOG_OUTPUT(L"Clip TranslateY");
    RunOnUIThread([&]()
    {
        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"clipTx"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"TranslateY");

        clipStoryboard = ref new Storyboard();
        clipStoryboard->Children->Append(da);

        clipStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"TranslateClip").GetString());

    LOG_OUTPUT(L"Clip ScaleX animation");
    RunOnUIThread([&]()
    {
        clipStoryboard->Stop();

        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"clipTx"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 2.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"ScaleX");

        clipStoryboard = ref new Storyboard();
        clipStoryboard->Children->Append(da);

        clipStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ScaleClip").GetString());

    LOG_OUTPUT(L"Clip Rotate animation, static center");
    RunOnUIThread([&]()
    {
        clipStoryboard->Stop();

        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"clipTx"));

        target->CenterX = 10;
        target->CenterY = 50;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 75.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Rotation");

        clipStoryboard = ref new Storyboard();
        clipStoryboard->Children->Append(da);

        clipStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotateClip").GetString());

    LOG_OUTPUT(L"Clip Skew AngleX animation");
    RunOnUIThread([&]()
    {
        clipStoryboard->Stop();

        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"clipTx"));
        target->CenterX = 0;
        target->CenterY = 0;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 45.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"SkewX");

        clipStoryboard = ref new Storyboard();
        clipStoryboard->Children->Append(da);

        clipStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"SkewClip").GetString());

    LOG_OUTPUT(L"Transform TranslateY, Clip ScaleX");
    RunOnUIThread([&]()
    {
        clipStoryboard->Stop();

        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"canvTx"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"TranslateY");

        transformStoryboard = ref new Storyboard();
        transformStoryboard->Children->Append(da);

        transformStoryboard->Begin();
        clipStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"TrasnformTranslateY_ClipSCaleX").GetString());

    LOG_OUTPUT(L"Unanimated clip on a comp node - clip visual has no transform set");
    RunOnUIThread([&]()
    {
        clipStoryboard->Stop();
        transformStoryboard->Stop();

        animatedCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        animatedCanvas->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"StaticClipOnCompNode").GetString());
}

void DoubleAnimationTests::ClipAnimationWUC()
{
    ClipAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::TestFillBehavior(
    Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
    Microsoft::UI::Xaml::Media::Animation::FillBehavior animFillBehavior,
    Microsoft::UI::Xaml::Media::Animation::FillBehavior sbFillBehavior,
    double expectedUIThreadValue,
    bool forceCompNode,
    Platform::String^ animationEndFileName,
    double staticOverrideValue,
    Platform::String^ staticOverrideFileName
    )
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Storyboard^ storyboard = nullptr;
    double uiThreadValue = -1;

    wh->SetTimeManagerClockOverrideConstant(0);

    RunOnUIThread([&]()
    {
        Canvas^ canv = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        canv->CompositeMode = forceCompNode ? ElementCompositeMode::SourceOver : ElementCompositeMode::Inherit;

        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        target->X = 0;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 1000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        da->FillBehavior = animFillBehavior;
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->FillBehavior = sbFillBehavior;
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    // Tick at clock 0s
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, forceCompNode ? L"ActiveCompNode" : L"Active");

    // Tick at clock 1s = storyboard expires
    wh->SetTimeManagerClockOverrideConstant(1);
    wh->FireDCompAnimationCompleted(storyboard);
    wh->SynchronouslyTickUIThread(1);
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        uiThreadValue = target->X;
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, animationEndFileName);
    VERIFY_ARE_EQUAL(expectedUIThreadValue, uiThreadValue);

    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        target->X = staticOverrideValue;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, staticOverrideFileName);

    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        uiThreadValue = target->X;
    });
    VERIFY_ARE_EQUAL(staticOverrideValue, uiThreadValue);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::FillBehavior(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"[Idle]");
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    LOG_OUTPUT(L"[HoldEnd - element no longer has a comp node after animation ends]");
    TestFillBehavior(rootCanvas, FillBehavior::HoldEnd, FillBehavior::HoldEnd, 100, false, L"HoldEndNoCompNode", 4321, "HoldEndNoCompNode_SO");

    LOG_OUTPUT(L"[HoldEnd - element still has a comp node after animation ends]");
    TestFillBehavior(rootCanvas, FillBehavior::HoldEnd, FillBehavior::HoldEnd, 100, true, L"HoldEndWithCompNode", 4321, "HoldEndWithCompNode_SO");

    LOG_OUTPUT(L"[Animation Stop - element no longer has a comp node after animation ends]");
    TestFillBehavior(rootCanvas, FillBehavior::Stop, FillBehavior::HoldEnd, 0, false, L"StopNoCompNode", 4321, "StopNoCompNode_SO");

    LOG_OUTPUT(L"[Animation Stop - element still has a comp node after animation ends]");
    TestFillBehavior(rootCanvas, FillBehavior::Stop, FillBehavior::HoldEnd, 0, true, L"StopWithCompNode", 4321, "StopWithCompNode_SO");

    LOG_OUTPUT(L"[Storyboard Stop - element no longer has a comp node after animation ends]");
    TestFillBehavior(rootCanvas, FillBehavior::HoldEnd, FillBehavior::Stop, 0, false, L"StopNoCompNode", 4321, "StopNoCompNode_SO");

    LOG_OUTPUT(L"[Storyboard Stop - element still has a comp node after animation ends]");
    TestFillBehavior(rootCanvas, FillBehavior::HoldEnd, FillBehavior::Stop, 0, true, L"StopWithCompNode", 4321, "StopWithCompNode_SO");
}

void DoubleAnimationTests::FillBehaviorWUC()
{
    FillBehavior(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::TestOpacity(
    Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
    bool forceCompNode,
    Platform::String^ animationFileName,
    Platform::String^ stopAnimationFileName,
    double from
    )
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Storyboard^ storyboard = nullptr;
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        target->CompositeMode = forceCompNode ? ElementCompositeMode::SourceOver : ElementCompositeMode::Inherit;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = from;
        da->To = 0.0;
        da->AutoReverse = true;
        ::Windows::Foundation::TimeSpan span; span.Duration = 5000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Opacity");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, animationFileName);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, stopAnimationFileName);
}

void DoubleAnimationTests::OpacityAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"> Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    LOG_OUTPUT(L"> Opacity, no forced comp node");
    TestOpacity(rootCanvas, false, L"Opacity", L"OpacityStop", 1.0);

    LOG_OUTPUT(L"> Opacity, forced comp node");
    TestOpacity(rootCanvas, true, L"OpacityForcedCompNode", L"OpacityStopForcedCompNode", 1.0);

    LOG_OUTPUT(L"> Infinite opacity");
    TestOpacity(rootCanvas, false, L"inf", L"inf-stop", 3.402823466e+38F);

    Storyboard^ storyboard = nullptr;
    RunOnUIThread([&]()
    {
        wf::TimeSpan span;

        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(border);
        wh->WindowContent = rootCanvas;

        DoubleAnimationUsingKeyFrames^ daukf = ref new DoubleAnimationUsingKeyFrames();
        Storyboard::SetTarget(daukf, border);
        Storyboard::SetTargetProperty(daukf, L"Opacity");

        span.Duration = 0L; KeyTime kt1; kt1.TimeSpan = span;
        DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
        kf1->Value = 1;
        kf1->KeyTime = kt1;
        daukf->KeyFrames->Append(kf1);

        span.Duration = 30000000L; KeyTime kt2; kt2.TimeSpan = span;
        DiscreteDoubleKeyFrame^ kf2 = ref new DiscreteDoubleKeyFrame();
        kf2->Value = 0;
        kf2->KeyTime = kt2;
        daukf->KeyFrames->Append(kf2);

        span.Duration = 40000000L; KeyTime kt3; kt3.TimeSpan = span;
        DiscreteDoubleKeyFrame^ kf3 = ref new DiscreteDoubleKeyFrame();
        kf3->Value = 1;
        kf3->KeyTime = kt3;
        daukf->KeyFrames->Append(kf3);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(daukf);

        LOG_OUTPUT(L"> Discrete opacity key frames - used by ProgressBar");
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "DiscreteKeyFrame");

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::OpacityAnimationWUC()
{
    OpacityAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::TrickyStoryboardsWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTargetAndZeroRepeat.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    Storyboard^ storyboard = nullptr;

    // Negative BeginTimes not allowed
    LOG_OUTPUT(L"[Cannot convert]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.5;
        da->To = 0.5;
        span.Duration = 20000000L; da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Opacity)");

        if (storyboard != nullptr) { storyboard->Stop(); }
        storyboard = ref new Storyboard();
        span.Duration = 10000000L; storyboard->Duration = DurationHelper::FromTimeSpan(span);
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ParentClipsChild");

    // Negative BeginTimes not allowed
    LOG_OUTPUT(L"[Negative BeginTime with repeat]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        span.Duration = 20000000L; da->Duration = DurationHelper::FromTimeSpan(span);
        span.Duration = -10000000L; da->BeginTime = span;
        da->RepeatBehavior = RepeatBehaviorHelper::FromCount(2);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        if (storyboard != nullptr) { storyboard->Stop(); }
        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NegativeBeginTime-Repeat");

    LOG_OUTPUT(L"[Repeat 0x]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = safe_cast<DoubleAnimation^>(rootCanvas->FindName(L"zeroRepeatDA"));
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        if (storyboard != nullptr) { storyboard->Stop(); }
        storyboard = safe_cast<Storyboard^>(rootCanvas->FindName(L"zeroRepeatSB"));

        // WUC does not allow 0x repeats, but Xaml will not try to create a WUC animation because we'll complete the animation instantly.
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Repeat0");

    LOG_OUTPUT(L"[Duration 0]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da1 = ref new DoubleAnimation();
        da1->From = 50.0;
        da1->To = 50.0;
        span.Duration = 20000000L; da1->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da1, target);
        Storyboard::SetTargetProperty(da1, L"(Canvas.Left)");

        // This animation has 0 duration, but it still creates a WUC animation because it's part of a storyboard that has a non-zero duration.
        // Anything with explicit Duration="0" will be detected and skipped. We need a key frame animation that infers its duration from a single
        // key frame at time t=0.
        DoubleAnimationUsingKeyFrames^ da2 = ref new DoubleAnimationUsingKeyFrames();
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"(Canvas.Top)");

        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        span.Duration = 0L; KeyTime kt1; kt1.TimeSpan = span;
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da2->KeyFrames->Append(kf1);

        // Explicit duration="0" animation, to verify that this generates nothing
        DoubleAnimation^ da3 = ref new DoubleAnimation();
        da3->From = 1.0;
        da3->To = 1.0;
        span.Duration = 0L; da3->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da3, target);
        Storyboard::SetTargetProperty(da3, L"Opacity");

        if (storyboard != nullptr) { storyboard->Stop(); }
        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);
        storyboard->Children->Append(da2);
        storyboard->Children->Append(da3);

        // WUC does not allow 0x repeats, but Xaml will not try to create a WUC animation because we'll complete the animation instantly.
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Duration0");

    LOG_OUTPUT(L"[Inferred duration]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimationUsingKeyFrames^ da1 = ref new DoubleAnimationUsingKeyFrames();
        // No duration
        da1->SpeedRatio = 2.0f;
        Storyboard::SetTarget(da1, target);
        Storyboard::SetTargetProperty(da1, L"(Canvas.Left)");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da1->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 20000000L;
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 50;
        kf2->KeyTime = kt2;
        da1->KeyFrames->Append(kf2);

        ::Windows::Foundation::TimeSpan s3; s3.Duration = 30000000L;
        KeyTime kt3; kt3.TimeSpan = s3;
        LinearDoubleKeyFrame^ kf3 = ref new LinearDoubleKeyFrame();
        kf3->Value = 150;
        kf3->KeyTime = kt3;
        da1->KeyFrames->Append(kf3);

        if (storyboard != nullptr) { storyboard->Stop(); }
        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);

        // We should infer the duration of the key frame animation from its key frames.
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"InferredDuration");

    LOG_OUTPUT(L"[Animation duration forever]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da1 = ref new DoubleAnimation();
        da1->From = 50.0;
        da1->To = 50.0;
        da1->Duration = DurationHelper::Forever;
        Storyboard::SetTarget(da1, target);
        Storyboard::SetTargetProperty(da1, L"(Canvas.Left)");

        if (storyboard != nullptr) { storyboard->Stop(); }
        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"DurationForever");
}

void DoubleAnimationTests::CompleteAnimationWhileInvisibleWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"[Idle]");
    wh->SynchronouslyTickUIThread(2);

    wh->SetTimeManagerClockOverrideConstant(0);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Make window invisible to disable rendering]");
    wh->SetIsRenderEnabled(false);

    LOG_OUTPUT(L"[Kick off an animation]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        span.Duration = 10000000L; da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        span.Duration = 10000000L; storyboard->Duration = DurationHelper::FromTimeSpan(span);
        storyboard->Children->Append(da);

        storyboard->Begin();
    });

    LOG_OUTPUT(L"[Let animation tick to push WUC animation on target and to add target to a list of WUC animated targets]");
    // but since the window isn't visible, we won't render or submit a frame, which means the target doesn't get to update
    // its WUC expression and keeps its "animation is dirty" flag set.
    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"[Pause the animation]");
    RunOnUIThread([&]()
    {
        storyboard->Pause();
    });
    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"[Make window visible to enable rendering again]");
    wh->SetIsRenderEnabled(true);

    LOG_OUTPUT(L"[This should start a WUC animation that still finds the corresponding Xaml animation]");
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void DoubleAnimationTests::NegativeKeyTimesWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Negative key frame]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        // This animation has 0 duration, but it still creates a WUC animation because it's part of a storyboard that has a non-zero duration.
        // Anything with explicit Duration="0" will be detected and skipped. We need a key frame animation that infers its duration from a single
        // key frame at time t=0.
        DoubleAnimationUsingKeyFrames^ da1 = ref new DoubleAnimationUsingKeyFrames();
        Storyboard::SetTarget(da1, target);
        Storyboard::SetTargetProperty(da1, L"(Canvas.Left)");

        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        span.Duration = -10000000L; KeyTime kt1; kt1.TimeSpan = span;
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da1->KeyFrames->Append(kf1);

        LinearDoubleKeyFrame^ kf3 = ref new LinearDoubleKeyFrame();
        span.Duration = 10000000L; KeyTime kt3; kt3.TimeSpan = span;
        kf3->Value = 100;
        kf3->KeyTime = kt3;
        da1->KeyFrames->Append(kf3);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"[Negative + zero key frame]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        // This animation has 0 duration, but it still creates a WUC animation because it's part of a storyboard that has a non-zero duration.
        // Anything with explicit Duration="0" will be detected and skipped. We need a key frame animation that infers its duration from a single
        // key frame at time t=0.
        DoubleAnimationUsingKeyFrames^ da1 = ref new DoubleAnimationUsingKeyFrames();
        Storyboard::SetTarget(da1, target);
        Storyboard::SetTargetProperty(da1, L"(Canvas.Left)");

        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        span.Duration = -10000000L; KeyTime kt1; kt1.TimeSpan = span;
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da1->KeyFrames->Append(kf1);

        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        span.Duration = 0L; KeyTime kt2; kt2.TimeSpan = span;
        kf2->Value = 100;
        kf2->KeyTime = kt2;
        da1->KeyFrames->Append(kf2);

        LinearDoubleKeyFrame^ kf3 = ref new LinearDoubleKeyFrame();
        span.Duration = 10000000L; KeyTime kt3; kt3.TimeSpan = span;
        kf3->Value = 100;
        kf3->KeyTime = kt3;
        da1->KeyFrames->Append(kf3);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void DoubleAnimationTests::EasingFunctionsWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Elastic ease]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        span.Duration = 20000000L; da->Duration = DurationHelper::FromTimeSpan(span);
        CircleEase^ ease = ref new CircleEase();
        da->EasingFunction = ease;
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"CircleEase");

    LOG_OUTPUT(L"[EasingDoubleKeyFrame]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        span.Duration = 30000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        EasingDoubleKeyFrame^ kf1 = ref new EasingDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        ExponentialEase^ ease1 = ref new ExponentialEase();
        ease1->Exponent = 1.25f;
        ease1->EasingMode = EasingMode::EaseIn;
        kf1->EasingFunction = ease1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 20000000L;
        KeyTime kt2; kt2.TimeSpan = s2;
        EasingDoubleKeyFrame^ kf2 = ref new EasingDoubleKeyFrame();
        kf2->Value = 50;
        kf2->KeyTime = kt2;
        CubicEase^ ease2 = ref new CubicEase();
        kf2->EasingFunction = ease2;
        da->KeyFrames->Append(kf2);

        ::Windows::Foundation::TimeSpan s3; s3.Duration = 30000000L;
        KeyTime kt3; kt3.TimeSpan = s3;
        EasingDoubleKeyFrame^ kf3 = ref new EasingDoubleKeyFrame();
        kf3->Value = 150;
        kf3->KeyTime = kt3;
        ElasticEase^ ease3 = ref new ElasticEase();
        ease3->EasingMode = EasingMode::EaseInOut;
        ease3->Oscillations = 2;
        ease3->Springiness = 0.7f;
        kf3->EasingFunction = ease3;
        da->KeyFrames->Append(kf3);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EasingDoubleKeyFrame");
}

void DoubleAnimationTests::ProjectionAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTargetProjection.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ innerStoryboard = nullptr;
    Storyboard^ outerStoryboard = nullptr;
    Canvas^ canvasTarget = nullptr;

    LOG_OUTPUT(L"LocalOffsetX");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"LocalOffsetX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"LocalOffsetX").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    LOG_OUTPUT(L"GlobalOffsetYZ");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"GlobalOffsetY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 100.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"GlobalOffsetZ");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"GlobalOffsetYZ").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    LOG_OUTPUT(L"RotationX, CenterOfRotationX");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 90.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationX");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 1.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"CenterOfRotationX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotationXCenterX").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    LOG_OUTPUT(L"RotationX, CenterOfRotationX, Height goes to 0");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));
        canvasTarget = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 90.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationX");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 1.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 100000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"CenterOfRotationX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set height to 0 while CenterOfRotationX is animated (uses special handling in expression, should not crash)");
        canvasTarget->Height = 0;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotationXCenterXZeroHeight").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
        canvasTarget->Height = 100;
    });

    LOG_OUTPUT(L"RotationY, CenterOfRotationY, Width goes to 0");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));
        canvasTarget = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 90.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 1.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"CenterOfRotationY");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set width to 0 while CenterOfRotationY is animated (uses special handling in expression, should not crash)");
        canvasTarget->Height = 0;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotationYCenterYZeroWidth").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
        canvasTarget->Height = 100;
    });

    LOG_OUTPUT(L"RotationX, RotationY, CenterOfRotationX, CenterOfRotationY, Height and Width go to 0");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));
        canvasTarget = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 90.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationX");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 1.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"CenterOfRotationX");

        DoubleAnimation^ da3 = ref new DoubleAnimation();
        da3->From = 0.0;
        da3->To = 90.0;
        ::Windows::Foundation::TimeSpan span3; span3.Duration = 10000000L;
        da3->Duration = DurationHelper::FromTimeSpan(span3);
        Storyboard::SetTarget(da3, target);
        Storyboard::SetTargetProperty(da3, L"RotationY");

        DoubleAnimation^ da4 = ref new DoubleAnimation();
        da4->From = 0.0;
        da4->To = 1.0;
        ::Windows::Foundation::TimeSpan span4; span4.Duration = 10000000L;
        da4->Duration = DurationHelper::FromTimeSpan(span4);
        Storyboard::SetTarget(da4, target);
        Storyboard::SetTargetProperty(da4, L"CenterOfRotationY");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);
        innerStoryboard->Children->Append(da3);
        innerStoryboard->Children->Append(da4);

        innerStoryboard->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set height and width to 0 while CenterOfRotationX and CenterOfRotationY are animated (both RotateCenter and UndoRotateCenter should be dropped)");
        canvasTarget->Width = 0;
        canvasTarget->Height = 0;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotationXCenterXRotationYCenterYZeroWidthAndHeight").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
        canvasTarget->Width = 100;
        canvasTarget->Height = 100;
    });

    LOG_OUTPUT(L"Nested projection animations");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));
        PlaneProjection^ target2 = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"outerProj"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 45.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 45.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target2);
        Storyboard::SetTargetProperty(da2, L"RotationY");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);

        outerStoryboard = ref new Storyboard();
        outerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
        outerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Nested").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
        outerStoryboard->Stop();
        outerStoryboard = nullptr;
    });

    LOG_OUTPUT(L"Every projection animation");
    RunOnUIThread([&]()
    {
        PlaneProjection^ target = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"proj"));
        PlaneProjection^ target2 = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"outerProj"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 45.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 100.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"GlobalOffsetZ");

        DoubleAnimation^ da3 = ref new DoubleAnimation();
        da3->From = 0.0;
        da3->To = -100.0;
        ::Windows::Foundation::TimeSpan span3; span3.Duration = 10000000L;
        da3->Duration = DurationHelper::FromTimeSpan(span3);
        Storyboard::SetTarget(da3, target);
        Storyboard::SetTargetProperty(da3, L"LocalOffsetX");

        DoubleAnimation^ da4 = ref new DoubleAnimation();
        da4->From = 1.0;
        da4->To = 0.0;
        ::Windows::Foundation::TimeSpan span4; span4.Duration = 10000000L;
        da4->Duration = DurationHelper::FromTimeSpan(span4);
        Storyboard::SetTarget(da4, target);
        Storyboard::SetTargetProperty(da4, L"CenterOfRotationX");

        DoubleAnimation^ da5 = ref new DoubleAnimation();
        da5->From = 0.0;
        da5->To = -45.0;
        ::Windows::Foundation::TimeSpan span5; span5.Duration = 10000000L;
        da5->Duration = DurationHelper::FromTimeSpan(span5);
        Storyboard::SetTarget(da5, target2);
        Storyboard::SetTargetProperty(da5, L"RotationX");

        DoubleAnimation^ da6 = ref new DoubleAnimation();
        da6->From = 0.0;
        da6->To = -100.0;
        ::Windows::Foundation::TimeSpan span6; span6.Duration = 10000000L;
        da6->Duration = DurationHelper::FromTimeSpan(span6);
        Storyboard::SetTarget(da6, target2);
        Storyboard::SetTargetProperty(da6, L"GlobalOffsetX");

        DoubleAnimation^ da7 = ref new DoubleAnimation();
        da7->From = 0.0;
        da7->To = 100.0;
        ::Windows::Foundation::TimeSpan span7; span7.Duration = 10000000L;
        da7->Duration = DurationHelper::FromTimeSpan(span7);
        Storyboard::SetTarget(da7, target2);
        Storyboard::SetTargetProperty(da7, L"LocalOffsetZ");

        DoubleAnimation^ da8 = ref new DoubleAnimation();
        da8->From = 0.0;
        da8->To = 1.0;
        ::Windows::Foundation::TimeSpan span8; span8.Duration = 10000000L;
        da8->Duration = DurationHelper::FromTimeSpan(span8);
        Storyboard::SetTarget(da8, target2);
        Storyboard::SetTargetProperty(da8, L"CenterOfRotationZ");

        DoubleAnimation^ da9 = ref new DoubleAnimation();
        da9->From = 0.0;
        da9->To = 60.0;
        ::Windows::Foundation::TimeSpan span9; span9.Duration = 10000000L;
        da9->Duration = DurationHelper::FromTimeSpan(span9);
        Storyboard::SetTarget(da9, target2);
        Storyboard::SetTargetProperty(da9, L"RotationZ");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);
        innerStoryboard->Children->Append(da3);
        innerStoryboard->Children->Append(da4);

        outerStoryboard = ref new Storyboard();
        outerStoryboard->Children->Append(da5);
        outerStoryboard->Children->Append(da6);
        outerStoryboard->Children->Append(da7);
        outerStoryboard->Children->Append(da8);
        outerStoryboard->Children->Append(da9);

        innerStoryboard->Begin();
        outerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Combo").GetString());

    // Explicitly stop the running storyboards. DComp's scoped batches will stay alive as long as they're running,
    // which can cause false positives in leak detection. Relying on the scope guard to stop outstanding animations
    // doesn't give DComp enough time to complete the scoped batches, so do it as part of the test.
    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        outerStoryboard->Stop();
    });
    wh->WaitForIdle();
}

void DoubleAnimationTests::ProjectionAnimationWUC()
{
    ProjectionAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::ProjectionAnimationOnZeroSizedElementWUC()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto daXCompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); const auto& daXCompletedEvent = std::make_shared<Event>();
    auto daYCompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); const auto& daYCompletedEvent = std::make_shared<Event>();
    auto daZCompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); const auto& daZCompletedEvent = std::make_shared<Event>();
    auto daCenterXCompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); const auto& daCenterXCompletedEvent = std::make_shared<Event>();
    auto daCenterYCompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); const auto& daCenterYCompletedEvent = std::make_shared<Event>();
    auto daCenterZCompletedRegistration = CreateSafeEventRegistration(DoubleAnimation, Completed); const auto& daCenterZCompletedEvent = std::make_shared<Event>();

    PlaneProjection^ target = nullptr;
    Canvas^ rootCanvas = nullptr;
    Canvas^ canvas = nullptr;
    RunOnUIThread([&]()
    {
        target = ref new PlaneProjection();

        canvas = ref new Canvas();
        canvas->Width = 0;  // Zero sized canvas
        canvas->Height = 0;
        canvas->Projection = target;

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(canvas);

        wh->WindowContent = rootCanvas;
    });
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    RunOnUIThread([&]()
    {
        // These animations should all be skipped because their target is zero sized...
        DoubleAnimation^ daX = AnimationTestHelper::MakeDoubleAnimation(target, "RotationX");
        DoubleAnimation^ daY = AnimationTestHelper::MakeDoubleAnimation(target, "RotationY");
        DoubleAnimation^ daZ = AnimationTestHelper::MakeDoubleAnimation(target, "RotationZ");
        DoubleAnimation^ daCenterX = AnimationTestHelper::MakeDoubleAnimation(target, "CenterOfRotationX");
        DoubleAnimation^ daCenterY = AnimationTestHelper::MakeDoubleAnimation(target, "CenterOfRotationY");
        DoubleAnimation^ daCenterZ = AnimationTestHelper::MakeDoubleAnimation(target, "CenterOfRotationZ");

        // ...but they should still fire Completed events.
        daXCompletedRegistration.Attach(daX, ref new wf::EventHandler<Object^>([daXCompletedEvent](Object^ sender, Object^ e) { daXCompletedEvent->Set(); }));
        daYCompletedRegistration.Attach(daY, ref new wf::EventHandler<Object^>([daYCompletedEvent](Object^ sender, Object^ e) { daYCompletedEvent->Set(); }));
        daZCompletedRegistration.Attach(daZ, ref new wf::EventHandler<Object^>([daZCompletedEvent](Object^ sender, Object^ e) { daZCompletedEvent->Set(); }));
        daCenterXCompletedRegistration.Attach(daCenterX, ref new wf::EventHandler<Object^>([daCenterXCompletedEvent](Object^ sender, Object^ e) { daCenterXCompletedEvent->Set(); }));
        daCenterYCompletedRegistration.Attach(daCenterY, ref new wf::EventHandler<Object^>([daCenterYCompletedEvent](Object^ sender, Object^ e) { daCenterYCompletedEvent->Set(); }));
        daCenterZCompletedRegistration.Attach(daCenterZ, ref new wf::EventHandler<Object^>([daCenterZCompletedEvent](Object^ sender, Object^ e) { daCenterZCompletedEvent->Set(); }));

        storyboard = ref new Storyboard();
        storyboard->Children->Append(daX);
        storyboard->Children->Append(daY);
        storyboard->Children->Append(daZ);
        storyboard->Children->Append(daCenterX);
        storyboard->Children->Append(daCenterY);
        storyboard->Children->Append(daCenterZ);

        storyboard->Begin();
    });

    LOG_OUTPUT(L"Waiting for RotationX completed");
    daXCompletedEvent->WaitForDefault();
    LOG_OUTPUT(L"Waiting for RotationY completed");
    daYCompletedEvent->WaitForDefault();
    LOG_OUTPUT(L"Waiting for RotationZ completed");
    daZCompletedEvent->WaitForDefault();
    LOG_OUTPUT(L"Waiting for CenterOfRotationX completed");
    daCenterXCompletedEvent->WaitForDefault();
    LOG_OUTPUT(L"Waiting for CenterOfRotationY completed");
    daCenterYCompletedEvent->WaitForDefault();
    LOG_OUTPUT(L"Waiting for CenterOfRotationZ completed");
    daCenterZCompletedEvent->WaitForDefault();

    VERIFY_IS_TRUE(daXCompletedEvent->HasFired());
    VERIFY_IS_TRUE(daYCompletedEvent->HasFired());
    VERIFY_IS_TRUE(daZCompletedEvent->HasFired());
    VERIFY_IS_TRUE(daCenterXCompletedEvent->HasFired());
    VERIFY_IS_TRUE(daCenterYCompletedEvent->HasFired());
    VERIFY_IS_TRUE(daCenterZCompletedEvent->HasFired());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::Transform3DAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTargetTransform3D.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ innerStoryboard = nullptr;
    Storyboard^ outerStoryboard = nullptr;

    LOG_OUTPUT(L"TranslateX");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"TranslateX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"TranslateX").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"ScaleX");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"ScaleX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ScaleX").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"RotationX");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 90.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotationX").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"TranslateYCenterY");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 10.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"TranslateY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 1.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"CenterY");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"TranslateYCenterY").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"RotationYCenterX");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 90.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 1.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"CenterX");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);
        innerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"RotationYCenterX").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
    });

    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"Nested CompositeTransform3D animations");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));
        CompositeTransform3D^ target2 = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"innerTrans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 45.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 45.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target2);
        Storyboard::SetTargetProperty(da2, L"RotationY");

        innerStoryboard = ref new Storyboard();
        innerStoryboard->Children->Append(da);

        outerStoryboard = ref new Storyboard();
        outerStoryboard->Children->Append(da2);

        innerStoryboard->Begin();
        outerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Nested3D").GetString());

    RunOnUIThread([&]()
    {
        innerStoryboard->Stop();
        innerStoryboard = nullptr;
        outerStoryboard->Stop();
        outerStoryboard = nullptr;
    });

    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"All CompositeTransform3D animations");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da0 = ref new DoubleAnimation();
        da0->From = 0.0;
        da0->To = 45.0;
        ::Windows::Foundation::TimeSpan span0; span0.Duration = 10000000L;
        da0->Duration = DurationHelper::FromTimeSpan(span0);
        Storyboard::SetTarget(da0, target);
        Storyboard::SetTargetProperty(da0, L"ScaleX");

        DoubleAnimation^ da1 = ref new DoubleAnimation();
        da1->From = 0.0;
        da1->To = 45.0;
        ::Windows::Foundation::TimeSpan span1; span1.Duration = 10000000L;
        da1->Duration = DurationHelper::FromTimeSpan(span1);
        Storyboard::SetTarget(da1, target);
        Storyboard::SetTargetProperty(da1, L"ScaleY");

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 45.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 10000000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target);
        Storyboard::SetTargetProperty(da2, L"ScaleZ");


        DoubleAnimation^ da3 = ref new DoubleAnimation();
        da3->From = 0.0;
        da3->To = 45.0;
        ::Windows::Foundation::TimeSpan span3; span3.Duration = 10000000L;
        da3->Duration = DurationHelper::FromTimeSpan(span3);
        Storyboard::SetTarget(da3, target);
        Storyboard::SetTargetProperty(da3, L"RotationX");

        DoubleAnimation^ da4 = ref new DoubleAnimation();
        da4->From = 0.0;
        da4->To = 45.0;
        ::Windows::Foundation::TimeSpan span4; span4.Duration = 10000000L;
        da4->Duration = DurationHelper::FromTimeSpan(span4);
        Storyboard::SetTarget(da4, target);
        Storyboard::SetTargetProperty(da4, L"RotationY");

        DoubleAnimation^ da5 = ref new DoubleAnimation();
        da5->From = 0.0;
        da5->To = 45.0;
        ::Windows::Foundation::TimeSpan span5; span5.Duration = 10000000L;
        da5->Duration = DurationHelper::FromTimeSpan(span5);
        Storyboard::SetTarget(da5, target);
        Storyboard::SetTargetProperty(da5, L"RotationZ");


        DoubleAnimation^ da6 = ref new DoubleAnimation();
        da6->From = 0.0;
        da6->To = 45.0;
        ::Windows::Foundation::TimeSpan span6; span6.Duration = 10000000L;
        da6->Duration = DurationHelper::FromTimeSpan(span6);
        Storyboard::SetTarget(da6, target);
        Storyboard::SetTargetProperty(da6, L"TranslateX");

        DoubleAnimation^ da7 = ref new DoubleAnimation();
        da7->From = 0.0;
        da7->To = 45.0;
        ::Windows::Foundation::TimeSpan span7; span7.Duration = 10000000L;
        da7->Duration = DurationHelper::FromTimeSpan(span7);
        Storyboard::SetTarget(da7, target);
        Storyboard::SetTargetProperty(da7, L"TranslateY");

        DoubleAnimation^ da8 = ref new DoubleAnimation();
        da8->From = 0.0;
        da8->To = 45.0;
        ::Windows::Foundation::TimeSpan span8; span8.Duration = 10000000L;
        da8->Duration = DurationHelper::FromTimeSpan(span8);
        Storyboard::SetTarget(da8, target);
        Storyboard::SetTargetProperty(da8, L"TranslateZ");

        DoubleAnimation^ da9 = ref new DoubleAnimation();
        da9->From = 0.0;
        da9->To = 45.0;
        ::Windows::Foundation::TimeSpan span9; span9.Duration = 10000000L;
        da9->Duration = DurationHelper::FromTimeSpan(span9);
        Storyboard::SetTarget(da9, target);
        Storyboard::SetTargetProperty(da9, L"CenterX");

        DoubleAnimation^ da10 = ref new DoubleAnimation();
        da10->From = 0.0;
        da10->To = 45.0;
        ::Windows::Foundation::TimeSpan span10; span10.Duration = 10000000L;
        da10->Duration = DurationHelper::FromTimeSpan(span10);
        Storyboard::SetTarget(da10, target);
        Storyboard::SetTargetProperty(da10, L"CenterY");

        DoubleAnimation^ da11 = ref new DoubleAnimation();
        da11->From = 0.0;
        da11->To = 45.0;
        ::Windows::Foundation::TimeSpan span11; span11.Duration = 10000000L;
        da11->Duration = DurationHelper::FromTimeSpan(span11);
        Storyboard::SetTarget(da11, target);
        Storyboard::SetTargetProperty(da11, L"CenterZ");

        innerStoryboard = ref new Storyboard();
        outerStoryboard = ref new Storyboard();

        innerStoryboard->Children->Append(da0);
        outerStoryboard->Children->Append(da1);
        innerStoryboard->Children->Append(da2);
        outerStoryboard->Children->Append(da3);
        innerStoryboard->Children->Append(da4);
        outerStoryboard->Children->Append(da5);
        innerStoryboard->Children->Append(da6);
        outerStoryboard->Children->Append(da7);
        innerStoryboard->Children->Append(da8);
        outerStoryboard->Children->Append(da9);
        innerStoryboard->Children->Append(da10);
        outerStoryboard->Children->Append(da11);

        innerStoryboard->Begin();
        outerStoryboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"All3D").GetString());
}

void DoubleAnimationTests::Transform3DAnimationWUC()
{
    Transform3DAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::Transform3DAnimationLifecycle()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTargetTransform3D.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"First, add an animation to a CompositeTransform3D");
    RunOnUIThread([&]()
    {
        CompositeTransform3D^ target = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"trans"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 10.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"RotationX");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"HasAnimation").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
        storyboard = nullptr;
    });

    LOG_OUTPUT(L"Stop and remove the animation. The CompositeTransform3D should now have an empty rotate");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"DoesntHaveAnimation").GetString());
}

void DoubleAnimationTests::AnimationReadback(bool isKeyFrame)
{
    const auto& wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
        wh->DetachMockDComp();
    });

    RuntimeEnabledFeatureOverride featureDCompAnimations;
    wh->InjectMockDComp();

    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ sb = nullptr;

    {
        LOG_OUTPUT(L"Initial value should not have the animated value");

        double value;
        RunOnUIThread([&]()
        {
            TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

            sb = ref new Storyboard();

            if (isKeyFrame)
            {
                DoubleAnimationUsingKeyFrames^ anim = ref new DoubleAnimationUsingKeyFrames();
                ::Windows::Foundation::TimeSpan span;
                span.Duration = 360000000000L;
                anim->Duration = DurationHelper::FromTimeSpan(span);

                ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
                KeyTime kt1; kt1.TimeSpan = s1;
                DiscreteDoubleKeyFrame^ kf1 = ref new DiscreteDoubleKeyFrame();
                kf1->Value = 100;
                kf1->KeyTime = kt1;
                anim->KeyFrames->Append(kf1);

                ::Windows::Foundation::TimeSpan s2; s2.Duration = 180000000000L;
                KeyTime kt2; kt2.TimeSpan = s2;
                DiscreteDoubleKeyFrame^ kf2 = ref new DiscreteDoubleKeyFrame();
                kf2->Value = 75;
                kf2->KeyTime = kt2;
                anim->KeyFrames->Append(kf2);

                ::Windows::Foundation::TimeSpan s3; s3.Duration = 360000000000L;
                KeyTime kt3; kt3.TimeSpan = s3;
                DiscreteDoubleKeyFrame^ kf3 = ref new DiscreteDoubleKeyFrame();
                kf3->Value = 50;
                kf3->KeyTime = kt3;
                anim->KeyFrames->Append(kf3);

                Storyboard::SetTarget(anim, target);
                Storyboard::SetTargetProperty(anim, L"X");
                sb->Children->Append(anim);
            }
            else
            {
                DoubleAnimation^ anim = ref new DoubleAnimation();
                ::Windows::Foundation::TimeSpan span;
                span.Duration = 360000000000L;
                anim->Duration = DurationHelper::FromTimeSpan(span);
                anim->From = 100.0;
                anim->To = 50.0;

                Storyboard::SetTarget(anim, target);
                Storyboard::SetTargetProperty(anim, L"X");
                sb->Children->Append(anim);
            }

            sb->Begin();

            value = target->X;
        });
        VERIFY_ARE_EQUAL(0, value);
    }

    {
        LOG_OUTPUT(L"Animation should apply value while ticking");
        wh->SynchronouslyTickUIThread(2);

        double value;
        RunOnUIThread([&]()
        {
            value = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"))->X;
        });
        VERIFY_IS_GREATER_THAN_OR_EQUAL(100, value);
        VERIFY_IS_LESS_THAN_OR_EQUAL(99.9, value);
    }

    {
        LOG_OUTPUT(L"Animation should not apply value after seeking");

        double value;
        RunOnUIThread([&]()
        {
            ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 180000000000L;
            sb->Seek(seekSpan);

            value = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"))->X;
        });
        VERIFY_IS_GREATER_THAN_OR_EQUAL(100, value);
        VERIFY_IS_LESS_THAN_OR_EQUAL(99.9, value);

    }

    {
        LOG_OUTPUT(L"Animation should apply value after seeking and ticking");
        wh->SynchronouslyTickUIThread(2);

        double value;
        RunOnUIThread([&]()
        {
            value = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"))->X;
        });
        VERIFY_IS_GREATER_THAN_OR_EQUAL(75.1, value);
        VERIFY_IS_LESS_THAN_OR_EQUAL(74.9, value);
    }

    {
        LOG_OUTPUT(L"Animation should apply value after SeekAlignedToLastTick");

        double value;
        RunOnUIThread([&]()
        {
            ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 10000L;
            sb->SeekAlignedToLastTick(seekSpan);

            value = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"))->X;
        });
        VERIFY_IS_GREATER_THAN_OR_EQUAL(100, value);
        VERIFY_IS_LESS_THAN_OR_EQUAL(99.9, value);
    }

    {
        LOG_OUTPUT(L"Seeking to Filling");

        RunOnUIThread([&]()
        {
            ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 360500000000L;
            sb->Seek(seekSpan);
        });

        wh->SynchronouslyTickUIThread(2);

        double value;
        RunOnUIThread([&]()
        {
            value = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"))->X;
        });
        VERIFY_ARE_EQUAL(50, value);
    }

    {
        LOG_OUTPUT(L"Animation should not apply value after Stop");

        double value;
        RunOnUIThread([&]()
        {
            sb->Stop();

            value = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"))->X;
        });
        VERIFY_ARE_EQUAL(0, value);
    }
}

void DoubleAnimationTests::LinearAnimationReadback()
{
    AnimationReadback(false);
}

void DoubleAnimationTests::KeyFrameAnimationReadback()
{
    AnimationReadback(true);
}

void DoubleAnimationTests::RestartAnimationWithBaseValueReadback()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    // The initial value of the handoff animation depends on the source of the handoff. In order to make this predictable,
    // use a constant animation.
    LOG_OUTPUT(L"Initial Value is 5");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        target->X = 5;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->To = 10.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 1 second
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"From5").GetString());

    LOG_OUTPUT(L"Stop animation");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Static5").GetString());

    // The initial value of the handoff animation depends on the source of the handoff. In order to make this predictable,
    // use a constant animation.
    LOG_OUTPUT(L"Initial Value is 50");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        target->X = 50;

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"From50").GetString());
}

void DoubleAnimationTests::SwitchAnimationTargets()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;
    DoubleAnimation^ da1 = nullptr;
    DoubleAnimation^ da2 = nullptr;
    TranslateTransform^ target1 = nullptr;
    TranslateTransform^ target2 = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"da1 with target1, da2 with target2");
    RunOnUIThread([&]()
    {
        target1 = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));
        target2 = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr2"));

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 1 second

        da1 = ref new DoubleAnimation();
        da1->From = 0.0;
        da1->To = 10.0;
        da1->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da1, target1);
        Storyboard::SetTargetProperty(da1, L"X");

        da2 = ref new DoubleAnimation();
        da2->From = 10.0;
        da2->To = 0.0;
        da2->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da2, target2);
        Storyboard::SetTargetProperty(da2, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da1);
        storyboard->Children->Append(da2);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Before").GetString());

    LOG_OUTPUT(L"da1 with target2, da2 with target1");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
        Storyboard::SetTarget(da1, target2);
        Storyboard::SetTarget(da2, target1);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Switched").GetString());
}

void DoubleAnimationTests::OpacityAnimationHandoff()
{
    // With transform animations, the DComp transform object stays around even after the comp node is released, and the
    // DComp animation remains set on it. When a new animation begins, it can hand off from the previously attached DComp
    // animation.
    //
    // Opacity animations don't use an intermediate DComp object, they apply to the DComp visual directly. When the animation
    // goes into the Filling state, the DComp visual is deleted, along with the previous DComp animation. When a new animation
    // begins, there is no previous DComp animation to hand off from, so DComp uses the default opacity value of 1, even when
    // it shouldn't.
    //
    // This test sets up an animation of opacity 0 that goes into the filling state, then hands off from that to another
    // animation. The new animation is expected to start from 0.

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ baseline = nullptr;
    Storyboard^ storyboard = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->WaitForIdle();

    // The initial value of the handoff animation depends on the source of the handoff. In order to make this predictable,
    // use a constant animation.
    LOG_OUTPUT(L"Baseline animation");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 500000L;    // 50 ms
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Opacity");

        baseline = ref new Storyboard();
        baseline->Children->Append(da);

        baseline->Begin();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Baseline").GetString());

    LOG_OUTPUT(L"Handoff to opacity animation");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Opacity");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Handoff").GetString());
}

void DoubleAnimationTests::NoDCompCompletion()
{
    // If the DComp animation instance doesn't get a chance to complete, we don't want to leak a weak ref back to the animation.
    // Note: leak detection is currently not working, so this test will pass even if there is a leak. This is tracked by
    // <PostRunTests::CheckForLeaks doesn't work anymore>

    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    // 2 ticks - the animation doesn't get to finish
    wh->SynchronouslyTickUIThread(2);
}

void DoubleAnimationTests::NullDuration()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard = nullptr;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"DA").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"tr1"));

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"X");

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        da->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 20000000L;
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
        kf2->Value = 50;
        kf2->KeyTime = kt2;
        da->KeyFrames->Append(kf2);

        ::Windows::Foundation::TimeSpan s3; s3.Duration = 30000000L;
        KeyTime kt3; kt3.TimeSpan = s3;
        LinearDoubleKeyFrame^ kf3 = ref new LinearDoubleKeyFrame();
        kf3->Value = 150;
        kf3->KeyTime = kt3;
        da->KeyFrames->Append(kf3);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"DAUKF").GetString());
}

void DoubleAnimationTests::LeaveEnterTree()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"2Canvases.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Initial");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 50.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 1.0;
        span.Duration = 100000000L;    // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Opacity");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"Initial").GetString());

    LOG_OUTPUT(L"Reparent");
    RunOnUIThread([&]()
    {
        Canvas^ oldParent = safe_cast<Canvas^>(rootCanvas->FindName(L"parent1"));
        Canvas^ newParent = safe_cast<Canvas^>(rootCanvas->FindName(L"parent2"));
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        oldParent->Children->RemoveAt(0);
        newParent->Children->Append(target);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::Id, Platform::StringReference(L"Reparent").GetString());
}

void DoubleAnimationTests::ScrollBarIndicatorMode()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    TestCleanupWrapper cleanup([&]()
    {
        wh->SetPostTickCallback(nullptr);
    });

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ScrollBar.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;
    int tickCount = 0;
    std::shared_ptr<Event> stopTestEvent = std::make_shared<Event>();

    // Change IndicatorMode from MouseIndicator to None, but do it with an ObjectAnimationUsingKeyFrames.
    //
    // The ObjectAnimUKF will kick off a VSM storyboard when it ticks, which adds a new storyboard to the time manager
    // while it's ticking animations. This new storyboard wasn't being ticked, which caused an issue.
    RunOnUIThread([&]()
    {
        ScrollBar^ target = safe_cast<ScrollBar^>(rootCanvas->FindName(L"scrollb"));

        ObjectAnimationUsingKeyFrames^ oa = ref new ObjectAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 100000000L;    // 10 seconds
        oa->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(oa, target);
        Storyboard::SetTargetProperty(oa, L"IndicatorMode");

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 0;
        KeyTime kt2; kt2.TimeSpan = s2;
        DiscreteObjectKeyFrame^ kf = ref new DiscreteObjectKeyFrame();
        kf->Value = ScrollingIndicatorMode::None;
        kf->KeyTime = kt2;
        oa->KeyFrames->Append(kf);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(oa);

        storyboard->Begin();

        wh->SetPostTickCallback(ref new PostTickCallback([&]()
        {
            switch (tickCount)
            {

                case 0:
                {
                    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::NoComparison, Platform::StringReference(L"0Frame").GetString());
                    tickCount++;
                    break;
                }

                case 1:
                {
                    u->VerifyMockDCompOutput(MockDComp::AnimationComparison::NoComparison, Platform::StringReference(L"1Frame").GetString());
                    tickCount++;
                    stopTestEvent->Set();
                    break;
                }
            }
        }));
    });

    stopTestEvent->WaitForDefault();
}

void DoubleAnimationTests::EmptyKeyFrameAnimation()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(1);

    Storyboard^ linear = nullptr;
    Storyboard^ empty = nullptr;

    LOG_OUTPUT(L"Apply empty animation, no-op");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
        Canvas::SetLeft(target, 200);

        DoubleAnimationUsingKeyFrames^ da = ref new DoubleAnimationUsingKeyFrames();
        ::Windows::Foundation::TimeSpan span; span.Duration = 100000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        empty = ref new Storyboard();
        empty->Children->Append(da);

        empty->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Empty").GetString());

    LOG_OUTPUT(L"Linear animation takes over");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 20000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"(Canvas.Left)");

        linear = ref new Storyboard();
        linear->Children->Append(da);

        linear->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Linear").GetString());

    LOG_OUTPUT(L"Empty animation takes control again");
    RunOnUIThread([&]()
    {
        empty->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Empty2").GetString());
}

// Test that PixelSnapping is turned off during animation of transform, to prevent jittering
void DoubleAnimationTests::PixelSnappingDuringTransformAnimation()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-RectangleWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootGrid;
    });

    LOG_OUTPUT(L"Idle");
    wh->SynchronouslyTickUIThread(2);

    DoubleAnimation^ doubleAnimation = nullptr;
    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Animate transform");
    RunOnUIThread([&]()
    {
        TranslateTransform^ translateTransform = safe_cast<TranslateTransform^>(rootGrid->FindName(L"translateTransform"));

        doubleAnimation = ref new DoubleAnimation();
        doubleAnimation->From = 0.0;
        doubleAnimation->To = 100.0;
        ::Windows::Foundation::TimeSpan timeSpan;
        timeSpan.Duration = 20000000L;
        doubleAnimation->Duration = DurationHelper::FromTimeSpan(timeSpan);

        Storyboard::SetTarget(doubleAnimation, translateTransform);
        Storyboard::SetTargetProperty(doubleAnimation, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(doubleAnimation);

        storyboard->Begin();
    });

    LOG_OUTPUT(L"Check that PixelSnapping is turned off during transform animation");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "off");

    LOG_OUTPUT(L"Stop transform's animation");
    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });

    LOG_OUTPUT(L"Check that PixelSnapping is turned back on after transform animation has ended");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "on");
}

void DoubleAnimationTests::PixelSnap2Internal(DCompRendering dcompRendering)
{
    WUCRenderingScopeGuard guard(dcompRendering);

    auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
    auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
    std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    LOG_OUTPUT(L"Loading markup");
    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-PixelSnap2.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();
    xaml_shapes::Rectangle^ rect = nullptr;
    bool changedBrush = false;

    RunOnUIThread([&]()
    {
        ScrollViewer^ sv = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
        ScrollViewer^ svInner = safe_cast<ScrollViewer^>(sv->Content);
        rect = safe_cast<xaml_shapes::Rectangle^>(svInner->Content);

        viewChangingRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangingEventArgs^>(
            [&](Platform::Object^ sender, ScrollViewerViewChangingEventArgs^ args)
        {
            LOG_OUTPUT(L"ViewChanging, NextView: %f, %f, %f, FinalView: %f, %f, %f, IsInertial: %d",
                args->NextView->HorizontalOffset,
                args->NextView->VerticalOffset,
                args->NextView->ZoomFactor,
                args->FinalView->HorizontalOffset,
                args->FinalView->VerticalOffset,
                args->FinalView->ZoomFactor,
                args->IsInertial);

            // Currently there's another bug in XAML - we're not guaranteed to turn pixel-snapping off
            // on nested ScrollViewers as a transform animation is started.  We're choosing not to fix that bug for now,
            // instead we'll force the inner ScrollViewer to get pixel snapping turned off by dirtying it mid-animation.
            if (!changedBrush)
            {
                LOG_OUTPUT(L"Changing brush");
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
                changedBrush = true;
            }
        }));

        viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
            [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
        {
            if (!args->IsIntermediate)
            {
                LOG_OUTPUT(L"ViewChanged raised");
                viewChangedEvent->Set();
            }
        }));

        // Do an animated ChangeView, this will turn off pixel snapping during the animation, then re-enable it.
        LOG_OUTPUT(L"Doing ChangeView()");
        sv->ChangeView(nullptr, nullptr, 2.0f, false);
    });
    TestServices::WindowHelper->WaitForIdle();
    viewChangedEvent->WaitForDefault();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void DoubleAnimationTests::PixelSnap2WUC()
{
    PixelSnap2Internal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::TransformAnimationRealizationsHelper(bool animateRootGrid)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 400), 1.5f);
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    Grid^ rootGrid;
    ScaleTransform^ scale;
    TextBlock^ textBlock;
    Storyboard^ sb;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating elements");
        rootGrid = ref new Grid();
        scale = ref new ScaleTransform();

        textBlock = ref new TextBlock();
        textBlock->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        textBlock->Text = L"123";

        if (animateRootGrid)
        {
            rootGrid->RenderTransform = scale;

            // Also keep the CompNode for this element around to get additional code coverage in the RenderWalk
            rootGrid->CompositeMode = ElementCompositeMode::SourceOver;
        }
        else
        {
            textBlock->RenderTransform = scale;
        }

        rootGrid->Children->Append(textBlock);
        wh->WindowContent = rootGrid;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        // Setup animation that takes the Grid or TextBlock from 1x to 2x scale
        LOG_OUTPUT(L"Starting scale animation");
        ::Windows::Foundation::TimeSpan span; span.Duration = 1000000000L;   // 100 seconds

        DoubleAnimation^ daScaleX = ref new DoubleAnimation();
        daScaleX->From = 1.0;
        daScaleX->To = 2.0;
        daScaleX->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(daScaleX, scale);
        Storyboard::SetTargetProperty(daScaleX, L"ScaleX");

        DoubleAnimation^ daScaleY = ref new DoubleAnimation();
        daScaleY->From = 1.0;
        daScaleY->To = 2.0;
        daScaleY->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(daScaleY, scale);
        Storyboard::SetTargetProperty(daScaleY, L"ScaleY");

        sb = ref new Storyboard();
        sb->Children->Append(daScaleX);
        sb->Children->Append(daScaleY);

        sb->Begin();
    });

    // Tick the animation for a couple frames, the realization should stay at base scale
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Seeking animation");
        ::Windows::Foundation::TimeSpan span; span.Duration = 500000000L;   // 50 seconds, or halfway through the animation
        sb->Seek(span);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2Seek");

    RunOnUIThread([&]()
    {
        // Even if we dirty the element while it's animating, it should not change realization scale
        LOG_OUTPUT(L"Changing brush color to dirty element during animation");
        textBlock->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        // Finish the animation, the realization should jump to final scale
        LOG_OUTPUT(L"Finish the animation");
        sb->SkipToFill();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void DoubleAnimationTests::TransformAnimationRealizations1WUCFull()
{
    TransformAnimationRealizationsHelper(false /*animateRootGrid*/);
}

void DoubleAnimationTests::TransformAnimationRealizations2WUCFull()
{
    TransformAnimationRealizationsHelper(true /*animateRootGrid*/);
}

void DoubleAnimationTests::TransformAnimationLTEChildWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas;
    Canvas^ canvas;
    ScaleTransform^ scale;
    Storyboard^ sb;
    TextBlock^ textBlock;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating elements");
        rootCanvas = ref new Canvas();
        scale = ref new ScaleTransform();
        rootCanvas->RenderTransform = scale;

        canvas = ref new Canvas();
        auto translation = ref new TranslateTransform();
        translation->X = 0.75f;
        translation->Y = 0.75f;
        canvas->RenderTransform = translation;

        textBlock = ref new TextBlock();
        textBlock->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        textBlock->Text = L"123";

        rootCanvas->Children->Append(canvas);
        canvas->Children->Append(textBlock);

        lte = wh->AddTestLTE(textBlock, canvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        // Setup dummy animation that keeps the child underneath a running animation
        LOG_OUTPUT(L"Starting dummy animation");
        ::Windows::Foundation::TimeSpan span; span.Duration = 1000000000L;   // 100 seconds

        DoubleAnimation^ daScaleX = ref new DoubleAnimation();
        daScaleX->From = 1.0;
        daScaleX->To = 1.0;
        daScaleX->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(daScaleX, scale);
        Storyboard::SetTargetProperty(daScaleX, L"ScaleX");

        sb = ref new Storyboard();
        sb->Children->Append(daScaleX);

        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        // Remove the test LTE along with the sub-pixel RenderTransform, and verify the sub-pixel offset in the TextBlock goes away.
        // This is the step that's testing the bug fix, previously the sub-pixel offset remains.
        LOG_OUTPUT(L"Removing LTE");
        canvas->RenderTransform = nullptr;
        wh->RemoveTestLTE(lte);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Finish the animation");
        sb->SkipToFill();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

#if 0
void DoubleAnimationTests::RenderAnimatedCanvasFrameAnalysisScenario()
{
    const auto& wh = TestServices::WindowHelper;

    m_markerGenerator.MarkScenarioBegin();

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-RenderAnimatedCanvas.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->SynchronouslyTickUIThread(3);
    wh->WaitForIdle();

    m_markerGenerator.MarkScenarioEnd();
}

void DoubleAnimationTests::RenderAnimatedCanvasFrameAnalysis(DCompRendering dcompRendering)
{
    WUCRenderingScopeGuard wuc(dcompRendering, true /* resizeWindow */, false /* injectMockDComp */, false /* resetDevice */);

    // Running the scenario once before actually beginning performance
    // collection will warm up caches and give more consistent results.
    RenderAnimatedCanvasFrameAnalysisScenario();

    PerfAnalyzerSession session;
    for (int i = 0; i < 3; i++)
    {
        RenderAnimatedCanvasFrameAnalysisScenario();
    }
    session.Stop();
}

void DoubleAnimationTests::RenderAnimatedCanvasWUCFullFrameAnalysis()
{
    RenderAnimatedCanvasFrameAnalysis(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::RenderTransformAnimationFrameAnalysisScenario()
{
    m_markerGenerator.MarkScenarioBegin();

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    bool useDependentAnimation = false;
    int numRows = 10;
    int numCols = 10;
    int numIterations = 10;
    Storyboard^ bigSB = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        bigSB = ref new Storyboard();
    });
    TestServices::WindowHelper->WaitForIdle();

    if (useDependentAnimation)
    {
        LOG_OUTPUT(L"Kick off dependent animation to keep ticking UI thread. This lets us get more per-frame DWM data");
        RunOnUIThread([&]()
        {
            // This dependent animation can optionally be enabled to keep ticking the UI thread.
            Canvas^ canvasTarget = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
            DoubleAnimation^ daDependent = ref new DoubleAnimation();
            daDependent->From = 0.0;
            daDependent->To = 100.0;
            ::Windows::Foundation::TimeSpan span3; span3.Duration = 50000000L;
            daDependent->Duration = DurationHelper::FromTimeSpan(span3);
            Storyboard::SetTarget(daDependent, canvasTarget);
            Storyboard::SetTargetProperty(daDependent, L"Width");
            daDependent->EnableDependentAnimation = true;
            bigSB->Children->Append(daDependent);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    LOG_OUTPUT(L"Create 100 rectangles, each with a translate animation");
    auto bigSBCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto bigSBCompletedEvent = std::make_shared<Event>();
    RunOnUIThread([&]()
    {
        bigSBCompletedRegistration.Attach(bigSB, ref new wf::EventHandler<Object^>([bigSBCompletedEvent](Object^ sender, Object^ e) { bigSBCompletedEvent->Set(); }));

        for (int i = 0; i < numRows; i++)
        {
            for (int j = 0; j < numCols; j++)
            {
                xaml_shapes::Rectangle^ rect = safe_cast<xaml_shapes::Rectangle^>(XamlReader::Load("<Rectangle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='10' Height='10' Fill='Green'></Rectangle>"));

                rect->SetValue(Canvas::TopProperty, i*(numRows + 5));
                rect->SetValue(Canvas::LeftProperty, j*(numCols + 5));
                rootCanvas->Children->Append(rect);

                auto translateTransform = ref new xaml_media::TranslateTransform();
                rect->RenderTransform = translateTransform;

                DoubleAnimation^ doubleAnimation = ref new DoubleAnimation();
                doubleAnimation->By = 20.0;
                ::Windows::Foundation::TimeSpan timeSpan;
                timeSpan.Duration = 2500000L;       // 0.25 sec
                doubleAnimation->Duration = DurationHelper::FromTimeSpan(timeSpan);

                Storyboard::SetTarget(doubleAnimation, translateTransform);
                Storyboard::SetTargetProperty(doubleAnimation, L"X");

                bigSB->Children->Append(doubleAnimation);
            }
        }
    });
    TestServices::WindowHelper->WaitForIdle();

    for (int i = 0; i < numIterations; i++)
    {
        LOG_OUTPUT(L"Start animating the rectangles, iteration# %d", i);
        RunOnUIThread([&]()
        {
            bigSB->Begin();
        });

        bigSBCompletedEvent->WaitForDefault();
        LOG_OUTPUT(L"... iteration# %d completed", i);
    }

    m_markerGenerator.MarkScenarioEnd();
}

void DoubleAnimationTests::RenderTransformAnimationFrameAnalysis(DCompRendering dcompRendering)
{
    WUCRenderingScopeGuard wuc(dcompRendering, true /* resizeWindow */, false /* injectMockDComp */, false /* resetDevice */);

    // Running the scenario once before actually beginning performance
    // collection will warm up caches and give more consistent results.
    RenderTransformAnimationFrameAnalysisScenario();

    PerfAnalyzerSession session;
    for (int i = 0; i < 3; i++)
    {
        RenderTransformAnimationFrameAnalysisScenario();
    }
    session.Stop();
}

void DoubleAnimationTests::RenderTransformAnimationWUCFullFrameAnalysis()
{
    RenderTransformAnimationFrameAnalysis(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::SimulateCPGStartMenuTestFrameAnalysisScenario()
{
    m_markerGenerator.MarkScenarioBegin();

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    bool useDependentAnimation = false;
    int numRows = 10;
    int numCols = 10;
    int numIterations = 10;
    Storyboard^ bigSB = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        bigSB = ref new Storyboard();
    });
    TestServices::WindowHelper->WaitForIdle();

    if (useDependentAnimation)
    {
        LOG_OUTPUT(L"Kick off dependent animation to keep ticking UI thread. This lets us get more per-frame DWM data");
        RunOnUIThread([&]()
        {
            // This dependent animation can optionally be enabled to keep ticking the UI thread.
            Canvas^ canvasTarget = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
            DoubleAnimation^ daDependent = ref new DoubleAnimation();
            daDependent->From = 0.0;
            daDependent->To = 100.0;
            ::Windows::Foundation::TimeSpan span3; span3.Duration = 50000000L;
            daDependent->Duration = DurationHelper::FromTimeSpan(span3);
            Storyboard::SetTarget(daDependent, canvasTarget);
            Storyboard::SetTargetProperty(daDependent, L"Width");
            daDependent->EnableDependentAnimation = true;
            bigSB->Children->Append(daDependent);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    LOG_OUTPUT(L"Create 100 rectangles, each with a Transform3D that only has a TranslateY animation");
    auto bigSBCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto bigSBCompletedEvent = std::make_shared<Event>();
    RunOnUIThread([&]()
    {
        bigSBCompletedRegistration.Attach(bigSB, ref new wf::EventHandler<Object^>([bigSBCompletedEvent](Object^ sender, Object^ e) { bigSBCompletedEvent->Set(); }));

        for (int i = 0; i < numRows; i++)
        {
            for (int j = 0; j < numCols; j++)
            {
                xaml_shapes::Rectangle^ rect = safe_cast<xaml_shapes::Rectangle^>(XamlReader::Load("<Rectangle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='10' Height='10' Fill='Green'></Rectangle>"));

                rect->SetValue(Canvas::TopProperty, i*(numRows + 5));
                rect->SetValue(Canvas::LeftProperty, j*(numCols + 5));
                rootCanvas->Children->Append(rect);

                auto compositeTransform3D = ref new CompositeTransform3D;
                rect->Transform3D = compositeTransform3D;

                DoubleAnimation^ da = ref new DoubleAnimation();
                da->By = 20.0;
                ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L;
                da->Duration = DurationHelper::FromTimeSpan(span);
                Storyboard::SetTarget(da, compositeTransform3D);
                Storyboard::SetTargetProperty(da, L"TranslateY");

                bigSB->Children->Append(da);
            }
        }
    });
    TestServices::WindowHelper->WaitForIdle();

    for (int i = 0; i < numIterations; i++)
    {
        LOG_OUTPUT(L"Start animating the rectangles, iteration# %d", i);
        RunOnUIThread([&]()
        {
            bigSB->Begin();
        });

        bigSBCompletedEvent->WaitForDefault();
        LOG_OUTPUT(L"... iteration# %d completed", i);
    }

    m_markerGenerator.MarkScenarioEnd();
}

void DoubleAnimationTests::SimulateCPGStartMenuTestFrameAnalysis(DCompRendering dcompRendering)
{
    WUCRenderingScopeGuard wuc(dcompRendering, true /* resizeWindow */, false /* injectMockDComp */, false /* resetDevice */);

    // Running the scenario once before actually beginning performance
    // collection will warm up caches and give more consistent results.
    SimulateCPGStartMenuTestFrameAnalysisScenario();

    PerfAnalyzerSession session;
    for (int i = 0; i < 3; i++)
    {
        SimulateCPGStartMenuTestFrameAnalysisScenario();
    }
    session.Stop();
}

void DoubleAnimationTests::SimulateCPGStartMenuTestWUCFullFrameAnalysis()
{
    SimulateCPGStartMenuTestFrameAnalysis(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::ManyAnimationsFrameAnalysisScenario()
{
    m_markerGenerator.MarkScenarioBegin();

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    bool useDependentAnimation = false;
    int numRows = 10;
    int numCols = 10;
    int numIterations = 5;
    Storyboard^ bigSB = nullptr;
    DispatcherTimer^ timer = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        bigSB = ref new Storyboard();
    });
    TestServices::WindowHelper->WaitForIdle();

    if (useDependentAnimation)
    {
        LOG_OUTPUT(L"Kick off dependent animation to keep ticking UI thread. This lets us get more per-frame DWM data");
        RunOnUIThread([&]()
        {
            // This dependent animation can optionally be enabled to keep ticking the UI thread.
            Canvas^ canvasTarget = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));
            DoubleAnimation^ daDependent = ref new DoubleAnimation();
            daDependent->From = 0.0;
            daDependent->To = 100.0;
            ::Windows::Foundation::TimeSpan span3; span3.Duration = 50000000L;
            daDependent->Duration = DurationHelper::FromTimeSpan(span3);
            Storyboard::SetTarget(daDependent, canvasTarget);
            Storyboard::SetTargetProperty(daDependent, L"Width");
            daDependent->EnableDependentAnimation = true;
            bigSB->Children->Append(daDependent);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    LOG_OUTPUT(L"Create 100 rectangles, each with a CompositeTransform animation");
    auto bigSBCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto bigSBCompletedEvent = std::make_shared<Event>();
    auto timerCompletedRegistration = CreateSafeEventRegistration(DispatcherTimer, Tick); auto timerCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        bigSBCompletedRegistration.Attach(bigSB, ref new wf::EventHandler<Object^>([bigSBCompletedEvent](Object^ sender, Object^ e) { bigSBCompletedEvent->Set(); }));

        timer = ref new DispatcherTimer();
        ::Windows::Foundation::TimeSpan span; span.Duration = 0L; timer->Interval = span;
        timerCompletedRegistration.Attach(timer, ref new wf::EventHandler<Object^>([timerCompletedEvent](Object^ sender, Object^ e) { timerCompletedEvent->Set(); }));


        for (int i = 0; i < numRows; i++)
        {
            for (int j = 0; j < numCols; j++)
            {
                xaml_shapes::Rectangle^ rect = safe_cast<xaml_shapes::Rectangle^>(XamlReader::Load("<Rectangle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='10' Height='10' Fill='Green'></Rectangle>"));

                rect->SetValue(Canvas::TopProperty, i*(numRows + 5));
                rect->SetValue(Canvas::LeftProperty, j*(numCols + 5));
                rootCanvas->Children->Append(rect);

                // Add CompositeTransform
                {
                    auto compositeTransform = ref new xaml_media::CompositeTransform();

                    rect->RenderTransform = compositeTransform;

                    DoubleAnimation^ da = ref new DoubleAnimation();
                    da->From = 0.0;
                    da->To = 360.0;
                    ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L;
                    da->Duration = DurationHelper::FromTimeSpan(span);
                    Storyboard::SetTarget(da, compositeTransform);
                    Storyboard::SetTargetProperty(da, L"Rotation");

                    DoubleAnimation^ da2 = ref new DoubleAnimation();
                    da2->By = 20.0;
                    ::Windows::Foundation::TimeSpan timeSpan;
                    timeSpan.Duration = 2500000L;       // 0.25 sec
                    da2->Duration = DurationHelper::FromTimeSpan(timeSpan);
                    Storyboard::SetTarget(da2, compositeTransform);
                    Storyboard::SetTargetProperty(da2, L"TranslateX");

                    bigSB->Children->Append(da);
                    bigSB->Children->Append(da2);
                }

                {
                    auto projectionTransform = ref new xaml_media::PlaneProjection();
                    rect->Projection = projectionTransform;

                    DoubleAnimation^ da = ref new DoubleAnimation();
                    da->From = 0.0;
                    da->To = 360.0;
                    ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L;
                    da->Duration = DurationHelper::FromTimeSpan(span);
                    Storyboard::SetTarget(da, projectionTransform);
                    Storyboard::SetTargetProperty(da, L"RotationX");

                    DoubleAnimation^ da2 = ref new DoubleAnimation();
                    da2->From = 0.0;
                    da2->To = 0.1;
                    ::Windows::Foundation::TimeSpan span2; span2.Duration = 2500000L;
                    da2->Duration = DurationHelper::FromTimeSpan(span2);
                    Storyboard::SetTarget(da2, projectionTransform);
                    Storyboard::SetTargetProperty(da2, L"GlobalOffsetZ");

                    DoubleAnimation^ da3 = ref new DoubleAnimation();
                    da3->From = 0.0;
                    da3->To = -1.0;
                    ::Windows::Foundation::TimeSpan span3; span3.Duration = 2500000L;
                    da3->Duration = DurationHelper::FromTimeSpan(span3);
                    Storyboard::SetTarget(da3, projectionTransform);
                    Storyboard::SetTargetProperty(da3, L"LocalOffsetX");

                    DoubleAnimation^ da4 = ref new DoubleAnimation();
                    da4->By = 0.01;
                    ::Windows::Foundation::TimeSpan span4; span4.Duration = 2500000L;
                    da4->Duration = DurationHelper::FromTimeSpan(span4);
                    Storyboard::SetTarget(da4, projectionTransform);
                    Storyboard::SetTargetProperty(da4, L"CenterOfRotationX");

                    bigSB->Children->Append(da);
                    bigSB->Children->Append(da2);
                    bigSB->Children->Append(da3);
                    bigSB->Children->Append(da4);
                }
            }
        }
    });
    TestServices::WindowHelper->WaitForIdle();

    for (int i = 0; i < numIterations; i++)
    {
        LOG_OUTPUT(L"Start animating the rectangles, iteration# %d", i);
        RunOnUIThread([&]()
        {
            bigSB->Begin();
        });

        bigSBCompletedEvent->WaitForDefault();
        LOG_OUTPUT(L"... iteration# %d completed", i);
    }

    // An alternate approach to try to isolate expression creation is to constantly tick the UI
    // thread and kick off the animation on every tick as below. However the animations still tend
    // to complete and we still get data for some "light" DWM frames, i.e. ones presumably not creating/hooking up expressions.
    // RunOnUIThread([&]()
    //{
    //    timer->Start();
    //});

    //for (int i = 0; i < numIterations; i++)
    //{
    //    LOG_OUTPUT(L"Start animating the rectangles, iteration# %d", i);
    //    timerCompletedEvent->WaitForDefault();

    //    RunOnUIThread([&]()
    //    {
    //        bigSB->Stop();
    //        bigSB->Begin();
    //    });


    //    LOG_OUTPUT(L"... iteration# %d completed", i);
    //}

    m_markerGenerator.MarkScenarioEnd();
}

void DoubleAnimationTests::ManyAnimationsFrameAnalysis(DCompRendering dcompRendering)
{
    WUCRenderingScopeGuard wuc(dcompRendering, true /* resizeWindow */, false /* injectMockDComp */, false /* resetDevice */);

    // Running the scenario once before actually beginning performance
    // collection will warm up caches and give more consistent results.
    ManyAnimationsFrameAnalysisScenario();

    PerfAnalyzerSession session;
    for (int i = 0; i < 3; i++)
    {
        ManyAnimationsFrameAnalysisScenario();
    }
    session.Stop();
}

void DoubleAnimationTests::ManyAnimationsWUCFullFrameAnalysis()
{
    ManyAnimationsFrameAnalysis(DCompRendering::WUCCompleteSynchronousCompTree);
}
#endif

void DoubleAnimationTests::DependentAnimation()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-CanvasWithTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canv"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 50.0;
        da->To = 100.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 5000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Width");
        da->EnableDependentAnimation = true;

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Final");

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::RTLAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    FrameworkElement^ rootCanvas = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DoubleAnimationTests-RTLTarget.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"[Idle]");
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Idle");

    LOG_OUTPUT(L"[RTL Animation]");
    RunOnUIThread([&]()
    {
        TranslateTransform^ target = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"translate"));
        ::Windows::Foundation::TimeSpan span;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 50.0;
        span.Duration = 20000000L; da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Y");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(20);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Animating");
}

void DoubleAnimationTests::RTLAnimationWUC()
{
    RTLAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void DoubleAnimationTests::DeviceLostDuringAnimationWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    TranslateTransform^ translate;
    Storyboard^ storyboard;
    Canvas^ root;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Idle]");

        translate = ref new TranslateTransform();
        translate->X = 50;

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 6000000000L;    // 10 minutes

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 200.0;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, translate);
        Storyboard::SetTargetProperty(da, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        border->RenderTransform = translate;

        root = ref new Canvas();
        root->Children->Append(border);
        wh->WindowContent = root;
    });

    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Start animation]");
        storyboard->Begin();
    });

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Animating");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Device lost, detach animating object in the same frame, don't crash]");
        wh->ResetDeviceAndVisuals();
        root->Children->Clear();
    });

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"DeviceLost");
}

DoubleAnimationUsingKeyFrames^ DoubleAnimationTests::MakeDuration60DAUKF(DependencyObject^ target)
{
    ::Windows::Foundation::TimeSpan span;

    DoubleAnimationUsingKeyFrames^ kfa = ref new DoubleAnimationUsingKeyFrames();
    Storyboard::SetTarget(kfa, target);
    Storyboard::SetTargetProperty(kfa, L"X");

    span.Duration = 0L; KeyTime kt1; kt1.TimeSpan = span;
    LinearDoubleKeyFrame^ kf1 = ref new LinearDoubleKeyFrame();
    kf1->Value = 600;
    kf1->KeyTime = kt1;
    kfa->KeyFrames->Append(kf1);

    span.Duration = 600000000L; KeyTime kt2; kt2.TimeSpan = span;   // key frame at 60 seconds
    LinearDoubleKeyFrame^ kf2 = ref new LinearDoubleKeyFrame();
    // Use the same value as the first key frame. These animations are going to tick on the UI thread, so the
    // MockDComp snapshot is going to capture an animated value. We want a consistent number.
    kf2->Value = 600;
    kf2->KeyTime = kt2;
    kfa->KeyFrames->Append(kf2);

    return kfa;
}

void DoubleAnimationTests::KeyFrameAnimationClippedByDurationWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    TranslateTransform^ translate;
    Storyboard^ storyboard;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        translate = ref new TranslateTransform();
        translate->X = 50;

        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        border->RenderTransform = translate;

        root = ref new Canvas();
        root->Children->Append(border);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;

        LOG_OUTPUT(L"> DoubleAnimationUsingKeyFrames does the clipping");

        DoubleAnimationUsingKeyFrames^ kfa = MakeDuration60DAUKF(translate);
        span.Duration = 30000000L; kfa->Duration = DurationHelper::FromTimeSpan(span);  // duration 3 seconds on the animation

        storyboard = ref new Storyboard();
        storyboard->Children->Append(kfa);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
        ::Windows::Foundation::TimeSpan span;

        LOG_OUTPUT(L"> Storyboard does the clipping");

        DoubleAnimationUsingKeyFrames^ kfa = MakeDuration60DAUKF(translate);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(kfa);
        span.Duration = 30000000L; storyboard->Duration = DurationHelper::FromTimeSpan(span);  // duration 3 seconds on the storyboard
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::BounceEaseInvalidBounciness()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    TranslateTransform^ translate;
    Storyboard^ storyboard;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        translate = ref new TranslateTransform();
        translate->X = 50;

        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        border->RenderTransform = translate;

        root = ref new Canvas();
        root->Children->Append(border);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> DoubleAnimationUsingKeyFrames with a BounceEase with Bounciness=0. Don't crash.");

        BounceEase^ easingFunction = ref new BounceEase();
        easingFunction->Bounces = 3;
        easingFunction->Bounciness = 0;

        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = span;
        EasingDoubleKeyFrame^ kf1 = ref new EasingDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        kf1->EasingFunction = easingFunction;

        DoubleAnimationUsingKeyFrames^ kfa = ref new DoubleAnimationUsingKeyFrames();
        kfa->KeyFrames->Append(kf1);
        Storyboard::SetTarget(kfa, translate);
        Storyboard::SetTargetProperty(kfa, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(kfa);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::BackEaseInvalidAmplitude()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    TranslateTransform^ translate;
    Storyboard^ storyboard;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        translate = ref new TranslateTransform();
        translate->X = 50;

        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        border->RenderTransform = translate;

        root = ref new Canvas();
        root->Children->Append(border);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> DoubleAnimationUsingKeyFrames with a BounceEase with Bounciness=0. Don't crash.");

        BackEase^ easingFunction = ref new BackEase();
        easingFunction->Amplitude = -2;

        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = span;
        EasingDoubleKeyFrame^ kf1 = ref new EasingDoubleKeyFrame();
        kf1->Value = 100;
        kf1->KeyTime = kt1;
        kf1->EasingFunction = easingFunction;

        DoubleAnimationUsingKeyFrames^ kfa = ref new DoubleAnimationUsingKeyFrames();
        kfa->KeyFrames->Append(kf1);
        Storyboard::SetTarget(kfa, translate);
        Storyboard::SetTargetProperty(kfa, L"X");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(kfa);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void DoubleAnimationTests::DependentAnimationShouldNotRunByDefault()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Storyboard^ storyboard;
    Canvas^ root;
    Border^ border;

    RunOnUIThread([&]()
    {
        border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        root = ref new Canvas();
        root->Children->Append(border);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Start an animation on Border.Width. Expect it to be ignored.");

        ::Windows::Foundation::TimeSpan span; span.Duration = 10000000L;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 300.0;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, border);
        Storyboard::SetTargetProperty(da, L"Width");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restart the Storyboard. It should still be ignored.");

        storyboard->Stop();
        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}
} } } } } }
