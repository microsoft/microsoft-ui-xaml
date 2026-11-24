// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TreeView {

class TreeViewIntegrationTests : public WEX::TestClass<TreeViewIntegrationTests>
{
public:
    BEGIN_TEST_CLASS(TreeViewIntegrationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(RemoveAtEndRemovesLastChild)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies TreeViewNode.Children.RemoveAtEnd removes the last node.")
    END_TEST_METHOD()
};

} } } } } }
