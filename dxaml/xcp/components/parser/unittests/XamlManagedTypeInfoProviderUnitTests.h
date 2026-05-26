// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minxcptypes.h>
#include <minerror.h>
#include <palcore.h>
#include <IParserCoreServices.h>
#include <WexTestClass.h>

class XamlSchemaContext;
class XamlTypeInfoProvider;
struct IErrorService;
class XamlNodeStreamCacheManager;
class ParserErrorReporter;
class XamlTextReader;
class XamlNode;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XamlManagedTypeInfoProviderUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlManagedTypeInfoProviderUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(Create)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XamlManagedTypeInfoProvider can be created.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResolveAssembly)
            TEST_METHOD_PROPERTY(L"Description", L"Validates resolution of custom assemblies.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetTypeNamespace)
            TEST_METHOD_PROPERTY(L"Description", L"Validates parsing a typenamespace and returning the token for it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetTypeNamespaceForType)
            TEST_METHOD_PROPERTY(L"Description", L"Validates resolving a type namespace token from a given type")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LookupTypeFlags)
            TEST_METHOD_PROPERTY(L"Description", L"Validates type flags can be retrieved.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LookupPropertyFlags)
            TEST_METHOD_PROPERTY(L"Description", L"Validates property flags can be retrieved.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResolveTypeName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates type name can be resolved.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResolvePropertyName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates property name can be resolved.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResolveDependencyPropertyName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates dependency property name can be resolved.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ProviderMetadataQueryTests)
            TEST_METHOD_PROPERTY(L"Description", L"Validates various metadata information from the provider can be queried.")
        END_TEST_METHOD()

    private:
        std::shared_ptr<XamlSchemaContext> GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter);
        std::shared_ptr<XamlTypeInfoProvider> GetManagedTypeInfoProvider(const std::shared_ptr<XamlSchemaContext>& schemaContext);
    };

} } } } }
