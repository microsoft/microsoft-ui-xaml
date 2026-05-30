// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector_map.h>
#include <wil\resource.h>

class XamlParserContext;
class XamlType;
struct XamlTypeToken;

namespace Parser
{
    // Forward declaration
    enum class StableXbfTypeIndex : UINT16;

    class XamlPredicateService final
    {
    public:
        // Crack a conditional xmlns into its two constituent substrings:
        // the base xmlns, and the string representing the conditional predicate
        static void CrackConditionalXmlns(
            _In_ const xstring_ptr& xmlns,
            _Out_ xstring_ptr& baseXmlns,
            _Out_ xstring_ptr& conditionalPredicateSubstring);

        // Crack a conditional predicate into the predicate's XamlType, and a
        // string containing the arguments
        static void CrackConditionalPredicate(
            _In_ const std::shared_ptr<XamlParserContext>& xamlContext,
            _In_ const xstring_ptr& conditionalPredicateSubstring,
            _Out_ std::shared_ptr<XamlType>& predicateType,
            _Out_ xstring_ptr& args);

        // Given the input predicate XamlType and argument string, evaluate it
        // and return the result. Results are cached.
        static bool EvaluatePredicate(
            _In_ const std::shared_ptr<XamlType>& predicateType,
            _In_ const xstring_ptr& args);

#if XCP_MONITOR
        static void ClearEvaluationCache();
#endif

    private:
        static BOOL CALLBACK InitOnceCallback(
            PINIT_ONCE initOnce,
            PVOID Parameter,
            PVOID *Context);
        static XamlPredicateService& GetInstance();

        XamlPredicateService();
        [[nodiscard]] wil::cs_leave_scope_exit Lock() {return m_lock.lock();}

        containers::vector_map<XamlTypeToken, std::shared_ptr<containers::vector_map<xstring_ptr, bool>>> m_predicateEvaluationCache;
        containers::vector_map<xstring_ptr, Parser::StableXbfTypeIndex> m_knownXamlPredicates;
        wil::critical_section m_lock; // Protect the above per instance memebers
    };
}