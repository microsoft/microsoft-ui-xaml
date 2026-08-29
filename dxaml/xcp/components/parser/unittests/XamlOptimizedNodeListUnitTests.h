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

    class XamlOptimizedNodeListUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlOptimizedNodeListUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(Create)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlOptimizedNodeList can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetNormalizedCountForOldDictionary)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlOptimizedNodeList returns a normalized element count that is used by old dictionary deferral.")
        END_TEST_METHOD()
    };

} } } } }
