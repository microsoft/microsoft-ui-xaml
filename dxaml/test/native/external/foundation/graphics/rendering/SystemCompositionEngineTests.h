// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class SystemCompositionEngineTests : public WEX::TestClass<SystemCompositionEngineTests>
{
public:
    BEGIN_TEST_CLASS(SystemCompositionEngineTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(CompNode1WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp injection together. Verifies XAML uses the mock DComp device and the master-backed suite's tree dumps remain valid with the system composition engine.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode2WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode2.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode3WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode3.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode4WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode4.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode5WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode5.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode6WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode6.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode7WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode7.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode8WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode8.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode9WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode9.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode10WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode10.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode11WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode11.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode12WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode12.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode13WUCWithMockDComp)
        TEST_METHOD_PROPERTY(L"Description", L"System composition engine and MockDComp render coverage for CompNode13.xaml.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifySystemEngineEquivalent)
        TEST_METHOD_PROPERTY(L"Description", L"Uses CompositionEngine::GetForSystemEngine to verify that the selected engine's system equivalent is Windows.UI.Composition.Compositor. Because engine selection is process-wide, this proof verifies the complete CompositionEngineType.System run.")
    END_TEST_METHOD()

private:
    void LoadAndVerifyWithMockDComp(
        Platform::String^ markupFile,
        bool waitForIdle = true);
    void VerifySystemEngineEquivalentImpl();

    inline Platform::String^ GetResourcesPath() const;
};

} } } } } }
