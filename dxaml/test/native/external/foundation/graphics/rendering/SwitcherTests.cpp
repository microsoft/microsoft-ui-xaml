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
    // Switcher + MockDComp injection together. With the IXP identity refactor in place,
    // VerifyMockDCompOutput should produce XML byte-identical to the non-switcher master.
    LoadAndVerifySwitcherWithMockDComp(L"CompNode1.xaml");
}

// Shared helper for switcher + MockDComp tests. Mirrors the original CompNode1WUCFullSwitcherWithMockDComp
// body so that every CompNode*WUCFullSwitcher* test exercises an identical flow.
void SwitcherTests::LoadAndVerifySwitcherWithMockDComp(Platform::String^ markupFile)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    // injectMockDComp=true: opt back into MockDComp under switcher.
    WUCRenderingScopeGuard wuc(
        DCompRendering::WUCCompleteSynchronousCompTree,
        /*resizeWindow*/true,
        /*injectMockDComp*/true,
        /*resetDevice*/false,
        /*resetWindowContent*/false);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + markupFile));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    // Soft check: when MockDComp is interposed, the compositor returned to XAML may be
    // MockDCompDevice (which does NOT implement ISwitcherProxyInterfaceAccess). A failing
    // QI here is informational; it tells us MockDComp blocked the proxy from reaching XAML.
    // Use VerifyLiftedSystemCompositionPath (no MockDComp) for the strong proxy-engagement
    // assertion.
    RunOnUIThread([&]()
    {
        auto visual = Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(root);
        auto compositor = visual->Compositor;
        Microsoft::WRL::ComPtr<IUnknown> compositorUnknown;
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(compositor)->QueryInterface(IID_PPV_ARGS(&compositorUnknown)));
        Microsoft::WRL::ComPtr<ISwitcherProxyInterfaceAccess> switcherAccess;
        HRESULT hr = compositorUnknown.As(&switcherAccess);
        WEX::Logging::Log::Comment(WEX::Common::String().Format(
            L"ISwitcherProxyInterfaceAccess QI under MockDComp: hr=0x%08x (success=switcher proxy reached XAML; failure=MockDComp interposed)",
            hr));
    });

    // VerifyMockDCompOutput compares the captured tree to the master xml resource.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void SwitcherTests::VerifyLiftedSystemCompositionPath()
{
    // Stronger proof than the bare ISwitcherProxyInterfaceAccess QI: drill through the
    // proxy via GetInterface() and inspect the underlying system object's runtime class
    // name. The switcher proxy implementation forwards GetInterface() to its private
    // m_systemObject (a real Windows::UI::Composition object). If the runtime class name
    // comes back as "Windows.UI.Composition.Compositor", we know XAML's lifted compositor
    // is wrapping a real system-composition compositor (lifted -> system path), and not a
    // pure-lifted/standalone compositor.
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

        Microsoft::WRL::ComPtr<IUnknown> compositorUnknown;
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(compositor)->QueryInterface(IID_PPV_ARGS(&compositorUnknown)));

        // Step 1: confirm the lifted compositor is actually a switcher proxy.
        Microsoft::WRL::ComPtr<ISwitcherProxyInterfaceAccess> switcherAccess;
        VERIFY_SUCCEEDED(
            compositorUnknown.As(&switcherAccess),
            L"ISwitcherProxyInterfaceAccess QI failed - lifted compositor is not the switcher proxy");

        // Step 2: ask the proxy for its underlying system object as IInspectable.
        // The proxy implementation forwards this to m_systemObject->QueryInterface(IID_IInspectable, ...).
        Microsoft::WRL::ComPtr<IInspectable> systemObject;
        VERIFY_SUCCEEDED(
            switcherAccess->GetInterface(__uuidof(IInspectable), reinterpret_cast<void**>(systemObject.GetAddressOf())),
            L"GetInterface(IID_IInspectable) failed - switcher proxy has no underlying system object");
        VERIFY_IS_NOT_NULL(systemObject.Get(), L"Underlying system object is null - lifted-only path, not lifted->system");

        // Step 3: verify the underlying system object identifies itself as Windows.UI.Composition.Compositor.
        // This is the conclusive lifted-system-composition signal.
        wil::unique_hstring runtimeClassName;
        VERIFY_SUCCEEDED(systemObject->GetRuntimeClassName(runtimeClassName.put()));
        UINT32 length = 0;
        PCWSTR runtimeClassNameRaw = WindowsGetStringRawBuffer(runtimeClassName.get(), &length);
        WEX::Logging::Log::Comment(WEX::Common::String().Format(L"Switcher proxy underlying system object class name: '%s'", runtimeClassNameRaw));
        VERIFY_ARE_EQUAL(
            std::wstring(L"Windows.UI.Composition.Compositor"),
            std::wstring(runtimeClassNameRaw, length),
            L"Underlying system object is not Windows.UI.Composition.Compositor - lifted system-composition routing not active");
    });
}

} } } } } }
