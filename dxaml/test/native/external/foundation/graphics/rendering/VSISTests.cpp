// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VSISTests.h"
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

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

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

// Helper class to register for UpdatesNeeded and draw to the VSIS (Virtual Surface Image Source)
class VSISRenderer : public ComObject<IVirtualSurfaceUpdatesCallbackNative>
{
public:
    void SetDxgiDevice()
    {
        VERIFY_SUCCEEDED(m_pVSISNative->SetDevice(m_dxgiDevice.Get()));
    }

    VSISRenderer(IVirtualSurfaceImageSourceNative* pVSISNative)
    : m_pVSISNative(pVSISNative)
    , m_nextUpdateColor(D2D1::ColorF::DeepPink)
    {
        CreateD3DDevice();
        CreateD2DDevice();
        CreateD2DContext();
        SetDxgiDevice();
        VERIFY_SUCCEEDED(m_pVSISNative->RegisterForUpdatesNeeded(this));
    }

    ~VSISRenderer()
    {
        m_pVSISNative->RegisterForUpdatesNeeded(nullptr);
        CleanupDevices();
    }

    void CreateD3DDevice()
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

        ComPtr<ID3D10Multithread> spD3DMultiThread;
        VERIFY_SUCCEEDED(m_d3dDevice.As(&spD3DMultiThread));
        spD3DMultiThread->SetMultithreadProtected(TRUE);
    }

    void CreateD2DDevice()
    {
        VERIFY_SUCCEEDED(D2D1CreateDevice(m_dxgiDevice.Get(), nullptr, &m_d2dDevice));
    }

    void CreateD2DContext()
    {
        VERIFY_SUCCEEDED(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext));
    }

    void CleanupDevices()
    {
        m_d2dContext = nullptr;
        m_d2dDevice = nullptr;
        m_dxgiDevice = nullptr;
        m_d3dDevice = nullptr;
    }

    void Draw(ISurfaceImageSourceNative* pSIS, RECT rect, D2D1::ColorF color)
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

    HRESULT STDMETHODCALLTYPE UpdatesNeeded()
    {
        HRESULT hr = S_OK;
        try
        {
            DWORD dwRectCount;
            VERIFY_SUCCEEDED(m_pVSISNative->GetUpdateRectCount(&dwRectCount));
            VERIFY_IS_TRUE(dwRectCount == 1);

            RECT updateRect;
            VERIFY_SUCCEEDED(m_pVSISNative->GetUpdateRects(&updateRect, 1));
            VERIFY_IS_TRUE(updateRect == m_nextExpectedUpdateRect);
            Draw(m_pVSISNative, updateRect, m_nextUpdateColor);
        }
        catch (...)
        {
            hr = E_FAIL;
        }
        VERIFY_SUCCEEDED(hr);
        return hr;
    }

    void ScheduleRedraw(RECT rect, D2D1::ColorF color)
    {
        m_nextExpectedUpdateRect = rect;
        m_nextUpdateColor = color;
        VERIFY_SUCCEEDED(m_pVSISNative->Invalidate(rect));
    }

    void ExpectRedraw(RECT rect, D2D1::ColorF color)
    {
        m_nextExpectedUpdateRect = rect;
        m_nextUpdateColor = color;
    }

    RECT GetVisibleBounds()
    {
        RECT visibleBounds;
        VERIFY_SUCCEEDED(m_pVSISNative->GetVisibleBounds(&visibleBounds));
        return visibleBounds;
    }

    void Resize(int width, int height)
    {
        VERIFY_SUCCEEDED(m_pVSISNative->Resize(width, height));
    }

private:
    IVirtualSurfaceImageSourceNative* m_pVSISNative;
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<IDXGIDevice> m_dxgiDevice;
    ComPtr<ID2D1Device> m_d2dDevice;
    ComPtr<ID2D1DeviceContext> m_d2dContext;
    RECT m_nextExpectedUpdateRect;
    D2D1::ColorF m_nextUpdateColor;
};

Platform::String^ VSISTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool VSISTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool VSISTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool VSISTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void VSISTests::Basics1()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    // Load simple scenario XAML - a Canvas that we'll fill with a single VSIS as its Background
    LOG_OUTPUT(L"Loading VSIS1.xaml");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"VSIS1.xaml"));
    ComPtr<VSISRenderer> spVSISRenderer;
    int width, height;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        Canvas^ myCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"myCanvas"));
        width = static_cast<int>(myCanvas->Width);
        height = static_cast<int>(myCanvas->Height);
        VirtualSurfaceImageSource^ myVSIS = ref new VirtualSurfaceImageSource(width, height);
        ImageBrush^ myBrush = ref new ImageBrush();
        myBrush->ImageSource = myVSIS;
        myCanvas->Background = myBrush;

        // Fill entire VSIS with a solid color
        LOG_OUTPUT(L"Filling VSIS with Red");
        ComPtr<IVirtualSurfaceImageSourceNative> spVSISNative;
        (reinterpret_cast<IUnknown*>(myVSIS))->QueryInterface(IID_PPV_ARGS(&spVSISNative));
        spVSISRenderer.Attach(new VSISRenderer(spVSISNative.Get()));
        RECT updateRect = {0, 0, width, height};
        spVSISRenderer->ExpectRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Red));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"1");

    RunOnUIThread([&]()
    {
        // Do a partial update by invalidating a portion of the VSIS
        LOG_OUTPUT(L"Filling a portion of the VSIS");
        RECT updateRect = {0, 0, width/2, height/2};
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"2");

    RunOnUIThread([&]()
    {
        // Verify the visible bounds is as expected
        RECT expectedBounds = {0, 0, width, height};
        RECT visibleBounds = spVSISRenderer->GetVisibleBounds();
        VERIFY_IS_TRUE(visibleBounds == expectedBounds);
    });

    RunOnUIThread([&]()
    {
        // Resize the VSIS and redraw the new bounds
        int newWidth = width*2;
        int newHeight = height*2;
        LOG_OUTPUT(L"Resizing VSIS");
        spVSISRenderer->Resize(newWidth, newHeight);
        RECT updateRect = {0, 0, newWidth, newHeight};
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Yellow));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"3");
}

void VSISTests::Advanced1()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    // More complex scenario - a ScrollViewer with a Canvas filled with a VSIS as its Background
    // In this scenario we're primarily exercising the VSIS tiling code.  Tiles have a size of 504x504.
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"VSIS2.xaml"));
    ScrollViewer^ sv;
    ComPtr<VSISRenderer> spVSISRenderer;
    int widthSV, heightSV;
    int widthVSIS, heightVSIS;
    int tileSize = 504;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        Canvas^ myCanvas = safe_cast<Canvas^>(rootCanvas->FindName(L"myCanvas"));
        sv = safe_cast<ScrollViewer^>(rootCanvas->FindName(L"myScrollViewer"));
        widthVSIS = static_cast<int>(myCanvas->Width);
        heightVSIS = static_cast<int>(myCanvas->Height);
        widthSV = static_cast<int>(sv->Width);
        heightSV = static_cast<int>(sv->Height);
        VirtualSurfaceImageSource^ myVSIS = ref new VirtualSurfaceImageSource(widthVSIS, heightVSIS);
        ImageBrush^ myBrush = ref new ImageBrush();
        myBrush->ImageSource = myVSIS;
        myCanvas->Background = myBrush;

        // Fill the appropriate area of the VSIS - in this case we expect 4 tiles as a single update rect
        LOG_OUTPUT(L"Filling VSIS tiles with Red");
        ComPtr<IVirtualSurfaceImageSourceNative> spVSISNative;
        (reinterpret_cast<IUnknown*>(myVSIS))->QueryInterface(IID_PPV_ARGS(&spVSISNative));
        spVSISRenderer.Attach(new VSISRenderer(spVSISNative.Get()));
        RECT updateRect = {0, 0, tileSize*2, tileSize*2};
        spVSISRenderer->ExpectRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Red));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"1");

    RunOnUIThread([&]()
    {
        // Scroll the ScrollViewer to the bottom-right edge of the Canvas
        LOG_OUTPUT(L"Scrolling and redrawing VSIS");
        double scrollX = static_cast<double>(widthVSIS - widthSV);
        double scrollY = static_cast<double>(heightVSIS - heightSV);
        sv->ChangeView(scrollX, scrollY, 1.0f, true /*disableAnimation*/);
        RECT updateRect = {tileSize*2, tileSize*2, widthVSIS, heightVSIS};
        spVSISRenderer->ExpectRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly, L"2");
}

// Test case:  Validates the visible bounds is correctly computed in special-case scenario:
// When multiple VSIS's share a common ancestor, an optimization runs to cache and share
// the results from computing the visible bounds of that common ancestor.
// This test case produces that sort of element tree, runs through that optimization code,
// and validates that we generate the expected visible bounds of both VSIS's.
void VSISTests::MultiVSISBounds()
{
    TestCleanupWrapper cleanup([&]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

    LOG_OUTPUT(L"Loading markup");
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"MultiVSIS.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    VirtualSurfaceImageSource^ vsis1 = nullptr;
    VirtualSurfaceImageSource^ vsis2 = nullptr;
    int widthVSIS = 0;
    int heightVSIS = 0;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating VSIS's");
        Canvas^ canvas1 = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas1"));
        Canvas^ canvas2 = safe_cast<Canvas^>(rootCanvas->FindName(L"canvas2"));
        widthVSIS = static_cast<int>(canvas1->Width);
        heightVSIS = static_cast<int>(canvas1->Height);

        vsis1 = ref new VirtualSurfaceImageSource(widthVSIS, heightVSIS);
        ImageBrush^ brush1 = ref new ImageBrush();
        brush1->ImageSource = vsis1;
        canvas1->Background = brush1;

        vsis2 = ref new VirtualSurfaceImageSource(widthVSIS, heightVSIS);
        ImageBrush^ brush2 = ref new ImageBrush();
        brush2->ImageSource = vsis2;
        canvas2->Background = brush2;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        {
            LOG_OUTPUT(L"Validating visible bounds of VSIS #1");
            ComPtr<IVirtualSurfaceImageSourceNative> vsisNative;
            (reinterpret_cast<IUnknown*>(vsis1))->QueryInterface(IID_PPV_ARGS(&vsisNative));
            RECT visibleBounds;
            VERIFY_SUCCEEDED(vsisNative->GetVisibleBounds(&visibleBounds));
            RECT expectedBounds = {0, 0, widthVSIS, heightVSIS};
            VERIFY_IS_TRUE(visibleBounds == expectedBounds);
        }
        {
            LOG_OUTPUT(L"Validating visible bounds of VSIS #2");
            ComPtr<IVirtualSurfaceImageSourceNative> vsisNative;
            (reinterpret_cast<IUnknown*>(vsis2))->QueryInterface(IID_PPV_ARGS(&vsisNative));
            RECT visibleBounds;
            VERIFY_SUCCEEDED(vsisNative->GetVisibleBounds(&visibleBounds));
            RECT expectedBounds = {0, 0, 0, 0};
            VERIFY_IS_TRUE(visibleBounds == expectedBounds);
        }
    });
}

void VSISTests::NegativeCases()
{
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Negative test case:  Not allowed to Invalidate an area larger than the VSIS");
        int width = 1000;
        int height = 1000;
        VirtualSurfaceImageSource^ myVSIS = ref new VirtualSurfaceImageSource(width, height);

        ComPtr<IVirtualSurfaceImageSourceNative> spVSISNative;
        (reinterpret_cast<IUnknown*>(myVSIS))->QueryInterface(IID_PPV_ARGS(&spVSISNative));
        RECT updateRect = {0, 0, width*2, height*2};
        VERIFY_FAILED(spVSISNative->Invalidate(updateRect));
    });
}

void VSISTests::RegenerateVisual()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    wh->SetWindowSizeOverride(wf::Size(400, 400));

    MockDComp::IMockDCompDevice^ mockDevice = wh->MockDCompDevice;
    MockDComp::IMockDCompDevice2^ mockDevice2 = safe_cast<MockDComp::IMockDCompDevice2^>(mockDevice);
    unsigned int spriteVisualCleanedUp;

    ComPtr<VSISRenderer> spVSISRenderer;
    int width = 200;
    int height = 200;

    ComPtr<IVirtualSurfaceImageSourceNative> spVSISNative;

    RunOnUIThread([&]()
    {
        VirtualSurfaceImageSource^ myVSIS = ref new VirtualSurfaceImageSource(width, height);

        ImageBrush^ myBrush = ref new ImageBrush();
        myBrush->ImageSource = myVSIS;

        Canvas^ vsisCanvas = ref new Canvas();
        vsisCanvas->Width = width;
        vsisCanvas->Height = height;
        vsisCanvas->Background = myBrush;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Margin = ThicknessHelper::FromUniformLength(20);
        rootCanvas->Children->Append(vsisCanvas);

        LOG_OUTPUT(L"Filling VSIS with Red");
        (reinterpret_cast<IUnknown*>(myVSIS))->QueryInterface(IID_PPV_ARGS(&spVSISNative));
        spVSISRenderer.Attach(new VSISRenderer(spVSISNative.Get()));
        RECT updateRect = {0, 0, width, height};
        spVSISRenderer->ExpectRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Red));

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, 0);  //because dcomp background is transparent

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Partial update of VSIS, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width/2, height/2 };
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update of VSIS, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Green));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        width = width*2;
        height = height*2;
        LOG_OUTPUT(L"> Resizing VSIS, 1 additional sprite visual should have been cleaned up"); // Maybe Xaml doesn't have to dirty, but comp probably redraws everything anyway
        spVSISRenderer->Resize(width, height);
        RECT updateRect = {0, 0, width, height};
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Yellow));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Clear and resetting the SIS device");
        VERIFY_SUCCEEDED(spVSISNative->SetDevice(nullptr));
        spVSISRenderer->SetDxgiDevice();

        LOG_OUTPUT(L"> Full update of VSIS, 1 additional sprite visual should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Yellow));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

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

    LOG_OUTPUT(L"> SurfaceContentsLost event should have been raised, 1 additional sprite visuals should have been cleaned up. The VSIS should have already recovered");
    surfaceContentsLostEvent->WaitForDefault();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp + 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Partial update of VSIS, no additional sprite visuals should have been cleaned up. The VSIS should have already recovered as part of the callback right after device lost");
        RECT updateRect = { width/2, height/2, width, height };
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Yellow));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Full update of SIS, no additional sprite visuals should have been cleaned up");
        RECT updateRect = { 0, 0, width, height };
        spVSISRenderer->ScheduleRedraw(updateRect, D2D1::ColorF(D2D1::ColorF::Yellow));
    });
    wh->WaitForIdle();
    spriteVisualCleanedUp = VerifySpriteVisualsCleanedUp(mockDevice2, spriteVisualCleanedUp);
}


unsigned int VSISTests::VerifySpriteVisualsCleanedUp(MockDComp::IMockDCompDevice2^ mockDevice2, unsigned int expected)
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

} } } } } }
