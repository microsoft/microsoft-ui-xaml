// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <WindowsNumerics.h>
#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "XamlWinRTCompInteropUnrestrictedTests.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <DisableErrorReportingScopeGuard.h>
#include <WUCRenderingScopeGuard.h>
#include <Microsoft.UI.Composition.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Composition;
using namespace MockDComp;

namespace wfn = ::Windows::Foundation::Numerics;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ XamlWinRTCompInteropUnrestrictedTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"Resources\\Native\\Foundation\\Graphics\\DCompInterop\\";
}

bool XamlWinRTCompInteropUnrestrictedTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool XamlWinRTCompInteropUnrestrictedTests::ClassCleanup()
{
    return true;
}

bool XamlWinRTCompInteropUnrestrictedTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool XamlWinRTCompInteropUnrestrictedTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a WinRT Windows.UI.Composition.Visual instance for a UIElement using
//    the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual method.
//  - Reads and writes the Opacity property for that Visual.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::BasicGetElementVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Reading WinRT Visual.Opacity property value.");
        float opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's default Opacity=%f.", opacity);
        compositionVisual->Opacity = 0.25;
        opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's new Opacity=%f.", opacity);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Retrieves a WinRT Windows.UI.Composition.Visual instance for a UIElement using
//  the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual
//  method after the UIElement left the visual tree.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualAfterLeavingTreeInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove Border element from visual tree.");
        rootCanvas->Children->Clear();
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element again.");
        Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        VERIFY_ARE_EQUAL(nonNullCompositionVisual, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding Visual for Border element.");
        border = nullptr;
        compositionVisual = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a WinRT Windows.UI.Composition.Visual instance for a UIElement using
//    the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual method.
//  - Verifies that this Visual's parent is null.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithNullParentCheckInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Reading WinRT Visual.Parent property value.");
        ContainerVisual^ parent = compositionVisual->Parent;
        VERIFY_IS_NULL(parent);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a WinRT Windows.UI.Composition.Visual instance for a UIElement using
//    the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual method.
//  - Verifies that this Visual cannot be casted into a ContainerVisual.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithNullContainerCheckInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Casting WinRT Visual into a ContainerVisual.");
        ContainerVisual^ compositionContainerVisual = dynamic_cast<ContainerVisual^>(compositionVisual);
        VERIFY_IS_NOT_NULL(compositionContainerVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a WinRT Windows.UI.Composition.Visual instance for a UIElement using
//    the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual method.
//  - Simulates a device loss.
//  - Reads and writes the Opacity property for that Visual.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithDeviceLossInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        // Note: A common reason for compositionVisual being null (and this assert being hit) is an access
        // violation because DComp says that you don't have the right capability.  This is most likey because
        // you are attempting to run this test by itself.  You can either run the full set of tests for this
        // class, or manually set the PreviewCapability registry entry referenced earlier in this file.
        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Simulating device loss.");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Reading WinRT Visual.Opacity property value.");
        float opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's default Opacity=%f.", opacity);
        compositionVisual->Opacity = 0.25;
        opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's new Opacity=%f.", opacity);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding Windows.UI.Composition.Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  - Retrieves a WinRT Windows.UI.Composition.Visual instance for a UIElement using
//    the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual method.
//  - Writes the Opacity property for that Visual.
//  - Ensures the DComp device invokes Commit() to apply the opacity change.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithCommitRequestInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Clear the MockDCompDevice::m_wasCommitInvoked flag
        VERIFY_IS_TRUE(TestServices::Utilities->WasCommitInvoked);
        TestServices::Utilities->ResetWasCommitInvoked();
        VERIFY_IS_FALSE(TestServices::Utilities->WasCommitInvoked);

        LOG_OUTPUT(L"Writing WinRT Visual.Opacity property.");
        compositionVisual->Opacity = 0.25;
        float opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's new Opacity=%f.", opacity);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Make sure the MockDCompDevice::m_wasCommitInvoked flag was set,
        // which indicates that a Commit() was executed.
        VERIFY_IS_TRUE(TestServices::Utilities->WasCommitInvoked);

        LOG_OUTPUT(L"Discarding Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.SetElementChildVisual
//  with a null WinRT Visual, when no visual was previously set.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::ResetNullElementChildVisualInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        Border^ border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        LOG_OUTPUT(L"Resetting the already null child Windows.UI.Composition.Visual for the Border element.");
        ElementCompositionPreview::SetElementChildVisual(border, nullptr);
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.SetElementChildVisual
//  for an empty Border with a non-null WinRT Visual, when no visual was previously set.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForBorderInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual to set child.");
        ElementCompositionPreview::SetElementChildVisual(border, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding child Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes the ElementCompositionPreview.SetElementChildVisual for an unparented Button element,
//  then adds it to the visual tree and calls ElementCompositionPreview.GetElementChildVisual.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForUnparentedElementInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Button^ button = nullptr;
    Visual^ compositionVisual = nullptr;

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

        button = ref new Button();
        button->Content = "btn";
        button->Width = 100;
        button->Height = 25;

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for Button element to set child.");
        ElementCompositionPreview::SetElementChildVisual(button, compositionVisual);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for Button element.");
        Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(button);
        VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        VERIFY_ARE_EQUAL(nonNullCompositionVisual, compositionVisual);

        border->Child = button;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for Button element.");
        Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(button);
        VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        VERIFY_ARE_EQUAL(nonNullCompositionVisual, compositionVisual);

        LOG_OUTPUT(L"Discarding child Visual for Border element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.SetElementChildVisual
//  for a populated Panel with a non-null WinRT Visual, when no visual was previously set.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForPanelInternal()
{
    SetElementChildVisualForPanelPrivate(false /*panelChildrenUseCompositionNodes*/);
}

//------------------------------------------------------------------------
// Test case:
//  Invokes ElementCompositionPreview.SetElementChildVisual for a Panel populated
//  with XAML children which require composition nodes.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForComplexPanelInternal()
{
    SetElementChildVisualForPanelPrivate(true /*panelChildrenUseCompositionNodes*/);
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForPanelPrivate(_In_ bool panelChildrenUseCompositionNodes)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        if (panelChildrenUseCompositionNodes)
        {
            for (int iChild = 0; iChild < 3; iChild++)
            {
                LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementVisual for StackPanel child element.");
                Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(stackPanel->Children->GetAt(iChild));
                VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
            }
        }
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding child Visual for StackPanel element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes ElementCompositionPreview.SetElementChildVisual for a populated Panel.
//  Then invokes ElementCompositionPreview.GetElementChildVisual after the Panel
//  left the visual tree.
//  Invokes ElementCompositionPreview.SetElementChildVisual to reset the child.
//  Then invokes ElementCompositionPreview.GetElementChildVisual again to read
//  the null child.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::GetElementChildVisualAfterLeavingTreeInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove StackPanel element from visual tree.");
        rootCanvas->Children->Clear();

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        VERIFY_ARE_EQUAL(nonNullCompositionVisual, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        VERIFY_ARE_EQUAL(nonNullCompositionVisual, compositionVisual);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to reset child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, nullptr);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NULL(nullCompositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NULL(nullCompositionVisual);

        LOG_OUTPUT(L"Discarding child Visual for StackPanel element.");
        compositionVisual = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes the GetElementChildVisual to read the initially null child visual.
//  Invokes the public Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.SetElementChildVisual
//  for a populated Panel with a non-null WinRT Visual, when no visual was previously set.
//  Invokes the Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementChildVisual to read
//  that visual back.
//  On idle, invokes the SetElementChildVisual again to reset the visual.
//  Invokes the GetElementChildVisual again to read the null visual back.
//  On idle, invokes the SetElementChildVisual to set and reset the visual.
//  Invokes the GetElementChildVisual again to read the null visual back.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::ResetElementChildVisualForPanelInternal()
{
    ResetElementChildVisualForPanelPrivate(false /*panelChildrenUseCompositionNodes*/);
}

//------------------------------------------------------------------------
// Test case:
//  Same as ResetElementChildVisualForPanel except the Panel's children require
//  a composition node.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::ResetElementChildVisualForComplexPanelInternal()
{
    ResetElementChildVisualForPanelPrivate(true /*panelChildrenUseCompositionNodes*/);
}

void XamlWinRTCompInteropUnrestrictedTests::ResetElementChildVisualForPanelPrivate(_In_ bool panelChildrenUseCompositionNodes)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        if (panelChildrenUseCompositionNodes)
        {
            for (int iChild = 0; iChild < 3; iChild++)
            {
                LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementVisual for StackPanel child element.");
                Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(stackPanel->Children->GetAt(iChild));
                VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
            }
        }
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NULL(nullCompositionVisual);

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        VERIFY_ARE_EQUAL(nonNullCompositionVisual, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to reset child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, nullptr);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NULL(nullCompositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);

        LOG_OUTPUT(L"Immediately invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to reset child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, nullptr);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementChildVisual for StackPanel element.");
        Visual^ nullCompositionVisual = ElementCompositionPreview::GetElementChildVisual(stackPanel);
        VERIFY_IS_NULL(nullCompositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding child Visual for StackPanel element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes ElementCompositionPreview.SetElementChildVisual for a populated Panel
//  with a WinRT Visual. Then adds a XAML child to the Panel.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetChildAndAddChildToPanelInternal()
{
    SetElementChildVisualBeforeAddingChildToPanelPrivate(false /*panelChildrenUseCompositionNodes*/);
}

//------------------------------------------------------------------------
// Test case:
//  Same as SetChildAndAddChildToPanel except the Panel's
//  children require a composition node.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetChildAndAddChildToComplexPanelInternal()
{
    SetElementChildVisualBeforeAddingChildToPanelPrivate(true /*panelChildrenUseCompositionNodes*/);
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualBeforeAddingChildToPanelPrivate(_In_ bool panelChildrenUseCompositionNodes)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        if (panelChildrenUseCompositionNodes)
        {
            for (int iChild = 0; iChild < 3; iChild++)
            {
                LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementVisual for StackPanel child element.");
                Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(stackPanel->Children->GetAt(iChild));
                VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
            }
        }
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Append a XAML child to the StackPanel element.");
        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Height = 15;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);

        if (panelChildrenUseCompositionNodes)
        {
            LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementVisual for Rectangle about to be added to the StackPanel element.");
            Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(rect);
            VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
        }

        stackPanel->Children->InsertAt(3, rect);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding child Visual for StackPanel element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes ElementCompositionPreview.SetElementChildVisual for a populated Panel
//  with a WinRT Visual. Then removes the first and last XAML children from the Panel,
//  and finally clears out the last XAML child from the Panel.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetChildAndRemoveChildrenFromPanelInternal()
{
    SetElementChildVisualBeforeRemovingChildrenFromPanelPrivate(false /*panelChildrenUseCompositionNodes*/);
}

//------------------------------------------------------------------------
// Test case:
//  Same as SetChildAndRemoveChildrenFromPanel except the Panel's
//  children require a composition node.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetChildAndRemoveChildrenFromComplexPanelInternal()
{
    SetElementChildVisualBeforeRemovingChildrenFromPanelPrivate(true /*panelChildrenUseCompositionNodes*/);
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualBeforeRemovingChildrenFromPanelPrivate(_In_ bool panelChildrenUseCompositionNodes)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        if (panelChildrenUseCompositionNodes)
        {
            for (int iChild = 0; iChild < 3; iChild++)
            {
                LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementVisual for StackPanel child element.");
                Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(stackPanel->Children->GetAt(iChild));
                VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
            }
        }
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove first XAML child from StackPanel element.");
        stackPanel->Children->RemoveAt(0);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove last XAML child from StackPanel element.");
        stackPanel->Children->RemoveAt(1);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Clear last XAML child from StackPanel element.");
        stackPanel->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"4").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding child Visual for StackPanel element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Invokes ElementCompositionPreview.SetElementChildVisual for a populated Panel
//  with a WinRT Visual. Then clears all XAML children from the Panel.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetChildAndClearChildrenFromPanelInternal()
{
    SetElementChildVisualBeforeClearingChildrenFromPanelPrivate(false /*panelChildrenUseCompositionNodes*/);
}

//------------------------------------------------------------------------
// Test case:
//  Same as SetChildAndClearChildrenFromPanel except the Panel's
//  children require a composition node.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::SetChildAndClearChildrenFromComplexPanelInternal()
{
    SetElementChildVisualBeforeClearingChildrenFromPanelPrivate(true /*panelChildrenUseCompositionNodes*/);
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualBeforeClearingChildrenFromPanelPrivate(_In_ bool panelChildrenUseCompositionNodes)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        if (panelChildrenUseCompositionNodes)
        {
            for (int iChild = 0; iChild < 3; iChild++)
            {
                LOG_OUTPUT(L"Invoking ElementCompositionPreview::GetElementVisual for StackPanel child element.");
                Visual^ nonNullCompositionVisual = ElementCompositionPreview::GetElementVisual(stackPanel->Children->GetAt(iChild));
                VERIFY_IS_NOT_NULL(nonNullCompositionVisual);
            }
        }
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for StackPanel element to set child.");
        ElementCompositionPreview::SetElementChildVisual(stackPanel, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Clear all XAML children from the StackPanel element.");
        stackPanel->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Discarding child Visual for StackPanel element.");
        compositionVisual = nullptr;
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
}

Visual^ XamlWinRTCompInteropUnrestrictedTests::CreateVisual() const
{
    LOG_OUTPUT(L"Creating Windows.UI.Composition.Compositor.");

    // Create a WinRT Compositor based on a Xaml element rather than doing
    // Compositor^ compositor = ref new Compositor();
    // ElementCompositionPreview::SetElementChildVisual only accepts visuals
    // created on the same device as the Xaml elements.

    xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();

    Visual^ compositionVisual = ElementCompositionPreview::GetElementVisual(rect);
    VERIFY_IS_NOT_NULL(compositionVisual);

    Compositor^ compositor = compositionVisual->Compositor;
    VERIFY_IS_NOT_NULL(compositor);

    LOG_OUTPUT(L"Creating Windows.UI.Composition.SpriteVisual.");
    SpriteVisual^ spriteVisual = compositor->CreateSpriteVisual();
    VERIFY_IS_NOT_NULL(spriteVisual);

    LOG_OUTPUT(L"Setting Windows.UI.Composition.SpriteVisual's Color.");
    spriteVisual->Brush = compositor->CreateColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xFF, 0xEE, 0xDD, 0xCC));
    VERIFY_IS_NOT_NULL(spriteVisual->Brush);

    LOG_OUTPUT(L"Setting Windows.UI.Composition.SpriteVisual's Size.");
    wfn_::float2 size;
    size.x = 10.0f;
    size.y = 20.0f;
    spriteVisual->Size = size;

    rect = nullptr;
    compositionVisual = nullptr;

    return safe_cast<Visual^>(spriteVisual);
}

//------------------------------------------------------------------------
// Test case:
//  - Transforms the HandOff visual and hit tests it.
//------------------------------------------------------------------------
void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualInternal()
{
     TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

     Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-HitTest.xaml"));
     Border^ border = nullptr;
     Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;
     auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
     std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

     RunOnUIThread([&]()
     {
         VERIFY_IS_NOT_NULL(rootCanvas);
         TestServices::WindowHelper->WindowContent = rootCanvas;
     });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.ContainerVisual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Before");

    LOG_OUTPUT(L"Simulate Tap to detect if hit testing works before offset is changed.");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    // Change HandOffVisual's offset
    LOG_OUTPUT(L"Change HandOffVisual's offset.");
    RunOnUIThread([&]()
    {
        wfn_::float3 newOffset = { 109, 61, 0 };

        LOG_OUTPUT(L"Reading original WinRT Visual.Offset");
        wfn_::float3 offset = handOffVisual->Offset;
        VERIFY_IS_TRUE(offset.x == 0 && offset.y == 0);

        LOG_OUTPUT(L"Set WinRT Visual.Offset");
        handOffVisual->Offset = newOffset;
        offset = handOffVisual->Offset;
        VERIFY_IS_TRUE(offset.x == newOffset.x && offset.y == newOffset.y);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Offset");

    // Tap using hard-coded values to ensure that the element's local transform is not used to determine where to tap,
    // because the local transform uses the HandOff visual transform feature that this test is validating.
    LOG_OUTPUT(L"Simulate Tap to detect if hit testing works after offset is changed.");
    TestServices::InputHelper->Tap(wf::Point(383, 261));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

     // Change HandOffVisual's scale
    LOG_OUTPUT(L"Change HandOffVisual's scale.");
    RunOnUIThread([&]()
    {
        wfn_::float3 newScale = { 0.25, 0.25, 0 };
        wfn_::float3 newCenterPoint = { 160, 105, 0 };

        LOG_OUTPUT(L"Reading original WinRT Visual.Scale and Visual.CenterPoint");

        wfn_::float3 scale = handOffVisual->Scale;
        VERIFY_IS_TRUE(scale.x == 1 && scale.y == 1);

        wfn_::float3 centerPoint = handOffVisual->CenterPoint;
        VERIFY_ARE_EQUAL(0, centerPoint.x);
        VERIFY_ARE_EQUAL(0, centerPoint.y);


        LOG_OUTPUT(L"Set WinRT Visual.CenterPoint and Visual.Scale");

        handOffVisual->CenterPoint = newCenterPoint;
        centerPoint = handOffVisual->CenterPoint;
        VERIFY_ARE_EQUAL(newCenterPoint.x, centerPoint.x);
        VERIFY_ARE_EQUAL(newCenterPoint.y, centerPoint.y);

        handOffVisual->Scale = newScale;
        scale = handOffVisual->Scale;
        VERIFY_ARE_EQUAL(newScale.x, scale.x);
        VERIFY_ARE_EQUAL(newScale.y, scale.y);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Scale");

    // Tap using hard-coded values to ensure that the element's local transform is not used to determine where to tap,
    // because the local transform uses the HandOff visual transform feature that this test is validating.
    LOG_OUTPUT(L"Simulate Tap to detect if hit testing works after scale is changed.");
    TestServices::InputHelper->Tap(wf::Point(269, 166));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    // Change HandOffVisual's rotation
    LOG_OUTPUT(L"Change HandOffVisual's rotation.");
    RunOnUIThread([&]()
    {
        float newRotationAngleInDegrees = 45;

        LOG_OUTPUT(L"Restoring original CenterPoint");
        handOffVisual->CenterPoint = { 0, 0, 0 };

        LOG_OUTPUT(L"Reading original WinRT Visual.RotationAngleInDegrees");
        float rotationAngleInDegrees = handOffVisual->RotationAngleInDegrees;
        VERIFY_IS_TRUE(rotationAngleInDegrees == 0);

        LOG_OUTPUT(L"Set WinRT Visual.RotationAngleInDegrees");
        handOffVisual->RotationAngleInDegrees = newRotationAngleInDegrees;
        rotationAngleInDegrees = handOffVisual->RotationAngleInDegrees;
        VERIFY_IS_TRUE(rotationAngleInDegrees == newRotationAngleInDegrees);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Rotation");

    // Tap using hard-coded values to ensure that the element's local transform is not used to determine where to tap,
    // because the local transform uses the HandOff visual transform feature that this test is validating.
    LOG_OUTPUT(L"Simulate Tap to detect if hit testing works after rotation is changed.");
    TestServices::InputHelper->Tap(wf::Point(130, 145));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    // Change HandOffVisual's TransformMatrix
    LOG_OUTPUT(L"Change HandOffVisual's TransformMatrix.");
    RunOnUIThread([&]()
    {
        // Set Skew in Matrix
        wfn_::float4x4 newMatrix4x4 =
        {
            1.0f, // M11
            0.57735f, // M12
            0.0f, // M13
            0.0f, // M14
            1.0f, // M21
            1.0f, // M22
            0.0f, // M23
            0.0f, // M24
            0.0f, // M31
            0.0f, // M32
            1.0f, // M33
            0.0f, // M34
            0.0f, // M41
            0.0f, // M42
            0.0f, // M43
            1.0f  // M44
        };

        LOG_OUTPUT(L"Reading original WinRT Visual.TransformMatrix");
        wfn_::float4x4 matrix4x4 = handOffVisual->TransformMatrix;
        VERIFY_IS_TRUE(matrix4x4.m11 == 1.0f
            && matrix4x4.m12 == 0.0f
            && matrix4x4.m13 == 0.0f
            && matrix4x4.m14 == 0.0f
            && matrix4x4.m21 == 0.0f
            && matrix4x4.m22 == 1.0f
            && matrix4x4.m23 == 0.0f
            && matrix4x4.m24 == 0.0f
            && matrix4x4.m31 == 0.0f
            && matrix4x4.m32 == 0.0f
            && matrix4x4.m33 == 1.0f
            && matrix4x4.m34 == 0.0f
            && matrix4x4.m41 == 0.0f
            && matrix4x4.m42 == 0.0f
            && matrix4x4.m43 == 0.0f
            && matrix4x4.m44 == 1.0f
            );

        LOG_OUTPUT(L"Set WinRT Visual.TransformMatrix");
        handOffVisual->TransformMatrix = newMatrix4x4;
        matrix4x4 = handOffVisual->TransformMatrix;
        VERIFY_IS_TRUE(matrix4x4.m11 == newMatrix4x4.m11
                && matrix4x4.m12 == newMatrix4x4.m12
                && matrix4x4.m13 == newMatrix4x4.m13
                && matrix4x4.m14 == newMatrix4x4.m14
                && matrix4x4.m21 == newMatrix4x4.m21
                && matrix4x4.m22 == newMatrix4x4.m22
                && matrix4x4.m23 == newMatrix4x4.m23
                && matrix4x4.m24 == newMatrix4x4.m24
                && matrix4x4.m31 == newMatrix4x4.m31
                && matrix4x4.m32 == newMatrix4x4.m32
                && matrix4x4.m33 == newMatrix4x4.m33
                && matrix4x4.m34 == newMatrix4x4.m34
                && matrix4x4.m41 == newMatrix4x4.m41
                && matrix4x4.m42 == newMatrix4x4.m42
                && matrix4x4.m43 == newMatrix4x4.m43
                && matrix4x4.m44 == newMatrix4x4.m44
                );
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"TransformMatrix");

    // Tap using hard-coded values to ensure that the element's local transform is not used to determine where to tap,
    // because the local transform uses the HandOff visual transform feature that this test is validating.
    LOG_OUTPUT(L"Simulate Tap to verify hit testing after Visual.TransformMatrix is changed.");
    TestServices::InputHelper->Tap(wf::Point(130, 202));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualScaleRotate()
{
     TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

     Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-HitTest.xaml"));
     Border^ border = nullptr;
     Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;
     auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
     std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

     RunOnUIThread([&]()
     {
         VERIFY_IS_NOT_NULL(rootCanvas);
         TestServices::WindowHelper->WindowContent = rootCanvas;
     });
     TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.ContainerVisual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        border->Width = 50;
        border->Height = 50;
        Canvas::SetLeft(border, 200);
        Canvas::SetTop(border, 200);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });
    TestServices::WindowHelper->WaitForIdle();

    // Change HandOffVisual's Scale and Rotation
    LOG_OUTPUT(L"Change HandOffVisual's Scale and Rotation");
    RunOnUIThread([&]()
    {
         handOffVisual->RotationAngleInDegrees = 90;
         handOffVisual->Scale = { 2.0, 1.0, 0 };
     });
    TestServices::WindowHelper->WaitForIdle();

    // Tap using hard-coded values to ensure that the element's local transform is not used to determine where to tap,
    // because the local transform uses the HandOff visual transform feature that this test is validating.
    LOG_OUTPUT(L"Simulate Tap to detect if hit testing works after Scale/Rotation is changed.");
    TestServices::InputHelper->Tap(wf::Point(175, 225));
    pointerPressedEvent->WaitForDefault();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestFacadesWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-HitTest.xaml"));
    Border^ border = nullptr;
    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    RunOnUIThread([&]()
    {
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Base case:  Tap on Border");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wfn_::float3 translate = { 109, 61, 0 };
        border->Translation = translate;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap with Translate set");
    TestServices::InputHelper->Tap(wf::Point(383, 261));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wfn_::float3 newScale = { 0.25, 0.25, 0 };
        wfn_::float3 newCenterPoint = { 160, 105, 0 };

        border->CenterPoint = newCenterPoint;
        border->Scale = newScale;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap with Scale set");
    TestServices::InputHelper->Tap(wf::Point(269, 166));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        float newRotationAngleInDegrees = 45;

        border->CenterPoint = { 0, 0, 0 };
        border->Rotation = newRotationAngleInDegrees;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap with Rotation set");
    TestServices::InputHelper->Tap(wf::Point(130, 145));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        // Set Skew in Matrix
        wfn_::float4x4 newMatrix4x4 =
        {
            1.0f, // M11
            0.57735f, // M12
            0.0f, // M13
            0.0f, // M14
            1.0f, // M21
            1.0f, // M22
            0.0f, // M23
            0.0f, // M24
            0.0f, // M31
            0.0f, // M32
            1.0f, // M33
            0.0f, // M34
            0.0f, // M41
            0.0f, // M42
            0.0f, // M43
            1.0f  // M44
        };

        border->TransformMatrix = newMatrix4x4;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap with TransformMatrix set");
    TestServices::InputHelper->Tap(wf::Point(130, 202));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestFacadesScaleRotate()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-HitTest.xaml"));
    Border^ border = nullptr;
    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

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

        border->Width = 50;
        border->Height = 50;
        Canvas::SetLeft(border, 200);
        Canvas::SetTop(border, 200);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Setting Scale and Rotation");
    RunOnUIThread([&]()
    {
        border->Rotation = 90;
        border->Scale = { 2.0, 1.0, 1 };
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Simulate Tap to detect if hit testing works after Scale/Rotation is changed.");
    TestServices::InputHelper->Tap(wf::Point(175, 225));
    pointerPressedEvent->WaitForDefault();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisual2Internal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-HitTest2.xaml"));
    Border^ border = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;
    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.ContainerVisual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });
    TestServices::WindowHelper->WaitForIdle();

    // At this point the Border has a XAML layout offset of 100,100
    LOG_OUTPUT(L"Tapping (1)");
    TestServices::InputHelper->Tap(wf::Point(150, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        auto transform = ref new xaml_media::TranslateTransform();
        transform->X = 100;
        transform->Y = 100;
        border->RenderTransform = transform;
    });
    TestServices::WindowHelper->WaitForIdle();

    // At this point the Border has a XAML layout offset of 100,100 and a XAML TranslateTransform of 100,100 for a total of 200,200
    LOG_OUTPUT(L"Tapping (2)");
    TestServices::InputHelper->Tap(wf::Point(250, 250));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        border->RenderTransform = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();

    // At this point the Border has a XAML layout offset of 100,100
    LOG_OUTPUT(L"Tapping (3)");
    TestServices::InputHelper->Tap(wf::Point(150, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wfn_::float4x4 newMatrix4x4 =
        {
            1.0f, // M11
            0.0f, // M12
            0.0f, // M13
            0.0f, // M14
            0.0f, // M21
            1.0f, // M22
            0.0f, // M23
            0.0f, // M24
            0.0f, // M31
            0.0f, // M32
            1.0f, // M33
            0.0f, // M34
            100.0f, // M41  (translateX)
            100.0f, // M42  (translateY)
            0.0f, // M43
            1.0f  // M44
        };
        handOffVisual->TransformMatrix = newMatrix4x4;
    });
    TestServices::WindowHelper->WaitForIdle();

    // At this point the Border has a XAML layout offset of 100,100 and a HandOff Visual TransformMatrix of 100,100 for a total of 200,200
    LOG_OUTPUT(L"Tapping (4)");
    TestServices::InputHelper->Tap(wf::Point(250, 250));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisual3Internal(bool inWUCMode)
{
     TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

     Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HitTest3.xaml"));
     Border^ border = nullptr;
     Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;
     auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
     std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

     RunOnUIThread([&]()
     {
         VERIFY_IS_NOT_NULL(rootCanvas);
         TestServices::WindowHelper->WindowContent = rootCanvas;
     });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Border");
        StackPanel^ sp = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        border = safe_cast<Border^>(sp->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        LOG_OUTPUT(L"Retrieving HandOff visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);

        if (!inWUCMode)
        {
            // In legacy mode XAML doesn't set the HandoffVisual's Size property which makes AnchorPoint have no effect.  Set it and voila it will work.
            wfn_::float2 size = {static_cast<float>(border->Width), static_cast<float>(border->Height)};
            handOffVisual->Size = size;
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting AnchorPoint");
        wfn_::float2 anchor = {0.5f, 0.5f};
        handOffVisual->AnchorPoint = anchor;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        GeneralTransform^ transform = border->TransformToVisual(nullptr);
        wf::Point result = transform->TransformPoint(wf::Point(0, 0));
        LOG_OUTPUT(L"TransformToVisual produced (%f,%f)", result.X, result.Y);
        VERIFY_IS_TRUE(result == wf::Point(175,175));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Offset");
        wfn_::float3 offset = {10, 10, 0};
        handOffVisual->Offset = offset;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        GeneralTransform^ transform = border->TransformToVisual(nullptr);
        wf::Point result = transform->TransformPoint(wf::Point(0, 0));
        LOG_OUTPUT(L"TransformToVisual produced (%f,%f)", result.X, result.Y);
        VERIFY_IS_TRUE(result == wf::Point(185,185));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Offset back to 0,0");
        wfn_::float3 offset = {0, 0, 0};
        handOffVisual->Offset = offset;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting CenterPoint and Rotation");
        wfn_::float3 center = { 25, 25, 0 };
        handOffVisual->CenterPoint = center;
        handOffVisual->RotationAngleInDegrees = 90;
    });

    TestServices::WindowHelper->WaitForIdle();
    // WaitForIdle is not sufficient here as a wait for
    // the visuals to get updated.
    // Adding sleep as a workaround.
    Sleep(150);

    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        GeneralTransform^ transform = border->TransformToVisual(nullptr);
        wf::Point result = transform->TransformPoint(wf::Point(0, 0));
        LOG_OUTPUT(L"TransformToVisual produced (%f,%f)", result.X, result.Y);
        VERIFY_IS_TRUE(result == wf::Point(275,175));
    });
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualClip()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas;
    Border^ border;
    RectangleGeometry^ clip;
    Visual^ handOffVisual;
    InsetClip^ wucInsetClip;
    ICompositor^ wucCompositor;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;

        border = ref new Border();
        border->Width = 200;
        border->Height = 200;
        border->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
        rootCanvas->Children->Append(border);
    });

    RunOnUIThread([&]()
    {
        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"  > Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        LOG_OUTPUT(L"> Retrieving HandOff visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);

        LOG_OUTPUT(L"> Retrieving Compositor");
        wucCompositor = safe_cast<ICompositionObject^>(handOffVisual)->Compositor;
        VERIFY_IS_NOT_NULL(wucCompositor);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying small size. Clip is still null. We shouldn't be assuming a default inset clip of {0,0,0,0}.");

        wfn_::float2 size = {10.0f, 10.0f};
        handOffVisual->Size = size;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Resetting size. Clip is still null.");

        wfn_::float2 size = {200.0f, 200.0f};
        handOffVisual->Size = size;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying clip");

        clip = ref new RectangleGeometry();
        clip->Rect = Rect(0, 0, 75, 75);
        border->Clip = clip;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying WUC clip to be bigger");
        wucInsetClip = safe_cast<InsetClip^>(handOffVisual->Clip);
        wucInsetClip->RightInset = 20;
        wucInsetClip->BottomInset = 20;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying WUC clip to be smaller");
        wucInsetClip = safe_cast<InsetClip^>(handOffVisual->Clip);
        wucInsetClip->LeftInset = 120;
        wucInsetClip->TopInset = 120;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying the UIElement clip to be large again");

        clip = ref new RectangleGeometry();
        clip->Rect = Rect(50, 50, 100, 100);
        border->Clip = clip;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying WUC clip to be big");
        wucInsetClip = safe_cast<InsetClip^>(handOffVisual->Clip);
        wucInsetClip->LeftInset = 0;
        wucInsetClip->TopInset = 0;
        wucInsetClip->RightInset = 0;
        wucInsetClip->BottomInset = 0;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying the UIElement to be larger, which also resets the app-provided clip because it changes the insets that Xaml wants to set");
        border->Width = 400;
        border->Height = 400;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss (the new center of the element is clipped out)");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying clip transform to Xaml clip, which should have been restored on the element");

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 2;
        clip->Transform = scale;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Changing the clip object entirely - should clear the clip transform as well");
        wucInsetClip = wucCompositor->CreateInsetClip();
        handOffVisual->Clip = wucInsetClip;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Updating the new clip object");
        wucInsetClip->LeftInset = 50;
        wucInsetClip->TopInset = 50;
        wucInsetClip->RightInset = 250;
        wucInsetClip->BottomInset = 250;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss (the new clip clips out the center of the element)");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Changing the clip object entirely, to a new InsetClip that already has insets");

        wucInsetClip = wucCompositor->CreateInsetClip();
        wucInsetClip->LeftInset = 50;
        wucInsetClip->TopInset = 50;
        wucInsetClip->RightInset = 50;
        wucInsetClip->BottomInset = 50;

        handOffVisual->Clip = wucInsetClip;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving the clip a transform, which will clip the center of the element");

        wfn_::float3x2 transform = { 0.4f, 0, 0, 0.4f, 0, 0};
        wucInsetClip->TransformMatrix = transform;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Changing the clip object entirely, to a new InsetClip that already has insets and a clip transform");

        wucInsetClip = wucCompositor->CreateInsetClip();
        wucInsetClip->LeftInset = 50;
        wucInsetClip->TopInset = 50;
        wucInsetClip->RightInset = 250;
        wucInsetClip->BottomInset = 250;
        wfn_::float3x2 transform = { 2, 0, 0, 2, 0, 0};
        wucInsetClip->TransformMatrix = transform;

        handOffVisual->Clip = wucInsetClip;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing the clip object entirely.");

        handOffVisual->Clip = nullptr;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualClipWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestHandOffVisualClip();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualClipWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestHandOffVisualClip();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualTransform3D()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& ih = TestServices::InputHelper;

    Canvas^ rootCanvas;
    Border^ hitTestBorder;
    Border^ border;
    Visual^ handOffVisual;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        PerspectiveTransform3D^ t3d = ref new PerspectiveTransform3D();

        rootCanvas = ref new Canvas();
        rootCanvas->Transform3D = t3d;
        wh->WindowContent = rootCanvas;

        hitTestBorder = ref new Border();   // This element doesn't move around and is used to inject clicks
        hitTestBorder->Width = 200;
        hitTestBorder->Height = 200;
        hitTestBorder->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0));
        rootCanvas->Children->Append(hitTestBorder);

        border = ref new Border();
        border->Width = 200;
        border->Height = 200;
        border->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
        // Give it a comp node so it creates a WUC visual and updates all properties before we attach a listener.
        border->CompositeMode = ElementCompositeMode::SourceOver;
        rootCanvas->Children->Append(border);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"  > Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        LOG_OUTPUT(L"> Applying Transform3D with a Z component");
        CompositeTransform3D^ t3d = ref new CompositeTransform3D();
        t3d->TranslateZ = 1;    // WUC will notify us with a 3D transform matrix because of the listener that we attached this frame
        border->Transform3D = t3d;

        LOG_OUTPUT(L"> Retrieving HandOff visual"); // which attaches a listener
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit - the WUC transform is 3D, don't crash on a null 2D WUC transform");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying Transform3D");

        CompositeTransform3D^ t3d = ref new CompositeTransform3D();
        t3d->RotationY = 60;
        t3d->TranslateX = -50;
        t3d->TranslateZ = 1;    // Prevents dropping down to 2D mode
        border->Transform3D = t3d;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(-60 <= globalBounds.X && globalBounds.X <= -50);
        VERIFY_IS_TRUE(-40 <= globalBounds.Y && globalBounds.Y <= -30);
        VERIFY_IS_TRUE(60 <= globalBounds.Width && globalBounds.Width <= 70);
        VERIFY_IS_TRUE(240 <= globalBounds.Height && globalBounds.Height <= 250);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying WUC transform matrix to unrotate");
        wfn_::float4x4 matrix = handOffVisual->TransformMatrix;
        matrix.m11 = 1;
        matrix.m13 = 0;
        matrix.m31 = 0;
        matrix.m33 = 1;
        // TranslateX is still -50 from Xaml's transform 3D
        handOffVisual->TransformMatrix = matrix;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(-60 <= globalBounds.X && globalBounds.X <= -50);
        VERIFY_IS_TRUE(-10 <= globalBounds.Y && globalBounds.Y <= 0);
        VERIFY_IS_TRUE(200 <= globalBounds.Width && globalBounds.Width <= 210);
        VERIFY_IS_TRUE(200 <= globalBounds.Height && globalBounds.Height <= 210);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Switching to 2D in Xaml");

        border->Transform3D = nullptr;

        TranslateTransform^ tt = ref new TranslateTransform();
        tt->X = -75;
        border->RenderTransform = tt;

        // -75 is expected to overwrite the -50 from the 3D transform. If they stack then we get -125, which will cause a miss.
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        // There's no more 3D perspective. These are exact.
        VERIFY_IS_TRUE(-75 == globalBounds.X);
        VERIFY_IS_TRUE(0 == globalBounds.Y);
        VERIFY_IS_TRUE(200 == globalBounds.Width);
        VERIFY_IS_TRUE(200 == globalBounds.Height);
    });

    CompositeTransform3D^ t3d2d;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying Xaml Transform3D that's actually only 2D");

        t3d2d = ref new CompositeTransform3D();
        border->Transform3D = t3d2d;

        LOG_OUTPUT(L"> Hit test right away, before Xaml gets the property update from DComp. Don't crash.");
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        // Just after adding a Transform3D, a 3D stash is created from the current values in the
        // CompositeTransform3D's underlying matrix.  This case is interesting as we have not yet updated the matrix.
        // The bug was that the matrix was not initialized at all so we were copying garbage values into the stash.
        // With the fix we'll copy the identity matrix into the stash.
        // Since the 3D transform is currently identity, the overall transform has not changed and we can validate it now.
        VERIFY_IS_TRUE(-75 == globalBounds.X);
        VERIFY_IS_TRUE(0 == globalBounds.Y);
        VERIFY_IS_TRUE(200 == globalBounds.Width);
        VERIFY_IS_TRUE(200 == globalBounds.Height);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // After updating the visual tree, our stashes should be unchanged.  Validate again.
        LOG_OUTPUT(L"Validate bounds after updating visual tree");
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);
        VERIFY_IS_TRUE(-75 == globalBounds.X);
        VERIFY_IS_TRUE(0 == globalBounds.Y);
        VERIFY_IS_TRUE(200 == globalBounds.Width);
        VERIFY_IS_TRUE(200 == globalBounds.Height);

        // Now switch to the 3D transform which has no actual 3D in it.
        LOG_OUTPUT(L"Remove RenderTransform and update Transform3D with translate");
        border->RenderTransform = nullptr;
        t3d2d->TranslateX = 50;  // Which moves the element off to the right, not enough to miss the hit test
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(1);   // Help ensure we hear back from DWM and update the stash

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        // There's no 3D. These are exact.
        VERIFY_IS_TRUE(50 == globalBounds.X);
        VERIFY_IS_TRUE(0 == globalBounds.Y);
        VERIFY_IS_TRUE(200 == globalBounds.Width);
        VERIFY_IS_TRUE(200 == globalBounds.Height);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Modifying WUC transform matrix to be RotationY of 60 degrees + TranslateX of -50 + TranslateZ of 1 again");
        wfn_::float4x4 matrix = handOffVisual->TransformMatrix;
        matrix.m11 = 0.5f;
        matrix.m13 = 0.866f;
        matrix.m31 = -0.866f;
        matrix.m33 = 0.5f;
        matrix.m41 = -50;
        matrix.m43 = 1;
        handOffVisual->TransformMatrix = matrix;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(-60 <= globalBounds.X && globalBounds.X <= -50);
        VERIFY_IS_TRUE(-40 <= globalBounds.Y && globalBounds.Y <= -30);
        VERIFY_IS_TRUE(60 <= globalBounds.Width && globalBounds.Width <= 70);
        VERIFY_IS_TRUE(240 <= globalBounds.Height && globalBounds.Height <= 250);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying Xaml Transform3D to move element to the right");

        border->RenderTransform = nullptr;

        CompositeTransform3D^ t3d = ref new CompositeTransform3D();
        t3d->TranslateX = 200;  // Which moves the element off to the right, enough to miss the hit test
        t3d->TranslateZ = 1;    // Prevents dropping down to 2D mode
        border->Transform3D = t3d;
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(3);   // Help ensure we hear back from DWM and update the stash

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(199 <= globalBounds.X && globalBounds.X <= 201);
        VERIFY_IS_TRUE(-1 <= globalBounds.Y && globalBounds.Y <= 1);
        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Switching to 2D (identity) in WUC");
        wfn_::float4x4 matrix = wfn_::float4x4::identity();
        handOffVisual->TransformMatrix = matrix;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        // There's no more 3D perspective. These are exact.
        VERIFY_IS_TRUE(globalBounds.X == 0);
        VERIFY_IS_TRUE(globalBounds.Y == 0);
        VERIFY_IS_TRUE(globalBounds.Width == 200);
        VERIFY_IS_TRUE(globalBounds.Height == 200);
    });
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualTransform3DWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestHandOffVisualTransform3D();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualTranslateWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas;
    Border^ border;
    Visual^ handOffVisual;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating elements");
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;

        border = ref new Border();
        border->Width = 100;
        border->Height = 100;
        border->Background = ref new SolidColorBrush(mu::Colors::Green);
        rootCanvas->Children->Append(border);

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"  > Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        LOG_OUTPUT(L"Requesting HandOff visual and enabling Translation");
        handOffVisual = ElementCompositionPreview::GetElementVisual(border);
        ElementCompositionPreview::SetIsTranslationEnabled(border, true);

    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Applying Translation");
        handOffVisual->Properties->InsertVector3(L"Translation", {100, 100, 0});
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);   // DComp property change callbacks are async

    LOG_OUTPUT(L"Tapping (1)");
    TestServices::InputHelper->Tap(wf::Point(150, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Applying Offset in addition to Translate");
        handOffVisual->Offset = {100, 100, 0};
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);   // DComp property change callbacks are async

    LOG_OUTPUT(L"Tapping (2)");
    TestServices::InputHelper->Tap(wf::Point(250, 250));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(border, false);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tapping (3)");
    TestServices::InputHelper->Tap(wf::Point(150, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
}

// Verifies TransformToVisual works properly on element with Projection and HandOff visual
// This is a regression test
void XamlWinRTCompInteropUnrestrictedTests::Projection1Internal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"Projection1.xaml"));
    Border^ border = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        LOG_OUTPUT(L"Retrieving HandOff visual.");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // The Border has a Projection that has nothing actually set on it, which should contribute nothing to the point we transform.
        // So we expect the Border with layout offset of 100,100 to transform up to 100,100.
        //
        // Regression test notes:
        // The bug happens in WUC mode.  We accidentally use the WUC TransformVisual in GetLocalTransform.  This transform has
        // the projection matrix in it which GetLocalTransform cannot deal with correctly and thus TransformToVisual produces a
        // bogus transform.  This test verifies we end up with 100,100 instead of a wacky value.

        GeneralTransform^ transform = border->TransformToVisual(nullptr);
        wf::Point result = transform->TransformPoint(wf::Point(0, 0));
        LOG_OUTPUT(L"TransformToVisual produced (%f,%f)", result.X, result.Y);
        VERIFY_IS_TRUE(result == wf::Point(100,100));
    });
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualProjection()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& ih = TestServices::InputHelper;

    Canvas^ rootCanvas;
    Border^ hitTestBorder;
    Border^ border;
    Visual^ handOffVisual;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;

        hitTestBorder = ref new Border();   // This element doesn't move around and is used to inject clicks
        hitTestBorder->Width = 200;
        hitTestBorder->Height = 200;
        hitTestBorder->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0));
        rootCanvas->Children->Append(hitTestBorder);

        border = ref new Border();
        border->Width = 200;
        border->Height = 200;
        border->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));
        // Give it a comp node so it creates a WUC visual and updates all properties before we attach a listener.
        border->CompositeMode = ElementCompositeMode::SourceOver;
        rootCanvas->Children->Append(border);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"  > Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        LOG_OUTPUT(L"> Retrieving HandOff visual"); // which attaches a listener
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying Projection with a Z component");

        PlaneProjection^ p = ref new PlaneProjection();
        p->GlobalOffsetX = 250;
        p->GlobalOffsetY = 350;
        p->GlobalOffsetZ = 1;
        border->Projection = p;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect miss");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(249 <= globalBounds.X && globalBounds.X <= 251);
        VERIFY_IS_TRUE(349 <= globalBounds.Y && globalBounds.Y <= 351);
        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
    });

// This doesn't work - we no longer hit test correctly after Projection is stomped by WUC transform.
// We need to use the hand off visual hit testing stash and revert to 3D hit testing.
//
//    RunOnUIThread([&]()
//    {
//        LOG_OUTPUT(L"> Modifying WUC transform matrix to untranslate");
//        wfn_::float4x4 matrix = handOffVisual->TransformMatrix;
//        matrix.m41 = 0;
//        matrix.m42 = 0;
//        handOffVisual->TransformMatrix = matrix;
//    });
//    wh->WaitForIdle();
//
//    LOG_OUTPUT(L"> Clicking - expect hit");
//    ih->Tap(hitTestBorder);
//    pointerPressedEvent->WaitForDefault();
//    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
//    pointerPressedEvent->Reset();
//
//    RunOnUIThread([&]()
//    {
//        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);
//
//        VERIFY_IS_TRUE(-1 <= globalBounds.X && globalBounds.X <= 1);
//        VERIFY_IS_TRUE(-1 <= globalBounds.Y && globalBounds.Y <= 1);
//        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
//        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
//    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying Xaml Projection with no Z component");
        // But this still applies some depth. We only special case identity projections.

        PlaneProjection^ p = ref new PlaneProjection();
        p->GlobalOffsetX = 75;
        p->GlobalOffsetY = 75;
        border->Projection = p;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    ih->Tap(hitTestBorder);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(74 <= globalBounds.X && globalBounds.X <= 76);
        VERIFY_IS_TRUE(74 <= globalBounds.Y && globalBounds.Y <= 76);
        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
    });

// This doesn't work - we no longer hit test correctly after Projection is stomped by WUC transform.
// We need to use the hand off visual hit testing stash and revert to 3D hit testing.
//
//    RunOnUIThread([&]()
//    {
//        LOG_OUTPUT(L"> Switching to 3D (identity) in WUC");
//        wfn_::float4x4 matrix = wfn_::float4x4::identity();
//        // -125 should overwrite the 75 from the Projection. -125 is a miss.
//        // If they get mistakenly combined, we'll end up with -50 which is a hit.
//        matrix.m41 = -125;
//        matrix.m42 = -125;
//        matrix.m43 = 1;
//        handOffVisual->TransformMatrix = matrix;
//    });
//    wh->WaitForIdle();
//
//    LOG_OUTPUT(L"> Clicking - expect miss");
//    ih->Tap(hitTestBorder);
//    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
//    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
//    pointerPressedEvent->Reset();
//
//    RunOnUIThread([&]()
//    {
//        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);
//
//        VERIFY_IS_TRUE(-201 <= globalBounds.X && globalBounds.X <= -199);
//        VERIFY_IS_TRUE(-201 <= globalBounds.Y && globalBounds.Y <= -199);
//        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
//        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
//    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Applying Projection to move element to the right");

        PlaneProjection^ p = ref new PlaneProjection();
        p->GlobalOffsetX = 200;
        border->Projection = p;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);

        VERIFY_IS_TRUE(199 <= globalBounds.X && globalBounds.X <= 201);
        VERIFY_IS_TRUE(-1 <= globalBounds.Y && globalBounds.Y <= 1);
        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Updating Projection");

        PlaneProjection^ p = ref new PlaneProjection();
        p->GlobalOffsetX = 150;
        border->Projection = p;

// This doesn't work. We update right away.
//        LOG_OUTPUT(L"> Get bounds before we render. The bounds should not have updated.");

//        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);
//        VERIFY_IS_TRUE(199 <= globalBounds.X && globalBounds.X <= 201);
//        VERIFY_IS_TRUE(-1 <= globalBounds.Y && globalBounds.Y <= 1);
//        VERIFY_IS_TRUE(199 <= globalBounds.Width && globalBounds.Width <= 201);
//        VERIFY_IS_TRUE(199 <= globalBounds.Height && globalBounds.Height <= 201);
    });
    wh->WaitForIdle();

// This doesn't work - we no longer hit test correctly after Projection is stomped by WUC transform.
// We need to use the hand off visual hit testing stash and revert to 3D hit testing.
//
//    RunOnUIThread([&]()
//    {
//        LOG_OUTPUT(L"> Switching to 2D (identity) in WUC");
//        wfn_::float4x4 matrix = wfn_::float4x4::identity();
//        handOffVisual->TransformMatrix = matrix;
//    });
//    wh->WaitForIdle();
//
//    LOG_OUTPUT(L"> Clicking - expect hit");
//    ih->Tap(hitTestBorder);
//    pointerPressedEvent->WaitForDefault();
//    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
//    pointerPressedEvent->Reset();
//
//    RunOnUIThread([&]()
//    {
//        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(border, false);
//
//        // There's no more 3D perspective. These are exact.
//        VERIFY_IS_TRUE(globalBounds.X == 0);
//        VERIFY_IS_TRUE(globalBounds.Y == 0);
//        VERIFY_IS_TRUE(globalBounds.Width == 200);
//        VERIFY_IS_TRUE(globalBounds.Height == 200);
//    });

    // update projection property, get bounds immediately - bounds should not update
}

namespace
{
    ref class TestPanel sealed : public Microsoft::UI::Xaml::Controls::Panel
    {
    public:
        void SetArrangeOverridePosition(::Windows::Foundation::Point overridePosition)
        {
            m_overridePosition = overridePosition;
            InvalidateArrange();
        }

    protected:
        // Microsoft::UI::Xaml::IFrameworkElementOverrides overrides
        ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override
        {
            Microsoft::UI::Xaml::UIElement^ uie = Children->GetAt(0);
            uie->Measure(availableSize);

            return uie->DesiredSize;
        }

        ::Windows::Foundation::Size ArrangeOverride(::Windows::Foundation::Size finalSize) override
        {
            Microsoft::UI::Xaml::UIElement^ uie = Children->GetAt(0);
            uie->Arrange(::Windows::Foundation::Rect(m_overridePosition.X, m_overridePosition.Y, uie->DesiredSize.Width, uie->DesiredSize.Height));

            GeneralTransform^ transform = uie->TransformToVisual(this);
            wf::Point result = transform->TransformPoint(wf::Point(0, 0));
            LOG_OUTPUT(L"TransformToVisual produced (%f,%f)", result.X, result.Y);
            LOG_OUTPUT(L"Expecting (%f,%f)", m_overridePosition.X, m_overridePosition.Y);
            VERIFY_IS_TRUE(result == wf::Point(m_overridePosition.X, m_overridePosition.Y));

            return finalSize;
        }

    private:
        ::Windows::Foundation::Point m_overridePosition;
    };
}

void XamlWinRTCompInteropUnrestrictedTests::CustomLayout()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);


    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ rootCanvas;
    TestPanel^ testPanel;
    xaml_shapes::Rectangle^ rectangle;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        handOffVisual = ElementCompositionPreview::GetElementVisual(rectangle);

        rootCanvas = ref new Canvas();
        testPanel = ref new TestPanel();

        testPanel->Children->Append(rectangle);
        rootCanvas->Children->Append(testPanel);
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    // Tick a couple more times to guarantee we're listening to HandOff visual transform
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        testPanel->SetArrangeOverridePosition(::Windows::Foundation::Point(200, 200));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

//void XamlWinRTCompInteropUnrestrictedTests::HandOffClosedByApp()
//{
//    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
//    HandOffClosedByAppInternal();
//}


void XamlWinRTCompInteropUnrestrictedTests::BasicGetElementVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    BasicGetElementVisualInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualAfterLeavingTreeWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    GetElementVisualAfterLeavingTreeInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithNullParentCheckWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    GetElementVisualWithNullParentCheckInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithNullContainerCheckWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    GetElementVisualWithNullContainerCheckInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithDeviceLossWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    GetElementVisualWithDeviceLossInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::GetElementVisualWithCommitRequestWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    GetElementVisualWithCommitRequestInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::ResetNullElementChildVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ResetNullElementChildVisualInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForBorderWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetElementChildVisualForBorderInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForUnparentedElementWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetElementChildVisualForUnparentedElementInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetElementChildVisualForPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetElementChildVisualForComplexPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetElementChildVisualForComplexPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::GetElementChildVisualAfterLeavingTreeWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    GetElementChildVisualAfterLeavingTreeInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::ResetElementChildVisualForPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ResetElementChildVisualForPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::ResetElementChildVisualForComplexPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    ResetElementChildVisualForComplexPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetChildAndAddChildToPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetChildAndAddChildToPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetChildAndAddChildToComplexPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetChildAndAddChildToComplexPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetChildAndRemoveChildrenFromPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetChildAndRemoveChildrenFromPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetChildAndRemoveChildrenFromComplexPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetChildAndRemoveChildrenFromComplexPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetChildAndClearChildrenFromPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetChildAndClearChildrenFromPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::SetChildAndClearChildrenFromComplexPanelWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    SetChildAndClearChildrenFromComplexPanelInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisualWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestHandOffVisualInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisual2WUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestHandOffVisual2Internal();
}

void XamlWinRTCompInteropUnrestrictedTests::HitTestHandOffVisual3WUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestHandOffVisual3Internal(true /*inWUCMode*/);
}

void XamlWinRTCompInteropUnrestrictedTests::Projection1WUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    Projection1Internal();
}

void XamlWinRTCompInteropUnrestrictedTests::RenderCompScaledElementWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border;
    Microsoft::UI::Composition::Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Retrieving Windows.UI.Composition.ContainerVisual instance for Border element.]");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        // Force the border to use an alpha mask.
        border->CornerRadius = CornerRadiusHelper::FromUniformRadius(100);

        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
        VERIFY_IS_NOT_NULL(handOffVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"Before");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Scaling hand off visual, letting DComp notify us]");
        wfn_::float3 scale = { 0.25, 0.25, 0 };
        handOffVisual->Scale = scale;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Re-rendering the border]");
        border->CornerRadius = CornerRadiusHelper::FromUniformRadius(101);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"CompScaled");
}

void XamlWinRTCompInteropUnrestrictedTests::CompAnimationOnLoadWUC()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border;
    Microsoft::UI::Composition::Visual^ handOffVisual;

    auto loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(Border, Loaded);
    Microsoft::UI::Composition::ExpressionAnimation^ wucOpacityExpression;
    Microsoft::UI::Composition::ExpressionAnimation^ wucOffsetExpression;
    Microsoft::UI::Composition::ExpressionAnimation^ wucClipExpression;
    Microsoft::UI::Composition::ExpressionAnimation^ wucTransformMatrixExpression;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Retrieving Windows.UI.Composition.ContainerVisual instance for Border element.]");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        loadedRegistration.Attach(
            border,
            ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^)
        {
            LOG_OUTPUT(L"[Border loaded, getting handoff visual]");

            handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));
            VERIFY_IS_NOT_NULL(handOffVisual);

            handOffVisual->Opacity = 0.5f;

            wfn_::float3 offset = { 1, 2, 3 };
            handOffVisual->Offset = offset;

            wfn_::float4x4 transformMatrix = { 11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34, 41, 42, 43, 44 };
            handOffVisual->TransformMatrix = transformMatrix;

            Microsoft::UI::Composition::InsetClip^ insetClip = handOffVisual->Compositor->CreateInsetClip(10, 20, 30, 40);
            handOffVisual->Clip = safe_cast<Microsoft::UI::Composition::CompositionClip^>(insetClip);

            wfn_::float3x2 clipTransformMatrix = {110, 120, 210, 220, 310, 320};
            safe_cast<Microsoft::UI::Composition::CompositionClip^>(insetClip)->TransformMatrix = clipTransformMatrix;

            handOffVisual->CompositeMode = CompositionCompositeMode::DestinationInvert;

            loadedEvent->Set();
        }));

        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();
    loadedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Verifying all initial WUC properties preserved]");
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        float opacity = handOffVisual->Opacity;
        VERIFY_ARE_EQUAL(0.5f, opacity);

        wfn_::float3 offset = handOffVisual->Offset;
        // Xaml layout would have stomped over the values that the app set. Set them again and they should stick.
        VERIFY_ARE_EQUAL(9, offset.x);
        VERIFY_ARE_EQUAL(11, offset.y);
        VERIFY_ARE_EQUAL(0, offset.z);

        offset = { 1, 2, 3 };
        handOffVisual->Offset = offset;

        wfn_::float4x4 transformMatrix = handOffVisual->TransformMatrix;
        VERIFY_ARE_EQUAL(11, transformMatrix.m11);
        VERIFY_ARE_EQUAL(12, transformMatrix.m12);
        VERIFY_ARE_EQUAL(13, transformMatrix.m13);
        VERIFY_ARE_EQUAL(14, transformMatrix.m14);
        VERIFY_ARE_EQUAL(21, transformMatrix.m21);
        VERIFY_ARE_EQUAL(22, transformMatrix.m22);
        VERIFY_ARE_EQUAL(23, transformMatrix.m23);
        VERIFY_ARE_EQUAL(24, transformMatrix.m24);
        VERIFY_ARE_EQUAL(31, transformMatrix.m31);
        VERIFY_ARE_EQUAL(32, transformMatrix.m32);
        VERIFY_ARE_EQUAL(33, transformMatrix.m33);
        VERIFY_ARE_EQUAL(34, transformMatrix.m34);
        VERIFY_ARE_EQUAL(41, transformMatrix.m41);
        VERIFY_ARE_EQUAL(42, transformMatrix.m42);
        VERIFY_ARE_EQUAL(43, transformMatrix.m43);
        VERIFY_ARE_EQUAL(44, transformMatrix.m44);

        auto insetClip = safe_cast<Microsoft::UI::Composition::InsetClip^>(handOffVisual->Clip);
        VERIFY_ARE_EQUAL(10, insetClip->LeftInset);
        VERIFY_ARE_EQUAL(20, insetClip->TopInset);
        VERIFY_ARE_EQUAL(30, insetClip->RightInset);
        VERIFY_ARE_EQUAL(40, insetClip->BottomInset);

        auto clipTransformMatrix = safe_cast<Microsoft::UI::Composition::CompositionClip^>(handOffVisual->Clip)->TransformMatrix;
        VERIFY_ARE_EQUAL(110, clipTransformMatrix.m11);
        VERIFY_ARE_EQUAL(120, clipTransformMatrix.m12);
        VERIFY_ARE_EQUAL(210, clipTransformMatrix.m21);
        VERIFY_ARE_EQUAL(220, clipTransformMatrix.m22);
        VERIFY_ARE_EQUAL(310, clipTransformMatrix.m31);
        VERIFY_ARE_EQUAL(320, clipTransformMatrix.m32);

        CompositionCompositeMode compositeMode = handOffVisual->CompositeMode;
        VERIFY_ARE_EQUAL(CompositionCompositeMode::DestinationInvert, compositeMode);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 offset = handOffVisual->Offset;
        VERIFY_ARE_EQUAL(1, offset.x);
        VERIFY_ARE_EQUAL(2, offset.y);
        VERIFY_ARE_EQUAL(3, offset.z);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Setting properties on Border. This should stomp all over the properties set by the app.]");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        border->Opacity = 0.25f;

        Canvas::SetLeft(border, 1234);

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        border->RenderTransform = scale;

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = wf::Rect(-10, -20, 200, 150);  // width 200, height 150
        border->Clip = clip;

        border->CompositeMode = ElementCompositeMode::MinBlend;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Verifying all initial WUC properties overwritten with Xaml properties]");
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        float opacity = handOffVisual->Opacity;
        VERIFY_ARE_EQUAL(0.25f, opacity);

        wfn_::float3 offset = handOffVisual->Offset;
        VERIFY_ARE_EQUAL(1234, offset.x);
        VERIFY_ARE_EQUAL(11, offset.y);
        VERIFY_ARE_EQUAL(0, offset.z);

        wfn_::float4x4 transformMatrix = handOffVisual->TransformMatrix;
        VERIFY_ARE_EQUAL(2, transformMatrix.m11);
        VERIFY_ARE_EQUAL(0, transformMatrix.m12);
        VERIFY_ARE_EQUAL(0, transformMatrix.m13);
        VERIFY_ARE_EQUAL(0, transformMatrix.m14);
        VERIFY_ARE_EQUAL(0, transformMatrix.m21);
        VERIFY_ARE_EQUAL(1, transformMatrix.m22);
        VERIFY_ARE_EQUAL(0, transformMatrix.m23);
        VERIFY_ARE_EQUAL(0, transformMatrix.m24);
        VERIFY_ARE_EQUAL(0, transformMatrix.m31);
        VERIFY_ARE_EQUAL(0, transformMatrix.m32);
        VERIFY_ARE_EQUAL(1, transformMatrix.m33);
        VERIFY_ARE_EQUAL(0, transformMatrix.m34);
        VERIFY_ARE_EQUAL(0, transformMatrix.m41);
        VERIFY_ARE_EQUAL(0, transformMatrix.m42);
        VERIFY_ARE_EQUAL(0, transformMatrix.m43);
        VERIFY_ARE_EQUAL(1, transformMatrix.m44);

        auto insetClip = safe_cast<Microsoft::UI::Composition::InsetClip^>(handOffVisual->Clip);
        VERIFY_ARE_EQUAL(-10, insetClip->LeftInset);
        VERIFY_ARE_EQUAL(-20, insetClip->TopInset);
        VERIFY_ARE_EQUAL(321 - 200 + 10, insetClip->RightInset);
        VERIFY_ARE_EQUAL(210 - 150 + 20, insetClip->BottomInset);

        auto clipTransformMatrix = safe_cast<Microsoft::UI::Composition::CompositionClip^>(handOffVisual->Clip)->TransformMatrix;
        VERIFY_ARE_EQUAL(1, clipTransformMatrix.m11);
        VERIFY_ARE_EQUAL(0, clipTransformMatrix.m12);
        VERIFY_ARE_EQUAL(0, clipTransformMatrix.m21);
        VERIFY_ARE_EQUAL(1, clipTransformMatrix.m22);
        VERIFY_ARE_EQUAL(0, clipTransformMatrix.m31);
        VERIFY_ARE_EQUAL(0, clipTransformMatrix.m32);

        CompositionCompositeMode compositeMode = handOffVisual->CompositeMode;
        VERIFY_ARE_EQUAL(CompositionCompositeMode::MinBlend, compositeMode);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Starting WUC expressions on handoff visual.]");

        auto compositor = handOffVisual->Compositor;

        wucOpacityExpression = compositor->CreateExpressionAnimation("this.FinalValue");
        handOffVisual->StartAnimation("Opacity", wucOpacityExpression);

        wucOffsetExpression = compositor->CreateExpressionAnimation("   this.FinalValue");
        handOffVisual->StartAnimation("Offset", wucOffsetExpression);

        wucClipExpression = compositor->CreateExpressionAnimation("this.FinalValue   ");
        safe_cast<Microsoft::UI::Composition::CompositionClip^>(handOffVisual->Clip)->StartAnimation("TopInset", wucClipExpression);

        wucTransformMatrixExpression = compositor->CreateExpressionAnimation("   this.FinalValue   ");
        handOffVisual->StartAnimation("TransformMatrix", wucTransformMatrixExpression);

        // No CompositeMode animation
    });

    wh->WaitForIdle();
    LOG_OUTPUT(L"[Verifying expressions via MockDComp.]");
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Expressions");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Setting properties on Border to the same values. This should not stomp all over the expressions set by the app.]");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        border->Opacity = 0.75f;
        border->Opacity = 0.25f;

        Canvas::SetLeft(border, 4321);
        Canvas::SetLeft(border, 1234);

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        border->RenderTransform = scale;

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = wf::Rect(-10, -20, 200, 150);  // width 200, height 150
        border->Clip = clip;

        // No CompositeMode animation
    });

    wh->WaitForIdle();
    LOG_OUTPUT(L"[Verifying expressions via MockDComp.]");
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Expressions");
}

void XamlWinRTCompInteropUnrestrictedTests::HandoffVisualLTEInternal()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-GridView.xaml"));
    GridView^ gridView;
    Microsoft::UI::Composition::Visual^ handOffVisual;
    Microsoft::UI::Composition::Visual^ handInVisual;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Retrieving Windows.UI.Composition.ContainerVisual instance for Rectangle element.]");
        gridView = safe_cast<GridView^>(rootCanvas->FindName(L"gridView"));
        auto r1 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"two"));

        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(r1));
        VERIFY_IS_NOT_NULL(handOffVisual);

        LOG_OUTPUT(L"[Setting child DComp visual via ElementCompositionPreview::SetElementChildVisual.]");

        auto r2 = safe_cast<xaml_shapes::Rectangle^>(rootCanvas->FindName(L"four"));

        handInVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(handInVisual);
        ElementCompositionPreview::SetElementChildVisual(r2, handInVisual);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[Inserting new rectangle into the GridView to trigger portaling. This shouldn't crash.]");

        xaml_shapes::Rectangle^ newChild = ref new xaml_shapes::Rectangle();
        newChild->Width = 50;
        newChild->Height = 50;
        newChild->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        gridView->Items->InsertAt(0, newChild);
    });

    wh->WaitForIdle();
}

void XamlWinRTCompInteropUnrestrictedTests::HandoffVisualLTEWUC()
{
    WUCRenderingScopeGuard wucGuard(DCompRendering::WUCCompleteSynchronousCompTree);
    HandoffVisualLTEInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::HandInClosedByXamlWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HandInClosedByXamlInternal();
}

//void XamlWinRTCompInteropUnrestrictedTests::HandOffClosedByAppWUC()
//{
//    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
//    HandOffClosedByAppInternal();
//}

void XamlWinRTCompInteropUnrestrictedTests::HandInClosedByApp1WUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HandInClosedByApp1Internal();
}

void XamlWinRTCompInteropUnrestrictedTests::HandInClosedByApp2WUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HandInClosedByApp2Internal();
}

void XamlWinRTCompInteropUnrestrictedTests::HandInReusedByAppWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    HandInReusedByAppInternal();
}

void XamlWinRTCompInteropUnrestrictedTests::HandInVisualCausesDManipHitTestVisual()
{
    WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ scrollViewerRoot;
    Visual^ compositionVisual;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating empty tree. There should be no DManip hit test visual.");

        scrollViewerRoot = ref new Canvas();
        scrollViewerRoot->Width = 300;
        scrollViewerRoot->Height = 300;

        // Use an explicit ScrollViewer for this test. The RootScrollViewer isn't guaranteed to have a DManip hit testing
        // visual until a manipulation actually begins.
        ScrollViewer^ scrollViewer = ref new ScrollViewer();
        scrollViewer->Width = 100;
        scrollViewer->Height = 100;
        scrollViewer->Background = nullptr; // Remove default background
        scrollViewer->Content = scrollViewerRoot;

        // Use a Canvas for the actual root of the tree to avoid the RootScrollViewer and to simplify the MockDComp output.
        Canvas^ root = ref new Canvas();
        root->Children->Append(scrollViewer);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Empty");

    RunOnUIThread([&]()
    {
        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"> Setting a hand-in visual. There should now be a DManip hit test visual.");
        ElementCompositionPreview::SetElementChildVisual(scrollViewerRoot, compositionVisual);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"HandIn");
}

void XamlWinRTCompInteropUnrestrictedTests::ImplicitAnimationDisablerWUC()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/, false /*injectMockDComp*/, true/*resetDevice*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    Compositor^ compositor = nullptr;
    Canvas^ rootCanvas = nullptr;
    Border^ border = nullptr;
    Visual^ handoffVisual = nullptr;
    CompositionScopedBatch^ scopedBatch1 = nullptr;
    CompositionScopedBatch^ scopedBatch2 = nullptr;
    CompositionScopedBatch^ scopedBatch3 = nullptr;
    CompositionScopedBatch^ scopedBatch4 = nullptr;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating Border");
        border = ref new Border();
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        border->Width = 100;
        border->Height = 100;
        Canvas::SetLeft(border, 100);
        Canvas::SetTop(border, 100);

        LOG_OUTPUT(L"Getting Handoff Visual");
        handoffVisual = safe_cast<Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(border));

        LOG_OUTPUT(L"Creating Implicit Animations");
        compositor = handoffVisual->Compositor;

        // Implicitly animate the changes we'll be making to Offset during the course of the test
        Vector3KeyFrameAnimation^ offsetAnimation = compositor->CreateVector3KeyFrameAnimation();
        offsetAnimation->InsertExpressionKeyFrame(1.0f, "this.FinalValue");
        ::Windows::Foundation::TimeSpan span = {1000000L}; // 100 ms
        offsetAnimation->Duration = span;
        offsetAnimation->Target = "Offset";

        // Also implicitly animate the Opacity - we'll use this property to detect whether or not the animation actually happened
        ScalarKeyFrameAnimation^ opacityAnimation = compositor->CreateScalarKeyFrameAnimation();
        opacityAnimation->InsertExpressionKeyFrame(1.0f, "this.FinalValue - 0.2f");
        opacityAnimation->Duration = span;
        opacityAnimation->Target = "Opacity";

        // Now add these two implicit animations to a group so they animate together whenever Offset changes
        CompositionAnimationGroup^ animationGroup = compositor->CreateAnimationGroup();
        animationGroup->Add(offsetAnimation);
        animationGroup->Add(opacityAnimation);
        ImplicitAnimationCollection^ iaCollection = compositor->CreateImplicitAnimationCollection();
        iaCollection->Insert("Offset", animationGroup);
        handoffVisual->ImplicitAnimations = iaCollection;

        // Finally add the Border into the live XAML tree.
        // This should not kick off an animation and covers case #1 (when the visual is first created, don't implicitly animate property changes).
        rootCanvas->Children->Append(border);

        // Create a ScopedBatch for this step.  It's expected to fire even though no animation plays (this is by design).
        // When it fires we'll march on to the next step of the test (validation).
        scopedBatch1 = compositor->CreateScopedBatch(CompositionBatchTypes::Animation);
        scopedBatch1->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch 1 complete");
            scopedBatchCompleteEvent->Set();
        });
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scopedBatch1->End();
    });
    scopedBatchCompleteEvent->WaitForDefault();

    // We expect that the implicit animation did not play, leaving the Opacity set to its default of 1.0
    LOG_OUTPUT(L"Current Visual Offset = (%f,%f)", handoffVisual->Offset.x, handoffVisual->Offset.y);
    LOG_OUTPUT(L"Current Visual Opacity = %f", handoffVisual->Opacity);
    VERIFY_IS_TRUE(handoffVisual->Opacity == 1.0f);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Moving Border");
        // Move the Border to (200,200).  This is expected to trigger an implicit animation.
        Canvas::SetLeft(border, 200);
        Canvas::SetTop(border, 200);

        scopedBatch2 = compositor->CreateScopedBatch(CompositionBatchTypes::Animation);
        scopedBatch2->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch 2 complete");
            scopedBatchCompleteEvent->Set();
        });
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scopedBatch2->End();
    });
    scopedBatchCompleteEvent->WaitForDefault();

    // We expect that the implicit animation did play, leaving the Opacity set to 0.8
    LOG_OUTPUT(L"Current Visual Offset = (%f,%f)", handoffVisual->Offset.x, handoffVisual->Offset.y);
    LOG_OUTPUT(L"Current Visual Opacity = %f", handoffVisual->Opacity);
    VERIFY_IS_TRUE(handoffVisual->Opacity == 0.8f);

    RunOnUIThread([&]()
    {
        // Remove the Border from the live XAML tree
        LOG_OUTPUT(L"Taking Border out");
        rootCanvas->Children->Clear();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Add the Border back into the live XAML tree and also change its Offset
        // This is expected to not trigger an implicit animation (case #2, recreating the CompNode for the XAML element)
        LOG_OUTPUT(L"Putting Border back and moving");
        rootCanvas->Children->Append(border);
        Canvas::SetLeft(border, 100);
        Canvas::SetTop(border, 100);

        scopedBatch3 = compositor->CreateScopedBatch(CompositionBatchTypes::Animation);
        scopedBatch3->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch 3 complete");
            scopedBatchCompleteEvent->Set();
        });
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scopedBatch3->End();
    });
    scopedBatchCompleteEvent->WaitForDefault();

    // We expect that the implicit animation did not play, leaving the Opacity set to 0.8
    LOG_OUTPUT(L"Current Visual Offset = (%f,%f)", handoffVisual->Offset.x, handoffVisual->Offset.y);
    LOG_OUTPUT(L"Current Visual Opacity = %f", handoffVisual->Opacity);
    VERIFY_IS_TRUE(handoffVisual->Opacity == 0.8f);

    RunOnUIThread([&]()
    {
        // Move the Border to a different position in the Canvas's Children collection and also change its Offset
        // Ths is expected to not trigger an implicit animation (case #3, moving also re-creates new CompNode)
        LOG_OUTPUT(L"Doing Children Move of Border");
        rootCanvas->Children->Append(ref new Border());
        rootCanvas->Children->Move(0,1);
        Canvas::SetLeft(border, 200);
        Canvas::SetTop(border, 200);

        scopedBatch4 = compositor->CreateScopedBatch(CompositionBatchTypes::Animation);
        scopedBatch4->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch 4 complete");
            scopedBatchCompleteEvent->Set();
        });
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scopedBatch4->End();
    });
    scopedBatchCompleteEvent->WaitForDefault();

    // We expect that the implicit animation did not play, leaving the Opacity set to 0.8
    LOG_OUTPUT(L"Current Visual Offset = (%f,%f)", handoffVisual->Offset.x, handoffVisual->Offset.y);
    LOG_OUTPUT(L"Current Visual Opacity = %f", handoffVisual->Opacity);
    VERIFY_IS_TRUE(handoffVisual->Opacity == 0.8f);
}

void XamlWinRTCompInteropUnrestrictedTests::HandInClosedByXamlInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual to set child.");
        ElementCompositionPreview::SetElementChildVisual(border, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Clear RootCanvas children (including Border).");
        rootCanvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        HRESULT hr = S_OK;

        LOG_OUTPUT(L"Triggering final realease of Border.");
        border = nullptr;
        float opacityAfterClose = 0;

        LOG_OUTPUT(L"Try to get Opacity property. Xaml should have called IClosable::Close on handin visual in its destructor - expect RO_E_CLOSED.");
        try
        {
            opacityAfterClose = compositionVisual->Opacity;
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }

        VERIFY_ARE_EQUAL(hr, RO_E_CLOSED);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());
}

void XamlWinRTCompInteropUnrestrictedTests::HandOffClosedByAppInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Retrieving Windows.UI.Composition.Visual instance for Border element.");
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = ElementCompositionPreview::GetElementVisual(border);
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Reading WinRT Visual.Opacity property value.");
        float opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's default Opacity=%f.", opacity);
        compositionVisual->Opacity = 0.25;
        opacity = compositionVisual->Opacity;
        LOG_OUTPUT(L"Windows.UI.Composition.Visual's new Opacity=%f.", opacity);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        HRESULT hr = S_OK;

        LOG_OUTPUT(L"Discarding Visual for Border element.");
        rootCanvas->Children->Clear();

        try
        {
            delete compositionVisual;
            compositionVisual = nullptr;
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }
        VERIFY_FAILED(hr);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());
}

void XamlWinRTCompInteropUnrestrictedTests::HandInClosedByApp1Internal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(border);

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual to set child.");
        ElementCompositionPreview::SetElementChildVisual(border, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {
        // C++/CX expliciit Dispose() call doesn't compile, apparently by design:
        // error C2039: 'Dispose': is not a member of 'Platform::IDisposable'
        // note: You should invoke the destructor, '~IDisposable' instead
        //
        // Platform::IDisposable^ visualAsDisposable = safe_cast<Platform::IDisposable^>(compositionVisual1);
        // visualAsDisposable->Dispose();
        //
        // It turns out that in C++/CX, "delete" of an IDisposable has the effect of calling Dispose(), and then releasing once.
        // This is somewhat misleading, in that it doesn't imply destruction. So we use delete to safely test disposal here.
        LOG_OUTPUT(L"Delete of child Visual for Border element.");
        rootCanvas->Children->Clear();
        delete compositionVisual;
        compositionVisual = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Releasing UIElement reference, this should cause UIE dtor to be called and comp node to get cleaned up.");
        border = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());
}

void XamlWinRTCompInteropUnrestrictedTests::HandInClosedByApp2Internal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
    Border^ border = nullptr;
    Visual^ compositionVisual1 = nullptr;
    Visual^ compositionVisual2 = nullptr;
    Visual^ compositionVisual3 = nullptr;

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

        compositionVisual1 = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual1);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual1->Opacity = 0.25;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual to set child using handin visual #1.");
        ElementCompositionPreview::SetElementChildVisual(border, compositionVisual1);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Dispose of handin visual #1");

        wrl::ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(compositionVisual1));
        wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
        VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
        VERIFY_SUCCEEDED(spClosable->Close());
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Create and attach handin visual #2, replacing disposed handin visual #1");
        compositionVisual2 = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual2);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual2->Opacity = 0.5;

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual to set child using handin visual #2.");
        ElementCompositionPreview::SetElementChildVisual(border, compositionVisual2);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Create handin visual #3 and immediately dispose of it, then set it as handin");
        compositionVisual3 = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual3);

        LOG_OUTPUT(L"Setting Windows.UI.Composition.Visual's Opacity.");
        compositionVisual3->Opacity = 0.75;

        LOG_OUTPUT(L"Dispose of handin visual #3");
        wrl::ComPtr<IUnknown> spIUnknown (reinterpret_cast<IUnknown*>(compositionVisual3));
        wrl::ComPtr<ABI::Windows::Foundation::IClosable> spClosable;
        VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));
        VERIFY_SUCCEEDED(spClosable->Close());

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual to set child using handin visual #3.");
        ElementCompositionPreview::SetElementChildVisual(border, compositionVisual3);
    });

    TestServices::WindowHelper->WaitForIdle();
}

void XamlWinRTCompInteropUnrestrictedTests::HandInReusedByAppInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 350));

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-PanelUIElementDCompVisual.xaml"));
    StackPanel^ stackPanel = nullptr;
    Visual^ compositionVisual = nullptr;
    xaml_shapes::Rectangle^ rectangle0 = nullptr;
    xaml_shapes::Rectangle^ rectangle1 = nullptr;

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(rootCanvas);
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(rootCanvas->Children->GetAt(0));
        VERIFY_IS_NOT_NULL(stackPanel);

        compositionVisual = CreateVisual();
        VERIFY_IS_NOT_NULL(compositionVisual);

        rectangle0 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(0));
        rectangle1 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(1));
        VERIFY_IS_NOT_NULL(rectangle0);

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for Rectangle0 to set handin.");
        ElementCompositionPreview::SetElementChildVisual(rectangle0, compositionVisual);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

    RunOnUIThread([&]()
    {

        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual(nullptr) for Rectangle0 to clear handin, but let app keep a reference");
        ElementCompositionPreview::SetElementChildVisual(rectangle0, nullptr);

        LOG_OUTPUT(L"Remove Rectangle0 from StackPanel element.");
        stackPanel->Children->RemoveAt(0);

        LOG_OUTPUT(L"Delete app's reference to Rectangle0. It should get destroyed, but handin should not be disposed.");
        delete rectangle0;
    });

    // Tick a few extra times to make sure UIE is destroyed
    TestServices::WindowHelper->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Modify the handin - set opacity to 0.9876");
        compositionVisual->Opacity = 0.9876f;

        LOG_OUTPUT(L"Modify the handin - set color to green.");
        Compositor^ compositor = compositionVisual->Compositor;
        safe_cast<SpriteVisual^>(compositionVisual)->Brush = compositor->CreateColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xFF, 0x00, 0xFF, 0x00));

    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Invoking ElementCompositionPreview::SetElementChildVisual for Rectangle1 to set handin.");
        ElementCompositionPreview::SetElementChildVisual(rectangle1, compositionVisual);

        LOG_OUTPUT(L"Try to get Opacity property. This should succeed. If the visual had been closed, we would fail with exception wrapping hr RO_E_CLOSED.");
        float opacity = compositionVisual->Opacity;
        VERIFY_ARE_EQUAL(opacity, 0.9876f);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"4").GetString());
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCullingWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    ElementCulling_DontCullHandOffVisuals();
    ElementCulling_UncullAfterGettingHandOffVisuals();
    ElementCulling_UncullAfterAddingElementsWithHandOffVisuals();
    ElementCulling_Clip();
    ElementCulling_LayoutClip();
    ElementCulling_Transform();
}

Border^ XamlWinRTCompInteropUnrestrictedTests::ElementCulling_MakeBorder(::Windows::UI::Color color)
{
    const auto& rect1 = ref new xaml_shapes::Rectangle();
    rect1->Width = 10;
    rect1->Height = 10;
    rect1->Fill = ref new SolidColorBrush(color);

    const auto& rect2 = ref new xaml_shapes::Rectangle();
    rect2->Width = 20;
    rect2->Height = 20;
    rect2->Fill = ref new SolidColorBrush(color);

    const auto& rect3 = ref new xaml_shapes::Rectangle();
    rect3->Width = 30;
    rect3->Height = 30;
    rect3->Opacity = 0;
    rect3->Fill = ref new SolidColorBrush(color);

    const auto& grid = ref new Grid();
    grid->Children->Append(rect1);
    grid->Children->Append(rect2);
    grid->Children->Append(rect3);

    const auto& border = ref new Border();
    border->Width = 50;
    border->Height = 50;
    border->Background = ref new SolidColorBrush(color);
    border->Child = grid;
    return border;
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCulling_DontCullHandOffVisuals()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Border^ border1;
    Grid^ grid;

    RunOnUIThread([&]()
    {
        border1 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Green);
        const auto& border2 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Red);
        const auto& border3 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Blue);

        grid = ref new Grid();
        grid->Children->Append(border2);
        grid->Children->Append(border3);

        const auto& root = ref new Canvas();
        root->Children->Append(border1);
        root->Children->Append(grid);
        wh->WindowContent = root;

        // We don't actually need the WUC visuals to stay around
        ElementCompositionPreview::GetElementVisual(border1);
        ElementCompositionPreview::GetElementVisual(border2);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Setting Opacity=0 on element with hand off visuals = don't cull");
        LOG_OUTPUT(L"   Setting Opacity=0 above element with hand off visuals = cull");

        border1->Opacity = 0.0f;    // Doesn't cull border1
        grid->Opacity = 0.0f;       // Culls border2 and border3
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unculled");
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCulling_UncullAfterGettingHandOffVisuals()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Border^ border1;
    Border^ border2;

    RunOnUIThread([&]()
    {
        // Has opacity 0, will get hand off visual
        // Culled initially, unculled when the hand off visual is requested
        border1 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Green);
        border1->Opacity = 0.0f;

        // Has opacity 1 but is inside a grid with opacity 0, will get hand off visual
        // Culled initially, unculled when the hand off visual is requested
        border2 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Red);

        // Has opacity 1 but is inside a grid with opacity 0, has sibling that will get hand off visual
        // Culled initially, stays culled when sibling gets unculled
        const auto& border3 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Blue);

        const auto& grid = ref new Grid();
        grid->Children->Append(border2);
        grid->Children->Append(border3);
        grid->Opacity = 0.0f;

        const auto& root = ref new Canvas();
        root->Children->Append(border1);
        root->Children->Append(grid);
        wh->WindowContent = root;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Culled");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Get hand off visuals on Opacity=0 element = that element becomes unculled. Sibling elements stay culled.");
        LOG_OUTPUT(L"   Get hand off visuals below Opacity=0 element = that element stays culled.");

        // We don't actually need the WUC visuals to stay around
        ElementCompositionPreview::GetElementVisual(border1);
        ElementCompositionPreview::GetElementVisual(border2);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unculled");
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCulling_UncullAfterAddingElementsWithHandOffVisuals()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ root;
    Grid^ grid;

    RunOnUIThread([&]()
    {
        grid = ref new Grid();
        grid->Opacity = 0.0f;

        root = ref new Canvas();
        root->Children->Append(grid);
        wh->WindowContent = root;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Culled");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Add some elements that have hand off visuals = those elements aren't culled.");

        // Has opacity 0, has hand off visual
        // Unculled when added to the tree
        const auto& border1 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Green);
        border1->Opacity = 0.0f;
        ElementCompositionPreview::GetElementVisual(border1);
        root->Children->InsertAt(0, border1);

        // Has opacity 1, has hand off visual
        // Still culled when added to an Opacity=0 grid in the tree
        const auto& border2 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Red);
        ElementCompositionPreview::GetElementVisual(border2);

        const auto& innerGrid = ref new Grid();
        innerGrid->Children->Append(border2);
        grid->Children->Append(innerGrid);

        // Has opacity 1, no hand off visual
        // Culled when added to an Opacity=0 grid in the tree
        const auto& border3 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Blue);
        grid->Children->Append(border3);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unculled");
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCulling_Clip()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Border^ border1;
    Grid^ grid;

    RunOnUIThread([&]()
    {
        border1 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Green);
        const auto& border2 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Red);
        const auto& border3 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Blue);

        grid = ref new Grid();
        grid->Children->Append(border2);
        grid->Children->Append(border3);

        const auto& root = ref new Canvas();
        root->Children->Append(border1);
        root->Children->Append(grid);
        wh->WindowContent = root;

        // We don't actually need the WUC visuals to stay around
        ElementCompositionPreview::GetElementVisual(border1);
        ElementCompositionPreview::GetElementVisual(border2);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Setting empty clip on element with hand off visuals = don't cull");
        LOG_OUTPUT(L"   Setting empty clip above element with hand off visuals = cull");

        border1->Clip = ref new RectangleGeometry();
        grid->Clip = ref new RectangleGeometry();
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unculled-Clip");
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCulling_LayoutClip()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Grid^ outerGrid;

    RunOnUIThread([&]()
    {
        const auto& border1 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Green);
        const auto& border2 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Red);
        const auto& border3 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Blue);

        const auto& innerGrid = ref new Grid();
        innerGrid->Width = 50;
        innerGrid->Height = 50;
        innerGrid->Children->Append(border2);
        innerGrid->Children->Append(border3);

        outerGrid = ref new Grid();
        outerGrid->Children->Append(border1);
        outerGrid->Children->Append(innerGrid);

        const auto& root = ref new Canvas();
        root->Children->Append(outerGrid);
        wh->WindowContent = root;

        // We don't actually need the WUC visuals to stay around
        ElementCompositionPreview::GetElementVisual(border1);
        ElementCompositionPreview::GetElementVisual(border2);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Setting empty layout clip on element with hand off visuals = don't cull");
        LOG_OUTPUT(L"   Setting empty layout clip above element with hand off visuals = cull");

        outerGrid->Width = 0;
        outerGrid->Height = 0;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unculled-LayoutClip");
}

void XamlWinRTCompInteropUnrestrictedTests::ElementCulling_Transform()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Border^ border1;
    Grid^ grid;

    RunOnUIThread([&]()
    {
        border1 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Green);
        const auto& border2 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Red);
        const auto& border3 = ElementCulling_MakeBorder(Microsoft::UI::Colors::Blue);

        grid = ref new Grid();
        grid->Children->Append(border2);
        grid->Children->Append(border3);

        const auto& root = ref new Canvas();
        root->Children->Append(border1);
        root->Children->Append(grid);
        wh->WindowContent = root;

        // We don't actually need the WUC visuals to stay around
        ElementCompositionPreview::GetElementVisual(border1);
        ElementCompositionPreview::GetElementVisual(border2);
    });

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Setting transform with tiny scale on element with hand off visuals = still cull");
        LOG_OUTPUT(L"   Setting transform with tiny scale above element with hand off visuals = still cull");

        const auto& tinyScale = ref new ScaleTransform();
        tinyScale->ScaleX = 0.00000001;
        tinyScale->ScaleY = 0.00000001;

        border1->RenderTransform = tinyScale;
        grid->RenderTransform = tinyScale;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Culled-RenderTransform");
}

void XamlWinRTCompInteropUnrestrictedTests::CompositorTest()
{
    Window^ window = nullptr;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Getting Compositor from UI thread");
        window = Microsoft::UI::Xaml::Window::Current;
        auto compositor = window->Compositor;
        VERIFY_IS_NOT_NULL(compositor);

        auto testCanvas = ref new Canvas();
        auto handOffVisual = ElementCompositionPreview::GetElementVisual(testCanvas);
        auto compositor2 = handOffVisual->Compositor;
        VERIFY_ARE_EQUAL(compositor, compositor2);
    });

    bool expectedFailure = false;
    try
    {
        LOG_OUTPUT(L"Getting Compositor from non UI thread, should fail");
        auto compositor = window->Compositor;
    }
    catch (Platform::Exception^ e)
    {
        VERIFY_ARE_EQUAL(e->HResult, int(RPC_E_WRONG_THREAD));
        expectedFailure = true;
    }

    VERIFY_IS_TRUE(expectedFailure);

}

void XamlWinRTCompInteropUnrestrictedTests::Translation1WUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        const auto& root = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Width = 200;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Children->Append(rect);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Enabling Translation, but not using HandOff visual, should have no effect");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Forcing rect to create a CompNode");
        rect->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing rect's CompNode");
        rect->CompositeMode = ElementCompositeMode::Inherit;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

}

void XamlWinRTCompInteropUnrestrictedTests::Translation2WUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        const auto& root = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Children->Append(rect);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Enabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);

        LOG_OUTPUT(L"Requesting HandOff visual");
        handOffVisual = ElementCompositionPreview::GetElementVisual(rect);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Animating Translation");
        auto compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        auto animation = compositor->CreateExpressionAnimation("vector3(100, 100, 0)");
        handOffVisual->StartAnimation("Translation", animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting static Translation");
        handOffVisual->Properties->InsertVector3(L"Translation", {200, 200, 0});
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"5");
}

void XamlWinRTCompInteropUnrestrictedTests::Translation3WUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        const auto& root = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Children->Append(rect);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff visual");
        handOffVisual = ElementCompositionPreview::GetElementVisual(rect);

        LOG_OUTPUT(L"Enabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void XamlWinRTCompInteropUnrestrictedTests::Translation4WUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        const auto& root = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Children->Append(rect);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff visual");
        handOffVisual = ElementCompositionPreview::GetElementVisual(rect);

        LOG_OUTPUT(L"Enabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);

        LOG_OUTPUT(L"Animating Translation");
        auto compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        auto animation = compositor->CreateExpressionAnimation("vector3(100, 100, 0)");
        handOffVisual->StartAnimation("Translation", animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void XamlWinRTCompInteropUnrestrictedTests::Translation4BWUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ root;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating rect");
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);

        LOG_OUTPUT(L"Enabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);

        LOG_OUTPUT(L"Setting Implicit Show Animation");
        auto compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        auto animation = compositor->CreateExpressionAnimation("vector3(100, 100, 0)");
        animation->Target = "Translation";
        ElementCompositionPreview::SetImplicitShowAnimation(rect, animation);

        LOG_OUTPUT(L"Adding rect to the tree, should trigger implicit animation of Translation");
        root->Children->Append(rect);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void XamlWinRTCompInteropUnrestrictedTests::Translation5WUCFull()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Canvas^ root;
    Visual^ handOffVisual;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 400;
        root->Height = 400;
        root->Clip = ref new xaml_media::RectangleGeometry();
        root->Clip->Rect = {0,0,50,50};

        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Children->Append(rect);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff visual");
        handOffVisual = ElementCompositionPreview::GetElementVisual(rect);

        LOG_OUTPUT(L"Enabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);

        LOG_OUTPUT(L"Animating Translation");
        auto compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        auto animation = compositor->CreateExpressionAnimation("vector3(25, 25, 0)");
        handOffVisual->StartAnimation("Translation", animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing prepend clip");
        root->Clip = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Recreating prepend clip");
        root->Clip = ref new xaml_media::RectangleGeometry();
        root->Clip->Rect = {0,0,50,50};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disabling Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");
}

void XamlWinRTCompInteropUnrestrictedTests::GetHandOffVisualForAnimatingClip()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    ScaleTransform^ scale;
    Grid^ animatingGrid;
    Visual^ compositionVisual;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Static scene");

        scale = ref new ScaleTransform();

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Transform = scale;

        animatingGrid = ref new Grid();
        animatingGrid->Clip = clip;

        const auto& root = ref new Canvas();
        root->Children->Append(animatingGrid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Animate the clip transform");

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 1.0;
        ::Windows::Foundation::TimeSpan span; span.Duration = 100000000L; // 10 seconds
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, scale);
        Storyboard::SetTargetProperty(da, L"ScaleX");

        Storyboard^ sb = ref new Storyboard();
        sb->Children->Append(da);
        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Get the hand off visual. Don't crash.");
        compositionVisual = ElementCompositionPreview::GetElementVisual(animatingGrid);
    });
    VERIFY_IS_NOT_NULL(compositionVisual);
}

} } } } } }
