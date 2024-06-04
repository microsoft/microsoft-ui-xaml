// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlPredicateService.h"

#include "ObjectWriterContext.h"
#include "XamlQualifiedObject.h"
#include "XamlParserContext.h"
#include "MetadataApi.h"
#include "StableXbfIndexes.g.h"

#if XCP_MONITOR
#include "XcpAllocationDebug.h"
#endif

#define CONDITIONAL_PREDICATE_DELIMITER L'?'
#define CONDITIONAL_PREDICATE_ARGS_BEGIN L'('
#define CONDITIONAL_PREDICATE_ARGS_END L')'
#define CONDITIONAL_PREDICATE_ARG_DELIMITER L','

namespace Parser
{

std::vector<xstring_ptr> SplitArgumentsString(const xstring_ptr& args)
{
    std::vector<xstring_ptr> result;

    unsigned int firstIndex = 0;
    unsigned int secondIndex = args.FindChar(CONDITIONAL_PREDICATE_ARG_DELIMITER);

    xstring_ptr arg;
    while (secondIndex != xstring_ptr_view::npos)
    {
        THROW_IF_FAILED(args.SubString(firstIndex, secondIndex, &arg));
        result.emplace_back(arg);

        firstIndex = secondIndex + 1;
        secondIndex = args.FindChar(CONDITIONAL_PREDICATE_ARG_DELIMITER, firstIndex);
    }

    THROW_IF_FAILED(args.SubString(firstIndex, args.GetCount(), &arg));
    result.emplace_back(arg);

    return result;
}

XamlPredicateService& XamlPredicateService::GetInstance()
{
    static INIT_ONCE s_initOnce = INIT_ONCE_STATIC_INIT;
    static XamlPredicateService* s_instance = nullptr;
    InitOnceExecuteOnce(
        &s_initOnce,
        InitOnceCallback,
        &s_instance,
        nullptr);

    return *s_instance;
}

BOOL CALLBACK XamlPredicateService::InitOnceCallback(
    PINIT_ONCE /* unused */,
    PVOID Parameter,
    PVOID * /* unused */)
{
    // This is guaranteed to run exactly once, so we initialize the singleton instance
    // here as a local static, and return its address via the passed in pointer

    static XamlPredicateService instance;
    *(static_cast<XamlPredicateService**>(Parameter)) = &instance;

    return TRUE;
}

XamlPredicateService::XamlPredicateService()
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strIsApiContractNotPresent, L"IsApiContractNotPresent");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strIsApiContractPresent, L"IsApiContractPresent");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strIsPropertyNotPresent, L"IsPropertyNotPresent");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strIsPropertyPresent, L"IsPropertyPresent");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strIsTypeNotPresent, L"IsTypeNotPresent");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strIsTypePresent, L"IsTypePresent");

#if XCP_MONITOR
    auto stopIgnoringOnExit = XcpDebugStartIgnoringLeaks();
#endif
    m_knownXamlPredicates.reserve(6);
    m_knownXamlPredicates.emplace(c_strIsApiContractNotPresent, Parser::StableXbfTypeIndex::IsApiContractNotPresent);
    m_knownXamlPredicates.emplace(c_strIsApiContractPresent, Parser::StableXbfTypeIndex::IsApiContractPresent);
    m_knownXamlPredicates.emplace(c_strIsPropertyNotPresent, Parser::StableXbfTypeIndex::IsPropertyNotPresent);
    m_knownXamlPredicates.emplace(c_strIsPropertyPresent, Parser::StableXbfTypeIndex::IsPropertyPresent);
    m_knownXamlPredicates.emplace(c_strIsTypeNotPresent, Parser::StableXbfTypeIndex::IsTypeNotPresent);
    m_knownXamlPredicates.emplace(c_strIsTypePresent, Parser::StableXbfTypeIndex::IsTypePresent);
}

void XamlPredicateService::CrackConditionalXmlns(
    _In_ const xstring_ptr& xmlns,
    _Out_ xstring_ptr& baseXmlns,
    _Out_ xstring_ptr& conditionalPredicateSubstring)
{
    // The format for a conditional XML namespace string is '<xmlns string>?<conditional predicate>'
    auto delimiterIndex = xmlns.FindChar(CONDITIONAL_PREDICATE_DELIMITER);
    if (delimiterIndex != xstring_ptr_view::npos)
    {
        xmlns.SubString(0, delimiterIndex, &baseXmlns);
        xmlns.SubString(delimiterIndex + 1, xmlns.GetCount(), &conditionalPredicateSubstring);
    }
    else
    {
        baseXmlns = xmlns;
        conditionalPredicateSubstring = xstring_ptr::EmptyString();
    }
}

void XamlPredicateService::CrackConditionalPredicate(
    _In_ const std::shared_ptr<XamlParserContext>& xamlContext,
    _In_ const xstring_ptr& conditionalPredicateSubstring,
    _Out_ std::shared_ptr<XamlType>& predicateType,
    _Out_ xstring_ptr& args)
{
    // The format for a conditional predicate is '<xmlnsprefix>:<predicate name>(<arg1>,<arg2>,...,<argn>)'
    // where the substring '<xmlnsprefix>:<predicate name>' represents a type name

    auto parenIndex = conditionalPredicateSubstring.FindChar(CONDITIONAL_PREDICATE_ARGS_BEGIN);
    if (   parenIndex != xstring_ptr_view::npos 
        && conditionalPredicateSubstring.GetChar(conditionalPredicateSubstring.GetCount() - 1) == CONDITIONAL_PREDICATE_ARGS_END)
    {
        xstring_ptr typeName;
        conditionalPredicateSubstring.SubString(0, parenIndex, &typeName);

        // FUTURE: Because we do not yet support custom XamlPredicate types, the following code
        // (which would enable support for them) is commented out, and instead we'll do a simple
        // string lookup
        //std::shared_ptr<XamlTypeName> xamlTypeName;
        //XamlTypeNameParser typeNameParser(typeName);
        //THROW_IF_FAILED(typeNameParser.ParseXamlTypeName(xamlTypeName));
        //
        //// If the predicate name has an ignored xmlns, then it's an error
        //auto prefix = xamlTypeName->get_Prefix();
        //std::shared_ptr<XamlNamespace> xamlNamespace;
        //THROW_IF_FAILED(xamlContext->FindNamespaceByPrefix(prefix, xamlNamespace));
        //if (!xamlContext->IsNamespaceUriIgnored(xamlNamespace->get_TargetNamespace()))
        //{
        //    THROW_IF_FAILED(xamlNamespace->GetXamlType(xamlTypeName->get_Name(), xamlType));
        //    // Validate the returned type
        //    THROW_HR_IF(E_FAIL, xamlType->IsUnknown());
        //    
        //    predicateType = xamlType;
        //    conditionalPredicateSubstring.SubString(parenIndex + 1, conditionalPredicateSubstring.GetCount() - 1, &args);
        //    
        //    return;
        //}

        std::shared_ptr<XamlSchemaContext> schemaContext;
        THROW_IF_FAILED(xamlContext->get_SchemaContext(schemaContext));
        XamlPredicateService& xpsInstance = GetInstance();
        auto scopeLock = xpsInstance.Lock();// Protect the members
        auto knownPredicate = xpsInstance.m_knownXamlPredicates.find(typeName);
        if (knownPredicate != xpsInstance.m_knownXamlPredicates.end())
        {
            THROW_IF_FAILED(schemaContext->GetXamlTypeFromStableXbfIndex(knownPredicate->second, predicateType));
            conditionalPredicateSubstring.SubString(parenIndex + 1, conditionalPredicateSubstring.GetCount() - 1, &args);

            return;
        }
    }

    THROW_HR(E_FAIL);
}

bool XamlPredicateService::EvaluatePredicate(
    _In_ const std::shared_ptr<XamlType>& predicateType,
    _In_ const xstring_ptr& args)
{
    auto predicateToken = predicateType->get_TypeToken();
    XamlPredicateService& xpsInstance = GetInstance();
    auto scopeLock = xpsInstance.Lock();// Protect the members

    std::shared_ptr<containers::vector_map<xstring_ptr, bool>> tokenToArgsMap;
    auto found = xpsInstance.m_predicateEvaluationCache.find(predicateToken);
    if (found != xpsInstance.m_predicateEvaluationCache.end())
    {
        tokenToArgsMap = found->second;
    }
    else
    {
        tokenToArgsMap = std::make_shared<containers::vector_map<xstring_ptr, bool>>();
        xpsInstance.m_predicateEvaluationCache.emplace(predicateToken, tokenToArgsMap);
    }

    auto cachedResult = tokenToArgsMap->find(args);
    if (cachedResult != tokenToArgsMap->end())
    {
        return cachedResult->second;
    }
    else
    {
        // The string containing the predicate arguments is backed by an xstring_ptr_storage
        // object that is simply pointing to a section of the memory-mapped XBF stream and whose
        // lifetime is tied to the XAML core that loaded the XBF stream (via the per-core 
        // XamlNodeStreamCacheManager instance), but the XamlPredicateService singleton's 
        // lifetime is tied to that of the process. 
        // So if the core that first loaded the XBF goes away, the string stored in the cache
        // is now backed by a nonexistent xstring_ptr_storage. If another core then loads the 
        // same XBF, or even just checks to see if there is a cached evaluation result for the 
        // same predicate type (it doesn't need to be the same set of arguments), then the invalid 
        // string in the cache will get used resulting in an AV.
        // We need to clone the string to avoid the backing xstring_ptr_storage 
        // disappearing from underneath us, and thus ensure that it stays valid for the lifetime
        // of the XamlPredicateService singleton.
        // Note that this scenario does not apply to the XamlReader.Load() code path as it does
        // not use XBF, but the mitigation doesn't distinguish that use case as it is very niche.
        //
        // You'd think we could just use xstring_ptr::Clone(), but because the string is backed 
        // by an xstring_ptr_storage wrapping a memory-mapped file buffer, and thus is neither 
        // an ephemeral (allocated on stack or heap) buffer nor an HSTRING, Clone() simply 
        // shallow copies the xstring_ptr_storage which leaves us no better off than if we had 
        // not called Clone() in the first place.
        xstring_ptr argsClone;
        THROW_IF_FAILED(xstring_ptr::CloneBuffer(args.GetBuffer(), args.GetCount(), &argsClone));

        // Insertion, so need to calculate and cache value
        std::shared_ptr<XamlQualifiedObject> predicateInstance;
        THROW_IF_FAILED(predicateType->CreateInstance(predicateInstance));

        auto pObject = predicateInstance->GetDependencyObject();
        if (pObject)
        {
            ASSERT(   pObject->GetTypeIndex() == KnownTypeIndex::IsApiContractPresent
                   || pObject->GetTypeIndex() == KnownTypeIndex::IsApiContractNotPresent
                   || pObject->GetTypeIndex() == KnownTypeIndex::IsPropertyPresent
                   || pObject->GetTypeIndex() == KnownTypeIndex::IsPropertyNotPresent
                   || pObject->GetTypeIndex() == KnownTypeIndex::IsTypePresent
                   || pObject->GetTypeIndex() == KnownTypeIndex::IsTypeNotPresent);

            auto argsVector = SplitArgumentsString(argsClone);
            // We know the built-in predicates derive from IXamlPredicate
            bool result = static_cast<IXamlPredicate*>(pObject)->Evaluate(argsVector);
            tokenToArgsMap->emplace(argsClone, result);

            return result;
        }
    }

    // If we got here, then there was an error in evaluating the predicate, so throw an exception
    THROW_HR(E_FAIL);
}

#if XCP_MONITOR
void XamlPredicateService::ClearEvaluationCache()
{
    XamlPredicateService& xpsInstance = GetInstance();
    auto scopeLock = xpsInstance.Lock();// Protect the members
    xpsInstance.m_predicateEvaluationCache.clear();
    xpsInstance.m_predicateEvaluationCache.shrink_to_fit();
}
#endif
}