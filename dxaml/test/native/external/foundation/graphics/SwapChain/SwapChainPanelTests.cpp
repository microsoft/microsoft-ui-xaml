// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include "SwapChainPanelTests.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <Microsoft.UI.Input.h>
#include "SCPTestHelper.h"
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>
#include <D3D12.h>
#include <DXGI1_4.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace ::Windows::UI::Core;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::System::Threading;
using namespace Microsoft::WRL;
using namespace concurrency;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ SwapChainPanelTests::GetPathToFiles() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\swapchain\\";
}

bool SwapChainPanelTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SwapChainPanelTests::ClassCleanup()
{
    return true;
}

bool SwapChainPanelTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SwapChainPanelTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    test_infra::TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

SwapChainPanel^ SwapChainPanelTests::CreateSizedSwapChainPanel()
{
    // Create a swap chain panel with an explicit size. It needs the explicit size whenever the root of the tree
    // is a Canvas. Canvas will measure its children at infinite size, and the SwapChainPanel will set its desired
    // size as 0x0 as a result. This means the SwapChainElement inside the SwapChainPanel will also be 0x0, and
    // the SpriteVisual hosting the swap chain will be 0x0. The swap chain will not be visible on screen. An
    // explicit size allows the SwapChainPanel/SwapChainElement/SpriteVisual to have a nonzero size.
    SwapChainPanel^ scp = ref new SwapChainPanel();
    scp->Width = 400;
    scp->Height = 300;
    return scp;
}

void SwapChainPanelTests::SwapChainBasic()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L" Basic test for setting up a swap chain.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        SwapChainPanel^ scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain");
        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();

        LOG_OUTPUT(L"Updating SwapChain to Green");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Green,1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SwapChainPanelTests::SwapChainNull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L"Test setting swap chain to NULL, it should remove the existing swapchain element.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Grid^ root;
    SwapChainPanel^ scp;

    RunOnUIThread([&]()
    {
        root = ref new Grid();
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        // Don't explicitly size this SwapChainPanel. Its parent is a Grid so it'll be measured at a finite size.
        scp = ref new SwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain");
        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainNull(); // set NULL on not yet set SCP
        swapChainTestWrapper.SetSwapChain(); // This swapchain should not appear at the dump later
        LOG_OUTPUT(L"Updating SwapChain to Brown");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Brown,1));
        root->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting SwapChain to Null");
        swapChainTestWrapper.SetSwapChainNull();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating and setting SwapChain again.");
        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain(); // This swapchain should not appear at the dump later
        LOG_OUTPUT(L"Updating SwapChain to Yellow");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Yellow,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting SwapChainHandel to Null, it should also clear the previously set SwapChain");
        swapChainTestWrapper.SetSwapChainHandleNull();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void SwapChainPanelTests::SwapChainUpdate()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L"Test multiple updates to a swapchain.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain");

        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();

        LOG_OUTPUT(L"Updating SwapChain to Brown");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Brown,1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Updating swap chain contents again to white");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::White,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

}

void SwapChainPanelTests::SwapChainClear()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L"Test clearing content should remove the swapchain element.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain");

        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain(); // This swapchain should not appear at the dump later

        LOG_OUTPUT(L"Updating SwapChain to Brown");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Clearing swap chain content...");
        canvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

}

void SwapChainPanelTests::SwapChainMultiple()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    SwapChainPanelTestWrapper swapChainTestWrapper1;
    SwapChainPanelTestWrapper swapChainTestWrapper2;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper1.ReleaseSwapChainPanel();});
        RunOnUIThread([&](){swapChainTestWrapper2.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L"Test content has multiple swapchain attached.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp1 = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain1");
        swapChainTestWrapper1.CreateSwapChain(scp1,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper1.SetSwapChain();

        LOG_OUTPUT(L"Updating SwapChain1 to Brown");
        swapChainTestWrapper1.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
        canvas->Children->Append(scp1);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp2 = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating second SwapChain2");
        swapChainTestWrapper2.SetD3DDevice(swapChainTestWrapper1.GetD3DDevice());
        swapChainTestWrapper2.CreateSwapChain(scp2,100,100, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper2.SetSwapChain();

        LOG_OUTPUT(L"Updating second SwapChain2 to Pink");
        swapChainTestWrapper2.Update(D2D1::ColorF(D2D1::ColorF::Pink,1));
        canvas->Children->Append(scp2);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        swapChainTestWrapper1.SetSwapChain(); // set same swap chain again
        LOG_OUTPUT(L"Updating SwapChain1 to White");
        swapChainTestWrapper1.Update(D2D1::ColorF(D2D1::ColorF::White,1));

        swapChainTestWrapper2.SetSwapChain(); // set same swap chain again
        LOG_OUTPUT(L"Updating SwapChain2 to Green");
        swapChainTestWrapper2.Update(D2D1::ColorF(D2D1::ColorF::Green,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting NULL to SwapChain1 to clear");
        swapChainTestWrapper1.SetSwapChainNull();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void SwapChainPanelTests::SwapChainBadHandle()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]() {swapChainTestWrapper.ReleaseSwapChainPanel(); });
    });

    LOG_OUTPUT(L" Test for verifying the handling of a bad swap chain handle.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp = CreateSizedSwapChainPanel();

        swapChainTestWrapper.CreateSwapChain(scp, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);

        LOG_OUTPUT(L"Creating Device and Setting bad SwapChain handle");
        swapChainTestWrapper.SetSwapChainBadHandle();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

void SwapChainPanelTests::SwapChainHandleBasic()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel(); });
    });

    LOG_OUTPUT(L" Basic test for setting up a swap chain handle.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        xaml_controls::Canvas^ child = ref new Canvas();
        child->Width = 300;
        child->Height = 300;

        // Create a swap chain panel, and render content to it
        // Don't explicitly size this SwapChainPanel. It will have a child which gives it a desired size.
        SwapChainPanel^ scp = ref new SwapChainPanel();
        scp->Children->Append(child);

        LOG_OUTPUT(L"Creating Device and Setting SwapChain handle");
        swapChainTestWrapper.CreateSwapChainHandle(scp, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating SwapChain to Green");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Green, 1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SwapChainPanelTests::SwapChainHandleNull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;
    SwapChainPanel^ scp;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel(); });
    });

    LOG_OUTPUT(L"Test setting swap chain handle to NULL, it should remove the existing swapchain element.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain Handle");
        swapChainTestWrapper.CreateSwapChainHandle(scp, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainHandleNull(); // set NULL on not yet set SCP
        swapChainTestWrapper.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating SwapChain to Green");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Green, 1));
        canvas->Children->Append(scp);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting SwapChain Handle to Null");
        swapChainTestWrapper.SetSwapChainHandleNull();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;

        LOG_OUTPUT(L"Creating and setting SwapChain Handle again.");
        swapChainTestWrapper.CreateSwapChainHandle(scp, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating SwapChain to Yellow");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Yellow, 1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting SwapChain to Null, it should also clear the previously set SwapChainHandle");
        swapChainTestWrapper.SetSwapChainNull();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

}

void SwapChainPanelTests::SwapChainHandleUpdate()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel(); });
    });

    LOG_OUTPUT(L"Test multiple updates to a swapchain handle.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain Handle");
        swapChainTestWrapper.CreateSwapChainHandle(scp, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating SwapChain to Green");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Green, 1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Updating swap chain contents again...");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::White,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SwapChainPanelTests::SwapChainHandleClear()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel(); });
    });

    LOG_OUTPUT(L"Test clearing content should remove the swapchain element.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain handle");
        swapChainTestWrapper.CreateSwapChainHandle(scp, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating SwapChain to Green");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Green, 1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Clearing swap chain handle content...");
        canvas->Children->Clear();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SwapChainPanelTests::SwapChainHandleMultiple()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper1;
    SwapChainPanelTestWrapper swapChainTestWrapper2;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper1.ReleaseSwapChainPanel();});
        RunOnUIThread([&](){swapChainTestWrapper2.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L"Test content has multiple swapchain handle attached.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp1 = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain Handle1");
        swapChainTestWrapper1.CreateSwapChainHandle(scp1,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper1.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating SwapChain to Brown");
        swapChainTestWrapper1.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
        canvas->Children->Append(scp1);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        SwapChainPanel^ scp2 = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating second SwapChain Handle2");
        swapChainTestWrapper2.SetD3DDevice(swapChainTestWrapper1.GetD3DDevice());
        swapChainTestWrapper2.CreateSwapChainHandle(scp2,100,100, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper2.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating second SwapChain to Pink");
        swapChainTestWrapper2.Update(D2D1::ColorF(D2D1::ColorF::Pink,1));
        canvas->Children->Append(scp2);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        swapChainTestWrapper1.SetSwapChainHandle(); // set same swap chain handle again
        LOG_OUTPUT(L"Updating first SwapChain to White");
        swapChainTestWrapper1.Update(D2D1::ColorF(D2D1::ColorF::White,1));

        swapChainTestWrapper2.SetSwapChainHandle(); // set same swap chain handle again
        LOG_OUTPUT(L"Updating second SwapChain to Green");
        swapChainTestWrapper2.Update(D2D1::ColorF(D2D1::ColorF::Green,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set SwapChain Handle 1 to NULL");
        swapChainTestWrapper1.SetSwapChainHandleNull();
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void SwapChainPanelTests::DX12SwapChainDevice()
{
    int refCountBeforeSwapChainAddition, refCountAfterSwapChainRemoval;
    ID3D12Device* dx12Device = nullptr; // Not a smart pointer so we can verify final release
    SwapChainPanel^ swapChainPanel;

    TestCleanupWrapper cleanup([&]()
    {
        if (dx12Device != nullptr) dx12Device->Release();
    });

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Create SwapChain");
    RunOnUIThread([&]()
    {
        // Create a swap chain panel
        swapChainPanel = CreateSizedSwapChainPanel();
        canvas->Children->Append(swapChainPanel);
    });

    TestServices::WindowHelper->WaitForIdle();

    bool isDeviceDestructed = false;
    RunOnUIThread([&]()
    {
        ComPtr<IDXGIFactory4> factory;
        ComPtr<IDXGIAdapter1> adapter;
        ComPtr<ID3D12Device> device;
        ComPtr<IDXGISwapChain1> swapChain;
        ComPtr<ID3D12CommandQueue> dx12CommandQueue;

        LOG_OUTPUT(L"Create DX12 Device");
        VERIFY_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        VERIFY_SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        VERIFY_IS_NOT_NULL(adapter.Get());
        VERIFY_SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

        ID3D12Device* deviceTmp = device.Get();
        deviceTmp->AddRef();
        LOG_OUTPUT(L"Ref Count After Creation: %d", deviceTmp->Release());

        LOG_OUTPUT(L"Register for callback notification when device is destructed");
        {
            ComPtr<ID3DDestructionNotifier> notifier;
            VERIFY_SUCCEEDED(device.As(&notifier));
            VERIFY_SUCCEEDED(notifier->RegisterDestructionCallback([] (void* data) {
                *((bool*)data) = true;
            }, (void*)&isDeviceDestructed, nullptr));
        }

        LOG_OUTPUT(L"Create DX12 CommandQueue");
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0
        };

        deviceTmp->AddRef();
        LOG_OUTPUT(L"Ref Count Before Command Creation: %d", deviceTmp->Release());

        VERIFY_SUCCEEDED(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&dx12CommandQueue)));

        deviceTmp->AddRef();
        LOG_OUTPUT(L"Ref Count After Command Creation: %d", deviceTmp->Release());

        LOG_OUTPUT(L"Create DX SwapChain");
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = 100;
        swapChainDesc.Height = 100;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED;
        swapChainDesc.Flags = 0;

        VERIFY_SUCCEEDED(factory->CreateSwapChainForComposition(dx12CommandQueue.Get(), &swapChainDesc, nullptr, &swapChain));

        deviceTmp->AddRef();
        refCountBeforeSwapChainAddition = deviceTmp->Release();
        LOG_OUTPUT(L"Ref Count Before Set SwapChain: %d", refCountBeforeSwapChainAddition);

        LOG_OUTPUT(L"Attach DX SwapChain to SwapChainPanel");
        ComPtr<ISwapChainPanelNative> panelNative;
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative)));
        VERIFY_SUCCEEDED(panelNative->SetSwapChain(swapChain.Get()));

        // hold onto the device pointer for later release verification
        dx12Device = device.Detach();
        
        deviceTmp->AddRef();
        int refCountAfterSwapChainAddition = deviceTmp->Release();
        LOG_OUTPUT(L"Ref Count After Set SwapChain: %d", refCountAfterSwapChainAddition);

        VERIFY_ARE_EQUAL(refCountBeforeSwapChainAddition, refCountAfterSwapChainAddition);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        ID3D12Device* deviceTmp = dx12Device;
        deviceTmp->AddRef();
        int refCountBeforeSwapChainRemoval = deviceTmp->Release();
        LOG_OUTPUT(L"Ref Count Before Reset SwapChain: %d", refCountBeforeSwapChainRemoval);

        LOG_OUTPUT(L"Remove SwapChain from SwapChainPanel");
        ComPtr<ISwapChainPanelNative> panelNative;
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative)));
        VERIFY_SUCCEEDED(panelNative->SetSwapChain(nullptr));

        deviceTmp->AddRef();
        refCountAfterSwapChainRemoval = deviceTmp->Release();
        LOG_OUTPUT(L"Ref Count After Reset SwapChain: %d", refCountAfterSwapChainRemoval);

        VERIFY_ARE_EQUAL(refCountBeforeSwapChainRemoval, refCountAfterSwapChainRemoval);
        VERIFY_ARE_EQUAL(refCountBeforeSwapChainAddition, refCountAfterSwapChainRemoval);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(isDeviceDestructed);
        ID3D12Device* deviceTmp = dx12Device;
        dx12Device = nullptr;
        int finalRefCount = deviceTmp->Release();
        LOG_OUTPUT(L"Final Ref Count: %d, isDeviceDestructed: %d", finalRefCount, isDeviceDestructed);

        // Ensure that our reference is the last one being held
        VERIFY_ARE_EQUAL(0, finalRefCount);
        VERIFY_IS_TRUE(isDeviceDestructed);
    });
}

void SwapChainPanelTests::SwapChainAndSwapChainHandle()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    LOG_OUTPUT(L"Test setting SwapChain and SwapChain Handle on same content.");
    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    SwapChainPanel^ scp;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Create a swap chain panel, and render content to it
        scp = CreateSizedSwapChainPanel();

        LOG_OUTPUT(L"Creating Device and Setting SwapChain");
        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        LOG_OUTPUT(L"Updating SwapChain to Brown");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
        canvas->Children->Append(scp);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating SwapChainHandle on same content.");
        swapChainTestWrapper.CreateSwapChainHandle(scp,100,100, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChainHandle();

        LOG_OUTPUT(L"Updating second SwapChain to Pink");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Pink,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating another SwapChain on same content.");
        swapChainTestWrapper.CreateSwapChain(scp,100,100, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();

        LOG_OUTPUT(L"Updating second SwapChain to Green");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Green,1));
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

}

void SwapChainPanelTests::CompositionScaleChanged()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
    });

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, 1.5f);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto scaleChangedRegistration = CreateSafeEventRegistration(xaml_controls::SwapChainPanel, CompositionScaleChanged);
    auto scaleChangedEvent = std::make_shared<Event>();

    SwapChainPanel^ scp;
    std::vector<double> scaleXs;
    std::vector<double> scaleYs;

    RunOnUIThread([&]()
    {
        scp = CreateSizedSwapChainPanel();

        swapChainTestWrapper.CreateSwapChain(scp,200,200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();

        scaleChangedRegistration.Attach(scp, ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::SwapChainPanel^, Platform::Object^>([&](xaml_controls::SwapChainPanel^ sender, Platform::Object^ e)
        {
            scaleXs.push_back(scp->CompositionScaleX);
            scaleYs.push_back(scp->CompositionScaleY);
        }));

        LOG_OUTPUT(L"Verifying value of SwapChainPanel.CompositionScaleX/Y before adding to visual tree");
        float scalex = scp->CompositionScaleX;
        float scaley = scp->CompositionScaleY;
        LOG_OUTPUT(L"CompositionScaleX: %f, CompositionScaleY: %f", scalex, scaley);
        VERIFY_ARE_EQUAL(1.5f, scalex);
        VERIFY_ARE_EQUAL(1.5f, scaley);

        LOG_OUTPUT(L"Adding SwapChainPanel to visual tree");
        canvas->Children->Append(scp);

        LOG_OUTPUT(L"Verifying value of SwapChainPanel.CompositionScaleX/Y after adding to visual tree but before it renders");
        // We're testing the behavior while !IsInPCScene(). Even though the SCP is in the tree, we haven't marked IsInPCScene yet because we haven't done a render walk yet.
        scalex = scp->CompositionScaleX;
        scaley = scp->CompositionScaleY;
        LOG_OUTPUT(L"CompositionScaleX: %f, CompositionScaleY: %f", scalex, scaley);
        VERIFY_ARE_EQUAL(1.5f, scalex);
        VERIFY_ARE_EQUAL(1.5f, scaley);
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Loaded SCP - got CompositionScaleChanged with 1.5x & 1.5x");
    VERIFY_ARE_EQUAL(1u, scaleXs.size());
    VERIFY_ARE_EQUAL(1.5f, scaleXs[0]);
    VERIFY_ARE_EQUAL(1u, scaleYs.size());
    VERIFY_ARE_EQUAL(1.5f, scaleYs[0]);

    RunOnUIThread([&]()
    {
        CompositeTransform^ target = ref new CompositeTransform();
        scp->RenderTransform = target;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 1.0;
        da->To = 2.0;
        ::Windows::Foundation::TimeSpan span;
        span.Duration = 10000000L;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, target);
        Storyboard::SetTargetProperty(da, L"ScaleX");

        Storyboard^ sb = ref new Storyboard();
        sb->Children->Append(da);

        sb->Begin();
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Animation completed - got CompositionScaleChanged with final values, and no events in between");
    VERIFY_ARE_EQUAL(2u, scaleXs.size());
    VERIFY_ARE_EQUAL(1.5f, scaleXs[0]);
    VERIFY_ARE_EQUAL(3.0f, scaleXs[1]);
    VERIFY_ARE_EQUAL(2u, scaleYs.size());
    VERIFY_ARE_EQUAL(1.5f, scaleYs[0]);
    VERIFY_ARE_EQUAL(1.5f, scaleYs[1]);
}

void SwapChainPanelTests::HitTestChildWUCInternal()
{
    SwapChainPanelTestWrapper swapChainTestWrapper;

    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    TestCleanupWrapper cleanup([&]()
    {
        pointerPressedEvent.reset();
        m_button = nullptr;
        m_swapChainPanel = nullptr;
        RunOnUIThread([&](){swapChainTestWrapper.ReleaseSwapChainPanel();});
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain2.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
        m_swapChainPanel = safe_cast<SwapChainPanel^>(canvas->FindName(L"mySwapChainPanel"));
        m_button = safe_cast<xaml_controls::Button^>(canvas->FindName(L"myButton"));
        LOG_OUTPUT(L"Creating SwapChain");
        swapChainTestWrapper.CreateSwapChain(m_swapChainPanel, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        LOG_OUTPUT(L"Updating SwapChain");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
    });
    LOG_OUTPUT(L"Test1:  Base case");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test2:  IsHitTestVisible=false");
        m_button->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test3:  IsHitTestVisible=true");
        m_button->IsHitTestVisible = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test4:  IsEnabled=false");
        m_button->IsEnabled = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test5:  IsEnabled=true");
        m_button->IsEnabled = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test6:  IsHitTestVisible=false");
        m_button->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test7:  Visibility=Collapsed");
        m_button->Visibility = xaml::Visibility::Collapsed;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test8:  Visibility=Visible");
        m_button->Visibility = xaml::Visibility::Visible;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test9:  Clear Children");
        m_swapChainPanel->Children->Clear();
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test10:  Add child back");
        m_swapChainPanel->Children->Append(m_button);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test11:  Add Composite Mode=MinBlend");
        m_button->CompositeMode = ElementCompositeMode::MinBlend;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test12:  IsHitTestVisible=true");
        m_button->IsHitTestVisible = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"5");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test13:  IsHitTestVisible=false");
        m_button->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");
}

void SwapChainPanelTests::HitTestChildWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    HitTestChildWUCInternal();
}

void SwapChainPanelTests::HitTestOverlaySwapChainPanelWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    SwapChainPanelTestWrapper swapChainTestWrapper;
    SwapChainPanelTestWrapper swapChainTestWrapperOverlay;

    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    TestCleanupWrapper cleanup([&]()
    {
        pointerPressedEvent.reset();
        m_button = nullptr;
        m_swapChainPanel = nullptr;
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
            swapChainTestWrapperOverlay.ReleaseSwapChainPanel();
        });
    });

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas;
    SwapChainPanel^ overlaySwapChainPanel;

    RunOnUIThread([&]()
    {
        canvas = ref new xaml_controls::Canvas();

        m_swapChainPanel = ref new SwapChainPanel();
        m_swapChainPanel->Width = 400;
        m_swapChainPanel->Height = 400;
        LOG_OUTPUT(L"Creating SwapChain");
        swapChainTestWrapper.CreateSwapChain(m_swapChainPanel, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        LOG_OUTPUT(L"Updating SwapChain");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
        canvas->Children->Append(m_swapChainPanel);

        overlaySwapChainPanel = ref new SwapChainPanel();
        overlaySwapChainPanel->Width = 400;
        overlaySwapChainPanel->Height = 400;
        LOG_OUTPUT(L"Creating child SwapChain");
        swapChainTestWrapperOverlay.CreateSwapChain(overlaySwapChainPanel, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapperOverlay.SetSwapChain();
        LOG_OUTPUT(L"Updating SwapChain");
        swapChainTestWrapperOverlay.Update(D2D1::ColorF(D2D1::ColorF::Blue,1));
        canvas->Children->Append(overlaySwapChainPanel);

        TestServices::WindowHelper->WindowContent = canvas;
    });

    LOG_OUTPUT(L"Base case");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"IsHitTestVisible=false");
        overlaySwapChainPanel->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"IsHitTestVisible=true");
        overlaySwapChainPanel->IsHitTestVisible = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"IsHitTestVisible=false");
        overlaySwapChainPanel->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Visibility=Collapsed");
        overlaySwapChainPanel->Visibility = xaml::Visibility::Collapsed;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Visibility=Visible");
        overlaySwapChainPanel->Visibility = xaml::Visibility::Visible;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Remove overlay");
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(canvas->Children->IndexOf(overlaySwapChainPanel, &indexToRemove));
        canvas->Children->RemoveAt(indexToRemove);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Add overlay back");
        canvas->Children->Append(overlaySwapChainPanel);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void SwapChainPanelTests::HitTestPopup()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SwapChainPanelTestWrapper swapChainTestWrapper;
    xaml_primitives::Popup^ popup = nullptr;
    xaml_shapes::Rectangle^ rectangle = nullptr;

    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>(EventOptions::ManualReset);

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
        });
        pointerPressedEvent.reset();
        m_swapChainPanel = nullptr;
        popup = nullptr;
        rectangle = nullptr;
    });

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain3.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
        m_swapChainPanel = safe_cast<SwapChainPanel^>(canvas->FindName(L"mySwapChainPanel"));
        popup = safe_cast<xaml_primitives::Popup^>(canvas->FindName(L"myPopup"));
        rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"myRectangle"));
        LOG_OUTPUT(L"Creating SwapChain");
        swapChainTestWrapper.CreateSwapChain(m_swapChainPanel, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        LOG_OUTPUT(L"Updating SwapChain");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
    });
    LOG_OUTPUT(L"Test1:  Base case");
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test2:  IsHitTestVisible=false");
        rectangle->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test3:  IsHitTestVisible=true");
        rectangle->IsHitTestVisible = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test4:  IsHitTestVisible=false");
        rectangle->IsHitTestVisible = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test5:  Visibility=Collapsed");
        rectangle->Visibility = xaml::Visibility::Collapsed;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test6:  Visibility=Visible");
        rectangle->Visibility = xaml::Visibility::Visible;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test7:  Clearing child");
        popup->Child = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test8:  Adding child back");
        popup->Child = rectangle;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test9:  Closing Popup");
        popup->IsOpen = false;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Test10:  Opening Popup");
        popup->IsOpen = true;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void SwapChainPanelTests::HitTestTransparentBrushInternal()
{
    SwapChainPanelTestWrapper swapChainTestWrapper;
    xaml_controls::Canvas^ myCanvas = nullptr;

    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>(EventOptions::ManualReset);

    auto pressedEvent = std::make_shared<Event>();
    auto pressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
        });
        pointerPressedEvent.reset();
        m_swapChainPanel = nullptr;
        myCanvas = nullptr;
    });

    ::Windows::Foundation::Size size(400, 400);
    TestServices::WindowHelper->SetWindowSizeOverride(size);
    xaml_controls::Canvas^ canvas = safe_cast<xaml_controls::Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"SwapChain4.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
        m_swapChainPanel = safe_cast<SwapChainPanel^>(canvas->FindName(L"mySwapChainPanel"));
        myCanvas = safe_cast<xaml_controls::Canvas^>(canvas->FindName(L"myCanvas"));
        LOG_OUTPUT(L"Creating SwapChain");
        swapChainTestWrapper.CreateSwapChain(m_swapChainPanel, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        LOG_OUTPUT(L"Updating SwapChain");
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));

        pressedRegistration.Attach(
            myCanvas,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"PointerPressed event raised.");
            pressedEvent->Set();
        }));

    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    TestServices::InputHelper->Tap(myCanvas);
    pressedEvent->WaitForDefault();
}

void SwapChainPanelTests::HitTestTransparentBrushWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestTransparentBrushInternal();
}

void SwapChainPanelTests::HitTestDisabledParentWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    ::Windows::Foundation::Size size(400, 400);
    wh->SetWindowSizeOverride(size);

    Grid^ root;
    ContentControl^ scpParent;
    xaml_shapes::Rectangle^ rect;
    SwapChainPanelTestWrapper swapChainTestWrapper;
    SwapChainPanel^ scp;
    auto pressedEvent = std::make_shared<Event>();
    auto pressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
        });
        pressedEvent.reset();
        root = nullptr;
        scpParent = nullptr;
        rect = nullptr;
        scp = nullptr;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        root = ref new Grid();
        scpParent = ref new ContentControl();
        rect = ref new xaml_shapes::Rectangle();
        pressedRegistration.Attach(
            rect,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"PointerPressed event raised.");
            pressedEvent->Set();
        }));

        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 400;
        rect->Height = 400;
        scp = ref new SwapChainPanel();
        scp->Width = 400;
        scp->Height = 400;
        swapChainTestWrapper.CreateSwapChain(scp, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));
        root->Children->Append(rect);
        root->Children->Append(scpParent);
        scpParent->Content = scp;
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        scpParent->IsHitTestVisible = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        scpParent->IsHitTestVisible = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        scpParent->IsEnabled = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void SwapChainPanelTests::ResizeSwapChainPanel()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    ::Windows::Foundation::Size size(400, 400);
    wh->SetWindowSizeOverride(size);

    Grid^ root;
    ContentControl^ scpParent;
    SwapChainPanelTestWrapper swapChainTestWrapper;
    SwapChainPanel^ scp;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
        });
        root = nullptr;
        scpParent = nullptr;
        scp = nullptr;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");

        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

        scp = ref new SwapChainPanel();
        scp->Width = 200;
        scp->Height = 200;
        scp->ColumnDefinitions->Append(ref new xaml_controls::ColumnDefinition());
        scp->ColumnDefinitions->Append(ref new xaml_controls::ColumnDefinition());
        scp->Children->Append(rect);

        swapChainTestWrapper.CreateSwapChain(scp, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray,1));

        scpParent = ref new ContentControl();
        scpParent->Content = scp;

        root = ref new Grid();
        root->Children->Append(scpParent);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        scp->Width = 300;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void SwapChainPanelTests::CreateCoreIndependentInputSource()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    auto independentInputCreatedEvent = std::make_shared<Event>();

    SwapChainPanel^ scp;
    SwapChainPanelTestWrapper swapChainTestWrapper;
    ixp::InputPointerSource^ inputPointerSource;
    ixp::InputPointerSource^ inputPointerSourceNone;

    SwapChainPanel^ scpNotInTree;
    SwapChainPanelTestWrapper swapChainTestWrapperNotInTree;
    ixp::InputPointerSource^ inputPointerSourceNotInTree;

    SwapChainPanel^ scpWithoutSwapChain;
    ixp::InputPointerSource^ inputPointerSourceWithoutSwapChain;

    SwapChainPanel^ scpInputBeforeSwapChain;
    SwapChainPanelTestWrapper swapChainTestWrapperInputBeforeSwapChain;
    ixp::InputPointerSource^ inputPointerSourceInputBeforeSwapChain;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
            swapChainTestWrapperNotInTree.ReleaseSwapChainPanel();
            swapChainTestWrapperInputBeforeSwapChain.ReleaseSwapChainPanel();
        });
        root = nullptr;
        scp = nullptr;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");

        scp = ref new SwapChainPanel();
        scp->Width = 100;
        scp->Height = 100;

        swapChainTestWrapper.CreateSwapChain(scp, 100, 100, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray, 1));

        scpNotInTree = ref new SwapChainPanel();
        scpNotInTree->Width = 200;
        scpNotInTree->Height = 200;

        swapChainTestWrapperNotInTree.CreateSwapChain(scpNotInTree, 200, 200, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapperNotInTree.SetSwapChain();
        swapChainTestWrapperNotInTree.Update(D2D1::ColorF(D2D1::ColorF::Gray, 1));

        scpWithoutSwapChain = ref new SwapChainPanel();
        scpWithoutSwapChain->Width = 300;
        scpWithoutSwapChain->Height = 300;

        scpInputBeforeSwapChain = ref new SwapChainPanel();
        scpInputBeforeSwapChain->Width = 400;
        scpInputBeforeSwapChain->Height = 400;

        root = ref new Canvas();
        root->Children->Append(scp);
        root->Children->Append(scpWithoutSwapChain);
        root->Children->Append(scpInputBeforeSwapChain);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    auto workItemHandler = ref new ::Windows::System::Threading::WorkItemHandler([&] (IAsyncAction ^)
    {
        LOG_OUTPUT(L"Creating CII");
        inputPointerSource = scp->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Touch);
        inputPointerSourceNone = scp->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::None);
        inputPointerSourceNotInTree = scpNotInTree->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Touch);
        inputPointerSourceWithoutSwapChain = scpWithoutSwapChain->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Mouse);
        inputPointerSourceInputBeforeSwapChain = scpWithoutSwapChain->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Touch);
        wh->SynchronouslyTickUIThread(10);
        independentInputCreatedEvent->Set();
    });
    ::Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

    independentInputCreatedEvent->WaitForDefault();
    VERIFY_IS_NOT_NULL(inputPointerSource);
    VERIFY_IS_NULL(inputPointerSourceNone);
    VERIFY_IS_NOT_NULL(inputPointerSourceNotInTree);
    VERIFY_IS_NOT_NULL(inputPointerSourceWithoutSwapChain);
    VERIFY_IS_NOT_NULL(inputPointerSourceInputBeforeSwapChain);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1.5");

    RunOnUIThread([&]()
    {
        swapChainTestWrapperInputBeforeSwapChain.CreateSwapChain(scpInputBeforeSwapChain, 400, 400, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapperInputBeforeSwapChain.SetSwapChain();
        swapChainTestWrapperInputBeforeSwapChain.Update(D2D1::ColorF(D2D1::ColorF::Gray, 1));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void SwapChainPanelTests::HitTestingOnlySwapChainPanel()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    auto independentInputCreatedEvent = std::make_shared<Event>();

    SwapChainPanel^ scp;
    SwapChainPanelTestWrapper swapChainTestWrapper;
    ixp::InputPointerSource^ inputPointerSource;
    ixp::InputPointerSource^ inputPointerSourceNone;

    SwapChainPanel^ scpWithoutSwapChain;
    ixp::InputPointerSource^ inputPointerSourceWithoutSwapChain;

    SwapChainPanel^ scpWithoutSwapChainOrInputObserver;

    SwapChainPanel^ scpWithNullSwapChainAfterCII;
    ixp::InputPointerSource^ inputPointerSource2;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            swapChainTestWrapper.ReleaseSwapChainPanel();
        });
        root = nullptr;
        scp = nullptr;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating XAML tree");

        scp = ref new SwapChainPanel();
        scp->Width = 100;
        scp->Height = 100;

        swapChainTestWrapper.CreateSwapChain(scp, 100, 100, DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_PREMULTIPLIED);
        swapChainTestWrapper.SetSwapChain();
        swapChainTestWrapper.Update(D2D1::ColorF(D2D1::ColorF::Gray, 1));

        scpWithoutSwapChain = ref new SwapChainPanel();
        scpWithoutSwapChain->Width = 200;
        scpWithoutSwapChain->Height = 200;

        scpWithoutSwapChainOrInputObserver = ref new SwapChainPanel();
        scpWithoutSwapChainOrInputObserver->Width = 300;
        scpWithoutSwapChainOrInputObserver->Height = 300;

        scpWithNullSwapChainAfterCII = ref new SwapChainPanel();
        scpWithNullSwapChainAfterCII->Width = 400;
        scpWithNullSwapChainAfterCII->Height = 400;

        root = ref new Canvas();
        root->Children->Append(scp);
        root->Children->Append(scpWithoutSwapChain);
        root->Children->Append(scpWithoutSwapChainOrInputObserver);
        root->Children->Append(scpWithNullSwapChainAfterCII);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    auto workItemHandler = ref new ::Windows::System::Threading::WorkItemHandler([&] (IAsyncAction ^)
    {
        LOG_OUTPUT(L"> Creating CII");
        inputPointerSource = scp->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Touch);
        inputPointerSourceNone = scp->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::None);
        inputPointerSourceWithoutSwapChain = scpWithoutSwapChain->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Mouse);
        inputPointerSource2 = scpWithNullSwapChainAfterCII->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Touch);
        wh->SynchronouslyTickUIThread(10);
        independentInputCreatedEvent->Set();
    });
    ::Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

    independentInputCreatedEvent->WaitForDefault();
    independentInputCreatedEvent->Reset();
    VERIFY_IS_NOT_NULL(inputPointerSource);
    VERIFY_IS_NULL(inputPointerSourceNone);
    VERIFY_IS_NOT_NULL(inputPointerSourceWithoutSwapChain);
    VERIFY_IS_NOT_NULL(inputPointerSource2);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting null swap chain");

        ComPtr<ISwapChainPanelNative> panelNative;
        LogThrow_IfFailed(reinterpret_cast<IUnknown*>(scpWithNullSwapChainAfterCII)->QueryInterface(IID_PPV_ARGS(&panelNative)));
        LogThrow_IfFailed(panelNative->SetSwapChain(nullptr));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Resizing all SwapChainPanels");
        scp->Height = 50;
        scpWithoutSwapChain->Height = 50;
        scpWithoutSwapChainOrInputObserver->Height = 50;
        scpWithNullSwapChainAfterCII->Height = 50;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    workItemHandler = ref new ::Windows::System::Threading::WorkItemHandler([&] (IAsyncAction ^)
    {
        LOG_OUTPUT(L"> Creating another CII");
        inputPointerSource2 = scpWithNullSwapChainAfterCII->CreateCoreIndependentInputSource(::Microsoft::UI::Input::InputPointerSourceDeviceKinds::Mouse);
        wh->SynchronouslyTickUIThread(10);
        independentInputCreatedEvent->Set();
    });
    ::Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

    independentInputCreatedEvent->WaitForDefault();
    independentInputCreatedEvent->Reset();
    VERIFY_IS_NOT_NULL(inputPointerSource2);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"5");
}

// Helper class to implement IUnknown
template<typename T>
class ComObject : public T
{
private:
    ULONG m_cRef;

protected:
    ComObject() :
         m_cRef(1)
    {
    }
    virtual ~ComObject() { }

public:
    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return ++m_cRef;
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = --m_cRef;
        if (0 == cRef)
        {
            delete this;
        }
        return cRef;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ void** ppvObject)
    {
        *ppvObject = NULL;

        if (__uuidof(IUnknown) == riid)
        {
            *ppvObject = static_cast<IUnknown*>(this);
        }
        else if (__uuidof(T) == riid)
        {
            *ppvObject = static_cast<T*>(this);
        }
        else
        {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }
};

} } } }}}
