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

    class Xbf2MetadataUnitTests
    {
    public:
        BEGIN_TEST_CLASS(Xbf2MetadataUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(Create)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF v2 Metadata Reader can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBasicMetadataLoading)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF v2 Metadata Reader can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyStringTable)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF v2 Metadata Reader can load String Table entries.")
        END_TEST_METHOD()


        BEGIN_TEST_METHOD(VerifyXmlNamespaceTable)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF v2 Metadata Reader can load Xml Namespace Table entries.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTableLoadFailures)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF v2 Metadata Reader fails on simple table load errors.")
        END_TEST_METHOD()
    };

} } } } }
