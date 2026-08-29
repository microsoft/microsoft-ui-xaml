// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>
#include <wincodec.h>

#include "Utilities.h"
#include "TestServices.h"
#include "SmartStackLogger.h"
#include "WindowHelper.h"
#include "ThemingHelper.h"
#include "VisualTreeDumper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include "IXamlTestHooks-win.h"
#include "RenderingScopeGuard.h"
#include "Hosting.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Private::Infrastructure;

namespace Private { namespace Infrastructure {

RenderingScopeGuard::RenderingScopeGuard(
    _In_ test_infra::DCompRendering rendering,
    _In_ BOOLEAN resizeWindow,
    _In_ BOOLEAN injectMockDComp,
    _In_ BOOLEAN resetDevice,
    _In_ BOOLEAN resetWindowContent)
    : m_rendering(rendering)
    , m_injectMockDComp(injectMockDComp)
    , m_resetDevice(resetDevice)
{
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
        &m_testServices
    ));
    LogThrow_IfFailed(m_testServices->get_WindowHelper(&m_spWindowHelper));
    LogThrow_IfFailed(m_testServices->get_Utilities(&m_spUtilities));

    bool enableVisualDebugTags = false;
    bool enableWUCShapes = false;
    bool enableSynchronousCompTree = false; // TODO: this can be deleted. A couple of if statements in HWCompTreeNodeWinRT need to turn into asserts first.

    switch (m_rendering)
    {
        case test_infra::DCompRendering::DCompRendering_WUCCompleteSynchronousCompTree:
            enableVisualDebugTags = true;
            enableSynchronousCompTree = true;
            break;

        case test_infra::DCompRendering::DCompRendering_WUCShapesSynchronousCompTree:
            enableVisualDebugTags = true;
            enableWUCShapes = true;
            enableSynchronousCompTree = true;
            break;
    }

    LogThrow_IfFailed(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableVisualDebugTags), enableVisualDebugTags, &m_wereDebugTagsEnabled));
    LogThrow_IfFailed(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableWUCShapes), enableWUCShapes, &m_wereWUCShapesEnabled));
    LogThrow_IfFailed(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::SynchronousCompTreeUpdates), enableSynchronousCompTree, &m_wasSynchronousCompTreeEnabled));

    if (m_injectMockDComp)
    {
        LogThrow_IfFailed(m_spWindowHelper->InjectMockDComp());
    }

    if (m_resetDevice)
    {
        // Inject a device lost so that we reset the cached flags in DCompTreeHost, useful when injectMockDComp is false
        LogThrow_IfFailed(m_spWindowHelper->ResetDeviceAndVisuals());
    }

    if (resizeWindow)
    {
        wf::Size size = { 400, 300 };
        LogThrow_IfFailed(m_spWindowHelper->SetWindowSizeOverride(size));
    }

    if (resetWindowContent)
    {
        // Wait for the device lost injected to be picked up. This resets the "is in WUC mode" flags in DCompTreeHost and ensures we'll
        // read the regkeys again. Set the default window content so that WaitForIdle doesn't hang.
        // Note:  Tests that supply resetWindowContent == false must ensure that they will set Window.Content themselves.
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IInspectable> spCanvas;
            LogThrow_IfFailed(wf::ActivateInstance(
                wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Canvas).Get(),
                &spCanvas
            ));
            wrl::ComPtr<xaml::IUIElement> spElement;
            LogThrow_IfFailed(spCanvas.As(&spElement));
            LogThrow_IfFailed(m_spWindowHelper->put_WindowContent(spElement.Get()));
        });
        LogThrow_IfFailed(m_spWindowHelper->WaitForIdle());
    }
}

HRESULT RenderingScopeGuard::Close()
{
    LogThrow_IfFailed(m_spWindowHelper->ClearTestLTEs());
    LogThrow_IfFailed(m_spWindowHelper->CleanUpAfterTest());
    LogThrow_IfFailed(m_spWindowHelper->ResetWindowContentAndWaitForIdle());

    if (m_injectMockDComp)
    {
        LogThrow_IfFailed(m_spWindowHelper->DetachMockDComp());
    }
    if (m_resetDevice)
    {
        LogThrow_IfFailed(m_spWindowHelper->ResetDeviceAndVisuals());
    }

    LogThrow_IfFailed(m_spWindowHelper->SetTimeManagerClockOverrideConstant(-1));

    LogThrow_IfFailed(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableVisualDebugTags), m_wereDebugTagsEnabled, nullptr));
    LogThrow_IfFailed(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableWUCShapes), m_wereWUCShapesEnabled, nullptr));
    LogThrow_IfFailed(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::SynchronousCompTreeUpdates), m_wasSynchronousCompTreeEnabled, nullptr));

    // DComp XML variables (set via SetDCompXmlVariable) only take effect under the
    // MockDComp injected inside a rendering scope, so this is the natural place to
    // clear them. Doing it here means any test using a rendering scope gets variable
    // cleanup for free -- no separate guard needed. Clearing an empty set is a no-op.
    LogThrow_IfFailed(m_spUtilities->ClearDCompXmlVariables());

    return S_OK;
}

} }

