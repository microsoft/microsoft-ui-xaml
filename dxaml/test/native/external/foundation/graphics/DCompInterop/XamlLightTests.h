// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <HostingModeTestClass.h>

#include <RegKeyHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class XamlLightTests : public WEX::TestClass<XamlLightTests>
{
public:
    BEGIN_TEST_CLASS(XamlLightTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        TEST_CLASS_HOSTING_MODE(UAP)
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(BasicTest)
        TEST_METHOD_PROPERTY(L"Description", L"Puts a test light in the tree with a child element that wants to be targeted.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetEntersTree)
        TEST_METHOD_PROPERTY(L"Description", L"Adds new light targets to the tree.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetLeavesTree)
        TEST_METHOD_PROPERTY(L"Description", L"Removes existing light targets from the tree.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetChangesTargets)
        TEST_METHOD_PROPERTY(L"Description", L"Adds new IDs and removes old IDs from existing light targets in the tree.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightEntersTree)
        TEST_METHOD_PROPERTY(L"Description", L"Adds new lights to the tree.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightLeavesTree)
        TEST_METHOD_PROPERTY(L"Description", L"Removes existing lights from the tree.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightMovesInTree)
        TEST_METHOD_PROPERTY(L"Description", L"Moves lights from one place in the tree to another.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetsLTETarget)
        TEST_METHOD_PROPERTY(L"Description", L"Targeting an element that's itself the target of an LTE.")
        // Light coordinate space comp node is different
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightDisconnectedAfterTreeReset)
        TEST_METHOD_PROPERTY(L"Description", L"XamlLights get an OnDisconnected call to clean up when we tear down the tree.")
        // We can't recover from resetting the visual tree, so run this test in its own process.
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
        // Light coordinate space comp node is different
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompositionLightChanges)
        TEST_METHOD_PROPERTY(L"Description", L"Changes the CompositionLight inside the XamlLight.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ElementRegeneratesContent)
        TEST_METHOD_PROPERTY(L"Description", L"The target element makes new sprite visuals. They need to be targeted too.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetsBrushBasic)
        TEST_METHOD_PROPERTY(L"Description", L"Targeting a brush with a XamlLight. The brush is not shared.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetsBrushShared)
        TEST_METHOD_PROPERTY(L"Description", L"Targeting a brush that's shared in the tree.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LightTargetsBrushShared_DifferentLights)
        TEST_METHOD_PROPERTY(L"Description", L"One brush that's shared, targeted by different lights in different subtrees")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BrushAndElementTargeting)
        TEST_METHOD_PROPERTY(L"Description", L"Targeting the element explicitly as well as brushes set on it")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ROEFailTest)
        TEST_METHOD_PROPERTY(L"Description", L"Closes the underlying WUC light while the Xaml light is still in use.")
        // Light coordinate space comp node is different
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ElementZIndexChanges)
        TEST_METHOD_PROPERTY(L"Description", L"Changes the ZIndex of targeted elements.")
        // Light coordinate space comp node is different
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ChangeLightIdTest)
        TEST_METHOD_PROPERTY(L"Description", L"Changes the Id of an existing light.")
        // Light coordinate space comp node is different
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RootIsGrid)
        TEST_METHOD_PROPERTY(L"Description", L"Set a light on the RootScrollViewer. It should be lifted up to the CRootVisual in RS3+.")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MuxLightsInCoreWindow)
    END_TEST_METHOD()

private:
    Platform::String^ GetResourcesPath() const;

    void RootIsGridCommon(unsigned int expectedLightTargetCount);

    Microsoft::UI::Xaml::Controls::Canvas^ CreateCanvas(Microsoft::UI::Xaml::Media::Brush^ brush);

    Microsoft::UI::Xaml::Controls::Canvas^ CreateCanvas();
    Microsoft::UI::Xaml::Controls::Grid^ CreateGrid();
    Microsoft::UI::Xaml::Controls::Border^ CreateBorder();

    void NumberSpriteVisuals();
    Microsoft::UI::Composition::SpotLight^ CreateSpotLight();

    void MuxLightsCommon(bool isInIsland);
};

    class XamlLightTestsWpf : public WEX::TestClass<XamlLightTestsWpf>
    {
    public:
        BEGIN_TEST_CLASS(XamlLightTestsWpf)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        TEST_CLASS_PROPERTY(L"MasterFile:ClassName", L"XamlLightTests")
        TEST_CLASS_HOSTING_MODE_DEFAULT()
    END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

    private:
        Microsoft::UI::Composition::SpotLight^ CreateSpotLight();
        Microsoft::UI::Xaml::Controls::Canvas^ CreateCanvas(Microsoft::UI::Xaml::Media::Brush^ brush);
        Microsoft::UI::Xaml::Controls::Canvas^ CreateCanvas();
        void MuxLightsCommon(bool isInIsland);
        Microsoft::UI::Xaml::Controls::Grid^ CreateGrid();

    public:
        BEGIN_TEST_METHOD(APITest)
        TEST_METHOD_PROPERTY(L"Description", L"Puts a test light in the tree. Should not crash.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopup_RemoveLightsFromPopupRoot)
        TEST_METHOD_PROPERTY(L"Description", L"Set a light on the PopupRoot, open a windowed Popup, then remove the light on the PopupRoot.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        // Light coordinate space comp node is different
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MuxLightsInIslands)
        // This specifically tests islands
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()
    };

// Test light types

ref class TestLight sealed : public Microsoft::UI::Xaml::Media::XamlLight
{
public:
    TestLight(Microsoft::UI::Composition::SpotLight^ spotLight, Platform::String^ id);
    Platform::String^ GetId() override;
    void OnConnected(Microsoft::UI::Xaml::UIElement^ newElement) override;
    void OnDisconnected(Microsoft::UI::Xaml::UIElement^ oldElement) override;

    void VerifyElementsAndReset(
        Microsoft::UI::Xaml::UIElement^ connectedElement,
        Microsoft::UI::Xaml::UIElement^ disconnectedElement);

    void SetWUCLight(Microsoft::UI::Composition::SpotLight^ wucLight);

    void SetNewId(Platform::String^ newId);

private:

    Platform::String^ m_id;
    Microsoft::UI::Xaml::UIElement^ m_onConnectedElement;
    Microsoft::UI::Xaml::UIElement^ m_onDisconnectedElement;
};

ref class TestBrush sealed : public Microsoft::UI::Xaml::Media::XamlCompositionBrushBase
{
public:
    TestBrush(TestLight^ light);

    void OnConnected() override;
    void OnDisconnected() override;

private:
    TestLight^ m_light;
};

} } } } } }

