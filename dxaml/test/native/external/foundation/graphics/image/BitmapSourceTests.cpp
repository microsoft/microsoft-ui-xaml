// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "BitmapSourceTests.h"
#include "ImmediateReadRandomAccessStream.h"
#include <atomic>
#include <weakreference.h>
#include <WUCRenderingScopeGuard.h>

using namespace Concurrency;
using namespace Microsoft::WRL;
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

static ComPtr<IWeakReference> MakeWeakReference(_In_ IInspectable* value)
{
    ComPtr<IWeakReferenceSource> source;
    WEX::Common::Throw::IfFailed(value->QueryInterface(IID_PPV_ARGS(&source)));

    ComPtr<IWeakReference> weakReference;
    WEX::Common::Throw::IfFailed(source->GetWeakReference(&weakReference));

    return weakReference;
}

static ComPtr<IInspectable> ResolveWeakReference(_In_ const ComPtr<IWeakReference>& weakReference)
{
    ComPtr<IInspectable> resolved;
    WEX::Common::Throw::IfFailed(weakReference->Resolve(__uuidof(IInspectable), &resolved));

    return resolved;
}

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

void BitmapSourceTests::SetSourceAsync_ImageDestroyedDuringDecode_ReleasesAction()
{
    constexpr UINT32 iterationCount = 128;

    auto completionCount = std::make_shared<std::atomic<UINT32>>(0);
    auto completionStatus = std::make_shared<std::atomic<AsyncStatus>>(AsyncStatus::Started);
    auto completionResult = std::make_shared<std::atomic<HRESULT>>(E_PENDING);

    std::vector<ComPtr<IWeakReference>> imageWeakReferences;
    std::vector<ComPtr<IWeakReference>> actionWeakReferences;
    std::vector<ComPtr<IInspectable>> retainedActions;
    imageWeakReferences.reserve(iterationCount);
    actionWeakReferences.reserve(iterationCount);
    retainedActions.reserve(iterationCount);

    RunOnUIThread([&]()
    {
        for (UINT32 index = 0; index < iterationCount; ++index)
        {
            auto stream = MakeImmediateReadRandomAccessStream();
            auto image = ref new BitmapImage();
            auto action = image->SetSourceAsync(stream);

            if (index == 0)
            {
                action->Completed =
                    ref new AsyncActionCompletedHandler([completionCount, completionStatus, completionResult](
                        IAsyncAction^ completedAction,
                        AsyncStatus status)
                {
                    completionCount->fetch_add(1);
                    completionStatus->store(status);

                    HRESULT result = S_OK;
                    try
                    {
                        completedAction->GetResults();
                    }
                    catch (Platform::Exception^ exception)
                    {
                        result = exception->HResult;
                    }

                    completionResult->store(result);
                });
            }

            imageWeakReferences.emplace_back(MakeWeakReference(reinterpret_cast<IInspectable*>(image)));
            actionWeakReferences.emplace_back(MakeWeakReference(reinterpret_cast<IInspectable*>(action)));

            ComPtr<IInspectable> retainedAction;
            retainedAction = reinterpret_cast<IInspectable*>(action);
            retainedActions.emplace_back(std::move(retainedAction));

            action = nullptr;
            image = nullptr;
            stream = nullptr;
        }
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();

    UINT32 imagesAlive = 0;
    UINT32 startedActions = 0;
    UINT32 errorActions = 0;
    RunOnUIThread([&]()
    {
        for (UINT32 index = 0; index < iterationCount; ++index)
        {
            if (ResolveWeakReference(imageWeakReferences[index]) != nullptr)
            {
                ++imagesAlive;
            }

            auto action = reinterpret_cast<IAsyncAction^>(retainedActions[index].Get());
            if (action->Status == AsyncStatus::Started)
            {
                ++startedActions;
            }
            else if (action->Status == AsyncStatus::Error)
            {
                ++errorActions;
            }
        }
    });

    const auto observedCompletionCount = completionCount->load();
    const auto observedCompletionStatus = completionStatus->load();
    const auto observedCompletionResult = completionResult->load();

    RunOnUIThread([&]()
    {
        retainedActions.clear();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(2);
    TestServices::WindowHelper->WaitForIdle();

    UINT32 actionsAlive = 0;
    RunOnUIThread([&]()
    {
        for (const auto& actionWeakReference : actionWeakReferences)
        {
            if (ResolveWeakReference(actionWeakReference) != nullptr)
            {
                ++actionsAlive;
            }
        }
    });

    VERIFY_ARE_EQUAL(0u, imagesAlive, L"The BitmapImage instances did not reach destruction.");
    VERIFY_ARE_EQUAL(0u, startedActions, L"SetSourceAsync actions remained in Started state.");
    VERIFY_ARE_EQUAL(iterationCount, errorActions, L"SetSourceAsync actions did not transition to Error.");
    VERIFY_ARE_EQUAL(1u, observedCompletionCount, L"Image destruction did not invoke completion exactly once.");
    VERIFY_ARE_EQUAL(AsyncStatus::Error, observedCompletionStatus, L"The completion callback did not report Error.");
    VERIFY_ARE_EQUAL(E_ABORT, observedCompletionResult, L"The completed action did not report E_ABORT.");
    VERIFY_ARE_EQUAL(0u, actionsAlive, L"SetSourceAsync actions remained alive after caller release.");
}

} } } } } } }
