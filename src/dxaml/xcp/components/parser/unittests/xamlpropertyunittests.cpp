// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlPropertyUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <XamlPropertyName.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    void XamlPropertyUnitTests::Create()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto schemaContext = GetSchemaContext(spNullErrorReporter);
        auto xamlProperty = std::make_shared<XamlProperty>(schemaContext, XamlPropertyToken(tpkNative, KnownPropertyIndex::Grid_Row));

        VERIFY_IS_NOT_NULL(xamlProperty);
    }

    void XamlPropertyUnitTests::VerifyPropertyState()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto schemaContext = GetSchemaContext(spNullErrorReporter);
        auto xamlProperty = std::make_shared<XamlProperty>(schemaContext, XamlPropertyToken(tpkNative, KnownPropertyIndex::Grid_Row));

        VERIFY_IS_FALSE(xamlProperty->IsDirective());
        VERIFY_IS_FALSE(xamlProperty->IsImplicit());
    }

    void XamlPropertyUnitTests::VerifyPropertyNameParser()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strUnknownProperty, L"MyUnknownProperty");
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto schemaContext = GetSchemaContext(spNullErrorReporter);
        std::shared_ptr<XamlPropertyName> propertyName;
        std::shared_ptr<XamlNamespace> xamlNamespace;
        std::shared_ptr<XamlProperty> xamlProperty;

        VERIFY_SUCCEEDED(XamlPropertyName::Parse(xstring_ptr::NullString(), c_strUnknownProperty, propertyName));
    }

    void XamlPropertyUnitTests::CreateUnknownProperty()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strUnknownProperty, L"MyUnknownProperty");
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto schemaContext = GetSchemaContext(spNullErrorReporter);
        std::shared_ptr<UnknownProperty> spUnknownProperty;
        std::shared_ptr<XamlNamespace> xamlNamespace;

        VERIFY_SUCCEEDED(UnknownProperty::Create(schemaContext, c_strUnknownProperty, std::make_shared<XamlType>(), FALSE, spUnknownProperty));
        VERIFY_IS_NOT_NULL(spUnknownProperty);

        VERIFY_SUCCEEDED(UnknownProperty::Create(schemaContext, c_strUnknownProperty, std::make_shared<XamlType>(), xamlNamespace, FALSE, spUnknownProperty));
        VERIFY_IS_NOT_NULL(spUnknownProperty);
    }

    std::shared_ptr<XamlSchemaContext> XamlPropertyUnitTests::GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter)
    {
        ParserUtilities parserUtils;
        return parserUtils.GetSchemaContext(customErrorReporter);
    }

} } } } }
