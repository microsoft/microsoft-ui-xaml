// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThemeShadowTests.h"
#include "WUCRenderingScopeGuard.h"
#include <TestEvent.h>
#include "SafeEventRegistration.h"
#include "TestCleanupWrapper.h"
#include <TestComparisonGuards.h>


using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Composition;

using namespace MockDComp;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool ThemeShadowTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ThemeShadowTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ThemeShadowTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ThemeShadowTests::ThemeShadowLoadTest()
{
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        xaml_controls::Button^ button = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
            L"<Button Height='100' Width='200' Content='Test Button' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
            L"    <Button.Shadow>"
            L"        <ThemeShadow/>"
            L"    </Button.Shadow>"
            L"</Button>"
        ));

        VERIFY_IS_NOT_NULL(button->Shadow);

        StackPanel^ root = ref new StackPanel();
        root->Children->Append(button);

        wh->WindowContent = root;
    });

    wh->WaitForIdle();
}

Canvas^ CreateBasicCanvas()
{
    auto canvas = ref new Canvas();
    canvas->Width = 400;
    canvas->Height = 400;
    canvas->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);

    return canvas;
}

xaml_shapes::Rectangle^ CreateBasicDropShadowRectangle(Canvas^ rootCanvas)
{    
    xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
    rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
    rect->Width = 100;
    rect->Height = 100;
    rect->Translation = {0, 0, 32};
    rect->Shadow = ref new ThemeShadow();
    Canvas::SetLeft(rect, (rootCanvas->Width - rect->Width) / 2.0);
    Canvas::SetTop(rect, (rootCanvas->Height - rect->Height) / 2.0);
    rootCanvas->Children->Append(rect);

    return rect;
}

xaml_primitives::Popup^ CreateBasicDropShadowPopup()
{
    auto popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
        L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left' HorizontalOffset='100' VerticalOffset='100'> "
        L"  <Popup.Shadow>"
        L"    <ThemeShadow/>"
        L"  </Popup.Shadow>"
        L"  <Rectangle x:Name='popupChild' Width='100' Height='100' Fill='Red'/>"
        L"</Popup>"));

    VERIFY_IS_NOT_NULL(popup);

    return popup;
}

void ThemeShadowTests::ThemeShadowBasicDropShadow()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    Canvas^ rootCanvas;

    RunOnUIThread([&]()
    {
        rootCanvas = CreateBasicCanvas();

        // No Translation.Z
        auto rect = CreateBasicDropShadowRectangle(rootCanvas);
        rect->Translation = {0, 0, 0};

        // Default Translation.Z
        auto rect2 = CreateBasicDropShadowRectangle(rootCanvas);

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedPopup()
{
    ThemeShadowDropShadowWindowedPopupInternal(false, 2.0f);
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedPopupRTL()
{
    ThemeShadowDropShadowWindowedPopupInternal(true, 2.0f);
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedPopup125()
{
    ThemeShadowDropShadowWindowedPopupInternal(false, 1.25f);
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedPopupInternal(bool useRTL, float displayScale)
{
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, displayScale);

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false);

    xaml_controls::Canvas^ rootPanel;
    xaml_primitives::Popup^ popup;
    xaml_shapes::Rectangle^ rectangle;

    RunOnUIThread([&]()
    {
        rootPanel = dynamic_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='400' Height='400' Background='White'> \r\n"
            L"   <Popup x:Name='myPopup' HorizontalOffset='50' VerticalOffset='50'>"
            L"    <Rectangle x:Name='myRectangle' Width='50' Height='50' Fill='Red'/>"
            L"   </Popup>"
            L"</Canvas>"));

        VERIFY_IS_NOT_NULL(rootPanel);

        if (useRTL)
        {
            rootPanel->FlowDirection = FlowDirection::RightToLeft;
        }

        popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"myPopup"));
        VERIFY_IS_NOT_NULL(popup);
        popup->Translation = {0, 0, 32};

        rectangle = dynamic_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"myRectangle"));
        VERIFY_IS_NOT_NULL(rectangle);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->Popup_SetWindowed(popup);
        popup->IsOpen = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        popup->Shadow = ref new ThemeShadow();
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Shadow");

    RunOnUIThread([&]()
    {
        rectangle->Width = 100;
        rectangle->Height = 100;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Resize");

    RunOnUIThread([&]()
    {
        popup->Shadow = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"LostShadow");
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedParentlessPopup()
{
    ThemeShadowDropShadowWindowedParentlessPopupInternal(false);
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedParentlessPopupRTL()
{
    ThemeShadowDropShadowWindowedParentlessPopupInternal(true);
}

void ThemeShadowTests::ThemeShadowDropShadowWindowedParentlessPopupInternal(bool useRTL)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    xaml_primitives::Popup^ popup;
    Canvas^ rootCanvas;

    RunOnUIThread([&]()
    {
        rootCanvas = CreateBasicCanvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup = dynamic_cast<xaml_primitives::Popup^>(xaml_markup::XamlReader::Load(
            L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Left'> "
            L"  <Rectangle x:Name='popupChild' Width='100' Height='100' Fill='Red'/>"
            L"</Popup>"));

        VERIFY_IS_NOT_NULL(popup);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wh->Popup_SetWindowed(popup);
        popup->XamlRoot = rootCanvas->XamlRoot;
        popup->IsOpen = true;
        popup->Shadow = ref new ThemeShadow();
        popup->HorizontalOffset = 200;
        popup->VerticalOffset = 200;
        popup->Translation = {0, 0, 32};

        if (useRTL)
        {
            popup->FlowDirection = FlowDirection::RightToLeft;
        }
    });
    wh->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        popup->IsOpen = false;
    });
    wh->WaitForIdle();
}

void ThemeShadowTests::ThemeShadowDropShadowOpacity()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    Canvas^ rootCanvas;
    Canvas^ compositorCanvas;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        compositorCanvas = CreateBasicCanvas();
        rootCanvas = CreateBasicCanvas();
        rect = CreateBasicDropShadowRectangle(rootCanvas);
        rect->Opacity = 0.5f;
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Storyboard Animation");
        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 0.25;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000L;    // 100 ms
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, rect);
        Storyboard::SetTargetProperty(da, L"Opacity");

        Storyboard^ storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        sb1CompletedRegistration.Attach(storyboard, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        storyboard->Begin();
    });

    sb1CompletedEvent->WaitForDefault();
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"animation1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Facade Animation");
        Visual^ visual = ElementCompositionPreview::GetElementVisual(rect);
        auto compositor = visual->Compositor;

        auto animation = compositor->CreateScalarKeyFrameAnimation();
        animation->InsertKeyFrame(1.0, 0.1f);
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;
        animation->Target = "Opacity";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "animation2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Using HandOff Visual");
        ElementCompositionPreview::GetElementVisual(rect);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "handoff");

}

void ThemeShadowTests::ThemeShadowDropShadowHitTest()
{
    ThemeShadowDropShadowHitTestInternal(false);
}

void ThemeShadowTests::ThemeShadowDropShadowHitTestWindowedPopup()
{
    ThemeShadowDropShadowHitTestInternal(true);
}

void ThemeShadowTests::ThemeShadowDropShadowHitTestInternal(bool useWindowedPopup)
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    Canvas^ rootCanvas;
    xaml_shapes::Rectangle^ rect;
    xaml_primitives::Popup^ popup;
    FrameworkElement^ element;

    RunOnUIThread([&]()
    {
        rootCanvas = CreateBasicCanvas();
        if (useWindowedPopup)
        {
            // For this test we'll use a parentless popup
            popup = CreateBasicDropShadowPopup();
            element = safe_cast<FrameworkElement^>(popup->Child);
        }
        else
        {
            rect = CreateBasicDropShadowRectangle(rootCanvas);
            element = rect;
        }
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    if (useWindowedPopup)
    {
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->Popup_SetWindowed(popup);
            // Since the popup is unparented, set its XamlRoot.
            popup->XamlRoot = rootCanvas->XamlRoot;
            popup->IsOpen = true;
        });
        wh->WaitForIdle();
    }

    auto pointerPressedRegistration1 = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    auto pointerPressedRegistration2 = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();
    std::shared_ptr<Event> pointerPressedEvent2 = std::make_shared<Event>();

    pointerPressedRegistration1.Attach(
        rootCanvas,
        ref new xaml_input::PointerEventHandler(
        [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"PointerPressed event fired on Canvas");
        pointerPressedEvent->Set();
    }));

    pointerPressedRegistration2.Attach(
        element,
        ref new xaml_input::PointerEventHandler(
        [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"PointerPressed event fired on shadow caster");
        pointerPressedEvent2->Set();
    }));

    // Inject a mouse click inside the right edge of the shadow.  This should pass through to the Canvas.

    int offsetX;
    RunOnUIThread([&]()
    {
        offsetX = static_cast<int>(element->Width)/2 + 3;  // offest is from center, takes us 3 pixels to the right of the right edge
    });
    wh->WaitForIdle();

    TestServices::InputHelper->MouseButtonDown(element, offsetX, 0, MouseButton::Left);
    TestServices::InputHelper->MouseButtonUp(element, offsetX, 0, MouseButton::Left);

    LOG_OUTPUT(L"Waiting for pointerPressedEvent");
    pointerPressedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
}

void ThemeShadowTests::ThemeShadowDropShadowTargetOfLTE()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    Canvas^ rootCanvas;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        rootCanvas = CreateBasicCanvas();
        auto rect = CreateBasicDropShadowRectangle(rootCanvas);
        lte = wh->AddTestLTE(rect, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ThemeShadowTests::ThemeShadowBasicPopup()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    Canvas^ canvas1;
    xaml_primitives::Popup^ popup1;
    ThemeShadow^ shadow1;
    Button^ button1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with ThemeShadow - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Popup x:Name='popup1'>"
            L"    <Popup.Shadow>"
            L"      <ThemeShadow x:Name='shadow1'/>"
            L"    </Popup.Shadow>"
            L"    <Button x:Name='button1' Height='100' Width='200' Content='Test Button'/>"
            L"  </Popup>"
            L"</Grid>"
        ));

        canvas1 = safe_cast<Canvas^>(grid1->FindName(L"canvas1"));
        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));
        button1 = safe_cast<Button^>(popup1->Child);

        popup1->Translation =  {0, 0, 32};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NoReceivers");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Adding Receiver to Popup with ThemeShadow - expect custom shadow.");
        shadow1->Receivers->Append(canvas1);
        VERIFY_ARE_EQUAL(1u, shadow1->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddedReceivers");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Removing Receiver from Popup with ThemeShadow - expect global shadow.");
        shadow1->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(0u, shadow1->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"RemovedRecievers");
}

void ThemeShadowTests::ThemeShadowDeepCaster()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    Canvas^ canvas1;
    xaml_primitives::Popup^ popup1;
    ThemeShadow^ shadow1;
    ThemeShadow^ shadow2;

    Border^ border1;
    Button^ button1;
    RectangleGeometry^ clip;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with Caster in subtree (no receivers) - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Popup x:Name='popup1'>"
            L"    <StackPanel x:Name='stackPanel1' Background='#330000FF'>"
            L"      <Border x:Name='border1' Margin='50' Background='Transparent'>"
            L"        <Button x:Name='button1' Margin='7' Height='100' Width='200' Content='Test Button'>"
            L"          <Button.Shadow>"
            L"            <ThemeShadow x:Name='shadow1'/>"
            L"          </Button.Shadow>"
            L"        </Button>"
            L"      </Border>"
            L"    </StackPanel>"
            L"  </Popup>"
            L"</Grid>"
        ));

        canvas1 = safe_cast<Canvas^>(grid1->FindName(L"canvas1"));
        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));
        border1 = safe_cast<Border^>(grid1->FindName(L"border1"));
        button1 = safe_cast<Button^>(border1->Child);

        button1->Translation =  {0, 0, 32};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SingleDeepCaster");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Set Clip on deep caster - should affect shadow.");

        clip = ref new RectangleGeometry();
        clip->Rect = Rect(50, 50, 100, 100);
        button1->Clip = clip;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"ClipOnDeepCaster");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Try add a 2nd deep caster to same Popup - should be ignored.");
        shadow2 = ref new ThemeShadow();
        border1->Shadow = shadow2;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddSecondDeepCaster");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Removing 1st deep caster from Popup - 2nd deep caster should show up.");
        button1->Shadow = nullptr;
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"RemoveFirstDeepCaster");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Add Popup-level caster - should be ignored.");
        popup1->Shadow = shadow1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddPopupLevelCaster");
}

void ThemeShadowTests::ThemeShadowClips()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    Canvas^ canvas1;
    xaml_primitives::Popup^ popup1;
    ThemeShadow^ shadow1;
    Button^ button1;
    Grid^ wrapperGrid;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with ThemeShadow - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Popup x:Name='popup1' Height='100' Width='200'>"
            L"    <Grid x:Name='wrapperGrid' Height='201' Background='#330000FF'>"
            L"      <Button x:Name='button1' Height='100' Width='200' Content='Test Button'>"
            L"        <Button.Shadow>"
            L"          <ThemeShadow x:Name='shadow1'/>"
            L"        </Button.Shadow>"
            L"      </Button>"
            L"    </Grid>"
            L"  </Popup>"
            L"</Grid>"
        ));

        canvas1 = safe_cast<Canvas^>(grid1->FindName(L"canvas1"));
        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        wrapperGrid = safe_cast<Grid^>(popup1->FindName(L"wrapperGrid"));
        button1 = safe_cast<Button^>(wrapperGrid->FindName(L"button1"));
        shadow1 = safe_cast<ThemeShadow^>(button1->FindName(L"shadow1"));

        popup1->Translation =  {0, 0, 32};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    LOG_OUTPUT(L"  > Make caster wider than containing popup -shadow not affected (popups do not clip).");
    RunOnUIThread([&]()
    {
        button1->Width = 402;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"PopupDoesNotClip");

    LOG_OUTPUT(L"  > Make wwrapper Grid narrower than caster and containing popup- LayoutClip should affect shadow.");
    RunOnUIThread([&]()
    {
        wrapperGrid->Width = 151;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"LayoutClip");

    LOG_OUTPUT(L"  > Set UIElement.Clip on Popup.Child- should affect shadow.");
    RunOnUIThread([&]()
    {
        button1->Width = 200;

        RectangleGeometry^ buttonClip = ref new RectangleGeometry();
        buttonClip->Rect = Rect(0, 0, 99, 101);
        button1->Clip = buttonClip;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"ClipOnPopupChild");

    LOG_OUTPUT(L"  > Set UIElement.Clip on Popup- should not affect shadow (popups do not clip).");
    RunOnUIThread([&]()
    {
        RectangleGeometry^ popupClip = ref new RectangleGeometry();
        popupClip->Rect = Rect(0, 0, 78, 148);
        popup1->Clip = popupClip;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"ClipOnPopup");

    TranslateTransform^ translateTransform;

    LOG_OUTPUT(L"  > UIElement.Clip with static Translate Transform - should affect shadow.");
    RunOnUIThread([&]()
    {
        RectangleGeometry^ buttonClip = ref new RectangleGeometry();
        buttonClip->Rect = Rect(0, 0, 99, 101);

        translateTransform= ref new TranslateTransform();
        translateTransform->X = 50;
        buttonClip->Transform= translateTransform;

        wrapperGrid->ClearValue(FrameworkElement::WidthProperty);
        popup1->Clip = nullptr;
        button1->Clip = buttonClip;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"TranslateClip");

    LOG_OUTPUT(L"  > UIElement.Clip with animated Translate Transform - should affect shadow.");
    auto sb1CompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed); auto sb1CompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 100.0;
        da->To = 0.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000000L;    // 1 second
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, translateTransform);
        Storyboard::SetTargetProperty(da, L"X");

        Storyboard^ storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        sb1CompletedRegistration.Attach(storyboard, ref new wf::EventHandler<Object^>([sb1CompletedEvent](Object^ sender, Object^ e) { sb1CompletedEvent->Set(); }));

        storyboard->Begin();
    });

    sb1CompletedEvent->WaitForDefault();
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"TranslateClipAnimation");

    LOG_OUTPUT(L"Clip animation completed.");
}

void ThemeShadowTests::ThemeShadowBasicNonPopup()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    ThemeShadowBasicNonPopupInternal(false);
}

void ThemeShadowTests::ThemeShadowBasicNonPopupLTE()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    ThemeShadowBasicNonPopupInternal(true);
}

void ThemeShadowTests::ThemeShadowBasicNonPopupInternal(bool useLTE)
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    Canvas^ canvas1;
    ThemeShadow^ shadow1;
    Button^ button1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Non-popup (Button) with ThemeShadow without receivers - expect no shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(

            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Button x:Name='button1' Height='100' Width='200' Content='Test Button'>"
            L"    <Button.Shadow>"
            L"      <ThemeShadow x:Name='shadow1'/>"
            L"    </Button.Shadow>"
            L"  </Button>"
            L"</Grid>"
        ));

        canvas1 = safe_cast<Canvas^>(grid1->FindName(L"canvas1"));
        button1 = safe_cast<Button^>(grid1->Children->GetAt(1));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));

        button1->Translation =  {0, 0, 32};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoReceivers");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Adding Receiver to Non-popup (Button) with ThemeShadow - expect custom shadow.");
        shadow1->Receivers->Append(canvas1);
        VERIFY_ARE_EQUAL(1u, shadow1->Receivers->Size);

        if (useLTE)
        {
            wh->AddTestLTE(button1 /* target */, nullptr /* parent */, LTEParentMode::RootVisual, false /* isAbsolutelyPositioned */);
        }
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddedReceivers");

    if (useLTE)
    {
        wh->ClearTestLTEs();
        wh->WaitForIdle();
        u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddedReceiversNoLTE");
    }

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Removing Receiver from Botton with ThemeShadow, custom scene should be deleted");
        shadow1->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(0u, shadow1->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"RemovedRecievers");
}

void ThemeShadowTests::ThemeShadowProjectedShadowBasicPopupAndChild()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    ThemeShadowBasicPopupAndChild();
}

void ThemeShadowTests::ThemeShadowDropShadowBasicPopupAndChild()
{
    ThemeShadowBasicPopupAndChild();
}

void ThemeShadowTests::ThemeShadowBasicPopupAndChild()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    Canvas^ canvas1;
    xaml_primitives::Popup^ popup1;
    ThemeShadow^ shadow1;
    ThemeShadow^ shadow2;
    Button^ button1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup and Popup.Child both with ThemeShadow - expect Child's shadow only.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Popup x:Name='popup1'>"
            L"    <Popup.Shadow>"
            L"      <ThemeShadow x:Name='shadow1'/>"
            L"    </Popup.Shadow>"
            L"    <Button x:Name='button1' Height='100' Width='200' Content='Test Button'/>"
            L"  </Popup>"
            L"</Grid>"
        ));

        canvas1 = safe_cast<Canvas^>(grid1->FindName(L"canvas1"));
        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        button1 = safe_cast<Button^>(popup1->Child);
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));

        shadow2 = ref new ThemeShadow();
        shadow2->Receivers->Append(canvas1);
        button1->Shadow = shadow2;

        VERIFY_ARE_EQUAL(1u, shadow2->Receivers->Size);

        popup1->Translation =  {0, 0, 16};
        button1->Translation =  {0, 0, 32};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

void ThemeShadowTests::ReceiversAPI()
{
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        ThemeShadow^ shadow = ref new ThemeShadow();
        LOG_OUTPUT(L"  > ThemeShadow should have 0 receivers by default.");
        VERIFY_ARE_EQUAL(0u, shadow->Receivers->Size);

        Canvas^ canvas1 = ref new Canvas();
        Canvas^ canvas2 = ref new Canvas();

        LOG_OUTPUT(L"  > Add one element to the Receivers collection.");
        shadow->Receivers->Append(canvas1);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas1, shadow->Receivers->GetAt(0));

        LOG_OUTPUT(L"  > Add second element to the Receivers collection.");
        shadow->Receivers->Append(canvas2);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas1, shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(canvas2, shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Release an element in the weak collection.");
        canvas2 = nullptr;
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas1, shadow->Receivers->GetAt(0));
        VERIFY_IS_NULL(shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Remove an element from the weak collection.");
        shadow->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_IS_NULL(shadow->Receivers->GetAt(0));
    });
    wh->WaitForIdle();
}

void ThemeShadowTests::ReceiversAPI_InvalidArg_AddAncestor()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        //  <Root>
        //      <Canvas3>
        //          <Canvas2>
        //              <Canvas1 Shadow />
        //          </Canvas2>
        //          <Canvas2Sibling />
        //      </Canvas3>
        //  </Root>

        ThemeShadow^ shadow = ref new ThemeShadow();
        VERIFY_ARE_EQUAL(0u, shadow->Receivers->Size);

        Canvas^ canvas1 = ref new Canvas();
        canvas1->Shadow = shadow;

        Canvas^ canvas2 = ref new Canvas();
        canvas2->Children->Append(canvas1);

        Canvas^ canvas2Sibling = ref new Canvas();

        Canvas^ canvas3 = ref new Canvas();
        canvas3->Children->Append(canvas2);
        canvas3->Children->Append(canvas2Sibling);

        Canvas^ root = ref new Canvas();
        root->Children->Append(canvas3);
        wh->WindowContent = root;

        Canvas^ canvas4 = ref new Canvas();

        LOG_OUTPUT(L"  > Add non-live element to the Receivers collection.");
        shadow->Receivers->Append(canvas4);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));

        LOG_OUTPUT(L"  > Add live, non-ancestor element to the Receivers collection.");
        shadow->Receivers->Append(canvas2Sibling);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(canvas2Sibling, shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Add ancestor elements to the Receivers collection. Expect failure.");
        VERIFY_THROWS_WINRT(shadow->Receivers->Append(canvas1), Platform::InvalidArgumentException^);
        VERIFY_THROWS_WINRT(shadow->Receivers->Append(canvas2), Platform::InvalidArgumentException^);
        VERIFY_THROWS_WINRT(shadow->Receivers->Append(canvas3), Platform::InvalidArgumentException^);
        VERIFY_THROWS_WINRT(shadow->Receivers->Append(root), Platform::InvalidArgumentException^);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(canvas2Sibling, shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Release an element in the weak collection.");
        canvas4 = nullptr;
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_IS_NULL(shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(canvas2Sibling, shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Remove an element from the weak collection.");
        shadow->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas2Sibling, shadow->Receivers->GetAt(0));

        LOG_OUTPUT(L"  > Remove the only remaining element from the weak collection.");
        shadow->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(0u, shadow->Receivers->Size);
    });
    wh->WaitForIdle();
}

void ThemeShadowTests::ReceiversAPI_InvalidArg_ParentUnderReceiver()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    ThemeShadow^ shadow;
    Canvas^ root;
    Canvas^ canvas1;
    Canvas^ canvas2;
    Canvas^ canvas3;
    Canvas^ canvas4;

    RunOnUIThread([&]()
    {
        //  <Root>
        //      <Canvas3>
        //          <Canvas2Sibling />
        //      </Canvas3>
        //  </Root>
        //
        //  <Canvas2>
        //      <Canvas1 Shadow />
        //  </Canvas2>

        shadow = ref new ThemeShadow();
        VERIFY_ARE_EQUAL(0u, shadow->Receivers->Size);

        canvas1 = ref new Canvas();
        canvas1->Shadow = shadow;

        canvas2 = ref new Canvas();
        canvas2->Children->Append(canvas1);

        Canvas^ canvas2Sibling = ref new Canvas();

        canvas3 = ref new Canvas();
        canvas3->Children->Append(canvas2Sibling);

        root = ref new Canvas();
        root->Children->Append(canvas3);
        wh->WindowContent = root;

        canvas4 = ref new Canvas();

        LOG_OUTPUT(L"  > Add non-live element to the Receivers collection.");
        shadow->Receivers->Append(canvas4);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));

        LOG_OUTPUT(L"  > Try to add ancestor elements to the Receivers collection. Expect failure.");
        VERIFY_THROWS_WINRT(shadow->Receivers->Append(canvas1), Platform::InvalidArgumentException^);
        VERIFY_THROWS_WINRT(shadow->Receivers->Append(canvas2), Platform::InvalidArgumentException^);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));

        LOG_OUTPUT(L"  > Add live, non-ancestor (yet) element to the Receivers collection.");
        shadow->Receivers->Append(root);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(root, shadow->Receivers->GetAt(1));
    });
    wh->WaitForIdle();

    // We can't do this in the initial frame. The root hasn't been connected yet, so the ThemeShadow doesn't actually
    // enter the tree when it's set on the UIElement. Instead, it enters during VisualTree::SetPublicRootVisual, which
    // responds to E_INVALIDARG by resetting the entire tree rather than throwing an exception. Do this stuff on a
    // subsequent frame so the visual root has actually been set up.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Try to parent subtree under a Receiver element. Expect failure.");
        VERIFY_THROWS_WINRT(root->Children->Append(canvas2), Platform::InvalidArgumentException^);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(root, shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Try to parent subtree under the child of a Receiver element. Expect failure.");
        VERIFY_THROWS_WINRT(canvas3->Children->Append(canvas2), Platform::InvalidArgumentException^);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));
        VERIFY_ARE_EQUAL(root, shadow->Receivers->GetAt(1));

        LOG_OUTPUT(L"  > Remove live, non-ancestor (yet) element from the Receivers collection.");
        shadow->Receivers->RemoveAt(1);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));

        LOG_OUTPUT(L"  > Parent subtree.");
        canvas3->Children->Append(canvas2);
        VERIFY_ARE_EQUAL(1u, shadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas4, shadow->Receivers->GetAt(0));
    });
    wh->WaitForIdle();
}

void ThemeShadowTests::ReceiversAPI_InvalidArg_SharedShadow()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    ThemeShadow^ sharedShadow;
    Canvas^ canvas1;
    Canvas^ canvas2;

    RunOnUIThread([&]()
    {
        //  <Root>
        //      <Canvas5>
        //          <Canvas4 SharedShadow />
        //          <Canvas3 SharedShadow />
        //          <Canvas2>
        //              <Canvas1 SharedShadow />
        //          </Canvas>
        //      </Canvas3>
        //  </Root>

        sharedShadow = ref new ThemeShadow();
        VERIFY_ARE_EQUAL(0u, sharedShadow->Receivers->Size);

        canvas1 = ref new Canvas();
        canvas1->Shadow = sharedShadow;

        canvas2 = ref new Canvas();
        canvas2->Children->Append(canvas1);

        Canvas^ canvas3 = ref new Canvas();
        canvas3->Shadow = sharedShadow;

        Canvas^ canvas4 = ref new Canvas();
        canvas4->Shadow = sharedShadow;

        Canvas^ canvas5 = ref new Canvas();
        canvas5->Children->Append(canvas4);
        canvas5->Children->Append(canvas3);
        canvas5->Children->Append(canvas2);

        Canvas^ root = ref new Canvas();
        root->Children->Append(canvas5);
        wh->WindowContent = root;

        LOG_OUTPUT(L"  > Try to add ancestor elements to sharedShadow.Receivers collection. Expect failure.");
        VERIFY_THROWS_WINRT(sharedShadow->Receivers->Append(canvas1), Platform::InvalidArgumentException^);
        VERIFY_THROWS_WINRT(sharedShadow->Receivers->Append(canvas2), Platform::InvalidArgumentException^);
        VERIFY_THROWS_WINRT(sharedShadow->Receivers->Append(canvas5), Platform::InvalidArgumentException^);
        VERIFY_ARE_EQUAL(0u, sharedShadow->Receivers->Size);

        LOG_OUTPUT(L"  > Remove shared shadow from canvas1.");
        canvas1->Shadow = nullptr;

        LOG_OUTPUT(L"  > Add canvas2 to sharedShadow.Receivers collection.");
        sharedShadow->Receivers->Append(canvas2);
        VERIFY_ARE_EQUAL(1u, sharedShadow->Receivers->Size);
        VERIFY_ARE_EQUAL(canvas2, sharedShadow->Receivers->GetAt(0));
    });
    wh->WaitForIdle();

    // We can't do this in the initial frame. The root hasn't been connected yet, so the ThemeShadow doesn't actually
    // enter the tree when it's set on the UIElement. Instead, it enters during VisualTree::SetPublicRootVisual, which
    // responds to E_INVALIDARG by resetting the entire tree rather than throwing an exception. Do this stuff on a
    // subsequent frame so the visual root has actually been set up.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Try to add sharedShadow back to canvas1. Expect failure.");
        VERIFY_THROWS_WINRT(canvas1->Shadow = sharedShadow, Platform::InvalidArgumentException^);
        VERIFY_IS_NULL(canvas1->Shadow);

        LOG_OUTPUT(L"  > Remove canvas1 from the tree.");
        canvas2->Children->RemoveAt(0);

        LOG_OUTPUT(L"  > Add sharedShadow back to canvas1.");
        canvas1->Shadow = sharedShadow;
        VERIFY_ARE_EQUAL(sharedShadow, canvas1->Shadow);

        LOG_OUTPUT(L"  > Try to parent subtree under a Receiver element. Expect failure.");
        VERIFY_THROWS_WINRT(canvas2->Children->Append(canvas1), Platform::InvalidArgumentException^);
    });
    wh->WaitForIdle();
}

void ThemeShadowTests::ReceiversAPI_SharedReceivers()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    ThemeShadow^ shadow;
    ThemeShadow^ shadow2;
    Canvas^ sharedReceiver;
    Canvas^ deletedSharedReceiver;
    Canvas^ canvas3;

    RunOnUIThread([&]()
    {
        //  <Root>
        //      <Canvas3>
        //          <Canvas2 Shadow />
        //          <Canvas1 Shadow />
        //          <SharedReceiver />
        //          <DeletedSharedReceiver />
        //      </Canvas3>
        //  </Root>

        LOG_OUTPUT(L"  > Creating tree.");

        shadow = ref new ThemeShadow();
        Canvas^ canvas1 = ref new Canvas();
        canvas1->Shadow = shadow;

        shadow2 = ref new ThemeShadow();
        Canvas^ canvas2 = ref new Canvas();
        canvas2->Shadow = shadow2;

        sharedReceiver = ref new Canvas();
        deletedSharedReceiver = ref new Canvas();

        canvas3 = ref new Canvas();
        canvas3->Children->Append(canvas2);
        canvas3->Children->Append(canvas1);
        canvas3->Children->Append(sharedReceiver);
        canvas3->Children->Append(deletedSharedReceiver);

        Canvas^ root = ref new Canvas();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(canvas3);
        wh->WindowContent = root;

        // Give the root a comp node to work around an assert in ThemeShadowGlobalScene::UpdateReceivers.
        root->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Setting receivers on shadow.");
        shadow->Receivers->Append(sharedReceiver);
        shadow->Receivers->Append(deletedSharedReceiver);
        VERIFY_ARE_EQUAL(2u, shadow->Receivers->Size);

        LOG_OUTPUT(L"  > Setting receivers on shadow2.");
        shadow2->Receivers->Append(sharedReceiver);
        shadow2->Receivers->Append(deletedSharedReceiver);
        VERIFY_ARE_EQUAL(2u, shadow2->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Removing receivers from shadow. They're still used by shadow2.");
        shadow->Receivers->RemoveAt(1);
        shadow->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(0u, shadow->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Deleting deletedSharedReceiver. It's still (weakly) referenced by shadow2.");
        canvas3->Children->RemoveAt(3);
        deletedSharedReceiver = nullptr;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Removing receivers from shadow2, including the deleted deletedSharedReceiver.");
        shadow2->Receivers->RemoveAt(1);    // This UIElement has already been deleted
        shadow2->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(0u, shadow2->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoReceivers");
}

void ThemeShadowTests::ThemeShadowFallback()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ForceShadowsPolicy(true);
    });

    Grid^ grid1;
    Canvas^ canvas1;
    xaml_primitives::Popup^ popup1;
    ThemeShadow^ shadow0;
    ThemeShadow^ shadow1;
    xaml_shapes::Rectangle^ rectangle1;
    Button^ button1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with ThemeShadow - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Rectangle x:Name='rectangle1' Height='10' Width='20' Fill='Red'>"
            L"    <Rectangle.Shadow>"
            L"      <ThemeShadow x:Name='shadow0'/>"
            L"    </Rectangle.Shadow>"
            L"  </Rectangle>"
            L"  <Popup x:Name='popup1'>"
            L"    <Popup.Shadow>"
            L"      <ThemeShadow x:Name='shadow1'/>"
            L"    </Popup.Shadow>"
            L"    <Button x:Name='button1' Height='100' Width='200' Content='Test Button'/>"
            L"  </Popup>"
            L"</Grid>"
        ));

        canvas1 = safe_cast<Canvas^>(grid1->FindName(L"canvas1"));
        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        shadow0 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow0"));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));
        rectangle1 = safe_cast<xaml_shapes::Rectangle^>(grid1->FindName(L"rectangle1"));
        button1 = safe_cast<Button^>(popup1->Child);

        popup1->Translation = {0, 0, 32};
        rectangle1->Translation = {0, 0, 64};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"  > Adding Receivers to Shadow0. Now Shadow0 has custom receivers, while shadow1 uses default scene.");
    RunOnUIThread([&]()
    {
        shadow0->Receivers->Append(canvas1);
        VERIFY_ARE_EQUAL(1u, shadow0->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    LOG_OUTPUT(L"  > Disabling shadow. All scenes should be cleared.");
    wh->ForceShadowsPolicy(false);
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShadowsDisabled");

    LOG_OUTPUT(L"  > Re-enabling shadow. All shadows from Initial state should be restored.");
    wh->ForceShadowsPolicy(true);
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShadowsRestored");

    LOG_OUTPUT(L"  > Apply several back-to-back disable and enable calls, and make sure the scene is fully restored");
    RunOnUIThread([&]()
    {
        wh->ForceShadowsPolicy(true);
        wh->ForceShadowsPolicy(false);
        wh->ForceShadowsPolicy(true);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShadowsRestored");

    LOG_OUTPUT(L"  > Disabling shadow again, and removing custom receiver while shadow is disabled.");
    RunOnUIThread([&]()
    {
        wh->ForceShadowsPolicy(false);
        shadow0->Receivers->RemoveAt(0);
        VERIFY_ARE_EQUAL(0u, shadow0->Receivers->Size);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShadowsDisabledAgain");

    LOG_OUTPUT(L"  > Re-enabling shadow again. Only the popup-based shadow from Initial state should be restored.");
    wh->ForceShadowsPolicy(true);
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShadowsRestoredAgain");
}

void ThemeShadowTests::EnsureRootCanvasCompNodeBasic()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Canvas^ root;
    ThemeShadow^ shadow;
    xaml_primitives::Popup^ popup;
    Button^ button;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 488;
        root->Height = 388;
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::LightBlue);

        shadow = ref new ThemeShadow();

        popup = ref new xaml_primitives::Popup();
        popup->Shadow = shadow;

        button = ref new Button();
        button->Width = 100;
        button->Height = 100;
        button->Content = "Button";
        button->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        popup->Child = button;

        root->Children->Append(popup);
        popup->Translation = { 0, 0, 32 };

        LOG_OUTPUT(L"Elements created");

        wh->WindowContent = root;
        LOG_OUTPUT(L"Window Content set");
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup->IsOpen = true;
        LOG_OUTPUT(L"Popup opened");
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"");
}

void ThemeShadowTests::EnsureRootCanvasCompNodeLateAdd()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Canvas^ root;
    ThemeShadow^ shadow;
    xaml_primitives::Popup^ popup1;
    xaml_primitives::Popup^ popup2;
    Button^ button1;
    Button^ button2;

    RunOnUIThread([&]()
    {
        //First we only put root canvas into the
        //tree, no compnode at this time
        root = ref new Canvas();
        root->Width = 488;
        root->Height = 388;
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::LightBlue);
        LOG_OUTPUT(L"Root canvas created");

        wh->WindowContent = root;
        LOG_OUTPUT(L"Window Content set");
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoCompNode");

    RunOnUIThread([&]()
    {
        shadow = ref new ThemeShadow();
        popup1 = ref new xaml_primitives::Popup();
        popup1->Shadow = shadow;
        button1 = ref new Button();
        button1->Width = 100;
        button1->Height = 50;
        button1->Content = "Button";
        button1->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        popup1->Child = button1;

        popup2 = ref new xaml_primitives::Popup();
        button2 = ref new Button();
        button2->Width = 100;
        button2->Height = 50;
        button2->Content = "Button";
        button2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        popup2->Child = button2;

        root->Children->Append(popup1);
        root->Children->Append(popup2);
        popup1->Translation = { 0, 0, 32 };
        popup2->Translation = { 0, 0, 32 };
        LOG_OUTPUT(L"Elements created");
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
        popup2->IsOpen = true;
        LOG_OUTPUT(L"Popup opened");
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddCanvasCompNode");

    RunOnUIThread([&]()
    {
        //Share the shadow to the second popup, SetValue will be called and
        //EnsureRootCanvasCompNode should be executed
        popup2->Shadow = shadow;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"SharedShadow");
}

void ThemeShadowTests::EnsureRootCanvasCompNodeParentLessPopup()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Canvas^ root;
    ThemeShadow^ shadow1;
    ThemeShadow^ shadow2;
    xaml_primitives::Popup^ popup1;
    xaml_primitives::Popup^ popup2;
    Button^ button1;
    Button^ button2;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 488;
        root->Height = 388;
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::LightBlue);
        LOG_OUTPUT(L"Root canvas created");

        //parentless popups
        popup1 = ref new xaml_primitives::Popup();
        button1 = ref new Button();
        button1->Width = 100;
        button1->Height = 50;
        button1->Content = "Button";
        button1->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        popup1->Child = button1;
        popup1->Translation = { 0, 0, 32 };

        popup2 = ref new xaml_primitives::Popup();
        button2 = ref new Button();
        button2->Width = 100;
        button2->Height = 50;
        button2->Content = "Button";
        button2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        popup2->Child = button2;
        popup2->Translation = { 0, 0, 32 };
        shadow2 = ref new ThemeShadow();
        popup2->Shadow = shadow2;

        wh->WindowContent = root;
        LOG_OUTPUT(L"Window Content set");
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
        popup2->IsOpen = true;
        LOG_OUTPUT(L"Popups opened");
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        //set shadow here to test the SetValue() method
        shadow1 = ref new ThemeShadow();
        popup1->Shadow = shadow1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"");
}

void ThemeShadowTests::ThemeShadowCasterFiltering_TwoCasters()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    Canvas^ canvas1;
    xaml_primitives::Popup^ popup1;
    xaml_primitives::Popup^ popup2;
    ThemeShadow^ shadow1;
    ThemeShadow^ shadow2;
    Button^ button1;
    Button^ button2;
    Border^ border1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with Caster in subtree (no receivers) - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Popup x:Name='popup1'>"
            L"    <StackPanel x:Name='stackPanel1' Background='#330000FF'>"
            L"      <Border x:Name='border1' Margin='50' Background='Transparent'>"
            L"        <Button x:Name='button1' Height='101' Width='201' Content='Button1'>"
            L"          <Button.Shadow>"
            L"            <ThemeShadow x:Name='shadow1'/>"
            L"          </Button.Shadow>"
            L"        </Button>"
            L"      </Border>"
            L"    </StackPanel>"
            L"  </Popup>"
            L"  <Popup x:Name='popup2'>"
            L"    <Button x:Name='button2' Height='102' Width='202' Content='Button2'>"
            L"      <Button.Shadow>"
            L"        <ThemeShadow x:Name='shadow2'/>"
            L"        </Button.Shadow>"
            L"      </Button>"
            L"  </Popup>"
            L"</Grid>"
        ));

        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        popup2 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup2"));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));
        shadow2 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow2"));
        border1 = safe_cast<Border^>(grid1->FindName(L"border1"));
        button1 = safe_cast<Button^>(border1->Child);
        button2 = safe_cast<Button^>(popup2->Child);

        button1->Translation = {0, 0, 32};
        button2->Translation = {20, 20, 32};

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
        popup2->IsOpen = true;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"  > Button1/2 @Z=32 -> no filtering in Xaml (but coplanar shadow visually dropped by COMP).");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EqualDepth");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Elevate Button1 to 64 -> Button1 filtered");
        button1->Translation = {0, 0, 64};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"ElevateButton1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Lower button1 back to 32 -> Button1 still filtered (filtering is persistent)");
        button1->Translation = {0, 0, 32};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"EqualDepth_Again");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Remove (filtered) Button1 -> Button2 continues casting");
        border1->Child = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Removed_Caster1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Unset ThemeShadow on Button2  -> no active casters.");
        button2->Shadow = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Removed_ThemeShadow_Caster2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Add back ThemeShadow for Button2 -> Button2 is casting");
        button2->Shadow = shadow2;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddBack_ThemeShadow_Caster2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Add back Button1 @Z=32 -> Button1 has new compnode, casts again");
        border1->Child = button1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"AddBack_Caster1_EqualDepth");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Elevate Button1 to 64 again -> Button1 filtered");
        button1->Translation = {0, 0, 64};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"ElevateButton1_Again");
}

void ThemeShadowTests::ThemeShadowCasterFiltering_ThreeCasters()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    xaml_primitives::Popup^ popup1;
    xaml_primitives::Popup^ popup2;
    xaml_primitives::Popup^ popup3;
    ThemeShadow^ shadow1;
    ThemeShadow^ shadow2;
    ThemeShadow^ shadow3;
    Button^ button1;
    Button^ button2;
    Button^ button3;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with Caster in subtree (no receivers) - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Canvas x:Name='canvas1' Background='LightBlue' Height='121' Width='121'/>"
            L"  <Popup x:Name='popup1'>"
            L"    <StackPanel x:Name='stackPanel1' Background='#330000FF'>"
            L"      <Border x:Name='border1' Margin='50' Background='Transparent'>"
            L"        <Button x:Name='button1' Height='101' Width='201' Content='Button1'>"
            L"          <Button.Shadow>"
            L"            <ThemeShadow x:Name='shadow1'/>"
            L"          </Button.Shadow>"
            L"        </Button>"
            L"      </Border>"
            L"    </StackPanel>"
            L"  </Popup>"
            L"  <Popup x:Name='popup2'>"
            L"    <Button x:Name='button2' Height='102' Width='202' Content='Button2'>"
            L"      <Button.Shadow>"
            L"        <ThemeShadow x:Name='shadow2'/>"
            L"        </Button.Shadow>"
            L"      </Button>"
            L"  </Popup>"
            L"  <Popup x:Name='popup3'>"
            L"    <Button x:Name='button3' Height='103' Width='203' Content='Button3'>"
            L"      <Button.Shadow>"
            L"        <ThemeShadow x:Name='shadow3'/>"
            L"        </Button.Shadow>"
            L"      </Button>"
            L"  </Popup>"
            L"</Grid>"
        ));

        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        popup2 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup2"));
        popup3 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup3"));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));
        shadow2 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow2"));
        shadow3 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow3"));
        button1 = safe_cast<Button^>(grid1->FindName(L"button1"));
        button2 = safe_cast<Button^>(grid1->FindName(L"button2"));
        button3 = safe_cast<Button^>(grid1->FindName(L"button3"));

        button1->Translation = { 0, 0, 50 };
        button2->Translation = { 20, 20, 50 };
        button3->Translation = { 40, 40, 50 };

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
        popup2->IsOpen = true;
        popup3->IsOpen = true;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"  > Button1/2/3 @Z=50A. -> All 3 buttons should have shadow.");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EqualDepth");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Button1 @Z=100, Button2 @Z=50, Button3 @Z=75 -> Button1 filtered");
        button1->Translation = { 0, 0, 100 };
        button2->Translation = { 20, 20, 50 };
        button3->Translation = { 40, 40, 75 };
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Z_100_50_75");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Button1 @Z=100, Button2 @Z=75, Button3 @Z=50 -> Button1 and Button2 filtered");
        button1->Translation = { 0, 0, 100 };
        button2->Translation = { 20, 20, 75 };
        button3->Translation = { 40, 40, 50 };
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Z_100_75_50");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Button1 @Z=100, Button2 @Z=150, Button3 @Z=200 -> Button1 and Button2 (filtered is persistent)");
        button1->Translation = { 0, 0, 100 };
        button2->Translation = { 20, 20, 150 };
        button3->Translation = { 40, 40, 200 };
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Z_100_150_200");
}

void ThemeShadowTests::ThemeShadowCasterFiltering_NestedPopups()
{
    RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    TestCleanupWrapper cleanup;

    Grid^ grid1;
    xaml_primitives::Popup^ popup1;
    xaml_primitives::Popup^ popup2;
    xaml_primitives::Popup^ popup3;
    ThemeShadow^ shadow1;
    ThemeShadow^ shadow2;
    ThemeShadow^ shadow3;
    Grid^ caster1;
    Button^ caster2;
    Button^ caster3;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Popup with Caster in subtree (no receivers) - expect global shadow.");

        grid1 = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='grid1' Height='300' Width='300' Background='LightGreen'>"
            L"  <Popup x:Name='popup1'>"
            L"    <Grid x:Name='caster1' Height='101' Width='201'>"
            L"      <Grid.Shadow>"
            L"        <ThemeShadow x:Name='shadow1'/>"
            L"      </Grid.Shadow>"
            L"      <Popup x:Name='popup2'>"
            L"        <Button x:Name='caster2' Height='102' Width='202' Content='Button2'>"
            L"          <Button.Shadow>"
            L"            <ThemeShadow x:Name='shadow2'/>"
            L"          </Button.Shadow>"
            L"        </Button>"
            L"      </Popup>"
            L"    </Grid>"
            L"  </Popup>"
            L"  <Popup x:Name='popup3'>"
            L"    <Button x:Name='caster3' Height='103' Width='203' Content='Button3'>"
            L"      <Button.Shadow>"
            L"        <ThemeShadow x:Name='shadow3'/>"
            L"      </Button.Shadow>"
            L"    </Button>"
            L"  </Popup>"
            L"</Grid>"
        ));

        popup1 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup1"));
        popup2 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup2"));
        popup3 = safe_cast<xaml_primitives::Popup^>(grid1->FindName(L"popup3"));
        shadow1 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow1"));
        shadow2 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow2"));
        shadow3 = safe_cast<ThemeShadow^>(grid1->FindName(L"shadow3"));
        caster1 = safe_cast<Grid^>(grid1->FindName(L"caster1"));
        caster2 = safe_cast<Button^>(grid1->FindName(L"caster2"));
        caster3 = safe_cast<Button^>(grid1->FindName(L"caster3"));

        caster1->Translation = { 0, 0, 50 };
        caster2->Translation = { 20, 20, 50 };
        caster3->Translation = { 40, 40, 75 };

        wh->WindowContent = grid1;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // There is some strange timing issue that occurs when creating the scene with the popup open
        // and this can lead to an extra tick being handled that will cause masters to mismatch (due
        // to extra visuals getting destructed and recreated).  So open the popup after the scene
        // has settled.
        popup1->IsOpen = true;
        popup2->IsOpen = true;
        popup3->IsOpen = true;
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"  > Popup2 @Z=50 nested in Popup1 @Z=50, w/ Popup3 @Z=75. Popup2's caster should be filtered since Popup2 is affectively at Z=100.");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Popup2_Filtered");
}

void ThemeShadowTests::ThemeShadowDropShadowLoadTest()
{
    ThemeShadowLoadTest();
}

void ThemeShadowTests::ThemeShadowDropShadowBasicPopup()
{
    ThemeShadowBasicPopup();
}

void ThemeShadowTests::ThemeShadowDropShadowVerifyShadowPixels()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_controls::Canvas^ rootPanel;
    xaml_controls::Canvas^ rtbCanvas;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Canvas");
        rootPanel = safe_cast<Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='myCanvas1' Height='300' Width='300' Background='Green'>"
            L"  <Rectangle x:Name='rectangle' Height='100' Width='100' Canvas.Top='50' Fill='Red'/>"
            L"</Canvas>"
        ));

        LOG_OUTPUT(L"Creating RTB Canvas");
        rtbCanvas = safe_cast<Canvas^>(xaml_markup::XamlReader::Load(
            L"  <Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rtbCanvas' Height='100' Width='100' Background='White' Canvas.Top='150'>"
            L"      <Rectangle x:Name='rectForRTB' Height='50' Width='50' Fill='Black' Canvas.Top='25' Canvas.Left='25'>"
            L"          <Rectangle.Shadow>"
            L"              <ThemeShadow/>"
            L"          </Rectangle.Shadow>"
            L"      </Rectangle>"
            L"  </Canvas>"
        ));
        
        wh->WindowContent = rtbCanvas;
    });
    wh->WaitForIdle();

    xaml_media::Imaging::RenderTargetBitmap^ rtb;
    auto renderedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync started.");
        rtb = ref new xaml_media::Imaging::RenderTargetBitmap();

        concurrency::create_task(rtb->RenderAsync(rtbCanvas)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });
    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ snapshot = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle"));
        auto imageBrush = ref new xaml_media::ImageBrush();
        imageBrush->ImageSource = rtb;
        snapshot->Fill = imageBrush;

        wh->WindowContent = rootPanel;
    });
    wh->WaitForIdle();

    u->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    u->ResetMockDCompSurfaceId();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void ThemeShadowTests::ThemeShadowDropShadowDynamicCornerRadius()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ rootCanvas;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        rootCanvas = CreateBasicCanvas();
        rect = CreateBasicDropShadowRectangle(rootCanvas);
        rect->RadiusX = 4.0f;
        rect->RadiusY = 4.0f;
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4_CR");

    RunOnUIThread([&]()
    {
        rect->RadiusX = 0.0f;
        rect->RadiusY = 0.0f;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "No_CR");

    RunOnUIThread([&]()
    {
        rect->RadiusX = 4.0f;
        rect->RadiusY = 8.0f;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Uneven_CR");
}

void ThemeShadowTests::ThemeShadowDropShadowDynamicCornerRadiusOnParent()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ rootCanvas;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Canvas");
        rootCanvas = safe_cast<Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='myCanvas1' Height='300' Width='300' Background='White'>"
            L"  <Grid x:Name='myBorder' Height='100' Width='100' CornerRadius='10,10,10,10'>"
            L"    <Rectangle x:Name='myRectangle' Height='100' Width='100' Fill='Black'>"
            L"      <Rectangle.Shadow>"
            L"        <ThemeShadow/>"
            L"      </Rectangle.Shadow>"
            L"    </Rectangle>"
            L"  </Grid>"
            L"</Canvas>"
        ));
        auto myRectangle = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"myRectangle"));
        myRectangle->Translation = {0, 0, 32};

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "10_CR");
}

void ThemeShadowTests::ThemeShadowDropShadowRoundedCornersTargetOfLTE()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    Canvas^ rootCanvas;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        rootCanvas = CreateBasicCanvas();
        auto rect = CreateBasicDropShadowRectangle(rootCanvas);
        rect->RadiusX = 4.0f;
        rect->RadiusY = 4.0f;
        lte = wh->AddTestLTE(rect, rootCanvas, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ThemeShadowTests::ThemeShadowDropShadowUseCachedBrush()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    Canvas^ rootCanvas;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Canvas");
        rootCanvas = safe_cast<Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='myCanvas1' Height='300' Width='300' Background='White'>"
            L"    <Rectangle x:Name='myRectangle1' Height='30' Width='30' Fill='Black' RadiusX='4' RadiusY='4'>"
            L"      <Rectangle.Shadow>"
            L"        <ThemeShadow/>"
            L"      </Rectangle.Shadow>"
            L"    </Rectangle>"
            L"    <Rectangle x:Name='myRectangle2' Height='20' Width='20' Fill='Black' RadiusX='4' RadiusY='4'>"
            L"      <Rectangle.Shadow>"
            L"        <ThemeShadow/>"
            L"      </Rectangle.Shadow>"
            L"    </Rectangle>"
            L"    <Rectangle x:Name='myRectangle3' Height='30' Width='30' Fill='Black' RadiusX='2' RadiusY='4'>"
            L"      <Rectangle.Shadow>"
            L"        <ThemeShadow/>"
            L"      </Rectangle.Shadow>"
            L"    </Rectangle>"
            L"    <Rectangle x:Name='myRectangle4' Height='30' Width='30' Fill='Black' RadiusX='4' RadiusY='2'>"
            L"      <Rectangle.Shadow>"
            L"        <ThemeShadow/>"
            L"      </Rectangle.Shadow>"
            L"    </Rectangle>"
            L"    <Rectangle x:Name='myRectangle5' Height='10' Width='10' Fill='Black' RadiusX='3' RadiusY='3'>"
            L"      <Rectangle.Shadow>"
            L"        <ThemeShadow/>"
            L"      </Rectangle.Shadow>"
            L"    </Rectangle>"
            L"</Canvas>"
        ));
        auto myRectangle1 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"myRectangle1"));
        myRectangle1->Translation = {0, 0, 32};
        auto myRectangle2 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"myRectangle2"));
        myRectangle2->Translation = {0, 0, 32};
        auto myRectangle3 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"myRectangle3"));
        myRectangle3->Translation = {0, 0, 32};
        auto myRectangle4 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"myRectangle4"));
        myRectangle4->Translation = {0, 0, 32};
        auto myRectangle5 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"myRectangle5"));
        myRectangle5->Translation = {0, 0, 32};

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ThemeShadowTests::ThemeShadowDropShadowSystemThemeRedrawRTB()
{
    // The RenderTargetBitmap shadow output differs slightly between OS builds (no JPEG here),
    // so allow the small per-channel tolerance.
    ImageCompareToleranceGuard tolerance(RENDER_COMPARE_TOLERANCE_SMALL);
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_controls::Canvas^ rootPanel;
    xaml_controls::Canvas^ rtbCanvas;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Canvas");
        rootPanel = safe_cast<Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='myCanvas1' Height='300' Width='300' Background='Green'>"
            L"  <Rectangle x:Name='rectangle' Height='100' Width='100' Canvas.Top='50' Fill='Red'/>"
            L"  <Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rtbCanvas' Height='100' Width='100' Background='White' Canvas.Top='150'>"
            L"      <Rectangle x:Name='rectForRTB' Height='50' Width='50' Fill='Black' Canvas.Top='25' Canvas.Left='25' RadiusX='4' RadiusY='4'>"
            L"          <Rectangle.Shadow>"
            L"              <ThemeShadow/>"
            L"          </Rectangle.Shadow>"
            L"      </Rectangle>"
            L"  </Canvas>"
            L"</Canvas>"
        ));
        auto rectForRTB = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectForRTB"));
        rectForRTB->Translation = {0, 0, 32};

        wh->WindowContent = rootPanel;
        rtbCanvas = safe_cast<Canvas^>(rootPanel->FindName(L"rtbCanvas"));
    });
    wh->WaitForIdle();

    xaml_media::Imaging::RenderTargetBitmap^ rtb;
    auto renderedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rootPanel->RequestedTheme = ElementTheme::Light;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync started.");
        rtb = ref new xaml_media::Imaging::RenderTargetBitmap();

        concurrency::create_task(rtb->RenderAsync(rtbCanvas)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });
    renderedEvent->WaitForDefault();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ snapshot = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle"));
        auto imageBrush = ref new xaml_media::ImageBrush();
        imageBrush->ImageSource = rtb;
        snapshot->Fill = imageBrush;
    });
    wh->WaitForIdle();

    u->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    u->ResetMockDCompSurfaceId();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "Light");

    RunOnUIThread([&]()
    {
        rootPanel->RequestedTheme = ElementTheme::Dark;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync started.");
        rtb = ref new xaml_media::Imaging::RenderTargetBitmap();

        concurrency::create_task(rtb->RenderAsync(rtbCanvas)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });
    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ snapshot = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle"));
        auto imageBrush = ref new xaml_media::ImageBrush();
        imageBrush->ImageSource = rtb;
        snapshot->Fill = imageBrush;
    });
    wh->WaitForIdle();

    u->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    u->ResetMockDCompSurfaceId();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "Dark");
}

void ThemeShadowTests::ThemeShadowDropShadowCornerRoundingOnControl()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;
    xaml_controls::Canvas^ canvas;
    xaml_controls::Button^ button;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Button with corner radius set");
        canvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='mygrid' Height='300' Width='300' Background='White'>"
            L"  <Button Height='50' Width='50' Name='mybutton' Content='Test Button' Canvas.Top='100' Canvas.Left='100' Background='Black'>"
            L"    <Button.Shadow>"
            L"        <ThemeShadow/>"
            L"    </Button.Shadow>"
            L"  </Button>"
            L"</Canvas>"
        ));
        VERIFY_IS_NOT_NULL(canvas);
        button = safe_cast<xaml_controls::Button^>(canvas->FindName(L"mybutton"));
        VERIFY_IS_NOT_NULL(button);
        button->Translation = {0, 0, 32};
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "WithoutCR");

    RunOnUIThread([&]()
    {
        button->CornerRadius = { 4, 4, 4, 4 };
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "WithCR");
}

} } } } } }
