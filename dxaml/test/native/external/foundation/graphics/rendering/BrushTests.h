// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include "FeatureFlags.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

using namespace Microsoft::UI::Xaml::Controls;

class BrushTests : public WEX::TestClass<BrushTests>
{
public:
    BEGIN_TEST_CLASS(BrushTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")

#if defined(_WIN64) // Disable the test on 64-bit build because of floating point differences between x87 and sse instructions
        TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ZeroSizedGradient)
        TEST_METHOD_PROPERTY(L"Description", L"Discard the bad gradient when the 0x0 element gets a size.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ChangeSolidColorBrushColor)
        TEST_METHOD_PROPERTY(L"Description", L"Change the color of a CSolidColorBrush.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LinearGradientAfterDeviceLoss)
        TEST_METHOD_PROPERTY(L"Description", L"Render with a linear gradient brush after a device loss.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
    BEGIN_TEST_METHOD(LinearGradientFacadeProperties)
        TEST_METHOD_PROPERTY(L"Description", L"Render using Facade properties")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LinearGradientFacadeAnimations)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Facade properties")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
#endif

    BEGIN_TEST_METHOD(UseWindowCompositionSystemBackdrop)
        TEST_METHOD_PROPERTY(L"Description", L"Gets and sets the Window's SystemBackdrop brush before and after DCompTreeHost's full setup, then resets it.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UseNullWindowCompositionSystemBackdrop)
        TEST_METHOD_PROPERTY(L"Description", L"Gets and resets the Window's already null SystemBackdrop brush.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopulatePropertyInfoOverride)
        TEST_METHOD_PROPERTY(L"Description", L"Implement PopulatePropertyInfoOverride virtual")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushChangeBrush)
        TEST_METHOD_PROPERTY(L"Description", L"Test varying the XCBB.CompositionBrush property.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushFallbackColor)
        TEST_METHOD_PROPERTY(L"Description", L"Fill canvas with XCBB.FallbackColor due to BitmapCache rendering.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushTransforms)
        TEST_METHOD_PROPERTY(L"Description", L"Test XCBB with various transforms applied to the brush or element.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushNullBrush)
        TEST_METHOD_PROPERTY(L"Description", L"Test nulling out XCBB's CompositionBrush.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushShapesAndControls)
        TEST_METHOD_PROPERTY(L"Description", L"Test XCBB's with a variety of elements and sharing of brushes between elements.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushListViewItem_PlateauScale)
        TEST_METHOD_PROPERTY(L"Description", L"Test XCBB's with a ListViewItem under plateau scaling.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Test animating element painted with XCBB's.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(XamlCompositionBrushOnConnected)
        TEST_METHOD_PROPERTY(L"Description", L"An XCBB should only call OnConnected when attached to a UIE, not a resource dictionary.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SolidColorBrushTransition)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListViewItemPresenterBrushTransition)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BrushTransition_Handoff)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SolidColorBrushTransition_SetStaticValueWhenComplete)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
    //void RenderTest(Platform::String^ fileName);
    Microsoft::UI::Xaml::Media::LinearGradientBrush^ MakeLinearGradientBrush(float startX, float endX);
    Microsoft::UI::Xaml::Controls::Panel^ CreateLinearGradientTestRoot(Microsoft::UI::Xaml::Media::LinearGradientBrush^ brush);
};

} } } } } }

