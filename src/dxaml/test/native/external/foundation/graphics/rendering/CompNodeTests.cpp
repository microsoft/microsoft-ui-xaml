// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CompNodeTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Shapes;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ CompNodeTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
}

bool CompNodeTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool CompNodeTests::ClassCleanup()
{
    return true;
}

bool CompNodeTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool CompNodeTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void CompNodeTests::LoadAndVerify(Platform::String^ markupFile, DCompRendering rendering, bool waitForIdle)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(rendering);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + markupFile));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });

    // WaitForIdle will cause us to wait until animations complete and animated properties have
    // static values, so animation expressions would not be present in the dump. We want to capture
    // those expressions, so for tests with animations, take the dump after a few ticks, while animation is running.
    if (waitForIdle)
    {
        wh->WaitForIdle();
    }
    else
    {
        wh->SynchronouslyTickUIThread(3);
    }

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void CompNodeTests::CompNode9B()
{
    LoadAndVerify(L"CompNode9B.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode10B()
{
    LoadAndVerify(L"CompNode10B.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode13B()
{
    LoadAndVerify(L"CompNode13B.xaml");
}

void CompNodeTests::CompNode13C()
{
    LoadAndVerify(L"CompNode13C.xaml");
}

void CompNodeTests::CompNode13D()
{
    LoadAndVerify(L"CompNode13D.xaml");
}

void CompNodeTests::CompNode13E()
{
    LoadAndVerify(L"CompNode13E.xaml");
}

void CompNodeTests::CompNode13G()
{
    LoadAndVerify(L"CompNode13G.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode13I()
{
    LoadAndVerify(L"CompNode13I.xaml");
}

void CompNodeTests::CompNode9OptWUC()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CompNode9B.xaml"));
    Storyboard^ sb = nullptr;
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        sb = safe_cast<Storyboard^>(root->FindName(L"myStoryboard"));
        sb->Stop();
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Stop");

    RunOnUIThread([&]()
    {
        sb->Begin();
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

}

void CompNodeTests::CompNode9DynWUC()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    MockDComp::IMockDCompDevice^ mockDevice = wh->MockDCompDevice;
    MockDComp::IMockDCompDevice2^ mockDevice2 = safe_cast<MockDComp::IMockDCompDevice2^>(mockDevice);
    unsigned int expressionCount;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CompNode9B.xaml"));
    Canvas^ canvas = nullptr;
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        canvas = safe_cast<Canvas^>(root->FindName(L"myCanvas"));
        canvas->FlowDirection = xaml::FlowDirection::RightToLeft;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"RTL");

    RunOnUIThread([&]()
    {
        canvas->FlowDirection = xaml::FlowDirection::LeftToRight;
    });
    wh->SynchronouslyTickUIThread(2);

    // At this point in time we just released the RTL expression, and no longer have FlowDirection in the UBER expression.
    // It would be nice to verify via a mock dump but unfortunately our mock layer has problems releasing mock expressions
    // and would dump an inaccurate view of the visual tree - it would incorrectly show the expression still exists when it really doesn't.
    // Instead, we'll simply verify the expression count.
    RunOnUIThread([&]()
    {
        mockDevice2->GetWUCExpressionCount(&expressionCount);
        VERIFY_ARE_EQUAL(expressionCount, 4u);
    });
}

void CompNodeTests::CompNode1WUCFull()
{
    LoadAndVerify(L"CompNode1.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode2WUCFull()
{
    LoadAndVerify(L"CompNode2.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode3WUCFull()
{
    LoadAndVerify(L"CompNode3.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode4WUCFull()
{
    LoadAndVerify(L"CompNode4.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode5WUCFull()
{
    LoadAndVerify(L"CompNode5.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode6WUCFull()
{
    LoadAndVerify(L"CompNode6.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode7WUCFull()
{
    LoadAndVerify(L"CompNode7.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode8WUCFull()
{
    LoadAndVerify(L"CompNode8.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode9WUCFull()
{
    LoadAndVerify(L"CompNode9.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode10WUCFull()
{
    LoadAndVerify(L"CompNode10.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode11WUCFull()
{
    LoadAndVerify(L"CompNode11.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode12WUCFull()
{
    LoadAndVerify(L"CompNode12.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode13WUCFull()
{
    LoadAndVerify(L"CompNode13.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode13HWUCFull()
{
    LoadAndVerify(L"CompNode13H.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode14WUCFull()
{
    LoadAndVerify(L"CompNode14.xaml", DCompRendering::WUCCompleteSynchronousCompTree);
}

void CompNodeTests::CompNode15WUCFull()
{
    LoadAndVerify(L"CompNode15.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode15BWUCFull()
{
    LoadAndVerify(L"CompNode15B.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode16WUCFull()
{
    LoadAndVerify(L"CompNode16.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode17WUCFull()
{
    LoadAndVerify(L"CompNode17.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::CompNode18WUCFull()
{
    LoadAndVerify(L"CompNode18.xaml", DCompRendering::WUCCompleteSynchronousCompTree, false /* waitForIdle */);
}

void CompNodeTests::PopupDeviceLostWUCFull()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas;
    xaml_primitives::Popup^ popup;
    xaml_primitives::Popup^ nestedPopup;

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup = ref new xaml_primitives::Popup();
        nestedPopup = ref new xaml_primitives::Popup();
        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;
        nestedPopup->Child = rect;
        popup->Child = nestedPopup;
        rootCanvas->Children->Append(popup);
        popup->IsOpen = true;
        nestedPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"Simulating device lost, verify no crash");
    wh->SimulateDeviceLost();
    wh->WaitForIdle();
}

void CompNodeTests::ReleaseExpressionsWUC()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas;
    Border^ border;
    TranslateTransform^ translate;
    CompositeTransform3D^ compositeT3D;
    PlaneProjection^ projection;

    MockDComp::IMockDCompDevice^ mockDevice = wh->MockDCompDevice;
    MockDComp::IMockDCompDevice2^ mockDevice2 = safe_cast<MockDComp::IMockDCompDevice2^>(mockDevice);
    unsigned int expressionCount;

    RunOnUIThread([&]()
    {
        translate = ref new TranslateTransform();

        RotateTransform^ rotate = ref new RotateTransform();

        TransformGroup^ transformGroup = ref new TransformGroup();
        transformGroup->Children->Append(translate);
        transformGroup->Children->Append(rotate);

        compositeT3D = ref new CompositeTransform3D();

        projection = ref new PlaneProjection();

        border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        border->RenderTransform = transformGroup;
        border->Transform3D = compositeT3D;
        border->Projection = projection;

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(border);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();

    LOG_OUTPUT(L"[0 expressions expected: everything is static]");
    mockDevice2->GetWUCExpressionCount(&expressionCount);
    VERIFY_ARE_EQUAL(0u, expressionCount);

    Storyboard^ translateSB = nullptr;
    Storyboard^ transform3DSB = nullptr;
    Storyboard^ projectionSB = nullptr;
    Storyboard^ opacitySB = nullptr;
    Storyboard^ offsetSB = nullptr;

    RunOnUIThread([&]()
    {
        ::Windows::Foundation::TimeSpan span; span.Duration = 100000000L;

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 0.0;
        da->To = 100.0;
        da->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da, translate);
        Storyboard::SetTargetProperty(da, L"X");

        translateSB = ref new Storyboard();
        translateSB->Children->Append(da);

        DoubleAnimation^ da2 = ref new DoubleAnimation();
        da2->From = 0.0;
        da2->To = 100.0;
        da2->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da2, compositeT3D);
        Storyboard::SetTargetProperty(da2, L"RotationX");

        transform3DSB = ref new Storyboard();
        transform3DSB->Children->Append(da2);

        DoubleAnimation^ da3 = ref new DoubleAnimation();
        da3->From = 0.0;
        da3->To = 100.0;
        da3->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da3, projection);
        Storyboard::SetTargetProperty(da3, L"RotationX");

        projectionSB = ref new Storyboard();
        projectionSB->Children->Append(da3);

        DoubleAnimation^ da4 = ref new DoubleAnimation();
        da4->From = 0.0;
        da4->To = 1.0;
        da4->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da4, border);
        Storyboard::SetTargetProperty(da4, L"Opacity");

        opacitySB = ref new Storyboard();
        opacitySB->Children->Append(da4);

        DoubleAnimation^ da5 = ref new DoubleAnimation();
        da5->From = 0.0;
        da5->To = 100.0;
        da5->Duration = DurationHelper::FromTimeSpan(span);
        Storyboard::SetTarget(da5, border);
        Storyboard::SetTargetProperty(da5, L"(Canvas.Left)");

        offsetSB = ref new Storyboard();
        offsetSB->Children->Append(da5);

        translateSB->Begin();
        transform3DSB->Begin();
        projectionSB->Begin();
        opacitySB->Begin();
        offsetSB->Begin();
    });
    wh->SynchronouslyTickUIThread(1);

    // Note: These checks need to run on the UI thread. Otherwise, we could be getting the WUC expression count while the UI thread is
    // rendering a frame and creating expressions, which creates nondeterministic behavior.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[12 expressions expected: uber, 2D transforms (translate + rotate + group), 3D transform + rotate component, projection + rotation component + rotateX component + perspective component, WUC visual's opacity, offset]");
        mockDevice2->GetWUCExpressionCount(&expressionCount);
        VERIFY_ARE_EQUAL(12u, expressionCount);
    });

    RunOnUIThread([&]()
    {
        translateSB->Stop();
        transform3DSB->Stop();
        offsetSB->Stop();
    });
    wh->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        // The offset expression was cleaned up because the storyboard stopped.
        //
        // The rotate component of the 3D transform is cleaned up because it's no longer animated.
        //
        // The 3D transform and transform groups still keep their expressions, because the TransformMatrix is still animated via the
        // projection. Once that animation stops, everything should get cleaned up.
        LOG_OUTPUT(L"[10 expressions expected: uber, 2D transforms (translate + rotate + group), 3D transform, projection + rotation component + rotateX component + perspective component, WUC visual's opacity]");
        mockDevice2->GetWUCExpressionCount(&expressionCount);
        VERIFY_ARE_EQUAL(10u, expressionCount);
    });

    RunOnUIThread([&]()
    {
        projectionSB->Stop();
        opacitySB->Stop();
    });
    wh->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"[0 expressions expected: everything is static again]");
        mockDevice2->GetWUCExpressionCount(&expressionCount);
        VERIFY_ARE_EQUAL(0u, expressionCount);
    });
}

void CompNodeTests::ABcD_RemoveB()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ a;
    Canvas^ b;
    Canvas^ c;
    Canvas^ d;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        d = ref new Canvas();
        d->Width = 100;
        d->Height = 100;
        d->Background = ref new SolidColorBrush(Colors::Red);
        d->CompositeMode = ElementCompositeMode::SourceOver;

        c = ref new Canvas();
        c->Width = 200;
        c->Height = 200;
        c->Background = ref new SolidColorBrush(Colors::Red);
        c->Children->Append(d);

        b = ref new Canvas();
        b->Width = 300;
        b->Height = 300;
        b->Background = ref new SolidColorBrush(Colors::Red);
        b->CompositeMode = ElementCompositeMode::SourceOver;
        b->Children->Append(c);

        a = ref new Canvas();
        a->Width = 400;
        a->Height = 400;
        a->Background = ref new SolidColorBrush(Colors::Red);
        a->CompositeMode = ElementCompositeMode::SourceOver;
        a->Children->Append(b);

        wh->WindowContent = a;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing comp node B.");
        b->CompositeMode = ElementCompositeMode::Inherit;
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void CompNodeTests::ABcD_RemoveBAddC()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ a;
    Canvas^ b;
    Canvas^ c;
    Canvas^ d;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        d = ref new Canvas();
        d->Width = 100;
        d->Height = 100;
        d->Background = ref new SolidColorBrush(Colors::Red);
        d->CompositeMode = ElementCompositeMode::SourceOver;

        c = ref new Canvas();
        c->Width = 200;
        c->Height = 200;
        c->Background = ref new SolidColorBrush(Colors::Red);
        c->Children->Append(d);

        b = ref new Canvas();
        b->Width = 300;
        b->Height = 300;
        b->Background = ref new SolidColorBrush(Colors::Red);
        b->CompositeMode = ElementCompositeMode::SourceOver;
        b->Children->Append(c);

        a = ref new Canvas();
        a->Width = 400;
        a->Height = 400;
        a->Background = ref new SolidColorBrush(Colors::Red);
        a->CompositeMode = ElementCompositeMode::SourceOver;
        a->Children->Append(b);

        wh->WindowContent = a;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing comp node B.");
        b->CompositeMode = ElementCompositeMode::Inherit;

        LOG_OUTPUT(L"> Adding comp node C.");
        c->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void CompNodeTests::RemoveCommonParentNode()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ parent;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        Canvas^ a = ref new Canvas();
        a->Width = 10;
        a->Height = 10;
        a->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ b = ref new Canvas();
        b->Width = 20;
        b->Height = 20;
        b->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ c = ref new Canvas();
        c->Width = 30;
        c->Height = 30;
        c->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ d = ref new Canvas();
        d->Width = 40;
        d->Height = 40;
        d->CompositeMode = ElementCompositeMode::SourceOver;

        parent = ref new Canvas();
        parent->CompositeMode = ElementCompositeMode::SourceOver;
        parent->Children->Append(a);
        parent->Children->Append(b);
        parent->Children->Append(c);
        parent->Children->Append(d);

        Canvas^ root = ref new Canvas();
        root->Children->Append(parent);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing comp node on parent. The children should still be ordered correctly after reparenting.");
        parent->CompositeMode = ElementCompositeMode::Inherit;
    });

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void CompNodeTests::XboxGuideRegression()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    //
    //  <Canvas A CompNode>
    //      <Canvas B CompNode />   <!-- Clean element containing comp node -->
    //      <Canvas C Content />    <!-- New comp node added above B -->
    //      <Canvas D CompNode />   <!-- Another clean element containing comp node -->
    //      <Canvas E Content />
    //      <Canvas F Content />    <!-- New comp node added above D, with a SpriteVisual in between -->
    //  </Canvas A>
    //
    // Then give C and F comp nodes. C's comp node needs to be inserted behind B, and F's comp node needs to be inserted
    // behind D. If B and D aren't correctly found, then A's child comp nodes will be in the wrong order.
    //
    // Then take A's comp node away. We reparent A's comp node children to the parent, and if A's comp node children are
    // in the wrong order, then the WUC visuals will be reparented in the wrong order, which will show up in MockDComp.
    //

    Canvas^ a;
    Canvas^ c;
    Canvas^ f;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        a = ref new Canvas();
        a->Width = 10;
        a->Height = 10;
        a->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ b = ref new Canvas();
        b->Width = 20;
        b->Height = 20;
        b->CompositeMode = ElementCompositeMode::SourceOver;

        c = ref new Canvas();
        c->Width = 30;
        c->Height = 30;
        c->Background = ref new SolidColorBrush(Colors::Red);

        Canvas^ d = ref new Canvas();
        d->Width = 40;
        d->Height = 40;
        d->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ e = ref new Canvas();
        e->Width = 50;
        e->Height = 50;
        e->Background = ref new SolidColorBrush(Colors::Red);

        f = ref new Canvas();
        f->Width = 60;
        f->Height = 60;
        f->Background = ref new SolidColorBrush(Colors::Red);

        a->Children->Append(b);
        a->Children->Append(c);
        a->Children->Append(d);
        a->Children->Append(e);
        a->Children->Append(f);

        Canvas^ root = ref new Canvas();
        root->Children->Append(a);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving comp node to c and f. The comp node children should be ordered correctly.");
        c->CompositeMode = ElementCompositeMode::SourceOver;
        f->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing comp node from a. The WUC visuals should be ordered correctly.");
        a->CompositeMode = ElementCompositeMode::Inherit;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void CompNodeTests::NewCompNodeAboveCleanSubtreeContainingCompNode()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    //
    //  <Canvas A CompNode>
    //      <Canvas B>              <!-- Clean subtree containing comp node -->
    //          <Canvas C CompNode />
    //      </Canvas B>
    //      <Canvas D Content />    <!-- New comp node added above B -->
    //      <Canvas E>              <!-- Another clean subtree containing comp node -->
    //          <Canvas F CompNode />
    //      </Canvas E>
    //      <Canvas G Content />
    //      <Canvas H Content />    <!-- New comp node adde above E, with a SpriteVisual in between -->
    //  </Canvas A>
    //
    // Then give D and H comp nodes. D's comp node needs to be inserted behind C, and H's comp node needs to be inserted
    // behind F. If C and F aren't correctly found, then A's child comp nodes will be in the wrong order.
    //
    // Then take A's comp node away. We reparent A's comp node children to the parent, and if A's comp node children are
    // in the wrong order, then the WUC visuals will be reparented in the wrong order, which will show up in MockDComp.
    //

    Canvas^ a;
    Canvas^ d;
    Canvas^ h;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        a = ref new Canvas();
        a->Width = 10;
        a->Height = 10;
        a->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ b = ref new Canvas();
        b->Width = 20;
        b->Height = 20;

        Canvas^ c = ref new Canvas();
        c->Width = 30;
        c->Height = 30;
        c->CompositeMode = ElementCompositeMode::SourceOver;

        d = ref new Canvas();
        d->Width = 40;
        d->Height = 40;
        d->Background = ref new SolidColorBrush(Colors::Red);

        Canvas^ e = ref new Canvas();
        e->Width = 50;
        e->Height = 50;

        Canvas^ f = ref new Canvas();
        f->Width = 60;
        f->Height = 60;
        f->CompositeMode = ElementCompositeMode::SourceOver;

        Canvas^ g = ref new Canvas();
        g->Width = 70;
        g->Height = 70;
        g->Background = ref new SolidColorBrush(Colors::Red);

        h = ref new Canvas();
        h->Width = 80;
        h->Height = 80;
        h->Background = ref new SolidColorBrush(Colors::Red);

        a->Children->Append(b);
        b->Children->Append(c);
        a->Children->Append(d);
        a->Children->Append(e);
        e->Children->Append(f);
        a->Children->Append(g);
        a->Children->Append(h);

        Canvas^ root = ref new Canvas();
        root->Children->Append(a);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving comp node to d and h. The comp node children should be ordered correctly.");
        d->CompositeMode = ElementCompositeMode::SourceOver;
        h->CompositeMode = ElementCompositeMode::SourceOver;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing comp node from a. The WUC visuals should be ordered correctly.");
        a->CompositeMode = ElementCompositeMode::Inherit;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void CompNodeTests::RasterizationScale()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Building tree.");

        xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 50;
        ellipse->Height = 50;
        ellipse->Fill = ref new xaml_media::SolidColorBrush(Colors::Red);

        Canvas^ a = ref new Canvas();
        a->Children->Append(ellipse);

        Canvas^ b = ref new Canvas();
        b->CompositeMode = ElementCompositeMode::SourceOver;
        b->RasterizationScale = 2;
        b->Children->Append(a);

        Canvas^ c = ref new Canvas();
        c->CompositeMode = ElementCompositeMode::SourceOver;
        c->Children->Append(b);

        Canvas^ root = ref new Canvas();
        root->RasterizationScale = 2;
        root->Children->Append(c);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } }
