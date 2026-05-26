// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ColorAnimationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

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

Platform::String^ ColorAnimationTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool ColorAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ColorAnimationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ColorAnimationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ColorAnimationTests::CreateColorAnimation(
    Canvas^ rootCanvas,
    Platform::String^ brushName,
    ::Windows::UI::Color toColor,
    int64 duration,
    Storyboard^ storyboard
    )
{
    SolidColorBrush^ target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(brushName));

    ColorAnimation^ animation = ref new ColorAnimation();
    animation->To = toColor;
    ::Windows::Foundation::TimeSpan span; span.Duration = duration;
    animation->Duration = DurationHelper::FromTimeSpan(span);
    Storyboard::SetTarget(animation, target);
    Storyboard::SetTargetProperty(animation, L"Color");

    storyboard->Children->Append(animation);
}

void ColorAnimationTests::CreateColorAnimation(
    Canvas^ rootCanvas,
    SolidColorBrush^ target,
    ::Windows::UI::Color fromColor,
    ::Windows::UI::Color toColor,
    int64 duration,
    bool autoReverse,
    bool repeatForever,
    Storyboard^ storyboard
    )
{
    ColorAnimation^ animation = ref new ColorAnimation();
    animation->From = fromColor;
    animation->To = toColor;
    ::Windows::Foundation::TimeSpan span; span.Duration = duration;
    animation->Duration = DurationHelper::FromTimeSpan(span);
    animation->AutoReverse = autoReverse;
    if (repeatForever)
    {
        animation->RepeatBehavior = RepeatBehaviorHelper::Forever;
    }
    Storyboard::SetTarget(animation, target);
    Storyboard::SetTargetProperty(animation, L"Color");

    storyboard->Children->Append(animation);
}

void ColorAnimationTests::CreateColorAnimation(
    Canvas^ rootCanvas,
    Platform::String^ brushName,
    ::Windows::UI::Color fromColor,
    ::Windows::UI::Color toColor,
    int64 duration,
    bool autoReverse,
    bool repeatForever,
    Storyboard^ storyboard
    )
{
    SolidColorBrush^ target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(brushName));

    CreateColorAnimation(rootCanvas, target, fromColor, toColor, duration, autoReverse, repeatForever, storyboard);
}

void ColorAnimationTests::CreateColorAnimation(
    Canvas^ rootCanvas,
    Platform::String^ brushName,
    ::Windows::UI::Color fromColor,
    ::Windows::UI::Color toColor,
    Storyboard^ storyboard
    )
{
    CreateColorAnimation(rootCanvas, brushName, fromColor, toColor, 10000000L, false, false, storyboard);
}

void ColorAnimationTests::CreateDoubleAnimation(
    Canvas^ rootCanvas,
    int64 duration,
    int iterations,
    Storyboard^ storyboard)
{
    Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

    DoubleAnimation^ da = ref new DoubleAnimation();
    da->From = 0.5;
    da->To = 1.0;
    ::Windows::Foundation::TimeSpan span; span.Duration = duration;
    da->Duration = DurationHelper::FromTimeSpan(span);
    if (iterations != 1)
    {
        da->RepeatBehavior = RepeatBehaviorHelper::FromCount(iterations);
    }
    Storyboard::SetTarget(da, target);
    Storyboard::SetTargetProperty(da, L"Opacity");

    storyboard->Children->Append(da);
}

void ColorAnimationTests::OneColorAnimation(
    DCompRendering dcompRendering,
    float iterations)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-SimpleCanvas.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Canvas Background");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), storyboard);
        if (iterations != 1.0f)
        {
            storyboard->RepeatBehavior = RepeatBehaviorHelper::FromCount(iterations);
        }

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Canvas").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::SimpleColorAnimationWUCFull()
{
    OneColorAnimation(DCompRendering::WUCCompleteSynchronousCompTree, 1.0f);
}

void ColorAnimationTests::LoopingColorAnimationWUCFull()
{
    OneColorAnimation(DCompRendering::WUCCompleteSynchronousCompTree, 3.0f);
}

void ColorAnimationTests::ForeverColorAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-SimpleCanvas.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"[Initial]");
    wh->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Animation loops forever]");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        CreateColorAnimation(
            rootCanvas,
            L"canvasBrush",
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff),
            10000000L,
            false,
            true,
            storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Animation").GetString());

    LOG_OUTPUT(L"[Storyboard loops forever]");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        CreateColorAnimation(
            rootCanvas,
            L"canvasBrush",
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff),
            10000000L,
            false,
            false,
            storyboard);

        storyboard->RepeatBehavior = RepeatBehaviorHelper::Forever;

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Storyboard").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::ForeverColorAnimationWUCFull()
{
    ForeverColorAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ColorAnimationTests::ColorAndDoubleAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-SimpleCanvas.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Color + Double]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), storyboard);
        CreateDoubleAnimation(rootCanvas, 10000000L, 1, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ColorOpacity").GetString());

    LOG_OUTPUT(L"[Color is longer]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), 20000000L, false, false, storyboard);
        CreateDoubleAnimation(rootCanvas, 10000000L, 1, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ColorLonger").GetString());

    LOG_OUTPUT(L"[Double is longer]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), storyboard);
        CreateDoubleAnimation(rootCanvas, 20000000L, 1, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"DoubleLonger").GetString());

    LOG_OUTPUT(L"[Color reverses]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), 10000000L, true, false, storyboard);
        CreateDoubleAnimation(rootCanvas, 10000000L, 1, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ColorReverse").GetString());

    LOG_OUTPUT(L"[Double repeats]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), storyboard);
        CreateDoubleAnimation(rootCanvas, 10000000L, 2, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"DoubleRepeat").GetString());

    LOG_OUTPUT(L"[Storyboard repeats]");
    RunOnUIThread([&]()
    {
        Canvas^ target = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas"));

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), storyboard);
        CreateDoubleAnimation(rootCanvas, 10000000L, 1, storyboard);

        storyboard->RepeatBehavior = RepeatBehaviorHelper::FromCount(3);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"StoryboardRepeat").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::ColorAndDoubleAnimationWUCFull()
{
    ColorAndDoubleAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ColorAnimationTests::SolidColorBrushAnimation(DCompRendering dcompRendering)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-CanvasWithTargetColor.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Canvas Background");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, L"canvasBrush", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"backgroundBrush1", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0xff, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"borderBrush1", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0xff, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"backgroundBrush2", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"borderBrush2", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"backgroundBrush3", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0xff, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"borderBrush3", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0xff, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"backgroundBrush4", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"borderBrush4", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"fillBrush1", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"strokeBrush1", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"fillBrush2", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"strokeBrush2", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"fillBrush3", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0xff, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"strokeBrush3", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0xff, 0xff), storyboard);
        CreateColorAnimation(rootCanvas, L"fillBrush4", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0xff), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"strokeBrush4", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0xff, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"fillBrush5", Microsoft::UI::ColorHelper::FromArgb(0, 0, 0xff, 0xff), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"strokeBrush5", Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"fillBrush6", Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0, 0xff), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"strokeBrush6", Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0xff, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"textBlockBrush1", Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0xff, 0xff), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);
        CreateColorAnimation(rootCanvas, L"textBlockBrush2", Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Canvas").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::SolidColorBrushAnimationWUCFull()
{
    SolidColorBrushAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ColorAnimationTests::VerifyColor(int upperA, int lowerA, int upperR, int lowerR, int upperG, int lowerG, int upperB, int lowerB, ::Windows::UI::Color actual)
{
    VERIFY_IS_GREATER_THAN_OR_EQUAL(upperA, actual.A);
    VERIFY_IS_LESS_THAN_OR_EQUAL(lowerA, actual.A);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(upperR, actual.R);
    VERIFY_IS_LESS_THAN_OR_EQUAL(lowerR, actual.R);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(upperG, actual.G);
    VERIFY_IS_LESS_THAN_OR_EQUAL(lowerG, actual.G);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(upperB, actual.B);
    VERIFY_IS_LESS_THAN_OR_EQUAL(lowerB, actual.B);
}

void ColorAnimationTests::VerifyColor(int expectedA, int expectedR, int expectedG, int expectedB, ::Windows::UI::Color actual)
{
    VERIFY_ARE_EQUAL(expectedA, actual.A);
    VERIFY_ARE_EQUAL(expectedR, actual.R);
    VERIFY_ARE_EQUAL(expectedG, actual.G);
    VERIFY_ARE_EQUAL(expectedB, actual.B);
}

void ColorAnimationTests::AnimationReadback(bool isKeyFrame)
{
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-CanvasWithTargetColor.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Idle");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    Storyboard^ sb = nullptr;

    {
        LOG_OUTPUT(L"Initial value should not have the animated value");

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            SolidColorBrush^ target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"));

            sb = ref new Storyboard();

            if (isKeyFrame)
            {
                ColorAnimationUsingKeyFrames^ anim = ref new ColorAnimationUsingKeyFrames();
                ::Windows::Foundation::TimeSpan span;
                span.Duration = 256000000000L;
                anim->Duration = DurationHelper::FromTimeSpan(span);

                ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
                KeyTime kt1; kt1.TimeSpan = s1;
                DiscreteColorKeyFrame^ kf1 = ref new DiscreteColorKeyFrame();
                kf1->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0);
                kf1->KeyTime = kt1;
                anim->KeyFrames->Append(kf1);

                ::Windows::Foundation::TimeSpan s2; s2.Duration = 128000000000L;
                KeyTime kt2; kt2.TimeSpan = s2;
                DiscreteColorKeyFrame^ kf2 = ref new DiscreteColorKeyFrame();
                kf2->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0x7f, 0x7f, 0);
                kf2->KeyTime = kt2;
                anim->KeyFrames->Append(kf2);

                ::Windows::Foundation::TimeSpan s3; s3.Duration = 256000000000L;
                KeyTime kt3; kt3.TimeSpan = s3;
                DiscreteColorKeyFrame^ kf3 = ref new DiscreteColorKeyFrame();
                kf3->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0);
                kf3->KeyTime = kt3;
                anim->KeyFrames->Append(kf3);

                Storyboard::SetTarget(anim, target);
                Storyboard::SetTargetProperty(anim, L"Color");
                sb->Children->Append(anim);
            }
            else
            {
                ColorAnimation^ anim = ref new ColorAnimation();
                ::Windows::Foundation::TimeSpan span;
                span.Duration = 256000000000L;
                anim->Duration = DurationHelper::FromTimeSpan(span);
                anim->From = Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0);
                anim->To = Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0);

                Storyboard::SetTarget(anim, target);
                Storyboard::SetTargetProperty(anim, L"Color");
                sb->Children->Append(anim);
            }

            sb->Begin();

            value = target->Color;
        });
        VerifyColor(0,0,0,0, value);
    }

    {
        LOG_OUTPUT(L"Animation should apply value while ticking");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            value = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"))->Color;
        });
        VerifyColor(0xff,0xff,0xff,0xfe,1,0,0,0, value);
    }

    {
        LOG_OUTPUT(L"Animation should not apply value after seeking");

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 128050000000L;
            sb->Seek(seekSpan);

            value = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"))->Color;
        });
        VerifyColor(0xff,0xff,0xff,0xfe,1,0,0,0, value);
    }

    {
        LOG_OUTPUT(L"Animation should apply value after seeking and ticking");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            value = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"))->Color;
        });
        // Due to the way CColorAnimation::InterpolateValues works, the alpha doesn't always add back up to 0xff.
        // DComp animation readback does a better job of interpolating and does report 0xff.
        VerifyColor(0xff,0xfe,0x80,0x7f,0x80,0x7f,0,0, value);
    }

    {
        LOG_OUTPUT(L"Animation should apply value after SeekAlignedToLastTick");

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 10000L;
            sb->SeekAlignedToLastTick(seekSpan);

            value = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"))->Color;
        });
        // DComp animation readback interpolates values that are off by 1
        VerifyColor(0xff,0xfe,0xff,0xfe,1,0,0,0, value);
    }

    {
        LOG_OUTPUT(L"Seeking to Filling");

        RunOnUIThread([&]()
        {
            ::Windows::Foundation::TimeSpan seekSpan; seekSpan.Duration = 256500000000L;
            sb->Seek(seekSpan);
        });

        TestServices::WindowHelper->SynchronouslyTickUIThread(2);

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            value = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"))->Color;
        });
        VerifyColor(0xff,0,0xff,0, value);
    }

    {
        LOG_OUTPUT(L"Animation should not apply value after Stop");

        ::Windows::UI::Color value;
        RunOnUIThread([&]()
        {
            sb->Stop();

            value = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"))->Color;
        });
        VerifyColor(0,0,0,0, value);
    }
}

void ColorAnimationTests::LinearAnimationReadbackWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    AnimationReadback(false);
}

void ColorAnimationTests::KeyFrameAnimationReadbackWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    AnimationReadback(true);
}

void ColorAnimationTests::HandoffToDepObjAnimInternal()
{
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-CanvasWithTargetColor.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Color animation");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        CreateColorAnimation(
            rootCanvas,
            L"canvasBrush",
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff),
            100000000L,
            false,
            false,
            storyboard);

        storyboard->Begin();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(10);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ca").GetString());

    LOG_OUTPUT(L"Object animation");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        SolidColorBrush^ target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"));

        ObjectAnimationUsingKeyFrames^ animation = ref new ObjectAnimationUsingKeyFrames();

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 0L;
        KeyTime kt1; kt1.TimeSpan = s1;
        DiscreteObjectKeyFrame^ kf1 = ref new DiscreteObjectKeyFrame();
        kf1->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0);
        kf1->KeyTime = kt1;
        animation->KeyFrames->Append(kf1);

        Storyboard::SetTarget(animation, target);
        Storyboard::SetTargetProperty(animation, L"Color");
        storyboard->Children->Append(animation);

        storyboard->Begin();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"oa").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::HandoffToDepObjAnimWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    HandoffToDepObjAnimInternal();
}

void ColorAnimationTests::NullDurationInternal()
{
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-CanvasWithTargetColor.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    Storyboard^ storyboard = nullptr;

    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        SolidColorBrush^ target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"));

        ColorAnimation^ animation = ref new ColorAnimation();
        animation->From = Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0);
        animation->To = Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff);
        Storyboard::SetTarget(animation, target);
        Storyboard::SetTargetProperty(animation, L"Color");
        // No duration set. Defaults to 1 second.

        storyboard->Children->Append(animation);

        storyboard->Begin();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"CA").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        SolidColorBrush^ target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"canvasBrush"));

        ColorAnimationUsingKeyFrames^ animation = ref new ColorAnimationUsingKeyFrames();
        Storyboard::SetTarget(animation, target);
        Storyboard::SetTargetProperty(animation, L"Color");
        // No duration set. Should infer from key frames.

        ::Windows::Foundation::TimeSpan s1; s1.Duration = 10000000L;
        KeyTime kt1; kt1.TimeSpan = s1;
        LinearColorKeyFrame^ kf1 = ref new LinearColorKeyFrame();
        kf1->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0);
        kf1->KeyTime = kt1;
        animation->KeyFrames->Append(kf1);

        ::Windows::Foundation::TimeSpan s2; s2.Duration = 20000000L;
        KeyTime kt2; kt2.TimeSpan = s2;
        LinearColorKeyFrame^ kf2 = ref new LinearColorKeyFrame();
        kf2->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0x7f, 0x7f, 0);
        kf2->KeyTime = kt2;
        animation->KeyFrames->Append(kf2);

        ::Windows::Foundation::TimeSpan s3; s3.Duration = 30000000L;
        KeyTime kt3; kt3.TimeSpan = s3;
        LinearColorKeyFrame^ kf3 = ref new LinearColorKeyFrame();
        kf3->Value = Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0);
        kf3->KeyTime = kt3;
        animation->KeyFrames->Append(kf3);

        storyboard = ref new Storyboard();
        storyboard->Children->Append(animation);

        storyboard->Begin();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"CAUKF").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::NullDurationWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    NullDurationInternal();
}

void ColorAnimationTests::SeekHandoffInternal()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->SetTimeManagerClockOverrideConstant(-1);
    });

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorAnimationTests-CanvasWithTargetColor.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;
    Storyboard^ storyboard2 = nullptr;

    TestServices::WindowHelper->SetTimeManagerClockOverrideConstant(0);

    LOG_OUTPUT(L"Color animation");
    RunOnUIThread([&]()
    {
        storyboard = ref new Storyboard();

        CreateColorAnimation(
            rootCanvas,
            L"canvasBrush",
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff),
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0, 0xff),
            10000000L,
            false,
            false,
            storyboard);

        storyboard->Begin();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ca").GetString());

    LOG_OUTPUT(L"Handoff color animation");
    RunOnUIThread([&]()
    {
        storyboard2 = ref new Storyboard();

        CreateColorAnimation(
            rootCanvas,
            L"canvasBrush",
            Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0),
            10000000L,
            storyboard2);

        TestServices::WindowHelper->SetTimeManagerClockOverrideConstant(0.5);
        storyboard2->Begin();
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ca2").GetString());

    LOG_OUTPUT(L"Seek handoff color animation");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 2500000L;
        storyboard2->Seek(span);

        TestServices::WindowHelper->SetTimeManagerClockOverrideConstant(1);
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ca3").GetString());

    LOG_OUTPUT(L"Handoff color animation expires");
    TestServices::WindowHelper->SetTimeManagerClockOverrideConstant(3);
    TestServices::WindowHelper->FireDCompAnimationCompleted(storyboard2);
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ca4").GetString());

    LOG_OUTPUT(L"Seek handoff color animation back to active");
    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 2500000L;
        storyboard2->Seek(span);

        TestServices::WindowHelper->SetTimeManagerClockOverrideConstant(4);
    });
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"ca5").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::SeekHandoffWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SeekHandoffInternal();
}

void ColorAnimationTests::CanvasWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas;
    Canvas^ canvas;
    SolidColorBrush^ brush;

    RunOnUIThread([&]()
    {
        brush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));

        canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 50;
        canvas->Background = brush;

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(canvas);

        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Canvas Background]");

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, brush, Microsoft::UI::ColorHelper::FromArgb(255, 255, 0, 0), Microsoft::UI::ColorHelper::FromArgb(255, 0, 0, 255), 10000000L, false, false, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Canvas").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

void ColorAnimationTests::ContentPresenterWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas;
    ContentPresenter^ contentPresenter;
    SolidColorBrush^ borderBrush;

    RunOnUIThread([&]()
    {
        borderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0, 0, 0, 0));

        contentPresenter = ref new ContentPresenter();
        contentPresenter->Width = 50;
        contentPresenter->Height = 50;
        contentPresenter->BorderBrush = borderBrush;
        contentPresenter->BorderThickness = ThicknessHelper::FromUniformLength(10);

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(contentPresenter);

        wh->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"> Initial");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Initial").GetString());

    Storyboard^ storyboard = nullptr;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Animating ContentPresenter BorderBrush");

        storyboard = ref new Storyboard();

        CreateColorAnimation(rootCanvas, borderBrush, Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0), Microsoft::UI::ColorHelper::FromArgb(255, 0, 0, 255), 10000000L, false, false, storyboard);

        storyboard->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"Animated").GetString());

    RunOnUIThread([&]()
    {
        storyboard->Stop();
    });
}

} } } } } }
