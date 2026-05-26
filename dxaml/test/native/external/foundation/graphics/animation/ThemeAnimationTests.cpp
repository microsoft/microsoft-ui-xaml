// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThemeAnimationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ ThemeAnimationTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool ThemeAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ThemeAnimationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ThemeAnimationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ThemeAnimationTests::ThemeAnimationTest(
    Platform::String^ fileName,
    Platform::String^ storyboardName,
    Platform::String^ variationName,
    DCompRendering dcompRendering
    )
{
    WUCRenderingScopeGuard wuc(dcompRendering);
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations> slowDownAnimations;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + fileName));
    Storyboard^ sb;
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        sb = safe_cast<Storyboard^>(rootCanvas->FindName(storyboardName));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        sb->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, variationName);

    RunOnUIThread([&]()
    {
        sb->Stop();
        sb->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, variationName);
}

void ThemeAnimationTests::PointerThemeAnimationTest(
    Platform::String^ fileName,
    Platform::String^ storyboardName,
    Platform::String^ variationName
    )
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;
    auto ih = TestServices::InputHelper;
    XamlRoot^ xamlRoot = nullptr;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations> slowDownAnimations;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + fileName));
    Storyboard^ sb;
    Microsoft::UI::Xaml::Shapes::Rectangle^ target;

    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
        target = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootCanvas->FindName(L"r"));
        sb = safe_cast<Storyboard^>(rootCanvas->FindName(storyboardName));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        xamlRoot = rootCanvas->XamlRoot;
    });

    ::Windows::Foundation::Point overridePoint(5.0f, 145.0f);
    wh->SetPrimaryPointerLastPositionOverride(overridePoint, xamlRoot);

    TestCleanupWrapper cleanup([wh, xamlRoot]()
    {
        wh->ClearPrimaryPointerLastPositionOverride(xamlRoot);
    });

    RunOnUIThread([&]()
    {
        sb->Begin();
    });

    // Wait several frames before taking the DComp snapshot. PointerDownThemeAnimation will transform the global point down the
    // tree to the element, then apply a tilt on the element depending on where the click was. Apply this tilt changes the
    // transform on the element, which means on the next frame, the same global point will transform down to a _different_ local
    // point, and the tilt will be different. Allow plenty of frames to let this feedback loop stabilize.
    TestServices::WindowHelper->SynchronouslyTickUIThread(10);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, variationName);

    RunOnUIThread([&]()
    {
        sb->Stop();
    });
}

void ThemeAnimationTests::DragItemThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_dragItem", L"DragItem");
}

void ThemeAnimationTests::DragOverThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_dragOver", L"DragOver");
}

void ThemeAnimationTests::DropTargetThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_dropTarget", L"DropTarget");
}

void ThemeAnimationTests::FadeInThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_fadeIn", L"FadeIn");
}

void ThemeAnimationTests::FadeOutThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_fadeOut", L"FadeOut");
}

void ThemeAnimationTests::PointerDownThemeAnimation()
{
    PointerThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_pointerDown", L"PointerDown");
}

void ThemeAnimationTests::PointerDownThemeAnimationWithTransformGroup()
{
    PointerThemeAnimationTest(L"ThemeAnimationTests-PointerDownWithTargetTransformGroup.xaml", L"_pointerDownWithTransform", L"PointerDownWithTransform");
}

void ThemeAnimationTests::PointerUpThemeAnimation()
{
    // PointerUpThemeAnimation currently no-ops, so this doesn't need pointer input.
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_pointerUp", L"PointerUp");
}

void ThemeAnimationTests::PopInThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_popIn", L"PopIn");
}

void ThemeAnimationTests::PopOutThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_popOut", L"PopOut");
}

void ThemeAnimationTests::RepositionThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_reposition", L"Reposition");
}

void ThemeAnimationTests::SwipeBackThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_swipeBack", L"SwipeBack");
}

void ThemeAnimationTests::SwipeHintThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-ThemeAnimations.xaml", L"_swipeHint", L"SwipeHint");
}

void ThemeAnimationTests::SplitOpenThemeAnimationWUC()
{
    ThemeAnimationTest(L"ThemeAnimationTests-SplitThemeAnimations.xaml", L"_splitOpen", L"SplitOpen");
}

void ThemeAnimationTests::SplitCloseThemeAnimationWUC()
{
    ThemeAnimationTest(L"ThemeAnimationTests-SplitThemeAnimations.xaml", L"_splitClose", L"SplitClose");
}

void ThemeAnimationTests::DrillInThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-DrillThemeAnimations.xaml", L"_drillIn", L"DrillIn");
}

void ThemeAnimationTests::DrillOutThemeAnimation()
{
    ThemeAnimationTest(L"ThemeAnimationTests-DrillThemeAnimations.xaml", L"_drillOut", L"DrillOut");
}

void ThemeAnimationTests::SplitOpenWithClipAnimation(DCompRendering dcompRendering)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations> slowDownAnimations;

    // The theme animation applies an animation on the transition clip. Stack it with another animation on the element clip.

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ThemeAnimationTests-SplitThemeAnimations.xaml"));
    Storyboard^ sb;
    Storyboard^ storyboard;
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
        sb = safe_cast<Storyboard^>(rootCanvas->FindName(L"_splitOpen"));
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        sb->Begin();

        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"clipTx"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"ScaleY");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });

    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"Anim");

    RunOnUIThread([&]()
    {
        sb->Stop();
        storyboard->Stop();
    });

}

void ThemeAnimationTests::SplitOpenWithClipAnimationWUC()
{
    SplitOpenWithClipAnimation(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ThemeAnimationTests::SplitOpenWithOpacityAnimation()
{
    // The theme animation applies an animation on the transition opacity. Stack it with another animation on the element opacity.

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations> slowDownAnimations;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ThemeAnimationTests-SplitThemeAnimations.xaml"));
    Storyboard^ sb;
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        sb = safe_cast<Storyboard^>(rootCanvas->FindName(L"_splitOpen"));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        sb->Begin();

        Microsoft::UI::Xaml::Shapes::Rectangle^ target = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootCanvas->FindName(L"r2"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"Opacity");

        Storyboard^ storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"Anim");
}

void ThemeAnimationTests::SplitCloseWithClipAndOpacityAnimation()
{
    // The theme animation applies an animation on the transition clip. Stack it with another animation on the element clip.

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations> slowDownAnimations;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ThemeAnimationTests-SplitThemeAnimations.xaml"));
    Storyboard^ sb;
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        sb = safe_cast<Storyboard^>(rootCanvas->FindName(L"_splitClose"));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        sb->Begin();

        CompositeTransform^ target = safe_cast<CompositeTransform^>(rootCanvas->FindName(L"clipTx"));

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"ScaleY");

        Storyboard^ storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        storyboard->Begin();

        Microsoft::UI::Xaml::Shapes::Rectangle^ target2 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootCanvas->FindName(L"r"));

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 1.0;
        da2->To = 0.0;
        ::Windows::Foundation::TimeSpan span2; span2.Duration = 2500000L;
        da2->Duration = DurationHelper::FromTimeSpan(span2);
        Storyboard::SetTarget(da2, target2);
        Storyboard::SetTargetProperty(da2, L"Opacity");

        Storyboard^ storyboard2 = ref new Storyboard();
        storyboard2->Children->Append(da2);

        storyboard2->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"Anim");
}

bool ThemeAnimationTests::VerifyMasterPointFuzzy(wf::Point& pt, wf::Point& ptMaster)
{
    LOG_OUTPUT(L"VerifyMasterPointFuzzy():  verifying (%f, %f) against master (%f, %f)", pt.X, pt.Y, ptMaster.X, ptMaster.Y);
    return (fabs(pt.X - ptMaster.X) <= 0.5f)
        && (fabs(pt.Y - ptMaster.Y) <= 0.5f);
}

void ThemeAnimationTests::TiltAnimation()
{
    TiltAnimationTest(false);
}

void ThemeAnimationTests::TiltAnimationWithMousePresent()
{
    // The tilt animation should always take the most recent pointer as its pivot.
    TiltAnimationTest(true);
}

void ThemeAnimationTests::TiltAnimationTest(bool mouseIsPresent)
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ThemeAnimationTests-Tilt.xaml"));
    Button^ button;
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        button = safe_cast<Button^>(rootCanvas->FindName(L"MyButton"));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Clicking on left arrow button to change selection");
    test_infra::TestServices::InputHelper->LeftMouseClick(button);
    TestServices::WindowHelper->WaitForIdle();

    if (mouseIsPresent)
    {
        LOG_OUTPUT(L"Verifying with Mouse in scene");
        TestServices::InputHelper->MoveMouse(wf::Point(5.0f, 5.0f));
    }

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Injecting DynamicPress");
        TestServices::InputHelper->DynamicPressCenter(button, static_cast<int>(button->ActualWidth)/2 - 5, static_cast<int>(button->ActualHeight)/2 - 5, PointerFinger::Finger1);
    });
    TestServices::WindowHelper->WaitForIdle();

    bool passed = false;
    // Unfortunately the PointerAnimationUsingKeyFrames animation is inherently non-deterministic,
    // because of the funky way it computes its progress, it does not have a fixed duration.
    // This makes it impossible to use DComp verification as the final state is not stable in the first place.
    // The approach being taken to verify here is:
    // -Pump up to 120 frames (2 seconds) waiting for the input to be processed (the tap can take a long time to process on phone)
    // -Transform the top/left and bottom/right points to global
    // -Verify the transformed points are within a small threshold
    // This at least verifies that the tilt caused the top/left and bottom/right corners to move as expected.
    for (int i = 0; !passed && i < 60; i++)
    {
        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(button, 0));
            Grid^ grid = safe_cast<Grid^>(controlTemplateRoot->FindName(L"Grid"));

            GeneralTransform^ transformToRoot = grid->TransformToVisual(nullptr);
            wf::Point topLeft = {0.0f, 0.0f};
            wf::Point bottomRight = {static_cast<float>(grid->ActualWidth-1), static_cast<float>(grid->ActualHeight-1)};
            wf::Point topLeftTransformed = transformToRoot->TransformPoint(topLeft);
            wf::Point bottomRightTransformed = transformToRoot->TransformPoint(bottomRight);

            wf::Point topLeftMaster = {22.1f, 23.5f};
            wf::Point bottomRightMaster = {368.9f, 221.1f};
            passed = VerifyMasterPointFuzzy(topLeftTransformed, topLeftMaster)
                && VerifyMasterPointFuzzy(bottomRightTransformed, bottomRightMaster);
        });
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    }
    VERIFY_IS_TRUE(passed);

    TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
    TestServices::WindowHelper->WaitForIdle();

    // Verify that having the mouse in scene will display the correct tilt after a tap.
    if (mouseIsPresent)
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Injecting mouse button down");
            TestServices::InputHelper->MouseButtonDown(wf::Point(static_cast<float>(button->ActualWidth) - 5, static_cast<float>(button->ActualHeight) - 5), MouseButton::Left);
        });

        TestServices::WindowHelper->WaitForIdle();

        // As above, the PointerAnimationUsingKeyFrames animation is inherently non-deterministic.
        TestServices::WindowHelper->SynchronouslyTickUIThread(10);
        TestServices::WindowHelper->WaitForIdle();

        // This mouse input doesn't seem to have the same delay on phone as the tap above. Try it once.
        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(button, 0));
            Grid^ grid = safe_cast<Grid^>(controlTemplateRoot->FindName(L"Grid"));

            GeneralTransform^ transformToRoot = grid->TransformToVisual(nullptr);
            wf::Point topLeft = {0.0f, 0.0f};
            wf::Point bottomRight = {static_cast<float>(grid->ActualWidth-1), static_cast<float>(grid->ActualHeight-1)};
            wf::Point topLeftTransformed = transformToRoot->TransformPoint(topLeft);
            wf::Point bottomRightTransformed = transformToRoot->TransformPoint(bottomRight);

            wf::Point topLeftMaster = {23.3f, 24.0f};
            wf::Point bottomRightMaster = {368.9f, 221.1f};
            VERIFY_IS_TRUE(VerifyMasterPointFuzzy(topLeftTransformed, topLeftMaster));
            VERIFY_IS_TRUE(VerifyMasterPointFuzzy(bottomRightTransformed, bottomRightMaster));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Releasing mouse button");
            TestServices::InputHelper->MouseButtonUp(wf::Point(0,0), MouseButton::Left);
        });
        TestServices::WindowHelper->WaitForIdle();
    }
}

void ThemeAnimationTests::FadeOutThemeAnimationNoDurationWUCFull()
{
    // This test currently covers the fix for [PC AppCompat][Settings]:-[SystemSettings.exe crashes on browsing 'Personalization' setting tabs].
    // It also covers: Xaml objects can be marked as having an independent animation when they don't anymore

    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations> slowDownAnimations;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ThemeAnimationTests-FadeOutZeroDuration.xaml"));
    Button^ button;
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;

        button = safe_cast<Button^>(rootCanvas->FindName(L"button"));
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[GoToState with no transition, plays instant FadeOutThemeAnimation, don't crash]");

        // This has a long duration.
        VisualStateManager::GoToState(button, "FadedFirst", false);

        // This targets the same button but is instant. It should take control of the TransitionTarget.Opacity from "FadedFirst"
        // and complete instantly on the next UI thread frame. But FadedFirst will continue to tick and mark the TransitionTarget
        // as having an opacity animation even when it doesn't. This sets up a state where the TransitionTarget is marked as having
        // an opacity animation (incorrectly, by FadedFirst), but doesn't have a WUC animation object set on it (because FadedSecond
        // completed and removed the WUC animation already).
        VisualStateManager::GoToState(button, "FadedSecond", false);
    });

    // Let the animations tick and do their thing
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        // Poke the element and have it render in the bad state.
        button->Width = 40;
    });

    // This causes the crash that 8600536 hits.
    wh->SynchronouslyTickUIThread(2);
}

} } } } } }
