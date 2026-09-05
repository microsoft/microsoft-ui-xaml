// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class SwitcherTests : public WEX::TestClass<SwitcherTests>
{
public:
    BEGIN_TEST_CLASS(SwitcherTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(CompNode1WUCFullSwitcherWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"Switcher + MockDComp injection together. Certifies MockDComp still interposes correctly under switcher, so the master-backed suite's mock-based tree dumps remain valid under SwitcherMode.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyLiftedSystemCompositionPath)
        TEST_METHOD_PROPERTY(L"Description", L"Engagement certifier: uses CompositionEngine::GetForSystemEngine to verify the lifted compositor's system-engine equivalent is Windows.UI.Composition.Compositor (proves lifted->system routing, not a silent no-op). Because the backend flip is process-wide, this one proof certifies the whole SwitcherMode run.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    // Switcher + MockDComp variant: loads markup, runs render walk under switcher with
    // MockDComp interposed, dumps tree XML and compares to master.
    void LoadAndVerifySwitcherWithMockDComp(Platform::String^ markupFile);

    inline Platform::String^ GetResourcesPath() const;
};

} } } } } }
