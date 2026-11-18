// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    // Covers the XAML text parser's node-stream validation: unknown elements, unknown members and
    // content on a type without a content property must fail with a specific, actionable parse error.
    class XamlNodeStreamValidationTests : public WEX::TestClass<XamlNodeStreamValidationTests>
    {
    public:
        BEGIN_TEST_CLASS(XamlNodeStreamValidationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(UnknownElementReportsUnknownType)
            TEST_METHOD_PROPERTY(L"Description", L"An element whose type does not exist in a known namespace fails with 'The type ... was not found'.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnknownElementInUnknownNamespaceReportsUnknownNamespace)
            TEST_METHOD_PROPERTY(L"Description", L"An element in an unresolvable XML namespace fails with an unknown-namespace error naming the namespace.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnknownAttributeReportsUnknownProperty)
            TEST_METHOD_PROPERTY(L"Description", L"An attribute that is not a property of the element's type fails with 'The property ... was not found in type ...'.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnknownAttachedAttributeReportsUnknownAttachableProperty)
            TEST_METHOD_PROPERTY(L"Description", L"A dotted attribute naming a missing attached property fails with the attachable-property error.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ContentOnTypeWithoutContentPropertyReportsCannotAddContent)
            TEST_METHOD_PROPERTY(L"Description", L"Text content after a real attribute on a type with no content property fails with 'Cannot add content to an object of type ...' (without a real property the text is treated as text initialization).")
        END_TEST_METHOD()
    };

} } } } } }
