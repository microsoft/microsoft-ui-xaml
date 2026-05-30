// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class CompNodeTests : public WEX::TestClass<CompNodeTests>
{
public:
    BEGIN_TEST_CLASS(CompNodeTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"9882729e-eea8-4b89-99e7-92145be50e76;bd1463b3-e5f2-4d54-9394-63a431c53a6e")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(CompNode9B)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode10B)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13B)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13C)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13D)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13E)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13G)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13I)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode9OptWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode9DynWUC)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // This test needs MockDComp in order to count the WUC expressions. MockDComp is disabled on OneCore.
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompNode1WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode2WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode3WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode4WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode5WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode6WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode7WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode8WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode9WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode10WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode11WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode12WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode13HWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode14WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode15WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode15BWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode16WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode17WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CompNode18WUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    TEST_METHOD(PopupDeviceLostWUCFull)

    BEGIN_TEST_METHOD(ABcD_RemoveB)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ABcD_RemoveBAddC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCommonParentNode)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XboxGuideRegression)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()    // <[19H1]:-[Xbox Friends list is missing content]>

    BEGIN_TEST_METHOD(NewCompNodeAboveCleanSubtreeContainingCompNode)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    void LoadAndVerify(
        Platform::String^ markupFile,
        Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering = Microsoft::UI::Xaml::Tests::Common::DCompRendering::WUCCompleteSynchronousCompTree,
        bool waitForIdle = true
        );
    inline Platform::String^ GetResourcesPath() const;

    BEGIN_TEST_METHOD(ReleaseExpressionsWUC)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // This test needs MockDComp in order to count the WUC expressions. MockDComp is disabled on OneCore.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride m_viewportInteractionOverride;
};

} } } } } }

