// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RegKeyHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ImplicitAnimationTests : public WEX::TestClass<ImplicitAnimationTests>
{
public:
    BEGIN_TEST_CLASS(ImplicitAnimationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(HideAnimation1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit hide animation test - remove one element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation1BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit hide animation test - remove implicit animation after playing")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation1CWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit hide animation test - remove implicit animation while playing")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit hide animation test - remove all children")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - remove parent")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation3BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - remove parent with multiple animating children")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation3CWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - remove parent with multiple nested animating children")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation4WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - recursive trigger")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation4BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - nested Hides")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation5WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - changes to animating subtree (case 1)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation5BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - changes to animating subtree (case 2)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation6WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - root of XAML tree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation7WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - interrupt hide with show")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation7BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - toggle effective visibility for no net effect")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation7CWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - move to a different place in the tree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation8WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - create CompNode just in time")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation9WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - explicit multi-removal")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation10WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - using Canvas.ZIndex")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation11WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - Change Border Child")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InterruptNestedHideWithShow)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit show animation test - append one element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation1BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit show animation test - no property stomping")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit show animation test - insert one element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - insert nested elements (top to bottom)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation4WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - insert nested elements (bottom to top)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation4BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - insert nested elements (bottom to top, 2nd variation)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation4CWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - insert nested elements (bottom to top, 3rd variation)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation5WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - attach animation after already in the tree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation5BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - attach animation just after adding to the tree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation6WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - interrupt show with hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation6BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - toggle effective visibility (variation 2) for no net effect")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation7WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - changes to animating subtree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimation8WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - cancel by removing parent")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit hide animation test - Collapse one element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - recursive trigger")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse2BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - recursive trigger, 2nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse2CWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - recursive trigger, 3nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse2DWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - recursive trigger, 4th variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - changes to animating subtree (case 1)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse3BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - changes to animating subtree (case 2)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse4WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - interruption")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse4BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - collapse then uncollapse for no net effect")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse5WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - Collapse, then remove")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse6WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - Collapse and create CompNode just in time")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationCollapse7WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit hide animation test - Add collapsed element to the tree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Basic implicit show animation test - Uncollapse one element")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - Uncollapse nested elements (top to bottom)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - Uncollapse nested elements (bottom to top)")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse3BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - Uncollapse with recursive trigger")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse3CWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - Uncollapse with recursive trigger, 2nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse3DWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - Uncollapse with recursive trigger, 3rd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse4WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show animation test - attach animation after uncollapsing")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse5WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - interrupt show with hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse6WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - changes to animating subtree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse7WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - cancel by removing parent")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShowAnimationUncollapse8WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - uncollapse and remove")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup1)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Popup.IsOpen = True plays Show")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup1b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup2)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Setting Popup.Child plays Show")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup3)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Putting Popup into live tree plays Show")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup3b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup4)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Popup.IsOpen = False plays Hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup4b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup6)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Removing Popup from tree plays Hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup6b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup7)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Interrupt Show with Hide, variation #1 - open and close popup")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup7b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup8)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Interrupt Show with Hide, variation #2 - close and open popup")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup8b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup11)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Interrupt Show with Hide, variation #3 - open and remove from tree")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup11b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup12)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Interrupt Show with Hide, variation #4 - remove from tree, add to tree and open")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup12b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup13)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Nested Popups play Show")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup13b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup14)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Nested Popups play Hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup14b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup17)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Windowed Popups play Show")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup17b)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup18)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Windowed Popups play Hide")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup18b)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup19)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - animations directly on Popup")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Popup20)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - end to end typical scenario")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(Popup20b)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimation_CollapseOrRemovePopupChild)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ParentlessPopupShow)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ParentlessPopupShow2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ParentlessPopupHide)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ParentlessPopupHide2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ParentlessPopupShowHide)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ParentlessPopupShowHide2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimatePopupAndContent)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CollapsePopup)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CollapsePopup2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CollapseParentlessPopup)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CollapseParentlessPopup2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CollapsePopupAncestor)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CollapsePopupAncestor2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CollapseWhilePopupDescendantHasHideAnimation)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes, crash in WUX.dll
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(CollapseWhilePopupDescendantHasHideAnimation2)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp nodes
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Show on load")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Show on load, 2nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView2BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Show on setting new ItemsSource")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Show on load, 3rd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView4WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Show on load, 4th variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView5WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items do not play Show on item add")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView6WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView scrolling into view does not play Show on realized items")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView7WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Hide on ListView tree removal")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView8WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Hide on ListView Collapse")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView9WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView items play Hide on ListView ItemsSource reset")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView10WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView removal of one item does not play Hide animation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView11WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListView, end-to-end scenario, play Show then play Hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView12WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - interrupt Show with Hide")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ListView13WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - ListViewItem sub-element animation does not play")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GroupedListView1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Grouped ListView, group headers play Show/Hide animations on load/remove")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GridView1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - GridView items play Show on load")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GridView2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - GridView items play Hide on unload")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GridView3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - GridView items play Hide on tree removal")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimationsDisabled)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - does not play when animation setting is disabled")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ImplicitPlusThemeTransition)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - Implicit aimation in combination with ThemeTransition")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Closed)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - animation is Closed by the app")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Closed2)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - animation is Closed by the app, 2nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Closed3)
        TEST_METHOD_PROPERTY(L"Description", L"Implicit show/hide animation test - animation is Closed by the app, 3rd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HideAnimationBelowOpacity0)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    Platform::String^ GetResourcesPath() const;

    Microsoft::UI::Composition::Compositor^ GetCompositor();

    void Popup1Common(bool putAnimationOnPopup);
    void Popup3Common(bool putAnimationOnPopup);
    void Popup4Common(bool putAnimationOnPopup);
    void Popup6Common(bool putAnimationOnPopup);
    void Popup7Common(bool putAnimationOnPopup);
    void Popup8Common(bool putAnimationOnPopup);
    void Popup11Common(bool putAnimationOnPopup);
    void Popup12Common(bool putAnimationOnPopup);
    void Popup13Common(bool putAnimationOnPopup);
    void Popup14Common(bool putAnimationOnPopup);
    void Popup17Common(bool putAnimationOnPopup);
    void Popup18Common(bool putAnimationOnPopup);
    void Popup19Common(bool putAnimationOnPopup);
    void Popup20Common(bool putAnimationOnPopup);

    void ParentlessPopupShowHideCommon(bool show, bool hide, bool putAnimationOnPopup);
    void CollapsePopupCommon(bool parentlessPopup, bool putAnimationOnPopup);
    void CollapsePopupAncestorCommon(bool putAnimationOnPopup);
    void CollapseWhilePopupDescendantHasHideAnimationCommon(bool togglePopupVisibility);

    Microsoft::UI::Composition::ScalarKeyFrameAnimation^ CreateOpacityKFA(long durationInSeconds);
};
} } } } } }


