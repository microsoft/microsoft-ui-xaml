// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ObjectWriterStack.h"
#include "ObjectWriterFrame.h"
#include "CompressedStackCacheHint.h"
#include "XamlContext.h"
#include "XamlProperty.h"
#include "XamlBinaryMetadata.h"
#include <stack_vector.h>
#include <utility>

class XamlTextSyntax;
class XamlProperty;
class XamlSavedContext;
class XamlSchemaContext;
class ObjectWriter;
class XamlServiceProviderContext;
class XamlType;
class XamlNamespace;

class ObjectWriterContext final
    : public XamlContext
    , public std::enable_shared_from_this<ObjectWriterContext>
{
private:
    ObjectWriterStack m_ObjectWriterStack;
    std::shared_ptr<XamlQualifiedObject> m_qoRootInstance;
    std::shared_ptr<XamlQualifiedObject> m_spEventRoot;
    std::shared_ptr<XamlQualifiedObject> m_qoParentXBindConnector;
    // These four items are cached for perf reasons.
    std::shared_ptr<XamlType> m_spStyleXamlType;
    std::shared_ptr<XamlType> m_spControlTemplateXamlType;
    std::shared_ptr<XamlProperty> m_spStyleTargetTypeXamlProperty;
    std::shared_ptr<XamlProperty> m_spControlTemplateTargetTypeProperty;

    xref_ptr<INameScope> m_spRootNamescope;
    xref_ptr<IPALUri> m_spBaseUri;
    // Used by Binding Xaml debugging for the generic.xaml case in order to store the ResourceUri.
    xref_ptr<IPALUri> m_spXamlResourceUri;
    xstring_ptr m_xbfHash;

    XUINT32 m_uiSavedDepth;
    bool m_bExpandTemplates;
    bool m_hasPreviousObjectWriterContext;

public:
    ObjectWriterContext(
                _In_ std::shared_ptr<XamlSchemaContext> spSchemaContext,
                _In_ std::shared_ptr<XamlQualifiedObject> spEventRoot,
                bool bExpandTemplates,
                bool hasPreviousObjectWriterContext,
                _In_opt_ const std::shared_ptr<XamlQualifiedObject>& spParentXBindConnector)
        : XamlContext(spSchemaContext)
        , m_uiSavedDepth(0)
        , m_spEventRoot(spEventRoot)
        , m_bExpandTemplates(bExpandTemplates)
        , m_hasPreviousObjectWriterContext(hasPreviousObjectWriterContext)
        , m_qoParentXBindConnector(spParentXBindConnector)
    {
    }

    static HRESULT Create(
                _In_ const std::shared_ptr<XamlSchemaContext>& inSchemaContext,
                _In_ std::shared_ptr<XamlQualifiedObject> spEventRoot,
                _In_ bool bExpandTemplates,
                _In_opt_ const std::shared_ptr<XamlQualifiedObject>& spParentXBindConnector,
                _Out_ std::shared_ptr<ObjectWriterContext>& rspObjectWriterContext);
    static HRESULT Create(
                _In_ const std::shared_ptr<XamlSavedContext>& spXamlSavedContext,
                _In_ std::shared_ptr<XamlQualifiedObject> spEventRoot,
                _In_ bool bExpandTemplates,
                _In_opt_ const std::shared_ptr<XamlQualifiedObject>& spParentXBindConnector,
                _Out_ std::shared_ptr<ObjectWriterContext>& rspObjectWriterContext);

    ~ObjectWriterContext() override
    {
    }

    const ObjectWriterFrame& Current() const
    {
        return m_ObjectWriterStack.Current();
    }

    ObjectWriterFrame& Current()
    {
        return m_ObjectWriterStack.Current();
    }

    const ObjectWriterFrame& Parent() const
    {
        return m_ObjectWriterStack.Parent();
    }

    ObjectWriterFrame& Parent()
    {
        return m_ObjectWriterStack.Parent();
    }

    const std::shared_ptr<XamlType> GetStyleXamlType() const
    {
        return m_spStyleXamlType;
    }

    const std::shared_ptr<XamlProperty> GetStyleTargetTypeXamlProperty() const
    {
        return m_spStyleTargetTypeXamlProperty;
    }

    const std::shared_ptr<XamlType> GetControlTemplateXamlType() const
    {
        return m_spControlTemplateXamlType;
    }

    const std::shared_ptr<XamlProperty> GetControlTemplateTargetTypeXamlProperty() const
    {
        return m_spControlTemplateTargetTypeProperty;
    }

    const std::shared_ptr<XamlQualifiedObject> GetXBindParentConnector() const
    {
        return m_qoParentXBindConnector;
    }

    HRESULT ServiceProvider_ResolveXamlType(
        _In_ const xstring_ptr_view& spName,
        _Out_ std::shared_ptr<XamlType>& rspXamlType);

    HRESULT ServiceProvider_ResolveXamlType(
        _In_ const XamlTypeToken& typeToken,
        _Out_ std::shared_ptr<XamlType>& rspXamlType);

    HRESULT ServiceProvider_GetAllAmbientValues(
        _In_ const std::shared_ptr<XamlType>& spCeilingType,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlType>& spTypeToFind,
        _In_ CompressedStackCacheHint cacheHint,
        _Out_ AmbientValuesVector& vecValues);

    HRESULT ServiceProvider_GetAmbientValue(
        _In_ const std::shared_ptr<XamlType>& spCeilingType1,
        _In_ const std::shared_ptr<XamlType>& spCeilingType2,
        _In_ const std::shared_ptr<XamlProperty>& spProperty1,
        _In_ const std::shared_ptr<XamlProperty>& spProperty2,
        _In_ const std::shared_ptr<XamlType>& spTypeToFind,
        _In_ CompressedStackCacheHint cacheHint,
        _Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue );

    HRESULT EnsureTargetTypeCaches();

    // NB: Not on original interface.
    HRESULT ServiceProvider_GetAmbientTargetType(_Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue)
    {
        IFC_RETURN(EnsureTargetTypeCaches());

        // Note:  This is less that what WPF would be looking up.  If more is required, the the args to GetFirstAmbientProperty
        // would need to be change to lists ceilingType and/or property
        IFC_RETURN(ServiceProvider_GetAmbientValue(
                    m_spStyleXamlType,
                    m_spControlTemplateXamlType,
                    m_spStyleTargetTypeXamlProperty,
                    m_spControlTemplateTargetTypeProperty,
                    std::shared_ptr<XamlType>(),
                    CompressedStackCacheHint::TargetType,
                    rqoValue));
        return S_OK;
    }

    HRESULT AddNamespacePrefix(
        _In_ const xstring_ptr& inPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace) override;

    std::shared_ptr<XamlNamespace> FindNamespaceByPrefix(
        _In_ const xstring_ptr& inPrefix) override;

    std::shared_ptr<XamlServiceProviderContext> get_TextSyntaxContext();

    const std::shared_ptr<XamlServiceProviderContext> get_MarkupExtensionContext();

    void PushScope();
    void PopScope();
    bool IsStackEmpty() const;
    bool IsInConditionalScope(bool ignoreInactiveScopes) const;
    INT32 get_Depth() const;
    INT32 get_SavedDepth() const;
    INT32 get_LiveDepth() const;
    ObjectWriterStack::iterator get_StackBegin();
    ObjectWriterStack::iterator get_StackEnd();
    bool get_ExpandTemplates() const { return m_bExpandTemplates; }

    std::shared_ptr<XamlQualifiedObject> get_RootInstance();

    bool get_CurrentIsPropertyAssigned( _In_ const std::shared_ptr<XamlProperty>& inProperty) const;

    HRESULT GetSavedContext( _Out_ std::shared_ptr<XamlSavedContext>& rspSavedContext );

    xref_ptr<IPALUri> get_BaseUri()
    {
        return m_spBaseUri;
    }

    void set_BaseUri(_In_ xref_ptr<IPALUri> spBaseUri )
    {
        m_spBaseUri = spBaseUri;
    }

    void set_RootNamescope(_In_ xref_ptr<INameScope> rootNamescope)
    {
        m_spRootNamescope = rootNamescope;
    }

    xref_ptr<INameScope> get_RootNamescope() const
    {
        return m_spRootNamescope;
    }

    xref_ptr<IPALUri> get_XamlResourceUri()
    {
        return m_spXamlResourceUri;
    }

    void set_XamlResourceUri(_In_ xref_ptr<IPALUri> spXamlResourceUri )
    {
        m_spXamlResourceUri = spXamlResourceUri;
    }

    void set_XbfHash(_In_ xstring_ptr strHash)
    {
        m_xbfHash = strHash;
    }

    const xstring_ptr& get_XbfHash() const
    {
        return m_xbfHash;
    }

    // Get whether there is a previous service provider context (or whether this
    // context is in the top level parse).
    bool HasPreviousObjectWriterContext()
    {
        return (m_hasPreviousObjectWriterContext);
    }

private:

    // Ambient values can be retrieved from either:
    // - CDependencyObject (via ObjectWriterFrame's weakref instance)
    // - shared_ptr<XamlQualifiedObject> (via one of ObjectWriterFrame's instances)
    // - XamlQualifiedObject - (via GetValue)
    // This struct wraps all three options into one, and provides getters so that
    // callers who just want a DO can just get a DO, and callers who need a shared_ptr can get one
    // Previously, a shared_ptr was created for everybody, even if they were just going to throw it away
    // because they only needed a DO. This allows us to delay that allocation until we know it's actually needed.
    struct AmbientValue
    {
        xref_ptr<CDependencyObject> m_do;
        XamlQualifiedObject m_qo;
        std::shared_ptr<XamlQualifiedObject> m_pqo;
        explicit operator bool() const WI_NOEXCEPT
        {
            Validate();
            return m_do || !m_qo.IsUnset() || m_pqo;
        }

        void Validate() const WI_NOEXCEPT
        {
            ASSERT((!m_do && m_qo.IsUnset()) || (!m_do && !m_pqo) || (m_qo.IsUnset() && !m_pqo));
        }

        xref_ptr<CDependencyObject> GetDo() const;
        XamlQualifiedObject GetQo() const;
        std::shared_ptr<XamlQualifiedObject> GetQoPtr() const;

    };
    // Find the next instance of a set of ambient properties given the current
    // ObjectWriter stack.
    //
    // (Note that WPF passes iterators for the ceilings and the properties, but
    // we can't easily pass them on the stack so we've simplified things to
    // include the maximum number of each that we need.)
    //
    // NOTE: THIS METHOD IS UNABLE TO FIND AMBIENT VALUES ASSIGNED TO ATTACHED PROPERTIES
    _Check_return_ HRESULT FindNextAmbientValue(
                // Iterator of the object writer stack frames
                _Inout_ ObjectWriterStack::iterator& startingIterator,
                _In_ CompressedStackCacheHint cacheHint,
                // Optional types that will end our search if walk over them
                _In_opt_ const std::shared_ptr<XamlType>& spCeilingType1,
                _In_opt_ const std::shared_ptr<XamlType>& spCeilingType2,
                // The properties whose values we are looking for
                _In_opt_ const std::shared_ptr<XamlProperty>& spProperty1,
                _In_opt_ const std::shared_ptr<XamlProperty>& spProperty2,
                // Optionally get the values of any properties of this type
                _In_opt_ const std::shared_ptr<XamlType>& spTypeToMatch,
                // The next value of the ambient property
                _Out_ AmbientValue& outValue,
                // Whether we stopped searching because we reached a ceiling type
                _Out_ bool *pfStoppedOnCeiling );

    static HRESULT CheckAmbient(_In_ const std::shared_ptr<XamlProperty>& spXamlProperty );

    HRESULT GetStackIterator(
        ObjectWriterStack::iterator& outIterator,
        XUINT32 uiLevel );

    HRESULT GetStackIterator(
        ObjectWriterStack::const_iterator& outIterator,
        XUINT32 uiLevel ) const;

public:
    HRESULT Runtime_GetAmbientValue(
                _In_ const XamlQualifiedObject& qoInstance,
                _In_ const std::shared_ptr<XamlProperty>& spXamlProperty,
                _Out_ XamlQualifiedObject& rqoOut )
    {
        RRETURN(spXamlProperty->GetAmbientValue(qoInstance, rqoOut));
    }
};

