// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minxcptypes.h>
#include <minerror.h>
#include <palcore.h>
#include <IParserCoreServices.h>
#include <WexTestClass.h>

class XamlSchemaContext;
struct IErrorService;
class XamlNodeStreamCacheManager;
class ParserErrorReporter;
class XamlTextReader;
class XamlNode;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XamlTextParserUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlTextParserUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(Create)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlTextParser can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParseXStringInXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates parsing of x:String elements.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParseNamedXStringInXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates directives and text initialization on named strings.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParseEmptyNamedXStringInXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates directives on empty named strings.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeNamedObjectBeforeConditionalProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Preserves unconditional names when the first property is conditional.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeDeferredTextBlockLanguage)
            TEST_METHOD_PROPERTY(L"Description", L"Initializes an unnamed TextBlock before assigning saved language at end-of-object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeSavedDirectivesOnContainer)
            TEST_METHOD_PROPERTY(L"Description", L"Preserves object-valued children when replaying saved names and language.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeNamedStringConnectsInitializedValue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that named strings are initialized before connecting.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeNamedStringConnectsConvertedId)
            TEST_METHOD_PROPERTY(L"Description", L"Validates initialization with an already converted connection ID.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeNamedStringBeforeConditionalProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Keeps string initialization, names, and connections outside conditional members.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeConditionalNamedStringBeforeConditionalProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Preserves the object's condition when moving string initialization outside a member condition.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EncodeUnknownConditionalTypeWithConnectionId)
            TEST_METHOD_PROPERTY(L"Description", L"Preserves encoding of unresolved conditional types with connection IDs.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParseIgnorableElements)
            TEST_METHOD_PROPERTY(L"Description", L"Validates parsing of ignorable elements.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyParseFailures)
            TEST_METHOD_PROPERTY(L"Description", L"Validates parsing failures.")
        END_TEST_METHOD()

    private:
        void VerifyStringConnection(bool numericConnection, bool conditionalMember = false, bool conditionalObject = false);
    };

} } } } }
