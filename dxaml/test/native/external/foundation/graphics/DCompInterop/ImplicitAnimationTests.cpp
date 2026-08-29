// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include <WindowsNumerics.h>
#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "ImplicitAnimationTests.h"
#include <GroupDataModel.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <DisableErrorReportingScopeGuard.h>
#include <WUCRenderingScopeGuard.h>
#include <Collection.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <NavigationThemeTransitionTestPage.xaml.h>
#include <CustomTypeMetadataProvider.h>

using namespace Private::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Composition;
using namespace MockDComp;

namespace wfn = ::Windows::Foundation::Numerics;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ ImplicitAnimationTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"Resources\\Native\\Foundation\\Graphics\\DCompInterop\\";
}

bool ImplicitAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ImplicitAnimationTests::ClassCleanup()
{
    return true;
}

bool ImplicitAnimationTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
    return true;
}

bool ImplicitAnimationTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

ScalarKeyFrameAnimation^ ImplicitAnimationTests::CreateOpacityKFA(long durationInSeconds)
{
    Compositor^ compositor = GetCompositor();
    ScalarKeyFrameAnimation^ kfa = compositor->CreateScalarKeyFrameAnimation();
    kfa->Duration = {durationInSeconds * 10000000LL};
    kfa->Target = "Opacity";
    return kfa;
}

void ImplicitAnimationTests::HideAnimation1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));

        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation1BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing Hide animation");
        ElementCompositionPreview::SetImplicitHideAnimation(r3, nullptr);

        LOG_OUTPUT(L"Putting element back into the tree");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing element - should not trigger");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation1CWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation1;
    ScalarKeyFrameAnimation^ opacityAnimation2;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        opacityAnimation1 = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation1->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation1->Duration = span;
        opacityAnimation1->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing Hide animation, currently playing animation should keep playing");
        ElementCompositionPreview::SetImplicitHideAnimation(r3, nullptr);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        auto handoffVisual = ElementCompositionPreview::GetElementVisual(r3);
        handoffVisual->StopAnimationGroup(opacityAnimation1);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r3 back in the tree");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r3 again, should not trigger Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r3 back in the tree and reattaching Hide animation");
        myStackPanel->Children->Append(r3);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation1);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r3 again, should trigger Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Replacing Hide animation while animation is running, should keep playing current animation");
        opacityAnimation2 = compositor->CreateScalarKeyFrameAnimation();
        opacityAnimation2->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {20000000000L};    // 2000 seconds, so we can see difference from opacityAnimation1
        opacityAnimation2->Duration = span;
        opacityAnimation2->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation2);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        auto handoffVisual = ElementCompositionPreview::GetElementVisual(r3);
        handoffVisual->StopAnimationGroup(opacityAnimation1);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r3 back in the tree");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r3 again, should trigger different Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        auto handoffVisual = ElementCompositionPreview::GetElementVisual(r3);
        handoffVisual->StopAnimationGroup(opacityAnimation2);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation2WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    Visual^ handoffVisual3;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));
        handoffVisual3 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual1->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r2, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation on all children");
        myStackPanel->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
        handoffVisual2->StopAnimationGroup(opacityAnimation);
        handoffVisual3->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation3WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing parent, should trigger recursive implicit Hide animation");
        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

}

void ImplicitAnimationTests::HideAnimation3BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value on baseline #2 below, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r2, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing parent, should trigger recursive implicit Hide animations");
        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on r1");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on r2");
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

}

void ImplicitAnimationTests::HideAnimation3CWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer2.xaml"));
    StackPanel^ myStackPanel;
    StackPanel^ sp1;
    xaml_shapes::Rectangle^ r1;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        sp1 = safe_cast<StackPanel^>(root->FindName(L"sp1"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(sp1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value on baseline #2 below, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(sp1, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing parent, should trigger recursive implicit Hide animations");
        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on sp1");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on r1");
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

}

void ImplicitAnimationTests::HideAnimation4WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value on baseline #2 below, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing parent, should trigger implicit Hide animations recursively on both parent and child");

        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on child");
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on parent");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

}

void ImplicitAnimationTests::HideAnimation4BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing parent, trigger implicit Hide animations recursively");

        root->Children->Clear();

        LOG_OUTPUT(L"Removing child, should stay visible and continue to animate");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r1, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation5WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering animation and simultaneously changing subtree, case 1:");
        LOG_OUTPUT(L"Removal of child of subtree is incorporated into the animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r1, &indexToRemove));

        myStackPanel->Children->RemoveAt(indexToRemove);
        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation5BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering animation and simultaneously changing subtree, case 2:");
        LOG_OUTPUT(L"Changes to child of subtree are incorporated");

        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation6WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        ScalarKeyFrameAnimation^ opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(root, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Window Content to null, no animation is triggered");
        wh->WindowContent = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::HideAnimation7WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation1;
    ScalarKeyFrameAnimation^ hideAnimation2;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation1 = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation1->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation1->Duration = span;
        hideAnimation1->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting element back into the tree - should interrupt Hide and play Show animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");

        handoffVisual->StopAnimationGroup(showAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r3 from the tree again, should trigger same Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Replacing Hide animation, should keep playing current animation");
        hideAnimation2 = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation2->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {20000000000L};    // 2000 seconds, so we can tell the difference from hideAnimation1
        hideAnimation2->Duration = span;
        hideAnimation2->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation2);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting element back into the tree - should interrupt Hide and play Show animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");

        handoffVisual->StopAnimationGroup(showAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r3 from the tree, should trigger new Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");

        handoffVisual->StopAnimationGroup(hideAnimation2);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");
}

void ImplicitAnimationTests::HideAnimation7BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove, then immediately re-add r3, should have no net effect");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::HideAnimation7CWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    StackPanel^ sp2;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        sp2 = ref new StackPanel();
        sp2->Width = 100;
        sp2->Height = 100;
        sp2->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        myStackPanel->Children->Append(sp2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove, then immediately add r3 to sp2, should not trigger any animations");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
        sp2->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation8WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Triggering Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));

        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation9WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    Visual^ handoffVisual3;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));
        handoffVisual3 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual1->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r2, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation on each child explicitly");
        myStackPanel->Children->RemoveAt(0);
        myStackPanel->Children->RemoveAt(0);
        myStackPanel->Children->RemoveAt(0);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
        handoffVisual2->StopAnimationGroup(opacityAnimation);
        handoffVisual3->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation10WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual2;
    Visual^ handoffVisual3;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));
        handoffVisual3 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual2->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r2, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting ZIndex and removing");
        xaml_controls::Canvas::SetZIndex(r2, -1);
        xaml_controls::Canvas::SetZIndex(r3, 1);
        myStackPanel->Children->RemoveAt(2);
        myStackPanel->Children->RemoveAt(1);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        handoffVisual2->StopAnimationGroup(opacityAnimation);
        handoffVisual3->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimation11WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicBorder.xaml"));
    Border^ myBorder;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    Compositor^ compositor;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myBorder = safe_cast<Border^>(root->FindName(L"myBorder"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));

        r2 = ref new xaml_shapes::Rectangle();
        r2->Width = 50;
        r2->Height = 50;
        r2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual1->Compositor;

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, hideAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r2, showAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Replacing Border Child");
        myBorder->Child = r2;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating animations");
        handoffVisual1->StopAnimationGroup(hideAnimation);
        handoffVisual2->StopAnimationGroup(showAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::InterruptNestedHideWithShow()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root;
    Grid^ outerElement;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree");

        StackPanel^ innerElement = ref new StackPanel();

        outerElement = ref new Grid();
        outerElement->Children->Append(innerElement);

        root = ref new Canvas();
        root->Children->Append(outerElement);
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating implicit animations. The outer element has hide, and the inner element has both show and hide.");

        ElementCompositionPreview::SetImplicitHideAnimation(outerElement, CreateOpacityKFA(900));

        ElementCompositionPreview::SetImplicitShowAnimation(innerElement, CreateOpacityKFA(1800));
        ElementCompositionPreview::SetImplicitHideAnimation(innerElement, CreateOpacityKFA(800));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Detaching outer element, start playing hide for both.");
        root->Children->Clear();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Hide");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Reattaching outer element to interrupt hide animations.");
        LOG_OUTPUT(L"  The outer hide animation will be cleaned up. The inner hide animation should be replaced with a show animation. Nothing should crash.");
        root->Children->Append(outerElement);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Show");
}

void ImplicitAnimationTests::ShowAnimation1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        myStackPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimation1BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

        LOG_OUTPUT(L"Changing opacity to non-default, should not stomp over animation");
        r4->Opacity = 0.5;

        LOG_OUTPUT(L"Triggering Show animation");
        myStackPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimation2WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation1;
    ScalarKeyFrameAnimation^ opacityAnimation2;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation1 = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation1->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation1->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation1->Duration = span;
        opacityAnimation1->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation1);

        LOG_OUTPUT(L"Triggering Show animation");
        myStackPanel->Children->InsertAt(0, r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Unsetting Show animation, animation should keep playing");
        ElementCompositionPreview::SetImplicitShowAnimation(r4, nullptr);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation1);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r4 from the tree and reattaching animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r4, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation1);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r4 back in the tree, should trigger Show animation");
        myStackPanel->Children->InsertAt(0, r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Replacing animation with new animation, should keep playing current animation");

        opacityAnimation2 = compositor->CreateScalarKeyFrameAnimation();
        opacityAnimation2->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation2->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {20000000000L};    // 2000 seconds, so we can tell the difference between opacityAnimation1
        opacityAnimation2->Duration = span;
        opacityAnimation2->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation2);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation1);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing r4 from the tree");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r4, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r4 back in the tree, should trigger new Show animation");
        myStackPanel->Children->InsertAt(0, r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation2);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

}

void ImplicitAnimationTests::ShowAnimation3WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        StackPanel^ parentPanel = ref new StackPanel();
        parentPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);

        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(parentPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(parentPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animations");
        myStackPanel->Children->Append(parentPanel);
        parentPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimation4WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        StackPanel^ parentPanel = ref new StackPanel();
        parentPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);

        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(parentPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(parentPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animations");
        parentPanel->Children->Append(r4);
        myStackPanel->Children->Append(parentPanel);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimation4BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        StackPanel^ parentPanel = ref new StackPanel();
        parentPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);

        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualChild->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        parentPanel->Children->Append(r4);
        myStackPanel->Children->Append(parentPanel);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimation4CWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    Compositor^ compositor;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual1->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r1, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r2, opacityAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing StackPanel and its children along with it");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(myStackPanel, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Re-adding StackPanel, should trigger nested Show animations");
        root->Children->Append(myStackPanel);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

}


void ImplicitAnimationTests::ShowAnimation5WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        LOG_OUTPUT(L"Attaching implicit animation, should not be triggered");
        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::ShowAnimation5BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        myStackPanel->Children->Append(r4);

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        LOG_OUTPUT(L"Attaching implicit animation, should not be triggered");
        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::ShowAnimation6WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation1;
    ScalarKeyFrameAnimation^ showAnimation2;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation1 = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation1->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation1->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation1->Duration = span;
        showAnimation1->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, showAnimation1);
        ElementCompositionPreview::SetImplicitHideAnimation(r4, hideAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        myStackPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting Show with Hide");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r4, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(hideAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r4 back in the tree, should trigger Show animation again");
        myStackPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Replacing Show animation, should continue playing current animation");
        showAnimation2 = compositor->CreateScalarKeyFrameAnimation();
        showAnimation2->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation2->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {20000000000L};    // 2000 seconds, so we can tell the difference between showAnimation1
        showAnimation2->Duration = span;
        showAnimation2->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(r4, showAnimation2);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting Show with Hide");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r4, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting r4 back in the tree, should trigger new Show animation");
        myStackPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(showAnimation2);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");
}

void ImplicitAnimationTests::ShowAnimation6BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r4));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r4, hideAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Add, then immediately remove r4, for no net effect");
        myStackPanel->Children->Append(r4);
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r4, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::ShowAnimation7WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        StackPanel^ parentPanel = ref new StackPanel();
        parentPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);

        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(parentPanel));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(parentPanel, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        parentPanel->Children->Append(r4);
        myStackPanel->Children->Append(parentPanel);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Changing color of element in animating subtree");
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ShowAnimation8WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r4;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        r4 = ref new xaml_shapes::Rectangle();
        r4->Width = 50;
        r4->Height = 50;
        r4->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r4, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        myStackPanel->Children->Append(r4);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing parent");
        root->Children->Clear();
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse2WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value for baseline #2, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing parent, should trigger implicit Hide animations recursively");
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating parent Hide animation");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating child Hide animation");
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::HideAnimationCollapse2BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animation");
        compositor = handoffVisualChild->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value for baseline #2, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing parent, should trigger implicit Hide animation recursively");
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating  Hide animation");
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse2CWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    Compositor^ compositor;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual1->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value for baseline #2, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r2, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing parent, should trigger implicit Hide animations recursively");
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation 1");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation 2");
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::HideAnimationCollapse2DWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer2.xaml"));
    StackPanel^ myStackPanel;
    StackPanel^ sp1;
    xaml_shapes::Rectangle^ r1;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        sp1 = safe_cast<StackPanel^>(root->FindName(L"sp1"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(sp1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value on baseline #2 below, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(sp1, opacityAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing parent, should trigger recursive implicit Hide animations");
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on r1");
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation on sp1");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::HideAnimationCollapse3WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering animation and simultaneously changing subtree, case 1:");
        LOG_OUTPUT(L"Removal of child of subtree is incorporated into the animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r1, &indexToRemove));

        myStackPanel->Children->RemoveAt(indexToRemove);
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse3BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myStackPanel, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering animation and simultaneously changing subtree, case 2:");
        LOG_OUTPUT(L"Changes to child of subtree are incorporated");

        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse4WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Un-Collapsing - should interrupt Hide and play Show animation");
        r3->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(showAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::HideAnimationCollapse4BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapse then uncollapse for no net effect");
        r3->Visibility = Visibility::Collapsed;
        r3->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::HideAnimationCollapse5WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting new animation");
        ScalarKeyFrameAnimation^ opacityAnimation2 = compositor->CreateScalarKeyFrameAnimation();
        opacityAnimation2->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {20000000000L};    // 2000 seconds, so we can tell the difference from opacityAnimation
        opacityAnimation2->Duration = span;
        opacityAnimation2->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation2);

        LOG_OUTPUT(L"Removing element, should continue playing current animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);

    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse6WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Triggering Hide animation");
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::HideAnimationCollapse7WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        r3->Visibility = Visibility::Collapsed;

        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();
        opacityAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Adding r3 to the tree, should not trigger Hide animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::ShowAnimationUncollapse1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        r3->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse2WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        myStackPanel->Visibility = Visibility::Collapsed;
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(myStackPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animations top to bottom, both animations should trigger");
        myStackPanel->Visibility = Visibility::Visible;
        r3->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse3WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisualParent;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        myStackPanel->Visibility = Visibility::Collapsed;
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisualParent = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisualParent->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(myStackPanel, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animations bottom to top, both animations should trigger");
        r3->Visibility = Visibility::Visible;
        myStackPanel->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        handoffVisualParent->StopAnimationGroup(opacityAnimation);
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse3BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisualChild;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisualChild = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animation");
        compositor = handoffVisualChild->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Uncollapsing parent, should recursively trigger child animation");
        myStackPanel->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisualChild->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse3CWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    Compositor^ compositor;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        r2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r2"));
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r2));

        LOG_OUTPUT(L"Creating Implicit Animation");
        compositor = handoffVisual1->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r1, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r2, opacityAnimation);

        LOG_OUTPUT(L"Uncollapsing parent, should recursively trigger child animations");
        myStackPanel->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse3DWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer2.xaml"));
    StackPanel^ myStackPanel;
    StackPanel^ sp1;
    xaml_shapes::Rectangle^ r1;
    Visual^ handoffVisual1;
    Visual^ handoffVisual2;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        sp1 = safe_cast<StackPanel^>(root->FindName(L"sp1"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visuals");
        handoffVisual1 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(sp1));
        handoffVisual2 = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value on baseline #2 below, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(sp1, opacityAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(r1, opacityAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Uncollapsing parent, should trigger recursive implicit Hide animations");
        myStackPanel->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating parent Show animation");
        handoffVisual1->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating child Show animation");
        handoffVisual2->StopAnimationGroup(opacityAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

}


void ImplicitAnimationTests::ShowAnimationUncollapse4WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        LOG_OUTPUT(L"Uncollapsing");
        r3->Visibility = Visibility::Visible;

        LOG_OUTPUT(L"Setting implicit animation collection - should not trigger");
        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::ShowAnimationUncollapse5WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Show animation");
        r3->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing - should interrupt Show and play Hide animation");
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handoffVisual->StopAnimationGroup(hideAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ShowAnimationUncollapse6WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        myStackPanel->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(myStackPanel));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(myStackPanel, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation and simultaneously changing child subtree");
        myStackPanel->Visibility = Visibility::Visible;
        r3->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handoffVisual->StopAnimationGroup(opacityAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse7WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Triggering Show animation");
        r3->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Cancel by removing parent");
        root->Children->Clear();
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ShowAnimationUncollapse8WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ opacityAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
        r3->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        opacityAnimation = compositor->CreateScalarKeyFrameAnimation();
        opacityAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        opacityAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, opacityAnimation);

        LOG_OUTPUT(L"Uncollapsing and removing, should not trigger animation");
        r3->Visibility = Visibility::Visible;

        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ImplicitAnimationTests::Popup1() { Popup1Common(false); }
void ImplicitAnimationTests::Popup1b() { Popup1Common(true); }
void ImplicitAnimationTests::Popup1Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);

        LOG_OUTPUT(L"Opening Popup, should play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup2()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));

        LOG_OUTPUT(L"Opening Popup");
        myPopup->IsOpen = true;

        LOG_OUTPUT(L"Removing Popup.Child from the tree");
        myPopup->Child = nullptr;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r1, showAnimation);

        LOG_OUTPUT(L"Setting Popup.Child, should play Show animation");
        myPopup->Child = r1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(r1);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup3() { Popup3Common(false); }
void ImplicitAnimationTests::Popup3b() { Popup3Common(true); }
void ImplicitAnimationTests::Popup3Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);

        LOG_OUTPUT(L"Removing Popup from the tree");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(myPopup, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);

        LOG_OUTPUT(L"Putting Popup into the tree, should play Show animation");
        root->Children->Append(myPopup);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup4() { Popup4Common(false); }
void ImplicitAnimationTests::Popup4b() { Popup4Common(true); }
void ImplicitAnimationTests::Popup4Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Closing Popup, should play Hide animation");
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup6() { Popup6Common(false); }
void ImplicitAnimationTests::Popup6b() { Popup6Common(true); }
void ImplicitAnimationTests::Popup6Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Removing Popup from tree, should play Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(myPopup, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup7() { Popup7Common(false); }
void ImplicitAnimationTests::Popup7b() { Popup7Common(true); }
void ImplicitAnimationTests::Popup7Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Opening Popup, should play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Closing Popup, should interrupt Show and play Hide animation");
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::Popup8() { Popup8Common(false); }
void ImplicitAnimationTests::Popup8b() { Popup8Common(true); }
void ImplicitAnimationTests::Popup8Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Closing Popup, should play Hide animation");
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Opening Popup, should interrupt Hide and play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::Popup11() { Popup11Common(false); }
void ImplicitAnimationTests::Popup11b() { Popup11Common(true); }
void ImplicitAnimationTests::Popup11Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Opening Popup, should play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing Popup from tree, should interrupt Show and play Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(myPopup, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::Popup12() { Popup12Common(false); }
void ImplicitAnimationTests::Popup12b() { Popup12Common(true); }
void ImplicitAnimationTests::Popup12Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Removing Popup from tree, should play Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(myPopup, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding Popup back into the tree, should interrupt Hide and play Show animation");
        root->Children->Append(myPopup);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::Popup13() { Popup13Common(false); }
void ImplicitAnimationTests::Popup13b() { Popup13Common(true); }
void ImplicitAnimationTests::Popup13Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"NestedPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_primitives::Popup^ nestedPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Border^ myBorder;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        nestedPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"nestedPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(nestedPopup) : safe_cast<UIElement^>(r1);
        myBorder = safe_cast<Border^>(root->FindName(L"myBorder"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(myBorder, showAnimation);
        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);

        LOG_OUTPUT(L"Opening outer Popup, should play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(myBorder);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Opening inner Popup, should play Show animation");
        nestedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ImplicitAnimationTests::Popup14() { Popup14Common(false); }
void ImplicitAnimationTests::Popup14b() { Popup14Common(true); }
void ImplicitAnimationTests::Popup14Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"NestedPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_primitives::Popup^ nestedPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Border^ myBorder;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        nestedPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"nestedPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(nestedPopup) : safe_cast<UIElement^>(r1);
        myBorder = safe_cast<Border^>(root->FindName(L"myBorder"));
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        nestedPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myBorder, hideAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Closing inner Popup, should play Hide animation");
        nestedPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating inner Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Closing outer Popup, should play Hide animation");
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating outer Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(myBorder);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ImplicitAnimationTests::Popup17() { Popup17Common(false); }
void ImplicitAnimationTests::Popup17b() { Popup17Common(true); }
void ImplicitAnimationTests::Popup17Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root;
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        wh->WindowContent = root;
        myPopup = ref new xaml_primitives::Popup();
        r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 50;
        r1->Height = 50;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
        myPopup->Child = r1;
        wh->Popup_SetWindowed(myPopup);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);

        LOG_OUTPUT(L"Opening Popup, should play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup18() { Popup18Common(false); }
void ImplicitAnimationTests::Popup18b() { Popup18Common(true); }
void ImplicitAnimationTests::Popup18Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root;
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        wh->WindowContent = root;
        myPopup = ref new xaml_primitives::Popup();
        r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 50;
        r1->Height = 50;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
        myPopup->Child = r1;
        wh->Popup_SetWindowed(myPopup);
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Closing Popup, should play Hide animation");
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Popup19()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ animation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        animation = compositor->CreateScalarKeyFrameAnimation();
        animation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        animation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        animation->Duration = span;
        animation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(myPopup, animation);

        animation = compositor->CreateScalarKeyFrameAnimation();
        animation->InsertKeyFrame(0.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        animation->InsertKeyFrame(1.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        animation->Duration = span;
        animation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(myPopup, animation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 50;
        r1->Height = 50;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        myPopup->Child = r1;
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Opening");

    RunOnUIThread([&]()
    {
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Closing");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(myPopup);
        handOffVisual->StopAnimationGroup(animation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

void ImplicitAnimationTests::Popup20() { Popup20Common(false); }
void ImplicitAnimationTests::Popup20b() { Popup20Common(true); }
void ImplicitAnimationTests::Popup20Common(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicPopup.xaml"));
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myPopup = safe_cast<xaml_primitives::Popup^>(root->FindName(L"myPopup"));
        r1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r1"));
        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(myPopup) : safe_cast<UIElement^>(r1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = GetCompositor();

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);

        LOG_OUTPUT(L"Opening Popup, should play Show animation");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Closing Popup");
        myPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ImplicitAnimationTests::HideAnimation_CollapseOrRemovePopupChild()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root;
    xaml_primitives::Popup^ myPopup;
    xaml_shapes::Rectangle^ r1;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree");

        r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 50;
        r1->Height = 50;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

        myPopup = ref new xaml_primitives::Popup();
        myPopup->Child = r1;

        root = ref new Canvas();
        root->Children->Append(myPopup);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating implicit animations");
        compositor = GetCompositor();

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitHideAnimation(myPopup, hideAnimation);

        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(r1);    // For consistency in output - give r1 a primary visual right now

        LOG_OUTPUT(L"> Opening popup");
        myPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Collapsing popup child. Should _not_ play hide animation. The animation is on the popup.");
        myPopup->Child->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "ContentCollapsed");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing popup child.");
        myPopup->Child->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing popup child. Should _not_ play hide animation. The animation is on the popup.");
        myPopup->Child = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "ContentNull");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring popup child.");
        myPopup->Child = r1;
        myPopup->IsOpen = true; // It was implicitly closed when the child was removed
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Put implicit animation on the child instead of the popup");
        ElementCompositionPreview::SetImplicitHideAnimation(myPopup, nullptr);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Collapsing popup child. Should play hide animation.");
        myPopup->Child->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Hide");   // The rectangle is collapsed and won't be hit test visible.

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(r1);
        handOffVisual->StopAnimationGroup(hideAnimation);

        LOG_OUTPUT(L"> Uncollapsing popup child.");
        myPopup->Child->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing popup child. Should play hide animation.");
        myPopup->Child = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Hide2");  // The rectangle is visible and will be hit test visible.

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(r1);
        handOffVisual->StopAnimationGroup(hideAnimation);

        LOG_OUTPUT(L"> Restoring popup child.");
        myPopup->Child = r1;
        myPopup->IsOpen = true; // It was implicitly closed when the child was removed
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(r1);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

void ImplicitAnimationTests::ParentlessPopupShow() { ParentlessPopupShowHideCommon(true /* show */, false /* hide */, false /* putAnimationOnPopup */); }
void ImplicitAnimationTests::ParentlessPopupShow2() { ParentlessPopupShowHideCommon(true /* show */, false /* hide */, true /* putAnimationOnPopup */); }

void ImplicitAnimationTests::ParentlessPopupHide() { ParentlessPopupShowHideCommon(false /* show */, true /* hide */, false /* putAnimationOnPopup */); }
void ImplicitAnimationTests::ParentlessPopupHide2() { ParentlessPopupShowHideCommon(false /* show */, true /* hide */, true /* putAnimationOnPopup */); }

void ImplicitAnimationTests::ParentlessPopupShowHide() { ParentlessPopupShowHideCommon(true /* show */, true /* hide */, false /* putAnimationOnPopup */); }
void ImplicitAnimationTests::ParentlessPopupShowHide2() { ParentlessPopupShowHideCommon(true /* show */, true /* hide */, true /* putAnimationOnPopup */); }

void ImplicitAnimationTests::ParentlessPopupShowHideCommon(bool show, bool hide, bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_primitives::Popup^ popup;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree");
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        xaml_shapes::Rectangle^ content = ref new xaml_shapes::Rectangle();
        content->Width = 50;
        content->Height = 50;
        content->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        popup = ref new xaml_primitives::Popup();
        popup->Child = content;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup->XamlRoot = xamlRoot;
            }
        }

        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(popup) : safe_cast<UIElement^>(content);

        LOG_OUTPUT(L"> Creating implicit animations");
        compositor = GetCompositor();

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        if (show)
        {
            showAnimation = compositor->CreateScalarKeyFrameAnimation();
            showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
            showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
            showAnimation->Duration = span;
            showAnimation->Target = "Opacity";
            ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);
        }

        if (hide)
        {
            hideAnimation = compositor->CreateScalarKeyFrameAnimation();
            hideAnimation->InsertKeyFrame(0.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
            hideAnimation->InsertKeyFrame(1.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
            hideAnimation->Duration = span;
            hideAnimation->Target = "Opacity";
            ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);
        }
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening parentless popup");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();
    if (show)
    {
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Opening");
    }

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing parentless popup");
        popup->IsOpen = false;
    });
    wh->WaitForIdle();
    if (hide)
    {
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Closing");
    }

    if (hide)
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Forcefully terminating Hide animation");
            Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
            handOffVisual->StopAnimationGroup(hideAnimation);
        });
        wh->WaitForImplicitShowHideComplete();
        wh->WaitForIdle();
    }
}

void ImplicitAnimationTests::AnimatePopupAndContent()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_shapes::Rectangle^ content;
    xaml_primitives::Popup^ popup;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ popupShowAnimation;
    ScalarKeyFrameAnimation^ contentShowAnimation;
    ScalarKeyFrameAnimation^ popupHideAnimation;
    ScalarKeyFrameAnimation^ contentHideAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree");
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        content = ref new xaml_shapes::Rectangle();
        content->Width = 50;
        content->Height = 50;
        content->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        popup = ref new xaml_primitives::Popup();
        popup->Child = content;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Creating implicit animations");
        compositor = GetCompositor();

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        popupShowAnimation = compositor->CreateScalarKeyFrameAnimation();
        popupShowAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        popupShowAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        popupShowAnimation->Duration = span;
        popupShowAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(popup, popupShowAnimation);

        contentShowAnimation = compositor->CreateScalarKeyFrameAnimation();
        contentShowAnimation->InsertKeyFrame(0.0f, 0.75f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        contentShowAnimation->InsertKeyFrame(1.0f, 0.75f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        contentShowAnimation->Duration = span;
        contentShowAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(content, contentShowAnimation);

        popupHideAnimation = compositor->CreateScalarKeyFrameAnimation();
        popupHideAnimation->InsertKeyFrame(0.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        popupHideAnimation->InsertKeyFrame(1.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        popupHideAnimation->Duration = span;
        popupHideAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(popup, popupHideAnimation);

        contentHideAnimation = compositor->CreateScalarKeyFrameAnimation();
        contentHideAnimation->InsertKeyFrame(0.0f, 0.25f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        contentHideAnimation->InsertKeyFrame(1.0f, 0.25f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        contentHideAnimation->Duration = span;
        contentHideAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(content, contentHideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Opening");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing popup");
        popup->IsOpen = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Closing");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(popup);
        handOffVisual->StopAnimationGroup(popupHideAnimation);
        handOffVisual = ElementCompositionPreview::GetElementVisual(content);
        handOffVisual->StopAnimationGroup(contentHideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

void ImplicitAnimationTests::CollapsePopup() { CollapsePopupCommon(false /* parentlessPopup */, false); }
void ImplicitAnimationTests::CollapsePopup2() { CollapsePopupCommon(false /* parentlessPopup */, true); }
void ImplicitAnimationTests::CollapseParentlessPopup() { CollapsePopupCommon(true /* parentlessPopup */, false); }
void ImplicitAnimationTests::CollapseParentlessPopup2() { CollapsePopupCommon(true /* parentlessPopup */, true); }
void ImplicitAnimationTests::CollapsePopupCommon(bool parentlessPopup, bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_primitives::Popup^ popup;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree");

        xaml_shapes::Rectangle^ content = ref new xaml_shapes::Rectangle();
        content->Width = 50;
        content->Height = 50;
        content->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        popup = ref new xaml_primitives::Popup();
        popup->Child = content;

        Canvas^ root = ref new Canvas();
        if (!parentlessPopup)
        {
            root->Children->Append(popup);
        }
        wh->WindowContent = root;

        if (parentlessPopup)
        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup->XamlRoot = xamlRoot;
            }
        }
        popup->IsOpen = true;

        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(popup) : safe_cast<UIElement^>(content);

        LOG_OUTPUT(L"> Creating implicit animations");
        compositor = GetCompositor();

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(0.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->InsertKeyFrame(1.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing popup");
        popup->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Hide");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing popup");
        popup->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Show");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing popup again");
        popup->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Hide");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        popup->IsOpen = false;
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

void ImplicitAnimationTests::CollapsePopupAncestor() { CollapsePopupAncestorCommon(false); }
void ImplicitAnimationTests::CollapsePopupAncestor2() { CollapsePopupAncestorCommon(true); }
void ImplicitAnimationTests::CollapsePopupAncestorCommon(bool putAnimationOnPopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ ancestor;
    xaml_primitives::Popup^ popup;
    UIElement^ animatedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree");

        xaml_shapes::Rectangle^ content = ref new xaml_shapes::Rectangle();
        content->Width = 50;
        content->Height = 50;
        content->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        popup = ref new xaml_primitives::Popup();
        popup->Child = content;

        ancestor = ref new Grid();
        ancestor->Children->Append(popup);

        Canvas^ root = ref new Canvas();
        root->Children->Append(ancestor);
        wh->WindowContent = root;

        popup->IsOpen = true;

        animatedElement = putAnimationOnPopup ? safe_cast<UIElement^>(popup) : safe_cast<UIElement^>(content);

        LOG_OUTPUT(L"> Creating implicit animations");
        compositor = GetCompositor();

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(animatedElement, showAnimation);

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(0.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->InsertKeyFrame(1.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing ancestor");
        ancestor->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Collapsing");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing ancestor");
        ancestor->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Uncollapsing");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing ancestor again");
        ancestor->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Collapsing");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

void ImplicitAnimationTests::CollapseWhilePopupDescendantHasHideAnimation() { CollapseWhilePopupDescendantHasHideAnimationCommon(false); }
void ImplicitAnimationTests::CollapseWhilePopupDescendantHasHideAnimation2() { CollapseWhilePopupDescendantHasHideAnimationCommon(true); }
void ImplicitAnimationTests::CollapseWhilePopupDescendantHasHideAnimationCommon(bool collapsePopup)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_shapes::Rectangle^ animatedElement;
    Grid^ content;
    xaml_primitives::Popup^ popup;
    UIElement^ collapsedElement;
    Compositor^ compositor;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree");

        animatedElement = ref new xaml_shapes::Rectangle();
        animatedElement->Width = 50;
        animatedElement->Height = 50;
        animatedElement->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        Grid^ parent = ref new Grid();
        parent->Children->Append(animatedElement);

        Grid^ grandparent = ref new Grid();
        grandparent->Children->Append(parent);

        content = ref new Grid();
        content->Children->Append(grandparent);

        popup = ref new xaml_primitives::Popup();
        popup->Child = content;

        Canvas^ root = ref new Canvas();
        root->Children->Append(popup);
        wh->WindowContent = root;

        popup->IsOpen = true;

        collapsedElement = collapsePopup ? safe_cast<UIElement^>(popup) : safe_cast<UIElement^>(content);

        LOG_OUTPUT(L"> Creating implicit animations");
        compositor = GetCompositor();

        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(0.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->InsertKeyFrame(1.0f, 0.5f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(animatedElement, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing animated descendant");
        animatedElement->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing ancestor. It should stay visible for the hide animation on the descendant.");
        collapsedElement->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing ancestor.");
        collapsedElement->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Hide animation");
        Visual^ handOffVisual = ElementCompositionPreview::GetElementVisual(animatedElement);
        handOffVisual->StopAnimationGroup(hideAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

void ImplicitAnimationTests::ListView1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ showAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ListView2WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Setting empty ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView2BWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    auto newItems = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Populating new ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            newItems->Append(L"New Item " + i);
        }

        LOG_OUTPUT(L"Setting new ItemsSource");
        listView->ItemsSource = newItems;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ImplicitAnimationTests::ListView3WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing ListView");
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(listView, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource while ListView is not in the live Tree");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding ListView to live tree, should realize containers and play Show animations");
        root->Children->Append(listView);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView4WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing ListView");
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));
        listView->Visibility = Visibility::Collapsed;

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Uncollapsing ListView, should realize containers and play Show animations");
        listView->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView5WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Adding initial item, should trigger Show animation");
        items->Append(L"Item 1");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
        auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
        handoffVisual->StopAnimationGroup(showAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding another item after initial load, should not trigger Show animation");
        items->Append(L"Item 2");
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView6WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 200));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 10;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        auto isp = safe_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);
        for (unsigned int i = 0; i <= static_cast<unsigned int>(isp->LastCacheIndex); i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Scrolling into view, should not animate newly realized items");
        listView->ScrollIntoView(listView->Items->GetAt(9));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView7WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitHideAnimation(lvi, hideAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing ListView, should trigger Hide animations");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(listView, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView8WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitHideAnimation(lvi, hideAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing ListView, should trigger Hide animations");
        listView->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView9WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    Visual^ handOffVisuals[3];
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitHideAnimation(lvi, hideAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            handOffVisuals[i] = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
        }

        LOG_OUTPUT(L"Resetting ItemsSource, should trigger Hide animations");
        listView->ItemsSource = nullptr;
     });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            handOffVisuals[i]->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView10WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    Visual^ handOffVisual;
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitHideAnimation(lvi, hideAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
        handOffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));

        LOG_OUTPUT(L"Removing one item, should not trigger Hide animation");
        items->RemoveAt(0);
     });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ListView11WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    Visual^ handOffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show and Hide
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            ElementCompositionPreview::SetImplicitHideAnimation(lvi, hideAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing ListView, should trigger Hide animations");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(listView, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void ImplicitAnimationTests::ListView12WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    Visual^ handOffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show and Hide
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            ElementCompositionPreview::SetImplicitShowAnimation(lvi, showAnimation);
            ElementCompositionPreview::SetImplicitHideAnimation(lvi, hideAnimation);
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing ListView, should cancel Show and trigger Hide animations");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(listView, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvi));
            handoffVisual->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::ListView13WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    ListView^ listView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ hideAnimation;
    UIElement^ subElement;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        listView = safe_cast<ListView^>(root->FindName(L"myListView"));

        choosingItemContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto lvi = ref new xaml_controls::ListViewItem();
            lvi->Style = sender->ItemContainerStyle;
            args->ItemContainer = lvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        listView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Attaching implicit animation to sub-element");
        Compositor^ compositor = GetCompositor();
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";
        auto lvi = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
        subElement = safe_cast<UIElement^>(lvi->ContentTemplateRoot);
        ElementCompositionPreview::SetImplicitHideAnimation(subElement, hideAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Collapsing sub-element, should not play Hide animation");
        subElement->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

}

void ImplicitAnimationTests::GroupedListView1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(ListViewBase, Loaded);
    auto choosingGroupHeaderContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingGroupHeaderContainer);

    xaml_controls::Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GroupedListView.xaml"));
    ListViewBase^ listView;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    auto headers = ref new Platform::Collections::Vector<xaml_controls::ListViewHeaderItem^>();

    RunOnUIThread([&] ()
    {
        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        listView = safe_cast<xaml_controls::ListViewBase^>(root->FindName(L"list"));
        auto cvs = safe_cast<xaml_data::CollectionViewSource^>(root->FindName(L"cvs"));

        choosingGroupHeaderContainerRegistration.Attach(listView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingGroupHeaderContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingGroupHeaderContainerEventArgs^ args)
        {
            auto lvhi = ref new xaml_controls::ListViewHeaderItem();
            headers->Append(lvhi);
            ElementCompositionPreview::SetImplicitShowAnimation(lvhi, showAnimation);
            ElementCompositionPreview::SetImplicitHideAnimation(lvhi, hideAnimation);
            args->GroupHeaderContainer = lvhi;
        }));

        auto dataCollection = MocoBasicDataSource::GetGroupedDataCollection(3, 1);
        cvs->Source = dataCollection;
        cvs->IsSourceGrouped = true;

        wh->WindowContent = root;

        loadedRegistration.Attach(listView, ref new xaml::RoutedEventHandler(
            [spHasLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
        {
            spHasLoadedEvent->Set();
        }));

    });
    spHasLoadedEvent->WaitForDefault();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < headers->Size; i++)
        {
            auto lvhi = safe_cast<xaml_controls::ListViewHeaderItem^>(headers->GetAt(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvhi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing ListView, should trigger Hide animations");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(listView, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < headers->Size; i++)
        {
            auto lvhi = safe_cast<xaml_controls::ListViewHeaderItem^>(headers->GetAt(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(lvhi));
            handoffVisual->StopAnimationGroup(hideAnimation);
        }
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

}

void ImplicitAnimationTests::GridView1WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    GridView^ gridView;
    unsigned int numItems = 3;
    ScalarKeyFrameAnimation^ showAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        gridView = safe_cast<GridView^>(root->FindName(L"myGridView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(gridView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto gvi = ref new xaml_controls::GridViewItem();
            ElementCompositionPreview::SetImplicitShowAnimation(gvi, showAnimation);
            args->ItemContainer = gvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        gridView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Show animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto gvi = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(i));
            auto handoffVisual = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(gvi));
            handoffVisual->StopAnimationGroup(showAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::GridView2WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    GridView^ gridView;
    unsigned int numItems = 3;
    Visual^ handOffVisuals[3];
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        gridView = safe_cast<GridView^>(root->FindName(L"myGridView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(gridView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto gvi = ref new xaml_controls::GridViewItem();
            ElementCompositionPreview::SetImplicitHideAnimation(gvi, hideAnimation);
            args->ItemContainer = gvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        gridView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto gvi = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(i));
            handOffVisuals[i] = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(gvi));
        }

        LOG_OUTPUT(L"Resetting ItemsSource, should trigger Hide animations");
        gridView->ItemsSource = nullptr;
     });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            handOffVisuals[i]->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::GridView3WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));

    auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

    GridView^ gridView;
    unsigned int numItems = 3;
    Visual^ handOffVisuals[3];
    ScalarKeyFrameAnimation^ hideAnimation;
    auto items = ref new Platform::Collections::Vector<Platform::String^>();

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        gridView = safe_cast<GridView^>(root->FindName(L"myGridView"));

        LOG_OUTPUT(L"Creating Implicit Animations");
        Compositor^ compositor = GetCompositor();

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        choosingItemContainerRegistration.Attach(gridView,
            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
            [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
        {
            auto gvi = ref new xaml_controls::GridViewItem();
            ElementCompositionPreview::SetImplicitHideAnimation(gvi, hideAnimation);
            args->ItemContainer = gvi;
            args->IsContainerPrepared = true;
        }));

        LOG_OUTPUT(L"Populating ItemsSource");
        for (unsigned int i = 0; i < numItems; i++)
        {
            items->Append(L"Item " + i);
        }

        LOG_OUTPUT(L"Setting ItemsSource");
        gridView->ItemsSource = items;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        for (unsigned int i = 0; i < numItems; i++)
        {
            auto gvi = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(i));
            handOffVisuals[i] = safe_cast<Visual^>(ElementCompositionPreview::GetElementVisual(gvi));
        }

        LOG_OUTPUT(L"Removing GridView, should trigger Hide animations");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(root->Children->IndexOf(gridView, &indexToRemove));
        root->Children->RemoveAt(indexToRemove);
     });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        for (unsigned int i = 0; i < numItems; i++)
        {
            handOffVisuals[i]->StopAnimationGroup(hideAnimation);
        }
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::AnimationsDisabled()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    bool wasAnimationDisabled = false;
    u->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations), true, &wasAnimationDisabled);
    TestCleanupWrapper cleanup([&]()
    {
        u->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations), wasAnimationDisabled, nullptr);
    });

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing element - should not play Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting element back into the tree - should not play Show animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::ImplicitPlusThemeTransition()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;
    Frame^ rootFrame;
    Page^ p1;
    xaml_shapes::Rectangle^ r1;
    xaml_shapes::Rectangle^ r2;
    ScalarKeyFrameAnimation^ hideAnimation;
    Visual^ handoffVisual;

    RunOnUIThread([&]()
    {
        p1 = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        p1->Transitions = ref new xaml_animation::TransitionCollection();
        p1->Transitions->Append(ref new xaml_animation::NavigationThemeTransition());
        r1 = ref new xaml_shapes::Rectangle();
        r1->Width = 200;
        r1->Height = 200;
        r1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
        p1->Content = r1;

        // Implicitly animate the Opacity during Hide
        auto compositor = GetCompositor();
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";
        handoffVisual = ElementCompositionPreview::GetElementVisual(r1);
        ElementCompositionPreview::SetImplicitHideAnimation(r1, hideAnimation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        p1->Frame->Navigate(wxaml_interop::TypeName(CustomTypes::NavigationThemeTransitionTestPage::typeid));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcefully terminating Hide animations");
        handoffVisual->StopAnimationGroup(hideAnimation);
    });

    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void ImplicitAnimationTests::Closed()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Closing animations");
        {
            wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(showAnimation));
            wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
            VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
            VERIFY_SUCCEEDED(spClosable->Close());
        }
        {
            wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(hideAnimation));
            wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
            VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
            VERIFY_SUCCEEDED(spClosable->Close());
        }

        LOG_OUTPUT(L"Removing element - should not trigger animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting element back into the tree - should not play Show animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Closed2()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);

        LOG_OUTPUT(L"Closing animations");
        {
            wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(showAnimation));
            wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
            VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
            VERIFY_SUCCEEDED(spClosable->Close());
        }
        {
            wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(hideAnimation));
            wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
            VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
            VERIFY_SUCCEEDED(spClosable->Close());
        }

    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing element - should not trigger animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting element back into the tree - should not play Show animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void ImplicitAnimationTests::Closed3()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BasicContainer.xaml"));
    StackPanel^ myStackPanel;
    xaml_shapes::Rectangle^ r3;
    Compositor^ compositor;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ showAnimation;
    ScalarKeyFrameAnimation^ hideAnimation;

    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
        myStackPanel = safe_cast<StackPanel^>(root->FindName(L"myStackPanel"));
        r3 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"r3"));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r3));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the Opacity during Show
        showAnimation = compositor->CreateScalarKeyFrameAnimation();

        // Animate from 1.0 => 1.0, otherwise we'll land on a random value at the end of the animation, which will cause instability
        showAnimation->InsertKeyFrame(0.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";

        // Implicitly animate the Opacity during Hide
        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";

        ElementCompositionPreview::SetImplicitShowAnimation(r3, showAnimation);
        ElementCompositionPreview::SetImplicitHideAnimation(r3, hideAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Triggering Hide animation");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(myStackPanel->Children->IndexOf(r3, &indexToRemove));
        myStackPanel->Children->RemoveAt(indexToRemove);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        // For some reason, closing the animation does not make it stop playing.
        // We actually want the animation to complete to test the cleanup code,
        // so forcefully stop it before closing.
        handoffVisual->StopAnimationGroup(hideAnimation);

        LOG_OUTPUT(L"Closing Hide animation");
        wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(hideAnimation));
        wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
        VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
        VERIFY_SUCCEEDED(spClosable->Close());
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Putting element back into the tree - trigger Show animation");
        myStackPanel->Children->Append(r3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        handoffVisual->StopAnimationGroup(showAnimation);

        LOG_OUTPUT(L"Closing Show animation");
        wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(showAnimation));
        wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
        VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
        VERIFY_SUCCEEDED(spClosable->Close());
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

}

void ImplicitAnimationTests::HideAnimationBelowOpacity0()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ implicitShowHideCanvas;
    Canvas^ opacityCanvas;
    Visual^ handoffVisual;
    ScalarKeyFrameAnimation^ hideAnimation;
    ScalarKeyFrameAnimation^ showAnimation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");

        implicitShowHideCanvas = ref new Canvas();
        implicitShowHideCanvas->Width = 50;
        implicitShowHideCanvas->Height = 50;
        implicitShowHideCanvas->Background = ref new SolidColorBrush(mu::Colors::Red);

        opacityCanvas = ref new Canvas();
        opacityCanvas->Children->Append(implicitShowHideCanvas);

        Canvas^ root = ref new Canvas();
        root->Children->Append(opacityCanvas);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting implicit animations.");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds

        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(implicitShowHideCanvas));

        Compositor^ compositor = handoffVisual->Compositor;

        hideAnimation = compositor->CreateScalarKeyFrameAnimation();
        hideAnimation->InsertKeyFrame(1.0f, 0.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        hideAnimation->Duration = span;
        hideAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitHideAnimation(implicitShowHideCanvas, hideAnimation);

        showAnimation = compositor->CreateScalarKeyFrameAnimation();
        showAnimation->InsertKeyFrame(1.0f, 1.0f, CompositionEasingFunction::CreateLinearEasingFunction(compositor));
        showAnimation->Duration = span;
        showAnimation->Target = "Opacity";
        ElementCompositionPreview::SetImplicitShowAnimation(implicitShowHideCanvas, showAnimation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Triggering Hide animation.");
        implicitShowHideCanvas->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting opacity 0 on Hide element's parent. This removes the Hide element's comp node.");
        opacityCanvas->Opacity = 0;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Triggering Show animation. This attempts to cancel the Hide animation on an element without a comp node. Don't crash.");
        implicitShowHideCanvas->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting opacity 1 on Show element's parent. The animation should be visible now.");
        opacityCanvas->Opacity = 1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Forcefully terminating Show animation.");
        handoffVisual->StopAnimationGroup(showAnimation);
    });
    wh->WaitForImplicitShowHideComplete();
    wh->WaitForIdle();
}

Microsoft::UI::Composition::Compositor^ ImplicitAnimationTests::GetCompositor()
{
    return Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
}

}}}}}}

