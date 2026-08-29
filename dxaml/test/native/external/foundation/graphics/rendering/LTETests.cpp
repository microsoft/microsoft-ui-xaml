// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LTETests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include "SCPTestHelper.h"
#include "MediaHelperMPE.h"

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ LTETests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool LTETests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool LTETests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool LTETests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void LTETests::Insert3Rectangles(GridView^ gridView)
{
    const auto& wh = TestServices::WindowHelper;

    // Force some frames between inserts into the GridView. If we insert items too fast, the AddRemoveThemeTransition gets skipped.
    // See GetSpeedOfChanges and ModernCollectionBasePanel::TransitionContextManager::IsCollectionMutatingFast.
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a new rectangle to trigger portaling. Don't crash.]");

        auto newChild = ref new xaml_shapes::Rectangle();
        newChild->Width = 50;
        newChild->Height = 50;
        newChild->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        gridView->Items->InsertAt(0, newChild);
    });

    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);   // Wait before adding more so the transition plays

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a new rectangle to trigger portaling of the element being tested. Don't crash.]");

        auto newChild = ref new xaml_shapes::Rectangle();
        newChild->Width = 50;
        newChild->Height = 50;
        newChild->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);

        gridView->Items->InsertAt(0, newChild);
    });

    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);   // Wait before adding more so the transition plays

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a new rectangle to trigger portaling. Don't crash.]");

        auto newChild = ref new xaml_shapes::Rectangle();
        newChild->Width = 50;
        newChild->Height = 50;
        newChild->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);

        gridView->Items->InsertAt(0, newChild);
    });

    wh->WaitForIdle();
}

void LTETests::PortalingMediaElement()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas;
    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    MediaPlayerElement^ mpe;
    auto playbackStateChangedEvent = std::make_shared<Event>();
    auto eventRegistration = CreateSafeEventRegistration(MediaPlaybackSession, PlaybackStateChanged);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a MediaPlayerElement. Don't crash.]");
        mpe = ref new xaml_controls::MediaPlayerElement();
        mpe->AutoPlay = true;
        mpe->MediaPlayer->IsLoopingEnabled = true;
        mpe->Width = 50;
        mpe->Height = 50;
        rootCanvas->Children->Append(mpe);

        eventRegistration.Attach(
            mpe->MediaPlayer->PlaybackSession,
            ref new wf::TypedEventHandler<MediaPlaybackSession ^,Platform::Object ^>([&](MediaPlaybackSession^ session, Platform::Object^)
            {
                if (session->PlaybackState == MediaPlaybackState::Playing)
                {
                    LOG_OUTPUT(L"> MediaPlaybackSession.PlaybackStateChanged fired, current state is Playing");
                    playbackStateChangedEvent->Set();
                }
            }));

        auto testUri = ref new Uri(Microsoft::UI::Xaml::Tests::Foundation::Graphics::Media::MediaHelperMPE::GetDefaultResourcePath() + L"blueframe_video.mp4");
        VERIFY_IS_NOT_NULL(testUri);
        auto source = ::Windows::Media::Core::MediaSource::CreateFromUri(testUri);
        mpe->Source = source;
    });

    playbackStateChangedEvent->WaitForDefault();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add an LTE targeting the MediaPlayerElement. Don't crash.]");
        wh->AddTestLTE(mpe, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a second LTE targeting the MediaPlayerElement. Don't crash.]");
        wh->AddTestLTE(mpe, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a third LTE targeting the MediaPlayerElement. Don't crash.]");
        wh->AddTestLTE(mpe, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });

    wh->WaitForIdle();
}

void LTETests::PortalingSwapChainPanel()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas;
    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    SwapChainPanel^ scp;
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Creating swap chain]");
        scp = ref new SwapChainPanel();
        scp->Width = 50;
        scp->Height = 50;
        rootCanvas->Children->Append(scp);

        swapChainTestWrapper.CreateSwapChain(scp,50,50, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Orange,1));
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add an LTE targeting the SwapChainPanel. Don't crash.]");
        wh->AddTestLTE(scp, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a second LTE targeting the SwapChainPanel. Don't crash.]");
        wh->AddTestLTE(scp, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a third LTE targeting the SwapChainPanel. Don't crash.]");
        wh->AddTestLTE(scp, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });

    wh->WaitForIdle();
}

void LTETests::PortalingRectangleWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    GridView^ gridView;
    RunOnUIThread([&]() { gridView = safe_cast<GridView^>(rootCanvas->FindName(L"gridView")); });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Add a Rectangle. Don't crash.]");
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        gridView->Items->InsertAt(0, rectangle);
    });

    wh->WaitForIdle();

    Insert3Rectangles(gridView);
}

void LTETests::PortalingCompNodeSubtree()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ root;
    Canvas^ lteTarget;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating chain of comp nodes.");

        Canvas^ one = ref new Canvas();
        one->Width = 10;
        one->Height = 10;
        one->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        one->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ two = ref new Canvas();
        two->Width = 20;
        two->Height = 20;
        two->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        two->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ three = ref new Canvas();
        three->Width = 30;
        three->Height = 30;
        three->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        three->CompositeMode = ElementCompositeMode::SourceOver;

        lteTarget = ref new Canvas();
        lteTarget->CompositeMode = ElementCompositeMode::SourceOver;
        lteTarget->Children->Append(one);
        lteTarget->Children->Append(two);
        lteTarget->Children->Append(three);

        root = ref new Canvas();
        root->Children->Append(lteTarget);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding LTE targeting root of comp node subtree.");

        wh->AddTestLTE(lteTarget, root, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding second LTE targeting root of comp node subtree. Don't crash.");

        UIElement^ lte = wh->AddTestLTE(lteTarget, root, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        lte->Opacity = 0.5;
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void LTETests::GridViewEntranceInFlyoutEntranceWUC()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);

    wh->SetTimeManagerClockOverrideConstant(0);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->SynchronouslyTickUIThread(2);   // Can't wait for idle - we've frozen time.

    FrameworkElement^ flyoutTarget;
    RunOnUIThread([&]() { flyoutTarget = safe_cast<FrameworkElement^>(rootCanvas->FindName(L"gridView")); });

    Flyout^ flyout;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Open a Flyout with a GridView inside.]");

        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 30;
        rectangle->Height = 30;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        auto gridView = ref new GridView();
        gridView->Width = 100;
        gridView->Height = 100;
        gridView->Items->Append(rectangle);

        flyout = ref new Flyout();
        flyout->Content = gridView;
        flyout->ShowAt(flyoutTarget);
    });

    wh->SynchronouslyTickUIThread(1);   // Get the transitions started with the time frozen at 0

    // This test involves a Flyout transition's opacity animation (duration 0.367) racing against a GridView transition's opacity
    // animation (duration 0.667). It repros after the Flyout transition completes but before the GridView transition completes.
    // We tear down the Flyout LTE and rebuild the tree inside, which inadvertently reattaches the GridView's opacity animation,
    // which restarts it.
    //
    // Note: This test (and the flickering bug) depends highly on default timing of theme animations.

    wh->SetTimeManagerClockOverrideConstant(0.4);   // Enough to complete the flyout's transition, but not the GridView's.

    // Wait for the WUC animation to complete and fire completed. Normally we'd use a test hook to force it to complete, but
    // in this case the Storyboard is generated and internal, and isn't available. 30 frames is enough for 400ms.
    wh->SynchronouslyTickUIThread(30);

    // We care about how many times ConnectAnimation() was called on the transition target. If it's called too many times, that
    // means the 0->1 opacity animation was restarted and there was a flicker on screen.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::AnimationComparison::ConnectAnimationCallCount, L"");

    RunOnUIThread([&]() { flyout->Hide(); });
}

void LTETests::OpacityAnimationInEntranceTransitionWUC()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);   // Also removes clock override

    wh->SetTimeManagerClockOverrideConstant(0);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->SynchronouslyTickUIThread(2);   // Can't wait for idle - we've frozen time.

    Storyboard^ sb;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Open a Flyout with a GridView inside.]");

        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 30;
        rectangle->Height = 30;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        auto gridView = safe_cast<GridView^>(rootCanvas->FindName(L"gridView"));
        gridView->Items->Append(rectangle);

        ::Windows::Foundation::TimeSpan span;
        span.Duration = 20000000L;

        auto opacityAnimation = ref new DoubleAnimation();
        opacityAnimation->From = 0.0;
        opacityAnimation->To = 1.0;
        opacityAnimation->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(opacityAnimation, rectangle);
        Storyboard::SetTargetProperty(opacityAnimation, L"Opacity");

        sb = ref new Storyboard();
        sb->Children->Append(opacityAnimation);
        sb->Begin();
    });

    wh->SynchronouslyTickUIThread(1);   // Get the transitions started with the time frozen at 0

    // This test involves a GridView transition's opacity animation (duration 0.667) racing against an explicit opacity animation
    // (duration 2). It repros after the transition completes but before the explicit opacity animation completes. We tear down
    // the transition LTE and rebuild the tree inside, which inadvertently reattaches the opacity animation, which restarts it.

    wh->SetTimeManagerClockOverrideConstant(1);   // Enough to complete the transition, but not the explicit animation.

    // Wait for the WUC animation to complete and fire completed. Normally we'd use a test hook to force it to complete, but
    // in this case the Storyboard is generated and internal, and isn't available. 75 frames is enough for 1s.
    wh->SynchronouslyTickUIThread(75);

    // We care about how many times ConnectAnimation() was called on the transition target. If it's called too many times, that
    // means the 0->1 opacity animation was restarted and there was a flicker on screen.
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::AnimationComparison::ConnectAnimationCallCount, L"");

    RunOnUIThread([&]()
    {
        sb->Stop();
    });
}

void LTETests::ExplicitLTE()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    RunOnUIThread([&]()
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        auto grid = ref new Grid();
        grid->Children->Append(rectangle);

        wh->AddTestLTE(rectangle, grid, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);

        Canvas^ root = ref new Canvas();
        root->Children->Append(grid);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void LTETests::PopupLTE()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ root;

    // Create the root and set it to the window content first to let the test stabilize. Resetting the window content will
    // reset the visual tree, which could delete existing visuals and mess with visual IDs in the MockDComp output.
    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        auto rectangle2 = ref new xaml_shapes::Rectangle();
        rectangle2->Width = 50;
        rectangle2->Height = 50;
        rectangle2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

        auto grid = ref new Grid();
        grid->Children->Append(rectangle);
        grid->Children->Append(rectangle2);

        auto popup = ref new Popup();
        popup->Child = grid;

        root->Children->Append(popup);

        popup->IsOpen = true;
        wh->AddTestLTE(rectangle, nullptr, LTEParentMode::PopupRoot, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

// Regression test: <Display drivers crash with kd break at CoreMessaging!Microsoft__CoreUI__DispatchGroupBatchEnableHandler$CallbackThunk+1ba>
// This bucket of crashes has a crash in HWRedirectedCompTreeNodeWinRT::UpdatePrimaryVisualTransformParent, due to the
// HWRedirectedCompTreeNodeWinRT having a m_redirectionTargetNoRef that's been deleted.
void LTETests::DeviceLost_Culled_LostTargetCompNode()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);

    const auto& wh = TestServices::WindowHelper;

    Border^ border;
    Grid^ grid;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
        // Note: on chk this hits an assert before hitting the crash

        border = ref new Border();
        border->CompositeMode = ElementCompositeMode::SourceOver;
        border->Child = rectangle;

        Border^ border2 = ref new Border();
        border2->Child = border;

        grid = ref new Grid();
        grid->CompositeMode = ElementCompositeMode::SourceOver;
        grid->Children->Append(border2);

        Canvas^ root = ref new Canvas();
        root->Children->Append(grid);
        wh->WindowContent = root;

        lte = wh->AddTestLTE(border2, root, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Setting the LTE to 0 opacity, which should release its comp node when we skip over it during the render walk, unless...
        LOG_OUTPUT(L"> Setting LTE to 0 opacity.");
        lte->Opacity = 0;

        // ...We hit a device lost in the same frame. The we consider the LTE to be "not in the PC scene" (since its property render
        // data is in the "device lost" state rather than the "rendering with WUC" state. This is the bug.
        LOG_OUTPUT(L"> Simulating device lost.");
        wh->SimulateDeviceLost();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Later on (or in the same frame), the LTE's target comp node goes away. Now the LTE's comp node is pointing to released memory.
        LOG_OUTPUT(L"> Removing the LTE's comp node target.");
        grid->CompositeMode = ElementCompositeMode::Inherit;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Later on (or in the same frame), something under the LTE gets dirtied. We rewalk the LTE's comp node to update it and hit the AV.
        LOG_OUTPUT(L"> Changing the comp node tree under the LTE");
        border->CompositeMode = ElementCompositeMode::Inherit;
    });
    LOG_OUTPUT(L"> Don't crash.");
    wh->WaitForIdle();
}

// Regression test: <Reliability: Failure in NULL_CLASS_PTR_READ_c0000005_Microsoft.UI.Xaml.dll!HWCompTreeNode::UpdateTreeVirtual.>
// This bucket of crashes has a crash in HWRedirectedCompTreeNodeWinRT::UpdateTreeVirtual, due to the
// HWRedirectedCompTreeNodeWinRT having a m_pUIElementNoRef that's been deleted.
void LTETests::DeviceLost_Culled_LostLTE()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);

    const auto& wh = TestServices::WindowHelper;

    Grid^ grid;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        Border^ border = ref new Border();
        border->Child = rectangle;

        grid = ref new Grid();
        grid->CompositeMode = ElementCompositeMode::SourceOver;
        grid->Children->Append(border);

        Canvas^ root = ref new Canvas();
        root->Children->Append(grid);
        wh->WindowContent = root;

        lte = wh->AddTestLTE(border, grid, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        // Force a comp node on the LTE, so that it keeps its comp node even after DetachTransition is called
        lte->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Setting the LTE to 0 opacity, which should release its comp node when we skip over it during the render walk, unless...
        LOG_OUTPUT(L"> Setting LTE to 0 opacity.");
        lte->Opacity = 0;

        // ...We hit a device lost in the same frame. The we consider the LTE to be "not in the PC scene" (since its property render
        // data is in the "device lost" state rather than the "rendering with WUC" state. This is the bug.
        LOG_OUTPUT(L"> Simulating device lost.");
        wh->SimulateDeviceLost();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Later on (or in the same frame), the LTE goes away. It doesn't detach its comp node from the tree because
        // it's in a bad state. Now its comp node is pointing to released memory as its UIElement.
        LOG_OUTPUT(L"> Removing the LTE from the tree, and releasing it.");
        wh->RemoveTestLTE(lte);
        lte = nullptr;
    });
    LOG_OUTPUT(L"> Don't crash.");
    wh->WaitForIdle();
}

// Regression test: <Reliability: Crash in NULL_CLASS_PTR_READ_c0000005_Microsoft.UI.Xaml.dll!CDependencyObject::NWPropagateDirtyFlag.>
// This bucket of crashes has a crash in dirty flag propagation, due to an element having a layout transition
// renderer that points to a deleted LTE.
void LTETests::ResetTreeWithOpenLTE()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true, true);

    const auto& wh = TestServices::WindowHelper;

    // Recover from resetting the visual tree at the end
    auto shutdownGuard = wil::scope_exit([wh]
    {
        wh->ShutdownXaml();
        wh->InitializeXaml();
    });

    Grid^ grid;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        // This brush propagates dirtiness up the tree, which eventually walks to the deleted LTE via the parent border.
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        Border^ border = ref new Border();
        border->Child = rectangle;

        grid = ref new Grid();
        grid->CompositeMode = ElementCompositeMode::SourceOver;
        grid->Children->Append(border);

        Canvas^ root = ref new Canvas();
        root->Children->Append(grid);
        wh->WindowContent = root;

        // Give the popup root a child collection, so that it can create a transition root off of that.
        auto popup = ref new Popup();
        popup->Child = ref new xaml_shapes::Rectangle();

        popup->IsOpen = true;
        popup->IsOpen = false;

        lte = wh->AddTestLTE(border, nullptr, LTEParentMode::PopupRoot, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    // Reset the visual tree, which deletes the LTEs without calling DetachTransition. This leaves the border with a
    // layout transition renderer that points to the deleted LTE. Normally tearing down a subtree is not a problem, because
    // the LTE's target is underneath the LTE in Z-order so the target is released first, but when resetting the entire
    // visual tree, the popup root (along with LTEs inside) gets torn down before the LTE targets in the root scroll viewer.
    LOG_OUTPUT(L"> Resetting the visual tree.");
    wh->ResetVisualTree();
    wh->WaitForTreeReset();
    LOG_OUTPUT(L"> Don't crash.");
}

void LTETests::SecondaryLTELeavesDanglingCompNodePointer()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    Canvas^ canvas1;
    Canvas^ unrelatedCanvas;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        //  <RootCanvas>
        //      <Canvas5>
        //          <Canvas4>
        //              <Canvas3 Target>
        //                  <Canvas2 CrashingCanvas>
        //                      <Popup IsOpen="true">
        //                          <Canvas1 HasCompNode />
        //                      </Popup>
        //                  </Canvas2>
        //              </Canvas3>
        //          </Canvas4>
        //          <LTE Secondary Target="Canvas3" />
        //      </Canvas5>
        //      <UnrelatedCanvas />
        //      <LTE Primary Target="Canvas3" />
        //  </RootCanvas>

        canvas1 = ref new Canvas();
        canvas1->Width = 10;
        canvas1->Height = 10;
        canvas1->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        canvas1->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ canvas2 = ref new Canvas();
        canvas2->Width = 20;
        canvas2->Height = 20;
        canvas2->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
        canvas2->Children->Append(canvas1);

        Canvas^ canvas3 = ref new Canvas();
        canvas3->Width = 30;
        canvas3->Height = 30;
        canvas3->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);
        canvas3->Children->Append(canvas2);

        Canvas^ canvas4 = ref new Canvas();
        canvas4->Width = 40;
        canvas4->Height = 40;
        canvas4->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        canvas4->Children->Append(canvas3);

        Canvas^ canvas5 = ref new Canvas();
        canvas5->Width = 50;
        canvas5->Height = 50;
        canvas5->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
        canvas5->Children->Append(canvas4);

        unrelatedCanvas = ref new Canvas();

        Canvas^ root = ref new Canvas();
        root->Children->Append(canvas5);
        root->Children->Append(unrelatedCanvas);
        wh->WindowContent = root;

        wh->AddTestLTE(canvas3, root, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);

        // The secondary LTE is rooted lower in the tree than the primary, so it renders first.
        wh->AddTestLTE(canvas3, canvas5, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing comp node in LTE target.");
        canvas1->CompositeMode = ElementCompositeMode::Inherit;

        // The render walk will run with override storage first. This walk doesn't correctly clean up the UIE tree's
        // "last comp node in subtree" markers, because it's an override LTE walk. The primary LTE walk then crashes
        // during incremental rendering later. Nothing crashes during this render walk because the temporary comp node
        // is kept alive by the command queued in the CompositorTreeHost. At the end of the frame, those commands are
        // flushed, and the temporary comp node is deleted, leaving the UIElement tree with a dangling pointer.
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Touching UIElement tree to trigger another render walk. Don't crash.");
        unrelatedCanvas->Width = 10;

        // All LTEs are dirtied on every frame, so we just need to request a frame. It doesn't matter what actually
        // gets rendered. When we walk the primary LTE this time, we access the dangling pointer in the UIElement tree
        // and crash.
    });
    wh->WaitForIdle();
}

void LTETests::NestedLTELeavesDanglingCompNodePointer()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    Canvas^ canvas1;
    Canvas^ canvas2;
    Canvas^ canvas3;
    Canvas^ canvas4;
    Popup^ popup;
    UIElement^ innerLTE;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        //  <VisualRoot>
        //      <LTE Outer Target="Canvas1" />
        //      <PopupRoot>
        //          <Canvas4>
        //              <Canvas3>
        //                  <Canvas2 HasCompNode>
        //                      <Canvas1 />
        //                  </Canvas2>
        //              </Canvas3>
        //              <LTE Inner Target="Canvas3" />
        //          </Canvas4>
        //      </PopupRoot>
        //      <Popup Child="Canvas4" IsOpen="True" />
        //  </RootCanvas>
        //
        // The outer LTE will pick up Canvas2's comp node as its redirection target.

        canvas1 = ref new Canvas();
        canvas1->Width = 10;
        canvas1->Height = 10;
        canvas1->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

        canvas2 = ref new Canvas();
        canvas2->Width = 20;
        canvas2->Height = 20;
        canvas2->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
        canvas2->CompositeMode = ElementCompositeMode::SourceOver;
        canvas2->Children->Append(canvas1);

        canvas3 = ref new Canvas();
        canvas3->Width = 30;
        canvas3->Height = 30;
        canvas3->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);
        canvas3->Children->Append(canvas2);

        canvas4 = ref new Canvas();
        canvas4->Width = 40;
        canvas4->Height = 40;
        canvas4->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        canvas4->Children->Append(canvas3);

        popup = ref new Popup();
        popup->Child = canvas4;

        Canvas^ root = ref new Canvas();
        root->Children->Append(popup);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup.");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting up LTEs.");
        wh->AddTestLTE(canvas1, nullptr, LTEParentMode::RootVisual, false /* isAbsolutelyPositioned */);

        innerLTE = wh->AddTestLTE(canvas3, canvas4, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting inner LTE to opacity 0. Don't crash.");
        innerLTE->Opacity = 0;

        // The inner LTE will be culled by the render walk. That will cause its target subtree (Canvas3) to leave the
        // PC scene, which releases all comp nodes inside. That causes Canvas2 to release its comp node. However, the
        // outer LTE has already rendered that frame, and has picked up Canvas2's comp node as its m_redirectionTargetNoRef.
        // The outer LTE then hits an AV when it tries to access that dangling pointer.
    });
    wh->WaitForIdle();
}

void LTETests::RasterizationScale()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    // LTEs aren't currently public, so there's no way an app can set RasterizationScale on it.

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        //  <RootCanvas>
        //      <StackPanel RasterizationScale="2">
        //          <Ellipse1 />
        //          <Canvas RasterizationScale="2">
        //              <Ellipse2 RasterizationScale="2" />
        //          </Canvas>
        //          <Ellipse3 RasterizationScale="2" />
        //          <LTE1 Target="Ellipse1" />  <!-- Picks up RS="2" from StackPanel, does not double count it -->
        //          <LTE2 Target="Canvas" />    <!-- Ignores RS on target, multiplies RS on target parent with RS on Ellipse2 -->
        //      </StackPanel>
        //      <LTE3 Target="Ellipse3" />      <!-- Picks up RS on the target's parent -->
        //  </RootCanvas>

        xaml_shapes::Ellipse^ ellipse1 = ref new xaml_shapes::Ellipse();
        ellipse1->Width = 100;
        ellipse1->Height = 100;
        ellipse1->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 26));

        xaml_shapes::Ellipse^ ellipse2 = ref new xaml_shapes::Ellipse();
        ellipse2->Width = 50;
        ellipse2->Height = 50;
        ellipse2->RasterizationScale = 2;
        ellipse2->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 51));

        Canvas^ canvas = ref new Canvas();
        canvas->RasterizationScale = 2;
        canvas->Children->Append(ellipse2);

        xaml_shapes::Ellipse^ ellipse3 = ref new xaml_shapes::Ellipse();
        ellipse3->Width = 100;
        ellipse3->Height = 100;
        ellipse3->RasterizationScale = 2;
        ellipse3->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 77));

        StackPanel^ stackPanel = ref new StackPanel();
        stackPanel->RasterizationScale = 2;
        stackPanel->Children->Append(ellipse1);
        stackPanel->Children->Append(canvas);
        stackPanel->Children->Append(ellipse3);

        Canvas^ root = ref new Canvas();
        root->Children->Append(stackPanel);
        wh->WindowContent = root;

        wh->AddTestLTE(ellipse1, stackPanel, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        wh->AddTestLTE(canvas, stackPanel, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        wh->AddTestLTE(ellipse3, root, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LTETests::BoundsDirtyFlags()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    // Canvases everywhere so layout doens't run to dirty bounds
    Canvas^ target;
    Canvas^ parent;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        //  <Canvas root>
        //      <LTE Target="target" />
        //      <Canvas parent>
        //          <Canvas target />
        //      </Canvas>
        //  </Canvas>

        target = ref new Canvas();
        target->Width = 100;
        target->Height = 100;
        target->Background = ref new SolidColorBrush(mu::Colors::Blue);

        parent = ref new Canvas();
        parent->Children->Append(target);

        root = ref new Canvas();
        root->Children->Append(parent);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    // Parent should include target in its bounds
    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 100, 100));
    });

    RunOnUIThread([&]()
    {
        // Add an absolutely positioned LTE targeting target, which dirties its bounds.
        // Parent's child bounds are also dirtied.
        wh->AddTestLTE(target, root, LTEParentMode::NormalTree, true /* isAbsolutelyPositioned */);

        // Hit test the subtree, which cleans all bounds under it.
        // This gives "parent" empty bounds. When it runs CUIElement::GenerateChildOuterBounds to get bounds for
        // its child, it will skip "target" because it has an LTE, so it's IsHiddenForLayoutTransition. This also
        // makes "parent" cache those empty bounds as its child bounds.
        wf::Point point(50, 50);
        auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, parent);

        // Collapse target. This marks its m_fNWVisibilityDirty flag.
        target->Visibility = Visibility::Collapsed;

        // Also dirty target's inner bounds. This time it doesn't propagate up to parent because target has an LTE.
        // CUIElement::NWPropagateDirtyFlag sees IsHiddenForLayoutTransition and doesn't call CDependencyObject::
        // NWPropagateDirtyFlag.
        target->RenderTransform = ref new TranslateTransform();

        // Note: target never renders directly since it has an LTE. LTEs go from HWWalk::LayoutTransitionElementRenderTarget
        // on the LTE directly to HWWalk::RenderContentAndChildren on its target, skipping HWWalk::Render which
        // checks visibility. Most importantly, this doesn't clear the m_fNWVisibilityDirty flag.
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Remove parent from the tree. This calls CTransition::CancelTransitions via LeaveImpl, which calls
        // RemoveAbsolutelyPositionedLayoutTransitionRenderers, which detaches the transition. The important
        // part here is that RemoveAbsolutelyPositionedLayoutTransitionRenderers calls LTE::DetachTransition
        // with a null target. This means DetachTransition won't get the parent of the target to propagate
        // dirty flags.
        root->Children->RemoveAt(0);

        // Add parent back to the tree.
        root->Children->Append(parent);

        // Make target visible again. This normally goes through CUIElement::NWSetVisibilityDirty to propagate
        // dirty flags up the tree via NWPropagateDirtyFlag. In this case that's all skipped, because the
        // element still has m_fNWVisibilityDirty set and also has inner bounds dirtied. So parent never sets
        // its m_childBoundsDirty flag, and doesn't know to update bounds underneath.
        target->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    // We've added/removed an LTE targeting "target", and collapsed/uncollapsed it. The tree should be back to its
    // starting state. This should report 100x100 bounds, but instead "parent" doesn't have its m_childBoundsDirty
    // flag marked, so it doesn't update bounds from "target". Instead, it uses its cached child bounds, which was
    // back when "target" was still collapsed. Those cached bounds are empty, so "parent" returns empty bounds and
    // fails to hit test.
    //
    // The bug fix is to explicitly propagate dirty flags in RemoveAbsolutelyPositionedLayoutTransitionRenderers.
    // This way "parent" will have its m_childBoundsDirty marked again and will get "target" to update its bounds
    // when it's next hit tested.
    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 100, 100));
    });

    wh->WaitForIdle();
}

} } } } } }
