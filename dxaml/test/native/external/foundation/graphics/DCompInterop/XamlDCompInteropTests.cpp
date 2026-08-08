// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "XamlDCompInteropTests.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <WUCRenderingScopeGuard.h>
#include <microsoft.ui.composition.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Composition;
using namespace ::Windows::Storage::Streams;

using namespace test_infra;
using namespace Concurrency;
using namespace MockDComp;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ XamlDCompInteropTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"Resources\\Native\\Foundation\\Graphics\\DCompInterop\\";
}

bool XamlDCompInteropTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool XamlDCompInteropTests::ClassCleanup()
{
    return true;
}

bool XamlDCompInteropTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool XamlDCompInteropTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a UIElement using the private
// ElementCompositionPreview interface implemented by UIElement.
//  - Set the visual's OffsetX & Y.
//  - Discards the visual.
//------------------------------------------------------------------------
void XamlDCompInteropTests::BasicUIElementDCompVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    Visual^ visual;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        visual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual);

        LOG_OUTPUT(L"Setting DComp visual's Opacity");
        visual->Opacity = 0.5f;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
}

//------------------------------------------------------------------------
// Test case:
//  Retrieves a DComp visual through ElementCompositionPreview for a
//  UIElement before the DComp resources were initialized.
//------------------------------------------------------------------------
void XamlDCompInteropTests::UIElementDCompVisualBeforeResourceCreationInternal()
{
    RunOnUIThread([&]()
    {
        Border^ border = nullptr;
        Visual^ visual;

        LOG_OUTPUT(L"Retrieving DComp visual for Border element.");
        border = ref new Microsoft::UI::Xaml::Controls::Border();
        VERIFY_IS_NOT_NULL(border);

        visual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual);
    });
}

//------------------------------------------------------------------------
// Test case:
// Various combinations of:
//  - Retrieves a DComp visual from a UIElement using the private
// ElementCompositionPreview interface implemented by UIElement.
//  - Sets the visual's OffsetX & Y.
//  - Provokes a device loss.
//  - Discards the visual.
//------------------------------------------------------------------------
void XamlDCompInteropTests::UIElementDCompVisualWithDeviceLossInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    Visual^ visual1 = nullptr;
    Visual^ visual2 = nullptr;
    Visual^ visual3 = nullptr;
    Visual^ visual4 = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element (1).");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);

        LOG_OUTPUT(L"Setting DComp visual's Opacity (1).");
        visual1->Opacity = 0.1f;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Simulating device loss (1).");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element (2).");
        visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        LOG_OUTPUT(L"Setting DComp visual's OffsetX/Y (2).");
        visual2->Opacity = 0.2f;

        // Returned visuals are expected to be the same
        VERIFY_ARE_EQUAL(visual1, visual2);
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Simulating device loss (2).");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element (3).");
        visual3 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual3);

        LOG_OUTPUT(L"Setting DComp visual's Opacity (3).");
        visual3->Opacity = 0.3f;

        // Returned visuals are expected to be the same
        VERIFY_ARE_EQUAL(visual2, visual3);
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Simulating device loss (3).");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element (4).");
        visual4 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual4);

        LOG_OUTPUT(L"Setting DComp visual's Opacity (4).");
        visual4->Opacity = 0.4f;

        // Returned visuals are expected to be the same
        VERIFY_ARE_EQUAL(visual3, visual4);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        visual1 = nullptr;
        visual2 = nullptr;
        visual3 = nullptr;
        visual4 = nullptr;
    });
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a UIElement using the private
// ElementCompositionPreview interface implemented by UIElement twice in a row.
//  - Discards the visual twice in a row too.
//------------------------------------------------------------------------
void XamlDCompInteropTests::UIElementDCompVisualAccessedDiscardedTwiceInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element twice.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        // Accessing 1st time
        Visual^ visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);

        // Accessing 2nd time
        Visual^ visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        // Same visual is expected to be returned
        VERIFY_ARE_EQUAL(visual1, visual2);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
}

//------------------------------------------------------------------------
// Test case: Retrieves and discards two DComp visuals from a UIElement
// using the private ElementCompositionPreview interface implemented by UIElement.
// Two distinct visuals are expected to be returned.
//------------------------------------------------------------------------
void XamlDCompInteropTests::UIElementDCompVisualsAccessedDiscardedSuccessivelyInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    Visual^ visual1;
    Visual^ visual2;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        LOG_OUTPUT(L"Retrieving 1st DComp visual for Border element.");
        visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);

        LOG_OUTPUT(L"Retrieving 2nd DComp visual for Border element.");
        visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        // Returned visuals are expected to be the same
        VERIFY_ARE_EQUAL(visual1, visual2);

        visual1 = nullptr;
        visual2 = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    // Now do the same thing but let a render walk insert the first visual.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving 1st DComp visual for Border element.");
        visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving 2nd DComp visual for Border element.");
        visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        // Returned visuals are expected to be the same
        VERIFY_ARE_EQUAL(visual1, visual2);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a DManip-manipulatable UIElement using
// the private ElementCompositionPreview interface implemented by UIElement.
//------------------------------------------------------------------------
void XamlDCompInteropTests::ManipulatableUIElementDCompVisualInternal()
{
    const auto& wh = TestServices::WindowHelper;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas;
    ScrollViewer^ scrollViewer;
    Border^ border;

    std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);

    Visual^ visual1;
    Visual^ visual2;

    RunOnUIThread([&]()
    {
        border = ref new Border();
        border->Width = 621;
        border->Height = 510;
        border->Background = ref new SolidColorBrush(Colors::Yellow);

        scrollViewer = ref new ScrollViewer();
        scrollViewer->Width = 321;
        scrollViewer->Height = 210;
        Canvas::SetLeft(scrollViewer, 9);
        Canvas::SetTop(scrollViewer, 11);
        scrollViewer->Content = border;

        LOG_OUTPUT(L"> Creating UIElement tree.");
        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(scrollViewer);
        wh->WindowContent = rootCanvas;

        loadedRegistration.Attach(scrollViewer, ref new xaml::RoutedEventHandler(
            [spHasLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"ScrollViewer Loaded event raised.");
            spHasLoadedEvent->Set();
        }));
    });

    spHasLoadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Retrieving DComp visual for Border element.");
        visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);

        LOG_OUTPUT(L"> Setting DComp visual's Opacity.");
        visual1->Opacity = 0.5f;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Retrieving DComp visual again for Border element to ensure recycling occurs.");
        visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        VERIFY_ARE_EQUAL(visual1, visual2);
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a UIElement using the private
// ElementCompositionPreview interface implemented by UIElement.
//  - Removes that UIElement from the tree.
//  - Retrieves the same DComp visual again.
//  - Re-enters the tree. Ensures same DComp visual is still present.
//  - Discards the visual.
//------------------------------------------------------------------------
void XamlDCompInteropTests::LeavingUIElementDCompVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    Visual^ visual1;
    Visual^ visual2;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    // Case #1: Remove the UIElement before the handoff visual is consumed in a render walk.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);

        LOG_OUTPUT(L"Setting DComp visual's Opacity.");
        visual1->Opacity = 0.5f;

        LOG_OUTPUT(L"Remove Border from tree.");
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element again, after its removal from the tree.");
        Visual^ visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        // Same visual is expected to be returned
        VERIFY_ARE_EQUAL(visual1, visual2);

        LOG_OUTPUT(L"Discarding DComp visual for Border element.");
        visual1 = nullptr;
        visual2 = nullptr;
        border = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    // case #2: Remove the UIElement after the handoff visual is consumed in a render walk.
    rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        visual1 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual1);

        LOG_OUTPUT(L"Setting DComp visual's Opacity.");
        visual1->Opacity = 0.5f;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove Border from tree.");
        rootCanvas->Children->Clear();

        LOG_OUTPUT(L"Retrieving DComp visual for Border element again, after its removal from the tree.");
        visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        // Same visual is expected to be returned
        VERIFY_ARE_EQUAL(visual1, visual2);

        visual2 = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Re-add Border to tree.");
        VERIFY_IS_NOT_NULL(border);
        rootCanvas->Children->Append(border);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element again, after its re-addition to the tree.");
        visual2 = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual2);

        // Same visual is expected to be returned
        VERIFY_ARE_EQUAL(visual1, visual2);

        visual1 = nullptr;
        visual2 = nullptr;

        LOG_OUTPUT(L"Remove Border from tree.");
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a UIElement that is not in the tree.
//  - Set the visual's OffsetX & Y.
//  - Discards the visual.
//------------------------------------------------------------------------
void XamlDCompInteropTests::UnparentedUIElementDCompVisualInternal()
{
    RunOnUIThread([&]()
    {
        Border^ border = nullptr;
        Visual^ visual;

        LOG_OUTPUT(L"Retrieving DComp visual for unparented Border element.");
        border = ref new Border();
        visual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual);

        LOG_OUTPUT(L"Setting DComp visual's Opacity.");
        visual->Opacity = 0.5f;
    });
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a UIElement that is not in the tree.
//  - Set the visual's OffsetX & Y.
//  - Enter the tree.
//  - Discards the visual.
//------------------------------------------------------------------------
void XamlDCompInteropTests::EnteringUIElementDCompVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    xaml_shapes::Rectangle^ rect = nullptr;

    Visual^ visual1;
    Visual^ visual2;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        rect = ref new xaml_shapes::Rectangle();

        visual1 = ElementCompositionPreview::GetElementVisual(rect);
        VERIFY_IS_NOT_NULL(visual1);

        LOG_OUTPUT(L"Setting DComp visual's Opacity");
        visual1->Opacity = 0.5f;

        LOG_OUTPUT(L"Add Rectangle to tree.");
        rootCanvas->Children->Append(rect);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element again, after entering the tree.");
        visual2 = ElementCompositionPreview::GetElementVisual(rect);
        VERIFY_IS_NOT_NULL(visual2);

        // Same visual is expected to be returned
        VERIFY_ARE_EQUAL(visual1, visual2);

        visual1 = nullptr;
        visual2 = nullptr;
    });
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from an invisible UIElement using the private
// ElementCompositionPreview interface implemented by UIElement.
//  - Set the visual's OffsetX & Y.
//  - Discards the visual.
//------------------------------------------------------------------------
void XamlDCompInteropTests::InvisibleUIElementDCompVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    Visual^ visual;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);
        border->Visibility = Visibility::Collapsed;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element.");
        visual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual);

        LOG_OUTPUT(L"Setting DComp visual's Opacity.");
        visual->Opacity = 0.5f;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

void XamlDCompInteropTests::DestroyUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, true /* resizeWindow */, false /* injectMockDComp */, false /* resetDevice*/);
    DestroyUIElementDCompVisualInternal();
}

void XamlDCompInteropTests::DestroyUIElementDCompVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;

    Visual^ visual;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving DComp visual for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        visual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(visual);

        LOG_OUTPUT(L"Remove Border from tree.");
        rootCanvas->Children->Clear();

        LOG_OUTPUT(L"Cause UIElement's destructor to be run. We shouldn't crash in force closing handoff code.");
        border = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a DComp visual from a UIElement using the private
// ElementCompositionPreview interface implemented by UIElement.
//  - RTB the element.
//------------------------------------------------------------------------
void XamlDCompInteropTests::DCompVisualRTB()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas;

    Border^ outerBorder;
    Visual^ outerVisual;
    Border^ innerBorder;
    Visual^ innerVisual;

    RenderTargetBitmap^ outerRTB;
    wf::IAsyncOperation<IBuffer^>^ outerGetPixelsAsyncOperation;
    IBuffer^ outerBuffer;
    auto outerRenderedEvent = std::make_shared<Event>();
    auto outerGetPixelsEvent = std::make_shared<Event>();

    RenderTargetBitmap^ innerRTB;
    wf::IAsyncOperation<IBuffer^>^ innerGetPixelsAsyncOperation;
    IBuffer^ innerBuffer;
    auto innerRenderedEvent = std::make_shared<Event>();
    auto innerGetPixelsEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");

        innerBorder = ref new Border();
        innerBorder->Background = ref new SolidColorBrush(Colors::Yellow);

        outerBorder = ref new Border();
        outerBorder->Width = 321;
        outerBorder->Height = 210;
        Canvas::SetLeft(outerBorder, 9);
        Canvas::SetTop(outerBorder, 11);
        outerBorder->Child = innerBorder;

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(outerBorder);
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting DComp visual's Opacity.");
        innerVisual = ElementCompositionPreview::GetElementVisual(innerBorder);
        VERIFY_IS_NOT_NULL(innerVisual);
        innerVisual->Opacity = 0.5f;
        outerVisual = ElementCompositionPreview::GetElementVisual(outerBorder);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        outerRTB = ref new RenderTargetBitmap();
        innerRTB = ref new RenderTargetBitmap();

        LOG_OUTPUT(L"> RenderTargetBitmap::RenderAsync.");
        create_task(outerRTB->RenderAsync(outerBorder)).then([&]()
        {
            LOG_OUTPUT(L"  > Outer RenderTargetBitmap::RenderAsync completed.");
            outerRenderedEvent->Set();
        });
        create_task(innerRTB->RenderAsync(innerBorder)).then([&]()
        {
            LOG_OUTPUT(L"  > Inner RenderTargetBitmap::RenderAsync completed.");
            innerRenderedEvent->Set();
        });
    });

    outerRenderedEvent->WaitForDefault();
    innerRenderedEvent->WaitForDefault();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> RenderTargetBitmap::GetPixelsAsync.");
        outerGetPixelsAsyncOperation = outerRTB->GetPixelsAsync();
        innerGetPixelsAsyncOperation = innerRTB->GetPixelsAsync();

        auto outerGetPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>([&](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"  > Outer GetPixelsAsync operation completed.");
            outerBuffer = operation->GetResults();
            outerGetPixelsEvent->Set();
        });
        VERIFY_IS_NOT_NULL(outerGetPixelsCallback);
        outerGetPixelsAsyncOperation->Completed = outerGetPixelsCallback;
        auto innerGetPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>([&](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"  > Inner GetPixelsAsync operation completed.");
            innerBuffer = operation->GetResults();
            innerGetPixelsEvent->Set();
        });
        VERIFY_IS_NOT_NULL(innerGetPixelsCallback);
        innerGetPixelsAsyncOperation->Completed = innerGetPixelsCallback;
    });

    outerGetPixelsEvent->WaitForDefault();
    innerGetPixelsEvent->WaitForDefault();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Verifying bitmap content.");
        DataReader^ outerDataReader = DataReader::FromBuffer(outerBuffer);
        DataReader^ innerDataReader = DataReader::FromBuffer(innerBuffer);
        Platform::Array<byte>^ outerBytes = ref new Platform::Array<byte>(outerBuffer->Length);
        Platform::Array<byte>^ innerBytes = ref new Platform::Array<byte>(innerBuffer->Length);
        outerDataReader->ReadBytes(outerBytes);
        innerDataReader->ReadBytes(innerBytes);
        VERIFY_ARE_EQUAL(outerBytes->Length, static_cast<unsigned int>(321 * 210 * 4));
        VERIFY_ARE_EQUAL(innerBytes->Length, static_cast<unsigned int>(321 * 210 * 4));

        // Every pixel should be yellow (bgra = 0x00ffffff)
        // The inner RTB ignores local properties like opacity, so it should be fully opaque.
        // The outer RTB respects the opacity of inner elements, so it should be translucent. 
        // Don't actually test every pixel - just one pixel every row is fine
        for (int i = 0; i < 321 * 210; i += 322)
        {
            WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

            // In SpriteVisuals mode, we use live capture and the pixels are expected to be premultiplied alpha
            // Note: Composition rendering sometimes rounds down when calculating the half-transparent pixel. Accept either 127 or 128.
            // Using 0xfe for the full color doens't fix this. Composition can still round down to 126.
            VERIFY_ARE_EQUAL(outerBytes[i * 4 + 0], 0);
            VERIFY_IS_TRUE(127 <= outerBytes[i * 4 + 1] && outerBytes[i * 4 + 1] <= 128);
            VERIFY_IS_TRUE(127 <= outerBytes[i * 4 + 2] && outerBytes[i * 4 + 1] <= 128);
            VERIFY_IS_TRUE(127 <= outerBytes[i * 4 + 3] && outerBytes[i * 4 + 1] <= 128);

            VERIFY_ARE_EQUAL(innerBytes[i * 4 + 0], 0);
            VERIFY_ARE_EQUAL(innerBytes[i * 4 + 1], 255);
            VERIFY_ARE_EQUAL(innerBytes[i * 4 + 2], 255);
            VERIFY_ARE_EQUAL(innerBytes[i * 4 + 3], 255);
        }
    });

    LOG_OUTPUT(L"> Verify rendering after RTB. The handoff visual should still be in the tree.");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
}

void XamlDCompInteropTests::BasicUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    BasicUIElementDCompVisualInternal();
}

void XamlDCompInteropTests::UIElementDCompVisualBeforeResourceCreationWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    UIElementDCompVisualBeforeResourceCreationInternal();
}

void XamlDCompInteropTests::UIElementDCompVisualWithDeviceLossWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    UIElementDCompVisualWithDeviceLossInternal();
}

void XamlDCompInteropTests::UIElementDCompVisualAccessedDiscardedTwiceWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    UIElementDCompVisualAccessedDiscardedTwiceInternal();
}

void XamlDCompInteropTests::UIElementDCompVisualsAccessedDiscardedSuccessivelyWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    UIElementDCompVisualsAccessedDiscardedSuccessivelyInternal();
}

void XamlDCompInteropTests::ManipulatableUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ManipulatableUIElementDCompVisualInternal();
}

void XamlDCompInteropTests::LeavingUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    LeavingUIElementDCompVisualInternal();
}

void XamlDCompInteropTests::UnparentedUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    UnparentedUIElementDCompVisualInternal();
}

void XamlDCompInteropTests::EnteringUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    EnteringUIElementDCompVisualInternal();
}

void XamlDCompInteropTests::InvisibleUIElementDCompVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    InvisibleUIElementDCompVisualInternal();
}

} } } } } }
