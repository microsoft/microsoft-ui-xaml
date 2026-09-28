// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SwitcherTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <RuntimeParameters.h>
#include <wrl.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Shapes;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ SwitcherTests::GetResourcesPath() const
{
    // Reuse the CompNodeTests resources
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
}

bool SwitcherTests::ClassSetup()
{
    // Enable the switcher BEFORE any compositor or composition object is created.
    // CompositionEngine::TrySetProcessEngine must be called before InitializeXaml.
    // Signature: bool TrySetProcessEngine(CompositionEngineType requested);
    try
    {
        WEX::Common::String switcherLafToken;
        if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"SwitcherLafToken", switcherLafToken))
            && !switcherLafToken.IsEmpty())
        {
            auto unlockResult = ::Windows::ApplicationModel::LimitedAccessFeatures::TryUnlockFeature(
                ref new Platform::String(L"com.microsoft.windows.composition.engine"),
                ref new Platform::String(static_cast<const wchar_t*>(switcherLafToken)),
                ref new Platform::String(
                    L"8wekyb3d8bbwe has registered their use of "
                    L"com.microsoft.windows.composition.engine with Microsoft and agrees to the terms of use."));
            WEX::Logging::Log::Comment(WEX::Common::String().Format(
                L"SwitcherTests: LAF TryUnlockFeature status=%d",
                static_cast<int>(unlockResult->Status)));
        }

        bool ok = Microsoft::UI::Composition::CompositionEngine::TrySetProcessEngine(
            Microsoft::UI::Composition::CompositionEngineType::System);

        if (!ok)
        {
            WEX::Logging::Log::Comment(WEX::Common::String().Format(
                L"TrySetProcessEngine(System) did not engage (ok=%d) - skipping",
                static_cast<int>(ok)));
            return false;
        }
    }
    catch (Platform::Exception^ ex)
    {
        WEX::Logging::Log::Comment(WEX::Common::String().Format(
            L"CompositionEngine API not available (hr=0x%08x) - switcher bits not in this build, skipping",
            ex->HResult));
        return false;
    }

    WEX::Logging::Log::Comment(L"Switcher enabled via CompositionEngine::TrySetProcessEngine(System)");

    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SwitcherTests::ClassCleanup()
{
    return true;
}

bool SwitcherTests::TestSetup()
{
    // Tests in this class mirror the lifted CompNodeTests pattern: inject MockDComp,
    // load XAML, call VerifyMockDCompOutput. Switcher is enabled process-wide in
    // ClassSetup via CompositionEngine::TrySetProcessEngine(System).
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SwitcherTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SwitcherTests::CompNode1WUCFullSwitcherWithMockDComp()
{
    // Switching leaves one 16-byte process-lifetime allocation attributed to
    // XamlCheckProcessRequirements. Keep the exemption scoped to this test.
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

    // Switcher + MockDComp injection together. With the IXP identity refactor in place,
    // VerifyMockDCompOutput should produce XML byte-identical to the non-switcher master.
    LoadAndVerifySwitcherWithMockDComp(L"CompNode1.xaml");
}

void SwitcherTests::CompNode2WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode2.xaml");
}

void SwitcherTests::CompNode3WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode3.xaml");
}

void SwitcherTests::CompNode4WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode4.xaml");
}

void SwitcherTests::CompNode5WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode5.xaml");
}

void SwitcherTests::CompNode6WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode6.xaml");
}

void SwitcherTests::CompNode7WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode7.xaml");
}

void SwitcherTests::CompNode8WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode8.xaml");
}

void SwitcherTests::CompNode9WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode9.xaml");
}

void SwitcherTests::CompNode10WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode10.xaml");
}

void SwitcherTests::CompNode11WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode11.xaml");
}

void SwitcherTests::CompNode12WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode12.xaml");
}

void SwitcherTests::CompNode13WUCFullSwitcherWithMockDComp()
{
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
    LoadAndVerifySwitcherWithMockDComp(L"CompNode13.xaml");
}

// Shared helper for switcher + MockDComp tests. Mirrors the original CompNode1WUCFullSwitcherWithMockDComp
// body so that every CompNode*WUCFullSwitcher* test exercises an identical flow.
void SwitcherTests::LoadAndVerifySwitcherWithMockDComp(Platform::String^ markupFile)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + markupFile));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    MockDComp::IMockDCompDevice^ mockDevice = wh->MockDCompDevice;
    VERIFY_IS_NOT_NULL(
        mockDevice,
        L"XAML's DComp device does not implement IMockDCompDevice under switcher");

    // VerifyMockDCompOutput compares the captured tree to the master xml resource.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SwitcherTests::VerifyLiftedSystemCompositionPath()
{
    // Public-API proof of the lifted->system path. CompositionEngine::GetForSystemEngine()
    // returns the underlying system-composition object for a lifted Microsoft.UI.Composition
    // object (see https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.compositionengine).
    // If the lifted compositor's system-engine equivalent reports its runtime class name as
    // "Windows.UI.Composition.Compositor", XAML's lifted compositor is backed by a real
    // system-composition compositor (lifted -> system path), not a pure-lifted/standalone one.
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

        // Step 1: public API - get the system-engine equivalent of the lifted compositor.
        // Returns null only if the system engine was never set or the object has no system equivalent.
        Platform::Object^ systemObject = Microsoft::UI::Composition::CompositionEngine::GetForSystemEngine(compositor);
        VERIFY_IS_NOT_NULL(
            systemObject,
            L"CompositionEngine::GetForSystemEngine returned null - lifted-only path, not lifted->system");

        // Step 2: verify the underlying system object identifies itself as Windows.UI.Composition.Compositor.
        // This is the conclusive lifted-system-composition signal.
        IInspectable* systemInspectable = reinterpret_cast<IInspectable*>(systemObject);
        wil::unique_hstring runtimeClassName;
        VERIFY_SUCCEEDED(systemInspectable->GetRuntimeClassName(runtimeClassName.put()));
        UINT32 length = 0;
        PCWSTR runtimeClassNameRaw = WindowsGetStringRawBuffer(runtimeClassName.get(), &length);
        WEX::Logging::Log::Comment(WEX::Common::String().Format(L"System-engine object class name: '%s'", runtimeClassNameRaw));
        VERIFY_ARE_EQUAL(
            std::wstring(L"Windows.UI.Composition.Compositor"),
            std::wstring(runtimeClassNameRaw, length),
            L"System-engine object is not Windows.UI.Composition.Compositor - lifted system-composition routing not active");
    });
}

} } } } } }
