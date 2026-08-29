// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PopupTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>
#include <AnimationTestHelper.h>
#include <WindowsNumerics.h>
#include <collection.h>
#include "VisualDebugTags.h"
#include <WindowAutoCloser.h>
#include <SafeEventRegistration.h>
#include <FocusTestHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Shapes;

using namespace test_infra;
using namespace MockDComp;
using namespace Platform;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ PopupTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
}

bool PopupTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PopupTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool PopupTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

Popup^ PopupTests::MakeRTLPopupInLTRTree()
{
    const auto& wh = TestServices::WindowHelper;

    // Grid1[compnode]-Grid2-Canvas3[RTL]-Popup[RTL]-Rectangle[RTL]
    Grid^ grid1;
    Grid^ grid2;
    Canvas^ canvas3;
    Popup^ popup;
    xaml_shapes::Rectangle^ rectangle;

    RunOnUIThread([&]()
    {
        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->FlowDirection = FlowDirection::RightToLeft;
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        popup = ref new Popup();
        popup->FlowDirection = FlowDirection::RightToLeft;
        popup->Width = 100;
        popup->Height = 100;
        popup->Child = rectangle;
        Canvas::SetLeft(popup, 50.0);
        Canvas::SetTop(popup, 50.0);

        canvas3 = ref new Canvas();
        canvas3->FlowDirection = FlowDirection::RightToLeft;
        canvas3->Width = 200;
        canvas3->Height = 200;
        canvas3->Children->Append(popup);

        grid2 = ref new Grid();
        grid2->FlowDirection = FlowDirection::LeftToRight;
        grid2->Width = 200;
        grid2->Height = 200;
        grid2->Children->Append(canvas3);

        grid1 = ref new Grid();
        grid1->FlowDirection = FlowDirection::LeftToRight;
        grid1->Width = 200;
        grid1->Height = 200;
        grid1->CompositeMode = ElementCompositeMode::SourceOver;
        grid1->Children->Append(grid2);

        wh->WindowContent = grid1;
    });

    // Let the tree render before opening the popup. Otherwise, the popup could be incorrectly detected as a parentless popup.
    wh->WaitForIdle();

    return popup;
}

void PopupTests::WindowedPopupHasForcedBackground()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ popup;
    Popup^ windowedPopup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Opening popups");

        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        xaml_shapes::Rectangle^ rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 10;
        rectangle->Height = 10;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        Canvas^ canvas = ref new Canvas();
        canvas->Width = 300;
        canvas->Height = 300;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Transparent);
        canvas->Children->Append(rectangle);

        popup = ref new Popup();
        popup->Child = canvas;
        popup->IsOpen = true;

        xaml_shapes::Rectangle^ rectangle2 = ref new xaml_shapes::Rectangle();
        rectangle2->Width = 12;
        rectangle2->Height = 10;
        rectangle2->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        Canvas^ canvas2 = ref new Canvas();
        canvas2->Width = 302;
        canvas2->Height = 300;
        canvas2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Transparent);
        canvas2->Children->Append(rectangle2);

        windowedPopup = ref new Popup();
        wh->Popup_SetWindowed(windowedPopup);
        windowedPopup->Child = canvas2;
        windowedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Device lost");
        wh->ResetDeviceAndVisuals();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > 2.0 zoom scale");
        wh->SetWindowSizeOverrideWithScale(wf::Size(400, 300), 2.0f);
        wh->ResetDeviceAndVisuals();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Device lost");
        wh->ResetDeviceAndVisuals();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void PopupTests::ParentedPopup_ValidateRequestedThemePropagation()
{
    const auto& wh = TestServices::WindowHelper;

    Popup^ popup = MakeRTLPopupInLTRTree();

    RunOnUIThread([&]()
    {
        auto parent = safe_cast<FrameworkElement^>(popup->Parent);
        parent->RequestedTheme = xaml::ElementTheme::Light;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup->IsOpen = true;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto child = safe_cast<FrameworkElement^>(popup->Child);
        VERIFY_IS_TRUE(child->ActualTheme == xaml::ElementTheme::Light);
    });
}

void PopupTests::ParentedPopup_RTLSubtreeInLTRTreeWUC()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ popup = MakeRTLPopupInLTRTree();

    RunOnUIThread([&]()
    {
        popup->IsOpen = true;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void PopupTests::ParentedPopup_RTLSubtreeInLTRTreeWUC_AnimatedOffset()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ popup = MakeRTLPopupInLTRTree();
    Storyboard^ sb;

    RunOnUIThread([&]()
    {
        popup->IsOpen = true;

        DoubleAnimation^ da = AnimationTestHelper::MakeDoubleAnimation(popup, L"(Canvas.Left)");
        da->From = 75.0;
        da->To = 75.0;

        sb = ref new Storyboard();
        sb->Children->Append(da);
        sb->Begin();
    });

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        sb->Stop();
    });
    wh->WaitForIdle();
}

void PopupTests::ParentedPopup_RTLSubtreeInLTRTreeWUC_HandoffVisual()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ popup = MakeRTLPopupInLTRTree();
    Microsoft::UI::Composition::Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(popup));
        ::Windows::Foundation::Numerics::float3 newOffset = { 12, 34, 0 };
        handOffVisual->Offset = newOffset;
    });

    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

Popup^ PopupTests::MakePopup(UIElement^ child)
{
    Canvas^ canvas = ref new Canvas();
    canvas->Width = 10;
    canvas->Height = 10;
    if (child)
    {
        canvas->Children->Append(child);
    }

    Popup^ popup;
    popup = ref new Popup();
    popup->Child = canvas;
    return popup;
}

void PopupTests::ResetNestedPopups()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    Popup^ r1;  // Rooted
    Popup^ r1_1;
    Popup^ r1_1_1;
    Popup^ r1_2;
    Popup^ u2;  // Unrooted
    Popup^ u3;
    Popup^ u3_1;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popup tree.");

        r1_1_1 = MakePopup(nullptr);
        r1_1 = MakePopup(r1_1_1);
        r1_2 = MakePopup(nullptr);
        Canvas^ canvas = ref new Canvas();
        canvas->Children->Append(r1_1);
        canvas->Children->Append(r1_2);
        r1 = MakePopup(canvas);

        u2 = MakePopup(nullptr);

        u3_1 = MakePopup(nullptr);
        u3 = MakePopup(u3_1);

        root = ref new Canvas();
        root->Children->Append(r1);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                u2->XamlRoot = xamlRoot;
                u3->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Open nested popups in a messy order. Verify that we don't crash when resetting the visual tree.");
        LOG_OUTPUT(L"> Rooted popups - open parent, then child, then grandparent, then sibling.");
        LOG_OUTPUT(L"> Unrooted popups - open child, then parent.");
        r1_1->IsOpen = true;
        r1_1_1->IsOpen = true;
        u2->IsOpen = true;
        r1->IsOpen = true;
        u3_1->IsOpen = true;
        u3->IsOpen = true;
        r1_2->IsOpen = true;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Ending test, which closes all nested popups and all rooted popups via TestCleanupWrapper. Don't crash.");
}

void PopupTests::WindowedPopupsMoveWithAncestorChain()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ popupAncestor;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating windowed popup tree.");

        Canvas^ popupContent = ref new Canvas();
        popupContent->Width = 100;
        popupContent->Height = 100;
        popupContent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        Popup^ grandchildPopup = MakePopup(popupContent);
        grandchildPopup->ShouldConstrainToRootBounds = false;
        grandchildPopup->VerticalOffset = 50;
        Canvas::SetLeft(grandchildPopup, 50);

        Canvas^ childCanvas = ref new Canvas();
        childCanvas->CompositeMode = ElementCompositeMode::SourceOver;
        childCanvas->Children->Append(grandchildPopup);
        Canvas::SetLeft(childCanvas, 100);
        Canvas::SetTop(childCanvas, 100);

        Canvas^ popupContent2 = ref new Canvas();
        popupContent2->Width = 100;
        popupContent2->Height = 100;
        popupContent2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        Popup^ childPopup = MakePopup(popupContent2);
        childPopup->ShouldConstrainToRootBounds = false;
        childPopup->VerticalOffset = 100;
        Canvas::SetLeft(childPopup, 100);

        popupAncestor = ref new Canvas();
        popupAncestor->Children->Append(childPopup);
        popupAncestor->Children->Append(childCanvas);

        Canvas^ root = ref new Canvas();
        root->Children->Append(popupAncestor);
        wh->WindowContent = root;

        childPopup->IsOpen = true;
        grandchildPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Initial");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Moving windowed popup ancestor. The windowed popup is expected to move to follow the ancestor.");
        Canvas::SetLeft(popupAncestor, 100);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ParentMoved");
}

void PopupTests::RasterizationScale()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ belowRasterizationScale;
    Popup^ hasRasterizationScale;   // Note: Popup's own RasterizationScale is ignored, just like how a ScaleTransform on a Popup will still leave blurry content
    Popup^ aboveRasterizationScale;

    Popup^ hasRasterizationScale_parentless;
    Popup^ aboveRasterizationScale_parentless;

    Canvas^ root;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popup tree.");

        {
            xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
            ellipse->Width = 100;
            ellipse->Height = 100;
            ellipse->RasterizationScale = 2;
            ellipse->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 26));

            aboveRasterizationScale = MakePopup(ellipse);
        }

        {
            xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
            ellipse->Width = 100;
            ellipse->Height = 100;
            ellipse->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 51));

            hasRasterizationScale = MakePopup(ellipse);
            hasRasterizationScale->RasterizationScale = 2;
        }

        {
            xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
            ellipse->Width = 100;
            ellipse->Height = 100;
            ellipse->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 77));

            belowRasterizationScale = MakePopup(ellipse);
        }
        Canvas^ rasterizationScaleCanvas = ref new Canvas();
        rasterizationScaleCanvas->RasterizationScale = 2;
        rasterizationScaleCanvas->Children->Append(belowRasterizationScale);

        root = ref new Canvas();
        root->Children->Append(aboveRasterizationScale);
        root->Children->Append(hasRasterizationScale);
        root->Children->Append(rasterizationScaleCanvas);

        {
            xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
            ellipse->Width = 100;
            ellipse->Height = 100;
            ellipse->RasterizationScale = 2;
            ellipse->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 102));

            aboveRasterizationScale_parentless = MakePopup(ellipse);
        }

        {
            xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
            ellipse->Width = 100;
            ellipse->Height = 100;
            ellipse->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 128));

            hasRasterizationScale_parentless = MakePopup(ellipse);
            hasRasterizationScale_parentless->RasterizationScale = 2;
        }

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                aboveRasterizationScale_parentless->XamlRoot = xamlRoot;
                hasRasterizationScale_parentless->XamlRoot = xamlRoot;
            }
        }

        aboveRasterizationScale->IsOpen = true;
        hasRasterizationScale->IsOpen = true;
        belowRasterizationScale->IsOpen = true;

        aboveRasterizationScale_parentless->IsOpen = true;
        hasRasterizationScale_parentless->IsOpen = true;
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void PopupTests::InlinePopupInWindowedPopup()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ windowedPopup;
    xaml_shapes::Rectangle^ windowedPopupContent;
    Popup^ inlineRedPopup;
    Popup^ inlineBluePopup;

    Canvas^ root;
    Canvas^ windowedPopupCanvas;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popup tree.");

        xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 100;
        ellipse->Height = 100;
        ellipse->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));
        inlineBluePopup = MakePopup(ellipse);

        xaml_shapes::Ellipse^ ellipse2 = ref new xaml_shapes::Ellipse();
        ellipse2->Width = 100;
        ellipse2->Height = 100;
        ellipse2->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        inlineRedPopup = MakePopup(ellipse2);

        windowedPopupContent = ref new xaml_shapes::Rectangle();
        windowedPopupContent->Width = 50;
        windowedPopupContent->Height = 50;
        windowedPopupContent->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 0));

        windowedPopup = MakePopup(nullptr);
        windowedPopup->HorizontalOffset = 100;
        windowedPopup->VerticalOffset = 100;
        windowedPopup->ShouldConstrainToRootBounds = false;

        windowedPopupCanvas = safe_cast<Canvas^>(windowedPopup->Child);
        windowedPopupCanvas->Children->Append(inlineBluePopup);
        windowedPopupCanvas->Children->Append(windowedPopupContent);
        windowedPopupCanvas->Children->Append(inlineRedPopup);

        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Opening inline popups inside windowed popup. The one opened second should be on top (blue covers red).");
    RunOnUIThread([&]()
    {
        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                windowedPopup->XamlRoot = xamlRoot;
            }
        }

        windowedPopup->IsOpen = true;

        // The second child opens first.
        inlineRedPopup->IsOpen = true;

        // The first child opens second. Note that this covers inlinePopup2 (i.e. blue covers red) because it opened later. Their positions in the tree has no effect.
        inlineBluePopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested1");

    LOG_OUTPUT(L"> Close inner inline popup and reopen.");
    RunOnUIThread([&]()
    {
        inlineBluePopup->IsOpen = false;
    });
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        inlineBluePopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested1");

    LOG_OUTPUT(L"> Close outer windowed popup and reopen. The inner inline popup closed when its outer windowed popup closed and will not reopen automatically.");
    RunOnUIThread([&]()
    {
        windowedPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        windowedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested2");

    LOG_OUTPUT(L"> Reopen inner inline popup.");
    RunOnUIThread([&]()
    {
        inlineBluePopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested3");

    LOG_OUTPUT(L"> Close outer windowed popup, reparent inner inline popup to not be nested, and reopen.");
    RunOnUIThread([&]()
    {
        windowedPopup->IsOpen = false;
        inlineBluePopup->IsOpen = false;

        windowedPopupCanvas = safe_cast<Canvas^>(windowedPopup->Child);
        windowedPopupCanvas->Children->Clear();
        root->Children->Append(inlineBluePopup);

        inlineBluePopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NotNested");

    LOG_OUTPUT(L"> Close inline popup, reparent to windowed popup, and reopen.");
    RunOnUIThread([&]()
    {
        inlineBluePopup->IsOpen = false;

        root->Children->RemoveAtEnd();
        windowedPopupCanvas->Children->Append(inlineBluePopup);
        windowedPopupCanvas->Children->Append(windowedPopupContent);
        windowedPopupCanvas->Children->Append(inlineRedPopup);

        windowedPopup->IsOpen = true;
        inlineBluePopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested4");

    LOG_OUTPUT(L"> Reopen second inline popup. It should cover the first inline popup which opened earlier (red covers blue).");
    RunOnUIThread([&]()
    {
        inlineRedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested5");

    LOG_OUTPUT(L"> Close outer windowed popup. Reopen an inner inline popup while the outer windowed popup is still closed.");
    // The inner popup will still render in this case, as an inline popup. It'll behave like a parentless popup and will
    // not inherit any transform above the Popup itself.
    RunOnUIThread([&]()
    {
        windowedPopup->IsOpen = false;  // Implicitly closes nested popups
        inlineRedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested6");

    LOG_OUTPUT(L"> Open the outer windowed popup while the inner non-windowed popup is open.");
    // Xaml will not attempt to reparent the nested popup inside its windowed parent. The nested popup will continue to
    // live in the main tree.
    RunOnUIThread([&]()
    {
        windowedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Nested7");
}

void PopupTests::InlinePopupTabOrder()
{
    PopupTabOrder(false);
}

void PopupTests::WindowedPopupTabOrder()
{
    PopupTabOrder(true);
}

void PopupTests::PopupTabOrder(bool isWindowed)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& kh = TestServices::KeyboardHelper;

    TestCleanupWrapper cleanup;

    StackPanel^ stackPanel = nullptr;
    Button^ button1 = nullptr;
    Popup^ popup;
    Button^ button2 = nullptr;
    Button^ button3 = nullptr;

    RunOnUIThread([&]()
    {
        stackPanel = ref new StackPanel();
        stackPanel->HorizontalAlignment = HorizontalAlignment::Left;
        wh->WindowContent = stackPanel;

        button1 = ref new Button();
        button1->Width = 200;
        button1->Height = 50;
        button1->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        stackPanel->Children->Append(button1);

        popup = ref new Popup();
        popup->Width = 200;
        popup->Height = 50;
        if (isWindowed)
        {
            popup->ShouldConstrainToRootBounds = false;
        }
        stackPanel->Children->Append(popup);

        button2 = ref new Button();
        button2->Width = 200;
        button2->Height = 50;
        button2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        button2->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Black);
        button2->Content = isWindowed ? "Windowed" : "Inline";
        popup->Child = button2;
        popup->IsOpen = true;

        button3 = ref new Button();
        button3->Width = 200;
        button3->Height = 50;
        button3->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        stackPanel->Children->Append(button3);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        button2->Focus(xaml::FocusState::Programmatic);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Initial focus should be on button2.");
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button2));
    });

    LOG_OUTPUT(L"> Pressing Tab should move focus to button1.");
    kh->Tab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button1));
    });

    LOG_OUTPUT(L"> Pressing Tab again should move focus to button3.");
    kh->Tab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button3));
    });

    LOG_OUTPUT(L"> Pressing Tab should move focus to button1. This is a problem - button2 can never be focused again.");
    kh->Tab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button1));
    });

    LOG_OUTPUT(L"> Pressing Tab again should move focus to button3.");
    kh->Tab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button3));
    });

    LOG_OUTPUT(L"> Pressing Shift+Tab should move focus back to button1. This is a problem - button2 can never be focused again.");
    kh->ShiftTab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button1));
    });

    LOG_OUTPUT(L"> Pressing Shift+Tab once more should move focus back to button3.");
    kh->ShiftTab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button3));
    });

    LOG_OUTPUT(L"> Pressing Shift+Tab should move focus back to button1.");
    kh->ShiftTab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button1));
    });

    LOG_OUTPUT(L"> Pressing Shift+Tab once more should move focus back to button3.");
    kh->ShiftTab();
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(wh->WindowContent->XamlRoot)->Equals(button3));
    });
}

void PopupTests::SystemBackdropRoundedCorners()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    Popup^ popupBorder0;
    Popup^ popupBorder3;
    Popup^ popupBorderUneven;
    Popup^ popupSquareBorder0;
    Popup^ popupSquareBorder3;
    Popup^ popupSquareBorderUneven;
    Grid^ changingGrid;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popups.");
        DesktopAcrylicBackdrop^ backdrop = ref new DesktopAcrylicBackdrop();

        {
            Border^ border0 = ref new Border();
            border0->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(8);

            Grid^ grid = ref new Grid();
            grid->Width = 150;
            grid->Height = 150;
            grid->Children->Append(border0);

            popupBorder0 = ref new Popup();
            popupBorder0->ShouldConstrainToRootBounds = false;
            popupBorder0->SystemBackdrop = backdrop;
            popupBorder0->Child = grid;
        }

        {
            Border^ border3 = ref new Border();
            border3->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(8);
            border3->BorderThickness = xaml::Thickness({ 3, 3, 3, 3 });
            border3->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            changingGrid = ref new Grid();
            changingGrid->Width = 150;
            changingGrid->Height = 150;
            changingGrid->Children->Append(border3);

            popupBorder3 = ref new Popup();
            popupBorder3->ShouldConstrainToRootBounds = false;
            popupBorder3->SystemBackdrop = backdrop;
            popupBorder3->Child = changingGrid;
        }

        {
            Border^ borderUneven = ref new Border();
            borderUneven->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(8);
            borderUneven->BorderThickness = xaml::Thickness({ 3, 3, 3, 2 });
            borderUneven->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            Grid^ grid = ref new Grid();
            grid->Width = 150;
            grid->Height = 150;
            grid->Children->Append(borderUneven);

            popupBorderUneven = ref new Popup();
            popupBorderUneven->ShouldConstrainToRootBounds = false;
            popupBorderUneven->SystemBackdrop = backdrop;
            popupBorderUneven->Child = grid;
        }

        {
            Border^ squareBorder0 = ref new Border();

            Grid^ grid = ref new Grid();
            grid->Width = 150;
            grid->Height = 150;
            grid->Children->Append(squareBorder0);

            popupSquareBorder0 = ref new Popup();
            popupSquareBorder0->ShouldConstrainToRootBounds = false;
            popupSquareBorder0->SystemBackdrop = backdrop;
            popupSquareBorder0->Child = grid;
        }

        {
            Border^ squareBorder3 = ref new Border();
            squareBorder3->BorderThickness = xaml::Thickness({ 3, 3, 3, 3 });
            squareBorder3->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            Grid^ grid = ref new Grid();
            grid->Width = 150;
            grid->Height = 150;
            grid->Children->Append(squareBorder3);

            popupSquareBorder3 = ref new Popup();
            popupSquareBorder3->ShouldConstrainToRootBounds = false;
            popupSquareBorder3->SystemBackdrop = backdrop;
            popupSquareBorder3->Child = grid;
        }

        {
            Border^ squareBorderUneven = ref new Border();
            squareBorderUneven->BorderThickness = xaml::Thickness({ 3, 3, 3, 2 });
            squareBorderUneven->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            Grid^ grid = ref new Grid();
            grid->Width = 150;
            grid->Height = 150;
            grid->Children->Append(squareBorderUneven);

            popupSquareBorderUneven = ref new Popup();
            popupSquareBorderUneven->ShouldConstrainToRootBounds = false;
            popupSquareBorderUneven->SystemBackdrop = backdrop;
            popupSquareBorderUneven->Child = grid;
        }

        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popups.");

        popupBorder0->XamlRoot = root->XamlRoot;
        popupBorder0->IsOpen = true;

        popupBorder3->XamlRoot = root->XamlRoot;
        popupBorder3->IsOpen = true;

        popupBorderUneven->XamlRoot = root->XamlRoot;
        popupBorderUneven->IsOpen = true;

        popupSquareBorder0->XamlRoot = root->XamlRoot;
        popupSquareBorder0->IsOpen = true;

        popupSquareBorder3->XamlRoot = root->XamlRoot;
        popupSquareBorder3->IsOpen = true;

        popupSquareBorderUneven->XamlRoot = root->XamlRoot;
        popupSquareBorderUneven->IsOpen = true;
    });
    wh->WaitForIdle();

    wfn_::float3 originalOffset;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Rounded corners, no borders.");
        VisualTreeVerifier^ verifier0 = VisualTreeVerifier::CreateFromElement(popupBorder0);
        verifier0
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifier0->VerifyVisualSize(150, 150);
        originalOffset = verifier0->GetVisualOffset();
        verifier0->VerifyRoundedCornerClip(0, 0, 150, 150, 8);

        LOG_OUTPUT(L"> Rounded corners, 3px borders.");
        VisualTreeVerifier^ verifier3 = VisualTreeVerifier::CreateFromElement(popupBorder3);
        verifier3
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifier3->VerifyVisualSize(150, 150);
        verifier3->VerifyRoundedCornerClip(3, 3, 147, 147, 6.5);

        LOG_OUTPUT(L"> Rounded corners, uneven borders.");
        VisualTreeVerifier^ verifierUneven = VisualTreeVerifier::CreateFromElement(popupBorderUneven);
        verifierUneven
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifierUneven->VerifyVisualSize(150, 150);
        verifierUneven->VerifyRoundedCornerClip(0, 0, 150, 150, 8);

        LOG_OUTPUT(L"> Square corners, no borders.");
        VisualTreeVerifier^ verifierSquare0 = VisualTreeVerifier::CreateFromElement(popupSquareBorder0);
        verifierSquare0
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifierSquare0->VerifyVisualSize(150, 150);
        verifierSquare0->VerifyRoundedCornerClip(0, 0, 150, 150, 0);

        LOG_OUTPUT(L"> Square corners, 3px borders.");
        VisualTreeVerifier^ verifierSquare3 = VisualTreeVerifier::CreateFromElement(popupSquareBorder3);
        verifierSquare3
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifierSquare3->VerifyVisualSize(150, 150);
        verifierSquare3->VerifyRoundedCornerClip(3, 3, 147, 147, 0);

        LOG_OUTPUT(L"> Square corners, uneven borders.");
        VisualTreeVerifier^ verifierSquareUneven = VisualTreeVerifier::CreateFromElement(popupSquareBorderUneven);
        verifierSquareUneven
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifierSquareUneven->VerifyVisualSize(150, 150);
        verifierSquareUneven->VerifyRoundedCornerClip(0, 0, 150, 150, 0);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Updating a popup child's size.");
        changingGrid->Width = 300;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Verifying that the backdrop size updated too.");
        VisualTreeVerifier^ verifier3 = VisualTreeVerifier::CreateFromElement(popupBorder3);
        verifier3
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifier3->VerifyVisualSize(300, 150);
        verifier3->VerifyRoundedCornerClip(3, 3, 297, 147, 6.5);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Updating a popup's position.");
        popupBorder0->VerticalOffset = 300;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Verifying that the backdrop position updated too.");
        VisualTreeVerifier^ verifier0 = VisualTreeVerifier::CreateFromElement(popupBorder0);
        verifier0
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        verifier0->VerifyVisualOffset(originalOffset.x, originalOffset.y + 300);
    });
}

void PopupTests::WindowedPopupDeviceLost()
{
    const auto& wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanupWrapper;

    Canvas^ root;
    Popup^ popup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popup.");

        Border^ border = ref new Border();
        border->Width = 150;
        border->Height = 150;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        popup = ref new Popup();
        popup->ShouldConstrainToRootBounds = false;
        popup->Child = border;

        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup.");

        popup->XamlRoot = root->XamlRoot;
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Device lost. Don't crash.");
        wh->ResetDeviceOnly();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromElement(popup);
        verifier
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            // Popup's comp node
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_PublicRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
            ->WalkThroughSimpleCompNode()
            ->WalkToChildAtIndex(0, 1);
        verifier->VerifyVisualSize(150, 150);
    });
}

void PopupTests::WindowedPopupAncestorScale()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& ih = TestServices::InputHelper;
    TestCleanupWrapper cleanupWrapper;

    Canvas^ root;
    Canvas^ clickTarget;
    Popup^ popup;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popup.");

        Border^ border = ref new Border();
        border->Width = 150;
        border->Height = 150;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"> Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        popup = ref new Popup();
        popup->ShouldConstrainToRootBounds = false;
        popup->Child = border;

        root = ref new Canvas();
        wh->WindowContent = root;

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 0.5;

        Grid^ doubleGrid = ref new Grid();
        doubleGrid->RenderTransform = scale;
        doubleGrid->Children->Append(popup);
        root->Children->Append(doubleGrid);

        // The popup is 150x150, under 2x / 0.5x it becomes 300x75.
        // Inject the click at (250,50), which misses the untransformed popup but hits the transformed one.
        clickTarget = ref new Canvas();
        clickTarget->Width = 100;
        clickTarget->Height = 100;
        Canvas::SetLeft(clickTarget, 200);
        root->Children->Append(clickTarget);
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
        VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromElement(popup);
        verifier->VerifyTransformMatrix_Scale(2, 0.5);
        verifier
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            // Popup's comp node
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_PublicRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
            ->WalkThroughSimpleCompNode()
            ->WalkToChildAtIndex(0, 1);
        verifier->VerifyVisualSize(150, 150);
    });

    LOG_OUTPUT(L"> Tapping on popup");
    ih->Tap(clickTarget);

    pointerPressedEvent->WaitForDefault();
}

void PopupTests::NoXamlRoot()
{
    const auto& wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanupWrapper;

    Canvas^ root;
    Popup^ parentlessPopup;
    Popup^ parentlessWindowedPopup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating popup.");

        parentlessPopup = ref new Popup();
        parentlessPopup->Child = ref new Border();

        parentlessWindowedPopup = ref new Popup();
        parentlessWindowedPopup->ShouldConstrainToRootBounds = false;
        parentlessWindowedPopup->Child = ref new Border();

        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup.");

        HRESULT hr = S_OK;

        try
        {
            parentlessPopup->IsOpen = true;
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }

        VERIFY_ARE_EQUAL(hr, E_UNEXPECTED);

        try
        {
            parentlessWindowedPopup->IsOpen = true;
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }

        VERIFY_ARE_EQUAL(hr, E_UNEXPECTED);
    });
    wh->WaitForIdle();
}

void PopupTests::CloseWindowWithPopupOpen()
{
    TestCleanupWrapper cleanup;

    WindowAutoCloser window1;

    TextBox^ tb1;
    auto tb1ContextMenuOpeningRegistration = CreateSafeEventRegistration(TextBox, ContextMenuOpening);
    auto tb1ContextMenuOpeningEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button x:Name='btn1' Width='150' Height='50' Content='Button1'/>"
            L"  <TextBox x:Name='tb1' Width='150' Height='50' Text='Some text'/>"
            L"</StackPanel>"));
        tb1 = safe_cast<TextBox^>(rootPanel->FindName(L"tb1"));

        window1.Attach(ref new Window());
        window1->Title = "Window1";
        window1->Content = rootPanel;
        window1->Activate();
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Setting focus to tb1 in window1");
    FocusTestHelper::EnsureFocusInIsland(tb1, FocusState::Pointer);
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(tb1->XamlRoot)->Equals(tb1));

        // Select the text and copy it to the clipboard. This is to ensure there is a
        // need to show the context menu both for the ability to copy and to paste.
        tb1->SelectAll();
        tb1->CopySelectionToClipboard();

        tb1ContextMenuOpeningRegistration.Attach(tb1,
            ref new xaml_controls::ContextMenuOpeningEventHandler([tb1ContextMenuOpeningEvent](Platform::Object^ sender, xaml_controls::ContextMenuEventArgs^ e)
        {
            LOG_OUTPUT(L"TextBox ContextMenuOpening");
            tb1ContextMenuOpeningEvent->Set();
        }));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Open the context menu using Shift+F10.");
    TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_f10#$u$_f10#$u$_shift");

    TestServices::WindowHelper->WaitForIdle();
    tb1ContextMenuOpeningEvent->WaitForDefault();

    LOG_OUTPUT(L"Closing the window.");
    window1.Close();
    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Window closed.");
}

} } } } } }
