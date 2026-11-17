// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SystemCompositionEngineTests.h"
#include <XamlTailored.h>
#include "FileLoader.h"
#include <wrl.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ SystemCompositionEngineTests::GetResourcesPath() const
{
    // Reuse the CompNodeTests resources
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
}

bool SystemCompositionEngineTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SystemCompositionEngineTests::ClassCleanup()
{
    return true;
}

bool SystemCompositionEngineTests::TestSetup()
{
    // The focused tests mirror the existing CompNode master-backed flow while
    // the WPF host selects the system composition engine before XAML initialization.
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SystemCompositionEngineTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SystemCompositionEngineTests::CompNode1WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode1.xaml");
}

void SystemCompositionEngineTests::CompNode2WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode2.xaml");
}

void SystemCompositionEngineTests::CompNode3WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode3.xaml");
}

void SystemCompositionEngineTests::CompNode4WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode4.xaml");
}

void SystemCompositionEngineTests::CompNode5WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode5.xaml");
}

void SystemCompositionEngineTests::CompNode6WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode6.xaml");
}

void SystemCompositionEngineTests::CompNode7WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode7.xaml");
}

void SystemCompositionEngineTests::CompNode8WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode8.xaml");
}

void SystemCompositionEngineTests::CompNode9WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode9.xaml");
}

void SystemCompositionEngineTests::CompNode10WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode10.xaml");
}

void SystemCompositionEngineTests::CompNode11WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode11.xaml");
}

void SystemCompositionEngineTests::CompNode12WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(
        L"CompNode12.xaml",
        false /* waitForIdle */);
}

void SystemCompositionEngineTests::CompNode13WUCWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifyWithMockDComp(L"CompNode13.xaml");
}

void SystemCompositionEngineTests::LoadAndVerifyWithMockDComp(
    Platform::String^ markupFile,
    bool waitForIdle)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(
        DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    StackPanel^ root = safe_cast<StackPanel^>(
        LoadXamlFileOnUIThread(GetResourcesPath() + markupFile));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    if (waitForIdle)
    {
        wh->WaitForIdle();
    }
    else
    {
        wh->SynchronouslyTickUIThread(3);
    }

    MockDComp::IMockDCompDevice^ mockDevice = wh->MockDCompDevice;
    VERIFY_IS_NOT_NULL(
        mockDevice,
        L"XAML's DComp device does not implement IMockDCompDevice with the system composition engine");

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SystemCompositionEngineTests::VerifySystemEngineEquivalent()
{
    // System composition engine selection leaves process-lifetime allocations that are
    // already exempted by the render methods in this class.
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    VerifySystemEngineEquivalentImpl();
}

void SystemCompositionEngineTests::VerifySystemEngineEquivalentImpl()
{
    // CompositionEngine::GetForSystemEngine returns the underlying system object
    // for a Microsoft.UI.Composition object. A Windows.UI.Composition.Compositor
    // result verifies that XAML is using the system composition engine rather than
    // the default in-process composition engine.
    auto wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(
        DCompRendering::WUCCompleteSynchronousCompTree,
        /*resizeWindow*/true,
        /*injectMockDComp*/false,
        /*resetDevice*/false,
        /*resetWindowContent*/false);

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CompNode1.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto visual = Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(root);
        auto compositor = visual->Compositor;

        // Step 1: public API - get the selected compositor's system-engine equivalent.
        // Returns null only if the system engine was never set or the object has no system equivalent.
        Platform::Object^ systemObject = Microsoft::UI::Composition::CompositionEngine::GetForSystemEngine(compositor);
        VERIFY_IS_NOT_NULL(
            systemObject,
            L"CompositionEngine::GetForSystemEngine returned null - the system composition engine is not active");

        // Step 2: verify the underlying system object identifies itself as Windows.UI.Composition.Compositor.
        // This is the conclusive system composition engine signal.
        IInspectable* systemInspectable = reinterpret_cast<IInspectable*>(systemObject);
        wil::unique_hstring runtimeClassName;
        VERIFY_SUCCEEDED(systemInspectable->GetRuntimeClassName(runtimeClassName.put()));
        UINT32 length = 0;
        PCWSTR runtimeClassNameRaw = WindowsGetStringRawBuffer(runtimeClassName.get(), &length);
        WEX::Logging::Log::Comment(WEX::Common::String().Format(L"System-engine object class name: '%s'", runtimeClassNameRaw));
        VERIFY_ARE_EQUAL(
            std::wstring(L"Windows.UI.Composition.Compositor"),
            std::wstring(runtimeClassNameRaw, length),
            L"System-engine object is not Windows.UI.Composition.Compositor - system composition engine routing is not active");
    });
}

} } } } } }
