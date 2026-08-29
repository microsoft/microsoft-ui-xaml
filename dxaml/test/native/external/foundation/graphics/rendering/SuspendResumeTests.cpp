// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SuspendResumeTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>
#include <SafeEventRegistration.h>
#include <DisableRenderingScopeGuard.h>
#include "Microsoft.UI.Xaml.CompositionTarget-private.h"

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics;

Platform::String^ SuspendResumeTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
}

bool SuspendResumeTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SuspendResumeTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SuspendResumeTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SuspendResumeTests::SuspendResume()
{
    // Don't disconnect root, not triggered by timer, offer resources
    TestSuspendResume(false, false, true);
}

void SuspendResumeTests::SuspendResume_NoOffer()
{
    // Don't disconnect root, is triggered by timer, don't offer resources
    TestSuspendResume(false, true, false);
}

void SuspendResumeTests::SuspendResume_DisconnectRoot()
{
    // Disconnect root, not triggered by timer, offer resources
    TestSuspendResume(true, false, true);
}

void SuspendResumeTests::TestSuspendResume(bool forceDisconnectRoot, bool isTriggeredByResourceTimer, bool allowOfferResources)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    TestCleanupWrapper cleanup([wh]()
    {
        wh->ForceDisconnectRootOnSuspend(false);
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_shapes::Rectangle^ rectangle;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = canvas;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        canvas->Children->Append(rectangle);
    });

    LOG_OUTPUT(L"Live tree");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Active");

    LOG_OUTPUT(L"Suspend");
    wh->ForceDisconnectRootOnSuspend(forceDisconnectRoot);
    wh->TriggerSuspend(isTriggeredByResourceTimer, allowOfferResources);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Suspended");
    u->VerifyAreSurfaceResourcesOffered(allowOfferResources);

    LOG_OUTPUT(L"Update the tree - no rendering change because we're suspended");
    RunOnUIThread([&]()
    {
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Suspended");

    LOG_OUTPUT(L"Resume - tree changes are rendered");
    wh->TriggerResume();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Resumed");
}

void SuspendResumeTests::ZoomScaleChangeWhileInvisibleWUC()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true /* resizeWindow */, false /* we don't need MockDComp */);

    auto canvasLayoutUpdatedRegistration = CreateSafeEventRegistration(Canvas, LayoutUpdated);
    int layoutUpdatedCount = 0;

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

        Canvas^ canvas = ref new Canvas();
        canvas->Children->Append(rectangle);
        canvasLayoutUpdatedRegistration.Attach(canvas, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^)
        {
            layoutUpdatedCount++;
        }));
        TestServices::WindowHelper->WindowContent = canvas;
    });

    LOG_OUTPUT(L"[Visible - layout updated once]");
    wh->WaitForIdle();
    VERIFY_ARE_EQUAL(1, layoutUpdatedCount);

    LOG_OUTPUT(L"[Disable rendering]");
    DisableRenderingScopeGuard disableRendering;

    LOG_OUTPUT(L"[Update zoom scale]");
    wh->SetWindowSizeOverrideWithScale(wf::Size(400, 300), 1.5f);

    LOG_OUTPUT(L"[Pump some more frames]");
    wh->SynchronouslyTickUIThread(5);

    LOG_OUTPUT(L"[We should have reran layout exactly once after the zoom scale update.]");
    VERIFY_ARE_EQUAL(2, layoutUpdatedCount);
}

void SuspendResumeTests::SuspendResume_PrivateAPIWUCFull()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_shapes::Rectangle^ rectangle;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = canvas;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        canvas->Children->Append(rectangle);
    });

    LOG_OUTPUT(L"Live tree");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Active");

    LOG_OUTPUT(L"Private suspend API");
    RunOnUIThread([&]()
    {
        ICompositionTargetStatics^ compositionTargetStatics;
        VERIFY_SUCCEEDED(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Media.CompositionTarget").Get(),
            reinterpret_cast<IInspectable**>(&compositionTargetStatics)));

        wrl::ComPtr<ICompositionTargetPrivate> compositionTargetPrivate;
        reinterpret_cast<IUnknown*>(compositionTargetStatics)->QueryInterface(IID_PPV_ARGS(&compositionTargetPrivate));

        compositionTargetPrivate->SuspendRendering(true);
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Suspended");
    u->VerifyAreSurfaceResourcesOffered(false); // The private API doesn't offer anything

    LOG_OUTPUT(L"Update the tree - no rendering change because we're suspended");
    RunOnUIThread([&]()
    {
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Suspended");

    LOG_OUTPUT(L"Resume - tree changes are rendered");
    RunOnUIThread([&]()
    {
        ICompositionTargetStatics^ compositionTargetStatics;
        VERIFY_SUCCEEDED(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Media.CompositionTarget").Get(),
            reinterpret_cast<IInspectable**>(&compositionTargetStatics)));

        wrl::ComPtr<ICompositionTargetPrivate> compositionTargetPrivate;
        reinterpret_cast<IUnknown*>(compositionTargetStatics)->QueryInterface(IID_PPV_ARGS(&compositionTargetPrivate));

        compositionTargetPrivate->SuspendRendering(false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Resumed");
}

void SuspendResumeTests::MakeLISOnSuspend()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_shapes::Rectangle^ rectangle;
    std::shared_ptr<Event> loadCompletedEvent = std::make_shared<Event>();
    SafeEventRegistrationType(LoadedImageSurface, LoadCompleted) loadCompletedRegistration = CreateSafeEventRegistration(LoadedImageSurface, LoadCompleted);

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = canvas;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        canvas->Children->Append(rectangle);
    });

    wh->WaitForIdle();

    LOG_OUTPUT(L"Suspend");
    wh->TriggerSuspend(true /* isTriggeredByResourceTimer */, true /* allowOfferResources */);

    LOG_OUTPUT(L"Create LIS and put it to visual tree");

    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");

    RunOnUIThread([&]()
    {
        auto lis = LoadedImageSurface::StartLoadFromStream(bitmapStream);

        loadCompletedRegistration.Attach(lis,
            ref new wf::TypedEventHandler<LoadedImageSurface^, LoadedImageSourceLoadCompletedEventArgs^>(
                [&](Platform::Object ^sender, LoadedImageSourceLoadCompletedEventArgs^ args)
        {
            LOG_OUTPUT(L"Received LoadCompleted event");
            loadCompletedEvent->Set();
        }));

        auto rootVisual = Hosting::ElementCompositionPreview::GetElementVisual(rectangle);
        auto spriteVisual = rootVisual->Compositor->CreateSpriteVisual();
        auto o = spriteVisual->Offset;
        o.x = 100;
        o.y = 100;
        spriteVisual->Offset = o;
        auto s = spriteVisual->Size;
        s.x = 200;
        s.y = 200;
        spriteVisual->Size = s;
        spriteVisual->Brush = rootVisual->Compositor->CreateSurfaceBrush(lis);
        Hosting::ElementCompositionPreview::SetElementChildVisual(Window::Current->Content, spriteVisual);
    });
    loadCompletedEvent->WaitForDefault();

    LOG_OUTPUT(L"Resume - tree changes are rendered");
    wh->TriggerResume();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

