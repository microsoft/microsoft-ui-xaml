// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SISTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;
using namespace WEX::Common;
using namespace ::Windows::Internal;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ SISTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool SISTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SISTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SISTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SISTests::CreateD3DDevice()
{
    const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    unsigned int flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    VERIFY_SUCCEEDED(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        nullptr,
        nullptr
        ));

    VERIFY_SUCCEEDED(m_d3dDevice.As(&m_dxgiDevice));
}

void SISTests::EnableMultithreading()
{
    ComPtr<ID3D10Multithread> spD3DMultiThread;
    VERIFY_SUCCEEDED(m_d3dDevice.As(&spD3DMultiThread));
    spD3DMultiThread->SetMultithreadProtected(TRUE);
}

void SISTests::CreateD2DDevice()
{
    VERIFY_SUCCEEDED(D2D1CreateDevice(m_dxgiDevice.Get(), nullptr, &m_d2dDevice));
}

void SISTests::CreateD2DContext()
{
    VERIFY_SUCCEEDED(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext));
}

void SISTests::CleanupDevices()
{
    m_d2dContext = nullptr;
    m_d2dDevice = nullptr;
    m_dxgiDevice = nullptr;
    m_d3dDevice = nullptr;
}

void SISTests::Draw(ISurfaceImageSourceNative* pSIS, RECT rect, D2D1::ColorF color)
{
    POINT offset;
    ComPtr<IDXGISurface> spSurface;
    VERIFY_SUCCEEDED(pSIS->BeginDraw(rect, &spSurface, &offset));
    ComPtr<ID2D1Bitmap1> spD2DBitmap;
    VERIFY_SUCCEEDED(m_d2dContext->CreateBitmapFromDxgiSurface(spSurface.Get(), nullptr, &spD2DBitmap));
    m_d2dContext->SetTarget(spD2DBitmap.Get());
    m_d2dContext->BeginDraw();
    m_d2dContext->Clear(color);
    VERIFY_SUCCEEDED(m_d2dContext->EndDraw());
    VERIFY_SUCCEEDED(pSIS->EndDraw());
    m_d2dContext->SetTarget(nullptr);
}

void SISTests::DrawWithD2D(ISurfaceImageSourceNativeWithD2D* pSIS, RECT rect, D2D1::ColorF color)
{
    POINT offset;
    ComPtr<ID2D1DeviceContext> spDeviceContext;
    VERIFY_SUCCEEDED(pSIS->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
    spDeviceContext->Clear(color);
    VERIFY_SUCCEEDED(pSIS->EndDraw());
}

void SISTests::DrawWithD2DSuspend(ISurfaceImageSourceNativeWithD2D* pSIS, RECT rect, D2D1::ColorF color)
{
    POINT offset;
    ComPtr<ID2D1DeviceContext> spDeviceContext;
    VERIFY_SUCCEEDED(pSIS->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
    spDeviceContext->Clear(color);
    VERIFY_SUCCEEDED(pSIS->SuspendDraw());
}

void SISTests::DrawWithD2DMultiThreaded(ISurfaceImageSourceNativeWithD2D* pSIS, RECT rect, D2D1::ColorF color)
{
    POINT offset;
    ComPtr<ID2D1DeviceContext> spDeviceContext;
    VERIFY_SUCCEEDED(pSIS->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
    spDeviceContext->Clear(color);

    FlushWork();

    RunOnUIThread([&]()
    {
        VERIFY_SUCCEEDED(pSIS->EndDraw());
    });
}

void SISTests::FlushWork()
{
    ComPtr<IInspectable> spSISFactory;
    ComPtr<ISurfaceImageSourceManagerNative> spSISManager;

    HStringReference id(L"Microsoft.UI.Xaml.Media.Imaging.SurfaceImageSource");
    VERIFY_SUCCEEDED(::Windows::Foundation::GetActivationFactory(id.Get(), &spSISFactory));
    VERIFY_SUCCEEDED(spSISFactory.As<ISurfaceImageSourceManagerNative>(&spSISManager));
    VERIFY_SUCCEEDED(spSISManager->FlushAllSurfacesWithDevice(m_d2dDevice.Get()));
}

ComPtr<ISurfaceImageSourceNative> SISTests::GetSISNative(ISurfaceImageSource^ sis)
{
    ComPtr<ISurfaceImageSourceNative> spSISNative;
    (reinterpret_cast<IUnknown*>(sis))->QueryInterface(IID_PPV_ARGS(&spSISNative));

    return spSISNative;
}

ComPtr<ISurfaceImageSourceNativeWithD2D> SISTests::GetSISNativeWithD2D(ISurfaceImageSource^ sis)
{
    ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative;
    (reinterpret_cast<IUnknown*>(sis))->QueryInterface(IID_PPV_ARGS(&spSISNative));

    return spSISNative;
}

void SISTests::Basics1()
{
    TestCleanupWrapper cleanup([&]()
    {
        CleanupDevices();
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    CreateD3DDevice();
    CreateD2DDevice();
    CreateD2DContext();

    // Load simple scenario XAML - a Canvas that we'll fill with a single SIS as its Background
    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();
    ComPtr<ISurfaceImageSourceNative> spSISNative;
    int width, height;

    RunOnUIThread([&]()
    {
        myCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"myCanvas"));
        width = static_cast<int>(myCanvas->Width);
        height = static_cast<int>(myCanvas->Height);
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ImageBrush^ myBrush = ref new ImageBrush();
        myBrush->ImageSource = mySIS;
        myCanvas->Background = myBrush;

        // Fill the entire SIS with a solid color using ISurfaceImageSourceNative
        LOG_OUTPUT(L"Filling SIS with Red");
        RECT updateRect = { 0, 0, width, height };
        spSISNative = GetSISNative(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Red));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"1");

    RunOnUIThread([&]()
    {
        // Do a partial update
        LOG_OUTPUT(L"Filling portion of SIS");
        RECT updateRect = { 0, 0, width/2, height/2 };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"2");
}

void SISTests::Basics2()
{
    TestCleanupWrapper cleanup([&]()
    {
        CleanupDevices();
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    CreateD3DDevice();
    EnableMultithreading();
    CreateD2DDevice();

    // Load simple scenario XAML - a Canvas that we'll fill with a single SIS as its Background
    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();
    ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative;
    int width, height;

    RunOnUIThread([&]()
    {
        myCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"myCanvas"));
        width = static_cast<int>(myCanvas->Width);
        height = static_cast<int>(myCanvas->Height);
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ImageBrush^ myBrush = ref new ImageBrush();
        myBrush->ImageSource = mySIS;
        myCanvas->Background = myBrush;

        // Fill the entire SIS with a solid color using ISurfaceImageSourceNativeWithD2D
        LOG_OUTPUT(L"Filling SIS with Red");
        RECT updateRect = { 0, 0, width, height };
        spSISNative = GetSISNativeWithD2D(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_d2dDevice.Get()));
        DrawWithD2D(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Red));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"1");

    // Perform a partial update from a background thread
    LOG_OUTPUT(L"Filling portion of SIS");
    RECT updateRect = { 0, 0, width/2, height/2 };
    DrawWithD2DMultiThreaded(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"2");
}

void SISTests::Advanced1()
{
    TestCleanupWrapper cleanup([&]()
    {
        CleanupDevices();
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 420));

    CreateD3DDevice();
    CreateD2DDevice();

    // More complex scenario - 2 Canvas elements each gets filled with a SIS as its Background
    LOG_OUTPUT(L"Loading SIS2.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS2.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        Canvas^ myCanvas1 = safe_cast<Canvas^>(rootCanvas->FindName(L"myCanvas1"));
        Canvas^ myCanvas2 = safe_cast<Canvas^>(rootCanvas->FindName(L"myCanvas2"));
        int width = static_cast<int>(myCanvas1->Width);
        int height = static_cast<int>(myCanvas1->Height);
        SurfaceImageSource^ mySIS1 = ref new SurfaceImageSource(width, height);
        SurfaceImageSource^ mySIS2 = ref new SurfaceImageSource(width, height);
        ImageBrush^ myBrush1 = ref new ImageBrush();
        ImageBrush^ myBrush2 = ref new ImageBrush();
        myBrush1->ImageSource = mySIS1;
        myCanvas1->Background = myBrush1;
        myBrush2->ImageSource = mySIS2;
        myCanvas2->Background = myBrush2;

        // Fill both SIS's with ISurfaceImageSourceNativeWithD2D
        LOG_OUTPUT(L"Filling 2 SIS's");
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative1;
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative2;
        RECT updateRect = { 0, 0, width, height };
        spSISNative1 = GetSISNativeWithD2D(mySIS1);
        spSISNative2 = GetSISNativeWithD2D(mySIS2);
        VERIFY_SUCCEEDED(spSISNative1->SetDevice(m_d2dDevice.Get()));
        VERIFY_SUCCEEDED(spSISNative2->SetDevice(m_d2dDevice.Get()));
        DrawWithD2DSuspend(spSISNative1.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Red));
        DrawWithD2DSuspend(spSISNative2.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Red));
        VERIFY_SUCCEEDED(spSISNative1->ResumeDraw());
        VERIFY_SUCCEEDED(spSISNative1->EndDraw());
        VERIFY_SUCCEEDED(spSISNative2->EndDraw());
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"1");
}

void SISTests::NegativeCases()
{
    TestCleanupWrapper cleanup([&]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        CleanupDevices();
    });

    CreateD3DDevice();
    CreateD2DDevice();

    RunOnUIThread([&]()
    {
        bool testPass = false;
        try
        {
            // Not allowed to create negative sized SIS
            SurfaceImageSource^ mySIS = ref new SurfaceImageSource(-10, -10);
        }
        catch(Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });

    int width = 100;
    int height = 100;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case: Not allowed to call SetDevice while in drawing state");
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNative> spSISNative = GetSISNative(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));
        POINT offset;
        RECT rect = {0, 0, width, height};
        ComPtr<IDXGISurface> spSurface;
        VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, &spSurface, &offset));
        VERIFY_FAILED(spSISNative->SetDevice(m_dxgiDevice.Get()));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case: Not allowed to supply an update rect that's bigger than the SIS's bounds");
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNative> spSISNative = GetSISNative(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));
        POINT offset;
        RECT rect = {0, 0, width*2, height*2};
        ComPtr<IDXGISurface> spSurface;
        VERIFY_FAILED(spSISNative->BeginDraw(rect, &spSurface, &offset));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case: Not allowed to call BeginDraw while in drawing state");
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNative> spSISNative = GetSISNative(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));
        POINT offset;
        RECT rect = {0, 0, width, height};
        ComPtr<IDXGISurface> spSurface;
        VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, &spSurface, &offset));
        VERIFY_FAILED(spSISNative->BeginDraw(rect, &spSurface, &offset));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case: Not allowed to call BeginDraw on SIS while another SIS is in drawing state");
        SurfaceImageSource^ mySIS1 = ref new SurfaceImageSource(width, height);
        SurfaceImageSource^ mySIS2 = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNative> spSISNative1 = GetSISNative(mySIS1);
        ComPtr<ISurfaceImageSourceNative> spSISNative2 = GetSISNative(mySIS2);
        VERIFY_SUCCEEDED(spSISNative1->SetDevice(m_dxgiDevice.Get()));
        VERIFY_SUCCEEDED(spSISNative2->SetDevice(m_dxgiDevice.Get()));
        POINT offset;
        RECT rect = {0, 0, width, height};
        ComPtr<IDXGISurface> spSurface;
        VERIFY_SUCCEEDED(spSISNative1->BeginDraw(rect, &spSurface, &offset));
        VERIFY_FAILED(spSISNative2->BeginDraw(rect, &spSurface, &offset));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case: Not allowed to call EndDraw while not in drawing state");
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNative> spSISNative = GetSISNative(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));
        VERIFY_FAILED(spSISNative->EndDraw());
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case:  Not allowed to call SuspendDraw or ResumeDraw while not in drawing state");
        int width = 100;
        int height = 100;
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNativeWithD2D = GetSISNativeWithD2D(mySIS);
        VERIFY_SUCCEEDED(spSISNativeWithD2D->SetDevice(m_d2dDevice.Get()));
        VERIFY_FAILED(spSISNativeWithD2D->SuspendDraw());
        VERIFY_FAILED(spSISNativeWithD2D->ResumeDraw());
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case: Not allowed to mix ISurfaceImageSourceNative and ISurfaceImageSourceNativeWithD2D");
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        ComPtr<ISurfaceImageSourceNative> spSISNative = GetSISNative(mySIS);
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNativeWithD2D = GetSISNativeWithD2D(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));
        VERIFY_FAILED(spSISNativeWithD2D->SetDevice(m_dxgiDevice.Get()));
    });

    LOG_OUTPUT(L"Negative test case: Not allowed to call BeginDraw from background thread using ISurfaceImagesourceNativeWithD2D if D3D device is not multithreaded");
    SurfaceImageSource^ mySIS;
    ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNativeWithD2D;
    RunOnUIThread([&]()
    {
        mySIS = ref new SurfaceImageSource(width, height);
        spSISNativeWithD2D = GetSISNativeWithD2D(mySIS);
        VERIFY_SUCCEEDED(spSISNativeWithD2D->SetDevice(m_d2dDevice.Get()));
    });
    POINT offset;
    RECT rect = {0, 0, width, height};
    ComPtr<IDXGISurface> spSurface;
    VERIFY_FAILED(spSISNativeWithD2D->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spSurface, &offset));
}

void SISTests::SuspendFailureTestOldAPI()
{
    auto wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
        CleanupDevices();
    });

    CreateD3DDevice();
    CreateD2DDevice();

    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(100, 100);
        ComPtr<ISurfaceImageSourceNative> spSISNative = GetSISNative(mySIS);

        LOG_OUTPUT(L"Triggering simulated Suspend");
        wh->TriggerSuspend(true, true);

        // Now try to SetDevice
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));

        {
            LOG_OUTPUT(L"Begin to draw some content, this should fail");
            int width = 100, height = 100;
            POINT offset;
            RECT rect = { 0, 0, width, height };
            ComPtr<IDXGISurface> spSurface;
            VERIFY_FAILED(spSISNative->BeginDraw(rect, &spSurface, &offset));
        }

        LOG_OUTPUT(L"Triggering simulated Resume");
        wh->TriggerResume();

        // Now try to BeginDraw again - this time it should succeed.
        {
            LOG_OUTPUT(L"Begin to draw some content again, this time should succeed");
            int width = 100, height = 100;
            POINT offset;
            RECT rect = { 0, 0, width, height};
            ComPtr<IDXGISurface> spSurface;
            VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, &spSurface, &offset));
        }
    });
}

void SISTests::SuspendFailureTest()
{
    auto wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
        CleanupDevices();
    });

    CreateD3DDevice();
    CreateD2DDevice();

    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(100, 100);
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNativeWithD2D = GetSISNativeWithD2D(mySIS);

        LOG_OUTPUT(L"Triggering simulated Suspend");
        wh->TriggerSuspend(true, true);

        // Now try to SetDeviceWithD2D - this is expected to fail as it tries to create a DComp surface while device is in offered state.

        VERIFY_FAILED(spSISNativeWithD2D->SetDevice(m_d2dDevice.Get()));

        LOG_OUTPUT(L"Triggering simulated Resume");
        wh->TriggerResume();

        // Now try to SetDeviceWithD2D again - this time it should succeed.
        VERIFY_SUCCEEDED(spSISNativeWithD2D->SetDevice(m_d2dDevice.Get()));
    });
}

void SISTests::SuspendFailureTest2()
{
    auto wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
        CleanupDevices();
    });

    CreateD3DDevice();
    CreateD2DDevice();

    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(100, 100);
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative = GetSISNativeWithD2D(mySIS);

        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_d2dDevice.Get()));

        {
            ComPtr<ID2D1DeviceContext> spDeviceContext;
            RECT rect = {0,0,100,100};
            POINT offset;
            VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
            VERIFY_SUCCEEDED(spSISNative->EndDraw());
        }

        LOG_OUTPUT(L"Triggering simulated Suspend");
        wh->TriggerSuspend(true, true);

        {
            ComPtr<ID2D1DeviceContext> spDeviceContext;
            RECT rect = {0,0,100,100};
            POINT offset;
            HRESULT hr = spSISNative->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset);
            LOG_OUTPUT(L"BeginDraw returned HRESULT = 0x%x", hr);
            VERIFY_IS_TRUE(hr == E_FAIL);
        }

        LOG_OUTPUT(L"Triggering simulated Resume");
        wh->TriggerResume();

        {
            ComPtr<ID2D1DeviceContext> spDeviceContext;
            RECT rect = {0,0,100,100};
            POINT offset;
            VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
        }
    });
}

void SISTests::RegenerateVisual()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestCleanupWrapper cleanup([&]()
    {
        CleanupDevices();
    });

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    MockDComp::IMockDCompDevice^ mockDevice = wh->MockDCompDevice;
    MockDComp::IMockDCompDevice2^ mockDevice2 = safe_cast<MockDComp::IMockDCompDevice2^>(mockDevice);
    unsigned int spriteVisualCleanedUp;

    CreateD3DDevice();
    CreateD2DDevice();
    CreateD2DContext();

    int width = 200;
    int height = 200;

    ComPtr<ISurfaceImageSourceNative> spSISNative;
    ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNativeD2D;

    RunOnUIThread([&]()
    {
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(width, height);
        spSISNative = GetSISNative(mySIS);
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));

        ImageBrush^ myBrush = ref new ImageBrush();
        myBrush->ImageSource = mySIS;

        Canvas^ sisCanvas = ref new Canvas();
        sisCanvas->Width = width;
        sisCanvas->Height = height;
        sisCanvas->Background = myBrush;

        SurfaceImageSource^ myD2DSIS = ref new SurfaceImageSource(width, height);
        spSISNativeD2D = GetSISNativeWithD2D(myD2DSIS);
        VERIFY_SUCCEEDED(spSISNativeD2D->SetDevice(m_d2dDevice.Get()));

        ImageBrush^ myBrush2 = ref new ImageBrush();
        myBrush2->ImageSource = myD2DSIS;

        Canvas^ d2dSISCanvas = ref new Canvas();
        d2dSISCanvas->Width = width;
        d2dSISCanvas->Height = height;
        d2dSISCanvas->Background = myBrush2;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Margin = ThicknessHelper::FromUniformLength(20);
        rootCanvas->Children->Append(sisCanvas);
        rootCanvas->Children->Append(d2dSISCanvas);

        // Fill the entire SIS with a solid color using ISurfaceImageSourceNative
        LOG_OUTPUT(L"> Filling SISes");
        RECT updateRect = { 0, 0, width, height };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Red));
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Red));

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, 0); //because dcomp background is transparent

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Partial update of SIS, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width/2, height/2 };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update of SIS, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Yellow));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Clear and resetting the SIS device");
        VERIFY_SUCCEEDED(spSISNative->SetDevice(nullptr));
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_dxgiDevice.Get()));

        LOG_OUTPUT(L"> Full update of SIS, 1 additional sprite visual should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Partial update with D2D, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { width/2, height/2, width, height }; // LTRB
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update with D2D, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Clear and resetting the SIS device");
        VERIFY_SUCCEEDED(spSISNativeD2D->SetDevice(nullptr));
        VERIFY_SUCCEEDED(spSISNativeD2D->SetDevice(m_d2dDevice.Get()));

        LOG_OUTPUT(L"> Full update of SIS, 1 additional sprite visual should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Triggering simulated Suspend");
        wh->TriggerSuspend(true, true);

        {
            ComPtr<ID2D1DeviceContext> spDeviceContext;
            RECT rect = {0,0,100,100};
            POINT offset;
            HRESULT hr = spSISNativeD2D->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset);
            LOG_OUTPUT(L"> BeginDraw returned HRESULT = 0x%x", hr);
            VERIFY_IS_TRUE(hr == E_FAIL);
        }

        LOG_OUTPUT(L"> Triggering simulated Resume");
        wh->TriggerResume();

        LOG_OUTPUT(L"> Full update with D2D, 2 additional sprite visuals should have been cleaned up because we're forcing a redraw");
        RECT updateRect = { 0, 0, width, height };
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Blue));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 2);

    auto surfaceContentsLostEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
    ::Windows::Foundation::EventRegistrationToken surfaceContentsLostEventToken;
    RunOnUIThread([&]()
    {
        auto onSurfaceContentsLost = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            surfaceContentsLostEvent->Set();
            Microsoft::UI::Xaml::Media::CompositionTarget::SurfaceContentsLost::remove(surfaceContentsLostEventToken);
        });
        surfaceContentsLostEventToken = Microsoft::UI::Xaml::Media::CompositionTarget::SurfaceContentsLost::add(onSurfaceContentsLost);
    });

    LOG_OUTPUT(L"> Device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);

    LOG_OUTPUT(L"> SurfaceContentsLost event should have been raised, 2 additional sprite visuals should have been cleaned up (and replaced with surfaceless brush visuals)");
    surfaceContentsLostEvent->WaitForDefault();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update of SIS, 1 additional sprite visual should have been cleaned up for the first BeginDraw after a device lost");
        RECT updateRect = { 0, 0, width, height };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Orange));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update of SIS, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        Draw(spSISNative.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Orange));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update with D2D, we should get E_SURFACE_CONTENTS_LOST because we didn't set another device");

        ComPtr<ID2D1DeviceContext> spDeviceContext;
        RECT rect = {0,0,100,100};
        POINT offset;
        HRESULT hr = spSISNativeD2D->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset);
        LOG_OUTPUT(L"> BeginDraw returned HRESULT = 0x%x", hr);
        VERIFY_IS_TRUE(hr == E_SURFACE_CONTENTS_LOST);
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update with D2D, 1 additional sprite visual should have been cleaned up for the first BeginDraw after a device lost");
        RECT updateRect = { 0, 0, width, height };
        VERIFY_SUCCEEDED(spSISNativeD2D->SetDevice(m_d2dDevice.Get()));
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update with d2d, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        DrawWithD2D(spSISNativeD2D.Get(), updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);
}


unsigned int SISTests::VerifySpriteVisualsCleanedUp(MockDComp::IMockDCompDevice2^ mockDevice2, unsigned int expected)
{
    unsigned int actual;
    RunOnUIThread([&]()
    {
        mockDevice2->GetWUCSpriteVisualsEverUnparentedCount(&actual);
        VERIFY_ARE_EQUAL(expected, actual);
        LOG_OUTPUT(L">");
    });
    return actual;
}

void SISTests::OfferReclaimChangeSFReleaseBeforeReclaim()
{
    auto wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
        CleanupDevices();
    });

    CreateD3DDevice();
    CreateD2DDevice();

    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        //create a new SIS element
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(100, 100);
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative = GetSISNativeWithD2D(mySIS);

        //setDevice
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_d2dDevice.Get()));

        //drawing
        {
            ComPtr<ID2D1DeviceContext> spDeviceContext;
            RECT rect = { 0,0,100,100 };
            POINT offset;
            VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
            VERIFY_SUCCEEDED(spSISNative->EndDraw());
        }

        //Trigger suspend and offer SurfaceFactory
        LOG_OUTPUT(L"Triggering Suspend");
        wh->TriggerSuspend(true, true);

        //pass NULL to setDevice to release SF
        VERIFY_SUCCEEDED(spSISNative->SetDevice(NULL));

        //resume, make sure this is successful
        //because the released SF should not be resumed
        LOG_OUTPUT(L"Triggering Resume");
        wh->TriggerResume();
    });

}

void SISTests::OfferInDrawingState()
{
    auto wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup([&]()
    {
        wh->ResetWindowContentAndWaitForIdle();
        CleanupDevices();
    });

    CreateD3DDevice();
    CreateD2DDevice();

    LOG_OUTPUT(L"Loading SIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SIS1.xaml"));
    Canvas^ myCanvas;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        //create a new SIS element
        SurfaceImageSource^ mySIS = ref new SurfaceImageSource(100, 100);
        ComPtr<ISurfaceImageSourceNativeWithD2D> spSISNative = GetSISNativeWithD2D(mySIS);

        //setDevice
        VERIFY_SUCCEEDED(spSISNative->SetDevice(m_d2dDevice.Get()));

        //drawing
        {
            ComPtr<ID2D1DeviceContext> spDeviceContext;
            RECT rect = { 0,0,100,100 };
            POINT offset;
            VERIFY_SUCCEEDED(spSISNative->BeginDraw(rect, __uuidof(ID2D1DeviceContext), &spDeviceContext, &offset));
        }

        //Trigger suspend and offer SurfaceFactory
        //The goal is to make sure the offer does not
        //crash, no need to check anything
        LOG_OUTPUT(L"Triggering Suspend");
        wh->TriggerSuspend(true, true);

        //resume, make sure this is successful
        LOG_OUTPUT(L"Triggering Resume");
        wh->TriggerResume();
    });

}

} } } } } }
