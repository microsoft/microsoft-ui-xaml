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

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XamlSchemaContextUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlSchemaContextUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(Create)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SpecialNamespacesInitialized)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext initialized the special xml namespaces.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SourceAssemblies)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can work with source assembly contexts.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlAsssemblyInSchema)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can add and locate XamlAssembly in its schema.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CrackXmlns)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can crack open the xml namespace string.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlNamespacesInSchema)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can add and locate XamlNamespaces in its schema.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlTypeNamespacesInSchema)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can add and locate XamlTypeNamespaces in its schema.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlTypesInSchema)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can add and locate XamlTypes in its schema.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlPropertiesInSchema)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can add and locate XamlProperties in its schema.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlTypeInfoProviderInSchema)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can retrieve the correct type info provider.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CachedTypesAndProperties)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext can retrieve cached well known types and properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetErrorService)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext's error service can be modified.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetXamlTextSyntax)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlSchemaContext's XamlTextSyntax can be retrieved.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(XamlTokenToProviderMapping)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates tokens map to the right provider.")
        END_TEST_METHOD()

        private:
        std::shared_ptr<XamlSchemaContext> GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter);
    };

} } } } }

