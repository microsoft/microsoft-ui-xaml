// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IXamlSchemaObject.h"
#include "TypeBits.h"
#include "XamlTypeTokens.h"

class XamlSchemaContext;
class XamlNamespace;
class XamlProperty;
class XamlTextSyntax;
class XamlServiceProviderContext;
struct XamlQualifiedObject;

// XamlType is an abstraction that keeps the parser decoupled from
// the underlying runtime. It adds a layer of indirection between
// Jupiter's type system, keeping the parser from needing deep knowledge
// about the runtime.
class XamlType
    : public IXamlSchemaObject
{

public:
    XamlType() = default;
    
    XamlType(
        const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        const std::shared_ptr<XamlNamespace>& spNamespace,
        const XamlTypeToken sTypeToken,
        _In_ const xstring_ptr& inTypeName)
        : m_spXamlSchemaContext(spSchemaContext)
        , m_spXamlNamespace(spNamespace)
        , m_spName(inTypeName)
        , m_sTypeToken(sTypeToken)
        , m_flagsBoolTypeBits(btbNone)
        , m_flagsBoolTypeValidBits(btbIsDirective) // We'll trust that the directive is always set properly
        , m_bHasContentProperty(TRUE)
    {}
        


    // Instantiates the actual object this abstraction represents. This is one of the key
    // bridges between the parser and the runtime.
    HRESULT CreateInstance(_Out_ std::shared_ptr<XamlQualifiedObject>& rqoOut);
    
    HRESULT CreateFromValue(
        _In_ std::shared_ptr<XamlServiceProviderContext> spTextSyntaxContext,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
        _In_ bool fIsPropertyAssignment,
        _Out_ std::shared_ptr<XamlQualifiedObject>& rqoOut
    );
    
    bool Equals(_In_ const std::shared_ptr<XamlType>& spOtherType)
    {
        return (spOtherType && (spOtherType->get_TypeToken() == m_sTypeToken));
    }

    static bool AreEqual(_In_ const std::shared_ptr<XamlType>& spFirst, _In_ const std::shared_ptr<XamlType>& spSecond)
    {
        if (spFirst == spSecond)
        {
            return true;
        }
        else if (!spFirst || !spSecond)
        {
            return false;
        }

        return (spFirst->get_TypeToken() == spSecond->get_TypeToken());
    }

    bool IsUnknown() const;
    HRESULT IsPublic(bool& outValue);
    HRESULT IsConstructible(bool& outValue);
    HRESULT IsString(bool& outValue);
    HRESULT IsCollection(bool& outValue);
    HRESULT IsDictionary(bool& outValue);
    HRESULT IsMarkupExtension(bool& outValue);
    HRESULT IsTemplate(bool& outValue);
    HRESULT IsDirective(bool& outValue);
    HRESULT TrimSurroundingWhitespace(bool& outValue);
    HRESULT IsWhitespaceSignificantCollection(bool& outValue);

    // TODO: does this really belong here?  It isn't 
    // a pure xaml concept
    HRESULT IsISupportInitialize(bool& outValue);
    
    HRESULT get_Name(_Out_ xstring_ptr* pstrOutName);
    
    // Get the full name of the type
    HRESULT get_FullName(_Out_ xstring_ptr* pstrOutFullName);
    
    HRESULT get_TextSyntax(std::shared_ptr<XamlTextSyntax>& outTextSyntax);
    HRESULT get_ContentProperty(std::shared_ptr<XamlProperty>& outContentProperty);

    // Collection types may optionally provide a type used to wrap literal
    // content from the parser before inserting into the collection.  The parser
    // won't actually wrap the content, but merely know it's safe to pass to the
    // collection which will handle it.
    HRESULT get_ContentWrapper(_Out_opt_ std::shared_ptr<XamlType>& spContentWrapperType);
    
    HRESULT get_RuntimeNameProperty(std::shared_ptr<XamlProperty>& outRuntimeNameProperty);
    HRESULT get_XmlLangProperty(std::shared_ptr<XamlProperty>& outXmlLangProperty);
    HRESULT get_DictionaryKeyProperty(std::shared_ptr<XamlProperty>& outDictionaryKeyProperty);
    HRESULT get_BaseType(std::shared_ptr<XamlType>& outBaseType);
    HRESULT get_MarkupExtensionReturnType(std::shared_ptr<XamlType>& outReturnType);
    HRESULT get_MarkupExtensionExpressionType(std::shared_ptr<XamlType>& outExpressionType);
    HRESULT get_CollectionItemType(std::shared_ptr<XamlType>& collectionItemType);

    const XamlTypeToken& get_TypeToken() const  
    { 
        return m_sTypeToken; 
    }
    
    HRESULT GetProperty(_In_ const xstring_ptr& inPropertyName, std::shared_ptr<XamlProperty>& outProperty);
    HRESULT GetDependencyProperty(_In_ const xstring_ptr& inPropertyName, std::shared_ptr<XamlProperty>& outProperty);
    HRESULT GetAttachableProperty(_In_ const xstring_ptr& inPropertyName, std::shared_ptr<XamlProperty>& outProperty);
    HRESULT GetXamlNamespace(std::shared_ptr<XamlNamespace>& outNamespace);
    HRESULT IsAssignableFrom(const std::shared_ptr<XamlType>& xamlType, _Out_ bool& bIsAssignableFrom);

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
    bool HasValidRuntimeIndex() const
    {
        return m_uiRuntimeIndex != invalidIndex;
    }

    void SetIsDirective();

private: 
    void SetBoolTypeBit(BoolTypeBits fTypeBit, bool bValue);
    void SetBoolTypeBits(const XamlBitSet<BoolTypeBits>& fTypeBit, const XamlBitSet<BoolTypeBits>& fValueBits);

    HRESULT GetBoolTypeBit(BoolTypeBits fTypeBit, bool& bValue);
    HRESULT RetrieveBoolTypeBits(const XamlBitSet<BoolTypeBits>& fTypeBits);

    HRESULT TryGetPropertyFromCache(_In_ const xstring_ptr& inPropertyName, XamlPropertyAndTypeToken& tPropertyAndType)
    {
        auto itPropertyAndType = m_PropertyCache.find(inPropertyName);
        if (itPropertyAndType == m_PropertyCache.end())
        {
            RRETURN(S_FALSE);
        }
        else{
            tPropertyAndType = itPropertyAndType->second;
            RRETURN(S_OK);
        }
    }

    HRESULT AddPropertyToCache(_In_ const xstring_ptr& inPropertyName, XamlPropertyAndTypeToken& tPropertyAndType)
    {
        auto result = m_PropertyCache.insert({ inPropertyName, tPropertyAndType });

        if (result.second) RRETURN(S_OK);

        auto itResult = result.first;
        itResult->second = tPropertyAndType;

        RRETURN(S_FALSE);
    }

    HRESULT TryGetDependencyPropertyFromCache(_In_ const xstring_ptr& inPropertyName, XamlPropertyAndTypeToken& tPropertyAndType)
    {
        auto itPropertyAndType = m_DependencyPropertyCache.find(inPropertyName);
        if (itPropertyAndType == m_DependencyPropertyCache.end())
        {
            RRETURN(S_FALSE);
        }
        else{
            tPropertyAndType = itPropertyAndType->second;
            RRETURN(S_OK);
        }
    }

    HRESULT AddDependencyPropertyToCache(_In_ const xstring_ptr& inPropertyName, XamlPropertyAndTypeToken& tPropertyAndType)
    {
        auto result = m_DependencyPropertyCache.insert({ inPropertyName, tPropertyAndType });

        if (result.second) RRETURN(S_OK);

        auto itResult = result.first;
        itResult->second = tPropertyAndType;

        RRETURN(S_FALSE);
    }

protected:
    HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext);

private:
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    std::shared_ptr<XamlTextSyntax> m_spXamlTextSyntax;
    xstring_ptr m_spName;
    xstring_ptr m_spFullName;     // The full name of the type (including its CLR namespace).
    std::shared_ptr<XamlProperty> m_spRuntimeName;
    std::shared_ptr<XamlProperty> m_spContentProperty;
    std::shared_ptr<XamlType> m_spContentWrapper;
    std::weak_ptr<XamlNamespace> m_spXamlNamespace;
    containers::vector_map<xstring_ptr, XamlPropertyAndTypeToken> m_PropertyCache;
    containers::vector_map<xstring_ptr, XamlPropertyAndTypeToken> m_DependencyPropertyCache;
    XamlBitSet<BoolTypeBits> m_flagsBoolTypeBits;
    XamlBitSet<BoolTypeBits> m_flagsBoolTypeValidBits;
    XamlTypeToken m_sTypeToken;

    // TODO: Use these flags for the other cacheable values such as
    // ContentProperty, RuntimeNameProperty, etc.
    XamlBitSet<NonBoolTypeValidBits> m_flagsNonBoolTypeValidBits;
    XamlTypeToken m_sBaseTypeToken; // The equivalent Core base type of this type.
    bool m_bHasContentProperty;
    
    static const size_t invalidIndex = static_cast<size_t>(-1);
    size_t m_uiRuntimeIndex = invalidIndex;
};

class KnownXamlType 
    : public XamlType
{

public:
    KnownXamlType(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const std::shared_ptr<XamlNamespace>& spNamespace,
                const XamlTypeToken sTypeToken,
                _In_ const xstring_ptr& inTypeName)
        : XamlType(
                    spSchemaContext, 
                    spNamespace, 
                    sTypeToken, 
                    inTypeName)
    {
    }

    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
                const std::shared_ptr<XamlNamespace>& spNamespace,
                _In_ const xstring_ptr& inTypeName,
                const XamlTypeToken sTypeToken,
                std::shared_ptr<XamlType>& outType);
    
    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
                const std::shared_ptr<XamlNamespace>& spNamespace,
                _In_ const xstring_ptr& inTypeName,
                std::shared_ptr<XamlType>& outType)
    {
        XamlTypeToken ttUnknown;
        RRETURN(Create(spSchemaContext, spNamespace, inTypeName, ttUnknown, outType));
    }
    
    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
                const XamlTypeToken sTypeToken,
                std::shared_ptr<XamlType>& outType)
    {
        std::shared_ptr<XamlNamespace> spNamespace;
        xstring_ptr spTypeName;
        RRETURN(Create(spSchemaContext, spNamespace, spTypeName, sTypeToken, outType));
    }
};

class UnknownType 
    : public XamlType
{
public:
    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
                const std::shared_ptr<XamlNamespace>& inNamespace,
                _In_ const xstring_ptr& inTypeName,
                std::shared_ptr<XamlType>& outType);

    UnknownType(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                const XamlTypeToken sTypeToken,
                _In_ const xstring_ptr& inTypeName)
        : XamlType(
                    spSchemaContext, 
                    inNamespace, 
                    sTypeToken, 
                    inTypeName)
    {
    }
};

