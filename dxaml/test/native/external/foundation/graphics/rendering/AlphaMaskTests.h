// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>
#include <wextestclass.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {
    class AlphaMaskTests
    {
    public:
        BEGIN_TEST_CLASS(AlphaMaskTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
            TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
#if defined (_WIN64) // Disable tests on 64 bit because of floating point differences between x87 and see instructions
            TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(TextBlockTests)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextBlockBeforeLayout)
            TEST_CLASS_PROPERTY(L"Description", L"Obtain the AlphaMask from a TextBlock before it's gone through layout")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextBlockSelection)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RenderTransforms)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Passes first iteration but fails the rest - two different masks in masters near the end
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BasicImageTests)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BasicShapeTests)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ShapeFillAlpha)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ShapeFillAlphaCleanup)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ShapeFillImageBrush)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MultiElement)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MultiElementPlateauScaleChange)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MultiElementDeviceLost)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MultiElementPopup)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParentlessRTLPopup)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RequestWhileOffered)
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Investigate test failures from IXP 9/10 drop: VerifySingleSelectionWithReorder, VerifyMultipleSelectionWithReorder, RequestWhileOffered
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RequestWhileDeviceLost)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RequestWhileDeviceGone)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ZeroSizeElementTests)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UseAfterClose)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        END_TEST_METHOD()
    };

} } } } } }


