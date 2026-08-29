// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class XamlDCompInteropTests : public WEX::TestClass<XamlDCompInteropTests>
{
public:
    BEGIN_TEST_CLASS(XamlDCompInteropTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(DCompVisualRTB)
        TEST_METHOD_PROPERTY(L"Description", L"Uses ElementCompositionPreview::GetElementVisual, then RTBs the element.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BasicUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the ElementCompositionPreview interface by accessing a DComp visual for a UIElement.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UIElementDCompVisualBeforeResourceCreationWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves a DComp visual through ElementCompositionPreview for a UIElement before the DComp resources were initialized.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UIElementDCompVisualWithDeviceLossWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the ElementCompositionPreview interface by accessing a DComp visual for a UIElement and then provokes a device loss.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UIElementDCompVisualAccessedDiscardedTwiceWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Accesses a DComp visual through ElementCompositionPreview twice.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UIElementDCompVisualsAccessedDiscardedSuccessivelyWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves and discards two distinct DComp visuals through ElementCompositionPreview for the same UIElement.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ManipulatableUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves and discards DComp visuals through ElementCompositionPreview for a DManip-manipulatable UIElement.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LeavingUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves a DComp visual through ElementCompositionPreview for a UIElement that is subsequently removed from the tree.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UnparentedUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves a DComp visual through ElementCompositionPreview for a UIElement that is not in the tree.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EnteringUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves a DComp visual through ElementCompositionPreview for a UIElement that is subsequently added to the tree.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InvisibleUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves a DComp visual through ElementCompositionPreview for an invisible UIElement.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DestroyUIElementDCompVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Retrieves a DComp visual through ElementCompositionPreview for a UIElement, then causes the UIElement's destruction.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;

    void BasicUIElementDCompVisualInternal();
    void BasicUIElementDCompVisualSharedInternal();
    void UIElementDCompVisualBeforeResourceCreationInternal();
    void UIElementDCompVisualWithDeviceLossInternal();
    void UIElementDCompVisualAccessedDiscardedTwiceInternal();
    void UIElementDCompVisualsAccessedDiscardedSuccessivelyInternal();
    void ManipulatableUIElementDCompVisualInternal();
    void LeavingUIElementDCompVisualInternal();
    void UnparentedUIElementDCompVisualInternal();
    void EnteringUIElementDCompVisualInternal();
    void InvisibleUIElementDCompVisualInternal();
    void DestroyUIElementDCompVisualInternal();

};


} } } } } }

