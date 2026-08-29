// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "Closable.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

using namespace test_infra;
using namespace ::Windows::Foundation;

class WUCRenderingScopeGuard
{
public:
    explicit WUCRenderingScopeGuard(
        DCompRendering rendering,
        bool resizeWindow = true,
        bool injectMockDComp = true,
        bool resetDevice = false,
        bool resetWindowContent = true)
    {
        Platform::Object^ pGuard = TestServices::Utilities->CreateRenderingScopeGuard(
            rendering,
            !!resizeWindow,
            !!injectMockDComp,
            !!resetDevice,
            !!resetWindowContent);
        wrl::ComPtr<IInspectable> spClosable(reinterpret_cast<IInspectable*>(pGuard));
        spClosable.As(&m_spGuard);
    }

    ~WUCRenderingScopeGuard()
    {
        m_spGuard->Close();
    }

    // Disallow copying/moving
    WUCRenderingScopeGuard(const WUCRenderingScopeGuard&) = delete;
    WUCRenderingScopeGuard(WUCRenderingScopeGuard&&) = delete;
    WUCRenderingScopeGuard& operator=(const WUCRenderingScopeGuard&) = delete;
    WUCRenderingScopeGuard& operator=(WUCRenderingScopeGuard&&) = delete;

private:
    wrl::ComPtr<ABI::Windows::Foundation::IClosable> m_spGuard;
};

// Helper class to make it much easier to turn on the DumpMockDManipHitTestVisual feature key when using WUCRenderingScopeGuard
class WUCRenderingScopeGuardWithDManipHitTestVisual : public WUCRenderingScopeGuard
{
public:
    explicit WUCRenderingScopeGuardWithDManipHitTestVisual(
        DCompRendering rendering,
        bool resizeWindow = true,
        bool injectMockDComp = true,
        bool resetDevice = false,
        bool resetWindowContent = true)
    : WUCRenderingScopeGuard(rendering, resizeWindow, injectMockDComp, resetDevice, resetWindowContent)
    {
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DumpMockDManipHitTestVisual), true, &m_wasEnabled);
    }

    ~WUCRenderingScopeGuardWithDManipHitTestVisual()
    {
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DumpMockDManipHitTestVisual), m_wasEnabled, nullptr);
    }

private:
    bool m_wasEnabled;
};

class SkipRSVBackgroundScopeGuard
{
public:
    explicit SkipRSVBackgroundScopeGuard()
    {
        LOG_OUTPUT(L"> Skipping setting background on RootScrollViewer's content");
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DoNotSetRootScrollViewerBackground), true, &m_wasEnabled);
    }

    ~SkipRSVBackgroundScopeGuard()
    {
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DoNotSetRootScrollViewerBackground), m_wasEnabled, nullptr);
    }

private:
    bool m_wasEnabled;
};

template<RuntimeFeatureBehavior::RuntimeEnabledFeature F> class RuntimeEnabledFeatureScopeGuard
{
public:
    explicit RuntimeEnabledFeatureScopeGuard()
    {
        LOG_OUTPUT(L"> Setting RuntimeEnabledFeature");
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(F), true, &m_wasEnabled);
    }

    ~RuntimeEnabledFeatureScopeGuard()
    {
        TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(F), m_wasEnabled, nullptr);
    }

private:
    bool m_wasEnabled;
};

} } } } }
