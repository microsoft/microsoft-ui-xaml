// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    // Covers the XAML collection initialization-string syntax: a string attribute assigned to a
    // collection property that has no type converter of its own (TransformGroup.Children) is split by
    // the parser into entries, each converted with the item type's text syntax (Transform from a matrix string).
    class CollectionInitializationStringIntegrationTests : public WEX::TestClass<CollectionInitializationStringIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CollectionInitializationStringIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ItemsInitializationStringCreatesItems)
            TEST_METHOD_PROPERTY(L"Description", L"A comma-separated Children attribute creates one MatrixTransform per entry, in order.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsInitializationStringHonorsQuotesAndEscapes)
            TEST_METHOD_PROPERTY(L"Description", L"Quoted literals keep commas as one entry; escaped commas and ordinary characters are unescaped; escaped brackets, quotes and backslashes are accepted without splitting or ending an entry.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsInitializationStringTrimsUnquotedWhitespace)
            TEST_METHOD_PROPERTY(L"Description", L"Whitespace around unquoted entries and around quoted literals is ignored.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsInitializationStringWithInvalidSyntaxFailsToParse)
            TEST_METHOD_PROPERTY(L"Description", L"Malformed initialization strings raise a XAML parse error and create no items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsInitializationStringWithBlankEntryFailsConversion)
            TEST_METHOD_PROPERTY(L"Description", L"An entry that is empty after trimming is rejected with a type conversion error.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsInitializationStringEmptyValueFailsToParse)
            TEST_METHOD_PROPERTY(L"Description", L"An empty Children attribute is not a valid initialization string and raises a XAML parse error.")
        END_TEST_METHOD()
    };

} } } } } }
