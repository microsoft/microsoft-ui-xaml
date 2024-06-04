// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IXamlSchemaObject.h"
#include "XamlTypeTokens.h"
#include "TypeBits.h"
#include "XamlQualifiedObject.h"

class XamlNamespace;
class XamlType;
class XamlSchemaContext;
class XamlProperty;
class XamlTextSyntax;

enum ImplicitPropertyType
{
    iptNone,
    iptInitialization,
    iptItems,
    iptPositionalParameters
};

class XamlProperty
    : public IXamlSchemaObject
{
public:
    XamlProperty() = default;

    XamlProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const XamlPropertyToken& sPropertyToken)
        : m_spSchemaContext(spSchemaContext)
        , m_sPropertyToken(sPropertyToken)
    {
    }

    XamlProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const XamlPropertyToken& sPropertyToken,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlType>& inPropertyType)
        : m_spSchemaContext(spSchemaContext)
        , m_sPropertyToken(sPropertyToken)
        , m_spName(inPropertyName)
        , m_spType(inPropertyType)
    {
    }

    XamlProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const XamlPropertyToken& sPropertyToken,
                const XamlTypeToken& sPropertyTypeToken,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlType>& inPropertyType)
        : m_spSchemaContext(spSchemaContext)
        , m_sPropertyToken(sPropertyToken)
        , m_spName(inPropertyName)
        , m_spType(inPropertyType)
        , m_sPropertyTypeToken(sPropertyTypeToken)
    {
    }

    XamlProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const XamlPropertyToken& sPropertyToken,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlType>& inUnknownDeclaringType)
        : m_spSchemaContext(spSchemaContext)
        , m_sPropertyToken(sPropertyToken)
        , m_spXamlNamespace(inNamespace)
        , m_spName(inPropertyName)
        , m_spDeclaringType(inUnknownDeclaringType)
    {
    }


    XamlProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const XamlPropertyToken& sPropertyToken,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlTextSyntax>& inXamlTextSyntax)
        : m_spSchemaContext(spSchemaContext)
        , m_sPropertyToken(sPropertyToken)
        , m_spName(inPropertyName)
        , m_spXamlTextSyntax(inXamlTextSyntax)
    {
    }

    XamlProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const XamlPropertyToken& sPropertyToken,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlType>& inPropertyType,
                const std::shared_ptr<XamlTextSyntax>& inXamlTextSyntax)
        : m_spSchemaContext(spSchemaContext)
        , m_sPropertyToken(sPropertyToken)
        , m_spXamlNamespace(inNamespace)
        , m_spName(inPropertyName)
        , m_spType(inPropertyType)
        , m_spXamlTextSyntax(inXamlTextSyntax)
    {
    }

    ~XamlProperty() override
    {
    }

    static bool AreEqual(const XamlProperty* first, const XamlProperty* second)
    {
        if (first == second) return true;
        if (!first || !second) return false;
        return first->get_PropertyToken() == second->get_PropertyToken();
    }

    static bool AreEqual(const std::shared_ptr<XamlProperty>& spFirst, const std::shared_ptr<XamlProperty>& spSecond)
    {
        return AreEqual(spFirst.get(), spSecond.get());
    }

    bool IsUnknown();
    bool IsPublic();
    bool IsBrowsable();
    bool IsReadOnly();
    bool IsStatic();
    bool IsAttachable();
    bool IsEvent();
    bool IsAmbient();
    virtual bool IsImplicit() const;
    virtual bool IsDirective() const;

    HRESULT get_Name(_Out_ xstring_ptr* pstrOutName);
    _Check_return_ HRESULT get_FullName(_Out_ xstring_ptr* pstrOutFullName);
    virtual _Check_return_ HRESULT get_DeclaringType(_Out_ std::shared_ptr<XamlType>& outDeclaringType);
    _Check_return_ HRESULT get_Type(_Out_ std::shared_ptr<XamlType>& outType);
    HRESULT get_TargetType(std::shared_ptr<XamlType>& outTargetType);
    HRESULT get_TextSyntax(std::shared_ptr<XamlTextSyntax>& outXamlTextSyntax);

    const XamlPropertyToken& get_PropertyToken() const
    {
        return m_sPropertyToken;
    }

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

    HRESULT DependsOn(std::shared_ptr<XamlProperty>& outDependsOn);

    _Check_return_ HRESULT GetValue(
        _In_ const XamlQualifiedObject& qoInstance,
        _Out_ XamlQualifiedObject& rqoOut);

    _Check_return_ HRESULT GetAmbientValue(
        _In_ const XamlQualifiedObject& qoInstance,
        _Out_ XamlQualifiedObject& rqoOut);

    _Check_return_ HRESULT SetValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
        _In_ bool bBindTemplates);

protected:
    void SetBoolPropertyBit(BoolPropertyBits fPropertyBit, bool bValue);
    void SetBoolPropertyBits(const XamlBitSet<BoolPropertyBits>& fPropertyBit, const XamlBitSet<BoolPropertyBits>& fValueBits);
    HRESULT GetBoolPropertyBit(BoolPropertyBits fPropertyBit, bool& fValue);
    HRESULT RetrieveBoolPropertyBits(const XamlBitSet<BoolPropertyBits>& fPropertyBits);
    HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext);

private:

    XamlBitSet<BoolPropertyBits> m_flagsBoolPropertyBits;
    XamlBitSet<BoolPropertyBits> m_flagsBoolPropertyValidBits;
    XamlPropertyToken m_sPropertyToken;
    XamlTypeToken m_sPropertyTypeToken;

    // Note: We need to prevent shared pointer cycles (otherwise we'll always
    // have strong references and never release the objects), so we make some of
    // the below pointers weak.  The general heuristic is to make "larger"
    // objects referencing "smaller" objects strong references with std::shared_ptr
    // and "smaller" objects back referencing "larger" objects weak references
    // using std::weak_ptr.  In this case, XamlNamespaces contain XamlTypes which
    // contain XamlProperties, so the references back to those classes from
    // XamlProperty are weak.

    std::weak_ptr<XamlSchemaContext> m_spSchemaContext;
    xstring_ptr m_spName;
    std::weak_ptr<XamlNamespace> m_spXamlNamespace;
    std::weak_ptr<XamlType> m_spDeclaringType;
    std::weak_ptr<XamlType> m_spType;
    std::shared_ptr<XamlTextSyntax> m_spXamlTextSyntax;
    static const size_t invalidIndex = static_cast<size_t>(-1);
    size_t m_uiRuntimeIndex = invalidIndex;
    //virtual IList<string> GetXmlNamespaces();
};

// TODO: Until we figure out the correct way to
// delegate the specialization through to the std::shared_ptr target type ...
template<>
struct DataStructureFunctionProvider< std::shared_ptr<XamlProperty> >
{
    static XUINT32 Hash(std::shared_ptr<XamlProperty>& data)
    {
        return DataStructureFunctionProvider<XamlPropertyToken>::Hash(data->get_PropertyToken());
    }

    static bool AreEqual(const std::shared_ptr<XamlProperty>& lhs, const std::shared_ptr<XamlProperty>& rhs)
    {
        return XamlProperty::AreEqual(lhs, rhs);
    }
};

class UnknownProperty final
    : public XamlProperty
{
private:
    // Keep a strong reference to any unknown declaring types.  The regular weak
    // reference m_spDeclaringType that we inherit from XamlProperty works on
    // the assumption that the schema context will always hold strong references
    // to the type and the property shouldn't because it creates potential
    // cycles (since types have strong references to their declared properties.)
    // Unfortunately UnknownType references aren't held by the schema context
    // and can be reclaimed between the XamlScanner that creates them and the
    // XamlPullParser that raises errors when we have an unknown type or
    // property.  We prevent that with a strong reference and override the
    // get_DeclaringType method to return this instance if m_spDeclaringType is
    // no longer available.
    std::shared_ptr<XamlType> m_spUnknownDeclaringType;

public:
    UnknownProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                XamlPropertyToken sPropertyToken,
                _In_ const xstring_ptr& inName,
                const std::shared_ptr<XamlType>& inAttachedOwnerType)
        : XamlProperty(
                    spSchemaContext,
                    sPropertyToken,
                    std::shared_ptr<XamlNamespace>(),
                    inName,
                    inAttachedOwnerType)
    {
    }

    // TODO: This parameter probably shouldn't be called attachedownertype
    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                _In_ const xstring_ptr& inName,
                const std::shared_ptr<XamlType>& inAttachedOwnerType,
                bool bIsAttachable,
                std::shared_ptr<UnknownProperty>& outProperty);

    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                _In_ const xstring_ptr& inName,
                const std::shared_ptr<XamlType>& inAttachedOwnerType,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                bool bIsAttachable,
                std::shared_ptr<UnknownProperty>& outProperty);

    // Override get_DeclaringType to return our private strong reference to any
    // unknown type if no value was provided by the base method.  See the
    // comment on m_spUnknownDeclaringType for more details.
    _Check_return_ HRESULT get_DeclaringType(_Out_ std::shared_ptr<XamlType>& outDeclaringType) override;
};


class ImplicitProperty final
    : public XamlProperty
{
public:
    ImplicitProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const ImplicitPropertyType& inImplicitPropertyType,
                XamlPropertyToken sPropertyToken,
                _In_ const xstring_ptr& inName,
                const std::shared_ptr<XamlType>& inPropertyType)
        : XamlProperty(
                    spSchemaContext,
                    sPropertyToken,
                    inName,
                    inPropertyType)
        , m_ImplicitPropertyType(inImplicitPropertyType)
    {
    }

    // TODO: need type for Object, and List<Object>
    // complete this method when those types are added.
    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const ImplicitPropertyType& inImplicitPropertyType,
                std::shared_ptr<ImplicitProperty>& outProperty);

    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const ImplicitPropertyType& inImplicitPropertyType,
                _In_ const xstring_ptr& inName,
                const std::shared_ptr<XamlType>& inPropertyType,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                bool bIsAttachable,
                std::shared_ptr<ImplicitProperty>& outProperty);

    bool IsImplicit() const override
    {
        return true;
    }

    ImplicitPropertyType get_ImplicitPropertyType() const
    {
        ASSERT(IsImplicit());
        return m_ImplicitPropertyType;
    }

private:
    ImplicitPropertyType m_ImplicitPropertyType;
};

class DirectiveProperty final : public XamlProperty
{
protected:
    DirectiveProperty(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                XamlPropertyToken sPropertyToken,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlType>& inPropertyType,
                const std::shared_ptr<XamlTextSyntax> inXamlTextSyntax)
        : XamlProperty(
                    spSchemaContext,
                    sPropertyToken,
                    inNamespace,
                    inPropertyName,
                    inPropertyType,
                    inXamlTextSyntax)
        , m_spDirectivePropertyType(inPropertyType)
    {
    }

public:

    static HRESULT Create(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                _In_ const xstring_ptr& inPropertyName,
                const std::shared_ptr<XamlType>& inPropertyType,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                const std::shared_ptr<XamlTextSyntax>& inXamlTextSyntax,
                XamlDirectives inDirective,
                std::shared_ptr<DirectiveProperty>& outProperty);

    bool IsDirective() const override { return true; }

    XamlDirectives get_DirectiveKind() const
    {
        ASSERT(get_PropertyToken().GetProviderKind() == tpkParser);
        return get_PropertyToken().GetDirectiveHandle();
    }

private:
    std::shared_ptr<XamlType> m_spDirectivePropertyType;
};

// TODO: Until we figure out the correct way to
// delegate the specialization through to the std::shared_ptr target type ...
template<>
struct DataStructureFunctionProvider< std::shared_ptr<DirectiveProperty> >
{
    static XUINT32 Hash(std::shared_ptr<DirectiveProperty>& data)
    {
        return DataStructureFunctionProvider<XamlPropertyToken>::Hash(data->get_PropertyToken());
    }

    static bool AreEqual(const std::shared_ptr<DirectiveProperty>& lhs, const std::shared_ptr<DirectiveProperty>& rhs)
    {
        return XamlProperty::AreEqual(lhs, rhs);
    }
};



