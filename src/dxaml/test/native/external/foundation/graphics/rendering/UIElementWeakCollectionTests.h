// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class UIElementWeakCollectionTests : public WEX::TestClass<UIElementWeakCollectionTests>
{
public:
    BEGIN_TEST_CLASS(UIElementWeakCollectionTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    TEST_METHOD(ItemAccess)

    TEST_METHOD(WeakRef)

    TEST_METHOD(NonDependencyObject)
};

} } } } } }
