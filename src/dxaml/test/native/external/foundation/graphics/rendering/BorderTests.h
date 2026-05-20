// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

class BorderTests : public WEX::TestClass<BorderTests>
{
public:
    BEGIN_TEST_CLASS(BorderTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Renders borders with SolidColorBrushes.")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;b7d949c9-6779-44c5-b16b-1f51e18f3866")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(RenderSolidColorBorders)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BorderDiscardMaskOnResizeWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GridDiscardMaskOnResizeWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ContentPresenterDiscardMaskOnResizeWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RectangleDiscardMaskOnResizeWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderTransparentBordersAndBackgrounds)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NineGridWithRoundedCorners)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundedCornersLayoutRounding)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // SetWindowSizeOverrideWithWindowScale doesn't work in islands mode
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundedCornersLayoutRounding2)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // SetWindowSizeOverrideWithWindowScale doesn't work in islands mode
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundedCornersLayoutRounding3)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // SetWindowSizeOverrideWithWindowScale doesn't work in islands mode
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundedCornersWithNoChildren)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NineGridOptimization)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // SetWindowSizeOverrideWithWindowScale doesn't work in islands mode
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NineGridOptimizationLayoutRounding)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // SetWindowSizeOverrideWithWindowScale doesn't work in islands mode
    END_TEST_METHOD()

    TEST_METHOD(AllBorderBorder)

private:
    inline Platform::String^ GetResourcesPath() const;

    void BorderDiscardMaskOnResizeInternal();
    void GridDiscardMaskOnResizeInternal();
    void ContentPresenterDiscardMaskOnResizeInternal();
    void RectangleDiscardMaskOnResizeInternal();

    Border^ MakeBorder(Panel^ parent, float width, float height, float x, float y);
    void AddBorderVariation(Panel^ parent, int x, Thickness thickness, CornerRadius cornerRadius, ImageBrush^ imageBrush);
};

} } } } } }


