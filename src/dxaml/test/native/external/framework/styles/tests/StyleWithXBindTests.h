// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Styles {

    class StyleWithXBindTests
    {

    public:
        StyleWithXBindTests();

        BEGIN_TEST_CLASS(StyleWithXBindTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

#pragma region Simulated x:Bind
        // These tests employ the moral equivalent of x:Bind, by placing an x:ConnectionId and x:Name on the "x:Bound"
        // Setters, thus presenting them to the parser the same way XamlCompiler would, as well as allowing them to be
        // manipulated from code-behind

        BEGIN_TEST_METHOD(BasicXBind_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that changes to mutable Setter.Value are reflected by controls with applied Style")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BasicXBindWithFaultIn_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that changes to mutable Setter.Value are reflected by controls with applied Style even when Style is faulted in")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XBindInBaseStyle_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that changes to mutable Setter.Value in a base Style are reflected in controls using a derived Style")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XBindInBaseStyleWithFaultIn_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that changes to mutable Setter.Value in a base Style are reflected in controls using a derived Style even when faulted in")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeAppliedStyle)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that x:Bind in Styles works when the applied Style is changed")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XBindInImplicitStyle_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that changes to mutable Setter.Value are reflected by controls with applied implicit Style")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XBindInStyleOverriddenByLocalValue_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that changes to mutable Setter.Value don't affect effective value if there is a local value override")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XBindInStyleMultipleProperties_Simulated)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that multiple Setters with x:Bind that have the same Property work as expected (last Setter wins)")
        END_TEST_METHOD()

#pragma endregion

    private:
        void XBindInBaseStyle_Simulated_Helper(bool faultIn);
        void BasicXBind_Simulated_Helper(bool faultIn);
    };
} } } } } }
