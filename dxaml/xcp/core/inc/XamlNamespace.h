// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once 

#include "IXamlSchemaObject.h"

class XamlProperty;
class DirectiveProperty;
class XamlType;
class XamlXmlNamespace;
class XamlParserContext;

namespace Parser
{
struct XamlPredicateAndArgs;
}

class XamlNamespace
    : public IXamlSchemaObject
    , public std::enable_shared_from_this<XamlNamespace>
{
protected:
    XamlNamespace() = default;
    XamlNamespace(size_t uiRuntimeIndex)
        : m_uiRuntimeIndex(uiRuntimeIndex)
        , m_conditionalPredicateString(xstring_ptr::EmptyString())
    {
    }
    XamlNamespace(xstring_ptr conditionalPredicate)
        : m_conditionalPredicateString(conditionalPredicate)
    {
    }
    XamlNamespace(size_t uiRuntimeIndex, xstring_ptr conditionalPredicate)
        : m_uiRuntimeIndex(uiRuntimeIndex)
        , m_conditionalPredicateString(conditionalPredicate)
    {
    }
    
public:
    ~XamlNamespace() override {}

    xstring_ptr get_TargetNamespace() const;

    virtual bool get_IsResolved() const = 0;

    // TODO: verify that we will be able to do without the typeArgs.  This seems to be used for Templates
    // virtual _Check_return_ HRESULT GetXamlType(_In_ const xstring_ptr_view& inTypeName, XamlType[]typeArgs, _Out_ std::shared_ptr<XamlType& outType) = 0;
    virtual _Check_return_ HRESULT GetXamlType(
        _In_ const xstring_ptr& inTypeName, 
        _Out_ std::shared_ptr<XamlType>& outType) = 0;

    // TODO: verify that this isn't needed.  Seems to be used for reflection
    // virtual _Check_return_ HRESULT GetAllXamlTypes(_Out_ IEnumerable<XamlType>** ppOut) = 0;

    virtual _Check_return_ HRESULT GetDirectiveType(
        _In_ const xstring_ptr& inTypeName, 
        _Out_ std::shared_ptr<XamlType>& outType) = 0;

    // TODO: verify that this isn't needed.  Seems to be used for reflection
    // virtual _Check_return_ HRESULT GetAllDirectiveTypes(_Out_ IEnumerable<XamlType>** ppOut) = 0;

    virtual _Check_return_ HRESULT GetDirectiveProperty(
        _In_ const xstring_ptr_view& inPropertyName, 
        _Out_ std::shared_ptr<DirectiveProperty>& ppOut) = 0;

    // Compares two XamlNamespaces by their TargetNamespace.
    // Presence (or absence) of a conditional predicate is disregarded,
    // since that has no effect on whether two XamlNamespaces map to the same thing
    virtual bool IsEqual(const XamlNamespace& rhs) const
    {
        return rhs.get_TargetNamespace().Equals(get_TargetNamespace());
    }

    virtual XamlXmlNamespace* AsXamlXmlNamespace() { return nullptr; }

    virtual std::shared_ptr<XamlNamespace> get_OriginalXamlXmlNamespace() { return nullptr; }

    // TODO: verify that this isn't needed.  Seems to be used for reflection
    // virtual _Check_return_ HRESULT GetAllDirectiveProperties(_Out_ IEnumerable<XamlProperty>** ppOut) = 0;

    //  this is for general MarkupExtension handling:  It just takes 
    // the type-name and appends "Extension" to it.
    _Check_return_ HRESULT GetTypeExtensionName(
        _In_ const xstring_ptr& inTypeName, 
        _Out_ xstring_ptr* pstrOutName);

    void ResolveConditionalPredicate(_In_ const std::shared_ptr<XamlParserContext>& xamlContext);
    
    void SetRuntimeIndex(size_t uiRuntimeIndex)
    {
        ASSERT(!HasValidRuntimeIndex());
        m_uiRuntimeIndex = uiRuntimeIndex;
    }
    size_t get_RuntimeIndex() const
    {
        ASSERT(HasValidRuntimeIndex());
        return m_uiRuntimeIndex;
    }

    bool HasValidRuntimeIndex() const { return m_uiRuntimeIndex != invalidIndex; }
    bool IsConditional() const 
    { 
        return !m_conditionalPredicateString.IsNullOrEmpty(); 
    }
    const auto& get_XamlPredicateAndArgs() const { return m_xamlPredicateAndArgs; }

    // Note that this will reset the namespace to the "unresolved conditional predicate" state
    void set_ConditionalPredicateString(const xstring_ptr& conditionalPredicateString)
    { 
        m_conditionalPredicateString = conditionalPredicateString;
        m_resolvedConditionalPredicate = false;
    }

    virtual std::shared_ptr<XamlNamespace> Clone() const = 0;

protected:
    virtual xstring_ptr get_TargetNamespaceCore() const = 0;

    static const size_t invalidIndex = static_cast<size_t>(-1);

    std::shared_ptr<Parser::XamlPredicateAndArgs> m_xamlPredicateAndArgs;
    xstring_ptr m_conditionalPredicateString;
    size_t m_uiRuntimeIndex = invalidIndex;
    bool m_resolvedConditionalPredicate = false;
};

