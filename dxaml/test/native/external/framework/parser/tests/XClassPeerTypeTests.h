// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    // Covers x:Class validation when Application.LoadComponent parses text XAML into an existing
    // root object: x:Class must name the root's runtime type or one of its base types.
    class XClassPeerTypeTests : public WEX::TestClass<XClassPeerTypeTests>
    {
    public:
        BEGIN_TEST_CLASS(XClassPeerTypeTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(LoadComponentWithXClassMatchingRootTypeSucceeds)
            TEST_METHOD_PROPERTY(L"Description", L"x:Class naming the root object's exact runtime type is accepted and the content is loaded.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LoadComponentWithXClassNamingBaseTypeSucceeds)
            TEST_METHOD_PROPERTY(L"Description", L"x:Class naming a base type of the root object is accepted and the content is loaded.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LoadComponentWithUnrelatedXClassFails)
            TEST_METHOD_PROPERTY(L"Description", L"x:Class naming a type the root object does not derive from raises a XAML parse error.")
        END_TEST_METHOD()
    };

} } } } } }
