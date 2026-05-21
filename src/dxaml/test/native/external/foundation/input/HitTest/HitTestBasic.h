// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

class HitTestBasic : public WEX::TestClass<HitTestBasic>
{
public:
    BEGIN_TEST_CLASS(HitTestBasic)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    // StackPanel hit test regression
    BEGIN_TEST_METHOD(StackPanelHitTestRegression)
        TEST_METHOD_PROPERTY(L"Description", L"Verify StackPanel gets hit by hit testing")
    END_TEST_METHOD()

    // ScrollViewer inside Popup test regression
    BEGIN_TEST_METHOD(ScrollViewerInsidePopupTestRegression)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that elements inside a scrollviewer inside a popup only hit if they are visible")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestLayoutClip1WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Verify LayoutClip properly clips hit-testing: basic case")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestLayoutClip2WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Verify LayoutClip properly clips hit-testing: with layout offset")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestLayoutClip3WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Verify LayoutClip properly clips hit-testing: with RenderTransform")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestLayoutClip4WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Verify LayoutClip properly clips hit-testing: with RenderTransform on Panel")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestRoundedCornerClipWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verify rounded corner clipping properly clips hit-testing")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestBackgroundInsetWithinBorderWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verify hit-testing when background is inset within borders")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The cursor is going out of window bounds
   END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestBackgroundExtendingUnderBorderWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verify hit-testing when background extends under borders")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
   END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestInfiniteOffsetChildWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verify hit testing a panel with a child at an infinite offset")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestPlaneProjectionWUCFull)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestParentedPopup_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestParentedPopup3D_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestParentedPopup_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - XamlIslandRoots require a subtree root in the API
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestParentedPopup3D_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - XamlIslandRoots require a subtree root in the API
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestNestedParentedPopup_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestNestedParentedPopup3D_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestNestedParentedPopup_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - XamlIslandRoots require a subtree root in the API
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestNestedParentedPopup3D_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - XamlIslandRoots require a subtree root in the API
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestParentlessPopup_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // By design - XamlIslandRoots require a subtree root in the API; UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestParentlessPopup3D_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // By design - XamlIslandRoots require a subtree root in the API; UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestParentlessPopup_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestParentlessPopup3D_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestNestedParentlessPopup_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // By design - XamlIslandRoots require a subtree root in the API; UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestNestedParentlessPopup3D_SubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // By design - XamlIslandRoots require a subtree root in the API; UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestNestedParentlessPopup_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestNestedParentlessPopup3D_NullSubtreeRoot)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // By design - UAP cannot hit parentless popup contents with a non-null subtree root
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestPopupOpen_3DChild_PreTransformed)
         TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestPopupOpen_3DChild_PostTransformed)
         TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestLTE)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // LTE test hook can't add to transition root of island's popup root
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(HitTestLTE3D)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // LTE test hook can't add to transition root of island's popup root
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NewLTETargetingExisting3D)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // VisualTreeHelper::FindElementsInHostCoordinates with no element not supported with DesktopWindowXamlSource
    END_TEST_METHOD()

    TEST_METHOD(LTESkipsSubtree_DepthUnderLTE)
    TEST_METHOD(LTESkipsSubtree_DepthOnLTE)

    BEGIN_TEST_METHOD(DManipHitTestVisual)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ProgrammaticHitTestWithEmptyTree)
        // This test tears down the tree and recovers by shutting down and reinitializing Xaml. ShutdownXaml is skipped for non-desktop SKUs
        // due to a stale DComp tree associated with the window after the DComp device is recreated. See comment
        // in WindowHelper::ShutdownXaml.
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Simulates SurfacePresenter where the root is null. XamlIslandRoots not needed.
    END_TEST_METHOD()

    inline Platform::String^ GetResourcesPath() const;
    static bool VerifyPointContainsElementWithVisualTreeHelper(UIElement^ source, UIElement^ target, wf::Point point);
    static void VerifyTappedPoints(UIElement^ root, UIElement^ tappedElement, std::vector<wf::Point> points, int missedOffsetX, int missedOffsetY, bool verifyUsingTaps, bool doWindowSetup);
    static void VerifyTappedPoints(UIElement^ root, UIElement^ tappedElement, std::vector<wf::Point> points, int missedOffset, bool verifyUsingTaps);
    static void VerifyTappedPoints(UIElement^ root, UIElement^ tappedElement, std::vector<wf::Point> points);
    static void VerifyTappedPointsNoWindowSetup(bool specifySubtreeRootElement, UIElement^ tappedElement, std::vector<wf::Point> points);

    static void WindowSetup(UIElement^ root);

    static Microsoft::UI::Xaml::Controls::Canvas^ MakeCanvas(bool include3D, double left, double top, ::Windows::UI::Color color, Microsoft::UI::Xaml::Controls::Canvas^ child);
    static std::vector<wf::Point> GetHitTestingPoints(FrameworkElement^ element, UIElement^ root, float offset = 5.0f);

private:
    void HitTestLayoutClip1Internal();
    void HitTestLayoutClip2Internal();
    void HitTestLayoutClip3Internal();

    void HitTestPlaneProjectionCommon();

    void HitTestPopupCommon(bool isParented, bool include3D, bool includeNestedPopup, bool specifySubtreeRootElement);
    void HitTestPopupOpen_3DChild(bool preTransformed);

    void HitTestLTECommon(bool include3D);
    void SetLTETransform(Microsoft::UI::Xaml::UIElement^ element, bool include3D, double x, double y);

    void LTESkipsSubtreeCommon(bool isDepthOnLTE);
};

} } } } } } }

