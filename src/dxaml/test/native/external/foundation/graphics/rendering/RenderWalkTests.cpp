// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RenderWalkTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ RenderWalkTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool RenderWalkTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool RenderWalkTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool RenderWalkTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void RenderWalkTests::AddCompNodeInternal()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    LOG_OUTPUT(L"(Split primitive group at the head)");

    SplitOne(L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1");

    SplitTwo(L"c1", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c11");

    SplitOneThenTheOther(L"c1", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c11");

    SplitOneThenTheOther(L"c1_1", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c11");

    SplitTwo(L"c1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    SplitOneThenTheOther(L"c1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    SplitOneThenTheOther(L"c1_2", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    SplitTwo(L"c1", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c13");

    SplitOneThenTheOther(L"c1", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c13");

    SplitOneThenTheOther(L"c1_3", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c13");

    LOG_OUTPUT(L"(Split primitive group in the middle)");

    SplitOne(L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2");

    SplitTwo(L"c2", L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c21");

    SplitOneThenTheOther(L"c2", L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c21");

    SplitOneThenTheOther(L"c2_1", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c21");

    SplitTwo(L"c2", L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c22");

    SplitOneThenTheOther(L"c2", L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c22");

    SplitOneThenTheOther(L"c2_2", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c22");

    SplitTwo(L"c2", L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c23");

    SplitOneThenTheOther(L"c2", L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c23");

    SplitOneThenTheOther(L"c2_3", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c23");

    LOG_OUTPUT(L"(Split primitive group at the tail)");

    SplitOne(L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3");

    SplitTwo(L"c3", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c31");

    SplitOneThenTheOther(L"c3", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c31");

    SplitOneThenTheOther(L"c3_1", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c31");

    SplitTwo(L"c3", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c32");

    SplitOneThenTheOther(L"c3", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c32");

    SplitOneThenTheOther(L"c3_2", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c32");

    SplitTwo(L"c3", L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    SplitOneThenTheOther(L"c3", L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    SplitOneThenTheOther(L"c3_3", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    LOG_OUTPUT(L"(Split separate primitive groups)");

    SplitTwo(L"c1", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c2");

    SplitOneThenTheOther(L"c1", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c2");

    SplitOneThenTheOther(L"c2", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c2");

    SplitTwo(L"c2_1", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21_c32");

    SplitOneThenTheOther(L"c2_1", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21_c32");

    SplitOneThenTheOther(L"c3_2", L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21_c32");

    SplitTwo(L"c3", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c13");

    SplitOneThenTheOther(L"c3", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c13");

    SplitOneThenTheOther(L"c1_3", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c13");
}

Canvas^ RenderWalkTests::ResetTree()
{
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    return rootCanvas;
}

void RenderWalkTests::AddCompNode(Canvas^ root, Platform::String^ elementName)
{
    UIElement^ element = safe_cast<UIElement^>(root->FindName(elementName));
    element->CompositeMode = ElementCompositeMode::SourceOver;
}

void RenderWalkTests::RemoveCompNode(Canvas^ root, Platform::String^ elementName)
{
    UIElement^ element = safe_cast<UIElement^>(root->FindName(elementName));
    element->CompositeMode = ElementCompositeMode::Inherit;
}

void RenderWalkTests::RemoveElement(FrameworkElement^ element)
{
    Panel^ parent = safe_cast<Panel^>(element->Parent);
    if (parent) // If element isn't in the tree, there will be no parent
    {
        unsigned int index = 0;
        parent->Children->IndexOf(element, &index);
        parent->Children->RemoveAt(index);
    }
}

void RenderWalkTests::SplitOne(Platform::String^ element1)
{
    LOG_OUTPUT(L"Split %s", element1->Data());
    Canvas^ rootCanvas = ResetTree();
    RunOnUIThread([&]() { AddCompNode(rootCanvas, element1); });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::SplitTwo(Platform::String^ element1, Platform::String^ element2)
{
    LOG_OUTPUT(L"Split %s + %s", element1->Data(), element2->Data());
    Canvas^ rootCanvas = ResetTree();
    RunOnUIThread([&]() { AddCompNode(rootCanvas, element1); AddCompNode(rootCanvas, element2); });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::SplitOneThenTheOther(Platform::String^ element1, Platform::String^ element2)
{
    LOG_OUTPUT(L"Split %s, then split %s ", element1->Data(), element2->Data());
    Canvas^ rootCanvas = ResetTree();
    RunOnUIThread([&]() { AddCompNode(rootCanvas, element1); });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]() { AddCompNode(rootCanvas, element2); });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

Canvas^ RenderWalkTests::ResetTreeForMerge()
{
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        AddCompNode(rootCanvas, L"c1");
        AddCompNode(rootCanvas, L"c1_1");
        AddCompNode(rootCanvas, L"c1_2");
        AddCompNode(rootCanvas, L"c1_3");
        AddCompNode(rootCanvas, L"c2");
        AddCompNode(rootCanvas, L"c2_1");
        AddCompNode(rootCanvas, L"c2_2");
        AddCompNode(rootCanvas, L"c2_3");
        AddCompNode(rootCanvas, L"c3");
        AddCompNode(rootCanvas, L"c3_1");
        AddCompNode(rootCanvas, L"c3_2");
        AddCompNode(rootCanvas, L"c3_3");
    });
    TestServices::WindowHelper->WaitForIdle();

    return rootCanvas;
}

void RenderWalkTests::RemoveCompNode_PropertyInternal()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    LOG_OUTPUT(L"(Merge leaf primitives)");

    MergeOne(L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11");

    MergeOne(L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c12");

    MergeOne(L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c13");

    MergeOne(L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21");

    MergeOne(L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c22");

    MergeOne(L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c23");

    MergeOne(L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c31");

    MergeOne(L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c32");

    MergeOne(L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c33");

    MergeTwo(L"c1_1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c12");

    MergeOneThenTheOther(L"c1_1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c12");

    MergeOneThenTheOther(L"c1_2", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c12");

    MergeTwo(L"c1_1", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c13");

    MergeOneThenTheOther(L"c1_1", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c13");

    MergeOneThenTheOther(L"c1_3", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c13");

    MergeTwo(L"c2_2", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c22_c31");

    MergeOneThenTheOther(L"c2_2", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c22_c31");

    MergeOneThenTheOther(L"c3_1", L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c22_c31");

    LOG_OUTPUT(L"(Merge trunk primitives)");

    MergeOne(L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1");

    MergeTwo(L"c1", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c11");

    MergeOneThenTheOther(L"c1", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c11");

    MergeOneThenTheOther(L"c1_1", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c11");

    MergeTwo(L"c1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    MergeOneThenTheOther(L"c1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    MergeOneThenTheOther(L"c1_2", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    MergeTwo(L"c1", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c13");

    MergeOneThenTheOther(L"c1", L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c13");

    MergeOneThenTheOther(L"c1_3", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c13");

    MergeOne(L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2");

    MergeTwo(L"c2", L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c21");

    MergeOneThenTheOther(L"c2", L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c21");

    MergeOneThenTheOther(L"c2_1", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c21");

    MergeTwo(L"c2", L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c22");

    MergeOneThenTheOther(L"c2", L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c22");

    MergeOneThenTheOther(L"c2_2", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c22");

    MergeTwo(L"c2", L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c23");

    MergeOneThenTheOther(L"c2", L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c23");

    MergeOneThenTheOther(L"c2_3", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c23");

    MergeOne(L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3");

    MergeTwo(L"c3", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c31");

    MergeOneThenTheOther(L"c3", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c31");

    MergeOneThenTheOther(L"c3_1", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c31");

    MergeTwo(L"c3", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c32");

    MergeOneThenTheOther(L"c3", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c32");

    MergeOneThenTheOther(L"c3_2", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c32");

    MergeTwo(L"c3", L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    MergeOneThenTheOther(L"c3", L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    MergeOneThenTheOther(L"c3_3", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");
}

void RenderWalkTests::MergeOne(Platform::String^ element1)
{
    LOG_OUTPUT(L"Merge %s", element1->Data());
    Canvas^ rootCanvas = ResetTreeForMerge();
    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, element1); });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::MergeTwo(Platform::String^ element1, Platform::String^ element2)
{
    LOG_OUTPUT(L"Merge %s + %s", element1->Data(), element2->Data());
    Canvas^ rootCanvas = ResetTreeForMerge();
    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, element1); RemoveCompNode(rootCanvas, element2); });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::MergeOneThenTheOther(Platform::String^ element1, Platform::String^ element2)
{
    LOG_OUTPUT(L"Merge %s, then merge %s ", element1->Data(), element2->Data());
    Canvas^ rootCanvas = ResetTreeForMerge();
    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, element1); });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, element2); });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::RemoveCompNode_Property_GroupMergeInternal()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ rootCanvas;

    LOG_OUTPUT(L"Merge into existing primitive groups - head, middle, tail");

    rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        AddCompNode(rootCanvas, L"c1");
        AddCompNode(rootCanvas, L"c1_1");
        AddCompNode(rootCanvas, L"c2");
        AddCompNode(rootCanvas, L"c2_2");
        AddCompNode(rootCanvas, L"c3");
        AddCompNode(rootCanvas, L"c3_3");
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        RemoveCompNode(rootCanvas, "c1_1");
        RemoveCompNode(rootCanvas, "c2_2");
        RemoveCompNode(rootCanvas, "c3_3");
    });
    TestServices::WindowHelper->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"LeavesMerged");

    LOG_OUTPUT(L"Merge into existing primitive groups - tail, middle, head");

    rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        AddCompNode(rootCanvas, L"c1");
        AddCompNode(rootCanvas, L"c1_3");
        AddCompNode(rootCanvas, L"c2");
        AddCompNode(rootCanvas, L"c2_2");
        AddCompNode(rootCanvas, L"c3");
        AddCompNode(rootCanvas, L"c3_1");
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, "c1_3"); });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, "c2_2"); });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]() { RemoveCompNode(rootCanvas, "c3_1"); });
    TestServices::WindowHelper->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"LeavesMerged");

    LOG_OUTPUT(L"Merge trunks");

    rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        AddCompNode(rootCanvas, L"c1");
        AddCompNode(rootCanvas, L"c1_1");
        AddCompNode(rootCanvas, L"c2");
        AddCompNode(rootCanvas, L"c2_2");
        AddCompNode(rootCanvas, L"c3");
        AddCompNode(rootCanvas, L"c3_3");
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        RemoveCompNode(rootCanvas, "c1");
        RemoveCompNode(rootCanvas, "c2");
        RemoveCompNode(rootCanvas, "c3");
    });
    TestServices::WindowHelper->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"TrunksMerged");

    LOG_OUTPUT(L"Merge everything");

    rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        AddCompNode(rootCanvas, L"c1");
        AddCompNode(rootCanvas, L"c1_3");
        AddCompNode(rootCanvas, L"c2");
        AddCompNode(rootCanvas, L"c2_2");
        AddCompNode(rootCanvas, L"c3");
        AddCompNode(rootCanvas, L"c3_1");
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        RemoveCompNode(rootCanvas, "c1");
        RemoveCompNode(rootCanvas, "c1_3");
        RemoveCompNode(rootCanvas, "c3_1");
        RemoveCompNode(rootCanvas, "c2_2");
        RemoveCompNode(rootCanvas, "c2");
        RemoveCompNode(rootCanvas, "c3");
    });
    TestServices::WindowHelper->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EverythingMerged");
}

Canvas^ RenderWalkTests::ResetTreeForRemove()
{
    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RenderWalk_Tree.xaml"));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
        AddCompNode(rootCanvas, L"c1");
        AddCompNode(rootCanvas, L"c1_1");
        AddCompNode(rootCanvas, L"c2");
        AddCompNode(rootCanvas, L"c2_2");
        AddCompNode(rootCanvas, L"c3");
        AddCompNode(rootCanvas, L"c3_3");
    });
    TestServices::WindowHelper->WaitForIdle();

    return rootCanvas;
}

void RenderWalkTests::RemoveOne(Platform::String^ element1)
{
    LOG_OUTPUT(L"Remove %s", element1->Data());
    Canvas^ rootCanvas = ResetTreeForRemove();
    RunOnUIThread([&]()
    {
        RemoveElement(safe_cast<FrameworkElement^>(rootCanvas->FindName(element1)));
    });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::RemoveTwo(Platform::String^ element1, Platform::String^ element2)
{
    LOG_OUTPUT(L"Remove %s + %s", element1->Data(), element2->Data());
    Canvas^ rootCanvas = ResetTreeForRemove();
    RunOnUIThread([&]()
    {
        FrameworkElement^ e1 = safe_cast<FrameworkElement^>(rootCanvas->FindName(element1));
        FrameworkElement^ e2 = safe_cast<FrameworkElement^>(rootCanvas->FindName(element2));
        RemoveElement(e1);
        RemoveElement(e2);
    });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::RemoveOneThenTheOther(Platform::String^ element1, Platform::String^ element2)
{
    LOG_OUTPUT(L"Remove %s, then remove %s ", element1->Data(), element2->Data());
    Canvas^ rootCanvas = ResetTreeForRemove();
    FrameworkElement^ e1;
    FrameworkElement^ e2;
    RunOnUIThread([&]()
    {
        e1 = safe_cast<FrameworkElement^>(rootCanvas->FindName(element1));
        e2 = safe_cast<FrameworkElement^>(rootCanvas->FindName(element2));
        RemoveElement(e1);
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        RemoveElement(e2);
    });
    TestServices::WindowHelper->WaitForIdle();
    // Don't verify MockDComp. Let the caller do it. This way if the verification fails, we get a unique line number
    // for the failure, rather than this line for every test case that calls this helper.
}

void RenderWalkTests::RemoveCompNode_RemoveElementsInternal()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    LOG_OUTPUT(L"(Remove leaf elements)");

    RemoveOne(L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11");

    RemoveOne(L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c12");

    RemoveOne(L"c1_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c13");

    RemoveOne(L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21");

    RemoveOne(L"c2_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c22");

    RemoveOne(L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c23");

    RemoveOne(L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c31");

    RemoveOne(L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c32");

    RemoveOne(L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c33");

    LOG_OUTPUT(L"(Remove trunk elements)");

    RemoveOne(L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1");

    RemoveOne(L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2");

    RemoveOne(L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3");

    LOG_OUTPUT(L"(Remove multiple elements)");

    RemoveTwo(L"c1_1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c12");

    RemoveOneThenTheOther(L"c1_1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c12");

    RemoveOneThenTheOther(L"c1_2", L"c1_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c11_c12");

    RemoveTwo(L"c2_1", L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21_c23");

    RemoveOneThenTheOther(L"c2_1", L"c2_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21_c23");

    RemoveOneThenTheOther(L"c2_3", L"c2_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c21_c23");

    RemoveTwo(L"c3_1", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c31_c32");

    RemoveOneThenTheOther(L"c3_1", L"c3_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c31_c32");

    RemoveOneThenTheOther(L"c3_2", L"c3_1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c31_c32");

    RemoveTwo(L"c1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    RemoveOneThenTheOther(L"c1", L"c1_2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    RemoveOneThenTheOther(L"c1_2", L"c1");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c1_c12");

    RemoveTwo(L"c3", L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    RemoveOneThenTheOther(L"c3", L"c3_3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    RemoveOneThenTheOther(L"c3_3", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c3_c33");

    RemoveTwo(L"c2", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c3");

    RemoveOneThenTheOther(L"c2", L"c3");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c3");

    RemoveOneThenTheOther(L"c3", L"c2");
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"c2_c3");
}

void RenderWalkTests::AddCompNode()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    AddCompNodeInternal();
}

void RenderWalkTests::RemoveCompNode_Property()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RemoveCompNode_PropertyInternal();
}

void RenderWalkTests::RemoveCompNode_Property_GroupMerge()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RemoveCompNode_Property_GroupMergeInternal();
}

void RenderWalkTests::RemoveCompNode_RemoveElements()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RemoveCompNode_RemoveElementsInternal();
}

void RenderWalkTests::AddCompNodeWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    AddCompNodeInternal();
}

void RenderWalkTests::RemoveCompNode_PropertyWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RemoveCompNode_PropertyInternal();
}

void RenderWalkTests::RemoveCompNode_Property_GroupMergeWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RemoveCompNode_Property_GroupMergeInternal();
}

void RenderWalkTests::RemoveCompNode_RemoveElementsWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RemoveCompNode_RemoveElementsInternal();
}

} } } } } }
