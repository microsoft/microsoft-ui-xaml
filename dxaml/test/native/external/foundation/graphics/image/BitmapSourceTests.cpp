// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "BitmapSourceTests.h"
#include <WUCRenderingScopeGuard.h>

using namespace Concurrency;
using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics { namespace Image {

Platform::String^ BitmapSourceTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
}

bool BitmapSourceTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool BitmapSourceTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool BitmapSourceTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void BitmapSourceTests::SetSource()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    LOG_OUTPUT(L"Getting stream of image");
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"barcelona.png");

    LOG_OUTPUT(L"Creating custom BitmapSource with SetSource");
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto bitmapSource = ref new ::Tests::Foundation::Graphics::Image::TestBitmapSource();
        VERIFY_IS_NOT_NULL(bitmapSource);
        bitmapSource->SetSource(bitmapStream);

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);
        testImage->Stretch = Stretch::Fill;
        testImage->Source = bitmapSource;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"SetSource");

    LOG_OUTPUT(L"Simulating device lost");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->ResetMockDCompSurfaceId();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"SetSource");
}

void BitmapSourceTests::SetSourceAsync()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    LOG_OUTPUT(L"Getting stream of image");
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"barcelona.png");

    auto sourceAsyncCompletionEvent = std::make_shared<Event>();

    ::Tests::Foundation::Graphics::Image::TestBitmapSource^ bitmapSource = nullptr;

    LOG_OUTPUT(L"Creating custom BitmapSource with SetSourceAsync");
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        bitmapSource = ref new ::Tests::Foundation::Graphics::Image::TestBitmapSource();
        VERIFY_IS_NOT_NULL(bitmapSource);

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);
        testImage->Stretch = Stretch::Fill;
        testImage->Source = bitmapSource;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        create_task(bitmapSource->SetSourceAsync(bitmapStream))
            .then([=]()
        {
            sourceAsyncCompletionEvent->Set();
        });
    });
    sourceAsyncCompletionEvent->WaitForDefault();

     // Tick the UI thread so that it processes any EnsureAndUpdateHardwareResources.
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } } }
