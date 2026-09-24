// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LoadedImageSurfaceTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "SafeEventRegistration.h"
#include "WUCRenderingScopeGuard.h"
#include <WindowsNumerics.h>

using namespace ::Windows::Foundation::Numerics;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

using namespace test_infra;
using namespace MockDComp;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics::Image;

namespace
{
    class LoadedImageSurfaceVerifier
    {
    public:
        // ctor must run on ui thread
        LoadedImageSurfaceVerifier(LoadedImageSurface^ loadedImageSurface)
            : m_loadedImageSurface(std::move(loadedImageSurface))
        {
            VERIFY_IS_NOT_NULL(m_loadedImageSurface);
            m_loadCompletedRegistration.Attach(m_loadedImageSurface,
                ref new wf::TypedEventHandler<LoadedImageSurface^, LoadedImageSourceLoadCompletedEventArgs^>(
                    [this](Platform::Object ^sender, LoadedImageSourceLoadCompletedEventArgs^ args)
            {
                LOG_OUTPUT(L"> Received LoadCompleted event");
                VERIFY_ARE_EQUAL(m_loadedImageSurface, sender);
                VERIFY_IS_NOT_NULL(args);
                if (m_checkLoadStatus)
                {
                    LOG_OUTPUT(L"  > Checking LoadCompleted status");
                    VERIFY_ARE_EQUAL(m_expectedStatus, args->Status);
                }
                m_loadCompletedEvent->Set();
            }));

            VERIFY_IS_FALSE(m_loadCompletedEvent->HasFired());
        }

        void SetExpectedStatus(LoadedImageSourceLoadStatus expectedStatus)
        {
            m_expectedStatus = expectedStatus;
        }

        void SetCheckLoadStatus(bool check)
        {
            m_checkLoadStatus = check;
        }

        void WaitLoaded()
        {
            m_loadCompletedEvent->WaitForDefault();
        }

        void ResetLoaded()
        {
            m_loadCompletedEvent->Reset();
        }

        xaml_shapes::Rectangle^ CreateRectangleWithSpriteVisual()
        {
            xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();

            Compositor^ compositor = ElementCompositionPreview::GetElementVisual(rect)->Compositor;

            SpriteVisual^ spriteVisual = compositor->CreateSpriteVisual();
            spriteVisual->Size = {100, 100};
            spriteVisual->Brush = compositor->CreateSurfaceBrush(m_loadedImageSurface);

            ElementCompositionPreview::SetElementChildVisual(rect, spriteVisual);
            return rect;
        }

        void PutToXamlTree()
        {
            RunOnUIThread([this]()
            {
                TestServices::WindowHelper->WindowContent = CreateRectangleWithSpriteVisual();
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        // Bushy tree inserts LIS-rendered surfaces into a collection container. Used for testing scenarios with multiple LIS requests.
        void PutToBushyXamlTree()
        {
            RunOnUIThread([this]()
            {
                xaml_controls::StackPanel^ contentAsStackPanel = dynamic_cast<xaml_controls::StackPanel^>(TestServices::WindowHelper->WindowContent);

                // Add top-level StackPanel if one doesn't exist already
                if (contentAsStackPanel == nullptr)
                {
                    contentAsStackPanel = ref new xaml_controls::StackPanel();
                    TestServices::WindowHelper->WindowContent = contentAsStackPanel;
                }

                xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
                rect->Width = 100;
                rect->Height = 100;
                contentAsStackPanel->Children->Append(rect);

                Compositor^ compositor = ElementCompositionPreview::GetElementVisual(rect)->Compositor;

                SpriteVisual^ spriteVisual = compositor->CreateSpriteVisual();
                spriteVisual->Size = {100, 100};
                spriteVisual->Brush = compositor->CreateSurfaceBrush(m_loadedImageSurface);

                ElementCompositionPreview::SetElementChildVisual(rect, spriteVisual);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        LoadedImageSurface^ GetSurface() const { return m_loadedImageSurface; }

    private:
        std::shared_ptr<Event> m_loadCompletedEvent = std::make_shared<Event>();
        SafeEventRegistrationType(LoadedImageSurface, LoadCompleted) m_loadCompletedRegistration =
            CreateSafeEventRegistration(LoadedImageSurface, LoadCompleted);
        LoadedImageSurface^ m_loadedImageSurface;
        LoadedImageSourceLoadStatus m_expectedStatus = LoadedImageSourceLoadStatus::Success;
        bool m_checkLoadStatus = true;
    };
}
Platform::String^ LoadedImageSurfaceTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
}

bool LoadedImageSurfaceTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();

    return true;
}

bool LoadedImageSurfaceTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool LoadedImageSurfaceTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void LoadedImageSurfaceTests::CreateFromUri()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CreateFromUriWithSize()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 400), scale);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {100, 100}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(150, 113), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(150 / scale, 113 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CreateFromUriWithSizeAndAtlasHint()
{
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting atlas hint");
        xaml::Window^ xamlWindow = xaml::Window::Current;
        xaml::IWindowPrivate^ windowPrivate = dynamic_cast<xaml::IWindowPrivate^>(xamlWindow);
        windowPrivate->SetAtlasSizeHint(256, 256);
    });

    auto exitGuard = wil::scope_exit([&]
    {
        // Need to reset the atlas size hint in order to avoid affecting other tests
        TestServices::Utilities->ResetAtlasSizeHint();
    });

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {100, 100}));
    });

    verifier->WaitLoaded();
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying atlas was correctly hinted");
        MockDComp::IMockDCompDevice^ mockDevice = TestServices::WindowHelper->MockDCompDevice;
        MockDComp::IMockDCompDevice2^ mockDevice2 = safe_cast<MockDComp::IMockDCompDevice2^>(mockDevice);
        ::Windows::Foundation::Size hintSize;
        mockDevice2->GetAtlasHintSize(&hintSize);
        VERIFY_ARE_EQUAL(hintSize.Width, 256);
        VERIFY_ARE_EQUAL(hintSize.Height, 256);
    });

    verifier->PutToXamlTree();

    RunOnUIThread([&]()
    {
        ::Windows::Foundation::Size decodedPhysicalSize = verifier->GetSurface()->DecodedPhysicalSize;
        ::Windows::Foundation::Size decodedSize = verifier->GetSurface()->DecodedSize;

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(100, 75), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(100, 75), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);

}

void LoadedImageSurfaceTests::CreateFromUriWithSizeVirtual()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 400), scale);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        // Note the requested size times scale may still be less than virtual threshold but we have to consider scale
        // multipliers of up to 7 when making the decision. 1000*7 clearly exceeds the virtual threshold.
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {1000, 1000}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500, 1125), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500 / scale, 1125 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CreateFromStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CreateFromStreamWithSize()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 400), scale);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream, {100, 100}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(150, 113), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(150 / scale, 113 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CreateFromStreamWithSizeVirtual()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 400), scale);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        // Note the requested size times scale may still be less than virtual threshold but we have to consider scale
        // multipliers of up to 7 when making the decision. 1000*7 clearly exceeds the virtual threshold.
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream, {1000, 1000}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500, 1125), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500 / scale, 1125 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::FirstUseAfterLoaded()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier->WaitLoaded();
    verifier->PutToXamlTree();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedSize);
    });

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CloseStreamWhileStillLoading()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));

        // Depending on the Moon phase loading may or may not complete successfully if the stream was closed while loading
        verifier->SetCheckLoadStatus(false);

        // In C++/CX operator delete on "hat" calls the object's IClosable::Close
        delete bitmapStream;
    });

    verifier->WaitLoaded();
}

void LoadedImageSurfaceTests::CloseStreamAfterLoaded()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier->WaitLoaded();

    // In C++/CX operator delete on "hat" calls the object's IClosable::Close
    delete bitmapStream;

    // Subsequent reloads should succeed with the closed stream
    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 400), scale);

    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->DecodedSize);
    });
}

void LoadedImageSurfaceTests::CloseCreatedFromStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));

        // In C++/CX operator delete on "hat" calls the object's IClosable::Close
        delete verifier->GetSurface();
    });
}

void LoadedImageSurfaceTests::CloseCreatedFromUri()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));

        // In C++/CX operator delete on "hat" calls the object's IClosable::Close
        delete verifier->GetSurface();
    });
}

void LoadedImageSurfaceTests::CloseAfterLoadCompleted()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"barcelona.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        // In C++/CX operator delete on "hat" calls the object's IClosable::Close
        delete verifier->GetSurface();
        VERIFY_THROWS_WINRT(verifier->GetSurface()->NaturalSize, Platform::ObjectDisposedException^);
    });
}

void LoadedImageSurfaceTests::DeviceLostRecover()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"barcelona.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier->WaitLoaded();

    TestServices::WindowHelper->SimulateDeviceLost();

    verifier->WaitLoaded();
    verifier->PutToXamlTree();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::DeviceLostInUse()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"barcelona.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    TestServices::WindowHelper->SimulateDeviceLost();

    verifier->WaitLoaded();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::DeviceLostInUseWindowHidden()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"barcelona.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    // Simulate device lost while the window is hidden.  Verify we don't crash when we resume.
    wh->TriggerSuspend(true, true);
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    wh->TriggerResume();

    verifier->WaitLoaded();

    wh->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::DeviceLostWindowHiddenBeforeLoad()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    // Simulate device lost while the window is hidden.  Verify we correctly handle creating
    // wrapper surface without a D3D device and create hardware surface after recovering.
    LOG_OUTPUT(L"Simulating losing device while window is hidden");
    wh->TriggerSuspend(true, true);
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        // At this point we have no D3D device.  Create a LoadedImageSurface and put it in the tree.
        LOG_OUTPUT(L"Creating LoadedImageSurface");
        auto uri = ref new Uri(GetResourcesPath() + L"barcelona.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });
    verifier->PutToXamlTree();

    LOG_OUTPUT(L"Triggering resume");
    wh->TriggerResume();
    verifier->WaitLoaded();

    wh->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::DeviceLostBeforeLoad()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;
    Uri^ uri;

    RunOnUIThread([&]()
    {
        // Load the first LoadedImageSurface.  This is just to populate the cache with decoded bits.
        LOG_OUTPUT(L"Loading first LoadedImageSurface");
        uri = ref new Uri(GetResourcesPath() + L"barcelona.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier1->WaitLoaded();
    verifier1->PutToXamlTree();

    RunOnUIThread([&]()
    {
        // Now simulate device lost and immediately create another LoadedImageSurface with the same URI.
        // This will hit the cache and allow us to test handling device lost in the OnImageViewUpdated code path.
        LOG_OUTPUT(L"Simulating device lost");
        wh->SimulateDeviceLost();

        LOG_OUTPUT(L"Loading second LoadedImageSurface with cached decoded bits, expect no crash");
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();
    verifier2->PutToXamlTree();

    // The order of decoding completion and recovering from device lost are not guaranteed, which can produce instability
    // of surface ID's if we use allocation order.  Controlling the order would require a complicated test hook,
    // and this test isn't focused on this aspect, so use CRC mode surface verification as a work-around.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnlyCRC);
}

void LoadedImageSurfaceTests::NonExistingUri()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"wtf.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
        verifier->SetExpectedStatus(LoadedImageSourceLoadStatus::NetworkError);
    });

    verifier->WaitLoaded();
}

void LoadedImageSurfaceTests::InvalidFormatFromUri()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"InvalidImage.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
        verifier->SetExpectedStatus(LoadedImageSourceLoadStatus::InvalidFormat);
    });

    verifier->WaitLoaded();
}

void LoadedImageSurfaceTests::PlateauScaleChange()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {1000, 1000}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "100");

    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 300), scale);

    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500, 1125), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500 / scale, 1125 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "150");
}

void LoadedImageSurfaceTests::PlateauScaleChangeDuringDownload()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {1000, 1000}));
    });

    verifier->PutToXamlTree();

    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 300), scale);

    verifier->WaitLoaded();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500, 1125), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500 / scale, 1125 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::PlateauScaleChangeStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    // Use PNG instead of JPEG. PNG is lossless so its Surface CRC is stable across
    // OS versions, while JPEG decode rounding can vary between builds.
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"rainier_2048x1536.png");
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream, {1000, 1000}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "100");

    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 300), scale);

    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500, 1125), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(1500 / scale, 1125 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "150");
}

void LoadedImageSurfaceTests::ExceedPlateauScaleLimit()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        // with the current scale limit 292*7=2044 which is just below the virtual threshold
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {292, 292}));
    });

    verifier->PutToXamlTree();
    verifier->WaitLoaded();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "100");

    // Exceed the max scale but the surface size should still stay within virtual limits
    float scale = 8;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 300), scale);

    verifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048 / scale, 1536 / scale), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2044, 1533), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2044 / scale, 1533 / scale), verifier->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "800");
}

void LoadedImageSurfaceTests::PlateauScaleChangeMRT()
{
#if 0
// TAEF image tests cannot access MRT resources
// Commenting out actual test body as it takes non-trivial amount of time and causes false positives on wildcard runs

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(L"ms-appx:///resources/native/external/foundation/graphics/image/testmrt.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier->WaitLoaded();
    verifier->PutToXamlTree();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "100");

    float scale = 2.0f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 300), scale);

    verifier->WaitLoaded();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "200");

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(250, 219), verifier->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(500, 438), verifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(250, 219), verifier->GetSurface()->DecodedSize);
    });
#endif
}

void LoadedImageSurfaceTests::JumboImageFromUri()
{
#if 0
// There is no reliable way to handle OOM in LoadedImageSurface
// Commenting out actual test body as it takes non-trivial amount of time and causes false positives on wildcard runs

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"65535.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
        verifier->SetExpectedStatus(LoadedImageSourceLoadStatus::Other);
    });

    verifier->WaitLoaded();
#endif
}

void LoadedImageSurfaceTests::LowMemoryStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"barcelona.png");

    RunOnUIThread([&]()
    {
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier->WaitLoaded();
    verifier->PutToXamlTree();

    ICompositionSurface ^before = TestServices::WindowHelper->GetRealCompositionSurface(verifier->GetSurface());

    LOG_OUTPUT(L"Triggering Suspend and disabling render");
    TestServices::WindowHelper->TriggerSuspend(true, true);
    TestServices::WindowHelper->SetIsRenderEnabled(false);

    LOG_OUTPUT(L"Triggering low memory condition");
    TestServices::WindowHelper->TriggerLowMemory();
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Triggering resume and re-enabling render");
    TestServices::WindowHelper->SetIsRenderEnabled(true);
    TestServices::WindowHelper->TriggerResume();

    verifier->WaitLoaded();

    ICompositionSurface ^after = TestServices::WindowHelper->GetRealCompositionSurface(verifier->GetSurface());

    VERIFY_ARE_EQUAL(before, after);

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void LoadedImageSurfaceTests::CreatePairFromSameUri()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(2048, 1536), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LoadedImageSurfaceTests::CreatePairFromSameUriWithSize()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {256, 256}));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {256, 256}));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LoadedImageSurfaceTests::CreatePairFromSameUriWithAndWithoutSize()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {256, 256}));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}


void LoadedImageSurfaceTests::CreatePairFromDifferentUris()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    RunOnUIThread([&]()
    {
        auto uri1 = ref new Uri(GetResourcesPath() + L"windows.png");
        auto uri2 = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri1));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri2));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LoadedImageSurfaceTests::CreatePairFromUriAndStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"windows.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LoadedImageSurfaceTests::CreatePairFromUriWithSizeAndStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"windows.png");

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {256, 256}));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(421, 332), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LoadedImageSurfaceTests::CreatePairForSameImageFromUriAndStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier1;
    std::shared_ptr<LoadedImageSurfaceVerifier> verifier2;

    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");

    RunOnUIThread([&]()
    {
        auto uri = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        verifier1 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri));
        verifier2 = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream));
    });

    verifier1->PutToBushyXamlTree();
    verifier2->PutToBushyXamlTree();

    verifier1->WaitLoaded();
    verifier2->WaitLoaded();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier1->GetSurface()->DecodedSize);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->NaturalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(256, 256), verifier2->GetSurface()->DecodedSize);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void LoadedImageSurfaceTests::UnofferDeviceBeforeImageResize()
{
    TestCleanupWrapper cleanup;

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;

    //create a window content with a rectangle
    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        TestServices::WindowHelper->WindowContent = rect;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Begin to load image");
        auto uri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, { 1000, 1000 }));
    });
    verifier->WaitLoaded();
    TestServices::WindowHelper->WaitForIdle();

    //Trigger suspend to guarantee that device is offered before we resize
    LOG_OUTPUT(L"Triggering suspend");
    TestServices::WindowHelper->TriggerSuspend(true, true);

    //Setting the plateau to a new scale requires resize to the images. A new decode process
    //will be operated on the image and we need to resize the surface of the image. This is where
    //the original bug happened. We need to make sure that we could handle this case so we change
    //the plateau scale here.
    float scale = 1.5f;
    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(::Windows::Foundation::Size(400, 300), scale);
    verifier->WaitLoaded();

    LOG_OUTPUT(L"Resuming from suspending");
    TestServices::WindowHelper->TriggerResume();

    TestServices::WindowHelper->WaitForIdle();
}

void LoadedImageSurfaceTests::ResizeLargerWhileDecodeIsPending()
{
    // CLoadedImageSurface::GetDecodePixelSize clamps at 1x scale. Use scales smaller than 1x.
    ResizeWhileDecodeIsPending(0.6f, 0.7f, 0.8f, 0.9f);
}

void LoadedImageSurfaceTests::ResizeSmallerWhileDecodeIsPending()
{
    // CLoadedImageSurface::GetDecodePixelSize clamps at 1x scale. Use scales smaller than 1x.
    ResizeWhileDecodeIsPending(1.0f, 0.9f, 0.8f, 0.7f);
}

void LoadedImageSurfaceTests::ResizeWhileDecodeIsPending(float scale1, float scale2, float scale3, float scale4)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    LOG_OUTPUT(L"> Initial scale is %.1f.", scale1);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale1);

    xaml_controls::Canvas^ root;
    std::shared_ptr<LoadedImageSurfaceVerifier> uriVerifier;
    std::shared_ptr<LoadedImageSurfaceVerifier> streamVerifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Initializing tree.");
        root = ref new Canvas();
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Suspending off-thread image decoding.");
        wh->SetSuspendOffThreadDecoding(true);

        auto uri = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        uriVerifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {200, 200}));

        streamVerifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream, {200, 200}));
    });

    LOG_OUTPUT(L"> Adding LoadedImageSurface to tree.");
    RunOnUIThread([&]()
    {
        root->Children->Append(uriVerifier->CreateRectangleWithSpriteVisual());
        root->Children->Append(streamVerifier->CreateRectangleWithSpriteVisual());
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Resuming off-thread image decoding. We have 3 seconds after this to update the root scale.");
    wh->SetSuspendOffThreadDecoding(false);

    LOG_OUTPUT(L"> Updating root scale to %.1f. This will issue another decode while the first is pending. Don't crash.", scale2);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale2);

    LOG_OUTPUT(L"> Waiting for LoadedImageSurface.");
    uriVerifier->WaitLoaded();
    streamVerifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        float expectedDimension = 200 * scale2;
        LOG_OUTPUT(L"> Expecting LoadedImageSurfaces to decode at %.2fx%.2f.", expectedDimension, expectedDimension);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), uriVerifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), streamVerifier->GetSurface()->DecodedPhysicalSize);
    });

    LOG_OUTPUT(L"> Suspending off-thread image decoding again.");
    wh->SetSuspendOffThreadDecoding(true);

    LOG_OUTPUT(L"> Updating root scale to %.1f. This will issue a decode.", scale3);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale3);

    LOG_OUTPUT(L"> Resuming off-thread image decoding. We have 3 seconds after this to update the root scale.");
    wh->SetSuspendOffThreadDecoding(false);

    LOG_OUTPUT(L"> Updating root scale to %.1f. This will issue another decode while the first is pending. Don't crash.", scale4);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale4);

    LOG_OUTPUT(L"> Waiting for LoadedImageSurface.");
    uriVerifier->WaitLoaded();
    streamVerifier->WaitLoaded();

    RunOnUIThread([&]()
    {
        float expectedDimension = 200 * scale4;
        LOG_OUTPUT(L"> Expecting LoadedImageSurfaces to decode at %.2fx%.2f.", expectedDimension, expectedDimension);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), uriVerifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), streamVerifier->GetSurface()->DecodedPhysicalSize);
    });

    wh->WaitForIdle();
}

void LoadedImageSurfaceTests::ResizeLargerWhileSurfaceUpdateIsPending()
{
    // CLoadedImageSurface::GetDecodePixelSize clamps at 1x scale. Use scales smaller than 1x.
    ResizeWhileSurfaceUpdateIsPending(0.6f, 0.7f, 0.8f, 0.9f);
}

void LoadedImageSurfaceTests::ResizeSmallerWhileSurfaceUpdateIsPending()
{
    // CLoadedImageSurface::GetDecodePixelSize clamps at 1x scale. Use scales smaller than 1x.
    ResizeWhileSurfaceUpdateIsPending(1.0f, 0.9f, 0.8f, 0.7f);
}

void LoadedImageSurfaceTests::ResizeWhileSurfaceUpdateIsPending(float scale1, float scale2, float scale3, float scale4)
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    LOG_OUTPUT(L"> Initial scale is %.1f.", scale1);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale1);

    xaml_controls::Canvas^ root;
    std::shared_ptr<LoadedImageSurfaceVerifier> uriVerifier;
    std::shared_ptr<LoadedImageSurfaceVerifier> streamVerifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Initializing tree.");
        root = ref new Canvas();
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Suspending surface updates.");
        wh->SetSuspendSurfaceUpdates(true);

        auto uri = ref new Uri(GetResourcesPath() + L"NoiseAsset_256X256_PNG.png");
        uriVerifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromUri(uri, {200, 200}));

        streamVerifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream, {200, 200}));
    });

    LOG_OUTPUT(L"> Adding LoadedImageSurfaces to tree. Wait for the first decode to complete and add a pending surface update.");
    RunOnUIThread([&]()
    {
        root->Children->Append(uriVerifier->CreateRectangleWithSpriteVisual());
        root->Children->Append(streamVerifier->CreateRectangleWithSpriteVisual());
    });
    wh->WaitForIdle();
    uriVerifier->WaitLoaded();
    streamVerifier->WaitLoaded();
    uriVerifier->ResetLoaded();
    streamVerifier->ResetLoaded();

    LOG_OUTPUT(L"> Updating root scale to %.1f. Wait for the second decode to add a pending surface update.", scale2);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale2);
    uriVerifier->WaitLoaded();
    streamVerifier->WaitLoaded();
    uriVerifier->ResetLoaded();
    streamVerifier->ResetLoaded();

    LOG_OUTPUT(L"> Resuming surface updates and flush them...");
    wh->SetSuspendSurfaceUpdates(false);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  ...and dirty the tree to trigger another frame. Don't crash.");
        TestServices::WindowHelper->WindowContent->Opacity = 0.5;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        float expectedDimension = 200 * scale2;
        LOG_OUTPUT(L"> Expecting LoadedImageSurfaces to decode at %.2fx%.2f.", expectedDimension, expectedDimension);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), uriVerifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), streamVerifier->GetSurface()->DecodedPhysicalSize);
    });
    LOG_OUTPUT(L"> Suspending surface updates again.");
    wh->SetSuspendSurfaceUpdates(true);

    LOG_OUTPUT(L"> Updating root scale to %.1f. Wait for the third decode to add a pending surface update.", scale3);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale3);
    uriVerifier->WaitLoaded();
    streamVerifier->WaitLoaded();
    uriVerifier->ResetLoaded();
    streamVerifier->ResetLoaded();

    LOG_OUTPUT(L"> Updating root scale to %.1f. Wait for the fourth decode to add a pending surface update.", scale4);
    wh->SetWindowSizeOverrideWithScale(wf::Size(), scale4);
    uriVerifier->WaitLoaded();
    streamVerifier->WaitLoaded();
    uriVerifier->ResetLoaded();
    streamVerifier->ResetLoaded();

    LOG_OUTPUT(L"> Resuming surface updates and flush them...");
    wh->SetSuspendSurfaceUpdates(false);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  ...and dirty the tree to trigger another frame. Don't crash.");
        TestServices::WindowHelper->WindowContent->Opacity = 1.0;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        float expectedDimension = 200 * scale4;
        LOG_OUTPUT(L"> Expecting LoadedImageSurfaces to decode at %.2fx%.2f.", expectedDimension, expectedDimension);

        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), uriVerifier->GetSurface()->DecodedPhysicalSize);
        VERIFY_ARE_EQUAL(::Windows::Foundation::Size(expectedDimension, expectedDimension), streamVerifier->GetSurface()->DecodedPhysicalSize);
    });

    wh->WaitForIdle();
}

void LoadedImageSurfaceTests::LoadAfterDeviceLostOnStartup()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    wh->SetWindowSizeOverrideWithScale(wf::Size(400, 300), 2.0f);

    std::shared_ptr<LoadedImageSurfaceVerifier> verifier;
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"80x80Checker.png");

    RunOnUIThread([&]()
    {
        // This test requires that there is no dispatch of Tick between simulating
        // device lost and the StartLoadFromStream() call completing.
        LOG_OUTPUT(L"> Pausing dispatch.");
        wh->PauseNewDispatchForTest();

        LOG_OUTPUT(L"> Simulating device lost on startup.");
        u->SimulateSwallowedDeviceLostOnStartup();

        // Run a short message pump (32 ms) to give an opportunity to tick if PauseNewDispatch
        // didn't work. This loop is mostly to test PauseNewDispatchForTest().
        auto start = ::GetTickCount();
        while (::GetTickCount() - start < 32)
        {
            MSG msg = {};
            if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::DispatchMessage(&msg);
            }
            else
            {
                ::Sleep(1);
            }
        }

        LOG_OUTPUT(L"> Loading LoadedImageSurface. Don't crash.");
        verifier = std::make_shared<LoadedImageSurfaceVerifier>(LoadedImageSurface::StartLoadFromStream(bitmapStream, {80, 80}));

        // Resume dispatch now that StartLoadFromStream is completed. It should have resulted in
        // a device lost failure in DCompSurface::InitializeSurface, which WinUI should recover
        // from on the next tick for the rest of the test.
        LOG_OUTPUT(L"> Resuming dispatch.");
        wh->ResumeNewDispatchForTest();
    });

    LOG_OUTPUT(L"> Adding LoadedImageSurface to tree.");
    verifier->PutToXamlTree();

    LOG_OUTPUT(L"> Waiting for LoadedImageSurface.");
    verifier->WaitLoaded();

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

