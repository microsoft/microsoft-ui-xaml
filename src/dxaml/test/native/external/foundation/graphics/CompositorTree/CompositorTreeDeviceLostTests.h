// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RuntimeEnabledFeatureOverride.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class CompositorTreeDeviceLostTests : public WEX::TestClass<CompositorTreeDeviceLostTests>
{


public:
    BEGIN_TEST_CLASS(CompositorTreeDeviceLostTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"9882729e-eea8-4b89-99e7-92145be50e76;bd1463b3-e5f2-4d54-9394-63a431c53a6e;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(AddRemoveSameNode)
    TEST_METHOD_PROPERTY(L"Description", L"Inserts a node and then remove its with a device lost in between")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AddNodeRemoveParent)
    TEST_METHOD_PROPERTY(L"Description", L"Inserts a node and the removes its parent with a device lost in between")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AddNodeRemovePreviousSibling1)
    TEST_METHOD_PROPERTY(L"Description", L"Inserts a node after the first child of the parent and then removes the first child after with a device lost in between")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AddNodeRemovePreviousSibling2)
    TEST_METHOD_PROPERTY(L"Description", L"Inserts a node after the second child of the parent and then removes the second child after with a device lost in between")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MoveThenRemoveNode)
    TEST_METHOD_PROPERTY(L"Description", L"Move a node and then removes it with a device lost in between")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AddTwoNodesThenRemoveFirst)
    TEST_METHOD_PROPERTY(L"Description", L"Adds two consecutive nodes and then removes the first one with a device lost in between")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderingNewCompNodeThatSplitsContainer)
        TEST_METHOD_PROPERTY(L"Description", L"Device lost when rendering a new comp node, which splits a container visual in the previous frame.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

private:

    typedef void(*RENDERACTION)(Microsoft::UI::Xaml::Controls::Panel^ rootPanel);

    static Microsoft::UI::Xaml::Controls::Border^ AddBorderChild(Microsoft::UI::Xaml::Controls::Border^ border);
    static Microsoft::UI::Xaml::Controls::Border^ CreateBorderElement(int style);

    static void SubmitRenderActions(RENDERACTION actions[], int actionCount, Microsoft::UI::Xaml::Controls::Panel^ panel);

    static Microsoft::UI::Xaml::Media::Imaging::WriteableBitmap^ CreateWriteableBitmap();

};

} } } } } }

