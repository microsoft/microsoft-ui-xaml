// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <limits>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "MUX-ETWEvents.h"
#include "etwwaiterproxy.h"
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <XamlFileTestEngine.h>
#include "SVGImageSourceTests.h"
#include <UIAutomationHelper.h>
#include <AutomationClient\AutomationClientManager.h>

#undef GetClassName // Conflicts with automation peer method of the same name

using namespace Concurrency;
using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Printing;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

// TODO: Try to remove synchronous ticks in these tests and make sure they are 100% reliable

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics { namespace Image {

class SvgImageEventWaitingContext
{
public:
    void Attach(xaml_imaging::SvgImageSource^ svgImageSource)
    {
        m_openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<xaml_imaging::SvgImageSource^, xaml_imaging::SvgImageSourceOpenedEventArgs^>(
                [&] (Platform::Object^ sender, xaml_imaging::SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"Opened event fired");
            m_opened = true;
            m_imageEvent->Set();
        }));

        m_failedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<xaml_imaging::SvgImageSource^, xaml_imaging::SvgImageSourceFailedEventArgs^>(
                [&] (Platform::Object^ sender, xaml_imaging::SvgImageSourceFailedEventArgs^)
        {
            LOG_OUTPUT(L"OpenFailed event fired");
            m_failed = true;
            m_imageEvent->Set();
        }));
    }

    void Detach()
    {
        m_openedRegistration.Detach();
        m_failedRegistration.Detach();
    }

    void WaitOpened()
    {
        LOG_OUTPUT(L"Waiting for an image event");
        m_imageEvent->WaitForDefault();
        VERIFY_IS_FALSE(m_failed);
        VERIFY_IS_TRUE(m_opened);
    }

    bool IsOpened() const
    {
        return m_opened;
    }

private:
    SafeEventRegistrationType(xaml_imaging::SvgImageSource, Opened) m_openedRegistration =
        CreateSafeEventRegistration(xaml_imaging::SvgImageSource, Opened);
    SafeEventRegistrationType(xaml_imaging::SvgImageSource, OpenFailed) m_failedRegistration =
        CreateSafeEventRegistration(xaml_imaging::SvgImageSource, OpenFailed);
    std::shared_ptr<Common::Event> m_imageEvent = std::make_shared<Common::Event>();
    bool m_opened = false;
    bool m_failed = false;
};

Platform::String^ SvgImageSourceTests::GetResourcesPath()
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
}

bool SvgImageSourceTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SvgImageSourceTests::TestCleanup()
{
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SvgImageSourceTests::SetSourceAsync()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    LOG_OUTPUT(L"Getting stream of image");
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"msft.svg");

    auto sourceAsyncCompletionEvent = std::make_shared<Event>();

    SvgImageSource^ svgImageSource = nullptr;

    wf::IAsyncOperation<Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus>^ setSourceAsyncOperation = nullptr;
    Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus status;

    LOG_OUTPUT(L"Creating SvgImageSource with SetSourceAsync");
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        svgImageSource = ref new SvgImageSource();
        VERIFY_IS_NOT_NULL(svgImageSource);

        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);
        testImage->Stretch = Stretch::None;
        testImage->Source = svgImageSource;
        svgImageSource->RasterizePixelWidth = 100;
        svgImageSource->RasterizePixelHeight = 100;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {

        setSourceAsyncOperation = svgImageSource->SetSourceAsync(bitmapStream);

        auto setSourceCallback = ref new wf::AsyncOperationCompletedHandler<Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus>(
            [&status, sourceAsyncCompletionEvent](wf::IAsyncOperation<Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"SetSourceAsync operation completed for svg");
            status = operation->GetResults();
            sourceAsyncCompletionEvent->Set();
        });
        VERIFY_IS_NOT_NULL(setSourceCallback);
        setSourceAsyncOperation->Completed = setSourceCallback;

    });
    sourceAsyncCompletionEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(status, Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus::Success);
     // Tick the UI thread so that it processes any EnsureAndUpdateHardwareResources.
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);

    // Set a jpg file to the SvgImageSource, expected to see SvgImageSourceLoadStatus::InvalidFormat here.
    LOG_OUTPUT(L"Getting stream of jpg image");
    bitmapStream = LoadBinaryFile(GetResourcesPath() + L"barcelona.jpg");

    RunOnUIThread([&]()
    {
        setSourceAsyncOperation = svgImageSource->SetSourceAsync(bitmapStream);

        auto setSourceCallback = ref new wf::AsyncOperationCompletedHandler<Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus>(
            [&status, sourceAsyncCompletionEvent](wf::IAsyncOperation<Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"SetSourceAsync operation completed for jpg");
            status = operation->GetResults();
            sourceAsyncCompletionEvent->Set();
        });
        VERIFY_IS_NOT_NULL(setSourceCallback);
        setSourceAsyncOperation->Completed = setSourceCallback;

    });

    sourceAsyncCompletionEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(status, Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus::InvalidFormat);
}

// Test case: Renders a simple image element.
void SvgImageSourceTests::UriSource()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    SvgImageSource^ svgImageSource = nullptr;
    SvgImageSource^ svgImageSourceCreatedWithUri = nullptr;
    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto openedRegistration2 = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageSourceOpenedEvent = std::make_shared<Event>();

    auto openFailedRegistration = CreateSafeEventRegistration(SvgImageSource, OpenFailed);
    auto svgImageSourceOpenFailedEvent = std::make_shared<Event>();
    SvgImageSourceLoadStatus status = SvgImageSourceLoadStatus::Success;
    xaml_controls::Image^ testImage = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->Stretch = Stretch::Fill;

        svgImageSource = ref new SvgImageSource();
        VERIFY_IS_NOT_NULL(svgImageSource);

        testImage->Source = svgImageSource;

        auto testUri = ref new Uri(GetResourcesPath() + L"msft.svg");
        VERIFY_IS_NOT_NULL(testUri);

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageSourceOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"Svg Opened Event Fired");
            svgImageSourceOpenedEvent->Set();
        }));

        svgImageSource->UriSource = testUri;
        svgImageSource->RasterizePixelWidth = 100;
        svgImageSource->RasterizePixelHeight = 100;
    });
    svgImageSourceOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenedEvent->HasFired());
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

    // Set to a jpg file, expected to see InvalidFormat.
    RunOnUIThread([&]()
    {
        auto testUri = ref new Uri(GetResourcesPath() + L"barcelona.jpg");
        openFailedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceFailedEventArgs^>([&status,svgImageSourceOpenFailedEvent](SvgImageSource^, SvgImageSourceFailedEventArgs^ args)
        {
            LOG_OUTPUT(L"Svg OpenFailed Event Fired");
            svgImageSourceOpenFailedEvent->Set();
            status = args->Status;
        }));
        svgImageSource->UriSource = testUri;
    });
    svgImageSourceOpenFailedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenFailedEvent->HasFired());
    VERIFY_ARE_EQUAL(status, Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus::InvalidFormat);

    // Set to an uri that is not available, expect to see NetworkError.
    RunOnUIThread([&]()
    {
        auto testUri = ref new Uri(GetResourcesPath() + L"blah.svg");
        svgImageSource->UriSource = testUri;
    });
    svgImageSourceOpenFailedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenFailedEvent->HasFired());
    VERIFY_ARE_EQUAL(status, Microsoft::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus::NetworkError);

    // Test the constructor with Uri.
    RunOnUIThread([&]()
    {
        svgImageSourceCreatedWithUri = ref new SvgImageSource(ref new Uri(GetResourcesPath() + L"msft.svg"));
        svgImageSourceCreatedWithUri->RasterizePixelWidth = 100;
        svgImageSourceCreatedWithUri->RasterizePixelHeight = 100;
        testImage->Source = svgImageSourceCreatedWithUri;
        openedRegistration2.Attach(
            svgImageSourceCreatedWithUri,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageSourceOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"Svg Opened Event Fired");
            svgImageSourceOpenedEvent->Set();
        }));
    });
    svgImageSourceOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly,L"2");
}

// Renders a simple image element using a relative path to reference the image.
void SvgImageSourceTests::SimpleImageElementRelativePath()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithSvgUri.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        auto svgImageSource = safe_cast<SvgImageSource^>(testImage->Source);

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageOpenedEvent->HasFired());
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}


void SvgImageSourceTests::ExplicitXaml()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithSvgUriExplicitSyntax.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        auto svgImageSource = safe_cast<SvgImageSource^>(testImage->Source);

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    // TODO: this renders correctly on screen, but there is an error when doing the below DComp comparsion.
    //TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SvgImageSourceTests::ImageBrush()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageBrushWithSvg.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto imageBrush = safe_cast<Microsoft::UI::Xaml::Media::ImageBrush^>(rootGrid->FindName(L"imageBrush"));
        VERIFY_IS_NOT_NULL(imageBrush);

        auto svgImageSource = safe_cast<SvgImageSource^>(imageBrush->ImageSource);

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SvgImageSourceTests::BitmapCache()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithSvgUri.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->CacheMode = ref new Microsoft::UI::Xaml::Media::BitmapCache();
        auto svgImageSource = safe_cast<SvgImageSource^>(testImage->Source);

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageOpenedEvent->HasFired());
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);

}

void SvgImageSourceTests::Printing()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithSvgUri.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    PrintDocument^ printDoc;
    Platform::Object^ printSource;

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        auto svgImageSource = safe_cast<SvgImageSource^>(testImage->Source);

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageOpenedEvent->HasFired());

    RunOnUIThread([&] () {
        printDoc = ref new PrintDocument();
        printSource = printDoc->DocumentSource;

        printDoc->AddPages += ref new AddPagesEventHandler(
            [&](Platform::Object^ sender, AddPagesEventArgs^ args)
            {
                printDoc->AddPage(safe_cast<UIElement^>(rootGrid));
                printDoc->AddPagesComplete();
            });
    });

     TestServices::WindowHelper->SynchronouslyTickUIThread(2);
     TestServices::WindowHelper->WaitForIdle();

     // Test is disabled due to:
     // TODO: Re-enable printing tests that were disabled as part of lifting Xaml tests.
     //TestServices::Utilities->VerifyPrinting(printSource);
}

void SvgImageSourceTests::PlateauScale()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 2.0f);
    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithSvgUri.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        auto svgImageSource = safe_cast<SvgImageSource^>(testImage->Source);
        svgImageSource->RasterizePixelWidth = 100.0;
        svgImageSource->RasterizePixelHeight = 100.0;
        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageOpenedEvent->HasFired());
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SvgImageSourceTests::AutomationPeerDefault()
{
    SvgAutomationTest(
        GetResourcesPath() + L"SimpleImage.xaml",
        GetResourcesPath() + L"msft.svg",
        L"Image",
        L"",
        L"",
        xaml_automation_peers::AutomationControlType::Image);
}

void SvgImageSourceTests::AutomationPeerNoContent()
{
    SvgAutomationTest(
        GetResourcesPath() + L"SimpleImage.xaml",
        nullptr,
        L"Image",
        L"",
        L"",
        xaml_automation_peers::AutomationControlType::Image);
}

void SvgImageSourceTests::AutomationSvgProperties()
{
    SvgAutomationTest(
        GetResourcesPath() + L"SimpleImage.xaml",
        GetResourcesPath() + L"msft_automation.svg",
        L"Image",
        L"Microsoft SVG",
        L"Microsoft SVG Description",
        xaml_automation_peers::AutomationControlType::Image);
}

void SvgImageSourceTests::AutomationPeerPropertiesOverride()
{
    SvgAutomationTest(
        GetResourcesPath() + L"SimpleImageAutomation.xaml",
        GetResourcesPath() + L"msft_automation.svg",
        L"Image",
        L"Simple Image Name",
        L"Simple Image Description",
        xaml_automation_peers::AutomationControlType::Image);
}

void SvgImageSourceTests::StretchMode()
{
    std::pair<xaml_media::Stretch, Platform::String^> modes[] = {
        { xaml_media::Stretch::None, L"None" },
        { xaml_media::Stretch::Fill, L"Fill" },
        { xaml_media::Stretch::Uniform, L"Uniform" },
        { xaml_media::Stretch::UniformToFill, L"UniformToFill" },
    };

    for (auto& mode : modes)
    {
        SimpleSvgTest(
            [&] (xaml_controls::Image^ image, xaml_imaging::SvgImageSource^ svgImageSource)
            {
                image->Stretch = mode.first;
            },
            mode.second);
    }
}

void SvgImageSourceTests::RasterizePixelWidth()
{
    SimpleSvgTest(
        [&] (xaml_controls::Image^ image, xaml_imaging::SvgImageSource^ svgImageSource)
        {
            svgImageSource->RasterizePixelWidth = 100.0;
        },
        nullptr);
}

void SvgImageSourceTests::RasterizePixelHeight()
{
    SimpleSvgTest(
        [&] (xaml_controls::Image^ image, xaml_imaging::SvgImageSource^ svgImageSource)
        {
            svgImageSource->RasterizePixelHeight = 100.0;
        },
        nullptr);
}

void SvgImageSourceTests::RasterizePixelWidthAndHeight()
{
    SimpleSvgTest(
        [&] (xaml_controls::Image^ image, xaml_imaging::SvgImageSource^ svgImageSource)
        {
            VERIFY_IS_TRUE(svgImageSource->RasterizePixelWidth == std::numeric_limits<double>::infinity());
            VERIFY_IS_TRUE(svgImageSource->RasterizePixelHeight == std::numeric_limits<double>::infinity());

            svgImageSource->RasterizePixelWidth = 100.0;
            svgImageSource->RasterizePixelHeight = 100.0;
        },
        nullptr);
}

void SvgImageSourceTests::RasterizeResizeDown()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    SvgImageEventWaitingContext eventWaitingContext;

    xaml_controls::Image ^image;

    RunOnUIThread([&]
    {
        image = ref new xaml_controls::Image();
        TestServices::WindowHelper->WindowContent = image;

        SvgImageSource ^svgImageSource = ref new SvgImageSource();

        eventWaitingContext.Attach(svgImageSource);
        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"TestPattern_Large.svg");

        image->Source = svgImageSource;
        image->Width = 200.0;
        image->Height = 200.0;
    });

    eventWaitingContext.WaitOpened();
    TestServices::WindowHelper->WaitForIdle();

    ETWWaiterProxy imageEtwWaiter;
    imageEtwWaiter.Start(
        WINDOWS_UI_XAML_ETW_PROVIDER,
        ImageUpdateHardwareResourcesEnd_value);

    RunOnUIThread([&]
    {
        image->Width = 100.0;
        image->Height = 100.0;
    });

    imageEtwWaiter.WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SvgImageSourceTests::DeviceLost()
{
    TestCleanupWrapper cleanup;
    SvgImageEventWaitingContext eventWaitingContext;
    XamlFileTestEngine engine;

    engine.SetXamlFilePath(GetResourcesPath() + L"SimpleImage.xaml");
    engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));

        auto svgImageSource = ref new SvgImageSource();
        testImage->Source = svgImageSource;
        eventWaitingContext.Attach(svgImageSource);
        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"msft.svg");
    });
    engine.SetPostInitWaitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        eventWaitingContext.WaitOpened();
        TestServices::WindowHelper->SimulateDeviceLost();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    });
    engine.Execute();
}

SvgImageSource^ SvgImageSourceTests::MakeSvgImageSource(
    Platform::String^ fileName,
    std::vector<std::shared_ptr<Event>>& svgOpenedEvents,
    std::vector<SafeEventRegistrationType(SvgImageSource, Opened)>& svgEventRegistrations)
{
    SvgImageSource^ svgImageSource = ref new SvgImageSource();
    svgImageSource->UriSource = ref new Uri(GetResourcesPath() + fileName);

    auto& event = svgOpenedEvents.emplace_back(std::make_shared<Event>());

    auto svgOpened = CreateSafeEventRegistration(SvgImageSource, Opened);
    svgOpened.Attach(
        svgImageSource,
        [event]() { event->Set(); });
    svgEventRegistrations.push_back(std::move(svgOpened));

    return svgImageSource;
}

xaml_controls::Image^ SvgImageSourceTests::MakeSvgImageElement(
    Platform::String^ fileName,
    double width,
    double height,
    std::vector<std::shared_ptr<Event>>& svgOpenedEvents,
    std::vector<SafeEventRegistrationType(SvgImageSource, Opened)>& svgEventRegistrations)
{
    xaml_controls::Image^ image = ref new xaml_controls::Image();
    image->Width = width;
    image->Height = height;
    image->Margin = ThicknessHelper::FromUniformLength(2);
    image->Source = MakeSvgImageSource(fileName, svgOpenedEvents, svgEventRegistrations);
    return image;
}

xaml_controls::Border^ SvgImageSourceTests::MakeElementWithSvgImageBrush(
    Platform::String^ fileName,
    double width,
    double height,
    std::vector<std::shared_ptr<Event>>& svgOpenedEvents,
    std::vector<SafeEventRegistrationType(SvgImageSource, Opened)>& svgEventRegistrations)
{
    xaml_media::ImageBrush^ imageBrush = ref new xaml_media::ImageBrush();
    imageBrush->ImageSource = MakeSvgImageSource(fileName, svgOpenedEvents, svgEventRegistrations);

    xaml_controls::Border^ border = ref new xaml_controls::Border();
    border->Width = width;
    border->Height = height;
    border->Margin = ThicknessHelper::FromUniformLength(2);
    border->Background = imageBrush;
    return border;
}

Canvas^ SvgImageSourceTests::MakeMarkerCanvas(float width)
{
    Canvas^ canvas = ref new Canvas();
    canvas->Width = width;
    canvas->Height = 10;
    canvas->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    return canvas;
}

void SvgImageSourceTests::DocumentWidthHeight()
{
    auto windowHelper = TestServices::WindowHelper;
    auto utilities = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    windowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(600, 600), 1.5f);

    std::vector<std::shared_ptr<Event>> svgOpenedEvents;
    std::vector<SafeEventRegistrationType(SvgImageSource, Opened)> svgEventRegistrations;

    RunOnUIThread([&]()
    {
        Grid^ root = ref new Grid();

        //
        // Put each SVG in a horizontal stack panel, then vertically stack the SVGs. This way, new files will be added
        // at the bottom and won't affect the MockDComp file numbers of existing test cases.
        //
        // For each test case, we have:
        // - Image, no explicit size
        // - Image, explicit width
        // - Image, explicit height
        // - Image, explicit width/height
        // - ImageBrush, no explicit size on Border
        // - ImageBrush, explicit width on Border
        // - ImageBrush, explicit height on Border
        // - ImageBrush, explicit width/height on Border
        //
        // Note: There's a bug here - Xaml will use the size of the root of the Xaml tree to decode, rather than respect the natural size of the SVG.
        // Xaml doesn't correctly infer natural size from SVG image
        //

        Canvas^ meet = ref new Canvas();
        meet->Children->Append(MakeMarkerCanvas(100));
        meet->Children->Append(MakeSvgImageElement(L"ryg-meet.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeSvgImageElement(L"ryg-meet.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeSvgImageElement(L"ryg-meet.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeSvgImageElement(L"ryg-meet.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        meet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(meet);

        Canvas^ slice = ref new Canvas();
        slice->Children->Append(MakeMarkerCanvas(200));
        slice->Children->Append(MakeSvgImageElement(L"ryg-slice.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeSvgImageElement(L"ryg-slice.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeSvgImageElement(L"ryg-slice.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeSvgImageElement(L"ryg-slice.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-slice.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-slice.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-slice.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        slice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-slice.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(slice);

        Canvas^ hMeet = ref new Canvas();
        hMeet->Children->Append(MakeMarkerCanvas(300));
        hMeet->Children->Append(MakeSvgImageElement(L"ryg-h-meet.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeSvgImageElement(L"ryg-h-meet.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeSvgImageElement(L"ryg-h-meet.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeSvgImageElement(L"ryg-h-meet.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        hMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(hMeet);

        Canvas^ wSlice = ref new Canvas();
        wSlice->Children->Append(MakeMarkerCanvas(400));
        wSlice->Children->Append(MakeSvgImageElement(L"ryg-w-slice.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeSvgImageElement(L"ryg-w-slice.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeSvgImageElement(L"ryg-w-slice.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeSvgImageElement(L"ryg-w-slice.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-w-slice.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-w-slice.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-w-slice.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        wSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-w-slice.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(wSlice);

        Canvas^ whMeet = ref new Canvas();
        whMeet->Children->Append(MakeMarkerCanvas(500));
        whMeet->Children->Append(MakeSvgImageElement(L"ryg-wh-meet.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeSvgImageElement(L"ryg-wh-meet.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeSvgImageElement(L"ryg-wh-meet.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeSvgImageElement(L"ryg-wh-meet.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-meet.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-meet.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-meet.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        whMeet->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-meet.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(whMeet);

        Canvas^ whSlice = ref new Canvas();
        whSlice->Children->Append(MakeMarkerCanvas(600));
        whSlice->Children->Append(MakeSvgImageElement(L"ryg-wh-slice.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeSvgImageElement(L"ryg-wh-slice.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeSvgImageElement(L"ryg-wh-slice.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeSvgImageElement(L"ryg-wh-slice.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        whSlice->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(whSlice);

        Canvas^ noViewBox = ref new Canvas();
        noViewBox->Children->Append(MakeMarkerCanvas(700));
        noViewBox->Children->Append(MakeSvgImageElement(L"ryg-noviewbox.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeSvgImageElement(L"ryg-noviewbox.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeSvgImageElement(L"ryg-noviewbox.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeSvgImageElement(L"ryg-noviewbox.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeElementWithSvgImageBrush(L"ryg-noviewbox.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeElementWithSvgImageBrush(L"ryg-noviewbox.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeElementWithSvgImageBrush(L"ryg-noviewbox.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        noViewBox->Children->Append(MakeElementWithSvgImageBrush(L"ryg-noviewbox.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(noViewBox);

        Canvas^ meetBig = ref new Canvas();
        meetBig->Children->Append(MakeMarkerCanvas(800));
        meetBig->Children->Append(MakeSvgImageElement(L"ryg-meet-big.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeSvgImageElement(L"ryg-meet-big.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeSvgImageElement(L"ryg-meet-big.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeSvgImageElement(L"ryg-meet-big.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet-big.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet-big.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet-big.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        meetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-meet-big.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(meetBig);

        Canvas^ hMeetBig = ref new Canvas();
        hMeetBig->Children->Append(MakeMarkerCanvas(900));
        hMeetBig->Children->Append(MakeSvgImageElement(L"ryg-h-meet-big.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeSvgImageElement(L"ryg-h-meet-big.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeSvgImageElement(L"ryg-h-meet-big.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeSvgImageElement(L"ryg-h-meet-big.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet-big.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet-big.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet-big.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        hMeetBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-h-meet-big.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(hMeetBig);

        Canvas^ whSliceBig = ref new Canvas();
        whSliceBig->Children->Append(MakeMarkerCanvas(1000));
        whSliceBig->Children->Append(MakeSvgImageElement(L"ryg-wh-slice-big.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeSvgImageElement(L"ryg-wh-slice-big.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeSvgImageElement(L"ryg-wh-slice-big.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeSvgImageElement(L"ryg-wh-slice-big.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice-big.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice-big.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice-big.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        whSliceBig->Children->Append(MakeElementWithSvgImageBrush(L"ryg-wh-slice-big.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(whSliceBig);

        Canvas^ negative = ref new Canvas();
        negative->Children->Append(MakeMarkerCanvas(1100));
        negative->Children->Append(MakeSvgImageElement(L"ryg-negative.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeSvgImageElement(L"ryg-negative.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeSvgImageElement(L"ryg-negative.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeSvgImageElement(L"ryg-negative.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        negative->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(negative);

        Canvas^ negative2 = ref new Canvas();
        negative2->Children->Append(MakeMarkerCanvas(1200));
        negative2->Children->Append(MakeSvgImageElement(L"ryg-negative2.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeSvgImageElement(L"ryg-negative2.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeSvgImageElement(L"ryg-negative2.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeSvgImageElement(L"ryg-negative2.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative2.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative2.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative2.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        negative2->Children->Append(MakeElementWithSvgImageBrush(L"ryg-negative2.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(negative2);

        Canvas^ positive = ref new Canvas();
        positive->Children->Append(MakeMarkerCanvas(1300));
        positive->Children->Append(MakeSvgImageElement(L"ryg-positive.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeSvgImageElement(L"ryg-positive.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeSvgImageElement(L"ryg-positive.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeSvgImageElement(L"ryg-positive.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeElementWithSvgImageBrush(L"ryg-positive.svg", std::nan(""), std::nan(""), svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeElementWithSvgImageBrush(L"ryg-positive.svg", 30, std::nan(""), svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeElementWithSvgImageBrush(L"ryg-positive.svg", std::nan(""), 30, svgOpenedEvents, svgEventRegistrations));
        positive->Children->Append(MakeElementWithSvgImageBrush(L"ryg-positive.svg", 30, 30, svgOpenedEvents, svgEventRegistrations));
        root->Children->Append(positive);

        Canvas^ rootCanvas = ref new Canvas();  // So there's no ScrollViewer info at the root
        rootCanvas->Children->Append(root);
        windowHelper->WindowContent = rootCanvas;
    });

    for (const auto& event : svgOpenedEvents)
    {
        event->WaitForDefault();
        VERIFY_IS_TRUE(event->HasFired());
    }
    windowHelper->WaitForIdle();

    utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SvgImageSourceTests::SimpleSvgTest(
    SvgImageSourceCallback svgImageSourceCallback,
    Platform::String^ variationString)
{
    TestCleanupWrapper cleanup;
    SvgImageEventWaitingContext eventWaitingContext;
    XamlFileTestEngine engine;

    engine.SetMockDCompVariation(variationString);
    engine.SetXamlFilePath(GetResourcesPath() + L"SimpleImage.xaml");
    engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        auto image = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));

        auto svgImageSource = ref new SvgImageSource();
        image->Source = svgImageSource;

        if (svgImageSourceCallback != nullptr)
        {
            svgImageSourceCallback(image, svgImageSource);
        }

        eventWaitingContext.Attach(svgImageSource);
        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"TestPattern_Large.svg");
    });
    engine.SetPostInitWaitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        eventWaitingContext.WaitOpened();
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    });
    engine.Execute();
}

void SvgImageSourceTests::SvgFileTest(
    Platform::String^ svgFilePath,
    Platform::String^ variationString)
{
    TestCleanupWrapper cleanup;
    SvgImageEventWaitingContext eventWaitingContext;
    XamlFileTestEngine engine;

    engine.SetMockDCompVariation(variationString);
    engine.SetXamlFilePath(GetResourcesPath() + L"SimpleImage.xaml");
    engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));

        auto svgImageSource = ref new SvgImageSource();
        testImage->Source = svgImageSource;
        eventWaitingContext.Attach(svgImageSource);
        svgImageSource->UriSource = ref new Uri(svgFilePath);
    });
    engine.SetPostInitWaitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        eventWaitingContext.WaitOpened();
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    });
    engine.Execute();
}

void SvgImageSourceTests::SvgAutomationTest(
    Platform::String^ xamlFilePath,
    Platform::String^ svgFilePath,
    Platform::String^ expectedClassName,
    Platform::String^ expectedName,
    Platform::String^ expectedFullDescription,
    xaml_automation_peers::AutomationControlType expectedControlType)
{
    TestCleanupWrapper cleanup;
    SvgImageEventWaitingContext eventWaitingContext;
    XamlFileTestEngine engine;
    engine.SetMockDCompVerificationEnabled(false);

    engine.SetXamlFilePath(xamlFilePath);
    engine.SetPostInitCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));

        auto svgImageSource = ref new SvgImageSource();
        testImage->Source = svgImageSource;
        eventWaitingContext.Attach(svgImageSource);

        if (svgFilePath != nullptr)
        {
            svgImageSource->UriSource = ref new Uri(svgFilePath);
        }
    });
    engine.SetPostInitWaitCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        if (svgFilePath != nullptr)
        {
            eventWaitingContext.WaitOpened();
        }
    });
    engine.SetValidationCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));
        auto automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(testImage);
        VERIFY_ARE_EQUAL(automationPeer->GetClassName(), expectedClassName);
        VERIFY_ARE_EQUAL(automationPeer->GetName(), expectedName);
        VERIFY_ARE_EQUAL(automationPeer->GetFullDescription(), expectedFullDescription);
        VERIFY_ARE_EQUAL(automationPeer->GetAutomationControlType(), expectedControlType);
    });
    engine.Execute();
}

void SvgImageSourceTests::VerifyImageSizing()
{
    TestCleanupWrapper cleanup;

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageSourceOpenedEvent = std::make_shared<Event>();

    xaml_controls::Image^ testImage = nullptr;
    SvgImageSource^ svgImageSource = nullptr;

    RunOnUIThread([&]()
    {
        auto stackPanel = ref new StackPanel();
        testImage = ref new xaml_controls::Image();

        svgImageSource = ref new SvgImageSource();
        testImage->Source = svgImageSource;

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageSourceOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SVG image source opened.");
            svgImageSourceOpenedEvent->Set();
        }));

        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"msft.svg");

        stackPanel->Children->Append(testImage);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });

    svgImageSourceOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenedEvent->HasFired());

    RunOnUIThread([&]()
    {
        auto xamlRootSize = testImage->XamlRoot->Size;
        auto maxSize = TestServices::WindowHelper->GetImageSourceMaxSize(svgImageSource);
        VERIFY_ARE_EQUAL(round(maxSize.Width), round(xamlRootSize.Width));
        VERIFY_ARE_EQUAL(round(maxSize.Height), round(xamlRootSize.Height));
    });
}

// Regression Test: [Watson Failure] caused by stack overflow from CSvgImageSource::OnDownloadImageAvailableImpl
//                  (INVALID_STACK_ACCESS_c0000005_Windows.UI.Xaml.dll!GetCallerReturnAddressFromDirectCaller)
//
// Test effective recovery from device loss during off-thread image decode
// Simulate the state as follows:
//     1. Set SvgImageSource.RasterizePixelWidth/Height so that we do not take decode-to-render-size codepath
//     2. Set ForceDeviceLostOnMetadataParse test hook which causes EncodedImageData::Parse calls from ImageProvider::GetImage to return DXGI_ERROR_DEVICE_REMOVED
void SvgImageSourceTests::DeviceLostOnOffThreadImageDecode()
{
    TestCleanupWrapper cleanup;

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageSourceOpenedEvent = std::make_shared<Event>();

    xaml_controls::Image^ testImage = nullptr;
    SvgImageSource^ svgImageSource = nullptr;

    RunOnUIThread([&]()
    {
        auto stackPanel = ref new StackPanel();
        testImage = ref new xaml_controls::Image();

        svgImageSource = ref new SvgImageSource();
        svgImageSource->RasterizePixelWidth = 100;
        svgImageSource->RasterizePixelHeight = 100;
        testImage->Source = svgImageSource;

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageSourceOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SVG image source opened.");
            svgImageSourceOpenedEvent->Set();
        }));

        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"msft.svg");

        TestServices::Utilities->SimulateDeviceLostOnMetadataParse();
        stackPanel->Children->Append(testImage);
        TestServices::WindowHelper->WindowContent = stackPanel;

    });

    svgImageSourceOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenedEvent->HasFired());
}

// [Watson Failure] caused by STOWED_EXCEPTION_887a0005_Windows.UI.Xaml.dll!CD3D11Device::EnsureD2DResources
void SvgImageSourceTests::DeviceLostOnCreatingSvgDecoder_Uri()
{
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageSourceOpenedEvent = std::make_shared<Event>();
    auto failedRegistration = CreateSafeEventRegistration(SvgImageSource, OpenFailed);
    auto svgImageSourceFailedEvent = std::make_shared<Event>();

    xaml_controls::Image^ testImage = nullptr;
    SvgImageSource^ svgImageSource = nullptr;

    RunOnUIThread([&]()
    {
        auto stackPanel = ref new StackPanel();
        testImage = ref new xaml_controls::Image();
        testImage->Width = 150;
        testImage->Height = 150;

        u->SimulateDeviceLostOnCreatingSvgDecoder();

        svgImageSource = ref new SvgImageSource();
        svgImageSource->RasterizePixelWidth = 100;
        svgImageSource->RasterizePixelHeight = 100;
        testImage->Source = svgImageSource;

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageSourceOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"> SVG image source opened.");
            svgImageSourceOpenedEvent->Set();
        }));

        failedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceFailedEventArgs^>([svgImageSourceFailedEvent](SvgImageSource^, SvgImageSourceFailedEventArgs^)
        {
            LOG_OUTPUT(L"> SVG image source failed.");
            svgImageSourceFailedEvent->Set();
        }));

        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"msft.svg");

        stackPanel->Children->Append(testImage);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });

    svgImageSourceOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenedEvent->HasFired());
    VERIFY_IS_FALSE(svgImageSourceFailedEvent->HasFired());

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

// [Watson Failure] caused by STOWED_EXCEPTION_887a0005_Windows.UI.Xaml.dll!CD3D11Device::EnsureD2DResources
void SvgImageSourceTests::DeviceLostOnCreatingSvgDecoder_Stream()
{
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    LOG_OUTPUT(L"Getting stream of image");
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"msft.svg");

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageSourceOpenedEvent = std::make_shared<Event>();
    auto failedRegistration = CreateSafeEventRegistration(SvgImageSource, OpenFailed);
    auto svgImageSourceFailedEvent = std::make_shared<Event>();

    xaml_controls::Image^ testImage = nullptr;
    SvgImageSource^ svgImageSource = nullptr;

    auto sourceAsyncCompletionEvent = std::make_shared<Event>();
    wf::IAsyncOperation<SvgImageSourceLoadStatus>^ setSourceAsyncOperation = nullptr;
    SvgImageSourceLoadStatus status;

    RunOnUIThread([&]()
    {
        auto stackPanel = ref new StackPanel();
        testImage = ref new xaml_controls::Image();
        testImage->Width = 150;
        testImage->Height = 150;

        u->SimulateDeviceLostOnCreatingSvgDecoder();

        svgImageSource = ref new SvgImageSource();
        svgImageSource->RasterizePixelWidth = 100;
        svgImageSource->RasterizePixelHeight = 100;
        testImage->Source = svgImageSource;

        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageSourceOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"> SVG image source opened.");
            svgImageSourceOpenedEvent->Set();
        }));

        failedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceFailedEventArgs^>([svgImageSourceFailedEvent](SvgImageSource^, SvgImageSourceFailedEventArgs^)
        {
            LOG_OUTPUT(L"> SVG image source failed.");
            svgImageSourceFailedEvent->Set();
        }));

        setSourceAsyncOperation = svgImageSource->SetSourceAsync(bitmapStream);

        auto setSourceCallback = ref new wf::AsyncOperationCompletedHandler<SvgImageSourceLoadStatus>(
            [&status, sourceAsyncCompletionEvent](wf::IAsyncOperation<SvgImageSourceLoadStatus>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"> SetSourceAsync operation completed for svg");
            status = operation->GetResults();
            sourceAsyncCompletionEvent->Set();
        });
        VERIFY_IS_NOT_NULL(setSourceCallback);
        setSourceAsyncOperation->Completed = setSourceCallback;

        stackPanel->Children->Append(testImage);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });

    // Even though we can recover from the device lost error, this SetSourceAsync operation reports a failure because
    // it was the one that hit the device lost error.
    sourceAsyncCompletionEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(status, SvgImageSourceLoadStatus::Other);

    svgImageSourceOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageSourceOpenedEvent->HasFired());
    VERIFY_IS_FALSE(svgImageSourceFailedEvent->HasFired());

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SvgImageSourceTests::ReloadsOnScaleChange()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    ::Windows::Foundation::Size size(800, 600);

    xaml_controls::Image^ testImage = nullptr;
    SvgImageSource^ svgImageSource = nullptr;

    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    auto sourceAsyncCompletionEvent = std::make_shared<Event>();
    wf::IAsyncOperation<SvgImageSourceLoadStatus>^ setSourceAsyncOperation = nullptr;

    RunOnUIThread([&]()
    {
        auto stackPanel = ref new StackPanel();
        testImage = ref new xaml_controls::Image();
        testImage->Width = 150;
        testImage->Height = 150;

        svgImageSource = ref new SvgImageSource();
        auto testUri = ref new Uri(GetResourcesPath() + L"msft.svg");
        VERIFY_IS_NOT_NULL(testUri);
        svgImageSource->UriSource = testUri;

        testImage->Source = svgImageSource;

        auto svgImageSource = safe_cast<SvgImageSource^>(testImage->Source);
        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"SvgImageSource Opened Event Fired");
            svgImageOpenedEvent->Set();
        }));

        stackPanel->Children->Append(testImage);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });

    svgImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_TRUE(svgImageOpenedEvent->HasFired());

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "A");

    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 2.0f);
    TestServices::WindowHelper->SynchronouslyTickUIThread(10);
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "B");
}

class ForceXamlInvisibleScopeGuard
{
public:
    ForceXamlInvisibleScopeGuard()
    {
        TestServices::WindowHelper->SetXamlVisibilityOverride(false);
        TestServices::WindowHelper->SetIsRenderEnabled(false);
    }

    ~ForceXamlInvisibleScopeGuard()
    {
        TestServices::WindowHelper->SetIsRenderEnabled(true);
        TestServices::WindowHelper->SetXamlVisibilityOverride(true);
    }
};

class StopThrottlingImageTaskDispatcherScopeGuard
{
public:
    StopThrottlingImageTaskDispatcherScopeGuard() {}

    ~StopThrottlingImageTaskDispatcherScopeGuard()
    {
        TestServices::WindowHelper->ThrottleImageTaskDispatcher(false, 0);
    }
};

void SvgImageSourceTests::SvgWithoutDevice()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    xaml_controls::Image^ testImage_UIA = nullptr;
    auto openedRegistration = CreateSafeEventRegistration(SvgImageSource, Opened);
    auto svgImageOpenedEvent = std::make_shared<Event>();

    // Ensure that we don't leave the image task dispatcher in throttled mode if something fails
    StopThrottlingImageTaskDispatcherScopeGuard stopThrottlingImageTaskDispatcher;

    RunOnUIThread([&]()
    {
        SvgImageSource^ svgImageSource = ref new SvgImageSource();
        svgImageSource->UriSource = ref new Uri(GetResourcesPath() + L"msft.svg");
        openedRegistration.Attach(
            svgImageSource,
            ref new wf::TypedEventHandler<SvgImageSource^, SvgImageSourceOpenedEventArgs^>([svgImageOpenedEvent](SvgImageSource^, SvgImageSourceOpenedEventArgs^)
        {
            LOG_OUTPUT(L"  > SvgImageSource.Opened fired");
            svgImageOpenedEvent->Set();
        }));

        // Let this one fully decode and render so it has metadata available.
        testImage_UIA = ref new xaml_controls::Image();
        testImage_UIA->Width = 150;
        testImage_UIA->Height = 150;
        testImage_UIA->Source = svgImageSource;

        root = ref new Canvas();
        root->Children->Append(testImage_UIA);
        wh->WindowContent = root;
    });
    svgImageOpenedEvent->WaitForDefault();
    wh->WaitForIdle();
    VERIFY_IS_TRUE(svgImageOpenedEvent->HasFired());

    // There's another existing bug here, which is a crash that happens when Xaml dereferences a deleted pointer if a
    // SVG finished downloading while we have no device. The fix for the crash is easy enough - use a weak pointer - but
    // there's no code path to retry parsing that SVG's metadata and continuing with a decode. That SVG will just not
    // show up in the tree. We're fixing the crash for now (it's a potential security problem) while leaving the
    // rendering issue open.
    // Allow a single imaging task through for the download. Let it complete and queue up the decode before we trigger device lost.
    wh->ThrottleImageTaskDispatcher(true, 1);
    RunOnUIThread([&]()
    {
        SvgImageSource^ svgImageSource2 = ref new SvgImageSource();
        svgImageSource2->UriSource = ref new Uri(GetResourcesPath() + L"TestPattern_Large.svg");

        // This one doesn't get to decode or render. We trigger device lost before any of it can happen.
        xaml_controls::Image^ testImage2 = ref new xaml_controls::Image();
        testImage2->Width = 150;
        testImage2->Height = 150;
        testImage2->Source = svgImageSource2;

        root->Children->Append(testImage2);
    });
    wh->WaitForIdle();

    {
        // Simulate a device lost and force a tick to have Xaml release the lost device. Force Xaml to be invisible so that
        // we don't create a new device. Note that these can happen while the UI thread is ticking in between. Once the
        // window is invisible Xaml won't render on a tick. We're free to simulate a device lost any time later.
        LOG_OUTPUT(L"> Simulating window becoming invisible and device lost.");
        ForceXamlInvisibleScopeGuard forceXamlInvisible;
        wh->SimulateDeviceLost();
        wh->SynchronouslyTickUIThread(1);

        // Now that the device has been lost and recovered, allow the pending decode to go through.
        wh->ThrottleImageTaskDispatcher(false, 0);
        wh->RequestExecuteImageTaskDispatcher();

        wrl::ComPtr<IUIAutomationElement> imageUIAElement;
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"TestImage";
            uiaInfo.m_cType = UIA_ImageControlTypeId;

            LOG_OUTPUT(L"> Attempting to get Image.Title automation property. Don't crash.");
            auto spAutomationClientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&imageUIAElement);
        });
    }
    wh->SynchronouslyTickUIThread(2);
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } } }
