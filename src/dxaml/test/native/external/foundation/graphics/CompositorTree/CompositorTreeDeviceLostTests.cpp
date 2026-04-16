// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "MUX-ETWEvents.h"
#include "etwwaiterproxy.h"

#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "CompositorTreeDeviceLostTests.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <WUCRenderingScopeGuard.h>
#include <robuffer.h>

using namespace ::Windows::Foundation;
using namespace Platform;

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace ::Windows::Storage::Streams;

using namespace test_infra;
using namespace Concurrency;
using namespace MockDComp;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool CompositorTreeDeviceLostTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool CompositorTreeDeviceLostTests::ClassCleanup()
{
    return true;
}

bool CompositorTreeDeviceLostTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool CompositorTreeDeviceLostTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

Border^ CompositorTreeDeviceLostTests::AddBorderChild(Border^ border)
{
    Border^ child = ref new Border();
    VERIFY_IS_NOT_NULL(child);

    child->Width = border->Width * .75;
    child->Height = border->Height *.75;

    RotateTransform ^childRotateTransform = ref new RotateTransform();
    child->RenderTransform = childRotateTransform;
    childRotateTransform->Angle = 45;

    TextBlock^ textBlock = ref new TextBlock();
    textBlock->Text = L"TextBlock";
    textBlock->Foreground = ref new SolidColorBrush((::Windows::UI::Color)border->Tag);
    child->Child = textBlock;

    //child->Background = border->Background;
    border->Child = child;

    return child;
}

Border^ CompositorTreeDeviceLostTests::CreateBorderElement(int style)
{
    WEX::Common::Throw::IfFalse(style >= 0 && style <= 9, E_INVALIDARG);

    // Create the element to be inserted.  We use various styles so the resulting nodes
    // in the tree can be distinquished from each other so we know the right nodes get
    // added and/or removed.
    Border^ border = ref new Border();
    VERIFY_IS_NOT_NULL(border);

    // Transform ensures we get new visuals
    RotateTransform ^rotateTransform = ref new RotateTransform();
    border->RenderTransform = rotateTransform;
    border->Width = 200;
    border->Height = 25;
    rotateTransform->Angle = 10 + (10 * style);

    ::Windows::UI::Color color = Microsoft::UI::Colors::White;
    switch (style)
    {
    case 0: color = Microsoft::UI::Colors::Red; break;
    case 1: color = Microsoft::UI::Colors::Orange; break;
    case 2: color = Microsoft::UI::Colors::Yellow; break;
    case 3: color = Microsoft::UI::Colors::Green; break;
    case 4: color = Microsoft::UI::Colors::Blue; break;
    case 5: color = Microsoft::UI::Colors::Purple; break;
    case 6: color = Microsoft::UI::Colors::Brown; break;
    case 7: color = Microsoft::UI::Colors::DarkGray; break;
    case 8: color = Microsoft::UI::Colors::LightGray; break;
    case 9: color = Microsoft::UI::Colors::Purple; break;
    }

    TextBlock^ textBlock = ref new TextBlock();
    textBlock->Text = L"TextBlock";
    textBlock->Foreground = ref new SolidColorBrush(color);
    border->Child = textBlock;

    // Remember the color we used in case we need to create a child
    border->Tag = color;

    return border;
}

void CompositorTreeDeviceLostTests::SubmitRenderActions(RENDERACTION actions[], int actionCount, Panel^ panel)
{
    // If there is less than two actions, there is no point in calling the function since its
    // purpose it to separate multiple actions by a device lost state in CompositorTreeHost::ProcessFrame
   WEX::Common::Throw::IfFalse(actionCount > 1, E_INVALIDARG);

    ::Windows::Foundation::EventRegistrationToken renderingEventToken;
    int actionIndex = 0;

    // make sure initialization is complete and rendered
    TestServices::WindowHelper->WaitForIdle();

    ETWWaiterProxy devicelostEtwWaiter(WINDOWS_UI_XAML_ETW_PROVIDER, RecordDeviceAsLostInfo_value);

    RunOnUIThread([&]()
    {
        // Register for the Composition Target Rendering event.  This will get called at the beginning
        // of each tick and allow us to perform a render action, modifying the tree.
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            if (actionIndex >= actionCount) return;

            actions[actionIndex++](panel);

            // Once we have executed the last action, clear the mock function so we get a
            // full render.
            static bool resetdevice = true;
            if (actionIndex >= actionCount)
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            }
            else if (resetdevice)
            {
                LOG_OUTPUT(L"Resetting DComp Graphics Device");
                TestServices::Utilities->ResetMockDCompGraphicsDevice();
            }
        });
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });

    // Wait until we have gotten a device lost.  Note: We currently only check for
    // at least one, which is just fine because we don't have any scenarios where we
    // would expect more than one.  If we end up doing that, we should look at
    // how the event waiters work to see if we can actually get the number that
    // we expect.  The other important aspect here is that we may not have been
    // able to inject MockDComp (e.g. there is currently a bug in OneCore on this).
    // If this is true, then the above Reset call simply noops and thus we will
    // never get the device lost ETW event.  In this case, just start waiting
    // for idle immediately.
    if (!TestServices::Utilities->IsMockDCompDisabled)
    {
        devicelostEtwWaiter.WaitForDefault();
    }

    // Wait until all the actions have been performed and things have settled down.
    LOG_OUTPUT(L"Waiting for render actions to complete");
    while (actionIndex < actionCount)
    {
        TestServices::WindowHelper->WaitForIdle();
    }
    TestServices::WindowHelper->WaitForIdle();
}

//------------------------------------------------------------------------
// Test case:
//  Adds a new node.
//  Device Lost
//  Remove added node
//------------------------------------------------------------------------
void CompositorTreeDeviceLostTests::AddRemoveSameNode()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

    // Create our initial layout
    StackPanel^ rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = ref new StackPanel();
        VERIFY_IS_NOT_NULL(rootPanel);
        LOG_OUTPUT(L"Creating initial layout of two elements");
        rootPanel->Children->Append(CreateBorderElement(0));
        rootPanel->Children->Append(CreateBorderElement(1));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    // Submit our render actions
    RENDERACTION actions[] = {
        [](Panel^ panel)
        {
            LOG_OUTPUT(L"Inserting new element between the initial elements");
            panel->Children->InsertAt(1, CreateBorderElement(8));
        },
            [](Panel^ panel)
        {
            LOG_OUTPUT(L"Removing newly inserted element");
            panel->Children->RemoveAt(1);
        } };

    SubmitRenderActions(actions, ARRAYSIZE(actions), rootPanel);

    // Validate the result
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

//------------------------------------------------------------------------
// Test case:
//  Adds a new node.
//  Device Lost
//  Remove new node's parent
//------------------------------------------------------------------------
void CompositorTreeDeviceLostTests::AddNodeRemoveParent()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

    // Create our initial layout
    StackPanel^ rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = ref new StackPanel();
        VERIFY_IS_NOT_NULL(rootPanel);
        LOG_OUTPUT(L"Creating initial layout of three elements");
        rootPanel->Children->Append(CreateBorderElement(0));
        rootPanel->Children->Append(CreateBorderElement(8));
        rootPanel->Children->Append(CreateBorderElement(1));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    // Submit our render actions
    RENDERACTION actions[] = {
        [](Panel^ panel)
        {
            LOG_OUTPUT(L"Inserting child of the second element");

            Border^ border = safe_cast<Border^>(panel->Children->GetAt(1));
            AddBorderChild(border);
        },
            [](Panel^ panel)
        {
            LOG_OUTPUT(L"Removing second element");
            panel->Children->RemoveAt(1);
        } };

    SubmitRenderActions(actions, ARRAYSIZE(actions), rootPanel);

    // Validate the result
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

//------------------------------------------------------------------------
// Test case:
//  Adds a new node after first child of parent
//  Device Lost
//  Remove new node's previous sibling
//------------------------------------------------------------------------
void CompositorTreeDeviceLostTests::AddNodeRemovePreviousSibling1()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

    StackPanel^ rootPanel;

    // Create our initial layout
    RunOnUIThread([&]()
    {
        rootPanel = ref new StackPanel();
        VERIFY_IS_NOT_NULL(rootPanel);
        LOG_OUTPUT(L"Creating initial layout of three elements");
        rootPanel->Children->Append(CreateBorderElement(8));
        rootPanel->Children->Append(CreateBorderElement(1));
        rootPanel->Children->Append(CreateBorderElement(2));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    // Submit our render actions
    RENDERACTION actions1[] = {
        [](Panel^ panel)
        {
            LOG_OUTPUT(L"Insert new element after first element");
            panel->Children->InsertAt(1, CreateBorderElement(0));
        },
            [](Panel^ panel)
        {
            LOG_OUTPUT(L"Remove the first element");
            panel->Children->RemoveAt(0);
        } };

    SubmitRenderActions(actions1, ARRAYSIZE(actions1), rootPanel);

    // Validate the result
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

}

//------------------------------------------------------------------------
// Test case:
//  Adds a new node after second child of parent
//  Device Lost
//  Remove new node's previous sibling
//------------------------------------------------------------------------
void CompositorTreeDeviceLostTests::AddNodeRemovePreviousSibling2()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

    StackPanel^ rootPanel;

    // Create our initial layout
    RunOnUIThread([&]()
    {
        rootPanel = ref new StackPanel();
        VERIFY_IS_NOT_NULL(rootPanel);
        LOG_OUTPUT(L"Creating initial layout of three elements");
        rootPanel->Children->Append(CreateBorderElement(0));
        rootPanel->Children->Append(CreateBorderElement(8));
        rootPanel->Children->Append(CreateBorderElement(2));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });


    // Submit our render actions
    RENDERACTION actions2[] = {
        [](Panel^ panel)
        {
            LOG_OUTPUT(L"Insert new element after second element");
            panel->Children->InsertAt(2, CreateBorderElement(1));
        },
            [](Panel^ panel)
        {
            LOG_OUTPUT(L"Remove second element");
            panel->Children->RemoveAt(1);
        } };

    SubmitRenderActions(actions2, ARRAYSIZE(actions2), rootPanel);

    // Validate the result
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);


}

//------------------------------------------------------------------------
// Test case:
//  Move a node.
//  Device Lost
//  Remove the node
//------------------------------------------------------------------------
void CompositorTreeDeviceLostTests::MoveThenRemoveNode()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

    // Create our initial layout
    StackPanel^ rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = ref new StackPanel();
        VERIFY_IS_NOT_NULL(rootPanel);
        LOG_OUTPUT(L"Creating initial layout of four elements");
        rootPanel->Children->Append(CreateBorderElement(1));
        rootPanel->Children->Append(CreateBorderElement(8));
        rootPanel->Children->Append(CreateBorderElement(2));
        rootPanel->Children->Append(CreateBorderElement(3));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    // Submit our render actions
    RENDERACTION actions[] = {
        [](Panel^ panel)
        {
            // We need to ensure that we get a device lost before we get to the moved node
            // (since that won't generate one), so insert a new node up front.
            LOG_OUTPUT(L"Insert new element at position 0");
            panel->Children->InsertAt(0, CreateBorderElement(0));

            LOG_OUTPUT(L"Remove the (now) third element");
            Border^ element = safe_cast<xaml_controls::Border^>(panel->Children->GetAt(2));
            panel->Children->RemoveAt(2);

            LOG_OUTPUT(L"Insert the removed element before the final node");
            panel->Children->InsertAt(3, element);
        },
        [](Panel^ panel)
        {
            LOG_OUTPUT(L"remove the element that was moved");
            panel->Children->RemoveAt(3);
        } };

    SubmitRenderActions(actions, ARRAYSIZE(actions), rootPanel);

    // Validate the result
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

}


//------------------------------------------------------------------------
// Test case:
//  Add two consecutive nodes.
//  Device Lost
//  Remove the first added node
//------------------------------------------------------------------------
void CompositorTreeDeviceLostTests::AddTwoNodesThenRemoveFirst()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

    // Create our initial layout
    StackPanel^ rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = ref new StackPanel();
        VERIFY_IS_NOT_NULL(rootPanel);
        LOG_OUTPUT(L"Creating initial layout of two elements");
        rootPanel->Children->Append(CreateBorderElement(0));
        rootPanel->Children->Append(CreateBorderElement(2));
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    // Submit our render actions
    RENDERACTION actions[] = {
        [](Panel^ panel)
        {
            // We need to ensure that we get a device lost before we get to the moved node
            // (since that won't generate one), so insert a new node up front.
            LOG_OUTPUT(L"Insert two element between initial nodes");
            panel->Children->InsertAt(1, CreateBorderElement(9));
            panel->Children->InsertAt(2, CreateBorderElement(1));
        },
        [](Panel^ panel)
            {
                LOG_OUTPUT(L"remove the first inserted element");
                panel->Children->RemoveAt(1);
            } };

    SubmitRenderActions(actions, ARRAYSIZE(actions), rootPanel);

    // Validate the result
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

void CompositorTreeDeviceLostTests::RenderingNewCompNodeThatSplitsContainer()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_shapes::Ellipse^ splittingElement;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Initial tree.");

        splittingElement = ref new xaml_shapes::Ellipse();
        splittingElement->Width = 100;
        splittingElement->Height = 50;
        splittingElement->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        Canvas^ victimElement = ref new Canvas();
        victimElement->Width = 100;
        victimElement->Height = 50;
        victimElement->Background = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xff, 0, 0xff, 0));

        StackPanel^ root = ref new StackPanel();
        root->Children->Append(splittingElement);
        root->Children->Append(victimElement);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Set comp node.");
        splittingElement->CompositeMode = ElementCompositeMode::SourceOver;

        LOG_OUTPUT(L"> Inject device lost on rendering - switch to an ImageBrush and prime MockDComp to fail surface creation.");
        ImageBrush^ bitmapBrush = ref new ImageBrush();
        bitmapBrush->ImageSource = CreateWriteableBitmap();
        splittingElement->Fill = bitmapBrush;
        u->SetReturnDeviceLostWhenCreatingSurfaces(true);

        LOG_OUTPUT(L"> Don't crash on device lost recovery.");
    });
    wh->SynchronouslyTickUIThread(2);

    LOG_OUTPUT(L"> Stop returning device lost.");
    u->SetReturnDeviceLostWhenCreatingSurfaces(false);
    wh->SynchronouslyTickUIThread(2);
}

WriteableBitmap^ CompositorTreeDeviceLostTests::CreateWriteableBitmap()
{
    WriteableBitmap^ bitmap = ref new WriteableBitmap(30, 30);

    IBuffer^ buffer = bitmap->PixelBuffer;
    wrl::ComPtr<IUnknown> pBuffer((IUnknown*)buffer);
    wrl::ComPtr<IBufferByteAccess> pBufferByteAccess;
    pBuffer.As(&pBufferByteAccess);

    byte* pDstPixels = nullptr;
    pBufferByteAccess->Buffer(&pDstPixels);

    uint32* pCurrentDstPixel = reinterpret_cast<uint32*>(pDstPixels);
    for (int y = 0; y < 30; y++)
    {
        for (int x = 0; x < 30; x++, pCurrentDstPixel++)
        {
            *pCurrentDstPixel = 0xff0000ff;
        }
    }

    return bitmap;
}

} } } } } }
