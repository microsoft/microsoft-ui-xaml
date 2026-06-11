// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class RenderWalkTests : public WEX::TestClass<RenderWalkTests>
{
public:
    BEGIN_TEST_CLASS(RenderWalkTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Tests the incremental render walk.")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(AddCompNode)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCompNode_Property)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCompNode_Property_GroupMerge)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCompNode_RemoveElements)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AddCompNodeWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCompNode_PropertyWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCompNode_Property_GroupMergeWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RemoveCompNode_RemoveElementsWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    void AddCompNodeInternal();

    void RemoveCompNode_PropertyInternal();

    void RemoveCompNode_Property_GroupMergeInternal();

    void RemoveCompNode_RemoveElementsInternal();

    inline Platform::String^ GetResourcesPath() const;

    xaml_controls::Canvas^ ResetTree();
    xaml_controls::Canvas^ ResetTreeForMerge();
    xaml_controls::Canvas^ ResetTreeForRemove();

    void AddCompNode(xaml_controls::Canvas^ root, Platform::String^ elementName);
    void RemoveCompNode(xaml_controls::Canvas^ root, Platform::String^ elementName);
    // We might be removing outside the tree, so we can't go look for the element in the tree.
    void RemoveElement(FrameworkElement^ element);

    void SplitOne(Platform::String^ element1);
    void SplitTwo(Platform::String^ element1, Platform::String^ element2);
    void SplitOneThenTheOther(Platform::String^ element1, Platform::String^ element2);

    void MergeOne(Platform::String^ element1);
    void MergeTwo(Platform::String^ element1, Platform::String^ element2);
    void MergeOneThenTheOther(Platform::String^ element1, Platform::String^ element2);

    void RemoveOne(Platform::String^ element1);
    void RemoveTwo(Platform::String^ element1, Platform::String^ element2);
    void RemoveOneThenTheOther(Platform::String^ element1, Platform::String^ element2);
};

} } } } } }

