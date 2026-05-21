// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ObjectAnimationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool ObjectAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ObjectAnimationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ObjectAnimationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ObjectAnimationTests::ObjectKeyFrameTransformAnimation()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = nullptr;
    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    LOG_OUTPUT(L"Initial");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"Create Object Animation on Rectangle.RenderTransform");
    RunOnUIThread([&]()
    {
        auto rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
        rect->Width = 50;
        rect->Height = 50;
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        Canvas::SetTop(rect, 50);
        Canvas::SetLeft(rect, 50);

        rootCanvas->Children->Append(rect);

        auto objectAnim = ref new ObjectAnimationUsingKeyFrames();
        {
            objectAnim->AllowDependentAnimations = true;

            auto keyFrame = ref new DiscreteObjectKeyFrame();
            auto transform = ref new TranslateTransform();
            transform->X = 500;
            transform->Y = 0;
            keyFrame->Value = transform;

            KeyTime keyTime;
            TimeSpan keyTimeSpan;
            keyTimeSpan.Duration = 0;
            keyTime.TimeSpan = keyTimeSpan;
            keyFrame->KeyTime = keyTime;

            objectAnim->KeyFrames->Append(keyFrame);

            TimeSpan timeSpanForAnim;
            timeSpanForAnim.Duration = 1;
            objectAnim->Duration = DurationHelper::FromTimeSpan(timeSpanForAnim);
            objectAnim->FillBehavior = FillBehavior::HoldEnd;

            Storyboard::SetTarget(objectAnim, rect);
            Storyboard::SetTargetProperty(objectAnim, "Rectangle.RenderTransform");
        }

        storyboard = ref new Storyboard();
        storyboard->Children->Append(objectAnim);
        storyboard->Begin();
    });

    LOG_OUTPUT(L"Verify TranslateTransform Applied");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    RunOnUIThread([&]()
    {
        auto rect = (Microsoft::UI::Xaml::Shapes::Rectangle^)rootCanvas->Children->GetAt(0);
        VERIFY_IS_NOT_NULL(rect->RenderTransform);

        auto translateTransform = static_cast<TranslateTransform^>(rect->RenderTransform);
        VERIFY_ARE_EQUAL(translateTransform->X, 500);

        storyboard->Stop();
    });

    LOG_OUTPUT(L"Verify TranslateTransform Removed");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    RunOnUIThread([&]()
    {
        auto rect = (Microsoft::UI::Xaml::Shapes::Rectangle^)rootCanvas->Children->GetAt(0);
        VERIFY_IS_NOT_NULL(rect->RenderTransform);

        auto translateTransform = dynamic_cast<TranslateTransform^>(rect->RenderTransform);
        VERIFY_IS_NULL(translateTransform);
    });
}

void ObjectAnimationTests::RectangleGeometry_NullRect()
{
    const auto& wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
    });

    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = nullptr;
    RectangleGeometry^ clip = nullptr;
    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();

        clip = ref new RectangleGeometry();
        clip->Rect = ::Windows::Foundation::Rect(0, 0, 50, 50);
        rootCanvas->Clip = clip;

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Create ObjectAnimationUsingKeyFrames on RectangleGeometry.Rect to null]");
    RunOnUIThread([&]()
    {
        auto objectAnim = ref new ObjectAnimationUsingKeyFrames();
        objectAnim->AllowDependentAnimations = true;

        auto keyFrame = ref new DiscreteObjectKeyFrame();
        keyFrame->Value = nullptr;

        KeyTime keyTime;
        TimeSpan keyTimeSpan;
        keyTimeSpan.Duration = 0;
        keyTime.TimeSpan = keyTimeSpan;
        keyFrame->KeyTime = keyTime;

        objectAnim->KeyFrames->Append(keyFrame);

        TimeSpan timeSpanForAnim;
        timeSpanForAnim.Duration = 1;
        objectAnim->Duration = DurationHelper::FromTimeSpan(timeSpanForAnim);
        objectAnim->FillBehavior = FillBehavior::HoldEnd;

        Storyboard::SetTarget(objectAnim, clip);
        Storyboard::SetTargetProperty(objectAnim, "RectangleGeometry.Rect");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(objectAnim);
        storyboard->Begin();
    });

    LOG_OUTPUT(L"[We shouldn't crash]");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
}

void ObjectAnimationTests::Margin_NullThickness()
{
    const auto& wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
    });

    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = nullptr;
    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        rootCanvas->Margin = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(5);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    Storyboard^ storyboard = nullptr;

    LOG_OUTPUT(L"[Create ObjectAnimationUsingKeyFrames on Canvas.Margin to null]");
    RunOnUIThread([&]()
    {
        auto objectAnim = ref new ObjectAnimationUsingKeyFrames();
        objectAnim->AllowDependentAnimations = true;

        auto keyFrame = ref new DiscreteObjectKeyFrame();
        keyFrame->Value = nullptr;

        KeyTime keyTime;
        TimeSpan keyTimeSpan;
        keyTimeSpan.Duration = 0;
        keyTime.TimeSpan = keyTimeSpan;
        keyFrame->KeyTime = keyTime;

        objectAnim->KeyFrames->Append(keyFrame);

        TimeSpan timeSpanForAnim;
        timeSpanForAnim.Duration = 1;
        objectAnim->Duration = DurationHelper::FromTimeSpan(timeSpanForAnim);
        objectAnim->FillBehavior = FillBehavior::HoldEnd;

        Storyboard::SetTarget(objectAnim, rootCanvas);
        Storyboard::SetTargetProperty(objectAnim, "Margin");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(objectAnim);
        storyboard->Begin();
    });

    LOG_OUTPUT(L"[We shouldn't crash]");
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
}

} } } } } }
