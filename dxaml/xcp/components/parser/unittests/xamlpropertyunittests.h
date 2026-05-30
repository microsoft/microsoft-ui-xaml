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

    class XamlPropertyUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlPropertyUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(Create)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlProperty can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPropertyState)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlProperty's states can be queried.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPropertyNameParser)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlPropertyName can parse a property name.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CreateUnknownProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates unknown XamlProperty can be created.")
        END_TEST_METHOD()

    private:
        std::shared_ptr<XamlSchemaContext> GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter);
    };

} } } } }
