// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlPredicateServiceUnitTests.h"
#include "XamlPredicateService.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include "XamlParserContext.h"
#include "ParserUtilities.h"
#include "StableXbfIndexes.g.h"

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

void XamlPredicateServiceUnitTests::VerifyCrackConditionalXmlns()
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(conditionalXmlns, L"http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(regularXmlns, L"http://schemas.microsoft.com/winfx/2006/xaml/presentation");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(conditionalXmlnsWithIncorrectDelimiter, L"http://schemas.microsoft.com/winfx/2006/xaml/presentation;IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(conditionalUsingXmlns, L"using:MyNamespace?IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)");
    
    xstring_ptr baseXmlns, conditionalPredicateString;

    LOG_OUTPUT(L"Verifying cracking of a conditional xmlns");
    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalXmlns(conditionalXmlns, baseXmlns, conditionalPredicateString));
    VERIFY_IS_TRUE(baseXmlns.Equals(L"http://schemas.microsoft.com/winfx/2006/xaml/presentation"));
    VERIFY_IS_TRUE(conditionalPredicateString.Equals(L"IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)"));

    LOG_OUTPUT(L"Verifying cracking of a condition-less xmlns");
    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalXmlns(regularXmlns, baseXmlns, conditionalPredicateString));
    VERIFY_IS_TRUE(baseXmlns.Equals(L"http://schemas.microsoft.com/winfx/2006/xaml/presentation"));
    VERIFY_IS_TRUE(conditionalPredicateString.Equals(xstring_ptr::EmptyString()));

    LOG_OUTPUT(L"Verifying cracking of a conditional xmlns with the wrong delimiter");
    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalXmlns(conditionalXmlnsWithIncorrectDelimiter, baseXmlns, conditionalPredicateString));
    VERIFY_IS_TRUE(baseXmlns.Equals(L"http://schemas.microsoft.com/winfx/2006/xaml/presentation;IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)"));
    VERIFY_IS_TRUE(conditionalPredicateString.Equals(xstring_ptr::EmptyString()));

    LOG_OUTPUT(L"Verifying cracking of a conditional 'using:' xmlns");
    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalXmlns(conditionalUsingXmlns, baseXmlns, conditionalPredicateString));
    VERIFY_IS_TRUE(baseXmlns.Equals(L"using:MyNamespace"));
    VERIFY_IS_TRUE(conditionalPredicateString.Equals(L"IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)"));
}

void XamlPredicateServiceUnitTests::VerifyCrackConditionalPredicate()
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strIsApiContractNotPresent, L"IsApiContractNotPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strIsApiContractPresent, L"IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strIsPropertyNotPresent, L"IsPropertyNotPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strIsPropertyPresent, L"IsPropertyPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strIsTypeNotPresent, L"IsTypeNotPresent(Windows.Foundation.UniversalApiContract,5)");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strIsTypePresent, L"IsTypePresent(Windows.Foundation.UniversalApiContract,5)");
    
    ParserUtilities parserUtils;
    auto schemaContext = parserUtils.GetSchemaContext(std::shared_ptr<ParserErrorReporter>());
    std::shared_ptr<XamlParserContext> parserContext;
    VERIFY_SUCCEEDED(XamlParserContext::Create(schemaContext, parserContext));

    LOG_OUTPUT(L"Verifying that we can crack all known XamlPredicate types");

    std::shared_ptr<XamlType> predicateType;
    xstring_ptr predicateArgs;

    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalPredicate(parserContext, strIsApiContractNotPresent, predicateType, predicateArgs));
    VERIFY_ARE_EQUAL(KnownTypeIndex::IsApiContractNotPresent, predicateType->get_TypeToken().GetHandle());
    VERIFY_IS_TRUE(predicateArgs.Equals(L"Windows.Foundation.UniversalApiContract,5"));

    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalPredicate(parserContext, strIsApiContractPresent, predicateType, predicateArgs));
    VERIFY_ARE_EQUAL(KnownTypeIndex::IsApiContractPresent, predicateType->get_TypeToken().GetHandle());
    VERIFY_IS_TRUE(predicateArgs.Equals(L"Windows.Foundation.UniversalApiContract,5"));

    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalPredicate(parserContext, strIsPropertyNotPresent, predicateType, predicateArgs));
    VERIFY_ARE_EQUAL(KnownTypeIndex::IsPropertyNotPresent, predicateType->get_TypeToken().GetHandle());
    VERIFY_IS_TRUE(predicateArgs.Equals(L"Windows.Foundation.UniversalApiContract,5"));

    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalPredicate(parserContext, strIsPropertyPresent, predicateType, predicateArgs));
    VERIFY_ARE_EQUAL(KnownTypeIndex::IsPropertyPresent, predicateType->get_TypeToken().GetHandle());
    VERIFY_IS_TRUE(predicateArgs.Equals(L"Windows.Foundation.UniversalApiContract,5"));

    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalPredicate(parserContext, strIsTypeNotPresent, predicateType, predicateArgs));
    VERIFY_ARE_EQUAL(KnownTypeIndex::IsTypeNotPresent, predicateType->get_TypeToken().GetHandle());
    VERIFY_IS_TRUE(predicateArgs.Equals(L"Windows.Foundation.UniversalApiContract,5"));

    VERIFY_NO_THROW(::Parser::XamlPredicateService::CrackConditionalPredicate(parserContext, strIsTypePresent, predicateType, predicateArgs));
    VERIFY_ARE_EQUAL(KnownTypeIndex::IsTypePresent, predicateType->get_TypeToken().GetHandle());
    VERIFY_IS_TRUE(predicateArgs.Equals(L"Windows.Foundation.UniversalApiContract,5"));
}

} } } } }