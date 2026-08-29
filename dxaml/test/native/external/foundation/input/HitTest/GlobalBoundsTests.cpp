// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "GlobalBoundsTests.h"
#include <XamlTailored.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "HitTestBasic.h"
#include "WUCRenderingScopeGuard.h"
#include "TestCleanupWrapper.h"

using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace ::Windows::Foundation;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

bool GlobalBoundsTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool GlobalBoundsTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool GlobalBoundsTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void GlobalBoundsTests::GetGlobalBounds()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Grid^ parent;
    Grid^ element;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        element = ref new Grid();
        element->Width = 100;
        element->Height = 100;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        parent = ref new Grid();
        parent->Width = 200;
        parent->Height = 200;
        parent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        parent->Children->Append(element);

        root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(100, 50, 200, 200));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(150, 100, 100, 100));
    });
}

void GlobalBoundsTests::GetGlobalBounds_RenderTransform()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Grid^ parent;
    Grid^ element;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        TranslateTransform^ translate = ref new TranslateTransform();
        translate->X = 50;

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 0.5;

        element = ref new Grid();
        element->Width = 100;
        element->Height = 100;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element->RenderTransform = translate;

        parent = ref new Grid();
        parent->Width = 300;
        parent->Height = 200;
        parent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        parent->RenderTransform = scale;
        parent->Children->Append(element);

        root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(50, 50, 150, 200));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // X offset: 50 from parent, 100/2 layout offset from element, 50/2 render transform from element
        VERIFY_IS_TRUE(globalBounds == wf::Rect(125, 100, 50, 100));
    });
}

void GlobalBoundsTests::GetGlobalBounds_IncludesChildBounds()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Grid^ parent;
    Grid^ element;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 2;

        element = ref new Grid();
        element->Width = 100;
        element->Height = 100;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element->RenderTransform = scale;

        parent = ref new Grid();
        parent->Width = 100;
        parent->Height = 100;
        parent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        parent->Children->Append(element);

        root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // Includes a child that's 100x2 = 200 wide, so parent's bounds are also 200 wide
        VERIFY_IS_TRUE(globalBounds == wf::Rect(150, 100, 200, 100));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(150, 100, 200, 100));
    });
}

void GlobalBoundsTests::GetGlobalBounds_Clip()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Grid^ parent;
    Grid^ element;
    Grid^ element2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = wf::Rect(0, 0, 150, 50);

        TranslateTransform^ translate = ref new TranslateTransform();
        translate->X = -100;
        RectangleGeometry^ clip2 = ref new RectangleGeometry();
        clip2->Rect = wf::Rect(0, 0, 150, 50);
        clip2->Transform = translate;

        RectangleGeometry^ parentClip = ref new RectangleGeometry();
        parentClip->Rect = wf::Rect(0, 0, 75, 150);

        element = ref new Grid();
        element->Width = 200;
        element->Height = 200;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element->Clip = clip;

        element2 = ref new Grid();
        element2->Width = 200;
        element2->Height = 200;
        element2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element2->Clip = clip2;

        parent = ref new Grid();
        parent->Width = 200;
        parent->Height = 200;
        parent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        parent->Clip = parentClip;
        parent->Children->Append(element);
        parent->Children->Append(element2);

        root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(100, 50, 75, 150));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(100, 50, 75, 50));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element2, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(100, 50, 50, 50));
    });
}

void GlobalBoundsTests::GetGlobalBounds_LayoutClip()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Microsoft::UI::Xaml::Shapes::Rectangle^ element; // Not a panel so layout clip applies as a self clip

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        element = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
        element->Width = 500;
        element->Height = 500;
        element->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(element);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 400, 300));
    });
}

void GlobalBoundsTests::GetGlobalBounds_LayoutClipAsParentClip()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Grid^ parent;
    Grid^ element;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 0.5;

        element = ref new Grid();
        element->Width = 400;
        element->Height = 100;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element->RenderTransform = scale;

        parent = ref new Grid();
        parent->Width = 300;
        parent->Height = 400;
        parent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
        parent->Children->Append(element);

        root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(50, 0, 300, 300));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // X: 50 from parent, 0 from element
        // Width: parent clips itself down to 300, so the width is 400/2 = 200.
        //      If the clip was on the element, it would get clipped down to 300 first, and end up with 300/2 = 150.
        VERIFY_IS_TRUE(globalBounds == wf::Rect(50, 150, 200, 100));
    });
}

void GlobalBoundsTests::GetGlobalBounds_WindowClip()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ root;
    Grid^ element;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = wf::Rect(0, 0, 100, 150);

        element = ref new Grid();
        element->Width = 500;
        element->Height = 500;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        root = ref new Grid();
        root->Width = 600;
        root->Height = 600;
        root->Children->Append(element);
        root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(element, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // There is still 50 offset applied by the root
        VERIFY_IS_TRUE(globalBounds == wf::Rect(50, 50, 350, 250));
    });
}

void GlobalBoundsTests::CompareElementIterators(std::vector<UIElement^>& expected, ::Windows::Foundation::Collections::IIterable<UIElement^>^ actual)
{
    auto iExpected = expected.begin();
    auto iActual = actual->First();

    while (iExpected != expected.end() && iActual->HasCurrent)
    {
        VERIFY_IS_TRUE(*iExpected == iActual->Current, L"Hit test result should match expected element.");
        iExpected++;
        iActual->MoveNext();
    }
    VERIFY_IS_TRUE(iExpected == expected.end(), L"Should have matched all elements in the expected list.");
    VERIFY_IS_FALSE(iActual->HasCurrent, L"Hit test result should have the expected number of elements.");
}

void GlobalBoundsTests::GetGlobalBounds_LTEEscapesClips()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ d;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        RectangleGeometry^ bClip = ref new RectangleGeometry();
        bClip->Rect = wf::Rect(0, 0, 75, 75);

        RectangleGeometry^ cClip = ref new RectangleGeometry();
        cClip->Rect = wf::Rect(0, 0, 50, 100);

        RectangleGeometry^ dClip = ref new RectangleGeometry();
        dClip->Rect = wf::Rect(0, 0, 100, 50);

        d = ref new Canvas();
        d->Width = 200;
        d->Height = 200;
        d->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        d->Clip = dClip;

        Canvas^ c = ref new Canvas();
        c->Clip = cClip;
        c->Children->Append(d);

        Canvas^ b = ref new Canvas();
        b->Clip = bClip;
        b->Children->Append(c);

        wh->AddTestLTE(d /* target */, b /* parent */, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);

        Canvas^ a = ref new Canvas();
        a->Children->Append(b);

        Canvas^ root = ref new Canvas();
        root->Width = 600;
        root->Height = 600;
        root->Children->Append(a);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(d, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // The LTE escaped c's clip and d's clip, but b's clip should still apply
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 75, 75));
    });
}

void GlobalBoundsTests::GetGlobalBounds_ParentedPopupEscapesClips()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ partiallyClipped;
    Popup^ partiallyClippedPopup;
    Canvas^ fullyClipped;
    Popup^ fullyClippedPopup;
    Canvas^ clipWithPopupOffsetChild;
    Popup^ clipWithPopupOffset;

    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        RectangleGeometry^ aClip = ref new RectangleGeometry();
        aClip->Rect = wf::Rect(0, 0, 75, 500);

        RectangleGeometry^ popupClip = ref new RectangleGeometry();
        popupClip->Rect = wf::Rect(0, 0, 50, 100);

        RectangleGeometry^ clip1 = ref new RectangleGeometry();
        clip1->Rect = wf::Rect(0, 0, 50, 100);

        RectangleGeometry^ clip2 = ref new RectangleGeometry();
        clip2->Rect = wf::Rect(0, 0, 50, 100);

        partiallyClipped = ref new Canvas();
        partiallyClipped->Width = 200;
        partiallyClipped->Height = 200;
        partiallyClipped->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        partiallyClipped->Clip = clip1;

        partiallyClippedPopup = ref new Popup();
        partiallyClippedPopup->IsOpen = false;
        partiallyClippedPopup->HorizontalOffset = 50;
        partiallyClippedPopup->Child = partiallyClipped;

        fullyClipped = ref new Canvas();
        fullyClipped->Width = 200;
        fullyClipped->Height = 200;
        fullyClipped->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        fullyClipped->Clip = clip2;

        fullyClippedPopup = ref new Popup();
        fullyClippedPopup->IsOpen = false;
        fullyClippedPopup->HorizontalOffset = 100;
        fullyClippedPopup->Child = fullyClipped;

        clipWithPopupOffsetChild = ref new Canvas();
        clipWithPopupOffsetChild->Width = 200;
        clipWithPopupOffsetChild->Height = 200;
        clipWithPopupOffsetChild->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        clipWithPopupOffset = ref new Popup();
        clipWithPopupOffset->IsOpen = false;
        clipWithPopupOffset->HorizontalOffset = 25;
        clipWithPopupOffset->Child = clipWithPopupOffsetChild;
        clipWithPopupOffset->Clip = popupClip;

        Canvas^ b = ref new Canvas();
        b->Children->Append(partiallyClippedPopup);
        b->Children->Append(fullyClippedPopup);
        b->Children->Append(clipWithPopupOffset);

        Canvas^ a = ref new Canvas();
        a->Clip = aClip;
        a->Children->Append(b);

        Canvas^ root = ref new Canvas();
        root->Width = 600;
        root->Height = 600;
        root->Children->Append(a);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popups.");
        partiallyClippedPopup->IsOpen = true;
        fullyClippedPopup->IsOpen = true;
        clipWithPopupOffset->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(partiallyClipped, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // The popup should have opened and escaped all clips.
        VERIFY_IS_TRUE(globalBounds == wf::Rect(50, 0, 50, 100));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(fullyClipped, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // The popup should have opened and escaped all clips.
        VERIFY_IS_TRUE(globalBounds == wf::Rect(100, 0, 50, 100));
    });

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(clipWithPopupOffsetChild, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        // A popup's HorizontalOffset is applied on its child, so it applies below the popup's own clip.
        VERIFY_IS_TRUE(globalBounds == wf::Rect(25, 0, 25, 100));
    });
}

void GlobalBoundsTests::FindElementsInHostCoordinatesCommon(bool include3D)
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Canvas^ root;
    Canvas^ grandparent;
    Canvas^ parent;
    Canvas^ sibling;
    Canvas^ me;
    Canvas^ child;
    Canvas^ grandchild;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandchild = HitTestBasic::MakeCanvas(include3D, 50, 0, Microsoft::UI::Colors::Blue, nullptr);

        child = HitTestBasic::MakeCanvas(include3D, 50, 0, Microsoft::UI::Colors::Green, grandchild);

        me = HitTestBasic::MakeCanvas(include3D, 50, 0, Microsoft::UI::Colors::Yellow, child);

        sibling = HitTestBasic::MakeCanvas(include3D, 100, 0, Microsoft::UI::Colors::Yellow, nullptr);  // Overlaps "child"

        parent = HitTestBasic::MakeCanvas(include3D, 0, 50, Microsoft::UI::Colors::Orange, nullptr);
        parent->Children->Append(sibling);
        parent->Children->Append(me);

        grandparent = HitTestBasic::MakeCanvas(include3D, 0, 50, Microsoft::UI::Colors::Red, parent);

        root = ref new Canvas();
        if (include3D)
        {
            root->Transform3D = ref new PerspectiveTransform3D();
        }
        root->Children->Append(grandparent);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect rect(110, 90, 10, 20);
        wf::Point point(120, 130);

        // FEInHC returns the whole ancestor chain, regardless of whether the ancestors were hit.
        {
            std::vector<UIElement^> expected = { child, me, sibling, parent, grandparent, root };

            auto xamlRoot = root->XamlRoot;
            UIElement^ xamlRootContent = xamlRoot->Content;

            LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on XamlRoot->Content.");
            auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, xamlRootContent);
            CompareElementIterators(expected, rectHitTest);

            LOG_OUTPUT(L"> Calling point FindElementsInHostCoordinates on XamlRoot->Content.");
            auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, xamlRootContent);
            CompareElementIterators(expected, pointHitTest);
        }

        {
            std::vector<UIElement^> expected = {child, me, sibling, parent, grandparent, root};

            LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on root.");
            auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, root);
            CompareElementIterators(expected, rectHitTest);

            LOG_OUTPUT(L"> Calling point FindElementsInHostCoordinates on root.");
            auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, root);
            CompareElementIterators(expected, pointHitTest);
        }

        {
            // The ancestor chain returned by FEInHC stops at the subtree root.
            std::vector<UIElement^> expected = {child, me, sibling, parent};

            LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on parent.");
            auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, parent);
            CompareElementIterators(expected, rectHitTest);

            LOG_OUTPUT(L"> Calling point FindElementsInHostCoordinates on parent.");
            auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, parent);
            CompareElementIterators(expected, pointHitTest);
        }

        {
            // Sibling isn't returned. It's not in the subtree.
            std::vector<UIElement^> expected = {child, me};

            LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on me.");
            auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, me);
            CompareElementIterators(expected, rectHitTest);

            LOG_OUTPUT(L"> Calling point FindElementsInHostCoordinates on me.");
            auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, me);
            CompareElementIterators(expected, pointHitTest);
        }

        {
            std::vector<UIElement^> expected = {};

            LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on grandchild.");
            auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, grandchild);
            CompareElementIterators(expected, rectHitTest);

            LOG_OUTPUT(L"> Calling point FindElementsInHostCoordinates on grandchild.");
            auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, grandchild);
            CompareElementIterators(expected, pointHitTest);
        }
    });
}

void GlobalBoundsTests::FindElementsInHostCoordinates()
{
    FindElementsInHostCoordinatesCommon(false /* include3D */);
}

void GlobalBoundsTests::FindElementsInHostCoordinates3D()
{
    FindElementsInHostCoordinatesCommon(true /* include3D */);
}

void GlobalBoundsTests::FindElementsInHostCoordinatesWPF()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect rect(110, 90, 10, 20);
        wf::Point point(120, 130);

        {
            bool testPass = false;
            try
            {
                LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on null. Expect failure in WPF.");
                auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, nullptr);
            }
            catch (Platform::Exception^ e)
            {
                VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
                testPass = true;
            }
            VERIFY_IS_TRUE(testPass);
        }

        {
            bool testPass = false;
            try
            {
                LOG_OUTPUT(L"> Calling point FindElementsInHostCoordinates on null. Expect failure in WPF.");
                auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, nullptr);
            }
            catch (Platform::Exception^ e)
            {
                VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
                testPass = true;
            }
            VERIFY_IS_TRUE(testPass);
        }
    });
}

void GlobalBoundsTests::FindElementsInHostCoordinates_SwapChainPanel()
{
    const auto& wh = TestServices::WindowHelper;

    xaml_shapes::Rectangle^ rectangle;
    SwapChainPanel^ swapChainPanel;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        swapChainPanel = ref new SwapChainPanel();
        swapChainPanel->Width = 100;
        swapChainPanel->Height = 100;
        Canvas::SetLeft(swapChainPanel, 200);

        root = ref new Canvas();
        root->Children->Append(rectangle);
        root->Children->Append(swapChainPanel);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        std::vector<UIElement^> expected = { rectangle, root };

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(point) with includeAllElements. Don't return the SwapChainPanel.");
        wf::Point point(50, 50);
        auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, nullptr, true /* includeAllElements */);
        CompareElementIterators(expected, pointHitTest);

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(rect) with includeAllElements. Don't return the SwapChainPanel.");
        wf::Rect rect(70, 70, 10, 20);
        auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, root, true /* includeAllElements */);
        CompareElementIterators(expected, rectHitTest);
    });

    RunOnUIThread([&]()
    {
        std::vector<UIElement^> expected = { swapChainPanel, root };

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(point) with includeAllElements. Return the SwapChainPanel.");
        wf::Point point(250, 50);
        auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, root, true /* includeAllElements */);
        CompareElementIterators(expected, pointHitTest);

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(rect) with includeAllElements. Return the SwapChainPanel.");
        wf::Rect rect(270, 70, 10, 20);
        auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, nullptr, true /* includeAllElements */);
        CompareElementIterators(expected, rectHitTest);
    });
}

void GlobalBoundsTests::FindElementsInHostCoordinates_BaseItemChrome()
{
    const auto& wh = TestServices::WindowHelper;

    TestCleanupWrapper cleanup;

    xaml_shapes::Rectangle^ rectangle;
    xaml_shapes::Rectangle^ listViewRectangle;
    ListViewItemPresenter^ listViewBaseItemChrome;
    ListView^ listView;
    CalendarViewDayItem^ calendarViewBaseItemChrome;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        // ListViewBaseItemChrome assumes that it's used inside a ContentControl in many places, so we can't create one
        // explicitly and put it inside our Canvas. Instead, build a ListView for real and get to the LVBIChrome with
        // VisualTreeHelper later.
        listViewRectangle = ref new xaml_shapes::Rectangle();
        listViewRectangle->Width = 300;
        listViewRectangle->Height = 300;
        listViewRectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);

        listView = ref new ListView();
        listView->Width = 300;
        listView->Height = 300;
        listView->Items->Append(listViewRectangle);
        Canvas::SetLeft(listView, 200);

        calendarViewBaseItemChrome = ref new CalendarViewDayItem();
        calendarViewBaseItemChrome->Width = 100;
        calendarViewBaseItemChrome->Height = 100;
        // CalendarViewBaseItemChrome assumes that it has an owner and will crash if it's improperly used (outside a
        // CalendarView, like we're doing here). We can skip over the code that makes that assumption if we don't render
        // the CVBIChrome though.
        calendarViewBaseItemChrome->Opacity = 0;
        Canvas::SetLeft(calendarViewBaseItemChrome, 600);

        root = ref new Canvas();
        root->Children->Append(rectangle);
        root->Children->Append(listView);
        root->Children->Append(calendarViewBaseItemChrome);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Finding ListViewItemPresenter.");
        listViewBaseItemChrome = safe_cast<ListViewItemPresenter^>(VisualTreeHelper::GetParent(listViewRectangle));
        VERIFY_IS_NOT_NULL(listViewBaseItemChrome);

        // Collapse the rectangle so it won't be hit tested. Otherwise this leaf element gets hit, and automatically
        // adds its entire ancestor chain (including the LVBIChrome) to the hit test results. We want the LVBIChrome
        // to hit test itself explicitly.
        listViewRectangle->Visibility = Visibility::Collapsed;

        // Give the LVBIChrome a layout size so it will be hit tested.
        listViewBaseItemChrome->Width = 300;
        listViewBaseItemChrome->Height = 300;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        std::vector<UIElement^> expected = { rectangle, root };

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(point) with includeAllElements. Don't return the ListViewBaseItemPresenter or the CalendarViewBaseItem.");
        wf::Point point(50, 50);
        auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, root, true /* includeAllElements */);
        CompareElementIterators(expected, pointHitTest);

        wf::Rect rect(70, 70, 10, 20);

        auto xamlRoot = root->XamlRoot;
        UIElement^ xamlRootContent = xamlRoot->Content;

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(rect) with includeAllElements on XamlRoot->Content. Don't return the ListViewBaseItemPresenter or the CalendarViewBaseItem.");
        auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, xamlRootContent, true /* includeAllElements */);
        CompareElementIterators(expected, rectHitTest);
    });

    RunOnUIThread([&]()
    {
        std::vector<UIElement^> expected = { listViewRectangle, listViewBaseItemChrome, listView, root };

        wf::Point point(300, 100);
        wfc::IIterable<UIElement^>^ pointHitTest;

        auto xamlRoot = root->XamlRoot;
        UIElement^ xamlRootContent = xamlRoot->Content;

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(point) with includeAllElements on XamlRoot->Content. Return the ListViewBaseItemPresenter.");
        pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, xamlRootContent, true /* includeAllElements */);

        // There are so many elements in the item's ancestor chain, including Grids & Borders & ScrollViewers, that we don't bother
        // checking all of them. Just make sure the LVBIChrome is in there.
        {
            bool chromeFound = false;
            auto iActual = pointHitTest->First();
            while (iActual->HasCurrent)
            {
                if (iActual->Current == listViewBaseItemChrome)
                {
                    chromeFound = true;
                    break;
                }
                iActual->MoveNext();
            }
            VERIFY_IS_TRUE(chromeFound);
        }

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(rect) with includeAllElements. Return the ListViewBaseItemPresenter.");
        wf::Rect rect(330, 110, 10, 20);
        auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, root, true /* includeAllElements */);
        // There are so many elements in the item's ancestor chain, including Grids & Borders & ScrollViewers, that we don't bother
        // checking all of them. Just make sure the LVBIChrome is in there.
        {
            bool chromeFound = false;
            auto iActual = rectHitTest->First();
            while (iActual->HasCurrent)
            {
                if (iActual->Current == listViewBaseItemChrome)
                {
                    chromeFound = true;
                    break;
                }
                iActual->MoveNext();
            }
            VERIFY_IS_TRUE(chromeFound);
        }
    });

    RunOnUIThread([&]()
    {
        std::vector<UIElement^> expected = { calendarViewBaseItemChrome, root };

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(point) with includeAllElements. Return the CalendarViewBaseItem.");
        wf::Point point(650, 50);
        auto pointHitTest = VisualTreeHelper::FindElementsInHostCoordinates(point, root, true /* includeAllElements */);
        CompareElementIterators(expected, pointHitTest);

        wf::Rect rect(670, 70, 10, 20);

        auto xamlRoot = root->XamlRoot;
        UIElement^ xamlRootContent = xamlRoot->Content;

        LOG_OUTPUT(L"> Calling FindElementsInHostCoordinates(rect) with includeAllElements on XamlRoot->Content. Return the CalendarViewBaseItem.");
        auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, xamlRootContent, true /* includeAllElements */);
        CompareElementIterators(expected, rectHitTest);
    });

    RunOnUIThread([&]()
    {
        // The leak detector finds a leak unless the Rectangle that we put in the ListView gets cleared out explicitly.
        listView->Items->Clear();
    });
    wh->WaitForIdle();
}

void GlobalBoundsTests::GetGlobalBounds_ChildHas3D()
{
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(400, 300));

    Grid^ parent;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        CompositeTransform3D^ composite = ref new CompositeTransform3D();
        composite->TranslateZ = 0.1;

        Grid^ element = ref new Grid();
        element->Width = 100;
        element->Height = 100;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element->Transform3D = composite;

        parent = ref new Grid();
        parent->Children->Append(element);

        Grid^ root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(parent, false /* ignoreClipping */);
        LOG_OUTPUT(L"  > Global Bounds: {%f, %f, %f, %f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(150, 100, 100, 100));
    });
}

void GlobalBoundsTests::ProjectionMakesConcavePolygon()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        CompositeTransform3D^ transform3D = ref new CompositeTransform3D();
        transform3D->TranslateZ = 0.000001;

        // These numbers came from a crash dump. They're some combination of 2D and 3D transforms in a branch of the subtree.
        // We'll just inject them directly without setting up the same branch.
        Matrix3D matrix = Matrix3DHelper::FromElements(
            601.041,    -106.066,   707.107,    -0.707107,
            77.2798,    805.598,    50.5005,    0.656606,
            -77.2798,   608.616,    -50.5005,   -0.656606,
            41349.6,    69194.9,    -4414.36,   932.704);

        Matrix3DProjection^ projection = ref new Matrix3DProjection();
        projection->ProjectionMatrix = matrix;

        canvas = ref new Canvas();
        canvas->Projection = projection;
        canvas->Transform3D = transform3D;      // Guarantees that we'll walk down here, even if the input is way outside known bounds
        canvas->Children->Append(rectangle);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        std::vector<UIElement^> expected = {};

        wf::Rect rect(19148, 8119, 2300, 2300);

        LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates on a rect that's way outside the window bounds. Don't hit an assert.");
        auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, canvas);
        CompareElementIterators(expected, rectHitTest);
    });
}

} } } } } } }
