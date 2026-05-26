// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Native { namespace External { namespace Framework
    {
        class CustomMarkupExtensionTests : public WEX::TestClass<CustomMarkupExtensionTests>
        {
        public:
            BEGIN_TEST_CLASS(CustomMarkupExtensionTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")

            END_TEST_CLASS()

            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)
            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_METHOD(BasicCustomMarkupExtensionFunctionality)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies using a custom markup extension, both with and without "
                    L"the 'Extension' suffix.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCustomMarkupExtensionAsTypeOfProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies using a custom markup extension as the value of a property "
                    L"whose type is the type of the markup extension does not result in a call to ProvideValue().")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNestedCustomMarkupExtension)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that a custom markup extension can provide the value of a "
                    L"property on another custom markup extension.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUseCustomMarkupExtensionInStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies using a custom markup extension, both with and without "
                    L"the 'Extension' suffix, in a Style")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUseCustomMarkupExtensionAsResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that the value provided by a custom markup extension can "
                    L"be a resource in a ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCustomMarkupExtensionThatReturnsBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that a Binding returned by a custom markup extension is "
                    L"correctly hooked up.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIProvideValueTarget)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the IProvideValueTarget implementation given to the custom markup extension")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIXamlTypeResolver)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the IXamlTypeResolver implementation given to the custom markup extension")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIRootObjectProvider)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the IRootObjectProvider implementation given to the custom markup extension")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIUriContext)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the IUriContext implementation given to the custom markup extension")
            END_TEST_METHOD()

        private:
            static void VerifyTypeNamesAreEqual(::Windows::UI::Xaml::Interop::TypeName actual, ::Windows::UI::Xaml::Interop::TypeName expected);
        };
    } } }
} } } }
