// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RenderPropertyTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>
#include <WindowsNumerics.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ RenderPropertyTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool RenderPropertyTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool RenderPropertyTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool RenderPropertyTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

// Loads "fileName.xaml", renders it, and compares the results against "fileName-master.txt".
void RenderPropertyTests::RenderTest(Platform::String^ fileName)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    LOG_OUTPUT(L"loading markup");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + fileName + L".xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    wh->SynchronouslyTickUIThread(2);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void RenderPropertyTests::RenderTransformWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderTest(L"RenderTransform");
}

void RenderPropertyTests::Opacity()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderTest(L"Opacity");
}

void RenderPropertyTests::Clip()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderTest(L"Clip");
}

void RenderPropertyTests::Projection()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderTest(L"Projection");
}

void RenderPropertyTests::Transform3D()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderTest(L"Transform3D");
}

void RenderPropertyTests::PopupInternal()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    LOG_OUTPUT(L"> Loading markup.");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"Popup.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    // Explicitly open the outer popups. This guarantees a deterministic order in which the popups are opened.
    // Note: We can't set IsOpen="true" in markup. If we did that, then the XamlReader will open those popups as soon
    // as the markup is loaded, but before the entire subtree gets set as the WindowContent. That effectively opens
    // all popups as parentless popups, which is not what we want. This was made worse when the LoadXamlFileOnUIThread
    // call ran before the StartApplication event was raised, so the entire tree was still marked non-live. We opened
    // a bunch of parentless popups on a non-live tree, and the nested popups inside never got their Enter calls, so
    // they never ran CPopup::StartComposition. When it came time to render, they entered the PC scene too early and
    // hit an assert later when they opened.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening outer popups.");
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p1"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p2"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p3"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p4"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p5"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p6"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p7"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p8"))->IsOpen = true;
        safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"p9"))->IsOpen = true;
    });
    wh->WaitForIdle();

    // Explicitly open the inner popups. This guarantees a deterministic order in which the popups are opened.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening inner popups.");
        xaml::Controls::Primitives::Popup^ innerPopup = safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"innerPopup"));
        if (innerPopup != nullptr)
        {
            innerPopup->IsOpen = true;
        }

        xaml::Controls::Primitives::Popup^ innerPopup2 = safe_cast<xaml::Controls::Primitives::Popup^>(rootCanvas->FindName(L"innerPopup2"));
        if (innerPopup2 != nullptr)
        {
            innerPopup2->IsOpen = true;
        }
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void RenderPropertyTests::PopupWUC()
{
    // Notes:
    // WUCRenderingScopeGuard by default sets Window.Content to a blank Canvas.  This causes the test to trigger a known issue in XAML:
    // -As the parser instantiates Popups in the markup we're loading, it sets the IsOpen property
    // -Popup responds by opening itself and putting its child underneath the PopupRoot, even before going live
    // -Later, when the root of this tree is set as Window.Content, if there is an existing Window.Content, a Reset of the tree occurs.
    // -The process of resetting the public root involves tearing down the current XAML tree, which will contain
    //  the open Popups and bad stuff ensues... basically this is not a supported scenario.
    // To work-around this issue, we disable automatic setting of Window.Content via resetWindowContent = false.  This prevents
    // the resetting of public root and avoids the issue.
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree,
                                true,  // resizeWindow
                                true,   // injectMockDComp
                                true,   // resetDevice
                                false   // resetWindowContent
                                );
    PopupInternal();
}

void RenderPropertyTests::WindowedPopupWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, 1.5f);

    xaml::Controls::Primitives::Popup^ popup = nullptr;

    auto popupOpenedEvent = std::make_shared<Event>();
    auto popupClosedEvent = std::make_shared<Event>();

    auto openedRegistration = CreateSafeEventRegistration(xaml::Controls::Primitives::Popup, Opened);
    auto closedRegistration = CreateSafeEventRegistration(xaml::Controls::Primitives::Popup, Closed);

    RunOnUIThread([&]()
    {
        auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='LayoutRoot' Background='Red' Width='400' Height='400' > \r\n"
            L"</StackPanel>"));

        VERIFY_IS_NOT_NULL(rootPanel);
        TestServices::WindowHelper->WindowContent = rootPanel;

        popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
            L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
            L"  <Rectangle Fill='Green' Width='200' Height='200'/> \r\n"
            L"</Popup>"));
        VERIFY_IS_NOT_NULL(popup);
        TestServices::WindowHelper->Popup_SetWindowed(popup);

        openedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
        {
            LOG_OUTPUT(L"Windowed popup: Open event fired");
            popupOpenedEvent->Set();
        }));

        closedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
        {
            LOG_OUTPUT(L"Windowed popup: Closed event fired");
            popupClosedEvent->Set();
        }));

        LOG_OUTPUT(L"Set popup->IsOpen = true");
        popup->HorizontalOffset = 50;
        popup->VerticalOffset = 20;
        popup->Translation = {100, 100, 0};
        popup->IsOpen = true;
    });

    popupOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(popup->IsOpen);
        LOG_OUTPUT(L"WindowedPopupOpenAndClose: Set popup->IsOpen = false");
        popup->IsOpen = false;
    });

    popupClosedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(popup->IsOpen);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void RenderPropertyTests::DirtyFlags(DCompRendering dcompRendering, bool forceAnimations)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(dcompRendering);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DirtyFlags.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootCanvas;
    });

    Storyboard^ storyboard = nullptr;

    if (forceAnimations)
    {
        RunOnUIThread([&]()
        {
            // We need these animations to hold while the test is running, to force us to build expressions.
            // We're not going to wait for them to complete at the end. Use 60 seconds.
            ::Windows::Foundation::TimeSpan span; span.Duration = 600000000L;

            auto target1 = safe_cast<TranslateTransform^>(rootCanvas->FindName(L"translateTransform"));
            DoubleAnimation^ da1 = ref new DoubleAnimation();
            da1->From = 0.0;
            da1->To = 0.0;
            da1->Duration = DurationHelper::FromTimeSpan(span);
            Storyboard::SetTarget(da1, target1);
            Storyboard::SetTargetProperty(da1, L"X");

            auto target2 = safe_cast<PlaneProjection^>(rootCanvas->FindName(L"planeProjection"));
            DoubleAnimation^ da2 = ref new DoubleAnimation();
            da2->From = 0.0;
            da2->To = 0.0;
            da2->Duration = DurationHelper::FromTimeSpan(span);
            Storyboard::SetTarget(da2, target2);
            Storyboard::SetTargetProperty(da2, L"CenterOfRotationX");

            auto target3 = safe_cast<CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"));
            DoubleAnimation^ da3 = ref new DoubleAnimation();
            da3->From = 0.0;
            da3->To = 0.0;
            da3->Duration = DurationHelper::FromTimeSpan(span);
            Storyboard::SetTarget(da3, target3);
            Storyboard::SetTargetProperty(da3, L"CenterX");

            storyboard = ref new Storyboard();
            storyboard->Children->Append(da1);
            storyboard->Children->Append(da2);
            storyboard->Children->Append(da3);
            storyboard->Begin();
        });
    }

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    {
        LOG_OUTPUT(L"TranslateTransform");

        RunOnUIThread([&]() { safe_cast<xaml_media::TranslateTransform^>(rootCanvas->FindName(L"translateTransform"))->X = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"tt-x");

        RunOnUIThread([&]() { safe_cast<xaml_media::TranslateTransform^>(rootCanvas->FindName(L"translateTransform"))->Y = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"tt-y");
    }

    {
        LOG_OUTPUT(L"ScaleTransform");

        RunOnUIThread([&]() { safe_cast<xaml_media::ScaleTransform^>(rootCanvas->FindName(L"scaleTransform"))->ScaleX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"sct-scaleX");

        RunOnUIThread([&]() { safe_cast<xaml_media::ScaleTransform^>(rootCanvas->FindName(L"scaleTransform"))->ScaleY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"sct-scaleY");

        RunOnUIThread([&]() { safe_cast<xaml_media::ScaleTransform^>(rootCanvas->FindName(L"scaleTransform"))->CenterX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"sct-centerX");

        RunOnUIThread([&]() { safe_cast<xaml_media::ScaleTransform^>(rootCanvas->FindName(L"scaleTransform"))->CenterY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"sct-centerY");
    }

    {
        LOG_OUTPUT(L"RotateTransform");

        RunOnUIThread([&]() { safe_cast<xaml_media::RotateTransform^>(rootCanvas->FindName(L"rotateTransform"))->CenterX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"rt-centerX");

        RunOnUIThread([&]() { safe_cast<xaml_media::RotateTransform^>(rootCanvas->FindName(L"rotateTransform"))->CenterY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"rt-centerY");

        RunOnUIThread([&]() { safe_cast<xaml_media::RotateTransform^>(rootCanvas->FindName(L"rotateTransform"))->Angle = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"rt-angle");
    }

    {
        LOG_OUTPUT(L"SkewTransform");

        RunOnUIThread([&]() { safe_cast<xaml_media::SkewTransform^>(rootCanvas->FindName(L"skewTransform"))->AngleX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"skt-angleX");

        RunOnUIThread([&]() { safe_cast<xaml_media::SkewTransform^>(rootCanvas->FindName(L"skewTransform"))->AngleY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"skt-angleY");

        RunOnUIThread([&]() { safe_cast<xaml_media::SkewTransform^>(rootCanvas->FindName(L"skewTransform"))->CenterX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"skt-centerX");

        RunOnUIThread([&]() { safe_cast<xaml_media::SkewTransform^>(rootCanvas->FindName(L"skewTransform"))->CenterY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"skt-centerY");
    }

    {
        LOG_OUTPUT(L"CompositeTransform");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->TranslateX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-translateX");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->TranslateY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-translateY");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->ScaleX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-scaleX");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->ScaleY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-scaleY");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->Rotation = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-rotation");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->SkewX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-skewX");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->SkewY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-skewY");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->CenterX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-centerX");

        RunOnUIThread([&]() { safe_cast<xaml_media::CompositeTransform^>(rootCanvas->FindName(L"compositeTransform"))->CenterY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct-centerY");
    }

    {
        LOG_OUTPUT(L"MatrixTransform");

        RunOnUIThread([&]() { safe_cast<xaml_media::MatrixTransform^>(rootCanvas->FindName(L"matrixTransform"))->Matrix =
            xaml_media::MatrixHelper::FromElements(2,2,2,2,2,2); });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"mt-matrix");
    }

    {
        LOG_OUTPUT(L"PlaneProjection");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->CenterOfRotationX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-centerX");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->CenterOfRotationY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-centerY");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->CenterOfRotationZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-centerZ");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->RotationX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-rotationX");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->RotationY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-rotationY");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->RotationZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-rotationZ");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->LocalOffsetX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-localX");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->LocalOffsetY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-localY");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->LocalOffsetZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-localZ");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->GlobalOffsetX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-globalX");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->GlobalOffsetY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-globalY");

        RunOnUIThread([&]() { safe_cast<xaml_media::PlaneProjection^>(rootCanvas->FindName(L"planeProjection"))->GlobalOffsetZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pp-globalZ");
    }

    {
        LOG_OUTPUT(L"Matrix3DProjection");

        RunOnUIThread([&]() { safe_cast<xaml_media::Matrix3DProjection^>(rootCanvas->FindName(L"matrixProjection"))->ProjectionMatrix =
            xaml_media::Media3D::Matrix3DHelper::FromElements(2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2); });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"mp-matrix");
    }

    {
        LOG_OUTPUT(L"CompositeTransform3D");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->CenterX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-centerX");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->CenterY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-centerY");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->CenterZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-centerZ");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->ScaleX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-scaleX");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->ScaleY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-scaleY");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->ScaleZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-scaleZ");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->TranslateX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-translateX");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->TranslateY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-translateY");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->TranslateZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-translateZ");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->RotationX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-rotationX");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->RotationY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-rotationY");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::CompositeTransform3D^>(rootCanvas->FindName(L"compositeTransform3D"))->RotationZ = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ct3d-rotationZ");
    }

    {
        LOG_OUTPUT(L"PerspectiveTransform3D");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::PerspectiveTransform3D^>(rootCanvas->FindName(L"perspectiveTransform3D"))->Depth = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pt3d-depth");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::PerspectiveTransform3D^>(rootCanvas->FindName(L"perspectiveTransform3D"))->OffsetX = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pt3d-offsetX");

        RunOnUIThread([&]() { safe_cast<xaml_media::Media3D::PerspectiveTransform3D^>(rootCanvas->FindName(L"perspectiveTransform3D"))->OffsetY = 2; });
        wh->SynchronouslyTickUIThread(1);
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"pt3d-offsetY");
    }

    if (storyboard != nullptr)
    {
        RunOnUIThread([&]() { storyboard->Stop(); });
    }
}

void RenderPropertyTests::DirtyFlagsWUCWithExpressions()
{
    DirtyFlags(DCompRendering::WUCCompleteSynchronousCompTree, true);
}

void RenderPropertyTests::DirtyFlagsCullingWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Grid^ grid;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Set up initial tree]");

        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Width = 20;
        rect->Height = 20;
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        grid = ref new Grid();
        grid->Width = 50;
        grid->Height = 50;
        grid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        grid->Children->Append(rect);

        Canvas^ root = ref new Canvas();
        root->Children->Append(grid);

        wh->WindowContent = root;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Cull out grid, update tree]");

        grid->Opacity = 0;

        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Width = 20;
        rect->Height = 20;
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        grid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        grid->Children->Append(rect);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Culled");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Make grid visible again, rendering should be updated]");

        grid->Opacity = 1;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Unculled");
}

void RenderPropertyTests::BrushColorChange()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"BrushColorChange.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    RunOnUIThread([&]()
    {
        SolidColorBrush^ brush = safe_cast<SolidColorBrush^>(rootCanvas->FindName(L"brush"));
        brush->Color = Microsoft::UI::ColorHelper::FromArgb(0, 0xff, 0xff, 0xff);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"TransparentBrush");
}

void RenderPropertyTests::StrokeDashOffset()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ShapeStroke.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"Initial");

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ rect = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"rect"));
        rect->StrokeDashArray->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"NoDash");
}

void RenderPropertyTests::RenderTransformOrigin()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderTransformOrigin.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

    RunOnUIThread([&]()
    {
        Grid^ g1 = safe_cast<Grid^>(rootCanvas->FindName(L"g1"));
        g1->Width = 400;
    });

    LOG_OUTPUT(L"Parent element resized, transforms should be updated");

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
}

void RenderPropertyTests::RTL()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RTL.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);   // For images
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        Canvas^ c1 = safe_cast<Canvas^>(rootCanvas->FindName(L"changeToRtl1"));
        c1->FlowDirection = FlowDirection::RightToLeft;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

} } } } } }
